/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2026 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                  David Korn <dgk@research.att.com>                   *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*         hyenias <58673227+hyenias@users.noreply.github.com>          *
*                                                                      *
***********************************************************************/
/*
 * AT&T Labs
 *
 */

#define putenv	___putenv

#include	"shopt.h"
#include	"defs.h"
#include	"variables.h"
#include	"path.h"
#include	"lexstates.h"
#include	"timeout.h"
#include	"FEATURE/externs"
#include	"streval.h"

#define NVCACHE		8	/* must be a power of 2 */
static char	*savesub = 0;
static Namval_t	NullNode;
static Dt_t	*Refdict;
static Dtdisc_t	_Refdisc =
{
	offsetof(struct Namref,np),sizeof(struct Namval_t*),sizeof(struct Namref)
};

static void	pushnam(Namval_t*,void*);
static char	*staknam(Namval_t*, char*);
static void	rightjust(char*, int, int);
static char	*lastdot(char*, int);

/*
 * The first two fields must correspond with those in 'struct adata' in nvdisc.c and 'struct tdata' in typeset.c
 * (those fields are used via a type conversion in scanfilter() in name.c)
 */
struct adata
{
	Namval_t	*tp;
	char		*mapname;
	char		**argnam;
};

/* for a 'typeset -T' type */
struct sh_type
{
	void		*previous;
	Namval_t	**nodes;
	Namval_t	*rp;
	short		numnodes;
	short		maxnodes;
};

#if NVCACHE
    struct Namcache
    {
	struct Cache_entry
	{
		Dt_t		*root;
		Dt_t		*last_root;
		char		*name;
		Namval_t	*np;
		Namval_t	*last_table;
		Namval_t	*namespace;
		int		flags;
		short		size;
		short		len;
	} entries[NVCACHE];
	short		index;
	short		ok;
    };
    static struct Namcache nvcache;
#endif

char		nv_local = 0;
static void(*nullscan)(Namval_t*,void*);

/* ========	name value pair routines	======== */

#include	"shnodes.h"
#include	"builtins.h"

static char *getbuf(size_t len)
{
	static char *buf;
	static size_t buflen;
	if(buflen < len)
		buf = sh_realloc(buf, buflen = len);
	return buf;
}

/*
 * output variable name in format for re-input
 */
void nv_outname(Sfio_t *out, char *name, int len)
{
	const char *cp=name, *sp;
	int c, offset = stktell(sh.stk);
	while(sp= strchr(cp,'['))
	{
		if(len>0 && cp+len <= sp)
			break;
		sfwrite(out,cp,++sp-cp);
		stkseek(sh.stk,offset);
		while(c= *sp++)
		{
			if(c==']')
				break;
			else if(c=='\\')
			{
				if(*sp=='[' || *sp==']' || *sp=='\\')
					c = *sp++;
			}
			sfputc(sh.stk,c);
		}
		sfputc(sh.stk,0);
		sfputr(out,sh_fmtq(stkptr(sh.stk,offset)),-1);
		if(len>0)
		{
			sfputc(out,']');
			return;
		}
		cp = sp-1;
	}
	if(*cp)
	{
		if(len>0)
			sfwrite(out,cp,len);
		else
			sfputr(out,cp,-1);
	}
	stkseek(sh.stk,offset);
}

/*
 * for 'typeset -T' types
 */
Namval_t *nv_addnode(Namval_t* np, int remove)
{
	struct sh_type	*sp = (struct sh_type*)sh.mktype;
	int		i;
	char		*name=0;
	if(sp->numnodes==0 && !nv_isnull(np) && sh.last_table)
	{
		/* could be an redefine */
		Dt_t *root = nv_dict(sh.last_table);
		sp->rp = np;
		nv_delete(np,root,NV_NOFREE);
		np = nv_search(sp->rp->nvname,root,NV_ADD);
	}
	if(sp->numnodes && strncmp(np->nvname,NV_CLASS,sizeof(NV_CLASS)-1))
	{
		name = (sp->nodes[0])->nvname;
		i = strlen(name);
		if(strncmp(np->nvname,name,i))
			return np;
	}
	if(sp->rp && sp->numnodes)
	{
		/* check for a redefine */
		if(name && np->nvname[i]=='.' && np->nvname[i+1]=='_' && np->nvname[i+2]==0)
			sp->rp = 0;
		else
		{
			Dt_t *root = nv_dict(sh.last_table);
			nv_delete(sp->nodes[0],root,NV_NOFREE);
			dtinsert(root,sp->rp);
			errormsg(SH_DICT,ERROR_exit(1),e_redef,sp->nodes[0]->nvname);
			UNREACHABLE();
		}
	}
	for(i=0; i < sp->numnodes; i++)
	{
		if(np == sp->nodes[i])
		{
			if(remove)
			{
				while(++i < sp->numnodes)
					sp->nodes[i-1] = sp->nodes[i];
				sp->numnodes--;
			}
			return np;
		}
	}
	if(remove)
		return np;
	if(sp->numnodes==sp->maxnodes)
	{
		sp->maxnodes += 20;
		sp->nodes = (Namval_t**)sh_realloc(sp->nodes,sizeof(Namval_t*)*sp->maxnodes);
	}
	sp->nodes[sp->numnodes++] = np;
	return np;
}

/*
 * Perform parameter assignment for a linked list of parameters
 * <flags> contains attributes for the parameters
 */
void nv_setlist(struct argnod *arg,int flags, Namval_t *typ)
{
	char		*cp;
	Namval_t	*np, *mp;
	char		*eqp;
	char		*trap=sh.st.trap[SH_DEBUGTRAP];
	char		*prefix = sh.prefix;
	int		traceon = (sh_isoption(SH_XTRACE)!=0);
	int		array = (flags&(NV_ARRAY|NV_IARRAY));
	Namarr_t	*ap;
	Namval_t	node;
	struct Namref	nr;
	int		maketype = flags&NV_TYPE;  /* make a 'typeset -T' type definition command */
	struct sh_type	shtp;
	Dt_t		*vartree, *save_vartree = NULL;
#if SHOPT_NAMESPACE
	Namval_t	*save_namespace = NULL;
#endif
	if(flags&NV_GLOBAL)
	{
		save_vartree = sh.var_tree;
		sh.var_tree = sh.var_base;
#if SHOPT_NAMESPACE
		save_namespace = sh.namespace;
		sh.namespace = NULL;
#endif
	}
	if(maketype)
	{
		shtp.previous = sh.mktype;
		sh.mktype=&shtp;
		shtp.numnodes=0;
		shtp.maxnodes = 20;
		shtp.rp = 0;
		shtp.nodes = (Namval_t**)sh_malloc(shtp.maxnodes*sizeof(Namval_t*));
	}
#if SHOPT_NAMESPACE
	if(sh.namespace && nv_dict(sh.namespace)==sh.var_tree)
		flags |= NV_NOSCOPE;
#endif /* SHOPT_NAMESPACE */
	flags &= ~(NV_TYPE|NV_ARRAY|NV_IARRAY);
	if(sh.prefix)
	{
		flags &= ~(NV_IDENT|NV_EXPORT);
		flags |= NV_VARNAME;
	}
	else
		sh.prefix_root = sh.first_root = 0;
	vartree = sh.prefix_root ? sh.prefix_root : sh.var_tree;
	for(;arg; arg=arg->argnxt.ap)
	{
		sh.used_pos = 0;
		if(arg->argflag&ARG_MAC)
		{
			sh.prefix = 0;
			cp = sh_mactrim(arg->argval,(flags&NV_NOREF)?-3:-1);
			sh.prefix = prefix;
		}
		else
		{
			stkseek(sh.stk,0);
			if(*arg->argval==0 && arg->argchn.ap && !(arg->argflag&~(ARG_APPEND|ARG_QUOTED|ARG_MESSAGE|ARG_ARRAY)))
			{
				int flag = (NV_VARNAME|NV_ARRAY|NV_ASSIGN);
				int sub=0;
				struct fornod *fp=(struct fornod*)arg->argchn.ap;
				Shnode_t *tp=fp->fortre;
				flag |= (flags&(NV_NOSCOPE|NV_STATIC|NV_FARRAY));
				if(arg->argflag&ARG_ARRAY)
					array |= NV_IARRAY;
				if(arg->argflag&ARG_QUOTED)
					cp = sh_mactrim(fp->fornam,-1);
				else
					cp = fp->fornam;
				/* Do not allow 'typeset -T' to override special built-ins -- however, exclude
				 * previously created type commands from this search as that is handled elsewhere. */
				if(maketype && (np=nv_search(cp,sh.bltin_tree,0)) && nv_isattr(np,BLT_SPC) && !nv_search(cp,sh.typedict,0))
				{
					errormsg(SH_DICT,ERROR_exit(1),"%s:%s",cp,is_spcbuiltin);
					UNREACHABLE();
				}
				error_info.line = fp->fortyp-sh.st.firstline;
				if(!array && tp->tre.tretyp!=TLST && tp->com.comset && !tp->com.comarg.ap && tp->com.comset->argval[0]==0 && tp->com.comset->argval[1]=='[')
					array |= (tp->com.comset->argflag&ARG_MESSAGE)?NV_IARRAY:NV_ARRAY;
				if(prefix && tp->com.comset && *cp=='[')
				{
					sh.prefix = 0;
					np = nv_open(prefix,sh.last_root,flag);
					sh.prefix = prefix;
					if(np)
					{
						if(nv_isvtree(np) && !nv_isarray(np))
						{
							sfputc(sh.stk,'.');
							sfputr(sh.stk,cp,-1);
							cp = stkfreeze(sh.stk,1);
						}
					}
				}
				np = nv_open(cp,sh.var_tree,flag|NV_ASSIGN);
				if ( (arg->argflag & ARG_APPEND) &&
				     (tp->tre.tretyp & COMMSK)==TCOM &&
				     tp->com.comset &&
				     !nv_isvtree(np) &&
				     ( ( (ap = nv_arrayptr(np)) &&
				         !ap->fun &&
				         !nv_opensub(np)
				       ) ||
				       ( !ap &&
					 nv_isarray(np) &&
					 tp->com.comarg.ap &&
					 ! ( (mp = nv_search(tp->com.comarg.ap->argval,sh.fun_tree,0)) &&
				             nv_isattr(mp,BLT_DCL)
				           )
				       )
				     )
				   )
				{
					if(tp->com.comarg.ap)
					{
						struct argnod *ap = tp->com.comset;
						while(ap->argnxt.ap)
							ap = ap->argnxt.ap;
						ap->argnxt.ap = tp->com.comarg.ap;
					}
					tp->com.comarg.ap = tp->com.comset;
					tp->com.comset = 0;
					tp->com.comtyp = COMSCAN;
				}
				if(nv_isattr(np,NV_RDONLY) && np->nvfun && !(flags&NV_RDONLY))
				{
					errormsg(SH_DICT,ERROR_exit(1),e_readonly, nv_name(np));
					UNREACHABLE();
				}
				if(nv_isattr(np,NV_NOFREE) && nv_isnull(np))
					nv_offattr(np,NV_NOFREE);
				if(nv_istable(np))
					nv_unset(np,0);
				if(typ && !array  && (!sh.prefix || nv_isnull(np) || nv_isarray(np)))
				{
					if(!(nv_isnull(np)) && !nv_isarray(np))
						nv_unset(np,0);
					 nv_settype(np,typ,0);
				}
				if((flags&NV_STATIC) && !nv_isattr(np,NV_EXPORT) && !nv_isnull(np))
					goto check_type;
				ap=nv_arrayptr(np);
#if SHOPT_FIXEDARRAY
				if(ap && ap->fixed)
					flags |= NV_FARRAY;
#endif /* SHOPT_FIXEDARRAY */
				if(array && (!ap || !ap->hdr.type))
				{
#if SHOPT_FIXEDARRAY
					if(!(arg->argflag&ARG_APPEND) && (!ap || !ap->fixed))
#else
					if(!(arg->argflag&ARG_APPEND))
#endif /* SHOPT_FIXEDARRAY */
						nv_unset(np,NV_EXPORT);
					if(array&NV_ARRAY)
					{
						nv_setarray(np,nv_associative);
						if(typ)
							nv_settype(np,typ,0);
					}
					else
					{
						nv_onattr(np,NV_ARRAY);
					}
				}
				if(array && tp->tre.tretyp!=TLST && !tp->com.comset && !tp->com.comarg.ap)
					goto check_type;
				/* check for array assignment */
				if(tp->tre.tretyp!=TLST && tp->com.comarg.ap && !tp->com.comset && ((array&NV_IARRAY) || !((mp=tp->com.comnamp) && nv_isattr(mp,BLT_DCL))))
				{
					int argc;
					Dt_t	*last_root = sh.last_root;
					char **argv = sh_argbuild(&argc,&tp->com,0);
					sh.last_root = last_root;
					if(sh.mktype && sh.dot_depth==0 && np==((struct sh_type*)sh.mktype)->nodes[0])
					{
						sh.mktype = 0;
						errormsg(SH_DICT,ERROR_exit(1),"%s: not a known type name",argv[0]);
						UNREACHABLE();
					}
					if(!(arg->argflag&ARG_APPEND))
					{
#if SHOPT_FIXEDARRAY
						if(!nv_isarray(np) || ((ap=nv_arrayptr(np)) && !ap->fixed && (ap->nelem&ARRAY_MASK)))
#else
						if(!nv_isarray(np) || ((ap=nv_arrayptr(np)) && (ap->nelem&ARRAY_MASK)))
#endif /* SHOPT_FIXEDARRAY */
						{
							if(ap)
								ap->nelem |= ARRAY_UNDEF;
							nv_unset(np,NV_EXPORT);
						}
					}
					nv_setvec(np,(arg->argflag&ARG_APPEND),argc,argv);
					if(traceon || trap)
					{
						int n = -1;
						char *name = nv_name(np);
						if(arg->argflag&ARG_APPEND)
							n = '+';
						if(trap)
							sh_debug(trap,name,NULL,argv,(arg->argflag&ARG_APPEND)|ARG_ASSIGN);
						if(traceon)
						{
							sh_trace(NULL,0);
							sfputr(sfstderr,name,n);
							sfwrite(sfstderr,"=( ",3);
							while(cp= *argv++)
								sfputr(sfstderr,sh_fmtq(cp),' ');
							sfwrite(sfstderr,")\n",2);
						}
					}
					goto check_type;
				}
				if((tp->tre.tretyp&COMMSK)==TFUN)
					goto skip;
				if(tp->tre.tretyp==0 && !tp->com.comset && !tp->com.comarg.dp)
				{
					if(!(arg->argflag&ARG_APPEND))
					{
						if(ap && array_elem(ap)>0)
						{
							nv_putsub(np, NULL, ARRAY_SCAN);
							if(!ap->fun && !(ap->nelem&ARRAY_TREE) && !np->nvfun->next && !nv_type(np))
							{
								unsigned short nvflag = np->nvflag;
								uint32_t nvsize = np->nvsize;
								nv_unset(np,NV_EXPORT);
								np->nvflag = nvflag;
								np->nvsize = nvsize;
							}
							else
							{
								ap->nelem++;
								while(1)
								{
									ap->nelem &= ~ARRAY_SCAN;
									nv_unset(np,NV_EXPORT);
									ap->nelem |= ARRAY_SCAN;
									if(!nv_nextsub(np))
										break;
								}
								ap->nelem--;
							}
						}
						else if(nv_isattr(np,NV_BINARY|NV_NOFREE|NV_RAW)!=(NV_BINARY|NV_NOFREE|NV_RAW) && !nv_isarray(np))
							nv_unset(np,NV_EXPORT);
					}
					goto skip;
				}
				if(tp->tre.tretyp==TLST || !tp->com.comset || tp->com.comset->argval[0]!='[')
				{
					if(tp->tre.tretyp!=TLST && !tp->com.comnamp && tp->com.comset && tp->com.comset->argval[0]==0 && tp->com.comset->argchn.ap)
					{
						if(prefix || np)
							cp = stkcopy(sh.stk,nv_name(np));
						sh.prefix = cp;
						if(tp->com.comset->argval[1]=='[')
						{
							if((arg->argflag&ARG_APPEND) && (!nv_isarray(np) || (nv_aindex(np)>=0)))
								nv_unset(np,0);
							if(!(array&NV_IARRAY) && !(tp->com.comset->argflag&ARG_MESSAGE))
								nv_setarray(np,nv_associative);
						}
						nv_setlist(tp->com.comset,flags&~NV_STATIC,0);
						sh.prefix = prefix;
						if(tp->com.comset->argval[1]!='[')
							 nv_setvtree(np);
						goto check_type;
					}
					if(*cp!='.' && *cp!='[' && strchr(cp,'['))
					{
						cp = stkcopy(sh.stk,nv_name(np));
						if(!(arg->argflag&ARG_APPEND))
							flag &= ~NV_ARRAY;
						if(sh.prefix_root = sh.first_root)
							vartree = sh.prefix_root;
						np = nv_open(cp,vartree,flag);
					}
					if(arg->argflag&ARG_APPEND)
					{
						if(nv_isarray(np))
						{
							if((sub=nv_aimax(np)) < 0  && nv_arrayptr(np))
							{
								errormsg(SH_DICT,ERROR_exit(1),e_badappend,nv_name(np));
								UNREACHABLE();
							}
							if(sub>=0)
								sub++;
						}
						if(!nv_isnull(np) && np->nvalue!=Empty && !nv_isvtree(np))
							sub=1;
					}
					else if(((np->nvalue && np->nvalue!=Empty)||nv_isvtree(np)|| nv_arrayptr(np)) && !nv_type(np))
					{
						int was_assoc_array = ap && ap->fun;
						nv_unset(np,NV_EXPORT);  /* this can free ap */
						if(was_assoc_array)
							 nv_setarray(np,nv_associative);
					}
				}
				else
				{
					if(!(arg->argflag&ARG_APPEND))
						nv_unset(np,NV_EXPORT);
					if(!(array&NV_IARRAY) && !nv_isarray(np))
						nv_setarray(np,nv_associative);
				}
			skip:
				if(sub>0)
				{
					sfprintf(sh.stk,"%s[%d]",prefix?nv_name(np):cp,sub);
					sh.prefix = stkfreeze(sh.stk,1);
					nv_putsub(np,NULL,ARRAY_ADD|ARRAY_FILL|sub);
				}
				else if(prefix)
					sh.prefix = stkcopy(sh.stk,nv_name(np));
				else
					sh.prefix = cp;
				sh.last_table = 0;
				if(sh.prefix)
				{
					if(*sh.prefix=='_' && sh.prefix[1]=='.' && nv_isref(L_ARGNOD))
					{
						struct Namref *nrp = L_ARGNOD->nvalue;
						sfprintf(sh.stk,"%s%s",nv_name(nrp->np),sh.prefix+1);
						sh.prefix = stkfreeze(sh.stk,1);
					}
					memset(&nr,0,sizeof(nr));
					memcpy(&node,L_ARGNOD,sizeof(node));
					L_ARGNOD->nvalue = &nr;
					nr.np = np;
					nr.root = sh.last_root;
					nr.table = sh.last_table;
					L_ARGNOD->nvflag = NV_REF|NV_NOFREE;
					L_ARGNOD->nvfun = 0;
				}
				sh_exec(tp,sh_isstate(SH_ERREXIT));
				if(sh.prefix)
				{
					L_ARGNOD->nvalue = node.nvalue;
					L_ARGNOD->nvflag = node.nvflag;
					L_ARGNOD->nvfun = node.nvfun;
				}
				sh.prefix = prefix;
				if(nv_isarray(np) && (mp=nv_opensub(np)))
					np = mp;
				while(tp->tre.tretyp==TLST)
				{
					if(!tp->lst.lstlef || !tp->lst.lstlef->tre.tretyp==TCOM || tp->lst.lstlef->com.comarg.ap || tp->lst.lstlef->com.comset && tp->lst.lstlef->com.comset->argval[0]!='[')
						break;
					tp = tp->lst.lstrit;

				}
				if(!nv_isarray(np) && !typ && (tp->com.comarg.ap || !tp->com.comset || tp->com.comset->argval[0]!='['))
				{
					nv_setvtree(np);
					if(tp->com.comarg.ap || tp->com.comset)
						np->nvfun->dsize = 0;
				}
				goto check_type;
			}
			cp = arg->argval;
			mp = 0;
		}
		/*
		 * Check if parent scope attributes need to be copied to the
		 * temporary local scope of an assignment preceding a command.
		 * https://github.com/ksh93/ksh/issues/465
		 */
		if((flags&(NV_EXPORT|NV_NOSCOPE))==(NV_EXPORT|NV_NOSCOPE) && dtvnext(vartree) && (eqp = strchr(cp,'=')))
		{
			*eqp = '\0';
			if((np = nv_search(cp,dtvnext(vartree),0)) && np->nvflag)
			{
				Namfun_t *fp;
				mp = nv_search(cp,vartree,NV_ADD|NV_NOSCOPE);
				mp->nvname = np->nvname;  /* put_lang() (init.c) compares nvname pointers */
				mp->nvflag = np->nvflag;
				mp->nvsize = np->nvsize;
				if (fp = nv_enforcedisc(np))
					nv_stack(mp, fp);
			}
			*eqp = '=';
		}
		/*
		 * Perform the assignment
		 */
		np = nv_open(cp,vartree,flags);
		if(!np->nvfun && (flags&NV_NOREF))
		{
			if(sh.used_pos)
				nv_onattr(np,NV_PARAM);
			else
				nv_offattr(np,NV_PARAM);
		}
		if(traceon || trap)
		{
			char *sp=cp;
			char *name=nv_name(np);
			char *sub=0;
			int append = 0;
			if(nv_isarray(np))
				sub = savesub;
			if(cp=lastdot(sp,'='))
			{
				if(cp[-1]=='+')
					append = ARG_APPEND;
				cp++;
			}
			if(traceon)
			{
				sh_trace(NULL,0);
				nv_outname(sfstderr,name,-1);
				if(sub)
					sfprintf(sfstderr,"[%s]",sh_fmtq(sub));
				if(cp)
				{
					if(append)
						sfputc(sfstderr,'+');
					sfprintf(sfstderr,"=%s\n",sh_fmtq(cp));
				}
			}
			if(trap)
			{
					char *av[2];
					av[0] = cp;
					av[1] = 0;
					sh_debug(trap,name,sub,av,append);
			}
		}
	check_type:
		if(maketype)
		{
			nv_open(shtp.nodes[0]->nvname,sh.var_tree,NV_ASSIGN|NV_VARNAME|NV_NOADD|NV_NOFAIL);
			np = nv_mktype(shtp.nodes,shtp.numnodes);
			free(shtp.nodes);
			sh.mktype = shtp.previous;
			maketype = 0;
			if(sh.namespace)
				free(sh.prefix);
			sh.prefix = 0;
			if(nr.np == np)
			{
				L_ARGNOD->nvalue = node.nvalue;
				L_ARGNOD->nvflag = node.nvflag;
				L_ARGNOD->nvfun = node.nvfun;
			}
		}
		/* continue loop */
	}
	if(flags&NV_GLOBAL)
	{
		sh.var_tree = save_vartree;
#if SHOPT_NAMESPACE
		sh.namespace = save_namespace;
#endif
	}
}

/*
 * copy the subscript onto the stack
 */
static void stak_subscript(const char *sub, int last)
{
	int c;
	sfputc(sh.stk,'[');
	while(c= *sub++)
	{
		if(c=='[' || c==']' || c=='\\')
			sfputc(sh.stk,'\\');
		sfputc(sh.stk,c);
	}
	sfputc(sh.stk,last);
}

/*
 * construct a new name from a prefix and base name on the stack
 */
static char *copystack(const char *prefix, const char *name, const char *sub)
{
	int last=0,offset = stktell(sh.stk);
	if(prefix)
	{
		sfputr(sh.stk,prefix,-1);
		if(*stkptr(sh.stk,stktell(sh.stk)-1)=='.')
			stkseek(sh.stk,stktell(sh.stk)-1);
		if(*name=='.' && name[1]=='[')
			last = stktell(sh.stk)+2;
		if(*name!='['  && *name!='.' && *name!='=' && *name!='+')
			sfputc(sh.stk,'.');
		if(*name=='.' && (name[1]=='=' || name[1]==0))
			sfputc(sh.stk,'.');
	}
	if(last)
	{
		sfputr(sh.stk,name,-1);
		if(sh_checkid(stkptr(sh.stk,last),NULL))
			stkseek(sh.stk,stktell(sh.stk)-2);
	}
	if(sub)
		stak_subscript(sub,']');
	if(!last)
		sfputr(sh.stk,name,-1);
	sfputc(sh.stk,0);
	return stkptr(sh.stk,offset);
}

/*
 * grow this stack string <name> by <n> bytes and move from cp-1 to end
 * right by <n>.  Returns beginning of string on the stack
 */
static char *stack_extend(const char *cname, char *cp, int n)
{
	char *name = (char*)cname;
	int offset = name - stkptr(sh.stk,0);
	int m = cp-name;
	stkseek(sh.stk,offset + strlen(name)+n+1);
	name = stkptr(sh.stk,offset);
	cp =  name + m;
	m = strlen(cp)+1;
	while(m-->0)
		cp[n+m]=cp[m];
	return (char*)name;
}

Namval_t *nv_create(const char *name,  Dt_t *root, int flags, Namfun_t *dp)
{
	char			*sub=0, *cp=(char*)name, *sp, *xp;
	int			c;
	Namval_t		*np=0, *nq=0;
	Namfun_t		*fp=0;
	long			mode, add=0;
	int			copy=0,isref,top=0,noscope=(flags&NV_NOSCOPE);
	int			nofree=0, level=0;
	Namarr_t		*ap;
	if(root==sh.var_tree)
	{
		if(dtvnext(root))
			top = 1;
		else
			flags &= ~NV_NOSCOPE;
	}
	if(!dp->disc)
		copy = dp->nofree&1;
	if(*cp=='.')
		cp++;
	while(1)
	{
		switch(c = *(unsigned char*)(sp = cp))
		{
		    case '[':
			if(flags&NV_NOARRAY)
			{
				dp->last = cp;
				return np;
			}
			cp = nv_endsubscript(NULL,sp,0);
			if(sp==name || sp[-1]=='.')
				c = *(sp = cp);
			goto skip;
		    case '.':
			if(flags&NV_IDENT)
				return NULL;
			if(root==sh.var_tree)
				flags &= ~NV_EXPORT;
			if(!copy && !(flags&NV_NOREF))
			{
				c = sp-name;
				copy = cp-name;
				dp->nofree |= 1;
				name = copystack(NULL, name,NULL);
				cp = (char*)name+copy;
				sp = (char*)name+c;
				c = '.';
			}
			/* FALLTHROUGH */
		skip:
		    case '+':
		    case '=':
			*sp = 0;
			/* FALLTHROUGH */
		    case 0:
			isref = 0;
			dp->last = cp;
			mode =  (c=='.' || (flags&NV_NOADD))?add:NV_ADD;
			if(level++ || ((flags&NV_NOSCOPE) && c!='.'))
				mode |= NV_NOSCOPE;
			np=0;
			if(top)
			{
				struct Ufunction *rp;
				Namval_t *mp;
				if((rp=sh.st.real_fun) && !rp->sdict && (flags&NV_STATIC))
				{
					Dt_t *dp = dtview(sh.var_tree,NULL);
					rp->sdict = dtopen(&_Nvdisc,Dtoset);
					dtview(rp->sdict,dp);
					dtview(sh.var_tree,rp->sdict);
				}
				if(mp = nv_search(name,sh.var_tree,0))
				{
#if SHOPT_NAMESPACE
					if(sh.var_tree->walk==sh.var_base || (sh.var_tree->walk!=sh.var_tree && sh.namespace &&  nv_dict(sh.namespace)==sh.var_tree->walk))
#else
					if(sh.var_tree->walk==sh.var_base)
#endif /* SHOPT_NAMESPACE */
					{
#if SHOPT_NAMESPACE
						if(!(nq = nv_search((char*)mp,sh.var_base,NV_REF)))
#endif /* SHOPT_NAMESPACE */
						nq = mp;
						sh.last_root = sh.var_tree->walk;
						if((flags&NV_NOSCOPE) && *cp!='.')
						{
							if(mode==0)
								root = sh.var_tree->walk;
							else
							{
								nv_delete(mp,NULL,NV_NOFREE);
								mp = 0;
							}
						}
						np = mp;
					}
					else if(!sh.invoc_local)
					{
						if(sh.var_tree->walk)
							root = sh.var_tree->walk;
						flags |= NV_NOSCOPE;
						noscope = 1;
						np = mp;
					}
				}
				if(rp && rp->sdict && (flags&NV_STATIC))
				{
					root = rp->sdict;
					if(np && sh.var_tree->walk==sh.var_tree)
					{
						nv_unset(np,0);
						nv_delete(np,sh.var_tree,0);
						np = 0;
					}
					if(!np || sh.var_tree->walk!=root)
						np =  nv_search(name,root,NV_NOSCOPE|NV_ADD);
				}
			}
#if SHOPT_NAMESPACE
			if(!np && !noscope && *name!='.' && sh.namespace && root==sh.var_tree)
				root = nv_dict(sh.namespace);
#endif /* SHOPT_NAMESPACE */
			if(np ||  (np = nv_search(name,root,mode)))
			{
				isref = nv_isref(np);
				sh.openmatch = root->walk?root->walk:root;
				if(top)
				{
					if(nq==np)
					{
						flags &= ~NV_NOSCOPE;
						root = sh.last_root;
					}
					else if(nq)
					{
						if(nv_isnull(np) && c!='.')
						{
							Namfun_t *fp = nv_enforcedisc(nq);
							if (fp)
								nv_stack(np,fp);
							if (fp || nq==OPTINDNOD)
							{
								np->nvname = nq->nvname;
#if SHOPT_NAMESPACE
								if(sh.namespace && nv_dict(sh.namespace)==sh.var_tree && nv_isattr(nq,NV_EXPORT))
									nv_onattr(np,NV_EXPORT);
#endif /* SHOPT_NAMESPACE */
								if(nq==OPTINDNOD)
								{
									np->nvfun = nq->nvfun;
									np->nvalue = &sh.st.optindex;
									nv_onattr(np,NV_INTEGER|NV_NOFREE);
								}
							}
						}
						flags |= NV_NOSCOPE;
					}
				}
				else if(add && nv_isnull(np) && c=='.' && cp[1]!='.')
					nv_setvtree(np);
#if SHOPT_NAMESPACE
				if(sh.namespace && root==nv_dict(sh.namespace))
				{
					flags |= NV_NOSCOPE;
					sh.last_table = sh.namespace;
				}
#endif /* SHOPT_NAMESPACE */
			}
			if(c)
				*sp = c;
			top = 0;
			if(isref)
			{
#if SHOPT_FIXEDARRAY
				int n=0,dim;
#endif /* SHOPT_FIXEDARRAY */
#if NVCACHE
				nvcache.ok = 0;
#endif
				if(nv_isattr(np,NV_NOOPTIMIZE) || c=='.') /* don't optimize */
					nv_setoptimize(NULL);
				else if((flags&NV_NOREF) && (c!='[' && *cp!='.'))
				{
					if(c && !(flags&NV_NOADD))
						nv_unref(np);
					return np;
				}
				while(nv_isref(np) && np->nvalue)
				{
					root = nv_reftree(np);
					sh.last_root = root;
					sh.last_table = nv_reftable(np);
					sub = nv_refsub(np);
#if SHOPT_FIXEDARRAY
					n = nv_refindex(np);
					dim = nv_refdimen(np);
#endif /* SHOPT_FIXEDARRAY */
					np = nv_refnode(np);
#if SHOPT_FIXEDARRAY
					if(n)
					{
						ap = nv_arrayptr(np);
						ap->nelem = dim;
						nv_putsub(np,NULL,n);
					}
					else
#endif /* SHOPT_FIXEDARRAY */
					if(sub && c!='.')
						nv_putsub(np,sub,0L);
					flags |= NV_NOSCOPE;
					noscope = 1;
				}
				sh.first_root = root;
				if(nv_isref(np))
				{
					/* At this point, it is known that np is an unset nameref */
					if(c=='[' || c=='.' || !(flags&NV_ASSIGN))
					{
						/* unsetref[foo]=bar or unsetref.foo=bar or $unsetref: error */
						errormsg(SH_DICT,ERROR_exit(1),e_noref,nv_name(np));
						UNREACHABLE();
					}
					else if(c==0)
					{
						/* unsetref=(...) or unsetref+=(...): remove -n attribute */
						errormsg(SH_DICT,ERROR_warn(0),e_rmref,nv_name(np));
						nv_offattr(np,NV_REF);
					}
				}
				if(sub && c==0)
				{
					if(flags&NV_ARRAY)
					{
						ap = nv_arrayptr(np);
						nq = nv_opensub(np);
						if((flags&NV_ASSIGN) && (!nq || nv_isnull(nq)))
							ap->nelem++;
						if(!nq)
							goto addsub;
						else
							np = nq;
					}
					return np;
				}
				if(np==nq)
					flags &= ~(noscope?0:NV_NOSCOPE);
#if SHOPT_FIXEDARRAY
				else if(c || n)
#else
				else if(c)
#endif /* SHOPT_FIXEDARRAY */
				{
#if SHOPT_FIXEDARRAY
					static char null[1] = "";
#endif /* SHOPT_FIXEDARRAY */
					c = (cp-sp);
					copy = strlen(cp=nv_name(np));
					dp->nofree |= 1;
#if SHOPT_FIXEDARRAY
					if(*sp==0)
						name = cp;
					else
#endif /* SHOPT_FIXEDARRAY */
						name = copystack(cp,sp,sub);
					sp = (char*)name + copy;
					cp = sp+c;
					c = *sp;
					if(!noscope)
						flags &= ~NV_NOSCOPE;
#if SHOPT_FIXEDARRAY
					if(c==0)
						nv_endsubscript(np,null,NV_ADD);
#endif /* SHOPT_FIXEDARRAY */
				}
				flags |= NV_NOREF;
				if(*cp==0 &&  nv_isnull(np) && !nv_isarray(np))
					nofree = NV_NOFREE;
			}
			sh.last_root = root;
			if(*cp && cp[1]=='.')
				cp++;
			if(c=='.' && (cp[1]==0 ||  cp[1]=='=' || cp[1]=='+'))
			{
				nv_local = 1;
				if(np)
					nv_onattr(np,nofree);
				return np;
			}
			if(cp[-1]=='.')
				cp--;
			do
			{
#if SHOPT_FIXEDARRAY
				int fixed;
#endif /* SHOPT_FIXEDARRAY */
				if(!np)
				{
					if(!nq && *sp=='[' && *cp==0 && cp[-1]==']')
					{
						/*
						 * for backward compatibility
						 * evaluate subscript for
						 * possible side effects
						 */
						cp[-1] = 0;
						sh_arith(sp+1);
						cp[-1] = ']';
					}
					return np;
				}
#if SHOPT_FIXEDARRAY
				fixed = 0;
				if((ap=nv_arrayptr(np)) && ap->fixed)
					fixed = 1;
#endif /* SHOPT_FIXEDARRAY */
				if(c=='[' || (c=='.' && nv_isarray(np)))
				{
					int n = 0;
					sh.nv_putsub_already_called_sh_arith = 0;
					sub = 0;
					mode &= ~NV_NOSCOPE;
					if(c=='[')
					{
#if SHOPT_FIXEDARRAY
						ap = nv_arrayptr(np);
#endif /* SHOPT_FIXEDARRAY */
						n = mode|nv_isarray(np);
						if(!mode && (flags&NV_ARRAY) && ((c=sp[1])=='*' || c=='@') && sp[2]==']')
						{
							/* not implemented yet */
							dp->last = cp;
							return np;
						}
#if SHOPT_FIXEDARRAY
						if(fixed)
							flags |= NV_FARRAY;
						else
#endif /* SHOPT_FIXEDARRAY */
						if((n&NV_ADD)&&(flags&NV_ARRAY))
							n |= ARRAY_FILL;
						if(flags&NV_ASSIGN)
							n |= NV_ADD;
						cp = nv_endsubscript(np,sp,n|(flags&(NV_ASSIGN|NV_FARRAY)));
#if SHOPT_FIXEDARRAY
						flags &= ~NV_FARRAY;
						if(fixed)
							flags &= ~NV_ARRAY;
#endif /* SHOPT_FIXEDARRAY */
					}
					else
						cp = sp;

					if((c = *cp)=='.'
					|| (c=='[' && nv_isarray(np))
					|| (n&ARRAY_FILL)
					|| ((
#if SHOPT_FIXEDARRAY
					     ap ||
#endif
					           (flags&NV_ASSIGN)) && (flags&NV_ARRAY)))
					{
						int m = cp-sp;
						sub = m?nv_getsub(np):0;
						if(!sub)
						{
							if(m && !(n&NV_ADD))
								return NULL;
							sub = "0";
						}
						n = strlen(sub)+2;
						if(!copy)
						{
							copy = cp-name;
							dp->nofree |= 1;
							name = copystack(NULL, name,NULL);
							cp = (char*)name+copy;
							sp = cp-m;
						}
						if(n <= m)
						{
							if(n)
							{
								memcpy(sp+1,sub,n-2);
								sp[n-1] = ']';
							}
							if(n < m)
							{
								char *dp = sp+n;
								while(*dp++=*cp++);
								cp = sp+n;
							}
						}
						else
						{
							int r = n-m;
							m = sp-name;
							name = stack_extend(name, cp-1, r);
							sp = (char*)name + m;
							*sp = '[';
							memcpy(sp+1,sub,n-2);
							sp[n-1] = ']';
							cp = sp+n;
						}
					}
					else if(c==0 && mode && (n=nv_aindex(np))>0)
						nv_putsub(np,NULL,n);
#if SHOPT_FIXEDARRAY
					else if(n==0 && !fixed && (c==0 || (c=='[' && !nv_isarray(np))))
#else
					else if(n==0 && (c==0 || (c=='[' && !nv_isarray(np))))
#endif /* SHOPT_FIXEDARRAY */
					{
						/* subscript must be 0 */
						/* avoid double arithmetic evaluation */
						if(sh.nv_putsub_already_called_sh_arith)
							n = sh.nv_putsub_idx;
						else
						{
							cp[-1] = 0;
							n = sh_arith(sp+1);
							cp[-1] = ']';
						}
						if(n)
							return NULL;
						if(nv_isarray(np))
							nv_putsub(np,"0",ARRAY_FILL);
						if(c)
							sp = cp;
					}
					dp->last = cp;
					if(nv_isarray(np) && (c=='[' || c=='.' || (flags&NV_ARRAY)))
					{
					addsub:
						sp = cp;
						if(!(nq = nv_opensub(np)))
						{
							Namarr_t *ap = nv_arrayptr(np);
							if(!sub && (flags&NV_NOADD))
								return NULL;
							n = mode|((flags&NV_NOADD)?0:NV_ADD);
							if(!ap && (n&NV_ADD))
							{
								nv_putsub(np,sub,ARRAY_FILL);
								ap = nv_arrayptr(np);
							}
							if(n && ap && !ap->table)
								ap->table = dtopen(&_Nvdisc,Dtoset);
							if(ap && ap->table && (nq=nv_search(sub,ap->table,n)))
								nq->nvmeta = np;
							if(nq && nv_isnull(nq))
								nq = nv_arraychild(np,nq,c);
						}
						if(nq)
						{
							if(c=='.' && !nv_isvtree(nq))
							{
								if(flags&NV_NOADD)
									return NULL;
								nv_setvtree(nq);
							}
							nv_onattr(np,nofree);
							nofree = 0;
							np = nq;
						}
						else if(strncmp(cp,"[0]",3))
							return nq;
						else
						{
							/* ignore [0]  */
							dp->last = cp += 3;
							c = *cp;
						}
					}
				}
#if SHOPT_FIXEDARRAY
				else if(nv_isarray(np) && (!fixed || cp[-1]!=']'))
#else
				else if(nv_isarray(np))
#endif /* SHOPT_FIXEDARRAY */
				{
					if(c==0 && (flags&NV_MOVE))
						return np;
					nv_putsub(np,NULL,ARRAY_UNDEF);
				}
				nv_onattr(np,nofree);
				nofree  = 0;
				if(c=='.' && (fp=np->nvfun))
				{
					for(; fp; fp=fp->next)
					{
						if(fp->disc && fp->disc->createf)
							break;
					}
					if(fp)
					{
						if((nq = (*fp->disc->createf)(np,cp+1,flags,fp)) == np)
						{
							add = NV_ADD;
							sh.last_table = 0;
							break;
						}
						else if(np=nq)
						{
							if((c = *(sp=cp=dp->last=fp->last))==0)
							{
								if(nv_isarray(np) && sp[-1]!=']')
									nv_putsub(np,NULL,ARRAY_UNDEF);
								return np;
							}
						}
					}
				}
			}
			while(c=='[');
			if(c!='.' || cp[1]=='.')
				return np;
			cp++;
			break;
		    default:
			dp->last = cp;
			if((c = mbchar(cp)) && !isaletter(c))
				return np;
			while(xp=cp, c=mbchar(cp), isaname(c));
			cp = xp;
		}
	}
	return np;
}

/*
 * delete the node <np> from the dictionary <root> and clear from the cache
 * if <root> is NULL, only the cache is cleared
 * if flags does not contain NV_NOFREE, the node is freed
 * if flags contains NV_REF, does not set NullNode to avoid defeating nameref loop detection
 * if np==0 && !root && flags==0, delete the Refdict dictionary
 */
void nv_delete(Namval_t* np, Dt_t *root, int flags)
{
#if NVCACHE
	int			c;
	struct Cache_entry	*xp;
	for(c=0,xp=nvcache.entries ; c < NVCACHE; xp= &nvcache.entries[++c])
	{
		if(xp->np==np)
			xp->root = 0;
	}
#endif
	if(!np && !root && flags==0)
	{
		if(Refdict)
			dtclose(Refdict);
		Refdict = 0;
		return;
	}
	if(root || !(flags&NV_NOFREE))
	{
		if(!(flags&NV_FUNCTION) && Refdict)
		{
			Namval_t **key = &np;
			struct Namref *rp;
			while(rp = (struct Namref*)dtmatch(Refdict,key))
			{
				if(rp->sub)
					free(rp->sub);
				rp->sub = 0;
				rp = dtremove(Refdict,rp);
				if(rp && !(flags&NV_REF))
					rp->np = &NullNode;
			}
		}
	}
	if(root)
	{
		if(dtdelete(root,np))
		{
			if(!(flags&NV_NOFREE) && ((flags&NV_FUNCTION) || !nv_subsaved(np,flags&NV_TABLE)))
			{
				Namarr_t *ap;
				if(nv_isarray(np) && np->nvfun && (ap=nv_arrayptr(np)) && array_assoc(ap))
				{
					/* free associative array from memory */
					while(nv_associative(np,0,NV_ANEXT))
						nv_associative(np,0,NV_ADELETE);
					nv_associative(np,0,NV_AFREE);
					free(np->nvfun);
				}
				free(np);
			}
		}
	}
}

/*
 * Put <arg> into associative memory.
 * If <flags> & NV_ARRAY then follow array to next subscript
 * If <flags> & NV_NOARRAY then subscript is not allowed
 * If <flags> & NV_NOSCOPE then use the current scope only
 * If <flags> & NV_ASSIGN then assignment is allowed
 * If <flags> & NV_IDENT then name must be an identifier
 * If <flags> & NV_VARNAME then name must be a valid variable name
 * If <flags> & NV_NOADD then node will not be added if not found
 * If <flags> & NV_NOREF then don't follow reference
 * If <flags> & NV_NOFAIL then don't generate an error message on failure
 * If <flags> & NV_STATIC then unset before an assignment
 * If <flags> & NV_UNATTR then unset attributes before assignment
 * SH_INIT is only set while initializing the environment
 */
Namval_t *nv_open(const char *name, Dt_t *root, int flags)
{
	char			*cp=(char*)name;
	int			c;
	Namval_t		*np=0;
	Namfun_t		fun;
	int			append=0;
	const char		*msg = e_varname;
	char			*fname = 0;
	int			offset = stktell(sh.stk);
	Dt_t			*funroot = NULL;
#if NVCACHE
	struct Cache_entry	*xp;
#endif
	sh_stats(STAT_NVOPEN);
	memset(&fun,0,sizeof(fun));
	sh.openmatch = 0;
	sh.last_table = 0;
	if(!root)
		root = sh.var_tree;
	sh.last_root = root;
	if(root==sh.fun_tree)
	{
		flags |= NV_NOREF;
		msg = e_badfun;
		if(strchr(name,'.'))
		{
			name = cp = copystack(0,name,NULL);
			fname = strrchr(cp,'.');
			*fname = 0;
			fun.nofree |= 1;
			flags &=  ~NV_IDENT;
			funroot = root;
			root = sh.var_tree;
		}
	}
	else if(!(flags&(NV_IDENT|NV_VARNAME|NV_ASSIGN)))
	{
		long mode = ((flags&NV_NOADD)?0:NV_ADD);
		if(flags&NV_NOSCOPE)
			mode |= NV_NOSCOPE;
		np = nv_search(name,root,mode);
		if(np && !(flags&NV_REF))
		{
			while(nv_isref(np))
			{
				sh.last_table = nv_reftable(np);
				np = nv_refnode(np);
			}
		}
		return np;
	}
	else if(sh.prefix && (flags&NV_ASSIGN))
	{
		name = cp = copystack(sh.prefix,name,NULL);
		fun.nofree |= 1;
	}
	c = *(unsigned char*)cp;
	if(root==sh.alias_tree)
	{
		msg = e_aliname;
		while((c= *(unsigned char*)cp++) && (c!='=') && (c!='/') &&
			(c>=0x200 || !(c=sh_lexstates[ST_NORM][c]) || c==S_EPAT || c==S_COLON));
		if(c= *--cp)
			*cp = 0;
		np = nv_search(name, root, (flags&NV_NOADD)?0:NV_ADD);
		if(c)
			*cp = c;
		goto skip;
	}
	else if(flags&NV_IDENT)
		msg = e_ident;
	else if(c=='.')
	{
		c = *++cp;
		flags |= NV_NOREF;
		if(root==sh.var_tree)
			root = sh.var_base;
		sh.last_table = 0;
	}
	if(c= !isaletter(c))
		goto skip;
#if NVCACHE
	for(c=0,xp=nvcache.entries ; c < NVCACHE; xp= &nvcache.entries[++c])
	{
		if(xp->root!=root)
			continue;
		if(xp->namespace==sh.namespace && (flags&(NV_ARRAY|NV_NOSCOPE))==xp->flags && strncmp(xp->name,name,xp->len)==0 && (name[xp->len]==0 || name[xp->len]=='=' || name[xp->len]=='+'))
		{
			sh_stats(STAT_NVHITS);
			np = xp->np;
			cp = (char*)name+xp->len;
			if(nv_isarray(np) && !(flags&NV_MOVE))
				 nv_putsub(np,NULL,ARRAY_UNDEF);
			sh.last_table = xp->last_table;
			sh.last_root = xp->last_root;
			goto nocache;
		}
	}
	nvcache.ok = 1;
#endif
	np = nv_create(name, root, flags, &fun);
	cp = fun.last;
#if NVCACHE
	if(np && nvcache.ok && cp[-1]!=']')
	{
		xp = &nvcache.entries[nvcache.index];
		if(*cp)
		{
			char *sp = strchr(name,*cp);
			if(!sp)
				goto nocache;
			xp->len = sp-name;
		}
		else
			xp->len = strlen(name);
		c = roundof(xp->len+1,32);
		if(c > xp->size)
			xp->name = sh_realloc(xp->name, xp->size = c);
		memcpy(xp->name,name,xp->len);
		xp->name[xp->len] = 0;
		xp->root = root;
		xp->np = np;
		xp->namespace = sh.namespace;
		xp->last_table = sh.last_table;
		xp->last_root = sh.last_root;
		xp->flags = (flags&(NV_ARRAY|NV_NOSCOPE));
		nvcache.index = (nvcache.index+1)&(NVCACHE-1);
	}
nocache:
	nvcache.ok = 0;
#endif
	if(fname)
	{
		c = (flags&NV_NOSCOPE)|((flags&NV_NOADD)?0:NV_ADD);
		*fname = '.';
		np = nv_search(name, funroot, c);
		*fname = 0;
	}
	else
	{
		if(*cp=='.' && cp[1]=='.')
		{
			append |= NV_NODISC;
			cp+=2;
		}
		if(*cp=='+' && cp[1]=='=')
		{
			append |= NV_APPEND;
			cp++;
		}
	}
	c = *cp;
skip:
	if(np && sh.mktype)
		np = nv_addnode(np,0);
	if(c=='=' && np && (flags&NV_ASSIGN))
	{
		cp++;
		if(sh_isstate(SH_INIT))
			nv_putval(np, cp, NV_RDONLY);
		else
		{
			char *sub=0, *prefix= sh.prefix;
			Namval_t *mp;
			Namarr_t *ap;
			int isref;
			sh.prefix = 0;
			if((flags&NV_STATIC) && !sh.mktype)
			{
				if(!nv_isnull(np))
				{
					sh.prefix = prefix;
					return np;
				}
			}
			isref = nv_isref(np);
#if SHOPT_FIXEDARRAY
			if(sh_isoption(SH_XTRACE) && (ap=nv_arrayptr(np)) && !ap->fixed)
#else
			if(sh_isoption(SH_XTRACE) && nv_isarray(np))
#endif /* SHOPT_FIXEDARRAY */
				sub = nv_getsub(np);
			c = msg==e_aliname? 0: (append | (flags&NV_EXPORT));
			if(isref)
				nv_offattr(np,NV_REF);
			if(!append && (flags&NV_UNATTR))
			{
				if(!np->nvfun)
					nv_unset(np,NV_EXPORT);
			}
			if(flags&NV_MOVE)
			{
				if(ap=nv_arrayptr(np))
				{
					if(mp=nv_opensub(np))
						np = mp;
					else if(!array_assoc(ap) && (mp = nv_open(cp,sh.var_tree,NV_NOFAIL|NV_VARNAME|NV_NOARRAY|NV_NOADD)) && nv_isvtree(np))
					{
						ap->nelem |= ARRAY_TREE;
						nv_putsub(np,NULL,ARRAY_ADD|nv_aindex(np));
						np = nv_opensub(np);
						ap->nelem &= ~ARRAY_TREE;
						ap->nelem -= 1;
					}
				}
				nv_unset(np,NV_EXPORT);
			}
			nv_putval(np, cp, c);
			if(isref)
			{
				if(nv_search((char*)np,sh.var_base,NV_REF))
					sh.last_root = sh.var_base;
				nv_setref(np,NULL,NV_VARNAME);
			}
			savesub = sub;
			sh.prefix = prefix;
		}
		nv_onattr(np, flags&NV_ATTRIBUTES);
	}
	else if(c)
	{
		if(flags&NV_NOFAIL)
			return NULL;
		if(c=='.')
			msg = e_noparent;
		else if(c=='[')
			msg = e_noarray;
		errormsg(SH_DICT,ERROR_exit(1),msg,name);
		UNREACHABLE();
	}
	if(fun.nofree&1)
		stkseek(sh.stk,offset);
	return np;
}

static int ja_size(char*, int, int);
static void ja_restore(void);
static char *savep;
static char savechars[8+1];

/*
 * Put value <sp> into name-value node <np>.
 * If <np> is an array, then the element given by the
 *   current index is assigned to.
 * If <flags> contains NV_RDONLY, readonly attribute is ignored.
 * If <flags> contains NV_INTEGER, sp is a pointer to a number.
 * If <flags> contains NV_NOFREE, previous value is freed, and <sp>
 * becomes value of node and <flags> becomes attributes.
 */
void nv_putval(Namval_t *np, const char *sp, int flags)
{
	void		**vpp;	/* pointer to value pointer */
	unsigned int	size = 0;
	int		was_local = nv_local;
#if SHOPT_FIXEDARRAY
	Namarr_t	*ap;
#endif /* SHOPT_FIXEDARRAY */
	/*
	 * When we are in an invocation-local scope and we are using the
	 * += operator, clone the variable from the previous scope.
	 */
	if(sh.invoc_local && (flags&NV_APPEND) && nv_isnull(np))
	{
		Namval_t *mp = nv_search(np->nvname,sh.var_tree->view,0);
		if(mp)
			nv_clone(mp,np,0);
	}
	/* Don't alter readonly variables */
	if(!(flags&NV_RDONLY) && nv_isattr(np,NV_RDONLY))
	{
		errormsg(SH_DICT,ERROR_exit(1),e_readonly, nv_name(np));
		UNREACHABLE();
	}
	/* Create a local scope when inside of a virtual subshell */
	nv_setoptimize(NULL);
	if(sh.subshell && !nv_local && !(flags&NV_RDONLY))
		sh_assignok(np,1);
	/* Export the variable if 'set -o allexport' is enabled */
	if(sh_isoption(SH_ALLEXPORT))
	{
		flags |= NV_EXPORT;
		nv_onattr(np,NV_EXPORT);
	}
	if(np->nvfun && np->nvfun->disc && !(flags&NV_NODISC) && !nv_isref(np))
	{
		/* This function contains disc */
		if(!nv_local)
		{
			nv_local=1;
			nv_putv(np,sp,flags,np->nvfun);
			if(sp && ((flags&NV_EXPORT) || nv_isattr(np,NV_EXPORT)))
				env_change();
			return;
		}
		/* called from disc, assign the actual value */
	}
	flags &= ~NV_NODISC;
	nv_local=0;
	if(flags&(NV_NOREF|NV_NOFREE))
	{
		if(np->nvalue && np->nvalue!=sp && !nv_isattr(np,NV_NOFREE))
			free(np->nvalue);
		np->nvalue = (void*)sp;
		nv_setattr(np,(flags&~NV_RDONLY)|NV_NOFREE);
		return;
	}
#if SHOPT_FIXEDARRAY
	if(np->nvalue && nv_isarray(np) && (ap=nv_arrayptr(np)) && !ap->fixed)
#else
	if(np->nvalue && nv_isarray(np) && nv_arrayptr(np))
#endif /* SHOPT_FIXEDARRAY */
		vpp = np->nvalue;
	else
		vpp = &np->nvalue;
	if(*vpp==Empty)
		*vpp = NULL;
	if(nv_isattr(np,NV_INTEGER))
	{
		if(nv_isattr(np, NV_DOUBLE) == NV_DOUBLE)
		{
			if(nv_isattr(np, NV_LONG) && sizeof(double)<sizeof(Sfdouble_t))
			{
				Sfdouble_t ld, old=0;
				if(flags&NV_INTEGER)
				{
					if(flags&NV_LONG)
						ld = *((Sfdouble_t*)sp);
					else if(flags&NV_SHORT)
						ld = *((float*)sp);
					else
						ld = *((double*)sp);
				}
				else
					ld = sh_arith(sp);
				if(!*vpp)
					*vpp = new_of(Sfdouble_t,0);
				else if(flags&NV_APPEND)
					old = *(Sfdouble_t*)*vpp;
				*(Sfdouble_t*)*vpp = old ? ld+old : ld;
			}
			else
			{
				double d,od=0;
				if(flags&NV_INTEGER)
				{
					if(flags&NV_LONG)
						d = (double)(*(Sfdouble_t*)sp);
					else if(flags&NV_SHORT)
						d = (double)(*(float*)sp);
					else
						d = *(double*)sp;
				}
				else
					d = sh_arith(sp);
				if(!*vpp)
					*vpp = new_of(double,0);
				else if(flags&NV_APPEND)
					od = *(double*)*vpp;
				*(double*)*vpp = od ? d+od : d;
			}
		}
		else
		{
			if(nv_isattr(np, NV_LONG) && sizeof(int32_t)<sizeof(Sflong_t))
			{
				Sflong_t ll=0,oll=0;
				if(flags&NV_INTEGER)
				{
					if((flags&NV_DOUBLE) == NV_DOUBLE)
					{
						if(flags&NV_LONG)
							ll = *((Sfdouble_t*)sp);
						else if(flags&NV_SHORT)
							ll = *((float*)sp);
						else
							ll = *((double*)sp);
					}
					else if(nv_isattr(np,NV_UNSIGN))
					{
						if(flags&NV_LONG)
							ll = *((Sfulong_t*)sp);
						else if(flags&NV_SHORT)
							ll = *((uint16_t*)sp);
						else
							ll = *((uint32_t*)sp);
					}
					else
					{
						if(flags&NV_LONG)
							ll = *((Sflong_t*)sp);
						else if(flags&NV_SHORT)
							ll = *((uint16_t*)sp);
						else
							ll = *((uint32_t*)sp);
					}
				}
				else if(sp)
					ll = (Sflong_t)sh_arith(sp);
				if(!*vpp)
					*vpp = new_of(Sflong_t,0);
				else if(flags&NV_APPEND)
					oll = *(Sflong_t*)*vpp;
				*(Sflong_t*)*vpp = ll + oll;
			}
			else
			{
				int32_t l=0;
				if(flags&NV_INTEGER)
				{
					if((flags&NV_DOUBLE) == NV_DOUBLE)
					{
						Sflong_t ll;
						if(flags&NV_LONG)
							ll = *((Sfdouble_t*)sp);
						else if(flags&NV_SHORT)
							ll = *((float*)sp);
						else
							ll = *((double*)sp);
						l = (int32_t)ll;
					}
					else if(nv_isattr(np,NV_UNSIGN))
					{
						if(flags&NV_LONG)
							l = *((Sfulong_t*)sp);
						else if(flags&NV_SHORT)
							l = *((uint16_t*)sp);
						else
							l = *(uint32_t*)sp;
					}
					else
					{
						if(flags&NV_LONG)
							l = *((Sflong_t*)sp);
						else if(flags&NV_SHORT)
							l = *((int16_t*)sp);
						else
							l = *(int32_t*)sp;
					}
				}
				else if(sp)
				{
					Sfdouble_t ld = sh_arith(sp);
					if(ld<0)
						l = (int32_t)ld;
					else
						l = (uint32_t)ld;
				}
				if(nv_size(np) <= 1)
					nv_setsize(np,10);
				if(nv_isattr(np,NV_SHORT))
				{
					int16_t os=0;
					if(!*vpp)
						*vpp = new_of(int16_t,0);
					else if(flags&NV_APPEND)
						os = *(int16_t*)*vpp;
					*(int16_t*)*vpp = os + (int16_t)l;
				}
				else
				{
					int32_t ol=0;
					if(!*vpp)
						*vpp = new_of(int32_t,0);
					else if(flags&NV_APPEND)
						ol = *(int32_t*)*vpp;
					*(int32_t*)*vpp = l + ol;
				}
			}
		}
	}
	else
	{
		void *tofree=0;
		int offset = 0;
		char buff[PATH_MAX];
		if(flags&NV_INTEGER)
		{
			if((flags&NV_DOUBLE)==NV_DOUBLE)
			{
				if(flags&NV_LONG)
					sfprintf(sh.strbuf,"%.*Lg",LDBL_DIG,*((Sfdouble_t*)sp));
				else
					sfprintf(sh.strbuf,"%.*g",DBL_DIG,*((double*)sp));
			}
			else if(flags&NV_UNSIGN)
			{
				if(flags&NV_LONG)
					sfprintf(sh.strbuf,"%I*lu",sizeof(Sfulong_t),*((Sfulong_t*)sp));
				else
					sfprintf(sh.strbuf,"%lu",(unsigned long)((flags&NV_SHORT)?*((uint16_t*)sp):*((uint32_t*)sp)));
			}
			else
			{
				if(flags&NV_LONG)
					sfprintf(sh.strbuf,"%I*ld",sizeof(Sflong_t),*((Sflong_t*)sp));
				else
					sfprintf(sh.strbuf,"%ld",(long)((flags&NV_SHORT)?*((int16_t*)sp):*((int32_t*)sp)));
			}
			sp = sfstruse(sh.strbuf);
		}
		if(nv_isattr(np, NV_HOST|NV_INTEGER)==NV_HOST && sp)
		{
			/*
			 * return the host file name given the UNIX name
			 */
			pathnative(sp,buff,sizeof(buff));
			if(buff[1]==':' && buff[2]=='/')
			{
				buff[2] = '\\';
				if(*buff>='A' &&  *buff<='Z')
					*buff += 'a'-'A';
			}
			sp = buff;
		}
		else if((nv_isattr(np, NV_RJUST|NV_ZFILL|NV_LJUST)) && sp)
		{
			for( ; *sp == ' ' || *sp == '\t'; sp++)
				;
	        	if((nv_isattr(np,NV_ZFILL)) && (nv_isattr(np,NV_LJUST)))
				for( ; *sp == '0'; sp++)
					;
			size = nv_size(np);
			if(size)
				size = ja_size((char*)sp,size,nv_isattr(np,NV_RJUST|NV_ZFILL));
		}
		if(!*vpp || *(char*)*vpp==0)
			flags &= ~NV_APPEND;
		if(!nv_isattr(np, NV_NOFREE))
		{
			/* delay free in case <sp> points into free region */
			tofree = *vpp;
		}
		if(nv_isattr(np,NV_BINARY) && !(flags&NV_RAW))
			tofree = 0;
		if(nv_isattr(np,NV_LJUST|NV_RJUST) && nv_isattr(np,NV_LJUST|NV_RJUST)!=(NV_LJUST|NV_RJUST))
			tofree = 0;
		if(!sp)
			*vpp = NULL;
		else
		{
			char *cp = NULL;		/* pointer to new string */
			unsigned int dot;		/* attribute or type length; defaults to string length */
			unsigned int append = 0;	/* offset for appending */
			if(sp==*vpp && !(flags&NV_APPEND))
				return;
			dot = strlen(sp);
			if(nv_isattr(np,NV_BINARY))
			{
				int oldsize = (flags&NV_APPEND)?nv_size(np):0;
				if(flags&NV_RAW)
				{
					if(tofree)
					{
						free((void*)tofree);
						nv_offattr(np,NV_NOFREE);
					}
					*vpp = (void*)sp;
					return;
				}
				size = 0;
				if(nv_isattr(np,NV_ZFILL))
					size = nv_size(np);
				if(size==0)
					size = oldsize + (3*dot/4);
				cp = (char*)sh_malloc(size+1);
				*cp = 0;
				nv_offattr(np,NV_NOFREE);
				if(oldsize)
					memcpy(cp,*vpp,oldsize);
				*vpp = cp;
				if(size <= oldsize)
					return;
				dot = base64decode(sp,dot, NULL, cp+oldsize, size-oldsize,NULL);
				dot += oldsize;
				if(!nv_isattr(np,NV_ZFILL) || nv_size(np)==0)
					nv_setsize(np,dot);
				else if(nv_isattr(np,NV_ZFILL) && (size>dot))
					memset(&cp[dot],0,size-dot);
				return;
			}
			else
			{
				if(size==0 && nv_isattr(np,NV_HOST)!=NV_HOST &&nv_isattr(np,NV_LJUST|NV_RJUST|NV_ZFILL))
				{
					nv_setsize(np,size=dot);
					tofree = *vpp;
				}
				else if(size > dot)
					dot = size;
				else if(nv_isattr(np,NV_LJUST|NV_RJUST)==NV_LJUST && dot>size)
					dot = size;
				if(flags&NV_APPEND)
				{
					if(dot==0)
						return;
					append = strlen(*vpp);
					if(!tofree || size)
					{
						offset = stktell(sh.stk);
						sfputr(sh.stk,*vpp,-1);
						sfputr(sh.stk,sp,0);
						sp = stkptr(sh.stk,offset);
						dot += append;
						append = 0;
					}
					else
					{
						flags &= ~NV_APPEND;
					}
				}
			}
			if(size==0 || tofree || dot || !(cp = *vpp))
			{
				if(dot==0 && !nv_isattr(np,NV_LJUST|NV_RJUST))
				{
					cp = AltEmpty;
					nv_onattr(np,NV_NOFREE);
				}
				else
				{
					if(tofree && tofree!=Empty && tofree!=AltEmpty)
					{
						cp = (char*)sh_realloc((void*)tofree, dot+append+1);
						tofree = 0;
					}
					else
						cp = (char*)sh_malloc(dot+append+1);
					cp[dot+append] = 0;
					nv_offattr(np,NV_NOFREE);
				}
			}
			if(dot)
			{
				char c = cp[dot+append];
				strncpy(cp+append, sp, dot+1);
				cp[dot+append] = c;
			}
			*vpp = cp;
			if(nv_isattr(np, NV_RJUST) && nv_isattr(np, NV_ZFILL))
				rightjust(cp,size,'0');
			else if(nv_isattr(np, NV_LJUST|NV_RJUST)==NV_RJUST)
				rightjust(cp,size,' ');
			else if(nv_isattr(np, NV_LJUST|NV_RJUST)==NV_LJUST)
			{
				char *dp;
				dp = strlen (cp) + cp;
				cp = cp+size;
				for (; dp < cp; *dp++ = ' ');
			 }
			/* restore original string */
			if(savep)
				ja_restore();
		}
		if(flags&NV_APPEND)
			stkseek(sh.stk,offset);
		if(tofree && tofree!=Empty && tofree!=AltEmpty)
			free((void*)tofree);
	}
	if(!was_local && ((flags&NV_EXPORT) || nv_isattr(np,NV_EXPORT)))
		env_change();
	return;
}

/*
 *
 *   Right-justify <str> so that it contains no more than
 *   <size> characters.  If <str> contains fewer than <size>
 *   characters, left-pad with <fill>.  Trailing blanks
 *   in <str> will be ignored.
 *
 *   If the leftmost digit in <str> is not a digit, <fill>
 *   will default to a blank.
 */
static void rightjust(char *str, int size, int fill)
{
	int n;
	char *cp,*sp;
	n = strlen(str);

	/* ignore trailing blanks */
	for(cp=str+n;n && *--cp == ' ';n--);
	if (n == size)
		return;
	if(n > size)
	{
		*(str+n) = 0;
		for (sp = str, cp = str+n-size; sp <= str+size; *sp++ = *cp++);
		return;
	}
	else *(sp = str+size) = 0;
	if (n == 0)
	{
		while (sp > str)
	       		*--sp = ' ';
		return;
	}
	while(n--)
	{
		sp--;
		*sp = *cp--;
	}
	if(!isdigit(*str))
		fill = ' ';
	while(sp>str)
		*--sp = fill;
	return;
}

/*
 * Handle left and right justified fields for multibyte characters.
 * Given physical size, return a logical size that reflects the screen width.
 * Multi-width characters are replaced by spaces if they cross the boundary.
 * <type> is non-zero for right-justified fields.
 */

static int ja_size(char *str,int size,int type)
{
	char *cp = str, *oldcp = str;
	int c = 0, n = size, oldn = size;
	while(*cp)
	{
		int outsize;
		wchar_t w;
		oldn = n;
		w = mbchar(cp);
		if((outsize = mbwidth(w)) <0)
			outsize = 0;
		size -= outsize;
		c = cp-oldcp;
		n += (c-outsize);
		oldcp = cp;
		if(size<=0 && type==0)
			break;
	}
	/* check for right justified fields that need truncating */
	if(size <0)
	{
		if(type==0)
		{
			/* left justified and character crosses field boundary */
			n = oldn;
			/* save boundary char and replace with spaces */
			size = c;
			savechars[size] = 0;
			while(size--)
			{
				savechars[size] = cp[size];
				cp[size] = ' ';
			}
			savep = cp;
		}
		size = -size;
		if(type)
			n -= (ja_size(str,size,0)-size);
	}
	return n;
}

static void ja_restore(void)
{
	char *cp = savechars;
	while(*cp)
		*savep++ = *cp++;
	savep = 0;
}

static char *staknam(Namval_t *np, char *value)
{
	char *p,*q;
	q = stkalloc(sh.stk,strlen(nv_name(np))+(value?strlen(value):0)+2);
	p=strcopy(q,nv_name(np));
	*p++ = '=';
	strcpy(p,value);
	return q;
}

/*
 * Called from sh_envgen() to push an individual variable to export
 */
static void pushnam(Namval_t *np, void *data)
{
	char *value;
	struct adata *ap = (struct adata*)data;
	if(strchr(np->nvname,'.'))
		return;
	ap->tp = 0;
	if(value=nv_getval(np))
		*ap->argnam++ = staknam(np,value);
}

/*
 * Generate the environment list for the child.
 */
char **sh_envgen(void)
{
	char **er;
	int namec;
	struct adata data;
	int i;
	data.tp = 0;
	data.mapname = 0;
	/* L_ARGNOD gets generated automatically as full path name of command */
	nv_offattr(L_ARGNOD,NV_EXPORT);
	namec = nv_scan(sh.var_tree,nullscan,NULL,NV_EXPORT,NV_EXPORT);
	namec += sh.save_env_n;
	er = stkalloc(sh.stk,(namec+4)*sizeof(char*));
	data.argnam = (er+=2) + sh.save_env_n;
	/* Physically copy the saved non-importable env vars, as the old environ[] may be freed by exscript() */
	for (i = 0; i < sh.save_env_n; i++)
	{
		size_t sz = strlen(sh.save_env[i]) + 1;
		char *cp = stkalloc(sh.stk, sz);
		memcpy(cp, sh.save_env[i], sz);
		er[i] = sh.save_env[i] = cp;
	}
	/* Add exported vars */
	nv_scan(sh.var_tree, pushnam,&data,NV_EXPORT, NV_EXPORT);
	*data.argnam = 0;
	return er;
}

struct scan
{
	void    (*scanfn)(Namval_t*, void*);
	int     scanmask;
	int     scanflags;
	int     scancount;
	void    *scandata;
};

static int scanfilter(Namval_t *np, struct scan *sp)
{
	int k=np->nvflag;
	struct adata *tp = (struct adata*)sp->scandata;
	char	*cp;
	if(!is_abuiltin(np) && tp && tp->tp && nv_type(np)!=tp->tp)
		return 0;
	if(sp->scanmask==NV_TABLE && nv_isvtree(np))
		k = NV_TABLE;
	if(sp->scanmask?(k&sp->scanmask)==sp->scanflags:(!sp->scanflags || (k&sp->scanflags)))
	{
		if(tp && tp->mapname)
		{
			if(sp->scanflags==NV_FUNCTION || sp->scanflags==(NV_NOFREE|NV_BINARY|NV_RAW))
			{
				int n = strlen(tp->mapname);
				if(strncmp(np->nvname,tp->mapname,n) || np->nvname[n]!='.' || strchr(&np->nvname[n+1],'.'))
					return 0;
			}
			else if((sp->scanflags==NV_UTOL||sp->scanflags==NV_LTOU) && (cp=(char*)nv_mapchar(np,0)) && strcmp(cp,tp->mapname))
				return 0;
		}
		if(!np->nvalue && !np->nvfun && !nv_isattr(np,~NV_DEFAULT))
			return 0;
		if(sp->scanfn)
		{
			if(nv_isarray(np))
				nv_putsub(np,NULL,0L);
			(*sp->scanfn)(np,sp->scandata);
		}
		sp->scancount++;
	}
	return 0;
}

/*
 * Handler function to invalidate a path name binding in the hash table
 */
void nv_rehash(Namval_t *np,void *data)
{
	NOT_USED(data);
	if(sh.subshell)
	{
		/* invalidate subshell node; if none exists, add a dummy */
		np = nv_search(nv_name(np),sh.track_tree,NV_ADD|NV_NOSCOPE);
	}
	nv_onattr(np,NV_NOALIAS);
}

/*
 * Walk through the name-value pairs
 * if <mask> is non-zero, then only nodes with (nvflags&mask)==flags
 *	are visited
 * If <mask> is zero, and <flags> non-zero, then nodes with one or
 *	more of <flags> is visited
 * If <mask> and <flags> are zero, then all nodes are visited
 */
int nv_scan(Dt_t *root, void (*fn)(Namval_t*,void*), void *data,int mask, int flags)
{
	Namval_t *np;
	Dt_t *base=0;
	struct scan sdata;
	sdata.scanmask = mask;
	sdata.scanflags = flags&~NV_NOSCOPE;
	sdata.scanfn = fn;
	sdata.scancount = 0;
	sdata.scandata = data;
	if(flags&NV_NOSCOPE)
		base = dtview((Dt_t*)root,0);
	for(np=(Namval_t*)dtfirst(root);np; np=(Namval_t*)dtnext(root,np))
		scanfilter(np, &sdata);
	if(base)
		dtview((Dt_t*)root,base);
	return sdata.scancount;
}

/*
 * create a new environment scope
 */
void sh_scope(struct argnod *envlist, int fun)
{
	Dt_t		*newscope, *newroot=sh.var_base;
	struct Ufunction	*rp;
#if SHOPT_NAMESPACE
	if(sh.namespace)
		newroot = nv_dict(sh.namespace);
#endif /* SHOPT_NAMESPACE */
	newscope = dtopen(&_Nvdisc,Dtoset);
	if(envlist)
	{
		dtview(newscope,(Dt_t*)sh.var_tree);
		sh.var_tree = newscope;
		nv_setlist(envlist,NV_EXPORT|NV_NOSCOPE|NV_IDENT|NV_ASSIGN,0);
		if(!fun)
			return;
		sh.var_tree = dtview(newscope,0);
	}
	if((rp=sh.st.real_fun)  && rp->sdict)
	{
		dtview(rp->sdict,newroot);
		newroot = rp->sdict;
	}
	dtview(newscope,(Dt_t*)newroot);
	sh.var_tree = newscope;
}

static void table_unset(Dt_t *root, int flags, Dt_t *oroot)
{
	Namval_t *np,*nq, *npnext;
	for(np=(Namval_t*)dtfirst(root);np;np=npnext)
	{
		if(nq=dtsearch(oroot,np))
		{
			if(nv_enforcedisc(nq))
			{
				unsigned int subshell = sh.subshell;
				sh.subshell = 0;
				if(nv_isattr(nq, NV_INTEGER))
				{
					Sfdouble_t d = nv_getnum(nq);
					nv_putval(nq,(char*)&d,NV_LDOUBLE);
				}
				else
					nv_putval(nq, nv_getval(nq), NV_RDONLY);
				sh.subshell = subshell;
				np->nvfun = 0;
			}
			if(nv_isattr(nq,NV_EXPORT))
				env_change();
		}
		sh.last_root = root;
		sh.last_table = 0;
		if(nv_isvtree(np))
		{
			int len = strlen(np->nvname);
			npnext = (Namval_t*)dtnext(root,np);
			while((nq=npnext) && strncmp(np->nvname,nq->nvname,len)==0 && nq->nvname[len]=='.')
			{
				nv_unset(nq,flags);
				npnext = (Namval_t*)dtnext(root,nq);
				nv_delete(nq,root,NV_TABLE);
			}
		}
		npnext = (Namval_t*)dtnext(root,np);
		if(nv_arrayptr(np))
			nv_putsub(np,NULL,ARRAY_SCAN);
		nv_unset(np,flags);
		nv_delete(np,root,NV_TABLE);
	}
}

/*
 *
 *   Set the value of <np> to 0, and nullify any attributes
 *   that <np> may have had.  Free any freeable space occupied
 *   by the value of <np>.  If <np> denotes an array member, it
 *   will retain its attributes.
 *   <flags> can contain NV_RDONLY to override the readonly attribute
 *	being cleared.
 *   <flags> can contain NV_EXPORT to preserve nvmeta.
 */
void	nv_unset(Namval_t *np,int flags)
{
	void		**vpp;	/* pointer to value pointer */
#if SHOPT_FIXEDARRAY
	Namarr_t	*ap;
#endif /* SHOPT_FIXEDARRAY */
	if(np==SH_LEVELNOD)
		return;
	if(!(flags&NV_RDONLY) && nv_isattr(np,NV_RDONLY))
	{
		errormsg(SH_DICT,ERROR_exit(1),e_readonly, nv_name(np));
		UNREACHABLE();
	}
	if(is_afunction(np) && np->nvalue)
	{
		struct Ufunction *rp = np->nvalue;
		struct slnod *slp = np->nvmeta;
		if(rp->running)
		{
			rp->running |= 1;
			return;
		}
		if(slp && !nv_isattr(np,NV_NOFREE))
		{
			struct Ufunction *rq;
			/* free function definition */
			char *name=nv_name(np),*cp= strrchr(name,'.');
			if(cp)
			{
				Namval_t *npv;
				*cp = 0;
				 npv = nv_open(name,sh.var_tree,NV_NOARRAY|NV_VARNAME|NV_NOADD);
				*cp++ = '.';
				if(npv && npv!=sh.namespace)
					nv_setdisc(npv,cp,NULL,(Namfun_t*)npv);
			}
			if(rp->fname && sh.fpathdict && (rq = (struct Ufunction*)nv_search(rp->fname,sh.fpathdict,0)))
			{
				do
				{
					if(rq->np != np)
						continue;
					dtdelete(sh.fpathdict,rq);
					break;
				}
				while(rq = (struct Ufunction*)dtnext(sh.fpathdict,rq));
			}
			if(rp->sdict)
			{
				Namval_t *mp, *nq;
				for(mp=(Namval_t*)dtfirst(rp->sdict);mp;mp=nq)
				{
					nq = dtnext(rp->sdict,mp);
					nv_unset(mp,NV_RDONLY);
					nv_delete(mp,rp->sdict,0);
				}
				dtclose(rp->sdict);
			}
			if(slp->slptr)
			{
				Stk_t *sp = slp->slptr;
				slp->slptr = NULL;
				stkclose(sp);
			}
			free(np->nvalue);
			np->nvalue = NULL;
		}
		goto done;
	}
	if(sh.subshell)
		sh_assignok(np,0);
	nv_offattr(np,NV_NODISC);
	if(np->nvfun && !nv_isref(np))
	{
		/* This function contains disc */
		if(!nv_local)
		{
			nv_local=1;
			nv_putv(np,NULL,flags,np->nvfun);
			nv_local=0;
			return;
		}
		/* called from disc, assign the actual value */
		nv_local=0;
	}
#if SHOPT_FIXEDARRAY
	if(np->nvalue && nv_isarray(np) && (ap=nv_arrayptr(np)) && !ap->fixed)
#else
	if(np->nvalue && nv_isarray(np) && nv_arrayptr(np))
#endif /* SHOPT_FIXEDARRAY */
		vpp = np->nvalue;
	else if(nv_isref(np) && !nv_isattr(np,NV_EXPORT|NV_MINIMAL) && np->nvalue)
	{
		struct Namref *nrp = np->nvalue;
		if(nrp->root && Refdict)
			dtremove(Refdict,nrp);
		if(nrp->sub)
			free(nrp->sub);
		free(nrp);
		np->nvalue = NULL;
		vpp = NULL;
	}
	else
		vpp = &np->nvalue;
	if(vpp && *vpp)
	{
		if(*vpp!=Empty && *vpp!=AltEmpty && !nv_isattr(np,NV_NOFREE))
			free(*vpp);
		*vpp = NULL;
	}
done:
	if(!nv_isarray(np) || !nv_arrayptr(np))
	{
		nv_setsize(np,0);
		if(!nv_isattr(np,NV_MINIMAL) || nv_isattr(np,NV_EXPORT))
		{
			if(nv_isattr(np,NV_EXPORT) && !strchr(np->nvname,'['))
				env_change();
			if(!(flags&NV_EXPORT) ||  nv_isattr(np,NV_EXPORT))
				np->nvmeta = NULL;
			nv_setattr(np,0);
		}
		else
		{
			nv_setattr(np,NV_MINIMAL);
			nv_delete(np,NULL,0);
		}
	}
}

/*
 * return the node pointer in the highest level scope
 */
Namval_t *sh_scoped(Namval_t *np)
{
	if(!dtvnext(sh.var_tree))
		return np;
	return dtsearch(sh.var_tree,np);
}

#if SHOPT_OPTIMIZE
struct optimize
{
	Namfun_t	hdr;
	char		**ptr;
	struct optimize	*next;
	Namval_t	*np;
};

static struct optimize *opt_free;

static void optimize_clear(Namval_t* np, Namfun_t *fp)
{
	struct optimize *op = (struct optimize*)fp;
	nv_stack(np,fp);
	nv_stack(np,NULL);
	for(;op && op->np==np; op=op->next)
	{
		if(op->ptr)
		{
			*op->ptr = 0;
			op->ptr = 0;
		}
	}
}

void nv_optimize_clear(Namval_t *np)
{
	Namfun_t *fp;
	for(fp = np->nvfun; fp; fp = fp->next)
	{
		if(fp->disc == &OPTIMIZE_disc)
		{
			optimize_clear(np,fp);
			return;
		}
	}
}

static void put_optimize(Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	nv_putv(np,val,flags,fp);
	optimize_clear(np,fp);
}

static Namfun_t *clone_optimize(Namval_t* np, Namval_t *mp, int flags, Namfun_t *fp)
{
	NOT_USED(np);
	NOT_USED(mp);
	NOT_USED(flags);
	NOT_USED(fp);
	return NULL;
}

const Namdisc_t OPTIMIZE_disc  = {sizeof(struct optimize),put_optimize,0,0,0,0,clone_optimize};

void nv_optimize(Namval_t *np)
{
	Namfun_t *fp;
	struct optimize *op, *xp = 0;
	if(nv_getoptimize())
	{
		if(np==SH_LINENO)
		{
			nv_setoptimize(NULL);
			return;
		}
		for(fp=np->nvfun; fp; fp = fp->next)
		{
			if(fp->disc && (fp->disc->getnum || fp->disc->getval))
			{
				nv_setoptimize(NULL);
				return;
			}
			if(fp->disc == &OPTIMIZE_disc)
				xp = (struct optimize*)fp;
		}
		if(xp && xp->ptr==nv_getoptimize())
			return;
		if(xp && xp->next)
		{
			struct optimize *xpn;
			for(xpn = xp->next; xpn; xpn = xpn->next)
				if(xpn->ptr == nv_getoptimize() && xpn->np == np)
					return;
		}
		if(op = opt_free)
			opt_free = op->next;
		else
			op=(struct optimize*)sh_calloc(1,sizeof(struct optimize));
		op->ptr = nv_getoptimize();
		op->np = np;
		if(xp)
		{
			op->hdr.disc = 0;
			op->next = xp->next;
			xp->next = op;
		}
		else
		{
			op->hdr.disc = &OPTIMIZE_disc;
			op->next = (struct optimize*)sh.optlist;
			sh.optlist = op;
			nv_stack(np,&op->hdr);
		}
	}
}

void sh_optclear(void *old)
{
	struct optimize *op,*opnext;
	for(op=(struct optimize*)sh.optlist; op; op = opnext)
	{
		opnext = op->next;
		if(op->ptr && op->hdr.disc)
		{
			nv_stack(op->np,&op->hdr);
			nv_stack(op->np,NULL);
		}
		op->next = opt_free;
		opt_free = op;
	}
	sh.optlist = old;
}

#endif /* SHOPT_OPTIMIZE */

/*
 *   Return a pointer to a character string that denotes the value
 *   of <np>.  If <np> refers to an array,  return a pointer to
 *   the value associated with the current index.
 *
 *   If the value of <np> is an integer, the string returned will
 *   be overwritten by the next call to nv_getval.
 *
 *   If <np> has no value, 0 is returned.
 */
char *nv_getval(Namval_t *np)
{
	void *vp = np->nvalue;	/* value pointer */
	size_t n;
	if(!nv_local && nv_getoptimize())
		nv_optimize(np);
	if((!np->nvfun || !np->nvfun->disc) && !nv_isattr(np,NV_ARRAY|NV_INTEGER|NV_FUNCT|NV_REF))
		goto done;
	if(nv_isref(np))
	{
		char *sub;
		if(!vp)
			return NULL;
		sh.last_table = nv_reftable(np);
		sub=nv_refsub(np);
		np = nv_refnode(np);
		if(sub)
		{
			sfprintf(sh.strbuf,"%s[%s]",nv_name(np),sub);
			return sfstruse(sh.strbuf);
		}
		return nv_name(np);
	}
	if(np->nvfun && np->nvfun->disc)
	{
		if(!nv_local)
		{
			nv_local=1;
			return nv_getv(np, np->nvfun);
		}
		nv_local=0;
	}
	if(nv_isattr(np,NV_INTEGER))
	{
		Sflong_t  ll;
		int base;
		if(!vp)
			return "0";
		if(nv_isattr(np,NV_DOUBLE)==NV_DOUBLE)
		{
			char *format;
			if(nv_isattr(np,NV_LONG) && sizeof(double)<sizeof(Sfdouble_t))
			{
				Sfdouble_t ld = *(Sfdouble_t*)vp;
				if(nv_isattr(np,NV_EXPNOTE))
					format = "%.*Lg";
				else if(nv_isattr(np,NV_HEXFLOAT))
					format = "%.*La";
				else
					format = "%.*Lf";
				sfprintf(sh.strbuf,format,nv_size(np),ld);
			}
			else
			{
				double d = *(double*)vp;
				if(nv_isattr(np,NV_EXPNOTE))
					format = "%.*g";
				else if(nv_isattr(np,NV_HEXFLOAT))
					format = "%.*a";
				else
					format = "%.*f";
				sfprintf(sh.strbuf,format,nv_size(np),d);
			}
			return sfstruse(sh.strbuf);
		}
		else if(nv_isattr(np,NV_UNSIGN))
		{
	        	if(nv_isattr(np,NV_LONG))
				ll = *(Sfulong_t*)vp;
			else if(nv_isattr(np,NV_SHORT))
				ll = *(uint16_t*)vp;
			else
				ll = *(uint32_t*)vp;
		}
		else if(nv_isattr(np,NV_LONG))
			ll = *(Sflong_t*)vp;
		else if(nv_isattr(np,NV_SHORT))
			ll = *(int16_t*)vp;
		else
			ll = *(uint32_t*)vp;
		base = nv_size(np);
		if(base==10)
			return fmtint(ll, nv_isattr(np,NV_UNSIGN));
		/* render a possibly signed non-base-10 integer with its base# prefix */
		sfprintf(sh.strbuf, nv_isattr(np,NV_UNSIGN) ? "%#..*I*u" : "%#..*I*d", base, sizeof ll, ll);
		return sfstruse(sh.strbuf);
	}
done:
	/*
	 * if NV_RAW flag is on, return pointer to binary data
	 * otherwise, base64 encode the data and return this string
	 */
	if(vp && nv_isattr(np,NV_BINARY) && !nv_isattr(np,NV_RAW))
	{
		char *cp;
		char *ep;
		int size= nv_size(np), insize=(4*size)/3+size/45+8;
		base64encode(vp, size, NULL, cp=getbuf(insize), insize, (void**)&ep);
		*ep = 0;
		return cp;
	}
	if(!nv_isattr(np,NV_LJUST|NV_RJUST) && (n = nv_size(np)) && vp && ((char*)vp)[n])
	{
		char *cp = getbuf(n + 1);
		memcpy(cp,vp,n);
		cp[n]=0;
		return cp;
	}
	return vp;
}

Sfdouble_t nv_getnum(Namval_t *np)
{
	void *vp;	/* value pointer */
	Sfdouble_t r=0;
	char *str;
	if(!nv_local && nv_getoptimize())
		nv_optimize(np);
	if(nv_istable(np))
	{
		errormsg(SH_DICT,ERROR_exit(1),e_number,nv_name(np));
		UNREACHABLE();
	}
     	if(np->nvfun && np->nvfun->disc)
	{
		if(!nv_local)
		{
			nv_local=1;
			return nv_getn(np, np->nvfun);
		}
		nv_local=0;
	}
	if(nv_isref(np))
	{
		str = nv_refsub(np);
		np = nv_refnode(np);
		if(str)
			nv_putsub(np,str,0L);
	}
     	if(nv_isattr(np,NV_INTEGER))
	{
		vp = np->nvalue;
		if(!vp || vp==Empty)
			r = 0;
		else if(nv_isattr(np, NV_DOUBLE)==NV_DOUBLE)
		{
			if(nv_isattr(np, NV_LONG))
	                       	r = *(Sfdouble_t*)vp;
			else
       	                	r = *(double*)vp;
		}
		else if(nv_isattr(np, NV_UNSIGN))
		{
			if(nv_isattr(np, NV_LONG))
				r = (Sflong_t)*(Sfulong_t*)vp;
			else if(nv_isattr(np, NV_SHORT))
				r = (Sflong_t)*(uint16_t*)vp;
			else
				r = *((uint32_t*)vp);
		}
		else
		{
			if(nv_isattr(np, NV_LONG))
				r = *(Sflong_t*)vp;
			else if(nv_isattr(np, NV_SHORT))
				r = *(int16_t*)vp;
			else
				r = *(int32_t*)vp;
		}
	}
	else if((str=nv_getval(np)) && *str!=0)
		r = sh_arith(str);
	return r;
}

/*
 *   Give <np> the attributes <newatts,> and change its current
 *   value to conform to <newatts>.  The <size> of left and right
 *   justified fields may be given.
 */
void nv_newattr (Namval_t *np, unsigned newatts, int size)
{
	char *sp;
	char *cp = 0;
	unsigned int n;
	Namval_t *mp = 0;
	Namarr_t *ap = 0;
	int oldsize,oldatts,trans;
	Namfun_t *fp= (newatts&NV_NODISC)?np->nvfun:0;
	char *prefix = sh.prefix;
	newatts &= ~NV_NODISC;

	/* check for restrictions */
	if(sh_isoption(SH_RESTRICTED) && ((sp=nv_name(np))==nv_name(PATHNOD) || sp==nv_name(SHELLNOD) || sp==nv_name(ENVNOD) || sp==nv_name(FPATHNOD)))
	{
		errormsg(SH_DICT,ERROR_exit(1),e_restricted,nv_name(np));
		UNREACHABLE();
	}
	/* handle attributes that do not change data separately */
	n = np->nvflag;
	trans = !(n&NV_INTEGER) && (n&(NV_LTOU|NV_UTOL)); /* transcode to lower or upper case */
	if(((n^newatts)&NV_EXPORT)) /* EXPORT attribute has been toggled */
	{
		/* record changes to the environment */
		if(n&NV_EXPORT)
		{
			/* EXPORT exists on old attributes therefore not on new */
			nv_offattr(np,NV_EXPORT);
			env_change();
		}
		else
		{
			/* EXPORT is now turned on for new attributes */
			nv_onattr(np,NV_EXPORT);
			env_change();
		}
		if((n^newatts)==NV_EXPORT && !trans)
			/* Only EXPORT attribute has changed and thus all work has been done. */
			return;
	}
	oldsize = nv_size(np);
	if((size==oldsize|| (n&NV_INTEGER)) && !trans && ((n^newatts)&~NV_NOCHANGE)==0)
	{
		if(size>0)
			np->nvsize = size;
		else if(size==NV_FLTSIZEZERO)
			np->nvsize = 0;
		nv_offattr(np, ~NV_NOFREE);
		nv_onattr(np, newatts);
		return;
	}
	if(size==NV_FLTSIZEZERO)
		size = 0;
	/* for an array, change all the elements */
	if((ap=nv_arrayptr(np)) && ap->nelem>0)
		nv_putsub(np,NULL,ARRAY_SCAN);
	oldsize = nv_size(np);
	oldatts = np->nvflag;
	if(fp)
		np->nvfun = 0;
	if(ap) /* add element to prevent array deletion */
	{
		ap->nelem++;
#if SHOPT_FIXEDARRAY
		if(ap->fixed)
		{
			nv_setsize(np,size);
			np->nvflag &= NV_ARRAY;
			np->nvflag |= newatts;
			goto skip;
		}
#endif /* SHOPT_FIXEDARRAY */
	}
	do
	{
		nv_setsize(np,oldsize);
		np->nvflag = oldatts;
		if (sp = nv_getval(np))
 		{
			if(nv_isattr(np,NV_ZFILL) && *sp=='0')
			{
				while(*sp=='0') sp++;	/* skip initial zeros */
				if(!*sp) sp--;		/* if number was 0, leave one zero */
			}
			n = strlen(sp);
			cp = (char*)sh_malloc((n >= (unsigned)size ? n : (unsigned)size) + 1);
			strcpy(cp, sp);
			if(sp && (mp=nv_opensub(np)))
			{
				if(trans)
				{
					nv_disc(np, &ap->hdr,NV_POP);
					nv_clone(np,mp,0);
					nv_disc(np, &ap->hdr,NV_FIRST);
					nv_offattr(mp,NV_ARRAY);
				}
				nv_newattr(mp,newatts&~NV_ARRAY,size);
			}
			if(!mp)
			{
				if(ap)
					ap->nelem &= ~ARRAY_SCAN;
				if(!trans)
					nv_unset(np,NV_RDONLY|NV_EXPORT);
				if(ap)
					ap->nelem |= ARRAY_SCAN;
			}
			if(size==0 && !(newatts&NV_INTEGER) && (newatts&NV_HOST)!=NV_HOST && (newatts&(NV_LJUST|NV_RJUST|NV_ZFILL)))
			{	/*
				 * Calculate the default terminal width for -L, -R, -Z if no numeric option-argument was given.
				 * Note: we count terminal positions, not characters (double-width adds 2, control char adds 0)
				 */
				char *cq = cp;
				wchar_t c;
				int w;
				n = 0;
				while(c = mbchar(cq))
					if ((w = mbwidth(c)) > 0)
						n += w;
				size = n;
			}
		}
		else if(!trans)
			nv_unset(np,NV_EXPORT);
		nv_setsize(np,size);
		np->nvflag &= (NV_ARRAY|NV_NOFREE);
		np->nvflag |= newatts;
		if (cp)
		{
			if(!mp)
				nv_putval (np, cp, NV_RDONLY);
			free(cp);
			cp = 0;
		}
		if(sh.subshell && !sh.subshare && nv_isattr(np,NV_ARRAY|NV_RDONLY)==NV_ARRAY && sp && sp!=Empty && sp!=AltEmpty && array_assoc(ap))
			free(sp);
	}
	while(ap && nv_nextsub(np));
#if SHOPT_FIXEDARRAY
skip:
#endif /* SHOPT_FIXEDARRAY */
	if(fp)
		np->nvfun = fp;
	if(ap)
		ap->nelem--;
	sh.prefix = prefix;
	return;
}

static inline char *oldgetenv(const char *string)
{
	char c0,c1;
	const char *cp, *sp;
	char **av = environ;
	if(!string || (c0= *string)==0)
		return NULL;
	if((c1=*++string)==0)
		c1= '=';
	while(cp = *av++)
	{
		if(cp[0]!=c0 || cp[1]!=c1)
			continue;
		sp = string;
		while(*sp && *sp++ == *++cp);
		if(*sp==0 && *++cp=='=')
			return (char*)(cp+1);
	}
	return NULL;
}

/*
 * This version of getenv uses the nval(3) storage to access environment values
 */
char *sh_getenv(const char *name)
{
	Namval_t *np, *savns;
	char *cp, *savpr;
	/* do not read from nval(3) storage before or during initialization */
	if(!sh.var_tree || sh_isstate(SH_INIT))
		return oldgetenv(name);
	/* deactivate a possible namespace or compound assignment */
	savns = sh.namespace, savpr = sh.prefix;
	sh.namespace = NULL, sh.prefix = NULL;
	if((np = nv_search(name,sh.var_tree,0)) && nv_isattr(np,NV_EXPORT))
		cp = nv_getval(np);
	else
		cp = NULL;
	sh.namespace = savns, sh.prefix = savpr;
	return cp;
}

/*
 * Some dynamic linkers will make this file see the libc getenv(),
 * so sh_getenv() is used for the astintercept() callback.  Plain
 * getenv() is provided for static links.
 */
char *getenv(const char *name)
{
	return sh_getenv(name);
}

#undef putenv
/*
 * This version of putenv uses the nval(3) storage to assign environment values
 */
int putenv(const char *name)
{
	Namval_t *np;
	if(name)
	{
		Namval_t *savns = sh.namespace;
		char *savpr = sh.prefix;
		/* deactivate a possible namespace or compound assignment */
		sh.namespace = NULL, sh.prefix = NULL;
		np = nv_open(name,sh.var_tree,NV_EXPORT|NV_IDENT|NV_NOARRAY|NV_ASSIGN);
		if(!strchr(name,'='))
			nv_unset(np,0);
		sh.namespace = savns, sh.prefix = savpr;
	}
	return 0;
}

/*
 * Override libast setenviron().
 */
char* sh_setenviron(const char *name)
{
	Namval_t *np;
	if(name)
	{
		Namval_t *savns = sh.namespace;
		char *savpr = sh.prefix;
		/* deactivate a possible namespace or compound assignment */
		sh.namespace = NULL, sh.prefix = NULL;
		np = nv_open(name,sh.var_tree,NV_EXPORT|NV_IDENT|NV_NOARRAY|NV_ASSIGN);
		sh.namespace = savns, sh.prefix = savpr;
		if(strchr(name,'='))
			return nv_getval(np);
		nv_unset(np,0);
	}
	return "";
}

/*
 * Same linker dance as with getenv() above.
 */
char* setenviron(const char *name)
{
	return sh_setenviron(name);
}

/*
 * normalize <cp> and return pointer to subscript if any
 * if <eq> is specified, return pointer to first = not in a subscript
 */
static char *lastdot(char *cp, int eq)
{
	char *ep=0;
	int c;
	if(eq)
		cp++;
	while(c= mbchar(cp))
	{
		if(c=='[')
		{
			if(*cp==']')
				cp++;
			else
				cp = nv_endsubscript(NULL,(ep=cp)-1,0);
		}
		else if(c=='.')
		{
			if(*cp=='[')
			{
				cp = nv_endsubscript(NULL,ep=cp,0);
				if((ep=sh_checkid(ep+1,cp)) < cp)
					cp=strcpy(ep,cp);
			}
			ep = 0;
		}
		else if(eq && c == '=')
			return cp-1;
	}
	return eq?0:ep;
}

int nv_rename(Namval_t *np, int flags)
{
	Namval_t		*mp=0,*nr=0;
	char			*cp;
	int			arraynp=0,arraynr,index= -1;
	Namval_t		*last_table = sh.last_table;
	Dt_t			*last_root = sh.last_root;
	Dt_t			*hp = 0;
	void			*nvmeta = NULL;
	char			*prefix = sh.prefix;
	Namarr_t		*ap;
	if(nv_isattr(np,NV_PARAM) && sh.st.prevst)
	{
		if(!(hp=(Dt_t*)sh.st.prevst->save_tree))
			hp = dtvnext(sh.var_tree);
	}
	if(!nv_isattr(np,NV_MINIMAL))
		nvmeta = np->nvmeta;
	if(nvmeta || (cp = nv_name(np)) && nv_isarray(np) && cp[strlen(cp)-1] == ']')
		arraynp = 1;
	if(!(cp=nv_getval(np)))
	{
		if(flags&NV_MOVE)
		{
			errormsg(SH_DICT,ERROR_exit(1),e_varname,"");
			UNREACHABLE();
		}
		return 0;
	}
	if(lastdot(cp,0) && nv_isattr(np,NV_MINIMAL))
	{
		errormsg(SH_DICT,ERROR_exit(1),e_varname,nv_name(np));
		UNREACHABLE();
	}
	arraynr = cp[strlen(cp)-1] == ']';
	if(nv_isarray(np) && !(mp=nv_opensub(np)))
		index=nv_aindex(np);
	sh.prefix = 0;
	if(!hp)
		hp = sh.var_tree;
	if(!(nr = nv_open(cp, hp, flags|NV_ARRAY|NV_NOSCOPE|NV_NOADD|NV_NOFAIL)))
	{
#if SHOPT_NAMESPACE
		if(sh.namespace)
			hp = nv_dict(sh.namespace);
		else
#endif /* SHOPT_NAMESPACE */
		hp = sh.var_base;
	}
	else if(sh.last_root)
		hp = sh.last_root;
	if(!nr)
		nr= nv_open(cp, hp, flags|NV_NOREF|((flags&NV_MOVE)?0:NV_NOFAIL));
	sh.prefix = prefix;
	if(!nr)
	{
		if(!nv_isvtree(np))
			nv_unset(np,0);
		return 0;
	}
	if(!mp && index>=0 && nv_isvtree(nr))
	{
		sfprintf(sh.strbuf,"%s[%d]",nv_name(np),index);
		/* create a virtual node */
		if(mp = nv_open(sfstruse(sh.strbuf),sh.var_tree,NV_VARNAME|NV_ADD|NV_ARRAY))
		{
			if(ap = nv_arrayptr(np))
				ap->nelem++;
			mp->nvmeta = nvmeta = np;
		}
	}
	if(mp)
	{
		nvmeta = np;
		np = mp;
	}
	if(nr==np)
	{
		if(index<0)
			return 0;
		if(cp = nv_getval(np))
			cp = sh_strdup(cp);
	}
	nv_unset(np,NV_EXPORT);
	if(nr==np)
	{
		nv_putsub(np,NULL, index);
		nv_putval(np,cp,0);
		free(cp);
		return 1;
	}
	sh.prev_table = sh.last_table;
	sh.prev_root = sh.last_root;
	sh.last_table = last_table;
	sh.last_root = last_root;
	if(flags&NV_MOVE)
	{
		if(arraynp && !nv_isattr(np,NV_MINIMAL) && (mp = np->nvmeta) && (ap = nv_arrayptr(mp)) && !ap->fun)
			ap->nelem++;
	}
	if((nv_arrayptr(nr) && !arraynr) || nv_isvtree(nr))
	{
		if(ap=nv_arrayptr(np))
		{
			if(!ap->table)
				ap->table = dtopen(&_Nvdisc,Dtoset);
			if(ap->table)
				mp = nv_search(nv_getsub(np),ap->table,NV_ADD);
			nv_arraychild(np,mp,0);
			nvmeta = np;
		}
		else
			mp = np;
		if(nr==SH_MATCHNOD)
		{
			Sfio_t *iop;
			Dt_t *save_root = sh.var_tree;
			int trace = sh_isoption(SH_XTRACE);
			sfprintf(sh.strbuf,"typeset -a %s=",nv_name(mp));
			nv_outnode(nr,sh.strbuf,-1,0);
			sfwrite(sh.strbuf,")\n",2);
			cp = sfstruse(sh.strbuf);
			iop = sfopen(NULL,cp,"s");
			if(trace)
				sh_offoption(SH_XTRACE);
			sh.var_tree = last_root;
			sh_eval(iop,SH_READEVAL);
			sh.var_tree = save_root;
			if(trace)
				sh_onoption(SH_XTRACE);
			if(flags&NV_MOVE)
				sh_setmatch(0,0,0,0,0);
		}
		else
			nv_clone(nr,mp,(flags&NV_MOVE)|NV_COMVAR);
		mp->nvmeta = nvmeta;
		if(flags&NV_MOVE)
		{
			if(arraynr && !nv_isattr(nr,NV_MINIMAL) && (mp = nr->nvmeta) && (ap = nv_arrayptr(mp)))
			{
				nv_putsub(mp,nr->nvname,0);
				nv_unset(mp,0);
			}
			nv_delete(nr,NULL,NV_NOFREE);
		}
	}
	else
	{
		nv_putval(np,nv_getval(nr),0);
		if(flags&NV_MOVE)
		{
			if(!nv_isattr(nr,NV_MINIMAL) && (mp = nr->nvmeta) && (ap = nv_arrayptr(mp)))
				ap->nelem--;
			nv_unset(nr,0);
		}
	}
	return 1;
}

/*
 * Create a reference node from <np> to $np in dictionary <hp>
 */
void nv_setref(Namval_t *np, Dt_t *hp, int flags)
{
	Namval_t	*nq=0, *nr=0;
	char		*ep,*cp;
	Dt_t		*root = sh.last_root, *hpnext=0;
	Namarr_t	*ap=0;
	Dt_t		*openmatch;
	struct Namref	*nrp;
	if(nv_isref(np))
		return;
	if(nv_isarray(np))
	{
		errormsg(SH_DICT,ERROR_exit(1),e_badref,nv_name(np));
		UNREACHABLE();
	}
	if(!(cp=nv_getval(np)))
	{
		nv_unset(np,0);
		nv_onattr(np,NV_REF);
		return;
	}
	if((ep = lastdot(cp,0)) && nv_isattr(np,NV_MINIMAL))
	{
		errormsg(SH_DICT,ERROR_exit(1),e_badref,nv_name(np));
		UNREACHABLE();
	}
	if(hp)
		hpnext = dtvnext(hp);
	if((nr=nv_open(cp, hp?hp:sh.var_tree, flags|NV_NOSCOPE|NV_NOADD|NV_NOFAIL)))
		nq = nr;
	else if(hpnext && dtvnext(hpnext)==sh.var_base && (nr=nv_open(cp,hpnext,flags|NV_NOSCOPE|NV_NOADD|NV_NOFAIL)))
		nq = nr;
	else if((openmatch=sh.openmatch) && hpnext==sh.var_base && (nr=nv_open(cp,hpnext,flags|NV_NOSCOPE|NV_NOADD|NV_NOFAIL)))
		nq = nr;
	if(nq)
		hp = sh.last_root;
	else
		hp = hp?(openmatch?openmatch:sh.var_base):sh.var_tree;
	if(nr==np)
	{
		if(sh.namespace && nv_dict(sh.namespace)==hp)
		{
			errormsg(SH_DICT,ERROR_exit(1),e_selfref,nv_name(np));
			UNREACHABLE();
		}
		/* bind to earlier scope, or add to global scope */
		if(!(hp=dtvnext(hp)) || (nq=nv_search((char*)np,hp,NV_ADD|NV_REF))==np)
		{
			errormsg(SH_DICT,ERROR_exit(1),e_selfref,nv_name(np));
			UNREACHABLE();
		}
		if(nv_isarray(nq))
			nv_putsub(nq,NULL,ARRAY_UNDEF);
	}
#if SHOPT_FIXEDARRAY
	if(nq && ep && nv_isarray(nq) && !((ap=nv_arrayptr(nq)) && ap->fixed) && !nv_getsub(nq))
#else
	if(nq && ep && nv_isarray(nq) && !nv_getsub(nq))
#endif /* SHOPT_FIXEDARRAY */
	{
		if(!nv_arrayptr(nq))
		{
			nv_putsub(nq,"1",ARRAY_FILL);
			nv_unset(nq,NV_RDONLY);
		}
		nv_endsubscript(nq,ep-1,NV_ARRAY);
	}
	if(!nr)
	{
		sh.last_root = 0;
		nr= nq = nv_open(cp, hp, flags);
		if(sh.last_root)
			hp = sh.last_root;
	}
	if(sh.last_root == sh.var_tree && root!=sh.var_tree)
	{
		nv_unset(np,NV_RDONLY);
		nv_onattr(np,NV_REF);
		errormsg(SH_DICT,ERROR_exit(1),e_globalref,nv_name(np));
		UNREACHABLE();
	}
	sh.instance = 1;
	if(nq && !ep && (ap=nv_arrayptr(nq)) && !(ap->nelem&(ARRAY_UNDEF|ARRAY_SCAN)))
		ep =  nv_getsub(nq);
#if SHOPT_FIXEDARRAY
	if(ep && !(ap && ap->fixed))
#else
	if(ep)
#endif /* SHOPT_FIXEDARRAY */
	{
		/* cause subscript evaluation and return result */
		if(nv_isarray(nq))
			ep = nv_getsub(nq);
		else
		{
			int n;
			ep[n=strlen(ep)-1] = 0;
			nv_putsub(nr, ep, ARRAY_FILL);
			ep[n] = ']';
			if(nq = nv_opensub(nr))
				ep = 0;
			else
				ep = nv_getsub(nq=nr);
		}
	}
	sh.instance = 0;
	sh.last_root = root;
	nv_unset(np,0);
	nv_delete(np,NULL,NV_REF);
	nrp = np->nvalue = sh_newof(0,struct Namref,1,sizeof(Dtlink_t));
	nrp->np = nq;
	nrp->root = hp;
	if(ep)
	{
#if SHOPT_FIXEDARRAY
		if(ap && ap->fixed)
			nrp->curi = ARRAY_FIXED|nv_arrfixed(nq,NULL,1,&nrp->dim);
		else
#endif /* SHOPT_FIXEDARRAY */
			nrp->sub = sh_strdup(ep);
	}
	nrp->table = sh.last_table;
	nv_onattr(np,NV_REF|NV_NOFREE);
	if(!Refdict)
	{
		NullNode.nvname = ".deleted";
		NullNode.nvflag = NV_RDONLY;
		Refdict = dtopen(&_Refdisc,Dtobag);
	}
	dtinsert(Refdict,nrp);
}

/*
 * get the scope corresponding to <index>
 * whence uses the same values as lseek()
 */
Shscope_t *sh_getscope(int index, int whence)
{
	struct sh_scoped *sp, *topmost;
	if(whence==SEEK_CUR)
		sp = &sh.st;
	else
	{
		if ((struct sh_scoped*)sh.topscope != sh.st.self)
			topmost = (struct sh_scoped*)sh.topscope;
		else
			topmost = &(sh.st);
		sp = topmost;
		if(whence==SEEK_SET)
		{
			int n =0;
			while(sp = sp->prevst)
				n++;
			index = n - index;
			sp = topmost;
		}
	}
	if(index < 0)
		return NULL;
	while(index-- && (sp = sp->prevst));
	return (Shscope_t*)sp;
}

/*
 * make <scoped> the top scope and return previous scope
 */
Shscope_t *sh_setscope(Shscope_t *scope)
{
	Shscope_t *old = (Shscope_t*)sh.st.self;
	*sh.st.self = sh.st;
	sh.st = *((struct sh_scoped*)scope);
	sh.var_tree = scope->var_tree;
	SH_PATHNAMENOD->nvalue = sh.st.filename;
	SH_FUNNAMENOD->nvalue = sh.st.funname;
	error_info.id = scope->cmdname;
	return old;
}

void sh_unscope(void)
{
	Dt_t *root = sh.var_tree;
	Dt_t *dp = dtview(root,NULL);
	if(dp)
	{
		table_unset(root,NV_RDONLY|NV_NOSCOPE,dp);
		if(sh.st.real_fun && dp==sh.st.real_fun->sdict)
		{
			dp = dtview(dp,NULL);
			sh.st.real_fun->sdict->view = dp;
		}
		sh.var_tree=dp;
		dtclose(root);
	}
}

/*
 * The inverse of creating a reference node
 */
void nv_unref(Namval_t *np)
{
	struct Namref *nrp = np->nvalue;
	Namval_t *nq;
	if(!nv_isref(np))
		return;
	nv_offattr(np,NV_NOFREE|NV_REF);
	if(!nrp)
		return;
	nq = nv_refnode(np);
	if(Refdict)
	{
		if(nrp->sub)
			free(nrp->sub);
		dtremove(Refdict,nrp);
	}
	free(nrp);
	np->nvalue = sh_strdup(nv_name(nq));
	nv_optimize_clear(nq);
}

char *nv_name(Namval_t *np)
{
	Namval_t *table;
	Namfun_t *fp;
#if SHOPT_FIXEDARRAY
	Namarr_t *ap = NULL;
#endif /* SHOPT_FIXEDARRAY */
	char *cp;
	if(is_abuiltin(np) || is_afunction(np))
	{
#if SHOPT_NAMESPACE
		if(sh.namespace && is_afunction(np))
		{
			char *name = nv_name(sh.namespace);
			int n = strlen(name);
			if(strncmp(np->nvname,name,n)==0 && np->nvname[n]=='.')
				return np->nvname+n+1;
		}
#endif /* SHOPT_NAMESPACE */
		return np->nvname;
	}
	if(!np->nvname)
		goto skip;
#if SHOPT_FIXEDARRAY
	ap = nv_arrayptr(np);
#endif /* SHOPT_FIXEDARRAY */
	if(!nv_isattr(np,NV_MINIMAL|NV_EXPORT) && np->nvmeta)
	{
		Namval_t *nq = sh.last_table, *mp = np->nvmeta;
		if(np==sh.last_table)
			sh.last_table = 0;
		if(nv_isarray(mp))
			sfprintf(sh.strbuf,"%s[%s]",nv_name(mp),np->nvname);
		else
			sfprintf(sh.strbuf,"%s.%s",nv_name(mp),np->nvname);
		sh.last_table = nq;
		return sfstruse(sh.strbuf);
	}
	if(nv_istable(np))
		sh.last_table = nv_parent(np);
	else if(!nv_isref(np))
	{
	skip:
		for(fp= np->nvfun ; fp; fp=fp->next)
		if(fp->disc && fp->disc->namef)
		{
			if(np==sh.last_table)
				sh.last_table = 0;
			return (*fp->disc->namef)(np,fp);
		}
	}
	if(!(table=sh.last_table) || np->nvname && *np->nvname=='.' || table==sh.namespace || np==table)
	{
#if SHOPT_FIXEDARRAY
		if(!ap || !ap->fixed || (ap->nelem&ARRAY_UNDEF))
			return np->nvname;
		table = 0;
#else
		return np->nvname;
#endif /* SHOPT_FIXEDARRAY */
	}
	if(table)
	{
		cp = nv_name(table);
		sfprintf(sh.strbuf,"%s.%s",cp,np->nvname);
	}
	else
		sfprintf(sh.strbuf,"%s",np->nvname);
#if SHOPT_FIXEDARRAY
	if(ap && ap->fixed)
		nv_arrfixed(np,sh.strbuf,1,NULL);
#endif /* SHOPT_FIXEDARRAY */
	return sfstruse(sh.strbuf);
}

Namval_t *nv_lastdict(void)
{
	return sh.last_table;
}

/*
 * libshell compatibility functions
 */
#define BYPASS_MACRO

void *nv_context BYPASS_MACRO (Namval_t *np)
{
	return nv_context(np);
}

int nv_isnull BYPASS_MACRO (Namval_t *np)
{
	return nv_isnull(np);
}

int nv_setsize BYPASS_MACRO (Namval_t *np, int size)
{
	int oldsize = nv_size(np);
	if(size>=0)
		np->nvsize = size;
	return oldsize;
}

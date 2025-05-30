/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2025 Contributors to ksh 93u+m           *
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
*                                                                      *
***********************************************************************/
/*
 * David Korn
 * AT&T Labs
 *
 */
#include	"shopt.h"
#include        "defs.h"
#include        "io.h"
#include        "variables.h"
#include	"builtins.h"

static const char sh_opttype[] =
"[-1c?\n@(#)$Id: type (ksh 93u+m) 2024-08-28 $\n]"
"[--catalog?" SH_DICT "]"
"[+NAME?\f?\f - set the type of variables to \b\f?\f\b]"
"[+DESCRIPTION?\b\f?\f\b sets the type on each of the variables specified "
	"by \aname\a to \b\f?\f\b. If \b=\b\avalue\a is specified, "
	"the variable \aname\a is set to \avalue\a before the variable "
	"is converted to \b\f?\f\b.]"
"[+?If no \aname\as are specified then the names and values of all "
	"variables of this type are written to standard output.]"
"[+?\b\f?\f\b is built in to the shell as a declaration command so that "
	"field splitting and pathname expansion are not performed on "
	"the arguments. Tilde expansion occurs on \avalue\a.]"
"[r?Enables readonly. Once enabled, the value cannot be changed or unset.]"
"[a]:?[type?Indexed array. Each \aname\a is converted to an indexed "
	"array of type \b\f?\f\b. If a variable already exists, the current "
	"value will become index \b0\b. If \b[\b\atype\a\b]]\b is "
	"specified, each subscript is interpreted as a value of enumeration "
	"type \atype\a.]"
"[A?Associative array. Each \aname\a is converted to an associative "
	"array of type \b\f?\f\b. If a variable already exists, the current "
	"value will become subscript \b0\b.]"
"[h]:[string?Used within a type definition to provide a help string  "
	"for variable \aname\a. Otherwise, it is ignored.]"
"[S?Used with a type definition to indicate that the variable is shared by "
	"each instance of the type. When used inside a function defined "
	"with the \bfunction\b reserved word, the specified variables "
	"will have function static scope. Otherwise, the variable is "
	"unset prior to processing the assignment list.]"
"[+DETAILS]\ftypes\f"
"\n"
"\n[name[=value]...]\n"
"\n"
"[+EXIT STATUS?]{"
	"[+0?Successful completion.]"
	"[+>0?An error occurred.]"
"}"

"[+SEE ALSO?\fother\f \breadonly\b(1), \btypeset\b(1)]"
;

typedef struct Namtype Namtype_t;
typedef struct Namchld
{
	Namfun_t	fun;
	Namtype_t 	*ptype;
	Namtype_t 	*ttype;
} Namchld_t;

struct Namtype
{
	Namfun_t	fun;
	Namval_t	*np;
	Namval_t	*parent;
	Namval_t	*bp;
	Namval_t	*cp;
#if SHOPT_NAMESPACE
	Namval_t	*nsp;
#endif /* SHOPT_NAMESPACE */
	char		*nodes;
	char		*data;
	Namchld_t	childfun;
	int		numnodes;
	char		**names;
	size_t		dsize;
	short		strsize;
	unsigned short	ndisc;
	unsigned short	current;
	unsigned short	nref;
};

typedef struct
{
	char		_cSfdouble_t;
	Sfdouble_t	_dSfdouble_t;
	char		_cdouble;
	double		_ddouble;
	char		_cfloat;
	float		_dfloat;
	char		_cSflong_t;
	Sflong_t	_dSflong_t;
	char		_cint32_t;
	int32_t		_dint32_t;
	char		_cint16_t;
	int16_t		_dint16_t;
	char		_cpointer;
	char		*_dpointer;
} _Align_;

#define alignof(t)	( offsetof(_Align_, _d##t) - offsetof(_Align_, _c##t) )

static void put_type(Namval_t*, const char*, int, Namfun_t*);
static Namval_t* create_type(Namval_t*, const char*, int, Namfun_t*);
static Namfun_t* clone_type(Namval_t*, Namval_t*, int, Namfun_t*);
static Namval_t* next_type(Namval_t*, Dt_t*, Namfun_t*);

static const Namdisc_t type_disc =
{
	sizeof(Namtype_t),
	put_type,
	0,
	0,
	0,
	create_type,
	clone_type,
	0,
	next_type,
	0,
};

size_t nv_datasize(Namval_t *np, size_t *offset)
{
	size_t s=0, a=0;
	if(nv_isattr(np,NV_INTEGER))
	{
		if(nv_isattr(np,NV_DOUBLE)==NV_DOUBLE)
		{
			if(nv_isattr(np, NV_LONG))
			{
				a = alignof(Sfdouble_t);
				s = sizeof(Sfdouble_t);
			}
			else if(nv_isattr(np, NV_SHORT))
			{
				a = alignof(float);
				s = sizeof(float);
			}
			else
			{
				a = alignof(double);
				s = sizeof(double);
			}
		}
		else
		{
			if(nv_isattr(np, NV_LONG))
			{
				a = alignof(Sflong_t);
				s = sizeof(Sflong_t);
			}
			else if(nv_isattr(np, NV_SHORT))
			{
				a = alignof(int16_t);
				s = sizeof(int16_t);
			}
			else
			{
				a = alignof(int32_t);
				s = sizeof(int32_t);
			}
		}
	}
	else if(nv_isattr(np, NV_BINARY) || nv_isattr(np,NV_LJUST|NV_RJUST|NV_ZFILL))
		s = nv_size(np);
	else
	{
		a = alignof(pointer);
		s = nv_size(np);
	}
	if(a>1 && offset)
		*offset = a*((*offset +a-1)/a);
	return s;
}

static char *name_chtype(Namval_t *np, Namfun_t *fp)
{
	Namchld_t	*pp = (Namchld_t*)fp;
	char		*cp, *sub;
	Namval_t	*tp = sh.last_table;
	Namval_t	*nq = pp->ptype->np;
	Namarr_t	*ap;
	if(nv_isattr(np,NV_REF|NV_TAGGED)==(NV_REF|NV_TAGGED))
		sh.last_table = 0;
	cp = nv_name(nq);
	if((ap = nv_arrayptr(nq)) && !(ap->nelem&ARRAY_UNDEF) && (sub= nv_getsub(nq)))
		sfprintf(sh.strbuf,"%s[%s].%s",cp,sub,np->nvname);
	else
		sfprintf(sh.strbuf,"%s.%s",cp,np->nvname);
#if SHOPT_FIXEDARRAY
	if((ap=nv_arrayptr(np)) && ap->fixed)
		nv_arrfixed(np,sh.strbuf,1,NULL);
#endif /* SHOPT_FIXEDARRAY */
	sh.last_table = tp;
	return sfstruse(sh.strbuf);
}

static void put_chtype(Namval_t* np, const char* val, int flag, Namfun_t* fp)
{
	if(!val && nv_isattr(np,NV_REF))
		return;
	nv_putv(np,val,flag,fp);
	if(!val)
	{
		Namchld_t	*pp = (Namchld_t*)fp;
		size_t		dsize=0,offset = (char*)np-(char*)pp->ptype;
		Namval_t	*mp = (Namval_t*)((char*)pp->ttype+offset);
		dsize = nv_datasize(mp,&dsize);
		if((char*)mp->nvalue >= pp->ttype->data && (char*)mp->nvalue < (char*)pp+pp->ttype->fun.dsize)
		{
			np->nvalue = pp->ptype->data + ((char*)mp->nvalue - pp->ptype->data);
			if(np->nvalue != mp->nvalue)
				memcpy(np->nvalue,mp->nvalue,dsize);
		}
		else if(!nv_isarray(mp) && mp->nvalue)
		{
			np->nvalue = mp->nvalue;
			nv_onattr(np,NV_NOFREE);
		}
		np->nvsize = mp->nvsize;
		np->nvflag = mp->nvflag&~NV_RDONLY;
	}
}

static Namfun_t *clone_chtype(Namval_t* np, Namval_t *mp, int flags, Namfun_t *fp)
{
	NOT_USED(np);
	NOT_USED(mp);
	if(flags&NV_NODISC)
		return NULL;
	return nv_clone_disc(fp,flags);
}

static const Namdisc_t chtype_disc =
{
	sizeof(Namchld_t),
	put_chtype,
	0,
	0,
	0,
	0,
	clone_chtype,
	name_chtype
};

static Namval_t *findref(void *nodes, int n)
{
	Namval_t	*tp,*np = nv_namptr(nodes,n);
	char		*name = np->nvname;
	int		i=n, len= strrchr(name,'.')-name;
	Namtype_t	*pp;
	while(--i>0)
	{
		np = nv_namptr(nodes,i);
		if(np->nvname[len]==0)
		{
			tp = nv_type(np);
			pp = (Namtype_t*)nv_hasdisc(tp,&type_disc);
			return nv_namptr(pp->nodes,n-i-1);
		}
	}
	return NULL;
}

static int fixnode(Namtype_t *dp, Namtype_t *pp, int i, struct Namref *nrp,int flag)
{
	Namval_t	*nq = nv_namptr(dp->nodes,i);
	Namfun_t	*fp;
	if(fp=nv_hasdisc(nq,&chtype_disc))
		nv_disc(nq, fp, NV_POP);
	if(nv_isattr(nq,NV_REF))
	{
		struct Namref *nrp2;
		nq->nvalue = nrp2 = nrp++;
		nv_setsize(nq,0);
		if(strchr(nq->nvname,'.'))
			nrp2->np = findref(dp->nodes,i);
		else
			nrp2->np = nv_namptr(pp->childfun.ttype->nodes,i);
		nrp2->root = sh.last_root;
		nrp2->table = pp->np;
		nq ->nvflag = NV_REF|NV_NOFREE|NV_MINIMAL;
		return 1;
	}
	if(nq->nvalue || nq->nvfun)
	{
		const char *data = nq->nvalue;
		if(nq->nvfun)
		{
			Namval_t *np = nv_namptr(pp->nodes,i);
			if(nv_isarray(nq))
				nq->nvalue = NULL;
			nq->nvfun = 0;
			if(nv_isarray(nq) && ((flag&NV_IARRAY) || nv_type(np)))
				clone_all_disc(np,nq,flag&~NV_TYPE);
			else
				clone_all_disc(np,nq,flag);
			if(fp)
				nv_disc(np, fp, NV_LAST);
		}
		if(data >=  pp->data && data < (char*)pp +pp->fun.dsize)
			nq->nvalue = dp->data + (data-pp->data);
		else if(!nq->nvfun && pp->childfun.ttype!=pp->childfun.ptype)
		{
			Namval_t *nr = nv_namptr( pp->childfun.ttype->nodes,i);
			if(nr->nvalue != nq->nvalue)
			{
				if(i=nv_size(nq))
				{
					void *vp = nq->nvalue;
					nq->nvalue = sh_malloc(i);
					memcpy(nq->nvalue,vp,i);
				}
				else
					nq->nvalue = sh_strdup(nq->nvalue);
				nv_offattr(nq,NV_NOFREE);
			}
		}
		else if(nq->nvalue==AltEmpty)
			nq->nvalue = Empty;
		else if(nq->nvalue==Empty)
			nv_offattr(nq,NV_NOFREE);
	}
	if(fp)
		nv_disc(nq, &dp->childfun.fun, NV_LAST);
	return 0;
}

static Namfun_t *clone_type(Namval_t* np, Namval_t *mp, int flags, Namfun_t *fp)
{
	Namtype_t		*dp, *pp=(Namtype_t*)fp;
	int			i;
	Namval_t		*nq, *nr;
	size_t			size = fp->dsize;
	int			save, offset=stktell(sh.stk);
	char			*cp;
	Dt_t			*root = sh.last_root;
	Namval_t		*last_table = sh.last_table;
	struct Namref		*nrp = 0;
	Namarr_t		*ap;
	if(flags&NV_MOVE)
	{
		pp->np = mp;
		pp->childfun.ptype = pp;
		return fp;
	}
	if(flags&NV_TYPE)
		return nv_clone_disc(fp,flags);
	if(size==0 && (!fp->disc || (size=fp->disc->dsize)==0))
		size = sizeof(Namfun_t);
	dp = (Namtype_t*)sh_malloc(size+pp->nref*sizeof(struct Namref));
	if(pp->nref)
	{
		nrp = (struct Namref*)((char*)dp + size);
		memset(nrp,0,pp->nref*sizeof(struct Namref));
	}
	memcpy(dp,pp,size);
	dp->parent = mp;
	dp->fun.nofree = (flags&NV_RDONLY?1:0);
	dp->np = mp;
	dp->childfun.ptype = dp;
	dp->nodes = (char*)(dp+1);
	dp->data = (char*)dp + (pp->data - (char*)pp);
	for(i=dp->numnodes; --i >= 0; )
	{
		nq = nv_namptr(dp->nodes,i);
		if(fixnode(dp,pp,i,nrp,NV_TYPE|(flags&NV_IARRAY)))
		{
			nrp++;
			nq = ((struct Namref*)nq->nvalue)->np;
		}
		if(flags==(NV_NOFREE|NV_ARRAY))
			continue;
		if(nq->nvalue || !nv_isvtree(nq) || nv_isattr(nq,NV_RDONLY))
		{
			/* see if default value has been overwritten */
			if(!mp->nvname)
				continue;
			sh.last_table = last_table;
			if(pp->strsize<0)
				cp = nv_name(np);
			else
				cp = nv_name(mp);
			sfputr(sh.stk,cp,'.');
			sfputr(sh.stk,nq->nvname,0);
			root = nv_dict(mp);
			save = fp->nofree;
			fp->nofree = 1;
			nr = nv_create(stkptr(sh.stk,offset),root,NV_VARNAME|NV_NOADD,fp);
			fp->nofree = save;
			stkseek(sh.stk,offset);
			if(nr)
			{
				if(nv_isattr(nq,NV_RDONLY) && (nq->nvalue || nv_isattr(nq,NV_INTEGER)))
				{
					errormsg(SH_DICT,ERROR_exit(1),e_readonly, nq->nvname);
					UNREACHABLE();
				}
				if(nv_isref(nq))
					nq = nv_refnode(nq);
				if((size = nv_datasize(nr,NULL)) && size==nv_datasize(nq,NULL))
					memcpy(nq->nvalue,nr->nvalue,size);
				else if(ap=nv_arrayptr(nr))
				{
					nv_putsub(nr,NULL,ARRAY_SCAN|ARRAY_NOSCOPE);
					do
					{
						if(array_assoc(ap))
							cp = (char*)((*ap->fun)(nr,NULL,NV_ANAME));
						else
							cp = nv_getsub(nr);
						nv_putsub(nq,cp,ARRAY_ADD|ARRAY_NOSCOPE);
						if(array_assoc(ap))
						{
							Namval_t *mr = (Namval_t*)((*ap->fun)(nr,NULL,NV_ACURRENT));
							Namval_t *mq = (Namval_t*)((*ap->fun)(nq,NULL,NV_ACURRENT));
							nv_clone(mr,mq,NV_MOVE);
							ap->nelem--;
							nv_delete(mr,ap->table,0);
						}
						else
						{
							cp = nv_getval(nr);
							nv_putval(nq,cp,0);
						}
					}
					while(nv_nextsub(nr));
				}
				else
					nv_putval(nq,nv_getval(nr),NV_RDONLY);
				if(sh.mktype)
					nv_addnode(nr,1);
				if(pp->strsize<0)
					continue;
				nv_unset(nr,0);
				if(!nv_isattr(nr,NV_MINIMAL))
					nv_delete(nr,sh.last_root,0);
			}
			else if(nv_isattr(nq,NV_RDONLY) && !nq->nvalue && !nv_isattr(nq,NV_INTEGER))
			{
				errormsg(SH_DICT,ERROR_exit(1),e_required,nq->nvname,nv_name(mp));
				UNREACHABLE();
			}
		}
	}
	if(nv_isattr(mp,NV_BINARY))
		mp->nvalue = dp->data;
	if(pp->strsize<0)
		dp->strsize = -pp->strsize;
	return &dp->fun;
}


/*
 * return Namval_t* corresponding to child <name> in <np>
 */
static Namval_t *create_type(Namval_t *np,const char *name,int flag,Namfun_t *fp)
{
	Namtype_t		*dp = (Namtype_t*)fp;
	const char		*cp=name;
	int			i=0,n;
	Namval_t		*nq=0;
	NOT_USED(flag);
	if(!name)
		return dp->parent;
	while((n=*cp++) && n != '=' && n != '+' && n!='[');
	n = (cp-1) -name;
	if(dp->numnodes && dp->strsize<0)
	{
		char *base =  (char*)np-(NV_MINSZ-sizeof(Dtlink_t));
		size_t m=strlen(np->nvname);
		while((nq=nv_namptr(base,++i)) && strncmp(nq->nvname,np->nvname,m)==0)
		{
			if(nq->nvname[m]=='.' && strncmp(name,&nq->nvname[m+1],n)==0 && nq->nvname[m+n+1]==0)
				goto found;
		}
		nq = 0;
	}
	else for(i=0; i < dp->numnodes; i++)
	{
		nq = nv_namptr(dp->nodes,i);
		if((n==0||strncmp(name,nq->nvname,n)==0) && nq->nvname[n]==0)
		{
			while(nv_isref(nq))
				nq = ((struct Namref*)nq->nvalue)->np;
			goto found;
		}
	}
	nq = 0;
found:
	if(nq)
	{
		fp->last = (char*)&name[n];
		sh.last_table = dp->parent;
	}
	else
	{
		if(name[n]!='=') for(i=0; i < dp->ndisc; i++)
		{
			if((strncmp(name,dp->names[i],n)==0) && dp->names[i][n]==0)
				return nq;
		}
		errormsg(SH_DICT,ERROR_exit(1),e_notelem,n,name,nv_name(np));
		UNREACHABLE();
	}
	return nq;
}

static void put_type(Namval_t* np, const char* val, int flag, Namfun_t* fp)
{
	Namval_t	*nq;
	if(val && (nq=nv_open(val,sh.var_tree,NV_VARNAME|NV_ARRAY|NV_NOADD|NV_NOFAIL)))
	{
		Namfun_t  *pp;
		if((pp=nv_hasdisc(nq,fp->disc)) && pp->type==fp->type)
		{
			if(!nq->nvmeta)
				flag |= NV_EXPORT;
			nv_unset(np, flag);
			nv_clone(nq,np,NV_IARRAY);
			return;
		}
	}
	nv_putv(np,val,flag,fp);
	if(!val)
	{
		Namtype_t	*dp = (Namtype_t*)fp;
		Namval_t	*nq;
		Namarr_t	*ap;
		int		i;
		if(nv_isarray(np) && (ap=nv_arrayptr(np)) && ap->nelem>0)
			return;
		for(i=0; i < dp->numnodes; i++)
		{
			nq = nv_namptr(dp->nodes,i);
			if(ap=nv_arrayptr(nq))
				ap->nelem |= ARRAY_UNDEF;
			if(!nv_hasdisc(nq,&type_disc))
				nv_unset(nq,flag|NV_TYPE|nv_isattr(nq,NV_RDONLY));
		}
		nv_disc(np,fp,NV_POP);
		if(!(fp->nofree&1))
			free(fp);
	}
}

static Namval_t *next_type(Namval_t* np, Dt_t *root,Namfun_t *fp)
{
	Namtype_t	*dp = (Namtype_t*)fp;
	if(!root)
	{
		Namarr_t	*ap = nv_arrayptr(np);
		if(ap && (ap->nelem&ARRAY_UNDEF))
			nv_putsub(np,NULL,ARRAY_SCAN);
		dp->current = 0;
	}
	else if(++dp->current>=dp->numnodes)
		return NULL;
	return nv_namptr(dp->nodes,dp->current);
}

static Namfun_t *clone_inttype(Namval_t* np, Namval_t *mp, int flags, Namfun_t *fp)
{
	Namfun_t	*pp = (Namfun_t*)sh_malloc(fp->dsize);
	NOT_USED(flags);
	memcpy(pp, fp, fp->dsize);
	fp->nofree &= ~1;
	if(nv_isattr(mp,NV_NOFREE) && mp->nvalue)
		memcpy(mp->nvalue, np->nvalue, fp->dsize - sizeof(*fp));
	else
		mp->nvalue = (char*)(fp+1);
	if(!nv_isattr(mp,NV_MINIMAL))
		mp->nvmeta = NULL;
	nv_offattr(mp,NV_RDONLY);
	return pp;
}

static int typeinfo(Opt_t* op, Sfio_t *out, const char *str, Optdisc_t *fp)
{
	char		*cp,**help,buffer[256];
	Namtype_t	*dp;
	Namval_t	*np,*nq,*tp;
	int		n, i, offset=stktell(sh.stk);
	Sfio_t		*sp;
	NOT_USED(op);
	np = *(Namval_t**)(fp+1);
	sfputr(sh.stk,NV_CLASS,'.');
	sfputr(sh.stk,np->nvname,0);
	np = nv_open(cp=stkptr(sh.stk,offset), sh.var_tree, NV_NOADD|NV_VARNAME);
	stkseek(sh.stk,offset);
	if(!np)
		return -1;
	if(!(dp=(Namtype_t*)nv_hasdisc(np,&type_disc)))
	{
		Namfun_t *fp;
		for(fp=np->nvfun;fp;fp=fp->next)
		{
			if(fp->disc && fp->disc->clonef==clone_inttype)
				break;
		}
		if(!fp)
			return -1;
		if(strcmp(str,"other")==0)
			return 0;
		tp = fp->type;
		nv_offattr(np,NV_RDONLY);
		fp->type = 0;
		if(np->nvmeta)
			sfprintf(out,"[+?\b%s\b is a %s.]\n", tp->nvname, (char*)np->nvmeta);
		cp = (char*)out->_next;
		sfprintf(out,"[+?\b%s\b is a %n ", tp->nvname, &i);
		nv_attribute(np,out,NULL, 1);
		if(cp[i+1]=='i')
			cp[i-1]='n';
		fp->type = tp;
		nv_onattr(np,NV_RDONLY);
		sfprintf(out," with default value \b%s\b.]",nv_getval(np));
		return 0;
	}
	if(strcmp(str,"other")==0)
	{
		Nambfun_t	*bp;
		if(bp=(Nambfun_t*)nv_hasdisc(np,nv_discfun(NV_DCADD)))
		{
			for(i=0; i < bp->num; i++)
			{
				if(nv_isattr(bp->bltins[i],NV_OPTGET))
					sfprintf(out,"\b%s.%s\b(3), ",np->nvname,bp->bnames[i]);
			}
		}
		return 0;
	}
	help = &dp->names[dp->ndisc];
	sp = sfnew(NULL,buffer,sizeof(buffer),-1,SFIO_STRING|SFIO_WRITE);
	sfprintf(out,"[+?\b%s\b defines the following fields:]{\n",np->nvname);
	for(i=0; i < dp->numnodes; i++)
	{
		nq = nv_namptr(dp->nodes,i);
		if(tp=nv_type(nq))
		{
			sfprintf(out,"\t[+%s?%s.\n",nq->nvname,tp->nvname);
			n = strlen(nq->nvname);
			while((cp=nv_namptr(dp->nodes,i+1)->nvname) && strncmp(cp,nq->nvname,n)==0 && cp[n]=='.')
				i++;
		}
		else
		{
			sfseek(sp,0, SEEK_SET);
			nv_attribute(nq,sp,NULL,1);
			cp = 0;
			if(!nv_isattr(nq,NV_REF))
				cp = sh_fmtq(nv_getval(nq));
			sfputc(sp,0);
			for(n=strlen(buffer); n>0 && buffer[n-1]==' '; n--);
			buffer[n] = 0;
			if(cp)
				sfprintf(out,"\t[+%s?%s, default value is %s.\n",nq->nvname,*buffer?buffer:"string",cp);
			else
				sfprintf(out,"\t[+%s?%s.\n",nq->nvname,*buffer?buffer:"string");
		}
		if(help[i])
			sfprintf(out,"  %s.",help[i]);
		sfputc(out,']');
	}
	sfprintf(out,"}\n");
	if(dp->ndisc>0)
	{
		stkseek(sh.stk,offset);
		sfputr(sh.stk,NV_CLASS,'.');
		sfputr(sh.stk,np->nvname,'.');
		n = stktell(sh.stk);
		sfprintf(out,"[+?\b%s\b defines the following discipline functions:]{\n",np->nvname);
		for(i=0; i < dp->ndisc; i++)
		{
			sfputr(sh.stk,dp->names[i],0);
			cp = 0;
			if((nq = nv_search(stkptr(sh.stk,offset),sh.fun_tree,0)) && nq->nvalue)
				cp = ((struct Ufunction*)nq->nvalue)->help;
			if(nq && nv_isattr(nq,NV_STATICF))
				sfprintf(out,"\t[+%s?:static:%s]\n",dp->names[i],cp?cp:Empty);
			else
				sfprintf(out,"\t[+%s?%s]\n",dp->names[i],cp?cp:Empty);
			if(cp)
				sfputc(out,'.');
			stkseek(sh.stk,n);
		}
		sfprintf(out,"}\n");
	}
	stkseek(sh.stk,offset);
	sfclose(sp);
	return 0;
}

static int std_disc(Namval_t *mp, Namtype_t *pp)
{
	const char		*sp, *cp = strrchr(mp->nvname,'.');
	const char		**argv;
	int			i;
	Namval_t		*np=0,*nq;
	if(cp)
		cp++;
	else
		cp = mp->nvname;
	if(strcmp(cp,"create")==0)
	{
		if(pp)
			pp->cp = mp;
		return 0;
	}
	for(argv=nv_discnames; sp=*argv; argv++)
	{
		if(strcmp(cp,sp)==0)
		{
			if(!pp)
				return 1;
			goto found;
		}
	}
	return 0;
found:
	if(strncmp(sp=mp->nvname,NV_CLASS,sizeof(NV_CLASS)-1)==0)
		sp += sizeof(NV_CLASS);
	sp += strlen(pp->fun.type->nvname)+1;
	if(sp == cp)
		np = pp->fun.type;
	else for(i=1; i < pp->numnodes; i++)
	{
		nq = nv_namptr(pp->nodes,i);
		if(strncmp(nq->nvname, sp, cp-sp-1)==0)
		{
			np = nq;
			break;
		}
	}
	nv_onattr(mp,NV_NOFREE);
	if(!np)
	{
		errormsg(SH_DICT,ERROR_exit(1),"%s: cannot set discipline for undeclared type member",sp);
		UNREACHABLE();
	}
	if(!nv_setdisc(np, cp, mp, (Namfun_t*)np))
		abort();
	return 1;
}


void nv_addtype(Namval_t *np, const char *optstr, Optdisc_t *op, size_t optsz)
{
	Namdecl_t	*cp = sh_newof(NULL,Namdecl_t,1,optsz);
	Optdisc_t	*dp = (Optdisc_t*)(cp+1);
	Namval_t	*bp;
	char		*name;
	if(optstr)
		cp->optstring = optstr;
	else
		cp->optstring = sh_opttype;
	memcpy(dp,op, optsz);
	cp->optinfof = dp;
	cp->tp = np;
	if(name=strrchr(np->nvname,'.'))
		name++;
	else
		name = np->nvname;
#if SHOPT_NAMESPACE
	if(bp=(Namval_t*)sh.namespace)
	{
		Namtype_t *tp = (Namtype_t*)nv_hasdisc(np, &type_disc);
		if(tp)
			tp->nsp = bp;
		if(!sh.strbuf2)
			sh.strbuf2 = sfstropen();
		sfprintf(sh.strbuf2,".%s.%s",nv_name(bp)+1,name);
		name = sfstruse(sh.strbuf2);
	}
#endif /* SHOPT_NAMESPACE */
	if((bp=nv_search(name,sh.fun_tree,NV_NOSCOPE)) && !bp->nvalue)
		nv_delete(bp,sh.fun_tree,0);
	bp = sh_addbuiltin(name, funptr(SYSTYPESET), cp);
	nv_onattr(bp,nv_isattr(SYSTYPESET,NV_PUBLIC));
	nv_onattr(np, NV_RDONLY);
}

void nv_newtype(Namval_t *mp)
{
	struct	{
		    Optdisc_t	opt;
		    Namval_t	*np;
		}	optdisc;
	memset(&optdisc,0,sizeof(optdisc));
	optdisc.opt.infof = typeinfo;
	optdisc.np = mp;
	nv_addtype(mp,sh_opttype, &optdisc.opt, sizeof(optdisc));
}

/*
 * This function creates a type out of the <numnodes> nodes in the
 * array <nodes>.  The first node is the name for the type
 */
Namval_t *nv_mktype(Namval_t **nodes, int numnodes)
{
	Namval_t	*mp=nodes[0], *bp=0, *np, *nq, **mnodes=nodes;
	int		i,j,k,nd=0,nref=0,iref=0,inherit=0;
	int		size=sizeof(NV_DATA), dsize=0, nnodes;
	size_t		offset=0,m;
	ssize_t		n;
	char		*name=0, *cp, *sp, **help;
	Namtype_t	*pp,*qp=0,*dp,*tp;
	Dt_t		*root = nv_dict(mp);
	struct Namref	*nrp = 0;
	Namfun_t	*fp;
	m = strlen(mp->nvname)+1;
	if(numnodes < 2)
	{
		cp = nodes[0]->nvname;
		nv_unset(nodes[0],NV_RDONLY);
		errormsg(SH_DICT,ERROR_exit(1),e_badtypedef,cp);
		UNREACHABLE();
	}
	n=strlen(nodes[1]->nvname);
	for(nnodes=1,i=1; i <numnodes; i++)
	{
		np=nodes[i];
		if(is_afunction(np))
		{
			if(!std_disc(np, NULL))
			{
				size += strlen(np->nvname+m)+1;
				if(strncmp(np->nvname,NV_CLASS,sizeof(NV_CLASS)-1)==0)
					size -= sizeof(NV_CLASS);
				nd++;
			}
			continue;
		}
		if(nv_isattr(np,NV_REF))
			iref++;
		if(np->nvmeta)
			size += strlen(np->nvmeta) + 1;
		if(strcmp(&np->nvname[m],NV_DATA)==0 && !nv_type(np))
			continue;
		if(qp)
		{	/* delete duplicates */
			for(j=0; j < qp->numnodes;j++)
			{
				nq = nv_namptr(qp->nodes,j);
				if(strcmp(nq->nvname,&np->nvname[m])==0)
					break;
			}
			if(j < qp->numnodes)
				continue;
		}
		nnodes++;
		if(name && strncmp(&name[m],&np->nvname[m],n)==0 && np->nvname[m+n]=='.')
			offset -= sizeof(char*);
		dsize = nv_datasize(np,&offset);
		if(!nv_isarray(np) && (dp=(Namtype_t*)nv_hasdisc(np, &type_disc)))
		{
			nnodes += dp->numnodes;
			if((n=dp->strsize)<0)
				n = -n;
			iref = nref += dp->nref;
			if(np->nvname[m]=='_' && np->nvname[m+1]==0 && (bp=nv_type(np)))
			{
				qp = dp;
				nd = dp->ndisc;
				nnodes = dp->numnodes;
				offset = 0;
				dsize = nv_size(np);
				size += n;
			}
			else
				size += n + dp->numnodes*(strlen(&np->nvname[m])+1);
			n = strlen(np->nvname);
			while((i+1) < numnodes && (cp=nodes[i+1]->nvname) && strncmp(cp,np->nvname,n)==0 && cp[n]=='.')
				i++;
		}
		else if(nv_isattr(np,NV_REF))
			nref++;
		offset += (dsize?dsize:4);
		size += (n=strlen(name=np->nvname)-m+1);
	}
	offset = roundof(offset,sizeof(char*));
	nv_setsize(mp,offset);
	k = roundof(sizeof(Namtype_t),sizeof(Sfdouble_t)) - sizeof(Namtype_t);
	pp = sh_newof(NULL, Namtype_t, 1, nnodes*NV_MINSZ + offset + size + (nnodes+nd)*sizeof(char*) + iref*sizeof(struct Namref)+k);
	pp->fun.dsize = sizeof(Namtype_t)+nnodes*NV_MINSZ +offset+k;
	pp->fun.type = mp;
	pp->parent = nv_lastdict();
	pp->np = mp;
	pp->bp = bp;
	pp->childfun.fun.disc = &chtype_disc;
	pp->childfun.fun.nofree = 1;
	pp->childfun.ttype = pp;
	pp->childfun.ptype = pp;
	pp->fun.disc = &type_disc;
	pp->nodes = (char*)(pp+1);
	pp->numnodes = nnodes;
	pp->data = pp->nodes + nnodes*NV_MINSZ +k;
	pp->dsize = offset;
	nrp = (struct Namref*)(pp->data+offset);
	pp->names = (char**)(nrp+iref);
	help = &pp->names[nd];
	pp->strsize = size;
	cp = (char*)&pp->names[nd+nnodes];
	if(qp)
		mnodes = sh_newof(NULL, Namval_t*, nd+1, 0);
	nd = 0;
	nq = nv_namptr(pp->nodes,0);
	nq->nvname = cp;
	nv_onattr(nq,NV_MINIMAL);
	cp = strcopy(cp,NV_DATA);
	*cp++ = 0;
	for(name=0, offset=0, k=i=1; i < numnodes; i++)
	{
		np=nodes[i];
		if(is_afunction(np))
		{
			sp = np->nvname+m;
			if(strncmp(np->nvname,NV_CLASS,sizeof(NV_CLASS)-1)==0)
				sp += sizeof(NV_CLASS);
			if(!std_disc(np, pp))
			{
				/* see if discipline already defined */
				for(j=0; j< nd; j++)
				{
					if(strcmp(sp,pp->names[j])==0)
					{
						mnodes[j] = nodes[i];
						break;
					}
				}
				if(j>=nd)
				{
					pp->names[nd] = cp;
					mnodes[nd++] = nodes[i];
					cp = strcopy(cp,sp);
					*cp++ = 0;
				}
				nv_onattr(mnodes[j],NV_NOFREE);
			}
			continue;
		}
		if(inherit)
		{
			for(j=0; j < k ; j++)
			{
				nq = nv_namptr(pp->nodes,j);
				if(strcmp(nq->nvname,&np->nvname[m])==0)
					break;
			}
			if(j < k)
			{
				sp = nv_getval(np);
				if(!nv_isvtree(np) && sp)
					nv_putval(nq,sp,0);
				goto skip;
			}
		}
		if(strcmp(&np->nvname[m],NV_DATA)==0 && !nv_type(np))
		{
			char *val=nv_getval(np);
			nq = nv_namptr(pp->nodes,0);
			nq->nvfun = 0;
			nv_putval(nq,(val?val:0),nv_isattr(np,~(NV_MINIMAL|NV_EXPORT|NV_ARRAY)));
			nq->nvflag = np->nvflag|NV_NOFREE|NV_MINIMAL;
			goto skip;
		}
		if(qp)
		{
			Nambfun_t	*bp;
			dp = (Namtype_t*)nv_hasdisc(nv_type(np), &type_disc);
			memcpy(pp->nodes,dp->nodes,dp->numnodes*NV_MINSZ);
			offset = nv_size(np);
			memcpy(pp->data,dp->data,offset);
			for(k=0;k < dp->numnodes; k++)
			{
				Namval_t *nr = nv_namptr(qp->nodes,k);
				nq = nv_namptr(pp->nodes,k);
				if(fixnode(pp,dp,k,nrp,0))
				{
					nrp++;
					nq = ((struct Namref*)nq->nvalue)->np;
				}
				if(!nv_isattr(nr,NV_REF) && !nv_hasdisc(nr,&type_disc))
				{
					if(nr->nvsize)
						memcpy(nq->nvalue, nr->nvalue, size = nv_datasize(nr,NULL));
					else
					{
						nq->nvalue = nr->nvalue;
						nv_onattr(nq,NV_NOFREE);
					}
				}
			}
			if(bp=(Nambfun_t*)nv_hasdisc(np,nv_discfun(NV_DCADD)))
			{
				for(j=0; j < bp->num; j++)
				{
					pp->names[nd++] = (char*)bp->bnames[j];
					mnodes[j] = bp->bltins[j];
				}
			}
			qp = 0;
			inherit=1;
			goto skip;
		}
		nq = nv_namptr(pp->nodes,k);
		if(np->nvmeta)
		{
			/* need to save the string pointer */
			nv_offattr(np,NV_EXPORT);
			help[k] = cp;
			cp = strcopy(cp,np->nvmeta);
			j = *help[k];
			if(islower(j))
				*help[k] = toupper(j);
			*cp++ = 0;
			np->nvmeta = NULL;
		}
		nq->nvname = cp;
		if(name && strncmp(name,&np->nvname[m],n)==0 && np->nvname[m+n]=='.')
			offset -= sizeof(char*);
		dsize = nv_datasize(np,&offset);
		cp = strcopy(name=cp, &np->nvname[m]);
		n = cp-name;
		*cp++ = 0;
		nq->nvsize = np->nvsize;
		nq->nvflag = (np->nvflag&~NV_EXPORT)|NV_NOFREE|NV_MINIMAL;
		if(dp = (Namtype_t*)nv_hasdisc(np, &type_disc))
		{
			int r,kfirst=k;
			char *cname = &np->nvname[m];
			/*
			 * If field is a type, mark the type by setting
			 * strsize<0.  This changes create_type()
			 */
			clone_all_disc(np,nq,NV_RDONLY);
			if(nv_isarray(np))
			{
				nv_disc(nq, &pp->childfun.fun, NV_LAST);
				k++;
				goto skip;
			}
			if(fp=nv_hasdisc(nq,&chtype_disc))
				nv_disc(nq, &pp->childfun.fun, NV_LAST);
			if(tp = (Namtype_t*)nv_hasdisc(nq, &type_disc))
				tp->strsize = -tp->strsize;
			for(r=0; r < dp->numnodes; r++)
			{
				Namval_t *nr = nv_namptr(dp->nodes,r);
				nq = nv_namptr(pp->nodes,++k);
				nq->nvname = cp;
				dsize = nv_datasize(nr,&offset);
				nq->nvflag = nr->nvflag;
				if(nr->nvalue)
				{
					Namchld_t *xp = (Namchld_t*)nv_hasdisc(nr,&chtype_disc);
					if(xp && (char*)nr->nvalue >= xp->ptype->data && (char*)nr->nvalue < xp->ptype->data + xp->ptype->fun.dsize)
					{
						nq->nvalue = pp->data + offset;
						memcpy(nq->nvalue, nr->nvalue, dsize);
						nv_onattr(nq,NV_NOFREE);
					}
					else
						nq->nvalue = sh_strdup(nr->nvalue);
					nv_disc(nq, &pp->childfun.fun, NV_LAST);
				}
				nq->nvsize = nr->nvsize;
				offset += dsize;
				if(*cname!='_' || cname[1])
				{
					cp = strcopy(cp,cname);
					*cp++ = '.';
				}
				cp = strcopy(cp,nr->nvname);
				*cp++ = 0;
			}
			while((i+1) < numnodes && (cname=&nodes[i+1]->nvname[m]) && strncmp(cname,&np->nvname[m],n)==0 && cname[n]=='.')
			{
				int j=kfirst;
				nv_unset(np,0);
				nv_delete(np,root,0);
				np = nodes[++i];
				while(j < k)
				{
					nq = nv_namptr(pp->nodes,++j);
					if(strcmp(nq->nvname,cname)==0)
					{
						if((char*)nq->nvalue >= pp->data && (char*)nq->nvalue < (char*)pp->names)
							memcpy(nq->nvalue, np->nvalue, nv_datasize(np,0));
						break;
					}
				}
			}
		}
		else
		{
			Namarr_t *ap;
			j = nv_isattr(np,NV_NOFREE);
			if(j==0 && (ap=nv_arrayptr(np)) && !ap->fun)
				j = 1;
			nq->nvfun = np->nvfun;
			np->nvfun = 0;
			nv_disc(nq, &pp->childfun.fun, NV_LAST);
			if(nq->nvfun)
			{
				for(fp=nq->nvfun; fp; fp = fp->next)
					fp->nofree |= 1;
			}
			nq->nvalue = np->nvalue;
			if(dsize  && (np->nvalue || !nv_isarray(np)))
			{
				nq->nvalue = pp->data + offset;
				sp = np->nvalue;
				if(sp)
					memcpy(nq->nvalue, sp, dsize);
				else if(nv_isattr(np,NV_LJUST|NV_RJUST))
					memset(nq->nvalue, ' ', dsize);
				if(!j)
					free(np->nvalue);
			}
			if(!nq->nvalue && nq->nvfun == &pp->childfun.fun)
			{
				if(nv_isattr(np,NV_ARRAY|NV_NOFREE)==(NV_ARRAY|NV_NOFREE))
					nq->nvalue = AltEmpty;
				else
					nq->nvalue = Empty;
			}
			np->nvalue = NULL;
			offset += (dsize?dsize:4);
		}
		k++;
	skip:
		if(!nv_isnull(np))
			nv_unset(np,0);
		nv_delete(np,root,0);
	}
	pp->ndisc = nd;
	pp->nref = nref;
	if(k>1)
	{
		nv_setsize(mp,offset);
		mp->nvalue = pp->data;
		nv_onattr(mp,NV_NOFREE|NV_BINARY|NV_RAW);
	}
	else if(!mp->nvalue)
		mp->nvalue = Empty;
	nv_onattr(mp,NV_TAGGED);
	nv_disc(mp, &pp->fun, NV_LAST);
	if(nd>0)
	{
		pp->names[nd] = 0;
		nv_adddisc(mp, (const char**)pp->names, mnodes);
	}
	if(mnodes!=nodes)
		free(mnodes);
	nv_newtype(mp);
	return mp;
}

Namval_t *nv_mkinttype(char *name, size_t size, int sign, const char *help, Namdisc_t *ep)
{
	Namval_t	*mp;
	Namfun_t	*fp;
	Namdisc_t	*dp;
	int		offset=stktell(sh.stk);
	sfputr(sh.stk,NV_CLASS,'.');
	sfputr(sh.stk,name,0);
	mp = nv_open(stkptr(sh.stk,offset), sh.var_tree, NV_VARNAME);
	stkseek(sh.stk,offset);
	offset = size + sizeof(Namdisc_t);
	fp = sh_newof(NULL, Namfun_t, 1, offset);
	fp->type = mp;
	fp->nofree |= 1;
	fp->dsize = sizeof(Namfun_t)+size;
	dp = (Namdisc_t*)(fp+1);
	if(ep)
		*dp = *ep;
	dp->clonef =  clone_inttype;
	fp->disc = dp;
	mp->nvalue = (char*)(fp+1) + sizeof(Namdisc_t);
	nv_setsize(mp,10);
	mp->nvmeta = (void*)help;
	nv_onattr(mp,NV_NOFREE|NV_RDONLY|NV_INTEGER|NV_EXPORT);
	if(size==16)
		nv_onattr(mp,NV_INT16);
	else if(size==64)
		nv_onattr(mp,NV_INT64);
	if(!sign)
		nv_onattr(mp,NV_UNSIGN);
	nv_disc(mp, fp, NV_LAST);
	nv_newtype(mp);
	return mp;
}

void nv_typename(Namval_t *tp, Sfio_t *out)
{
	char *v,*cp;
	Namtype_t	*dp;
	cp = nv_name(tp);
	if(v=strrchr(cp,'.'))
		cp = v+1;
	if((dp = (Namtype_t*)nv_hasdisc(tp,&type_disc)) && dp->bp)
	{
		nv_typename(dp->bp,out);
		sfprintf(out,"%s.%s",sfstruse(out),cp);
	}
	else
		sfputr(out,cp,-1);
}

Namval_t *nv_type(Namval_t *np)
{
	Namfun_t  *fp;
	if(nv_isattr(np,NV_BLTIN|BLT_DCL)==(NV_BLTIN|BLT_DCL))
	{
		Namdecl_t *ntp = (Namdecl_t*)nv_context(np);
		return ntp?ntp->tp:0;
	}
	for(fp=np->nvfun; fp; fp=fp->next)
	{
		if(fp->type)
			return fp->type;
		if(fp->disc && fp->disc->typef && (np= (*fp->disc->typef)(np,fp)))
			return np;
	}
	return NULL;
}

/*
 * call any and all create functions
 */
static void type_init(Namval_t *np)
{
	int 		i;
	Namtype_t	*dp, *pp=(Namtype_t*)nv_hasdisc(np,&type_disc);
	Namval_t	*nq;
	if(!pp)
		return;
	for(i=0; i < pp->numnodes; i++)
	{
		nq = nv_namptr(pp->nodes,i);
		if((dp=(Namtype_t*)nv_hasdisc(nq,&type_disc)) && dp->cp)
			sh_fun(dp->cp,nq, NULL);
	}
	if(pp->cp)
		sh_fun(pp->cp, np, NULL);
}

/*
 * This function turns variable <np>  to the type <tp>
 */
int nv_settype(Namval_t* np, Namval_t *tp, int flags)
{
	int		isnull = nv_isnull(np);
	int		rdonly = nv_isattr(np,NV_RDONLY);
	char		*val=0;
	Namarr_t	*ap=0;
	int		nelem = 0;
	Namval_t	*tq;
	if(nv_type(np)==tp)
		return 0;
	if(nv_isarray(np) && (tq=nv_type(np)))
	{
		if(tp==tq)
			return 0;
		errormsg(SH_DICT,ERROR_exit(1),e_redef,nv_name(np));
		UNREACHABLE();
	}
	if(sh.subshell && !sh.subshare)
		sh_subfork();
	if((ap=nv_arrayptr(np)) && ap->nelem>0)
	{
		nv_putsub(np,NULL,ARRAY_SCAN);
		ap->hdr.type = tp;
		do
		{
			nv_arraysettype(np, tp, nv_getsub(np),flags);
		}
		while(nv_nextsub(np));
	}
	else if(ap || nv_isarray(np))
	{
		flags &= ~NV_APPEND;
		if(!ap)
		{
			nv_putsub(np,"0",ARRAY_FILL);
			ap = nv_arrayptr(np);
			nelem = 1;
			nv_arraysettype(np,tp,"0",flags);
		}
	}
	else
	{
		/* non-typeset -T type */
		if(isnull)
			flags &= ~NV_APPEND;
		else if(!nv_isvtree(np))
		{
			val = sh_strdup(nv_getval(np));
			if(!(flags&NV_APPEND))
				nv_unset(np, NV_RDONLY);
		}
		if(!nv_clone(tp,np,flags|NV_NOFREE))
			return 0;
	}
	if(ap)
	{
		int nofree;
		nv_disc(np,&ap->hdr,NV_POP);
		np->nvalue = NULL;
		nv_clone(tp,np,flags|NV_NOFREE);
		if(np->nvalue && np->nvalue != Empty && !nv_isattr(np,NV_NOFREE))
			free(np->nvalue);
		np->nvalue = NULL;
		nofree = ap->hdr.nofree;
		ap->hdr.nofree = 0;
		ap->hdr.type = tp;
		nv_disc(np, &ap->hdr, NV_FIRST);
		ap->hdr.nofree = nofree;
		nv_onattr(np,NV_ARRAY);
		if(nelem)
		{
			ap->nelem++;
			nv_putsub(np,"0",0);
			nv_unset(np,NV_RDONLY|NV_TYPE);
			ap->nelem--;
		}
	}
	type_init(np);
	if(!rdonly)
		nv_offattr(np,NV_RDONLY);
	if(val)
	{
		nv_putval(np,val,NV_RDONLY);
		free(val);
	}
	return 0;
}

static void write_indent(Sfio_t *out,char *str,int n,int indent)
{
	int	c, first=1;
	char	*cp = str;
	while(n-- && (c = *str++))
	{
		if(c=='\n')
		{
			if(!first)
				sfnputc(out,'\t',indent);
			first = 0;
			sfwrite(out,cp,str-cp);
			cp = str;
		}
	}
	if(cp > str)
	{
		sfnputc(out,'\t',indent);
		sfwrite(out,cp,str-cp);
	}
}

int	sh_outtype(Sfio_t *out)
{
	Namval_t	node,*mp,*tp;
	Dt_t		*dp;
	char		*cp,*sp,nvtype[sizeof(NV_CLASS)];
	int		n=0,indent = 0;
	if(cp=sh.prefix)
	{
		indent=1;
		while(*cp)
		{
			if(*cp++ =='.')
				indent++;
		}
		n = cp-sh.prefix+1;
	}
	strcpy(nvtype,NV_CLASS);
	if(!(mp = nv_open(nvtype, sh.var_base,NV_NOADD|NV_VARNAME)))
		return 0;
	memcpy(&node,L_ARGNOD,sizeof(node));
	L_ARGNOD->nvfun = NULL;
	L_ARGNOD->nvalue = NULL;
	dp  = 	nv_dict(mp);
	if(indent==0)
	for(tp = (Namval_t*)dtfirst(dp); tp; tp = (Namval_t*)dtnext(dp,tp))
	{
		/* skip over enums */
		if(tp->nvfun && !nv_isvtree(tp))
			continue;
		if(!nv_search(tp->nvname,sh.bltin_tree,0))
			continue;
		sfprintf(out,"typeset -T %s\n",tp->nvname);
	}
	for(tp = (Namval_t*)dtfirst(dp); tp; tp = (Namval_t*)dtnext(dp,tp))
	{
		if(nv_isnull(tp) || !nv_isvtree(tp))
			continue;
		if(indent && (strncmp(tp->nvname,sh.prefix,n-1) || tp->nvname[n-1]!='.' || strchr(tp->nvname+n,'.')))
			continue;
		nv_settype(L_ARGNOD,tp,0);
		if(indent)
			sfnputc(out,'\t',indent);
		sfprintf(out,"typeset -T %s=",tp->nvname+n);
		sh.last_table = 0;
		cp = nv_getval(L_ARGNOD);
		if(indent)
			write_indent(out,cp,strlen(cp)-1,indent);
		else
			sfprintf(out,"%.*s",strlen(cp)-1,cp);
		nv_unset(L_ARGNOD,NV_RDONLY);
		for(sp=0; sp=nv_setdisc(tp,NULL,(Namval_t*)sp,(Namfun_t*)tp);)
		{
			mp = (Namval_t*)nv_setdisc(tp,sp,tp,(Namfun_t*)tp);
			if(!mp || mp==tp)
				continue;
			if(cp=strrchr(mp->nvname,'.'))
				cp++;
			else
				cp = mp->nvname;
			if(indent)
				sfnputc(out,'\t',indent);
			if(nv_isattr(mp,NV_FPOSIX))
				sfprintf(out,"\t%s()",cp);
			else
				sfprintf(out,"\tfunction %s",cp);
			if(mp->nvalue && nv_funtree(mp))
			{
				sfputc(out, '\n');
				sh_deparse(out, (Shnode_t*)(nv_funtree(mp)), 2 | nv_isattr(mp,NV_FPOSIX), 1);
				if(nv_isattr(mp,NV_STATICF|NV_TAGGED))
				{
					struct Ufunction *rp = mp->nvalue;
					if(indent)
						sfnputc(out,'\t',indent);
					sfwrite(out,"\ttypeset -f",11);
					if(nv_isattr(mp,NV_STATICF))
						sfputc(out,'S');
					if(nv_isattr(mp,NV_TAGGED))
						sfputc(out,'t');
					if(rp->help)
						sfprintf(out,"h '%s'",rp->help);
					sfprintf(out," %s\n",cp);
				}
			}
		}
		if(indent)
			sfnputc(out,'\t',indent);
		sfwrite(out,")\n",2);
	}
	dtdelete(sh.var_base,L_ARGNOD);
	memcpy(L_ARGNOD,&node,sizeof(node));
	dtinsert(sh.var_base,L_ARGNOD);
	return 0;
}

Namval_t *nv_typeparent(Namval_t *np)
{
	Namchld_t *fp;
	Namtype_t *tp;
	if(fp = (Namchld_t*)nv_hasdisc(np,&chtype_disc))
		return fp->ptype->parent;
	if(tp = (Namtype_t*)nv_hasdisc(np,&type_disc))
		return tp->parent;
	return NULL;
}

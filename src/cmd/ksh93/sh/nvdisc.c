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
*                  Andy Fiddaman <andy@omniosce.org>                   *
*                                                                      *
***********************************************************************/
/*
 * AT&T Labs
 *
 */

#include	"shopt.h"
#include        "defs.h"
#include        "variables.h"
#include        "builtins.h"
#include        "path.h"
#include	"io.h"
#include	"shlex.h"

static void assign(Namval_t*,const char*,int,Namfun_t*);

int nv_compare(Dt_t* dict, void *sp, void *dp, Dtdisc_t *disc)
{
	NOT_USED(dict);
	NOT_USED(disc);
	if(sp==dp)
		return 0;
	return strcmp((char*)sp,(char*)dp);
}

/*
 * call the next getval function in the chain
 */
char *nv_getv(Namval_t *np, Namfun_t *nfp)
{
	Namfun_t	*fp;
	char		*cp;
	if((fp = nfp) != NULL && !nv_local)
		fp = nfp = nfp->next;
	nv_local=0;
	for(; fp; fp=fp->next)
	{
		if(!fp->disc || (!fp->disc->getnum && !fp->disc->getval))
			continue;
		if(!nv_isattr(np,NV_NODISC) || fp==(Namfun_t*)nv_arrayptr(np))
			break;
	}
	if(fp && fp->disc->getval)
		cp = (*fp->disc->getval)(np,fp);
	else if(fp && fp->disc->getnum)
	{
		sfprintf(sh.strbuf,"%.*Lg",12,(*fp->disc->getnum)(np,fp));
		cp = sfstruse(sh.strbuf);
	}
	else
	{
		nv_local=1;
		cp = nv_getval(np);
	}
	return cp;
}

/*
 * call the next getnum function in the chain
 */
Sfdouble_t nv_getn(Namval_t *np, Namfun_t *nfp)
{
	Namfun_t	*fp;
	Sfdouble_t	d=0;
	char *str;
	if((fp = nfp) != NULL && !nv_local)
		fp = nfp = nfp->next;
	nv_local=0;
	for(; fp; fp=fp->next)
	{
		if(!fp->disc || (!fp->disc->getnum && !fp->disc->getval))
			continue;
		if(!fp->disc->getnum && nv_isattr(np,NV_INTEGER))
			continue;
		if(!nv_isattr(np,NV_NODISC) || fp==(Namfun_t*)nv_arrayptr(np))
			break;
	}
	if(fp && fp->disc && fp->disc->getnum)
		d = (*fp->disc->getnum)(np,fp);
	else if(nv_isattr(np,NV_INTEGER))
	{
		nv_local = 1;
		d =  nv_getnum(np);
	}
	else
	{
		if(fp && fp->disc && fp->disc->getval)
			str = (*fp->disc->getval)(np,fp);
		else
			str = nv_getv(np,fp?fp:nfp);
		if(str && *str)
			d = sh_arith(str);
	}
	return d;
}

/*
 * call the next assign function in the chain
 */
void nv_putv(Namval_t *np, const char *value, int flags, Namfun_t *nfp)
{
	Namfun_t	*fp, *fpnext;
	Namarr_t	*ap;
	if((fp=nfp) != NULL && !nv_local)
		fp = nfp = nfp->next;
	nv_local=0;
	if(flags&NV_NODISC)
		fp = 0;
	for(; fp; fp=fpnext)
	{
		fpnext = fp->next;
		if(!fp->disc || !fp->disc->putval)
		{
			if(!value && (!(ap=nv_arrayptr(np)) || ap->nelem==0))
			{
				if(fp->disc || !(fp->nofree&1))
					nv_disc(np,fp,NV_POP);
				if(!(fp->nofree&1))
					free(fp);
			}
			continue;
		}
		if(!nv_isattr(np,NV_NODISC) || fp==(Namfun_t*)nv_arrayptr(np))
			break;
	}
	if(!value && (flags&NV_TYPE) && fp && fp->disc->putval==assign)
		fp = 0;
	if(fp && fp->disc->putval)
		(*fp->disc->putval)(np,value, flags, fp);
	else
	{
		nv_local=1;
		if(value)
			nv_putval(np, value, flags);
		else
			nv_unset(np, flags&(NV_RDONLY|NV_EXPORT));
	}
}

#define LOOKUPS		0
#define ASSIGN		1
#define APPEND		2
#define UNASSIGN	3
#define LOOKUPN		4

struct	vardisc
{
	Namfun_t	fun;
	Namval_t	*disc[5];
};

struct blocked
{
	struct blocked	*next;
	Namval_t	*np;
	int		flags;
	void		*sub;
	int		isub;
};

static struct blocked	*blist;

#define isblocked(bp,type)	((bp)->flags & (1<<(type)))
#define block(bp,type)		((bp)->flags |= (1<<(type)))
#define unblock(bp,type)	((bp)->flags &= ~(1<<(type)))

/*
 * returns pointer to blocking structure
 */
static struct blocked *block_info(Namval_t *np, struct blocked *pp)
{
	struct blocked	*bp;
	void		*sub=0;
	int		isub=0;
	if(nv_isarray(np) && (isub=nv_aindex(np)) < 0)
		sub = nv_associative(np,NULL,NV_ACURRENT);
	for(bp=blist ; bp; bp=bp->next)
	{
		if(bp->np==np && bp->sub==sub && bp->isub==isub)
			return bp;
	}
	if(pp)
	{
		pp->np = np;
		pp->flags = 0;
		pp->isub = isub;
		pp->sub = sub;
		pp->next = blist;
		blist = pp;
	}
	return pp;
}

static void block_done(struct blocked *bp)
{
	blist = bp = bp->next;
	if(bp && (bp->isub>=0 || bp->sub))
		nv_putsub(bp->np, bp->sub,(bp->isub<0?0:bp->isub)|ARRAY_SETSUB);
}

/*
 * free discipline if no more discipline functions
 */
static void chktfree(Namval_t *np, struct vardisc *vp)
{
	int n;
	for(n=0; n< sizeof(vp->disc)/sizeof(*vp->disc); n++)
	{
		if(vp->disc[n])
			break;
	}
	if(n>=sizeof(vp->disc)/sizeof(*vp->disc))
	{
		/* no disc left so pop */
		Namfun_t *fp;
		if((fp=nv_stack(np, NULL)) && !(fp->nofree&1))
			free(fp);
	}
}

/*
 * This function performs an assignment disc on the given node <np>
 */
static void	assign(Namval_t *np,const char* val,int flags,Namfun_t *handle)
{
	int		type = (flags&NV_APPEND)?APPEND:ASSIGN;
	struct vardisc *vp = (struct vardisc*)handle;
	Namval_t *nq =  vp->disc[type];
	struct blocked	block, *bp;
	Namval_t	node;
	void		*saveval = np->nvalue;
	Namval_t	*tp, *nr;  /* for 'typeset -T' types */
	int		jmpval = 0;
	/* No unset discipline during virtual subshell cleanup or shell reinit */
	if(!val && (sh.nv_restore || sh_isstate(SH_INIT)))
		return;
	bp = block_info(np, &block);
	if(val && (tp=nv_type(np)) && (nr=nv_open(val,sh.var_tree,NV_VARNAME|NV_ARRAY|NV_NOADD|NV_NOFAIL)) && tp==nv_type(nr))
	{
		char *sub = nv_getsub(np);
		nv_unset(np,0);
		if(sub)
		{
			nv_putsub(np, sub, ARRAY_ADD);
			nv_putval(np,nv_getval(nr), 0);
		}
		else
			nv_clone(nr,np,0);
		goto done;
	}
	if(val || isblocked(bp,type))
	{
		if(!nq || isblocked(bp,type))
		{
			nv_putv(np,val,flags,handle);
			goto done;
		}
		node = *SH_VALNOD;
		if(!nv_isnull(SH_VALNOD))
		{
			nv_onattr(SH_VALNOD,NV_NOFREE);
			nv_unset(SH_VALNOD,0);
		}
		if(flags&NV_INTEGER)
			nv_onattr(SH_VALNOD,(flags&(NV_LONG|NV_DOUBLE|NV_EXPNOTE|NV_HEXFLOAT|NV_SHORT)));
		nv_putval(SH_VALNOD, val, (flags&NV_INTEGER)?flags:NV_NOFREE);
	}
	else
		nq =  vp->disc[type=UNASSIGN];
	if(nq && !isblocked(bp,type))
	{
		struct checkpt	checkpoint;
		int		savexit = sh.savexit;
		Lex_t		*lexp = (Lex_t*)sh.lex_context, savelex;
		int		bflag;
		/* disciplines like PS2 may run at parse time; save, reinit and restore the lexer state */
		savelex = *lexp;
		sh_lexopen(lexp, 0);   /* needs full init (0), not what it calls reinit (1) */
		block(bp,type);
		if(bflag = (type==APPEND && !isblocked(bp,LOOKUPS)))
			block(bp,LOOKUPS);
		sh_pushcontext(&checkpoint, SH_JMPFUN);
		jmpval = sigsetjmp(checkpoint.buff, 0);
		if(!jmpval)
			sh_fun(nq,np,NULL);
		sh_popcontext(&checkpoint);
		if(sh.topfd != checkpoint.topfd)
			sh_iorestore(checkpoint.topfd, jmpval);
		unblock(bp,type);
		if(bflag)
			unblock(bp,LOOKUPS);
		if(!vp->disc[type])
			chktfree(np,vp);
		*lexp = savelex;
		sh.savexit = savexit;	/* avoid influencing $? */
	}
	if(nv_isarray(np))
		np->nvalue = saveval;
	if(val)
	{
		char *cp;
		Sfdouble_t d;
		if(nv_isnull(SH_VALNOD))
			cp=0;
		else if(flags&NV_INTEGER)
		{
			d = nv_getnum(SH_VALNOD);
			cp = (char*)(&d);
			flags |= (NV_LONG|NV_DOUBLE);
			flags &= ~NV_SHORT;
		}
		else
			cp = nv_getval(SH_VALNOD);
		if(cp)
			nv_putv(np,cp,flags|NV_RDONLY,handle);
		nv_unset(SH_VALNOD,0);
		/* restore everything but the nvlink field */
		memcpy(&SH_VALNOD->nvname,  &node.nvname, sizeof(node)-sizeof(node.nvlink));
	}
	else if(np==SH_FUNNAMENOD)
		nv_putv(np,val,flags,handle);
	else if(!nq || !isblocked(bp,type))
	{
		Dt_t *root = sh_subfuntree(1);
		Namval_t *pp=0;
		int n;
		Namarr_t *ap;
		block(bp,type);
		nv_disc(np,handle,NV_POP);
		if(!nv_isattr(np,NV_MINIMAL))
			pp = np->nvmeta;
		nv_putv(np, val, flags, handle);
		if(sh.subshell)
			goto done;
		if(pp && nv_isarray(pp))
			goto done;
		if(nv_isarray(np) && (ap=nv_arrayptr(np)) && ap->nelem>0)
			goto done;
		for(n=0; n < sizeof(vp->disc)/sizeof(*vp->disc); n++)
		{
			if((nq=vp->disc[n]) && !nv_isattr(nq,NV_NOFREE))
			{
				nv_unset(nq,0);
				dtdelete(root,nq);
			}
		}
		unblock(bp,type);
		if(!(handle->nofree&1))
			free(handle);
	}
done:
	if(bp== &block)
		block_done(bp);
	if(nq && nq->nvalue && ((struct Ufunction*)nq->nvalue)->running==1)
	{
		((struct Ufunction*)nq->nvalue)->running=0;
		nv_unset(nq,0);
	}
	if(jmpval >= SH_JMPFUN)
		siglongjmp(*sh.jmplist,jmpval);
}

/*
 * This function executes a lookup disc and then performs
 * the lookup on the given node <np>
 */
static char*	lookup(Namval_t *np, int type, Sfdouble_t *dp,Namfun_t *handle)
{
	struct vardisc	*vp = (struct vardisc*)handle;
	struct blocked	block, *bp = block_info(np, &block);
	Namval_t	*nq = vp->disc[type];
	char		*cp=0;
	Namval_t	node;
	void		*saveval = np->nvalue;
	int		jmpval = 0;
	if(nq && !isblocked(bp,type))
	{
		struct checkpt	checkpoint;
		int		savexit = sh.savexit;
		Lex_t		*lexp = (Lex_t*)sh.lex_context, savelex;
		/* disciplines like PS2 may run at parse time; save, reinit and restore the lexer state */
		savelex = *lexp;
		sh_lexopen(lexp, 0);   /* needs full init (0), not what it calls reinit (1) */
		node = *SH_VALNOD;
		if(!nv_isnull(SH_VALNOD))
		{
			nv_onattr(SH_VALNOD,NV_NOFREE);
			nv_unset(SH_VALNOD,0);
		}
		if(type==LOOKUPN)
		{
			nv_onattr(SH_VALNOD,NV_DOUBLE|NV_INTEGER);
			nv_setsize(SH_VALNOD,10);
		}
		block(bp,type);
		block(bp, UNASSIGN);   /* make sure nv_setdisc doesn't invalidate 'vp' by freeing it */
		sh_pushcontext(&checkpoint, SH_JMPFUN);
		jmpval = sigsetjmp(checkpoint.buff, 0);
		if(!jmpval)
			sh_fun(nq,np,NULL);
		sh_popcontext(&checkpoint);
		if(sh.topfd != checkpoint.topfd)
			sh_iorestore(checkpoint.topfd, jmpval);
		unblock(bp,UNASSIGN);
		unblock(bp,type);
		if(!vp->disc[type])
			chktfree(np,vp);
		if(type==LOOKUPN)
		{
			cp = SH_VALNOD->nvalue;
			*dp = nv_getnum(SH_VALNOD);
		}
		else if(cp = nv_getval(SH_VALNOD))
			cp = stkcopy(sh.stk,cp);
		nv_unset(SH_VALNOD,NV_RDONLY);
		if(!nv_isnull(&node))
		{
			/* restore everything but the nvlink field */
			memcpy(&SH_VALNOD->nvname,  &node.nvname, sizeof(node)-sizeof(node.nvlink));
		}
		*lexp = savelex;
		sh.savexit = savexit;	/* avoid influencing $? */
	}
	if(nv_isarray(np))
		np->nvalue = saveval;
	if(bp== &block)
		block_done(bp);
	if(nq && nq->nvalue && ((struct Ufunction*)nq->nvalue)->running==1)
	{
		((struct Ufunction*)nq->nvalue)->running=0;
		nv_unset(nq,0);
	}
	if(jmpval >= SH_JMPFUN)
		siglongjmp(*sh.jmplist,jmpval);
	sh_sigcheck();
	/* nv_get{v,n} may throw an error and longjmp, so must come after restoring all state */
	if(!cp)
	{
		if(type==LOOKUPS)
			cp = nv_getv(np,handle);
		else
			*dp = nv_getn(np,handle);
	}
	return cp;
}

static char*	lookups(Namval_t *np, Namfun_t *handle)
{
	return lookup(np,LOOKUPS,NULL,handle);
}

static Sfdouble_t lookupn(Namval_t *np, Namfun_t *handle)
{
	Sfdouble_t	d;
	lookup(np,LOOKUPN, &d ,handle);
	return d;
}


/*
 * Set disc on given <event> to <action>
 * If action==np, the current disc is returned
 * A null return value indicates that no <event> is known for <np>
 * If <event> is NULL, then return the event name after <action>
 * If <event> is NULL, and <action> is NULL, return the first event
 */
char *nv_setdisc(Namval_t* np,const char *event,Namval_t *action,Namfun_t *fp)
{
	struct vardisc *vp = (struct vardisc*)np->nvfun;
	int type = -1;
	char *empty = "";
	while(vp)
	{
		if(vp->fun.disc && (vp->fun.disc->setdisc || vp->fun.disc->putval == assign))
			break;
		vp = (struct vardisc*)vp->fun.next;
	}
	if(vp && !vp->fun.disc)
		vp = 0;
	if(np == (Namval_t*)fp)
	{
		const char *name;
		int getname=0;
		/* top level call, check for get/set */
		if(!event)
		{
			if(!action)
				return (char*)nv_discnames[0];
			getname=1;
			event = (char*)action;
		}
		for(type=0; name=nv_discnames[type]; type++)
		{
			if(strcmp(event,name)==0)
				break;
		}
		if(getname)
		{
			event = 0;
			if(name && !(name = nv_discnames[++type]))
				action = 0;
		}
		if(!name)
		{
			for(fp=(Namfun_t*)vp; fp; fp=fp->next)
			{
				if(fp->disc && fp->disc->setdisc)
					return (*fp->disc->setdisc)(np,event,action,fp);
			}
		}
		else if(getname)
			return (char*)name;
	}
	if(!fp)
		return NULL;
	if(np != (Namval_t*)fp)
	{
		/* not the top level */
		while(fp = fp->next)
		{
			if(fp->disc && fp->disc->setdisc)
				return (*fp->disc->setdisc)(np,event,action,fp);
		}
		return NULL;
	}
	if (type < 0)
		return NULL;
	/* Handle GET/SET/APPEND/UNSET disc */
	if(np==SH_VALNOD || np==SH_LEVELNOD)
		return NULL;
	if(vp && vp->fun.disc->putval!=assign)
		vp = 0;
	if(!vp)
	{
		Namdisc_t	*dp;
		if(action==np)
			return (char*)action;
		vp = sh_newof(NULL,struct vardisc,1,sizeof(Namdisc_t));
		dp = (Namdisc_t*)(vp+1);
		vp->fun.disc = dp;
		memset(dp,0,sizeof(*dp));
		dp->dsize = sizeof(struct vardisc);
		dp->putval = assign;
		if(nv_isarray(np) && !nv_arrayptr(np))
			nv_putsub(np,NULL, 1);
		nv_stack(np, (Namfun_t*)vp);
	}
	if(action==np)
	{
		action = vp->disc[type];
		empty = 0;
	}
	else if(action)
	{
		Namdisc_t *dp = (Namdisc_t*)vp->fun.disc;
		if(type==LOOKUPS)
			dp->getval = lookups;
		else if(type==LOOKUPN)
			dp->getnum = lookupn;
		vp->disc[type] = action;
		nv_optimize_clear(np);
	}
	else
	{
		struct blocked *bp;
		action = vp->disc[type];
		vp->disc[type] = 0;
		if(!(bp=block_info(np,NULL)) || !isblocked(bp,UNASSIGN))
			chktfree(np,vp);
	}
	return action ? (char*)action : empty;
}

/*
 * Set disc on given <event> to <action>
 * If action==np, the current disc is returned
 * A null return value indicates that no <event> is known for <np>
 * If <event> is NULL, then return the event name after <action>
 * If <event> is NULL, and <action> is NULL, return the first event
 */
static char *setdisc(Namval_t* np,const char *event,Namval_t *action,Namfun_t *fp)
{
	Nambfun_t *vp = (Nambfun_t*)fp;
	int type,getname=0;
	const char *name;
	const char **discnames = vp->bnames;
	/* top level call, check for discipline match */
	if(!event)
	{
		if(!action)
			return (char*)discnames[0];
		getname=1;
		event = (char*)action;
	}
	for(type=0; name=discnames[type]; type++)
	{
		if(strcmp(event,name)==0)
			break;
	}
	if(getname)
	{
		event = 0;
		if(name && !(name = discnames[++type]))
			action = 0;
	}
	if(!name)
		return nv_setdisc(np,event,action,fp);
	else if(getname)
		return (char*)name;
	/* Handle the disciplines */
	if(action==np)
		action = vp->bltins[type];
	else if(action)
	{
		Namval_t *tp = nv_type(np);
		if(tp && (np = (Namval_t*)vp->bltins[type]) && nv_isattr(np,NV_STATICF))
		{
			errormsg(SH_DICT,ERROR_exit(1),e_staticfun,name,tp->nvname);
			UNREACHABLE();
		}
		vp->bltins[type] = action;
	}
	else
	{
		action = vp->bltins[type];
		vp->bltins[type] = 0;
	}
	return (char*)action;
}

static void putdisc(Namval_t* np, const char* val, int flag, Namfun_t* fp)
{
	nv_putv(np,val,flag,fp);
	if(!val && !(flag&NV_NOFREE))
	{
		Nambfun_t *vp = (Nambfun_t*)fp;
		int i;
		for(i=0; vp->bnames[i]; i++)
		{
			Namval_t *mp;
			if((mp=vp->bltins[i]) && !nv_isattr(mp,NV_NOFREE))
			{
				if(is_abuiltin(mp))
				{
					if(mp->nvfun && !nv_isattr(mp,NV_NOFREE))
						free(mp->nvfun);
					dtdelete(sh.bltin_tree,mp);
					free(mp);
				}
			}
		}
		nv_disc(np,fp,NV_POP);
		if(!(fp->nofree&1))
			free(fp);
	}
}

static const Namdisc_t Nv_bdisc	= {   0, putdisc, 0, 0, setdisc };

Namfun_t *nv_clone_disc(Namfun_t *fp, int flags)
{
	Namfun_t	*nfp;
	int		size;
	if(!fp->disc && !fp->next && (fp->nofree&1))
		return fp;
	if(!(size=fp->dsize) && (!fp->disc || !(size=fp->disc->dsize)))
		size = sizeof(Namfun_t);
	nfp = sh_newof(NULL,Namfun_t,1,size-sizeof(Namfun_t));
	memcpy(nfp,fp,size);
	nfp->nofree &= ~1;
	nfp->nofree |= (flags&NV_RDONLY)?1:0;
	return nfp;
}

int nv_adddisc(Namval_t *np, const char **names, Namval_t **funs)
{
	Nambfun_t *vp;
	int n=0;
	const char **av=names;
	if(av)
	{
		while(*av++)
			n++;
	}
	vp = sh_newof(NULL,Nambfun_t,1,n*sizeof(Namval_t*));
	vp->fun.dsize = sizeof(Nambfun_t)+n*sizeof(Namval_t*);
	vp->fun.nofree |= 2;
	vp->num = n;
	if(funs)
		memcpy(vp->bltins, funs,n*sizeof(Namval_t*));
	else while(n>=0)
		vp->bltins[n--] = 0;
	vp->fun.disc = &Nv_bdisc;
	vp->bnames = names;
	nv_stack(np,&vp->fun);
	return 1;
}

/*
 * push, pop, clne, or reorder disciplines onto node <np>
 * mode can be one of
 *    NV_FIRST:  Move or push <fp> to top of the stack or delete top
 *    NV_LAST:	 Move or push <fp> to bottom of stack or delete last
 *    NV_POP:	 Delete <fp> from top of the stack
 *    NV_CLONE:  Replace fp with a copy created my malloc() and return it
 */
Namfun_t *nv_disc(Namval_t *np, Namfun_t* fp, int mode)
{
	Namfun_t *lp, **lpp;
	if(nv_isref(np))
		return NULL;
	if(mode==NV_CLONE && !fp)
		return NULL;
	if(fp)
	{
		fp->subshell = sh.subshell;
		if((lp=np->nvfun)==fp)
		{
			if(mode==NV_CLONE)
			{
				lp = nv_clone_disc(fp,0);
				return np->nvfun=lp;
			}
			if(mode==NV_FIRST || mode==0)
				return fp;
			np->nvfun = lp->next;
			if(mode==NV_POP)
				return fp;
			if(mode==NV_LAST && (lp->next==0 || lp->next->disc==0))
				return fp;
		}
		/* see if <fp> is on the list already */
		lpp = &np->nvfun;
		if(lp)
		{
			while(lp->next && lp->next->disc)
			{
				if(lp->next==fp)
				{
					if(mode==NV_LAST && fp->next==0)
						return fp;
					if(mode==NV_CLONE)
					{
						fp = nv_clone_disc(fp,0);
						lp->next = fp;
						return fp;
					}
					lp->next = fp->next;
					if(mode==NV_POP)
						return fp;
					if(mode!=NV_LAST)
						break;
				}
				lp = lp->next;
			}
			if(mode==NV_LAST && lp->disc)
				lpp = &lp->next;
		}
		if(mode==NV_POP)
			return NULL;
		/* push */
		nv_offattr(np,NV_NODISC);
		if(mode==NV_LAST)
		{
			if(lp && !lp->disc)
				fp->next = lp;
			else
				fp->next = *lpp;
		}
		else
		{
			if((fp->nofree&1) && *lpp)
				fp = nv_clone_disc(fp,0);
			fp->next = *lpp;
		}
		*lpp = fp;
	}
	else
	{
		if(mode==NV_FIRST)
			return np->nvfun;
		else if(mode==NV_LAST)
			for(lp=np->nvfun; lp; fp=lp,lp=lp->next);
		else if(fp = np->nvfun)
			np->nvfun = fp->next;
	}
	return fp;
}

/*
 * returns discipline pointer if discipline with specified functions
 * is on the discipline stack
 */
Namfun_t *nv_hasdisc(Namval_t *np, const Namdisc_t *dp)
{
	Namfun_t *fp;
	for(fp=np->nvfun; fp; fp = fp->next)
	{
		if(fp->disc== dp)
			return fp;
	}
	return NULL;
}

static void *newnode(const char *name)
{
	int s;
	Namval_t *np = sh_newof(0,Namval_t,1,s=strlen(name)+1);
	np->nvname = (char*)np+sizeof(Namval_t);
	memcpy(np->nvname,name,s);
	return np;
}

/*
 * clone a numeric value
 */
static void *num_clone(Namval_t *np, void *val)
{
	int size;
	void *nval;
	if(!val)
		return NULL;
	if(nv_isattr(np,NV_DOUBLE)==NV_DOUBLE)
	{
		if(nv_isattr(np,NV_LONG))
			size = sizeof(Sfdouble_t);
		else if(nv_isattr(np,NV_SHORT))
			size = sizeof(float);
		else
			size = sizeof(double);
	}
	else
	{
		if(nv_isattr(np,NV_LONG))
			size = sizeof(Sflong_t);
		else if(nv_isattr(np,NV_SHORT))
			size = sizeof(int16_t);
		else
			size = sizeof(int32_t);
	}
	nval = sh_malloc(size);
	memcpy(nval,val,size);
	return nval;
}

void clone_all_disc( Namval_t *np, Namval_t *mp, int flags)
{
	Namfun_t *fp, **mfp = &mp->nvfun, *nfp, *fpnext;
	for(fp=np->nvfun; fp;fp=fpnext)
	{
		fpnext = fp->next;
		if(!fpnext && (flags&NV_COMVAR) && fp->disc && fp->disc->namef)
			return;
		if((fp->nofree&2) && (flags&NV_NODISC))
			nfp = 0;
		if(fp->disc && fp->disc->clonef)
			nfp = (*fp->disc->clonef)(np,mp,flags,fp);
		else	if(flags&NV_MOVE)
			nfp = fp;
		else
			nfp = nv_clone_disc(fp,flags);
		if(!nfp)
			continue;
		nfp->next = 0;
		*mfp = nfp;
		mfp = &nfp->next;
	}
}

/*
 * clone <mp> from <np> flags can be one of the following
 * NV_APPEND - append <np> onto <mp>
 * NV_MOVE - move <np> to <mp>
 * NV_NOFREE - mark the new node as nofree
 * NV_NODISC - disciplines with funs non-zero will not be copied
 * NV_COMVAR - cloning a compound variable
 */
int nv_clone(Namval_t *np, Namval_t *mp, int flags)
{
	Namfun_t	*fp, *fpnext;
	const char	*val = mp->nvalue;
	unsigned short	flag = mp->nvflag;
	unsigned short	size = mp->nvsize;
	for(fp=mp->nvfun; fp; fp=fpnext)
	{
		fpnext = fp->next;
		if(!fpnext && (flags&NV_COMVAR) && fp->disc && fp->disc->namef)
			break;
		if(!(fp->nofree&1))
			free(fp);
	}
	mp->nvfun = fp;
	if(fp=np->nvfun)
	{
		if(nv_isattr(mp,NV_EXPORT|NV_MINIMAL) == (NV_EXPORT|NV_MINIMAL))
		{
			mp->nvmeta = NULL;
			nv_offattr(mp,NV_MINIMAL);
		}
		if(!(flags&NV_COMVAR) && !nv_isattr(np,NV_MINIMAL) && np->nvmeta && !(nv_isattr(mp,NV_MINIMAL)))
			mp->nvmeta = np->nvmeta;
		mp->nvflag &= NV_MINIMAL;
	        mp->nvflag |= np->nvflag&~(NV_ARRAY|NV_MINIMAL|NV_NOFREE);
		flag = mp->nvflag;
		clone_all_disc(np, mp, flags);
	}
	if(flags&NV_APPEND)
		return 1;
	if(mp->nvsize == size)
		nv_setsize(mp,nv_size(np));
	if(mp->nvflag == flag)
		mp->nvflag = (np->nvflag&~(NV_MINIMAL))|(mp->nvflag&NV_MINIMAL);
	if(nv_isattr(np,NV_EXPORT))
		mp->nvflag |= (np->nvflag&NV_MINIMAL);
	if(mp->nvalue==val && !nv_isattr(np,NV_INTEGER))
	{
		if(np->nvalue && np->nvalue!=Empty && (!flags || ((flags&NV_COMVAR) && !(flags&NV_MOVE))))
		{
			if(size)
				mp->nvalue = sh_memdup(np->nvalue,size);
			else
			        mp->nvalue = sh_strdup(np->nvalue);
			nv_offattr(mp,NV_NOFREE);
		}
		else if((np->nvfun || !nv_isattr(np,NV_ARRAY)) && !(mp->nvalue = np->nvalue))
			nv_offattr(mp,NV_NOFREE);
	}
	if(flags&NV_MOVE)
	{
		if(nv_isattr(np,NV_INTEGER))
			mp->nvalue = np->nvalue;
		np->nvfun = NULL; /* This will remove the discipline function, if there is one */
		np->nvalue = NULL;
		if(!nv_isattr(np,NV_MINIMAL) || nv_isattr(mp,NV_EXPORT))
		{
			mp->nvmeta = np->nvmeta;
			if(nv_isattr(np,NV_MINIMAL))
			{
				np->nvmeta = NULL;
				np->nvflag = NV_EXPORT;
			}
			else
				np->nvflag = 0;
		}
		else
			np->nvflag &= NV_MINIMAL;
	        nv_setsize(np,0);
		return 1;
	}
	else if((flags&NV_ARRAY) && !nv_isattr(np,NV_MINIMAL))
		mp->nvmeta = np->nvmeta;
	if(nv_isattr(np,NV_INTEGER) && !nv_isarray(np) && mp->nvalue!=np->nvalue && np->nvalue!=Empty)
	{
		mp->nvalue = num_clone(np,np->nvalue);
		nv_offattr(mp,NV_NOFREE);
	}
	else if((flags&NV_NOFREE) && !nv_arrayptr(np))
	        nv_onattr(np,NV_NOFREE);
	return 1;
}

/*
 * Low-level function to look up <name> in <root>. Returns a pointer to the
 * name-value node found, or NULL if not found. <mode> is an options bitmask:
 * - If NV_NOSCOPE is set, the search is only done in <root> itself and
 *   not in any trees it has a view to (i.e. is connected to through scoping).
 * - If NV_ADD is set and <name> is not found, a node by that name is created
 *   in <root> and a pointer to the newly created node is returned.
 * - If NV_REF is set, <name> is a pointer to a name-value node, and that
 *   node's name is used.
 * Note: The mode bitmask is NOT compatible with nv_open's flags bitmask.
 */
Namval_t *nv_search(const char *name, Dt_t *root, int mode)
{
	Namval_t *np;
	Dt_t *dp = 0;
	/* do not find builtins when using 'command -x' */
	if(!(mode&NV_ADD) && sh_isstate(SH_XARG) && (root==sh.bltin_tree || root==sh.fun_tree))
		return NULL;
	if(mode&NV_NOSCOPE)
		dp = dtview(root,0);
	if(mode&NV_REF)
	{
		Namval_t *mp = (void*)name;
		if(!(np = dtsearch(root,mp)) && (mode&NV_ADD))
			name = nv_name(mp);
	}
	else
	{
		if(*name=='.' && root==sh.var_tree && !dp)
			root = sh.var_base;
		np = dtmatch(root,name);
	}
	/* skip dummy shell function node (function unset in virtual subshell) */
	if(np && !np->nvflag && root==sh.fun_tree)
		np = mode&NV_NOSCOPE ? NULL : dtmatch(sh.bltin_tree,name);
	if(!np && (mode&NV_ADD))
	{
		if(sh.namespace && !(mode&NV_NOSCOPE) && root==sh.var_tree)
			root = nv_dict(sh.namespace);
		else if(!dp && !(mode&NV_NOSCOPE))
		{
			Dt_t *next;
			while(next=dtvnext(root))
				root = next;
		}
		np = (Namval_t*)dtinsert(root,newnode(name));
	}
	if(dp)
		dtview(root,dp);
	return np;
}

/*
 * Finds function or builtin for given name and the discipline variable.
 * If var!=0, the variable pointer is returned and the built-in name is put onto the stack at the current offset.
 * Otherwise, a pointer to the built-in (variable or type) is returned and var contains the pointer to the variable.
 * If last==0 and first component of name is a reference, nv_bfsearch() will return NULL.
 */
Namval_t *nv_bfsearch(const char *name, Dt_t *root, Namval_t **var, char **last)
{
	int		c,offset = stktell(sh.stk);
	char		*sp, *cp=0;
	Namval_t	*np, *nq;
	char		*dname=0;
	if(var)
		*var = 0;
	/* check for . in the name before = */
	for(sp=(char*)name+1; *sp; sp++)
	{
		if(*sp=='=')
			return NULL;
		if(*sp=='[')
		{
			while(*sp=='[')
			{
				sp = nv_endsubscript(NULL,(char*)sp,0);
				if(sp[-1]!=']')
					return NULL;
			}
			if(*sp==0)
				break;
			if(*sp!='.')
				return NULL;
			cp = sp;
		}
		else if(*sp=='.')
			cp = sp;
	}
	if(!cp)
		return var ? nv_search(name,root,0) : NULL;
	sfputr(sh.stk,name,0);
	dname = cp+1;
	cp = stkptr(sh.stk,offset) + (cp-name);
	if(last)
		*last = cp;
	c = *cp;
	*cp = 0;
	nq=nv_open(stkptr(sh.stk,offset),0,NV_VARNAME|NV_NOADD|NV_NOFAIL);
	*cp = c;
	if(!nq)
	{
		np = 0;
		goto done;
	}
	if(!var)
	{
		np = nq;
		goto done;
	}
	*var = nq;
	if(c=='[')
		nv_endsubscript(nq, cp,NV_NOADD);
	stkseek(sh.stk,offset);
#if SHOPT_NAMESPACE
	if(nv_istable(nq))
	{
		Namval_t *nsp = sh.namespace;
		if(last==0)
			return nv_search(name,root,0);
		sh.namespace = 0;
		sfputr(sh.stk,nv_name(nq),-1);
		sh.namespace = nsp;
		sfputr(sh.stk,dname-1,0);
		np = nv_search(stkptr(sh.stk,offset),root,0);
		stkseek(sh.stk,offset);
		return np;
	}
#endif /* SHOPT_NAMESPACE */
	while(nv_isarray(nq) && !nv_isattr(nq,NV_MINIMAL|NV_EXPORT) && nq->nvmeta && nv_isarray((Namval_t*)nq->nvmeta))
		nq = nq->nvmeta;
	return (Namval_t*)nv_setdisc(nq,dname,nq,(Namfun_t*)nq);
done:
	stkseek(sh.stk,offset);
	return np;
}

/*
 * add or replace built-in version of command corresponding to <path>
 * The <bltin> argument is a pointer to the built-in
 * if <extra>==1, the built-in will be deleted
 * Special builtins cannot be added or deleted return failure
 * The return value for adding builtins is a pointer to the node or NULL on
 *   failure.  For delete NULL means success and the node that cannot be
 *   deleted is returned on failure.
 */
Namval_t *sh_addbuiltin(const char *path, Shbltin_f bltin, void *extra)
{
	const char	*name;
	char		*cp;
	Namval_t	*np, *nq=0;
	int		offset=stktell(sh.stk);
	if(extra==(void*)1)
		name = path;
	else if((name = path_basename(path))==path && bltin!=b_typeset && (nq=nv_bfsearch(name,sh.bltin_tree,NULL,&cp)))
		path = name = stkptr(sh.stk,offset);
	else if(sh.bltin_dir && extra!=(void*)1)
	{
		sfputr(sh.stk,sh.bltin_dir,'/');
		sfputr(sh.stk,name,0);
		path = stkptr(sh.stk,offset);
	}
	if(np = nv_search(name,sh.bltin_tree,0))
	{
		/* exists without a path */
		stkseek(sh.stk,offset);
		if(extra == (void*)1)
		{
			if(nv_isattr(np,BLT_SPC) && !sh_isstate(SH_INIT))
			{
				/* builtin(1) cannot delete special builtins */
				errormsg(SH_DICT,ERROR_exit(1),"cannot delete: %s%s",name,is_spcbuiltin);
				UNREACHABLE();
			}
			if(np->nvfun && !nv_isattr(np,NV_NOFREE))
				free(np->nvfun);
			dtdelete(sh.bltin_tree,np);
			return NULL;
		}
		if(!bltin)
			return np;
	}
	else for(np=(Namval_t*)dtfirst(sh.bltin_tree);np;np=(Namval_t*)dtnext(sh.bltin_tree,np))
	{
		if(strcmp(name,path_basename(nv_name(np))))
			continue;
		/* exists probably with different path so delete it */
		if(strcmp(path,nv_name(np)))
		{
			if(nv_isattr(np,BLT_SPC))
				return np;
			if(!bltin)
				bltin = funptr(np);
			if(np->nvmeta)
				dtdelete(sh.bltin_tree,np);
			if(extra == (void*)1)
				return NULL;
			np = NULL;
		}
		break;
	}
	if(!np && !(np = nv_search(path,sh.bltin_tree,bltin?NV_ADD:0)))
		return NULL;
	stkseek(sh.stk,offset);
	if(nv_isattr(np,BLT_SPC))
	{
		if(extra)
			np->nvfun = (Namfun_t*)extra;
		return np;
	}
	np->nvmeta = NULL;
	np->nvfun = NULL;
	if(bltin)
	{
		np->nvalue = bltin;
		nv_onattr(np,NV_BLTIN|NV_NOFREE);
		np->nvfun = (Namfun_t*)extra;
	}
	if(nq)
	{
		cp=nv_setdisc(nq,cp+1,np,(Namfun_t*)nq);
		if(!cp)
		{
			errormsg(SH_DICT,ERROR_exit(1),e_baddisc,name);
			UNREACHABLE();
		}
	}
	if(extra == (void*)1)
		return NULL;
	return np;
}

#undef nv_stack
extern Namfun_t *nv_stack(Namval_t *np, Namfun_t* fp)
{
	return nv_disc(np,fp,0);
}

struct table
{
	Namfun_t	fun;
	Namval_t	*parent;
	Dt_t		*dict;
};

static Namval_t *next_table(Namval_t* np, Dt_t *root,Namfun_t *fp)
{
	struct table *tp = (struct table *)fp;
	if(root)
		return (Namval_t*)dtnext(root,np);
	else
		return (Namval_t*)dtfirst(tp->dict);
}

static Namval_t *create_table(Namval_t *np,const char *name,int flags,Namfun_t *fp)
{
	struct table *tp = (struct table *)fp;
	sh.last_table = np;
	return nv_create(name, tp->dict, flags, fp);
}

static Namfun_t *clone_table(Namval_t* np, Namval_t *mp, int flags, Namfun_t *fp)
{
	struct table	*tp = (struct table*)fp;
	struct table	*ntp = (struct table*)nv_clone_disc(fp,0);
	Dt_t		*oroot=tp->dict,*nroot=dtopen(&_Nvdisc,Dtoset);
	if(!nroot)
		return NULL;
	memcpy(ntp,fp,sizeof(struct table));
	ntp->dict = nroot;
	ntp->parent = nv_lastdict();
	for(np=(Namval_t*)dtfirst(oroot);np;np=(Namval_t*)dtnext(oroot,np))
	{
		mp = (Namval_t*)dtinsert(nroot,newnode(np->nvname));
		nv_clone(np,mp,flags);
	}
	return &ntp->fun;
}

/*
 * The first two fields must correspond with those in 'struct adata' in name.c and 'struct tdata' in typeset.c
 * (those fields are used via a type conversion in scanfilter() in name.c)
 */
struct adata
{
	Namval_t	*tp;
	char		*mapname;
	char		**argnam;
	int		attsize;
	char		*attval;
};

static void delete_fun(Namval_t *np, void *data)
{
	NOT_USED(data);
	nv_delete(np,sh.fun_tree,NV_NOFREE);
}

static void put_table(Namval_t* np, const char* val, int flags, Namfun_t* fp)
{
	Dt_t		*root = ((struct table*)fp)->dict;
	Namval_t	*nq, *mp;
	Namarr_t	*ap;
	struct adata	data;
	if(val)
	{
		nv_putv(np,val,flags,fp);
		return;
	}
	if(nv_isarray(np) && (ap=nv_arrayptr(np)) && array_elem(ap))
		return;
	memset(&data,0,sizeof(data));
	data.mapname = nv_name(np);
	nv_scan(sh.fun_tree,delete_fun,&data,NV_FUNCTION,NV_FUNCTION|NV_NOSCOPE);
	dtview(root,0);
	for(mp=(Namval_t*)dtfirst(root);mp;mp=nq)
	{
		nv_unset(mp,flags);
		nq = (Namval_t*)dtnext(root,mp);
		dtdelete(root,mp);
		free(mp);
	}
	if(sh.last_root==root)
		sh.last_root = NULL;
	dtclose(root);
	if(!(fp->nofree&1))
		free(fp);
	np->nvfun = 0;
}

/*
 * return space separated list of names of variables in given tree
 */
static char *get_table(Namval_t *np, Namfun_t *fp)
{
	Dt_t *root = ((struct table*)fp)->dict;
	static Sfio_t *out;
	int first=1;
	Dt_t *base = dtview(root,0);
	if(out)
		sfseek(out,0,SEEK_SET);
	else
		out =  sfnew(NULL,NULL,-1,-1,SFIO_WRITE|SFIO_STRING);
	for(np=(Namval_t*)dtfirst(root);np;np=(Namval_t*)dtnext(root,np))
	{
		if(!nv_isnull(np) || np->nvfun || nv_isattr(np,~NV_NOFREE))
		{
			if(!first)
				sfputc(out,' ');
			else
				first = 0;
			sfputr(out,np->nvname,-1);
		}
	}
	sfputc(out,0);
	if(base)
		dtview(root,base);
	return (char*)out->_data;
}

static const Namdisc_t table_disc =
{
	sizeof(struct table),
	put_table,
	get_table,
	0,
	0,
	create_table,
	clone_table,
	0,
	next_table,
};

Namval_t *nv_parent(Namval_t *np)
{
	struct table *tp = (struct table *)nv_hasdisc(np,&table_disc);
	if(tp)
		return tp->parent;
	return NULL;
}

Dt_t *nv_dict(Namval_t* np)
{
	struct table *tp = (struct table*)nv_hasdisc(np,&table_disc);
	if(tp)
		return tp->dict;
	np = sh.last_table;
	if(np && (tp = (struct table*)nv_hasdisc(np,&table_disc)))
		return tp->dict;
	return sh.var_tree;
}

int nv_istable(Namval_t *np)
{
	return nv_hasdisc(np,&table_disc)!=0;
}

/*
 * create a mountable name-value pair tree
 */
Namval_t *nv_mount(Namval_t *np, const char *name, Dt_t *dict)
{
	Namval_t *mp, *pp;
	struct table *tp;
	if(nv_hasdisc(np,&table_disc))
		pp = np;
	else
		pp = nv_lastdict();
	tp = sh_newof(NULL, struct table,1,0);
	if(name)
	{
		Namfun_t *fp = pp->nvfun;
		mp = (*fp->disc->createf)(pp,name,0,fp);
	}
	else
		mp = np;
	nv_offattr(mp,NV_TABLE);
	if(!nv_isnull(mp))
		nv_unset(mp,NV_RDONLY);
	tp->dict = dict;
	tp->parent = pp;
	tp->fun.disc = &table_disc;
	nv_disc(mp, &tp->fun, NV_FIRST);
	return mp;
}

const Namdisc_t *nv_discfun(int which)
{
	switch(which)
	{
	    case NV_DCADD:
		return &Nv_bdisc;
	    case NV_DCRESTRICT:
		return &RESTRICTED_disc;
	}
	return NULL;
}

/*
 * Check if a variable node has a 'get' discipline.
 * Used by the nv_isnull() macro (see include/name.h).
 */
int nv_hasget(Namval_t *np)
{
	Namfun_t	*fp;
	if(np==sh_scoped(IFSNOD))
		return 0;	/* avoid BUG_IFSISSET: always return false for IFS */
	for(fp=np->nvfun; fp; fp=fp->next)
	{
		if(!fp->disc || (!fp->disc->getnum && !fp->disc->getval))
			continue;
		return 1;
	}
	return 0;
}

#if SHOPT_NAMESPACE
Namval_t *sh_fsearch(const char *fname, int add)
{
	if(*fname!='.')
	{
		int	offset = stktell(sh.stk);
		sfputr(sh.stk,nv_name(sh.namespace),'.');
		sfputr(sh.stk,fname,0);
		fname = stkptr(sh.stk,offset);
	}
	return nv_search(fname,sh_subfuntree(add&NV_ADD),add);
}
#endif /* SHOPT_NAMESPACE */

/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2024 Contributors to ksh 93u+m           *
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
 * shell intermediate code reader
 *
 */

#include	"shopt.h"
#include	"defs.h"
#include	"shnodes.h"
#include	"path.h"
#include	"io.h"

static struct dolnod	*r_comlist(void);
static struct argnod	*r_arg(void);
static struct ionod	*r_redirect(void);
static struct regnod	*r_switch(void);
static Shnode_t		*r_tree(void);
static char		*r_string(void);
static void		r_comarg(struct comnod*);

static Sfio_t *infile;

#define getnode(type)   stkalloc(sh.stk,sizeof(struct type))

Shnode_t *sh_trestore(Sfio_t *in)
{
	infile = in;
	return r_tree();
}
/*
 * read in a shell tree
 */
static Shnode_t *r_tree(void)
{
	long l = sfgetl(infile);
	int type;
	Shnode_t *t=0;
	if(l<0)
		return t;
	type = l;
	switch(type&COMMSK)
	{
		case TTIME:
		case TPAR:
			t = getnode(parnod);
			t->par.partre = r_tree();
			break;
		case TCOM:
			t = getnode(comnod);
			t->tre.tretyp = type;
			r_comarg((struct comnod*)t);
			break;
		case TSETIO:
		case TFORK:
			t = getnode(forknod);
			t->fork.forkline = sfgetu(infile);
			t->fork.forktre = r_tree();
			t->fork.forkio = r_redirect();
			break;
		case TIF:
			t = getnode(ifnod);
			t->if_.iftre = r_tree();
			t->if_.thtre = r_tree();
			t->if_.eltre = r_tree();
			break;
		case TWH:
			t = getnode(whnod);
			t->wh.whinc = (struct arithnod*)r_tree();
			t->wh.whtre = r_tree();
			t->wh.dotre = r_tree();
			break;
		case TLST:
		case TAND:
		case TORF:
		case TFIL:
			t = getnode(lstnod);
			t->lst.lstlef = r_tree();
			t->lst.lstrit = r_tree();
			break;
		case TARITH:
			t = getnode(arithnod);
			t->ar.arline = sfgetu(infile);
			t->ar.arexpr = r_arg();
			t->ar.arcomp = 0;
			if((t->ar.arexpr)->argflag&ARG_RAW)
				 t->ar.arcomp = sh_arithcomp((t->ar.arexpr)->argval);
			break;
		case TFOR:
			t = getnode(fornod);
			t->for_.forline = 0;
			if(type&FLINENO)
				t->for_.forline = sfgetu(infile);
			t->for_.fortre = r_tree();
			t->for_.fornam = r_string();
			t->for_.forlst = (struct comnod*)r_tree();
			break;
		case TSW:
			t = getnode(swnod);
			t->sw.swline = 0;
			if(type&FLINENO)
				t->sw.swline = sfgetu(infile);
			t->sw.swarg = r_arg();
			if(type&COMSCAN)
				t->sw.swio = r_redirect();
			else
				t->sw.swio = 0;
			t->sw.swlst = r_switch();
			break;
		case TFUN:
		{
			Stk_t *savstak;
			struct slnod *slp;
			struct functnod *fp;
			t = getnode(functnod);
			t->funct.functline = sfgetu(infile);
			t->funct.functnam = r_string();
			savstak = sh.stk;
			sh.stk = stkopen(STK_SMALL);
			slp = stkalloc(sh.stk,sizeof(struct slnod)+sizeof(struct functnod));
			slp->slchild = 0;
			slp->slnext = sh.st.staklist;
			sh.st.staklist = 0;
			fp = (struct functnod*)(slp+1);
			memset(fp, 0, sizeof(*fp));
			fp->functtyp = TFUN|FAMP;
			if(sh.st.filename)
				fp->functnam = stkcopy(sh.stk,sh.st.filename);
			t->funct.functtre = r_tree();
			t->funct.functstak = slp;
			t->funct.functargs = (struct comnod*)r_tree();
			slp->slptr = sh.stk;
			sh.stk = savstak;
			slp->slchild = sh.st.staklist;
			break;
		}
		case TTST:
			t = getnode(tstnod);
			t->tst.tstline = sfgetu(infile);
			if((type&TPAREN)==TPAREN)
				t->lst.lstlef = r_tree();
			else
			{
				t->lst.lstlef = (Shnode_t*)r_arg();
				if((type&TBINARY))
					t->lst.lstrit = (Shnode_t*)r_arg();
			}
	}
	if(t)
		t->tre.tretyp = type;
	return t;
}

static struct argnod *r_arg(void)
{
	struct argnod	*ap=0, *apold, *aptop=0;
	long		l;
	Stk_t		*stkp=sh.stk;
	while((l=sfgetu(infile))>0)
	{
		ap = stkseek(stkp,(unsigned)l+ARGVAL);
		if(!aptop)
			aptop = ap;
		else
			apold->argnxt.ap = ap;
		if(--l > 0)
			sfread(infile,ap->argval,(size_t)l);
		ap->argval[l] = 0;
		ap->argchn.cp = 0;
		ap->argflag = sfgetc(infile);
		ap = stkfreeze(stkp,0);
		if(*ap->argval==0 && (ap->argflag&ARG_EXP))
			ap->argchn.ap = (struct argnod*)r_tree();
		else if(*ap->argval==0 && (ap->argflag&~(ARG_APPEND|ARG_MESSAGE|ARG_QUOTED|ARG_ARRAY))==0)
		{
			struct fornod *fp = (struct fornod*)getnode(fornod);
			fp->fortyp = sfgetu(infile);
			fp->fortre = r_tree();
			fp->fornam = ap->argval+1;
			ap->argchn.ap = (struct argnod*)fp;
		}
		apold = ap;
	}
	if(ap)
		ap->argnxt.ap = 0;
	return aptop;
}

static struct ionod *r_redirect(void)
{
	long l;
	struct ionod *iop=0, *iopold, *ioptop=0;
	while((l=sfgetl(infile))>=0)
	{
		iop = (struct ionod*)getnode(ionod);
		if(!ioptop)
			ioptop = iop;
		else
			iopold->ionxt = iop;
		iop->iofile = l;
		if((l & IOPROCSUB) && !(l & IOLSEEK))
			iop->ioname = (char*)r_tree();	/* process substitution as file name to redirection */
		else
			iop->ioname = r_string();	/* file name, descriptor, etc. */
		if(iop->iodelim = r_string())
		{
			iop->iosize = sfgetl(infile);
			if(sh.heredocs)
				iop->iooffset = sfseek(sh.heredocs,0,SEEK_END);
			else
			{
				sh.heredocs = sftmp(512);
				iop->iooffset = 0;
			}
			sfmove(infile,sh.heredocs, iop->iosize, -1);
		}
		iopold = iop;
		if(iop->iofile&IOVNM)
			iop->iovname = r_string();
		else
			iop->iovname = 0;
		iop->iofile &= ~IOVNM;
	}
	if(iop)
		iop->ionxt = 0;
	return ioptop;
}

static void r_comarg(struct comnod *com)
{
	char *cmdname=0;
	com->comio = r_redirect();
	com->comset = r_arg();
	com->comstate = 0;
	if(com->comtyp&COMSCAN)
	{
		com->comarg.ap = r_arg();
		if(com->comarg.ap->argflag==ARG_RAW)
			cmdname = com->comarg.ap->argval;
	}
	else if(com->comarg.dp = r_comlist())
		cmdname = com->comarg.dp->dolval[ARG_SPARE];
	com->comline = sfgetu(infile);
	com->comnamq = 0;
	if(cmdname)
	{
		char *cp;
		com->comnamp = nv_search(cmdname,sh.fun_tree,0);
		if(com->comnamp && (cp =strrchr(cmdname+1,'.')))
		{
			*cp = 0;
			com->comnamp = nv_open(cmdname,sh.var_tree,NV_VARNAME|NV_NOADD|NV_NOARRAY);
			*cp = '.';
		}
	}
	else
		com->comnamp  = 0;
}

static struct dolnod *r_comlist(void)
{
	struct dolnod *dol=0;
	long l;
	char **argv;
	if((l=sfgetl(infile))>0)
	{
		dol = stkalloc(sh.stk,sizeof(struct dolnod) + sizeof(char*)*(l+ARG_SPARE));
		dol->dolnum = l;
		dol->dolbot = ARG_SPARE;
		argv = dol->dolval+ARG_SPARE;
		while(*argv++ = r_string());
	}
	return dol;
}

static struct regnod *r_switch(void)
{
	long l;
	struct regnod *reg=0,*regold,*regtop=0;
	while((l=sfgetl(infile))>=0)
	{
		reg = getnode(regnod);
		if(!regtop)
			regtop = reg;
		else
			regold->regnxt = reg;
		reg->regflag = l;
		reg->regptr = r_arg();
		reg->regcom = r_tree();
		regold = reg;
	}
	if(reg)
		reg->regnxt = 0;
	return regtop;
}

static char *r_string(void)
{
	Sfio_t *in = infile;
	unsigned long l = sfgetu(in);
	char *ptr;
	if(l == 0)
		return NULL;
	ptr = stkalloc(sh.stk,(unsigned)l);
	if(--l > 0 && sfread(in,ptr,(size_t)l) != (size_t)l)
		return NULL;
	ptr[l] = 0;
	return ptr;
}

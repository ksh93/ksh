/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2022 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 1.0                  *
*                    by AT&T Intellectual Property                     *
*                                                                      *
*                A copy of the License is available at                 *
*          http://www.eclipse.org/org/documents/epl-v10.html           *
*         (with md5 checksum b35adb5213ca9657e911e9befb180842)         *
*                                                                      *
*              Information and Software Systems Research               *
*                            AT&T Research                             *
*                           Florham Park NJ                            *
*                                                                      *
*                  David Korn <dgk@research.att.com>                   *
*                                                                      *
***********************************************************************/
/*
 * David Korn
 * AT&T Labs
 *
 * shell intermediate code reader
 *
 */

#include	"defs.h"
#include	"shnodes.h"
#include	"path.h"
#include	"io.h"
#include	<ccode.h>

static struct dolnod	*r_comlist(void);
static struct argnod	*r_arg(void);
static struct ionod	*r_redirect(void);
static struct regnod	*r_switch(void);
static Shnode_t		*r_tree(void);
static char		*r_string(void);
static void		r_comarg(struct comnod*);

static Sfio_t *infile;

#define getnode(type)   ((Shnode_t*)stkalloc(sh.stk,sizeof(struct type)))

Shnode_t *sh_trestore(Sfio_t *in)
{
	infile = in;
	return(r_tree());
}
/*
 * read in a shell tree
 */
static Shnode_t *r_tree(void)
{
	long l = sfgetl(infile); 
	register int type;
	register Shnode_t *t=0;
	if(l<0)
		return(t);
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
			Stak_t *savstak;
			struct slnod *slp;
			struct functnod *fp;
			t = getnode(functnod);
			t->funct.functloc = -1;
			t->funct.functline = sfgetu(infile);
			t->funct.functnam = r_string();
			savstak = stakcreate(STAK_SMALL);
			savstak = stakinstall(savstak, 0);
			slp = (struct slnod*)stkalloc(sh.stk,sizeof(struct slnod)+sizeof(struct functnod));
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
			slp->slptr =  stakinstall(savstak,0);
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
	return(t);
}

static struct argnod *r_arg(void)
{
	register struct argnod *ap=0, *apold, *aptop=0;
	register long l;
	Stk_t		*stkp=sh.stk;
	while((l=sfgetu(infile))>0)
	{
		ap = (struct argnod*)stkseek(stkp,(unsigned)l+ARGVAL);
		if(!aptop)
			aptop = ap;
		else
			apold->argnxt.ap = ap;
		if(--l > 0)
		{
			sfread(infile,ap->argval,(size_t)l);
			ccmaps(ap->argval, l, CC_ASCII, CC_NATIVE);
		}
		ap->argval[l] = 0;
		ap->argchn.cp = 0;
		ap->argflag = sfgetc(infile);
		ap = (struct argnod*)stkfreeze(stkp,0);
		if(*ap->argval==0 && (ap->argflag&ARG_EXP))
			ap->argchn.ap = (struct argnod*)r_tree();
		else if(*ap->argval==0 && (ap->argflag&~(ARG_APPEND|ARG_MESSAGE|ARG_QUOTED))==0)
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
	return(aptop);
}

static struct ionod *r_redirect(void)
{
	register long l;
	register struct ionod *iop=0, *iopold, *ioptop=0;
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
				iop->iooffset = sfseek(sh.heredocs,(off_t)0,SEEK_END);
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
	return(ioptop);
}

static void r_comarg(struct comnod *com)
{
	char *cmdname=0;
	com->comio = r_redirect();
	com->comset = r_arg();
	com->comstate = 0;
	if(com->comtyp&COMSCAN)
	{
		com->comarg = r_arg();
		if(com->comarg->argflag==ARG_RAW)
			cmdname = com->comarg->argval;
	}
	else if(com->comarg = (struct argnod*)r_comlist())
		cmdname = ((struct dolnod*)(com->comarg))->dolval[ARG_SPARE];
	com->comline = sfgetu(infile);
	com->comnamq = 0;
	if(cmdname)
	{
		char *cp;
		com->comnamp = (void*)nv_search(cmdname,sh.fun_tree,0);
		if(com->comnamp && (cp =strrchr(cmdname+1,'.')))
		{
			*cp = 0;
			com->comnamp =  (void*)nv_open(cmdname,sh.var_tree,NV_VARNAME|NV_NOADD|NV_NOARRAY);
			*cp = '.';
		}
	}
	else
		com->comnamp  = 0;
}

static struct dolnod *r_comlist(void)
{
	register struct dolnod *dol=0;
	register long l;
	register char **argv;
	if((l=sfgetl(infile))>0)
	{
		dol = (struct dolnod*)stkalloc(sh.stk,sizeof(struct dolnod) + sizeof(char*)*(l+ARG_SPARE));
		dol->dolnum = l;
		dol->dolbot = ARG_SPARE;
		argv = dol->dolval+ARG_SPARE;
		while(*argv++ = r_string());
	}
	return(dol);
}

static struct regnod *r_switch(void)
{
	register long l;
	struct regnod *reg=0,*regold,*regtop=0;
	while((l=sfgetl(infile))>=0)
	{
		reg = (struct regnod*)getnode(regnod);
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
	return(regtop);
}

static char *r_string(void)
{
	register Sfio_t *in = infile;
	register unsigned long l = sfgetu(in);
	register char *ptr;
	if(l == 0)
		return(NIL(char*));
	ptr = stkalloc(sh.stk,(unsigned)l);
	if(--l > 0)
	{
		if(sfread(in,ptr,(size_t)l)!=(size_t)l)
			return(NIL(char*));
		ccmaps(ptr, l, CC_ASCII, CC_NATIVE);
	}
	ptr[l] = 0;
	return(ptr);
}

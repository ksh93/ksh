/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2021 Contributors to ksh 93u+m           *
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
#pragma prototyped
/*
 *   Create and manage subshells avoiding forks when possible
 *
 *   David Korn
 *   AT&T Labs
 *
 */

#ifdef __linux__
#define _GNU_SOURCE	/* needed for O_PATH */
#endif

#include	"defs.h"
#include	<ls.h>
#include	"io.h"
#include	"fault.h"
#include	"shnodes.h"
#include	"shlex.h"
#include	"jobs.h"
#include	"variables.h"
#include	"path.h"

#ifndef PIPE_BUF
#   define PIPE_BUF	512
#endif

#ifndef O_SEARCH
#   ifdef O_PATH
#	define O_SEARCH	O_PATH
#   else
#	define O_SEARCH	O_RDONLY
#   endif
#endif

/*
 * Note that the following structure must be the same
 * size as the Dtlink_t structure
 */
struct Link
{
	struct Link	*next;
	Namval_t	*child;
	Dt_t		*dict;
	Namval_t	*node;
};

/*
 * The following structure is used for command substitution and (...)
 */
static struct subshell
{
	Shell_t		*shp;	/* shell interpreter */
	struct subshell	*prev;	/* previous subshell data */
	struct subshell	*pipe;	/* subshell where output goes to pipe on fork */
	Dt_t		*var;	/* variable table at time of subshell */
	struct Link	*svar;	/* save shell variable table */
	Dt_t		*sfun;	/* function scope for subshell */
	Dt_t		*strack;/* tracked alias scope for subshell */
	Pathcomp_t	*pathlist; /* for PATH variable */
	struct Error_context_s *errcontext;
	Shopt_t		options;/* save shell options */
	pid_t		subpid;	/* child process id */
	Sfio_t*		saveout;/* saved standard output */
	char		*pwd;	/* present working directory */
	void		*jobs;	/* save job info */
	mode_t		mask;	/* saved umask */
	short		tmpfd;	/* saved tmp file descriptor */
	short		pipefd;	/* read fd if pipe is created */
	char		jobcontrol;
	char		monitor;
	unsigned char	fdstatus;
	int		fdsaved; /* bit mask for saved file descriptors */
	int		sig;	/* signal for $$ */
	pid_t		bckpid;
	pid_t		cpid;
	int		coutpipe;
	int		cpipe;
	int		nofork;
	int		subdup;
	char		subshare;
	char		comsub;
#if _lib_fchdir
	int		pwdfd;	/* file descriptor for PWD */
	char		pwdclose;
#endif /* _lib_fchdir */
} *subshell_data;

static char subshell_noscope;	/* for temporarily disabling all virtual subshell scope creation */

static unsigned int subenv;


/*
 * This routine will turn the sftmp() file into a real /tmp file or pipe
 * if the /tmp file create fails
 */
void	sh_subtmpfile(char comsub_flag)
{
	if(sfset(sfstdout,0,0)&SF_STRING)
	{
		Shell_t *shp = sh_getinterp();
		register int fd;
		register struct checkpt	*pp = (struct checkpt*)shp->jmplist;
		register struct subshell *sp = subshell_data->pipe;
		/* save file descriptor 1 if open */
		if((sp->tmpfd = fd = sh_fcntl(1,F_DUPFD,10)) >= 0)
		{
			fcntl(fd,F_SETFD,FD_CLOEXEC);
			shp->fdstatus[fd] = shp->fdstatus[1]|IOCLEX;
			close(1);
		}
		else if(errno!=EBADF)
		{
			errormsg(SH_DICT,ERROR_system(1),e_toomany);
			UNREACHABLE();
		}
		/* popping a discipline forces a /tmp file create */
		if(comsub_flag != 1)
			sfdisc(sfstdout,SF_POPDISC);
		if((fd=sffileno(sfstdout))<0)
		{
			/* unable to create the /tmp file so use a pipe */
			int fds[3];
			Sfoff_t off;
			fds[2] = 0;
			sh_pipe(fds);
			sp->pipefd = fds[0];
			sh_fcntl(sp->pipefd,F_SETFD,FD_CLOEXEC);
			/* write the data to the pipe */
			if(off = sftell(sfstdout))
				write(fds[1],sfsetbuf(sfstdout,(Void_t*)sfstdout,0),(size_t)off);
			sfclose(sfstdout);
			if((sh_fcntl(fds[1],F_DUPFD, 1)) != 1)
			{
				errormsg(SH_DICT,ERROR_system(1),e_file+4);
				UNREACHABLE();
			}
			sh_close(fds[1]);
		}
		else
		{
			shp->fdstatus[fd] = IOREAD|IOWRITE;
			sfsync(sfstdout);
			if(fd==1)
				fcntl(1,F_SETFD,0);
			else
			{
				sfsetfd(sfstdout,1);
				shp->fdstatus[1] = shp->fdstatus[fd];
				shp->fdstatus[fd] = IOCLOSE;
			}
		}
		sh_iostream(shp,1);
		sfset(sfstdout,SF_SHARE|SF_PUBLIC,1);
		sfpool(sfstdout,shp->outpool,SF_WRITE);
		if(pp && pp->olist  && pp->olist->strm == sfstdout)
			pp->olist->strm = 0;
	}
}


/*
 * This routine creates a temp file if necessary, then forks a virtual subshell into a real subshell.
 * The parent routine longjmps back to sh_subshell()
 * The child continues possibly with its standard output replaced by temp file
 */
void sh_subfork(void)
{
	register struct subshell *sp = subshell_data;
	Shell_t	*shp = sp->shp;
	unsigned int curenv = shp->curenv;
	char comsub = shp->comsub;
	pid_t pid;
	char *trap = shp->st.trapcom[0];
	if(trap)
		trap = sh_strdup(trap);
	/* see whether inside $(...) */
	if(sp->pipe)
		sh_subtmpfile(shp->comsub);
	shp->curenv = 0;
	shp->savesig = -1;
	if(pid = sh_fork(shp,FSHOWME,NIL(int*)))
	{
		shp->curenv = curenv;
		/* this is the parent part of the fork */
		if(sp->subpid==0)
			sp->subpid = pid;
		if(trap)
			free((void*)trap);
		siglongjmp(*shp->jmplist,SH_JMPSUB);
	}
	else
	{
		/* this is the child part of the fork */
		sh_onstate(SH_FORKED);
		subshell_data = 0;
		shp->subshell = 0;
		shp->comsub = 0;
		sp->subpid=0;
		shp->st.trapcom[0] = (comsub==2 ? NIL(char*) : trap);
		shp->savesig = 0;
		/* sh_fork() increases ${.sh.subshell} but we forked an existing virtual subshell, so undo */
		shgd->realsubshell--;
	}
}

int nv_subsaved(register Namval_t *np, int flags)
{
	register struct subshell	*sp;
	register struct Link		*lp, *lpprev;
	for(sp = (struct subshell*)subshell_data; sp; sp=sp->prev)
	{
		lpprev = 0;
		for(lp=sp->svar; lp; lpprev=lp, lp=lp->next)
		{
			if(lp->node==np)
			{
				if(flags&NV_TABLE)
				{
					if(lpprev)
						lpprev->next = lp->next;
					else
						sp->svar = lp->next;
					free((void*)np);
					free((void*)lp);
				}
				return(1);
			}
		}
	}
	return(0);
}

/*
 * This routine will make a copy of the given node in the
 * layer created by the most recent virtual subshell if the
 * node hasn't already been copied.
 *
 * add == 0:    Move the node pointer from the parent shell to the current virtual subshell.
 * add == 1:    Create a copy of the node pointer in the current virtual subshell.
 * add == 2:    This will create a copy of the node pointer like 1, but it will disable the
 *              optimization for ${.sh.level}.
 */
Namval_t *sh_assignok(register Namval_t *np,int add)
{
	register Namval_t	*mp;
	register struct Link	*lp;
	struct subshell		*sp = subshell_data;
	Shell_t			*shp = sp->shp;
	Dt_t			*dp= shp->var_tree;
	Namval_t		*mpnext;
	Namarr_t		*ap;
	unsigned int		save;
	/*
	 * Don't create a scope if told not to (see nv_restore()) or if this is a subshare.
	 * Also, moving/copying ${.sh.level} (SH_LEVELNOD) may crash the shell.
	 */
	if(subshell_noscope || sh.subshare || add<2 && np==SH_LEVELNOD)
		return(np);
	if((ap=nv_arrayptr(np)) && (mp=nv_opensub(np)))
	{
		shp->last_root = ap->table;
		sh_assignok(mp,add);
		if(!add || array_assoc(ap))
			return(np);
	}
	for(lp=sp->svar; lp;lp = lp->next)
	{
		if(lp->node==np)
			return(np);
	}
	/* first two pointers use linkage from np */
	lp = (struct Link*)sh_malloc(sizeof(*np)+2*sizeof(void*));
	memset(lp,0, sizeof(*mp)+2*sizeof(void*));
	lp->node = np;
	if(!add &&  nv_isvtree(np))
	{
		Namval_t	fake;
		Dt_t		*walk, *root=shp->var_tree;
		char		*name = nv_name(np);
		size_t		len = strlen(name);
		fake.nvname = name;
		mpnext = dtnext(root,&fake);
		dp = root->walk?root->walk:root;
		while(mp=mpnext)
		{
			walk = root->walk?root->walk:root;
			mpnext = dtnext(root,mp);
			if(strncmp(name,mp->nvname,len) || mp->nvname[len]!='.')
				break;
			nv_delete(mp,walk,NV_NOFREE);
			*((Namval_t**)mp) = lp->child;
			lp->child = mp;
			
		}
	}
	lp->dict = dp;
	mp = (Namval_t*)&lp->dict;
	lp->next = subshell_data->svar; 
	subshell_data->svar = lp;
	save = shp->subshell;
	shp->subshell = 0;
	mp->nvname = np->nvname;
	if(nv_isattr(np,NV_NOFREE))
		nv_onattr(mp,NV_IDENT);
	nv_clone(np,mp,(add?(nv_isnull(np)?0:NV_NOFREE)|NV_ARRAY:NV_MOVE));
	shp->subshell = save;
	return(np);
}

/*
 * restore the variables
 */
static void nv_restore(struct subshell *sp)
{
	register struct Link *lp, *lq;
	register Namval_t *mp, *np;
	Namval_t	*mpnext;
	int		flags,nofree;
	subshell_noscope = 1;
	for(lp=sp->svar; lp; lp=lq)
	{
		np = (Namval_t*)&lp->dict;
		lq = lp->next;
		mp = lp->node;
		if(!mp->nvname)
			continue;
		flags = 0;
		if(nv_isattr(mp,NV_MINIMAL) && !nv_isattr(np,NV_EXPORT))
			flags |= NV_MINIMAL;
		if(nv_isarray(mp))
			 nv_putsub(mp,NIL(char*),ARRAY_SCAN);
		nofree = mp->nvfun?mp->nvfun->nofree:0;
		_nv_unset(mp,NV_RDONLY|NV_CLONE);
		if(nv_isarray(np))
		{
			nv_clone(np,mp,NV_MOVE);
			goto skip;
		}
		nv_setsize(mp,nv_size(np));
		if(!(flags&NV_MINIMAL))
			mp->nvenv = np->nvenv;
		mp->nvfun = np->nvfun;
		if(np->nvfun && nofree)
			np->nvfun->nofree = nofree;
		if(nv_isattr(np,NV_IDENT))
		{
			nv_offattr(np,NV_IDENT);
			flags |= NV_NOFREE;
		}
		mp->nvflag = np->nvflag|(flags&NV_MINIMAL);
		if(nv_cover(mp))
			nv_putval(mp,nv_getval(np),NV_RDONLY);
		else
			mp->nvalue = np->nvalue;
		if(nofree && np->nvfun && !np->nvfun->nofree)
			free((char*)np->nvfun);
		np->nvfun = 0;
		if(nv_isattr(mp,NV_EXPORT))
		{
			char *name = nv_name(mp);
			sh_envput(sp->shp->env,mp);
			if(*name=='_' && strcmp(name,"_AST_FEATURES")==0)
				astconf(NiL, NiL, NiL);
		}
		else if(nv_isattr(np,NV_EXPORT))
			env_delete(sp->shp->env,nv_name(mp));
		nv_onattr(mp,flags);
	skip:
		for(mp=lp->child; mp; mp=mpnext)
		{
			mpnext = *((Namval_t**)mp);
			dtinsert(lp->dict,mp);
		}
		free((void*)lp);
		sp->svar = lq;
	}
	subshell_noscope = 0;
}

/*
 * Return pointer to tracked alias tree (a.k.a. hash table, i.e. cached $PATH search results).
 * Create new one if in a subshell and one doesn't exist and 'create' is non-zero.
 */
Dt_t *sh_subtracktree(int create)
{
	register struct subshell *sp = subshell_data;
	if(create && sh.subshell && !sh.subshare)
	{
		if(sp && !sp->strack)
		{
			sp->strack = dtopen(&_Nvdisc,Dtset);
			dtuserdata(sp->strack,&sh,1);
			dtview(sp->strack,sh.track_tree);
			sh.track_tree = sp->strack;
		}
	}
	return(sh.track_tree);
}

/*
 * return pointer to function tree
 * create new one if in a subshell and one doesn't exist and create is non-zero
 */
Dt_t *sh_subfuntree(int create)
{
	register struct subshell *sp = subshell_data;
	if(create && sh.subshell && !sh.subshare)
	{
		if(sp && !sp->sfun)
		{
			sp->sfun = dtopen(&_Nvdisc,Dtoset);
			dtuserdata(sp->sfun,&sh,1);
			dtview(sp->sfun,sh.fun_tree);
			sh.fun_tree = sp->sfun;
		}
	}
	return(sh.fun_tree);
}

int sh_subsavefd(register int fd)
{
	register struct subshell *sp = subshell_data;
	register int old=0;
	if(sp)
	{
		old = !(sp->fdsaved&(1<<fd));
		sp->fdsaved |= (1<<fd);
	}
	return(old);
}

void sh_subjobcheck(pid_t pid)
{
	register struct subshell *sp = subshell_data;
	while(sp)
	{
		if(sp->cpid==pid)
		{
			sh_close(sp->coutpipe);
			sh_close(sp->cpipe);
			sp->coutpipe = sp->cpipe = -1;
			return;
		}
		sp = sp->prev;
	}
}

/*
 * Run command tree <t> in a virtual subshell
 * If comsub is not null, then output will be placed in temp file (or buffer)
 * If comsub is not null, the return value will be a stream consisting of
 * output of command <t>.  Otherwise, NULL will be returned.
 */

Sfio_t *sh_subshell(Shell_t *shp,Shnode_t *t, volatile int flags, int comsub)
{
	struct subshell sub_data;
	register struct subshell *sp = &sub_data;
	int jmpval,isig,nsig=0,fatalerror=0,saveerrno=0;
	unsigned int savecurenv = shp->curenv;
	int savejobpgid = job.curpgid;
	int *saveexitval = job.exitval;
	char **savsig;
	Sfio_t *iop=0;
	struct checkpt buff;
	struct sh_scoped savst;
	struct dolnod   *argsav=0;
	int argcnt;
	struct rand *rp;		/* current $RANDOM discipline function data */
	unsigned int save_rand_seed;	/* parent shell $RANDOM seed */
	int save_rand_last;		/* last random number from $RANDOM in parent shell */
	memset((char*)sp, 0, sizeof(*sp));
	sfsync(shp->outpool);
	sh_sigcheck(shp);
	shp->savesig = -1;
	if(argsav = sh_arguse(shp))
		argcnt = argsav->dolrefcnt;
	if(shp->curenv==0)
	{
		subshell_data=0;
		subenv = 0;
	}
	shp->curenv = ++subenv;
	savst = shp->st;
	sh_pushcontext(shp,&buff,SH_JMPSUB);
	shp->subshell++;		/* increase level of virtual subshells */
	shgd->realsubshell++;		/* increase ${.sh.subshell} */
	sp->prev = subshell_data;
	sp->shp = shp;
	sp->sig = 0;
	subshell_data = sp;
	sp->errcontext = &buff.err;
	sp->var = shp->var_tree;
	sp->options = shp->options;
	sp->jobs = job_subsave();
	sp->subdup = shp->subdup;
	shp->subdup = 0;
	/* make sure initialization has occurred */ 
	if(!shp->pathlist)
	{
		shp->pathinit = 1;
		path_get(shp,e_dot);
		shp->pathinit = 0;
	}
	if(!shp->pwd)
		path_pwd(shp,0);
	sp->bckpid = shp->bckpid;
	if(comsub)
		sh_stats(STAT_COMSUB);
	else
		job.curpgid = 0;
	sp->subshare = shp->subshare;
	sp->comsub = shp->comsub;
	shp->subshare = comsub==2;
	if(!shp->subshare)
		sp->pathlist = path_dup((Pathcomp_t*)shp->pathlist);
	if(comsub)
	{
		shp->comsub = comsub;
		job.bktick_waitall = (comsub==1);
	}
	if(!shp->subshare)
	{
		struct subshell *xp;
#if _lib_fchdir
		sp->pwdfd = -1;
		for(xp=sp->prev; xp; xp=xp->prev) 
		{
			if(xp->pwdfd>0 && xp->pwd && strcmp(xp->pwd,shp->pwd)==0)
			{
				sp->pwdfd = xp->pwdfd;
				break;
			}
		}
		if(sp->pwdfd<0)
		{
			int n = open(e_dot,O_SEARCH);
			if(n>=0)
			{
				sp->pwdfd = n;
				if(n<10)
				{
					sp->pwdfd = sh_fcntl(n,F_DUPFD,10);
					close(n);
				}
				if(sp->pwdfd>0)
				{
					fcntl(sp->pwdfd,F_SETFD,FD_CLOEXEC);
					sp->pwdclose = 1;
				}
			}
		}
#endif /* _lib_fchdir */
		sp->pwd = (shp->pwd?sh_strdup(shp->pwd):0);
		sp->mask = shp->mask;
		sh_stats(STAT_SUBSHELL);
		/* save trap table */
		shp->st.otrapcom = 0;
		shp->st.otrap = savst.trap;
		if((nsig=shp->st.trapmax)>0 || shp->st.trapcom[0])
		{
			savsig = sh_malloc(nsig * sizeof(char*));
			/*
			 * the data is, usually, modified in code like:
			 *	tmp = buf[i]; buf[i] = sh_strdup(tmp); free(tmp);
			 * so shp->st.trapcom needs a "deep copy" to properly save/restore pointers.
			 */
			for (isig = 0; isig < nsig; ++isig)
			{
				if(shp->st.trapcom[isig] == Empty)
					savsig[isig] = Empty;
				else if(shp->st.trapcom[isig])
					savsig[isig] = sh_strdup(shp->st.trapcom[isig]);
				else
					savsig[isig] = NIL(char*);
			}
			/* this is needed for var=$(trap) */
			shp->st.otrapcom = (char**)savsig;
		}
		sp->cpid = shp->cpid;
		sp->coutpipe = shp->coutpipe;
		sp->cpipe = shp->cpipe[1];
		shp->cpid = 0;
		sh_sigreset(0);
		/* save the current $RANDOM seed and state; reseed $RANDOM */
		rp = (struct rand*)RANDNOD->nvfun;
		save_rand_seed = rp->rand_seed;
		save_rand_last = rp->rand_last;
		sh_reseed_rand(rp);
	}
	jmpval = sigsetjmp(buff.buff,0);
	if(jmpval==0)
	{
		if(comsub)
		{
			/* disable job control */
			shp->spid = 0;
			sp->jobcontrol = job.jobcontrol;
			sp->monitor = (sh_isstate(SH_MONITOR)!=0);
			job.jobcontrol=0;
			sh_offstate(SH_MONITOR);
			sp->pipe = sp;
			/* save sfstdout and status */
			sp->saveout = sfswap(sfstdout,NIL(Sfio_t*));
			sp->fdstatus = shp->fdstatus[1];
			sp->tmpfd = -1;
			sp->pipefd = -1;
			/* use sftmp() file for standard output */
			if(!(iop = sftmp(PIPE_BUF)))
			{
				sfswap(sp->saveout,sfstdout);
				errormsg(SH_DICT,ERROR_system(1),e_tmpcreate);
				UNREACHABLE();
			}
			sfswap(iop,sfstdout);
			sfset(sfstdout,SF_READ,0);
			shp->fdstatus[1] = IOWRITE;
			if(!(sp->nofork = sh_state(SH_NOFORK)))
				sh_onstate(SH_NOFORK);
			flags |= sh_state(SH_NOFORK);
		}
		else if(sp->prev)
		{
			sp->pipe = sp->prev->pipe;
			flags &= ~sh_state(SH_NOFORK);
		}
		if(shp->savesig < 0)
		{
			shp->savesig = 0;
#if _lib_fchdir
			if(sp->pwdfd < 0 && !shp->subshare)	/* if we couldn't get a file descriptor to our PWD ... */
				sh_subfork();			/* ...we have to fork, as we cannot fchdir back to it. */
#else
			if(!shp->subshare)
			{
				if(sp->pwd && access(sp->pwd,X_OK)<0)
				{
					free(sp->pwd);
					sp->pwd = NIL(char*);
				}
				if(!sp->pwd)
					sh_subfork();
			}
#endif /* _lib_fchdir */
			sh_offstate(SH_INTERACTIVE);
			sh_exec(t,flags);
		}
	}
	if(comsub!=2 && jmpval!=SH_JMPSUB && shp->st.trapcom[0] && shp->subshell)
	{
		/* trap on EXIT not handled by child */
		char *trap=shp->st.trapcom[0];
		shp->st.trapcom[0] = 0;	/* prevent recursion */
		sh_trap(trap,0);
		free(trap);
	}
	sh_popcontext(shp,&buff);
	if(shp->subshell==0)	/* must be child process */
	{
		subshell_data = sp->prev;
		if(jmpval==SH_JMPSCRIPT)
			siglongjmp(*shp->jmplist,jmpval);
		shp->exitval &= SH_EXITMASK;
		sh_done(shp,0);
	}
	if(!shp->savesig)
		shp->savesig = -1;
	nv_restore(sp);
	if(comsub)
	{
		/* re-enable job control */
		if(!sp->nofork)
			sh_offstate(SH_NOFORK);
		job.jobcontrol = sp->jobcontrol;
		if(sp->monitor)
			sh_onstate(SH_MONITOR);
		if(sp->pipefd>=0)
		{
			/* sftmp() file has been returned into pipe */
			iop = sh_iostream(shp,sp->pipefd);
			sfclose(sfstdout);
		}
		else
		{
			job.bktick_waitall = 0;
			if(comsub!=1 && shp->spid)
			{
				int e = shp->exitval;
				job_wait(shp->spid);
				shp->exitval = e;
				if(shp->pipepid==shp->spid)
					shp->spid = 0;
				shp->pipepid = 0;
			}
			/* move tmp file to iop and restore sfstdout */
			iop = sfswap(sfstdout,NIL(Sfio_t*));
			if(!iop)
			{
				/* maybe locked try again */
				sfclrlock(sfstdout);
				iop = sfswap(sfstdout,NIL(Sfio_t*));
			}
			if(iop && sffileno(iop)==1)
			{
				int fd = sfsetfd(iop,sh_iosafefd(shp,3));
				if(fd<0)
				{
					shp->toomany = 1;
					((struct checkpt*)shp->jmplist)->mode = SH_JMPERREXIT;
					errormsg(SH_DICT,ERROR_system(1),e_toomany);
					UNREACHABLE();
				}
				if(fd >= shp->gd->lim.open_max)
					sh_iovalidfd(shp,fd);
				shp->sftable[fd] = iop;
				fcntl(fd,F_SETFD,FD_CLOEXEC);
				shp->fdstatus[fd] = (shp->fdstatus[1]|IOCLEX);
				shp->fdstatus[1] = IOCLOSE;
			}
			sfset(iop,SF_READ,1);
		}
		if(sp->saveout)
			sfswap(sp->saveout,sfstdout);
		else
			sfstdout = &_Sfstdout;
		/* check if standard output was preserved */
		if(sp->tmpfd>=0)
		{
			int err=errno;
			while(close(1)<0 && errno==EINTR)
				errno = err;
			if (fcntl(sp->tmpfd,F_DUPFD,1) != 1)
			{
				saveerrno = errno;
				fatalerror = 1;
			}
			sh_close(sp->tmpfd);
		}
		shp->fdstatus[1] = sp->fdstatus;
	}
	if(!shp->subshare)
	{
		path_delete((Pathcomp_t*)shp->pathlist);
		shp->pathlist = (void*)sp->pathlist;
	}
	job_subrestore(sp->jobs);
	shp->curenv = shp->jobenv = savecurenv;
	job.curpgid = savejobpgid;
	job.exitval = saveexitval;
	shp->bckpid = sp->bckpid;
	if(!shp->subshare)	/* restore environment if saved */
	{
		int n;
		shp->options = sp->options;
		/* Clean up subshell hash table. */
		if(sp->strack)
		{
			Namval_t *np, *next_np;
			/* Detach this scope from the unified view. */
			shp->track_tree = dtview(sp->strack,0);
			/* Free all elements of the subshell hash table. */
			for(np = (Namval_t*)dtfirst(sp->strack); np; np = next_np)
			{
				next_np = (Namval_t*)dtnext(sp->strack,np);
				nv_delete(np,sp->strack,0);
			}
			/* Close and free the table itself. */
			dtclose(sp->strack);
		}
		/* Clean up subshell function table. */
		if(sp->sfun)
		{
			Namval_t *np, *next_np;
			/* Detach this scope from the unified view. */
			shp->fun_tree = dtview(sp->sfun,0);
			/* Free all elements of the subshell function table. */
			for(np = (Namval_t*)dtfirst(sp->sfun); np; np = next_np)
			{
				next_np = (Namval_t*)dtnext(sp->sfun,np);
				if(!np->nvalue.rp)
				{
					/* Dummy node created by unall() to mask parent shell function. */
					nv_delete(np,sp->sfun,0);
					continue;
				}
				nv_onattr(np,NV_FUNCTION);  /* in case invalidated by unall() */
				if(np->nvalue.rp->fname && shp->fpathdict && nv_search(np->nvalue.rp->fname,shp->fpathdict,0))
				{
					/* Autoloaded function. It must not be freed. */
					np->nvalue.rp->fdict = 0;
					nv_delete(np,sp->sfun,NV_FUNCTION|NV_NOFREE);
				}
				else
				{
					_nv_unset(np,NV_RDONLY);
					nv_delete(np,sp->sfun,NV_FUNCTION);
				}
			}
			dtclose(sp->sfun);
		}
		n = shp->st.trapmax-savst.trapmax;
		sh_sigreset(1);
		if(n>0)
			memset(&shp->st.trapcom[savst.trapmax],0,n*sizeof(char*));
		shp->st = savst;
		shp->st.otrap = 0;
		if(nsig)
		{
			for (isig = 0; isig < nsig; ++isig)
				if (shp->st.trapcom[isig] && shp->st.trapcom[isig]!=Empty)
					free(shp->st.trapcom[isig]);
			memcpy((char*)&shp->st.trapcom[0],savsig,nsig*sizeof(char*));
			free((void*)savsig);
		}
		shp->options = sp->options;
		/* restore the present working directory */
#if _lib_fchdir
		if(sp->pwdfd > 0 && fchdir(sp->pwdfd) < 0)
#else
		if(sp->pwd && strcmp(sp->pwd,shp->pwd) && chdir(sp->pwd) < 0)
#endif /* _lib_fchdir */
		{
			saveerrno = errno;
			fatalerror = 2;
		}
		else if(sp->pwd && strcmp(sp->pwd,shp->pwd))
			path_newdir(shp,shp->pathlist);
		if(shp->pwd)
			free((void*)shp->pwd);
		shp->pwd = sp->pwd;
#if _lib_fchdir
		if(sp->pwdclose)
			close(sp->pwdfd);
#endif /* _lib_fchdir */
		if(sp->mask!=shp->mask)
			umask(shp->mask=sp->mask);
		if(shp->coutpipe!=sp->coutpipe)
		{
			sh_close(shp->coutpipe);
			sh_close(shp->cpipe[1]);
		}
		shp->cpid = sp->cpid;
		shp->cpipe[1] = sp->cpipe;
		shp->coutpipe = sp->coutpipe;
		/* restore $RANDOM seed and state */
		rp = (struct rand*)RANDNOD->nvfun;
		srand(rp->rand_seed = save_rand_seed);
		rp->rand_last = save_rand_last;
	}
	shp->subshare = sp->subshare;
	shp->subdup = sp->subdup;
	if(shp->subshell)
	{
		shp->subshell--;		/* decrease level of virtual subshells */
		shgd->realsubshell--;		/* decrease ${.sh.subshell} */
	}
	subshell_data = sp->prev;
	if(!argsav  ||  argsav->dolrefcnt==argcnt)
		sh_argfree(shp,argsav,0);
	if(shp->topfd != buff.topfd)
		sh_iorestore(shp,buff.topfd|IOSUBSHELL,jmpval);
	if(sp->sig)
	{
		if(sp->prev)
			sp->prev->sig = sp->sig;
		else
		{
			kill(shgd->current_pid,sp->sig);
			sh_chktrap(shp);
		}
	}
	sh_sigcheck(shp);
	shp->trapnote = 0;
	nsig = shp->savesig;
	shp->savesig = 0;
	if(nsig>0)
		kill(shgd->current_pid,nsig);
	if(sp->subpid)
	{
		job_wait(sp->subpid);
		if(comsub>1)
			sh_iounpipe(shp);
	}
	shp->comsub = sp->comsub;
	if(comsub && iop && sp->pipefd<0)
		sfseek(iop,(off_t)0,SEEK_SET);
	if(shp->trapnote)
		sh_chktrap(shp);
	if(shp->exitval > SH_EXITSIG)
	{
		int sig = shp->exitval&SH_EXITMASK;
		if(sig==SIGINT || sig== SIGQUIT)
			kill(shgd->current_pid,sig);
	}
	if(fatalerror)
	{
		((struct checkpt*)shp->jmplist)->mode = SH_JMPERREXIT;
		switch(fatalerror)
		{
			case 1:
				shp->toomany = 1;
				errno = saveerrno;
				errormsg(SH_DICT,ERROR_SYSTEM|ERROR_PANIC,e_redirect);
				UNREACHABLE();
			case 2:
				/* reinit PWD as it will be wrong */
				if(shp->pwd)
					free((void*)shp->pwd);
				shp->pwd = NIL(const char*);
				path_pwd(shp,0);
				errno = saveerrno;
				errormsg(SH_DICT,ERROR_SYSTEM|ERROR_PANIC,"Failed to restore PWD upon exiting subshell");
				UNREACHABLE();
			default:
				errormsg(SH_DICT,ERROR_SYSTEM|ERROR_PANIC,"Subshell error %d",fatalerror);
				UNREACHABLE();
		}
	}
	if(shp->ignsig)
		kill(shgd->current_pid,shp->ignsig);
	if(jmpval==SH_JMPSUB && shp->lastsig)
		kill(shgd->current_pid,shp->lastsig);
	if(jmpval && shp->toomany)
		siglongjmp(*shp->jmplist,jmpval);
	return(iop);
}

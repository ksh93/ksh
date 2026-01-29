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
*             dnewhall <dnewhall@users.noreply.github.com>             *
*                                                                      *
***********************************************************************/
/*
 *   Create and manage subshells avoiding forks when possible
 *
 *   David Korn
 *   AT&T Labs
 *
 */

#include	"shopt.h"
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
	struct subshell	*prev;	/* previous subshell data */
	struct subshell	*pipe;	/* subshell where output goes to pipe on fork */
	struct Link	*svar;	/* save shell variable table */
	Dt_t		*sfun;	/* function scope for subshell */
	Dt_t		*strack;/* tracked alias scope for subshell */
	Pathcomp_t	*pathlist; /* for PATH variable */
	Shopt_t		options;/* save shell options */
	pid_t		subpid;	/* child process ID */
	Sfio_t*		saveout;/* saved standard output */
	char		*pwd;	/* present working directory */
	void		*jobs;	/* save job info */
	mode_t		mask;	/* saved umask */
	int		tmpfd;	/* saved tmp file descriptor */
	int		pipefd;	/* read fd if pipe is created */
	char		jobcontrol;
	unsigned char	fdstatus;
	int		fdsaved; /* bit mask for saved file descriptors */
	int		sig;	/* signal for $$ */
	pid_t		bckpid;
	pid_t		cpid;
	int		coutpipe;
	int		cpipe;
	char		subshare;
	char		comsub;
	unsigned short	rand_seed[3];       /* parent shell $RANDOM seed */
	int		rand_last;          /* last random number from $RANDOM in parent shell */
	char		rand_state;         /* 0 means sp->rand_seed hasn't been set, 1 is the opposite */
	uint32_t	srand_upper_bound;  /* parent shell's upper bound for $SRANDOM */
	int		pwdfd;              /* parent shell's file descriptor for PWD */
#if !_lib_openat
	char		pwdclose;
#endif /* !_lib_openat */
} *subshell_data;

static unsigned int subenv;


/*
 * This routine will turn the sftmp() file into a real temporary file
 */
void	sh_subtmpfile(void)
{
	if(sfset(sfstdout,0,0)&SFIO_STRING)
	{
		int fd;
		struct checkpt	*pp = (struct checkpt*)sh.jmplist;
		struct subshell *sp = subshell_data->pipe;
		/* save file descriptor 1 if open */
		if((sp->tmpfd = fd = sh_fcntl(1,F_dupfd_cloexec,10)) >= 0)
		{
			if(F_dupfd_cloexec == F_DUPFD)
				sh_fcntl(fd,F_SETFD,FD_CLOEXEC);
			close(1);
		}
		else if(errno!=EBADF)
		{
			errormsg(SH_DICT,ERROR_system(1),e_toomany);
			UNREACHABLE();
		}
		/* popping a discipline forces a /tmp file create */
		sfdisc(sfstdout,SFIO_POPDISC);
		if((fd=sffileno(sfstdout))<0)
		{
			errormsg(SH_DICT,ERROR_SYSTEM|ERROR_PANIC,"could not create temp file");
			UNREACHABLE();
		}
		sh.fdstatus[fd] = IOREAD|IOWRITE;
		sfsync(sfstdout);
		if(fd==1)
			fcntl(1,F_SETFD,0);
		else
		{
			sfsetfd(sfstdout,1);
			sh.fdstatus[1] = sh.fdstatus[fd];
			sh.fdstatus[fd] = IOCLOSE;
		}
		sh_iostream(1);
		sfset(sfstdout,SFIO_SHARE|SFIO_PUBLIC,1);
		sfpool(sfstdout,sh.outpool,SFIO_WRITE);
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
	struct subshell *sp = subshell_data;
	unsigned int curenv = sh.curenv;
	char comsub = sh.comsub;
	pid_t pid;
	char *trap = sh.st.trapcom[0];
	if(trap)
		trap = sh_strdup(trap);
	/* see whether inside $(...) */
	if(sp->pipe)
		sh_subtmpfile();
	sh.curenv = 0;
	sh.savesig = -1;
	if(pid = sh_fork(FSHOWME,NULL))
	{
		sh.curenv = curenv;
		/* this is the parent part of the fork */
		if(sp->subpid==0)
			sp->subpid = pid;
		if(trap)
			free(trap);
		siglongjmp(*sh.jmplist,SH_JMPSUB);
	}
	else
	{
		/* this is the child part of the fork */
		/*
		 * In a virtual subshell, $RANDOM is not reseeded until it's used, so if that
		 * hasn't happened yet, invalidate the seed. This allows sh_save_rand_seed()
		 * to reseed $RANDOM later.
		 */
		if(!sp->rand_state)
			sh_invalidate_rand_seed();
		subshell_data = 0;
		sh.subshell = 0;
		sh.comsub = 0;
		sp->subpid=0;
		sh.st.trapcom[0] = (comsub==2 ? NULL : trap);
		sh.savesig = 0;
		/* sh_fork() increases ${.sh.subshell} but we forked an existing virtual subshell, so undo */
		sh.realsubshell--;
	}
}

int nv_subsaved(Namval_t *np, int flags)
{
	struct subshell	*sp;
	struct Link		*lp, *lpprev;
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
					free(np);
					free(lp);
				}
				return 1;
			}
		}
	}
	return 0;
}

/*
 * Save the current $RANDOM seed and state, then reseed $RANDOM.
 */
void sh_save_rand_seed(struct rand *rp, int reseed)
{
	struct subshell	*sp = subshell_data;
	if(!sh.subshare && sp && !sp->rand_state)
	{
		memcpy(sp->rand_seed, rp->rand_seed, sizeof sp->rand_seed);
		sp->rand_last = rp->rand_last;
		sp->rand_state = 1;
		if(reseed)
			sh_reseed_rand(rp);
	}
	else if(reseed && rp->rand_last == RAND_SEED_INVALIDATED)
		sh_reseed_rand(rp);
}

/*
 * This routine will make a copy of the given node in the
 * layer created by the most recent virtual subshell if the
 * node hasn't already been copied.
 *
 * add == 0:    Move the node pointer from the parent shell to the current virtual subshell.
 * add == 1:    Create a copy of the node pointer in the current virtual subshell.
 */
void sh_assignok(Namval_t *np,int add)
{
	Namval_t		*mp;
	struct Link		*lp;
	struct subshell		*sp = subshell_data;
	Dt_t			*dp= sh.var_tree;
	Namval_t		*mpnext;
	Namarr_t		*ap;
	unsigned int		save;
	/*
	 * Don't create a scope during virtual subshell cleanup (see nv_restore()) or if this is a subshare.
	 * Also, ${.sh.level} (SH_LEVELNOD) is handled specially and is not scoped in virtual subshells.
	 */
	if(sh.nv_restore || sh.subshare || np==SH_LEVELNOD)
		return;
	if((ap=nv_arrayptr(np)) && (mp=nv_opensub(np)))
	{
		sh.last_root = ap->table;
		sh_assignok(mp,add);
		if(!add || array_assoc(ap))
			return;
	}
	for(lp=sp->svar; lp;lp = lp->next)
	{
		if(lp->node==np)
			return;
	}
	/* first two pointers use linkage from np */
	lp = (struct Link*)sh_malloc(sizeof(*np)+2*sizeof(void*));
	memset(lp,0, sizeof(*mp)+2*sizeof(void*));
	lp->node = np;
	if(!add &&  nv_isvtree(np))
	{
		Namval_t	fake;
		Dt_t		*walk, *root=sh.var_tree;
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
	save = sh.subshell;
	sh.subshell = 0;
	mp->nvname = np->nvname;
	/* Copy value pointers for variables whose values are pointers into the static scope, sh.st */
	if((char*)np->nvalue >= (char*)&sh.st && (char*)np->nvalue < (char*)&sh.st + sizeof(struct sh_scoped))
		mp->nvalue = np->nvalue;
	if(nv_isattr(np,NV_NOFREE))
		nv_onattr(mp,NV_IDENT);
	nv_clone(np,mp,(add?(nv_isnull(np)?0:NV_NOFREE)|NV_ARRAY:NV_MOVE));
	sh.subshell = save;
}

/*
 * restore the variables
 */
static void nv_restore(struct subshell *sp)
{
	struct Link	*lp, *lq;
	Namval_t	*mp, *np;
	Namval_t	*mpnext;
	int		flags,nofree;
	sh.nv_restore = 1;
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
			 nv_putsub(mp,NULL,ARRAY_SCAN);
		nofree = mp->nvfun?mp->nvfun->nofree:0;
		if(np->nvalue==Empty)
		{
			if(nv_isnull(mp) && !nv_isvtree(np))
			{
				free(mp);
				goto skip;
			}
			if(mp->nvalue && mp->nvalue!=Empty)
				nv_offattr(mp,NV_NOFREE);
		}
		nv_unset(mp,NV_RDONLY|NV_CLONE);
		if(nv_isarray(np))
		{
			nv_clone(np,mp,NV_MOVE);
			goto skip;
		}
		nv_setsize(mp,nv_size(np));
		if(!(flags&NV_MINIMAL))
			mp->nvmeta = np->nvmeta;
		mp->nvfun = np->nvfun;
		if(np->nvfun && nofree)
			np->nvfun->nofree = nofree;
		if(nv_isattr(np,NV_IDENT))
		{
			nv_offattr(np,NV_IDENT);
			flags |= NV_NOFREE;
		}
		mp->nvflag = np->nvflag|(flags&NV_MINIMAL);
		if(nv_enforcedisc(mp))
			nv_putval(mp,nv_getval(np),NV_RDONLY);
		else
			mp->nvalue = np->nvalue;
		if(nofree && np->nvfun && !np->nvfun->nofree)
			free(np->nvfun);
		np->nvfun = 0;
		if(nv_isattr(mp,NV_EXPORT))
		{
			char *name = nv_name(mp);
			env_change();
			if(*name=='_' && strcmp(name,"_AST_FEATURES")==0)
				astconf(NULL, NULL, NULL);
		}
		else if(nv_isattr(np,NV_EXPORT))
			env_change();
		nv_onattr(mp,flags);
	skip:
		for(mp=lp->child; mp; mp=mpnext)
		{
			mpnext = *((Namval_t**)mp);
			dtinsert(lp->dict,mp);
		}
		free(lp);
		sp->svar = lq;
	}
	sh.nv_restore = 0;
}

/*
 * Return pointer to tracked alias tree (a.k.a. hash table, i.e. cached $PATH search results).
 * Create new one if in a subshell and one doesn't exist and 'create' is non-zero.
 */
Dt_t *sh_subtracktree(int create)
{
	struct subshell *sp = subshell_data;
	if(create && sh.subshell && !sh.subshare)
	{
		if(sp && !sp->strack)
		{
			sp->strack = dtopen(&_Nvdisc,Dtset);
			dtview(sp->strack,sh.track_tree);
			sh.track_tree = sp->strack;
		}
	}
	return sh.track_tree;
}

/*
 * return pointer to function tree
 * create new one if in a subshell and one doesn't exist and create is non-zero
 */
Dt_t *sh_subfuntree(int create)
{
	struct subshell *sp = subshell_data;
	if(create && sh.subshell && !sh.subshare)
	{
		if(sp && !sp->sfun)
		{
			sp->sfun = dtopen(&_Nvdisc,Dtoset);
			dtview(sp->sfun,sh.fun_tree);
			sh.fun_tree = sp->sfun;
		}
	}
	return sh.fun_tree;
}

int sh_subsavefd(int fd)
{
	struct subshell *sp = subshell_data;
	int old=0;
	if(sh.subshell && !sh.subshare)
	{
		old = !(sp->fdsaved&(1<<fd));
		sp->fdsaved |= (1<<fd);
	}
	return old;
}

void sh_subjobcheck(pid_t pid)
{
	struct subshell *sp = subshell_data;
	while(sp)
	{
		if(sp->cpid==pid)
		{
			if(sp->coutpipe > -1)
				sh_close(sp->coutpipe);
			if(sp->cpipe > -1)
				sh_close(sp->cpipe);
			sp->coutpipe = sp->cpipe = -1;
			return;
		}
		sp = sp->prev;
	}
}

#if _lib_openat

/*
 * Set the file descriptor for the current shell's PWD without wiping
 * out the stored file descriptor for the parent shell's PWD.
 */
void sh_pwdupdate(int fd)
{
	struct subshell *sp = subshell_data;
	if(!(sh.subshell && !sh.subshare && sh.pwdfd == sp->pwdfd) && sh.pwdfd > 0)
		sh_close(sh.pwdfd);
	sh.pwdfd = fd;
}

/*
 * Check if the parent shell has a valid PWD fd to return to.
 * Only for use by cd inside of virtual subshells.
 */
int sh_validate_subpwdfd(void)
{
	struct subshell *sp = subshell_data;
	return sp->pwdfd > 0;
}

#endif /* _lib_openat */

/*
 * Run command tree <t> in a virtual subshell
 * If comsub is not null, then output will be placed in temp file (or buffer)
 * If comsub is not null, the return value will be a stream consisting of
 * output of command <t>.  Otherwise, NULL will be returned.
 */
Sfio_t *sh_subshell(Shnode_t *t, volatile int flags, int comsub)
{
	struct subshell sub_data;
	struct subshell *sp = &sub_data;
	int jmpval,isig,nsig=0,fatalerror=0,saveerrno=0;
	unsigned int savecurenv = sh.curenv;
	int savejobpgid = job.curpgid;
	int *saveexitval = job.exitval;
	char **savsig;
	Sfio_t *iop=0;
	struct checkpt checkpoint;
	struct sh_scoped savst;
	struct dolnod   *argsav=0;
	int argcnt;
	memset((char*)sp, 0, sizeof(*sp));
	sfsync(sh.outpool);
	sh_sigcheck();
	sh.savesig = -1;
	if(argsav = sh_arguse())
		argcnt = argsav->dolrefcnt;
	if(sh.curenv==0)
	{
		subshell_data=0;
		subenv = 0;
	}
	sh.curenv = ++subenv;
	savst = sh.st;
	sh_pushcontext(&checkpoint,SH_JMPSUB);
	sh.subshell++;		/* increase level of virtual subshells */
	sh.realsubshell++;	/* increase ${.sh.subshell} */
	sp->prev = subshell_data;
	sp->sig = 0;
	subshell_data = sp;
	sp->options = sh.options;
	sp->jobs = job_subsave();
	/* make sure initialization has occurred */
	if(!sh.pathlist)
	{
		sh.pathinit = 1;
		path_get(e_dot);
		sh.pathinit = 0;
	}
#if !_lib_openat
	if(!sh.pwd)
		path_pwd();
#endif /* !_lib_openat */
	sp->bckpid = sh.bckpid;
	if(comsub)
		sh_stats(STAT_COMSUB);
	else
		job.curpgid = 0;
	sp->subshare = sh.subshare;
	sp->comsub = sh.comsub;
	sh.subshare = comsub==2;
	if(!sh.subshare)
		sp->pathlist = path_dup((Pathcomp_t*)sh.pathlist);
	if(comsub)
		sh.comsub = comsub;
	if(!sh.subshare)
	{
		char *save_debugtrap = 0;
#if _lib_openat
		sp->pwd = sh_strdup(sh.pwd);
		sp->pwdfd = sh.pwdfd;
#else
		struct subshell *xp;
		sp->pwdfd = -1;
		for(xp=sp->prev; xp; xp=xp->prev)
		{
			if(xp->pwdfd>0 && xp->pwd && strcmp(xp->pwd,sh.pwd)==0)
			{
				sp->pwdfd = xp->pwdfd;
				break;
			}
		}
		if(sp->pwdfd<0)
		{
			int n = sh_open(e_dot,O_SEARCH|O_cloexec);
			if(n>=0)
			{
				sp->pwdfd = n;
				if(n<10)
				{
					sp->pwdfd = sh_fcntl(n,F_dupfd_cloexec,10);
					sh_close(n);
				}
				if(sp->pwdfd>0)
				{
					if(!(sh.fdstatus[sp->pwdfd]&IOCLEX))
						sh_fcntl(sp->pwdfd,F_SETFD,FD_CLOEXEC);
					sp->pwdclose = 1;
				}
			}
		}
		sp->pwd = (sh.pwd?sh_strdup(sh.pwd):0);
#endif /* _lib_openat */
		sp->mask = sh.mask;
		sh_stats(STAT_SUBSHELL);
		/* save trap table */
		sh.st.otrapcom = 0;
		sh.st.otrap = savst.trap;
		if((nsig=sh.st.trapmax)>0 || sh.st.trapcom[0])
		{
			savsig = sh_malloc(nsig * sizeof(char*));
			/*
			 * the data is, usually, modified in code like:
			 *	tmp = buf[i]; buf[i] = sh_strdup(tmp); free(tmp);
			 * so sh.st.trapcom needs a "deep copy" to properly save/restore pointers.
			 */
			for (isig = 0; isig < nsig; ++isig)
			{
				if(sh.st.trapcom[isig] == Empty)
					savsig[isig] = Empty;
				else if(sh.st.trapcom[isig])
					savsig[isig] = sh_strdup(sh.st.trapcom[isig]);
				else
					savsig[isig] = NULL;
			}
			/* this is needed for var=$(trap) */
			sh.st.otrapcom = (char**)savsig;
		}
		sp->cpid = sh.cpid;
		sp->coutpipe = sh.coutpipe;
		sp->cpipe = sh.cpipe[1];
		sh.cpid = 0;
		if(sh_isoption(SH_FUNCTRACE) && sh.st.trap[SH_DEBUGTRAP] && *sh.st.trap[SH_DEBUGTRAP])
			save_debugtrap = sh_strdup(sh.st.trap[SH_DEBUGTRAP]);
		sh_sigreset(0);
		if(save_debugtrap)
			sh.st.trap[SH_DEBUGTRAP] = save_debugtrap;
		/* save upper bound for $SRANDOM */
		sp->srand_upper_bound = sh.srand_upper_bound;
	}
	jmpval = sigsetjmp(checkpoint.buff,0);
	if(jmpval==0)
	{
		if(comsub)
		{
			/* disable job control */
			sh.spid = 0;
			sp->jobcontrol = job.jobcontrol;
			job.jobcontrol=0;
			sh_offstate(SH_MONITOR);
			sp->pipe = sp;
			/* save sfstdout and status */
			sp->saveout = sfswap(sfstdout,NULL);
			sp->fdstatus = sh.fdstatus[1];
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
			sfset(sfstdout,SFIO_READ,0);
			sh.fdstatus[1] = IOWRITE;
			flags |= sh_state(SH_NOFORK);
		}
		else if(sp->prev)
		{
			sp->pipe = sp->prev->pipe;
			flags &= ~sh_state(SH_NOFORK);
		}
		if(sh.savesig < 0)
		{
			sh.savesig = 0;
#if !_lib_openat
			if(sp->pwdfd < 0 && !sh.subshare)	/* if we couldn't get a file descriptor to our PWD ... */
				sh_subfork();			/* ...we have to fork, as we cannot fchdir back to it. */
#endif /* !_lib_openat */
			/* Virtual subshells are not safe to suspend (^Z, SIGTSTP) in the interactive main shell. */
			if(sh_isstate(SH_INTERACTIVE))
			{
				sh_offstate(SH_INTERACTIVE);
				sh_offstate(SH_TTYWAIT);
				if(comsub)
					sigblock(SIGTSTP);
				else
					sh_subfork();
			}
			sh_exec(t,flags);
		}
	}
	if(comsub!=2 && jmpval!=SH_JMPSUB && sh.st.trapcom[0] && sh.subshell)
	{
		/* trap on EXIT not handled by child */
		char *trap=sh.st.trapcom[0];
		sh.st.trapcom[0] = 0;	/* prevent recursion */
		sh_trap(trap,0);
		free(trap);
	}
	if(sh.subshell==0)	/* we must have forked with sh_subfork(); this is the child process */
	{
		subshell_data = sp->prev;
		sh_popcontext(&checkpoint);
		if(jmpval==SH_JMPSCRIPT)
			siglongjmp(*sh.jmplist,jmpval);
		sh.exitval &= SH_EXITMASK;
		if(sh.chldexitsig)
			sh.exitval |= SH_EXITSIG;
		sh_done(0);
	}
	if(!sh.savesig)
		sh.savesig = -1;
	nv_restore(sp);
	if(comsub)
	{
		if(savst.states & sh_state(SH_INTERACTIVE))
			sigrelease(SIGTSTP);
		/* re-enable job control */
		job.jobcontrol = sp->jobcontrol;
		if(savst.states & sh_state(SH_MONITOR))
			sh_onstate(SH_MONITOR);
		if(sp->pipefd>=0)
		{
			/* sftmp() file has been returned into pipe */
			iop = sh_iostream(sp->pipefd);
			sfclose(sfstdout);
		}
		else
		{
			if(sh.spid)
			{
				int e = sh.exitval, c = sh.chldexitsig;
				job_wait(sh.spid);
				sh.exitval = e, sh.chldexitsig = c;
				if(sh.pipepid==sh.spid)
					sh.spid = 0;
				sh.pipepid = 0;
			}
			/* move tmp file to iop and restore sfstdout */
			iop = sfswap(sfstdout,NULL);
			if(!iop)
			{
				/* maybe locked try again */
				sfclrlock(sfstdout);
				iop = sfswap(sfstdout,NULL);
			}
			if(iop && sffileno(iop)==1)
			{
				int fd = sfsetfd_cloexec(iop,sh_iosafefd(3));
				if(fd<0)
				{
					sh.toomany = 1;
					((struct checkpt*)sh.jmplist)->mode = SH_JMPERREXIT;
					errormsg(SH_DICT,ERROR_system(1),e_toomany);
					UNREACHABLE();
				}
				if(fd >= sh.lim.open_max)
					sh_iovalidfd(fd);
				sh.sftable[fd] = iop;
				sh.fdstatus[fd] = (sh.fdstatus[1]|IOCLEX);
				sh.fdstatus[1] = IOCLOSE;
			}
			sfset(iop,SFIO_READ,1);
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
		sh.fdstatus[1] = sp->fdstatus;
	}
	if(!sh.subshare)
	{
		path_delete((Pathcomp_t*)sh.pathlist);
		sh.pathlist = sp->pathlist;
	}
	job_subrestore(sp->jobs);
	sh.curenv = sh.jobenv = savecurenv;
	job.curpgid = savejobpgid;
	job.exitval = saveexitval;
	sh.bckpid = sp->bckpid;
	if(!sh.subshare)	/* restore environment if saved */
	{
		int n;
		struct rand *rp;
		sh.options = sp->options;
		/* Clean up subshell hash table. */
		if(sp->strack)
		{
			Namval_t *np, *next_np;
			/* Detach this scope from the unified view. */
			sh.track_tree = dtview(sp->strack,0);
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
			sh.fun_tree = dtview(sp->sfun,0);
			/* Free all elements of the subshell function table. */
			for(np = (Namval_t*)dtfirst(sp->sfun); np; np = next_np)
			{
				struct Ufunction *rp = np->nvalue;
				next_np = (Namval_t*)dtnext(sp->sfun,np);
				if(!rp)
				{
					/* Dummy node created by unall() to mask parent shell function. */
					nv_delete(np,sp->sfun,0);
					continue;
				}
				nv_onattr(np,NV_FUNCTION);  /* in case invalidated by unall() */
				if(rp->fname && sh.fpathdict && nv_search(rp->fname,sh.fpathdict,0))
				{
					/* Autoloaded function. It must not be freed. */
					rp->fdict = NULL;
					nv_delete(np,sp->sfun,NV_FUNCTION|NV_NOFREE);
				}
				else
				{
					nv_unset(np,NV_RDONLY);
					nv_delete(np,sp->sfun,NV_FUNCTION);
				}
			}
			dtclose(sp->sfun);
		}
		n = sh.st.trapmax-savst.trapmax;
		sh_sigreset(1);
		if(n>0)
			memset(&sh.st.trapcom[savst.trapmax],0,n*sizeof(char*));
		sh.st = savst;
		sh.st.otrap = 0;
		if(nsig)
		{
			for (isig = 0; isig < nsig; ++isig)
				if (sh.st.trapcom[isig] && sh.st.trapcom[isig]!=Empty)
					free(sh.st.trapcom[isig]);
			memcpy((char*)&sh.st.trapcom[0],savsig,nsig*sizeof(char*));
			free(savsig);
		}
		sh.options = sp->options;
#if _lib_openat
		if(sh.pwdfd != sp->pwdfd)
		{
			/*
			 * Restore the parent shell's present working directory.
			 * Note: cd will always fork if sp->pwdfd is -1 (after calling sh_validate_subpwdfd()),
			 * which only occurs when a subshell is started with sh.pwdfd == -1. As such, in this
			 * if block sp->pwdfd is always > 0 (whilst sh.pwdfd is guaranteed to differ, and
			 * might not be valid).
			 */
			if(fchdir(sp->pwdfd) < 0)
			{
				/* Couldn't fchdir back; close the fd and cope with the error */
				sh_close(sp->pwdfd);
				saveerrno = errno;
				fatalerror = 2;
			}
			else if(sp->pwd && strcmp(sp->pwd,sh.pwd))
				path_newdir(sh.pathlist);
			if(fatalerror != 2)
				sh_pwdupdate(sp->pwdfd);
		}
#else
		/* restore the present working directory */
#if __QNX__
		/* workaround: on QNX 6.5.0, fchdir back to /dev fails with ENOSYS */
		if(sp->pwdfd > 0 && fchdir(sp->pwdfd) < 0 && !(errno==ENOSYS && chdir(sp->pwd)==0))
#else
		if(sp->pwdfd > 0 && fchdir(sp->pwdfd) < 0)
#endif /* __QNX__ */
		{
			saveerrno = errno;
			fatalerror = 2;
		}
		else if(sp->pwd && strcmp(sp->pwd,sh.pwd))
			path_newdir(sh.pathlist);
		if(sp->pwdclose)
			sh_close(sp->pwdfd);
#endif /* _lib_openat */
		free(sh.pwd);
		sh.pwd = sp->pwd;
		if(sp->mask!=sh.mask)
			umask(sh.mask=sp->mask);
		if(sh.coutpipe!=sp->coutpipe)
		{
			sh_close(sh.coutpipe);
			sh_close(sh.cpipe[1]);
		}
		sh.cpid = sp->cpid;
		sh.cpipe[1] = sp->cpipe;
		sh.coutpipe = sp->coutpipe;
		/* restore $RANDOM seed and state */
		rp = (struct rand*)RANDNOD->nvfun;
		if(sp->rand_state)
		{
			memcpy(rp->rand_seed, sp->rand_seed, sizeof rp->rand_seed);
			rp->rand_last = sp->rand_last;
		}
		/* restore $SRANDOM upper bound */
		sh.srand_upper_bound = sp->srand_upper_bound;
		/* Real subshells have their exit status truncated to 8 bits by the kernel.
		 * Since virtual subshells should be indistinguishable, do the same here. */
		sh.exitval &= SH_EXITMASK;
		if(sh.chldexitsig)
			sh.exitval |= SH_EXITSIG;
	}
	sh.subshare = sp->subshare;
	sh.subshell--;			/* decrease level of virtual subshells */
	sh.realsubshell--;		/* decrease ${.sh.subshell} */
	subshell_data = sp->prev;
	sh_popcontext(&checkpoint);
	if(!argsav  ||  argsav->dolrefcnt==argcnt)
		sh_argfree(argsav,0);
	if(sh.topfd != checkpoint.topfd)
		sh_iorestore(checkpoint.topfd|IOSUBSHELL,jmpval);
	if(sp->sig)
	{
		if(sp->prev)
			sp->prev->sig = sp->sig;
		else
		{
			kill(sh.current_pid,sp->sig);
			sh_chktrap();
		}
	}
	sh_sigcheck();
	sh.trapnote = 0;
	nsig = sh.savesig;
	sh.savesig = 0;
	if(nsig>0)
		kill(sh.current_pid,nsig);
	if(sp->subpid)
		job_wait(sp->subpid);
	sh.comsub = sp->comsub;
	if(comsub && iop && sp->pipefd<0)
		sfseek(iop,0,SEEK_SET);
	if(sh.trapnote)
		sh_chktrap();
	if(sh.exitval > SH_EXITSIG)
	{
		int sig = sh.exitval&SH_EXITMASK;
		if(sig==SIGINT || sig== SIGQUIT)
			kill(sh.current_pid,sig);
	}
	if(fatalerror)
	{
		((struct checkpt*)sh.jmplist)->mode = SH_JMPERREXIT;
		switch(fatalerror)
		{
			case 1:
				sh.toomany = 1;
				errno = saveerrno;
				errormsg(SH_DICT,ERROR_SYSTEM|ERROR_PANIC,e_redirect);
				UNREACHABLE();
			case 2:
				/* reinit PWD as it will be wrong */
				free(sh.pwd);
				sh.pwd = NULL;
				path_pwd();
				errno = saveerrno;
				errormsg(SH_DICT,ERROR_SYSTEM|ERROR_PANIC,"Failed to restore PWD upon exiting subshell");
				UNREACHABLE();
			default:
				errormsg(SH_DICT,ERROR_SYSTEM|ERROR_PANIC,"Subshell error %d",fatalerror);
				UNREACHABLE();
		}
	}
	if(sh.ignsig)
		kill(sh.current_pid,sh.ignsig);
	if(jmpval==SH_JMPSUB && sh.lastsig)
		kill(sh.current_pid,sh.lastsig);
	if(jmpval && sh.toomany)
		siglongjmp(*sh.jmplist,jmpval);
	return iop;
}

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
 *   Create and manage subshells avoiding forks when possible
 *
 *   David Korn
 *   AT&T Labs
 *
 */

#include	"FEATURE/options"
#include	"defs.h"
#include	<ls.h>
#include	"io.h"
#include	"fault.h"
#include	"shnodes.h"
#include	"shlex.h"
#include	"jobs.h"
#include	"variables.h"
#include	"path.h"
#include	"FEATURE/poll"

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
 * Pending pipe carrying the standard output of a child process that was
 * forked from within a command substitution. The parent process copies the
 * pipe's output into the comsub's output buffer or temp file. It blocks
 * until the pipe is disconnected (i.e., all writers have closed it).
 */
struct Comsubpipe
{
	struct Comsubpipe *next;
	pid_t		pid;	/* process ID of the child that writes to the pipe */
	int		rfd;	/* read end of the pipe */
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
	Dt_t		*sfaldt;/* subshell function autoload loop detection tree */
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
	uint8_t		fdstatus;
	int		fdsaved; /* bit mask for saved file descriptors */
	int		sig;	/* signal for $$ */
	pid_t		bckpid;
	pid_t		cpid;
	int		coutpipe;
	int		cpipe;
	char		subshare;
	char		comsub;
	unsigned int	rand_seed;          /* parent shell $RANDOM seed */
	int		rand_last;          /* last random number from $RANDOM in parent shell */
	char		rand_state;         /* 0 means sp->rand_seed hasn't been set, 1 is the opposite */
	int		pwdfd;              /* parent shell's file descriptor for PWD */
	struct Comsubpipe *cpipes;          /* pending command substitution output pipes */
	int		comsubtmp;          /* set if comsub output is a real temp file */
	dev_t		comsubdev;          /* device of the comsub output temp file */
	ino_t		comsubino;          /* inode of the comsub output temp file */
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
			ast_close(1);
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
		sh_iostream(1,0);
		sfset(sfstdout,SFIO_SHARE|SFIO_PUBLIC,1);
		sfpool(sfstdout,sh.outpool,SFIO_WRITE);
		if(pp && pp->olist  && pp->olist->strm == sfstdout)
			pp->olist->strm = NULL;
		/*
		 * Remember which file standard output now refers to, so that we can tell
		 * whether standard output still is the command substitution's output file
		 * (and has not been redirected in the meantime).
		 */
		if(sp && sp->pipe==sp)
		{
			struct stat st;
			if(fstat(1,&st)==0)
			{
				sp->comsubtmp = 1;
				sp->comsubdev = st.st_dev;
				sp->comsubino = st.st_ino;
			}
		}
	}
}


/**** The sh_comsub* and comsub* functions implement a blocking pipe mechanism for command substitutions ****/

/*
 * If the current process is executing a command substitution,
 * return a pointer to its subshell structure, otherwise NULL.
 */
static inline struct subshell *comsubdata(void)
{
	struct subshell *sp = subshell_data;
	if(!sh.comsub || !sp || !(sp = sp->pipe))
		return NULL;
	/* a command substitution is the subshell whose pipe points to itself */
	return sp->pipe==sp ? sp : NULL;
}

/*
 * Return nonzero if standard output has not been redirected away from
 * the command substitution's output capture buffer or temporary file.
 */
static inline int comsubstdout(struct subshell *sp)
{
	struct stat st;
	if(!sp->comsubtmp || sffileno(sfstdout) != 1 || !(sh.fdstatus[1]&IOWRITE) || fstat(1,&st) < 0)
		return 0;
	return st.st_dev==sp->comsubdev && st.st_ino==sp->comsubino;
}

/*
 * Return nonzero if file descriptor <fd> refers to the same file as the
 * command substitution's captured output, i.e., if it has been redirected
 * to standard output. File descriptors that the shell does not know to be
 * open for writing are ignored.
 */
static inline int comsubstdoutfd(struct subshell *sp, int fd)
{
	struct stat st;
	if(!sp->comsubtmp || fd<0 || fd>=sh.lim.open_max || fd==1 || !(sh.fdstatus[fd]&IOWRITE))
		return 0;
	/* disregard internally tracked file descriptor copies (user redirections never set these) */
	if(sh.fdptrs[fd])
		return 0;
	return fstat(fd,&st)==0 && st.st_dev==sp->comsubdev && st.st_ino==sp->comsubino;
}

/*
 * Copy the output that is currently available on the pipe <rfd> to
 * the command substitution's output buffer/temp file. Returns 1 if
 * the pipe is disconnected (i.e. the last writer has closed it).
 */
int sh_comsubpiperead(int rfd)
{
	char buff[SFIO_BUFSIZE];
	ssize_t n;
	while((n = read(rfd,buff,sizeof(buff))) > 0)
		sfwrite(sfstdout,buff,(size_t)n);
	return n == 0;
}

/*
 * Collect the output of any pending pipe that has been disconnected without
 * blocking, and close it. This stops file descriptors accumulating when a
 * command substitution starts many child processes.
 */
static void comsubpipescavenge(struct subshell *sp)
{
	struct Comsubpipe **cpipes_p = &sp->cpipes, *cpipe;
	while(*cpipes_p)
	{
		cpipe = *cpipes_p;
		if(sh_comsubpiperead(cpipe->rfd))
		{
			/* the pipe is disconnected */
			*cpipes_p = cpipe->next;
			sh_close(cpipe->rfd);
			free(cpipe);
			continue;
		}
		cpipes_p = &cpipe->next;
	}
}

/*
 * Prepare to fork a child process whose standard output is to be captured
 * by the command substitution that this process is executing. Creates a
 * pipe <pv>; the child's standard output must be connected to pv[1], and
 * the comsub process reads pv[0] when it is time to collect the child's
 * output. Returns 1 if a pipe was created, 0 otherwise.
 */
int sh_comsubpipe(int pv[2])
{
	struct subshell *sp = comsubdata();
	if(!sp || !comsubstdout(sp))
		return 0;
	comsubpipescavenge(sp);
	sh_pipe(pv,1);
	if(fcntl(pv[0],F_SETFL,O_NONBLOCK) < 0)
		errormsg(SH_DICT,ERROR_system(1),e_pipe);
	return 1;
}

/*
 * Redirect every file descriptor that is redirected to standard output, i.e.,
 * that refers to the command substitution's output temp file, to the write end
 * <wfd> of a command substitution output pipe. This ensures that its output is
 * captured in proper order with the rest of the command substitution's output.
 *
 * If <save> is nonzero, the original file descriptors are saved with
 * sh_iosave() so that sh_iorestore() can restore them in the command
 * substitution process itself afterwards; the value that sh.topfd had
 * before saving is returned for this purpose.
 *
 * Returns -1 if this is not a command substitution process.
 */
int sh_comsubpipestdout(int wfd, int save)
{
	struct subshell *sp = comsubdata();
	int fd, indx = sh.topfd, dupfd, clexec;
	if(!sp)
		return -1;
	for(fd = 0; fd < sh.lim.open_max; fd++)
	{
		if(!comsubstdoutfd(sp,fd) || (dupfd = sh_fcntl(wfd,F_DUPFD,10)) < 0)
			continue;
		clexec = sh.fdstatus[fd] & IOCLEX;
		if(save)
			sh_iosave(fd,indx,NULL);
		sh_iorenumber(dupfd,fd);
		/* sh_iorenumber() clears the close-on-exec flag; preserve it */
		if(clexec)
			sh_fcntl(fd,F_SETFD,FD_CLOEXEC);
	}
	return indx;
}

/*
 * Register the read end of a command substitution output pipe for process
 * <pid>, so that its output can be collected later.
 */
void sh_comsubpipeadd(pid_t pid, int rfd)
{
	struct Comsubpipe *cpipe;
	struct subshell *sp = comsubdata();
	if(!sp)
	{
		sh_close(rfd);
		return;
	}
	cpipe = sh_malloc(sizeof(struct Comsubpipe));
	cpipe->pid = pid;
	cpipe->rfd = rfd;
	cpipe->next = sp->cpipes;
	sp->cpipes = cpipe;
}

/*
 * Called in the parent process after the child has been forked, to register
 * the read end of the command substitution output pipe <pv> for process <pid>.
 */
void sh_comsubpipeparent(pid_t pid, int pv[2])
{
	sh_close(pv[1]);	/* the child has its own copy */
	sh_comsubpipeadd(pid,pv[0]);
}

/*
 * Called in the forked child process to connect standard output
 * to the write end of the command substitution output pipe <pv>.
 * No extra file descriptors are exposed to the child.
 */
void sh_comsubpipechild(int pv[2])
{
	sh_close(pv[0]);
	sh_close(1);  /* close existing stdout to free the Sfio stream */
	if(dup2(pv[1],1) < 0)
	{
		errormsg(SH_DICT,ERROR_SYSTEM|ERROR_PANIC,e_pipestdout);
		UNREACHABLE();
	}
	/* standard output is now a pipe, not the comsub's string stream or temp file */
	sh.fdstatus[1] = IONOSEEK|IOWRITE;
	sh_iostream(1,0);
	/*
	 * Any other file descriptors that are redirected to standard output must be
	 * connected to the pipe as well, so that their output is captured and
	 * properly ordered. The redirection is permanent in this child process.
	 */
	sh_comsubpipestdout(pv[1],0);
	sh_close(pv[1]);
}

/*
 * Collect the output of the pipe <cpipe> into the command substitution's
 * output buffer or temp file. If <pid> is zero, this blocks until the pipe
 * is disconnected, i.e. until all writers (including background children)
 * have closed it.
 *
 * If <pid> is nonzero, stop collecting as soon as that process has finished,
 * and leave the pipe pending.
 *
 * Returns 1 if the pipe is disconnected (so the caller must close it).
 */
static inline int comsubpipecollect(struct Comsubpipe *cpipe, pid_t pid)
{
#if _lib_poll
	struct pollfd pfd;
#endif /* _lib_poll */
	while(1)
	{
		if(sh_comsubpiperead(cpipe->rfd))
			return 1;
		if(pid && (job_isdone(pid) || job.savesig))
			return 0;
		/* wait for more output, disconnection, or a signal such as SIGCHLD */
#if _lib_poll
		pfd.fd = cpipe->rfd;
		pfd.events = POLLIN;
		if(poll(&pfd,1,100) < 0 && errno != EINTR)
			return 0;
#else
		sh_delay(.001,1);
#endif /* _lib_poll */
	}
}

/*
 * Collect the output of the comsub output pipes into the comsub's output
 * buffer or temporary file. If <pid> is nonzero, only collect the pipe(s)
 * for that process, and leave anything that remains after that process has
 * finished pending; if zero, collect all pending pipes, blocking until they
 * are disconnected.
 */
void sh_comsubpipedrain(pid_t pid)
{
	struct subshell *sp = comsubdata();
	struct Comsubpipe **cpipes_p, *cpipe;
	if(!sp || !sp->cpipes || !comsubstdout(sp))
		return;
	comsubpipescavenge(sp);
	cpipes_p = &sp->cpipes;
	while(*cpipes_p)
	{
		cpipe = *cpipes_p;
		if(pid && cpipe->pid!=pid)
		{
			cpipes_p = &cpipe->next;
			continue;
		}
		if(!comsubpipecollect(cpipe,pid))
		{
			cpipes_p = &cpipe->next;
			continue;
		}
		*cpipes_p = cpipe->next;
		sh_close(cpipe->rfd);
		free(cpipe);
	}
}

/*
 * Discard and close any remaining command substitution output pipes.
 */
static inline void comsubpipeclose(struct subshell *sp)
{
	struct Comsubpipe *cpipe, *next;
	for(cpipe = sp->cpipes; cpipe; cpipe = next)
	{
		next = cpipe->next;
		sh_close(cpipe->rfd);
		free(cpipe);
	}
	sp->cpipes = NULL;
}

/**** End of functions for a blocking pipe mechanism for command substitutions ****/


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
	int csp = 0, cspv[2];
	char *trap = sh.st.trapcom[0];
	if(trap)
		trap = sh_strdup(trap);
	/* see whether inside $(...) */
	if(sp->pipe)
	{
		sh_subtmpfile();
		/* the child's standard output must go through a pipe so we can wait for it */
		csp = sh_comsubpipe(cspv);
	}
	sh.curenv = 0;
	/* arm critical signal region */
	sh.savesig = -1;
	if(pid = sh_fork(F_SUBFORK,NULL))
	{
		sh.curenv = curenv;
		/* this is the parent part of the fork */
		if(csp)
			sh_comsubpipeparent(pid,cspv);
		if(sp->subpid==0)
			sp->subpid = pid;
		if(trap)
			free(trap);
		siglongjmp(*sh.jmplist,SH_JMPSUB);
	}
	else
	{
		/* this is the child part of the fork */
		if(csp)
			sh_comsubpipechild(cspv);
		/*
		 * In a virtual subshell, $RANDOM is not reseeded until it's used, so if that
		 * hasn't happened yet, invalidate the seed. This allows sh_save_rand_seed()
		 * to reseed $RANDOM later.
		 */
		if(!sp->rand_state)
			sh_invalidate_rand_seed();
		subshell_data = NULL;
		sh.subshell = 0;
		/* sh.comsub is deliberately *not* reset here; a forked command substitution is still a command substitution */
		sp->subpid=0;
		sh.st.trapcom[0] = (comsub==2 ? NULL : trap);
		/* disarm critical signal region */
		sh.savesig = 0;
		/* sh_fork() increases ${.sh.subshell} but we forked an existing virtual subshell, so undo */
		sh.realsubshell--;
	}
}

int nv_subsaved(Namval_t *np, nvflag_t flags)
{
	struct subshell	*sp;
	struct Link		*lp, *lpprev;
	for(sp = (struct subshell*)subshell_data; sp; sp=sp->prev)
	{
		lpprev = NULL;
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
		sp->rand_seed = rp->rand_seed;
		sp->rand_last = rp->rand_last;
		sp->rand_state = 1;
		if(reseed)
			sh_reseed_rand(rp);
	}
	else if(reseed && rp->rand_last == RAND_SEED_INVALIDATED)
		sh_reseed_rand(rp);
}

/*
 * Return 1 if <np> is a member of the type instance <mp>, possibly via one or
 * more nested compound members; that is, if <np> is stored in the value data
 * that <mp> owns.
 */
static int is_member(Namval_t *np, Namval_t *mp)
{
	Namval_t	*tp;
	while((tp = nv_typeparent(np)) && tp != np)
	{
		if(tp == mp)
			return 1;
		np = tp;
	}
	return 0;
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
	/*
	 * A type instance is not saved in a subshell scope if one of its members has
	 * already been saved there; the member's value is then restored on its own.
	 */
	if(add && nv_typeparent(np)==np)
		for(lp=sp->svar; lp; lp=lp->next)
			if(is_member(lp->node,np))
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
 * Helper for nv_restore() to restore a variable with a default discipline.
 * These need to be restored via nv_putval() so their disciplines restore state.
 *
 * mp: variable to restore into
 * np: copy to restore from (was saved by sh_assignok())
 */
static inline void restore_specialvar(Namval_t *mp, Namval_t *np)
{
	void *val;
	nvflag_t np_flags = np->nvflag;
	int mp_freeable = !nv_isattr(mp, NV_NOFREE);
	if (np_flags & NV_INTEGER)
	{
		/* For numeric types, re-use attribute logic from nv_putval. */
		Sfdouble_t ld = nv_getnum(np);
		if ((np_flags & NV_DOUBLE) == NV_DOUBLE)
		{
			if ((np_flags & NV_LONG) && sizeof(double) < sizeof(Sfdouble_t))
			{
				val = sh_malloc(sizeof(Sfdouble_t));
				*((Sfdouble_t*)val) = ld;
			}
			else
			{
				val = sh_malloc(sizeof(double));
				*((double*)val) = (double)ld;
			}
		}
		else
		{
			if (np_flags & NV_LONG)
			{
				val = sh_malloc(sizeof(Sflong_t));
				if (np_flags & NV_UNSIGN)
					*((Sfulong_t*)val) = (Sfulong_t)ld;
				else
					*((Sflong_t*)val) = (Sflong_t)ld;
			}
			else if (np_flags & NV_SHORT)
			{
				val = sh_malloc(sizeof(int16_t));
				if (np_flags & NV_UNSIGN)
					*((uint16_t*)val) = (uint16_t)ld;
				else
					*((int16_t*)val) = (int16_t)ld;
			}
			else
			{
				val = sh_malloc(sizeof(int32_t));
				if (np_flags & NV_UNSIGN)
					*((uint32_t*)val) = (uint32_t)ld;
				else
					*((int32_t*)val) = (int32_t)ld;
			}
		}
	}
	else
		val = nv_getval(np);
	nv_putval(mp, val, NV_RDONLY|NV_NOFREE|np_flags);
	/*
	 * Passing NV_NOFREE to nv_putval: (1) avoids creating a new copy
	 * of val; (2) sets NV_NOFREE in the node. We only want (1) here.
	 */
	if (mp_freeable)
		nv_offattr(mp, NV_NOFREE);
	/*
	 * Some disciplines may still create a newly allocated value.
	 */
	if (np->nvalue != mp->nvalue && !nv_isattr(np, NV_NOFREE))
		free(np->nvalue);
}

static inline void free_scalarvalue(Namval_t *n)
{
	if (n->nvalue && !nv_isvtree(n) && !nv_isref(n) && !nv_arrayptr(n) && !nv_isattr(n,NV_MINIMAL|NV_NOFREE))
	{
		if (n->nvalue!=Empty && n->nvalue!=AltEmpty)
			free(n->nvalue);
		n->nvalue = NULL;
	}
}

static inline void free_disciplines(Namval_t *n)
{
	Namfun_t *nf, *next;
	/* note: freeing disciplines breaks the checks in free_scalarvalue() */
	for (nf = n->nvfun; nf; nf = next)
	{
		next = nf->next;
		if (!(nf->namflags & NAMFUN_NOFREE))
			free(nf);
	}
}

/*
 * Return the number of disciplines on the list <fp>
 */
static inline size_t count_disciplines(Namfun_t *fp)
{
	size_t n = 0;
	while (fp)
		n++, fp = fp->next;
	return n;
}

/*
 * Free the discipline functions that were created within a virtual subshell
 * and return the resulting head of <mp>'s discipline list.
 *
 * New disciplines are pushed onto the head of a variable's discipline list, so
 * the ones created in the subshell precede the <keep> disciplines that were
 * already on the list when the subshell started. Those are not necessarily the
 * same objects, because sh_assignok() clones them into the saved copy of the
 * variable, so <saved> points at that clone list to avoid freeing a saved
 * discipline by mistake.
 */
static Namfun_t *free_subshell_disciplines(Namval_t *mp, Namfun_t *saved, size_t keep)
{
	Namfun_t	*fp, *next, *sp;
	size_t		n, total;
	if ((total = count_disciplines(mp->nvfun)) <= keep)
		return mp->nvfun;
	for (n = total - keep, fp = mp->nvfun; n && fp; n--)
	{
		for (sp = saved; sp && sp!=fp; sp = sp->next)
			;
		if (sp == fp)
			break;  /* the saved disciplines start here, so stop */
		next = fp->next;
		if (!(fp->namflags & NAMFUN_NOFREE))
			free(fp);
		mp->nvfun = fp = next;
	}
	return mp->nvfun;
}

/*
 * restore the variables
 */
static void nv_restore(struct subshell *sp)
{
	struct Link	*lp, *lq;
	Namval_t	*mp, *np;
	Namval_t	*mpnext;
	Namfun_t	*fp;
	nvflag_t	flags;
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
		{
			Namarr_t	*ap;
			nv_putsub(mp,NULL,ARRAY_SCAN);
			/*
			 * nv_putsub() clears ARRAY_SCAN again when the scan finds no
			 * elements, but array_putval() -- which nv_unset() calls below to
			 * free the array -- only frees an array that is flagged as fully
			 * scanned, so the array would be leaked. An array without elements
			 * is fully scanned by definition, so flag it as such.
			 */
			if ((ap = nv_arrayptr(mp)) && array_elem(ap)==0)
				ap->nelem |= ARRAY_SCAN;
		}
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
		fp = mp->nvfun;  /* save the pre-restore parent scope disciplines list */
		nv_unset(mp,NV_RDONLY|NV_CLONE);
		if(nv_isarray(np))
		{
			nv_clone(np,mp,NV_MOVE);
			goto skip;
		}
		nv_setsize(mp,nv_size(np));
		if(!(flags&NV_MINIMAL))
			mp->nvmeta = np->nvmeta;
		/*
		 * Free leftover disciplines and scalar values from subshell variables
		 * with shell discipline functions. Note that nv_unset() above does not
		 * run the unset discipline during virtual subshell cleanup, so any
		 * discipline created within the subshell is still on <mp>'s list here.
		 */
		if(!(fp && mp->nvfun==fp && np->nvfun && np->nvfun!=fp))
		{
			if (mp->nvfun)
			{
				if (np->nvfun)
					free_subshell_disciplines(mp, np->nvfun, count_disciplines(np->nvfun));
				else
				{
					if (mp->nvalue != np->nvalue)
						free_scalarvalue(mp);
					free_disciplines(mp);
				}
			}
			mp->nvfun = np->nvfun;
			fp = NULL;  /* Avoid duplicate freeing below */
		}
		else
		{
			/*
			 * The subshell redefined a discipline function instead of adding
			 * one, so <fp> is still the parent's own struct vardisc rather than
			 * one made in the subshell. Its definitions do refer to the subshell's
			 * functions now, so copy back the ones that sh_assignok() saved in
			 * <np> when the subshell started. The saved copy is freed below.
			 */
			nv_restore_disc(mp,np);
		}
		if(nv_isattr(np,NV_IDENT))
		{
			nv_offattr(np,NV_IDENT);
			flags |= NV_NOFREE;
		}
		mp->nvflag = np->nvflag|(flags&NV_MINIMAL);
		if(nv_enforcedisc(mp))
			restore_specialvar(mp, np);
		else
		{
			if(fp && mp->nvalue!=np->nvalue)
				free_scalarvalue(mp);
			mp->nvalue = np->nvalue;
		}
		if(fp)
			free_disciplines(np);
		np->nvfun = NULL;
		if(nv_isattr(mp,NV_EXPORT))
		{
			char *name = nv_name(mp);
			env_change();
			if(strcmp(name,"_AST_FEATURES")==0)
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

/*
 * Return pointer to the function autoload loop detection tree.
 * Create new one if in a subshell and one doesn't exist and 'create' is non-zero.
 */
Dt_t *sh_subloopdetecttree(int create)
{
	struct subshell *sp = subshell_data;
	if(!sh.funload_loopdetect_tree)
		sh.funload_loopdetect_tree = dtopen(&_Nvdisc,Dtoset);
	if(create && sh.subshell && !sh.subshare)
	{
		if(sp && !sp->sfaldt)
		{
			sp->sfaldt = dtopen(&_Nvdisc,Dtoset);
			dtview(sp->sfaldt, sh.funload_loopdetect_tree);
			sh.funload_loopdetect_tree = sp->sfaldt;
		}
	}
	return sh.funload_loopdetect_tree;
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
 * Close all prior file descriptors for the subshell PWDs no longer in use.
 * This is always called after forking to avoid leaking unused file descriptors
 * to the child processes.
 */
void sh_clear_subshell_pwdfd(void)
{
	struct subshell *sp = subshell_data;
#if _lib_openat
	int prevfd = sh.pwdfd;
	while(sp)
	{
		if(sp->pwdfd > -1 && sp->pwdfd != prevfd)
		{
			sh_close(sp->pwdfd);
			prevfd = sp->pwdfd;
		}
		sp = sp->prev;
	}
#else
	while(sp)
	{
		if(sp->pwdclose)
		{
			sh_close(sp->pwdfd);
			sp->pwdclose = 0;
			sp->pwdfd = -1;
		}
		sp = sp->prev;
	}
#endif /* _lib_openat */
}

/*
 * Run command tree <t> in a virtual subshell
 * If comsub is not null, then output will be placed in temp file (or buffer)
 * If comsub is not null, the return value will be a stream consisting of
 * output of command <t>.  Otherwise, NULL will be returned.
 */
Sfio_t *sh_subshell(Shnode_t *t, volatile int flags, char comsub)
{
	struct subshell sub_data;
	struct subshell *sp = &sub_data;
	int n, jmpval;
	volatile int fatalerror = 0, saveerrno = 0, argcnt;
	unsigned int savecurenv = sh.curenv;
	int savejobpgid = job.curpgid;
	int *saveexitval = job.exitval;
	char **volatile savsig;
	volatile size_t nsig = 0;
	Sfio_t *volatile iop = NULL;
	struct checkpt checkpoint;
	struct sh_scoped savst;
	struct dolnod   *argsav = NULL;
	sfsync(sh.outpool);
	memset((char*)sp, 0, sizeof(*sp));
	sp->options = sh.options;
	sp->subshare = sh.subshare;
	sp->comsub = sh.comsub;
	sp->pwdfd = -1;	/* pwdfd should not be initialized to stdin */
	sh_sigcheck();
	/* arm critical signal region */
	sh.savesig = -1;
	if(sh.curenv==0)
	{
		subshell_data = NULL;
		subenv = 0;
	}
	sh.curenv = ++subenv;
	savst = sh.st;
	sh.subshell++;		/* increase level of virtual subshells */
	sh.realsubshell++;	/* increase ${.sh.subshell} */
	sp->prev = subshell_data;
	subshell_data = sp;
	if(argsav = sh_arguse())
		argcnt = argsav->dolrefcnt;
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
	sh.subshare = comsub==2;
	if(!sh.subshare)
		sp->pathlist = path_dup((Pathcomp_t*)sh.pathlist);
	if(comsub)
		sh.comsub = comsub;
	if(!sh.subshare)
	{
		char *save_debugtrap = NULL;
#if _lib_openat
		sp->pwd = sh_strdup(sh.pwd);
		sp->pwdfd = sh.pwdfd;
#else
		struct subshell *xp;
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
			n = sh_open(e_dot,O_SEARCH|O_cloexec);
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
		sp->cpid = sh.cpid;
		sp->coutpipe = sh.coutpipe;
		sp->cpipe = sh.cpipe[1];
		sh.cpid = 0;
		/* save trap table */
		memset(sh.st.trapnofree, 0xFF, sizeof sh.st.trapnofree);
		sh.st.otrap = savst.trap;
		if((nsig = sh.st.trapmax * sizeof(char**)) > 0)
			savsig = sh.st.otrapcom = memcpy(stkalloc(sh.stk, nsig), sh.st.trapcom, nsig);
		else
			sh.st.otrapcom = NULL;
		if(sh_isoption(SH_FUNCTRACE) && sh.st.trap[SH_DEBUGTRAP] && *sh.st.trap[SH_DEBUGTRAP])
			save_debugtrap = sh_strdup(sh.st.trap[SH_DEBUGTRAP]);
		sh_sigreset(0);
		if(save_debugtrap)
			sh.st.trap[SH_DEBUGTRAP] = save_debugtrap;
	}
	sh_pushcontext(&checkpoint,SH_JMPSUB);
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
		/* execute the subshell if no signal was saved */
		if(sh.savesig < 0)
		{
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
			/* On scripts with -m on, we must fork any non-comsub subshell invoked from the main
			 * shell in order for it to start and lead a new process group (via _sh_fork()).  */
			else if(sh_isstate(SH_MONITOR))
				sh_subfork();
			/* disarm critical signal region */
			sh.savesig = 0;
			/* Execute the subshell tree */
			sh_exec(t,flags);
		}
	}
	/* re-arm critical signal region (do not overwrite saved signal if any) */
	if(!sh.savesig)
		sh.savesig = -1;
	if(comsub!=2 && jmpval!=SH_JMPSUB && sh.st.trapcom[0] && sh.subshell)
	{
		/* trap on EXIT not handled by child */
		char *trap=sh.st.trapcom[0];
		sh.st.trapcom[0] = NULL;	/* prevent recursion */
		sh_trap(trap,0);
		if(!sh.savesig)
			sh.savesig = -1;
		free(trap);
	}
	if(sh.subshell==0)	/* we must have forked with sh_subfork(); this is the child process */
	{
		subshell_data = sp->prev;
		sh_popcontext(&checkpoint);
		/* disarm critical signal region */
		sh.savesig = 0;
		if(jmpval==SH_JMPSCRIPT)
			siglongjmp(*sh.jmplist,jmpval);
		sh.exitval &= SH_EXITMASK;
		if(sh.chldexitsig)
			sh.exitval |= SH_EXITSIG;
		sh_done(0);
	}
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
			iop = sh_iostream(sp->pipefd,0);
			sfclose(sfstdout);
		}
		else
		{
			/*
			 * Collect the output of any child processes that are still
			 * connected to a pipe. This blocks until they have all
			 * disconnected from the pipe, so no output is lost.
			 */
			sh_comsubpipedrain(0);
			/*
			 * Wait for any remaining foreground child process.
			 */
			if(sh.spid)
			{
				int e = sh.exitval;
				char c = sh.chldexitsig;
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
		{
			sfswap(sp->saveout,sfstdout);
			sp->saveout = NULL;
		}
		else
			sfstdout = &_Sfstdout;
		/* check if standard output was preserved */
		if(sp->tmpfd>=0)
		{
			ast_close(1);
			if (fcntl(sp->tmpfd,F_DUPFD,1) != 1)
			{
				saveerrno = errno;
				fatalerror = 1;
			}
			sh_close(sp->tmpfd);
			sp->tmpfd = -1;
		}
		sh.fdstatus[1] = sp->fdstatus;
		comsubpipeclose(sp);
	}
	if(!sh.subshare)
	{
		path_delete((Pathcomp_t*)sh.pathlist);
		sh.pathlist = sp->pathlist;
		sp->pathlist = NULL;
	}
	if(sp->jobs)
	{
		job_subrestore(sp->jobs);
		sp->jobs = NULL;
	}
	sh.curenv = sh.jobenv = savecurenv;
	job.curpgid = savejobpgid;
	job.exitval = saveexitval;
	sh.bckpid = sp->bckpid;
	if(!sh.subshare)	/* restore environment if saved */
	{
		struct rand *rp;
		sh.options = sp->options;
		/* Clean up subshell hash table. */
		if(sp->strack)
		{
			Namval_t *np, *next_np;
			/* Detach this scope from the unified view. */
			sh.track_tree = dtview(sp->strack,NULL);
			/* Free all elements of the subshell hash table. */
			for(np = (Namval_t*)dtfirst(sp->strack); np; np = next_np)
			{
				next_np = (Namval_t*)dtnext(sp->strack,np);
				nv_delete(np,sp->strack,0);
			}
			/* Close and free the table itself. */
			dtclose(sp->strack);
			sp->strack = NULL;
		}
		/* Clean up subshell function table. */
		if(sp->sfun)
		{
			Namval_t *np, *next_np;
			/* Detach this scope from the unified view. */
			sh.fun_tree = dtview(sp->sfun,NULL);
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
			sp->sfun = NULL;
		}
		/* Clean up subshell autoload loop detection tree. */
		if(sp->sfaldt)
		{
			sh.funload_loopdetect_tree = dtview(sp->sfaldt,NULL);
			dtclose(sp->sfaldt);
			sp->sfaldt = NULL;
		}
		n = sh.st.trapmax-savst.trapmax;
		sh_sigreset(1);
		if(n>0)
			memset(&sh.st.trapcom[savst.trapmax],0,(size_t)n*sizeof(char*));
		sh.st = savst;
		sh.st.otrap = NULL;
		if(nsig)
			memcpy(sh.st.trapcom, savsig, nsig);
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
			rp->rand_seed = sp->rand_seed;
			rp->rand_last = sp->rand_last;
		}
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
		sh_argfree(argsav);
	if(sh.topfd != checkpoint.topfd)
		sh_iorestore(checkpoint.topfd|IOSUBSHELL,jmpval);
	/* disarm critical signal region; remember saved signal */
	n = sh.savesig;
	sh.savesig = 0;
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
	/* if a signal was saved in sh.savesig, reissue it */
	if(n > 0)
		kill(sh.current_pid, n);
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

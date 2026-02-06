/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2014 AT&T Intellectual Property          *
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
*                                                                      *
***********************************************************************/
/*
 * Fault handling routines
 *
 *   David Korn
 *   AT&T Labs
 *
 */

#include	"shopt.h"
#include	"defs.h"
#include	<fcin.h>
#include	"io.h"
#include	"history.h"
#include	"shlex.h"
#include	"variables.h"
#include	"jobs.h"
#include	"path.h"
#include	"builtins.h"
#include	"ulimit.h"

#define abortsig(sig)	(sig==SIGABRT || sig==SIGBUS || sig==SIGILL || sig==SIGSEGV)

static char	indone;
static int	cursig = -1;

/*
 * Most signals caught or ignored by the shell come here
*/
void	sh_fault(int sig)
{
	int 		flag=0;
	char		*trap;
	struct checkpt	*pp = (struct checkpt*)sh.jmplist;
	int		action=0;
	int		save_errno = errno;
	/* reset handler */
	if(!(sig&SH_TRAP))
		signal(sig, sh_fault);
	sig &= ~SH_TRAP;
#ifdef SIGWINCH
	if(sig==SIGWINCH)
		sh_winsize();
#endif  /* SIGWINCH */
	trap = sh.st.trapcom[sig];
	if(sh.savesig)
	{
		/* critical region, save and process later */
		if(!(sh.sigflag[sig]&SH_SIGIGNORE))
			sh.savesig = sig;
		goto done;
	}
	if(sig==SIGALRM && sh.bltinfun==b_sleep)
	{
		if(trap && *trap)
		{
			sh.trapnote |= SH_SIGTRAP;
			sh.sigflag[sig] |= SH_SIGTRAP;
		}
		goto done;
	}
	if(sh.subshell && trap && sig!=SIGINT && sig!=SIGQUIT
#ifdef SIGWINCH
	&& sig!=SIGWINCH
#endif
	&& sig!=SIGCONT)
	{
		sh.exitval = SH_EXITSIG|sig;
		sh_subfork();
		sh.exitval = 0;
		goto done;
	}
	/* handle ignored signals */
	if(trap && *trap==0)
		goto done;
	flag = sh.sigflag[sig]&~SH_SIGOFF;
	if(!trap)
	{
		if(flag&SH_SIGIGNORE)
		{
			if(sh.subshell)
				sh.ignsig = sig;
			sigrelease(sig);
			goto done;
		}
		if(flag&SH_SIGDONE)
		{
			void *ptr=0;
			if((flag&SH_SIGINTERACTIVE) && sh_isstate(SH_INTERACTIVE) && !sh_isstate(SH_FORKED))
			{
				/* check for TERM signal between fork/exec */
				if(sig==SIGTERM && job.in_critical)
					sh.trapnote |= SH_SIGTERM;
				goto done;
			}
			sh.lastsig = sig;
			sigrelease(sig);
			if(pp->mode != SH_JMPSUB)
			{
				if(pp->mode < SH_JMPSUB && !sh_isstate(SH_INTERACTIVE))
					pp->mode = sh.subshell?SH_JMPSUB:SH_JMPFUN;
				else
					pp->mode = SH_JMPEXIT;
			}
			if(sh.subshell)
				sh_exit(SH_EXITSIG);
			if(sig==SIGABRT || (abortsig(sig) && (ptr = malloc(1))))
			{
				if(ptr)
					free(ptr);
				sh_done(sig);
			}
			/* mark signal and continue */
			sh.trapnote |= SH_SIGSET;
			if(sig <= sh.sigmax)
				sh.sigflag[sig] |= SH_SIGSET;
			goto done;
		}
	}
	errno = 0;
	if(pp->mode==SH_JMPCMD || (pp->mode==1 && sh.bltinfun) && !(flag&SH_SIGIGNORE))
		sh.lastsig = sig;
	if(trap)
	{
		/*
		 * propagate signal to foreground group
		 */
		if(sig==SIGHUP && job.curpgid)
			killpg(job.curpgid,SIGHUP);
		flag = SH_SIGTRAP;
	}
	else
	{
		sh.lastsig = sig;
		flag = SH_SIGSET;
		if(sig==SIGTSTP && pp->mode==SH_JMPCMD)
		{
			if(sh_isstate(SH_STOPOK))
			{
				sh.trapnote |= SH_SIGTSTP;
				sigrelease(sig);
				sh_exit(SH_EXITSIG);
			}
			goto done;
		}
	}
#ifdef ERROR_NOTIFY
	if((error_info.flags&ERROR_NOTIFY) && sh.bltinfun)
		action = (*sh.bltinfun)(-sig,NULL,NULL);
	if(action>0)
		goto done;
#endif
	if(sh.bltinfun && sh.bltindata.notify)
	{
		sh.bltindata.sigset = 1;
		goto done;
	}
	sh.trapnote |= flag;
	if(sig <= sh.sigmax)
		sh.sigflag[sig] |= flag;
	if(pp->mode==SH_JMPCMD && sh_isstate(SH_STOPOK))
	{
		if(action<0)
			goto done;
		sigrelease(sig);
		sh_exit(SH_EXITSIG);
	}
done:
	/*
	 * Always restore errno, because this code is run during signal handling which may interrupt loops like:
	 *	while((fd = open(path, flags, mode)) < 0)
	 *		if(errno!=EINTR)
	 *			<throw error>;
	 * otherwise that may fail if a signal is caught between the open() call and the errno!=EINTR check.
	 */
	errno = save_errno;
	return;
}

/*
 * Get window size and update LINES and COLUMNS.
 * If the number of columns changed, flags a window size change in sh.winch.
 */
void	sh_winsize(void)
{
	int		lines, columns;
	int32_t		i;
	astwinsize(2,&lines,&columns);
	if (lines < 0 || lines > USHRT_MAX)
		lines = 0;
	if (columns < 0 || columns > USHRT_MAX)
		columns = 0;
	/*
	 * Update LINES and COLUMNS only when the values changed; this makes
	 * LINES.set and COLUMNS.set shell discipline functions more useful.
	 */
	if (lines && (lines != sh.lines || nv_isnull(LINES)) && (i = lines))
	{
		nv_putval(LINES, (char*)&i, NV_INT32|NV_RDONLY);
		sh.lines = lines;
	}
	if (columns && (columns != sh.columns || nv_isnull(COLUMNS)) && (i = columns))
	{
		nv_putval(COLUMNS, (char*)&i, NV_INT32|NV_RDONLY);
		sh.columns = columns;
		sh.winch = 1;
	}
}

/*
 * initialize signal handling
 */
void sh_siginit(void)
{
	int sig, n;
	const struct shtable2	*tp = shtab_signals;
	sig_begin();
	/* find the largest signal number in the table */
#if defined(SIGRTMIN) && defined(SIGRTMAX)
	if ((n = SIGRTMIN) > 0 && (sig = SIGRTMAX) > n && sig < SH_TRAP)
	{
		sh.sigruntime[SH_SIGRTMIN] = n;
		sh.sigruntime[SH_SIGRTMAX] = sig;
	}
#endif /* SIGRTMIN && SIGRTMAX */
	n = SIGTERM;
	while(*tp->sh_name)
	{
		sig = (tp->sh_number&((1<<SH_SIGBITS)-1));
		if (!(sig-- & SH_TRAP))
		{
			if ((tp->sh_number>>SH_SIGBITS) & SH_SIGRUNTIME)
				sig = sh.sigruntime[sig];
			if(sig>n && sig<SH_TRAP)
				n = sig;
		}
		tp++;
	}
	sh.sigmax = n++;
	sh.st.trapcom = (char**)sh_calloc(n,sizeof(char*));
	sh.sigflag = (unsigned char*)sh_calloc(n,1);
	sh.sigmsg = (char**)sh_calloc(n,sizeof(char*));
	for(tp=shtab_signals; sig=tp->sh_number; tp++)
	{
		n = (sig>>SH_SIGBITS);
		if((sig &= ((1<<SH_SIGBITS)-1)) > (sh.sigmax+1))
			continue;
		sig--;
		if(n&SH_SIGRUNTIME)
			sig = sh.sigruntime[sig];
		if(sig>=0)
		{
			sh.sigflag[sig] = n;
			if(*tp->sh_name)
				sh.sigmsg[sig] = (char*)tp->sh_value;
		}
	}
}

/*
 * Turn on trap handler for signal <sig>
 */
void	sh_sigtrap(int sig)
{
	int flag;
	void (*fun)(int);
	sh.st.otrapcom = 0;
	if(sig==0)
		sh_sigdone();
	else if(!((flag=sh.sigflag[sig])&(SH_SIGFAULT|SH_SIGOFF)))
	{
		/* don't set signal if already set or off by parent */
		if((fun=signal(sig,sh_fault))==SIG_IGN)
		{
			signal(sig,SIG_IGN);
			flag |= SH_SIGOFF;
		}
		else
		{
			flag |= SH_SIGFAULT;
			if(sig==SIGALRM && fun!=SIG_DFL && fun!=sh_fault)
				signal(sig,fun);
		}
		flag &= ~(SH_SIGSET|SH_SIGTRAP);
		sh.sigflag[sig] = flag;
	}
}

/*
 * set signal handler so sh_done is called for all caught signals
 */
void	sh_sigdone(void)
{
	int 	flag, sig = sh.sigmax;
	sh.sigflag[0] |= SH_SIGFAULT;
	for(sig=sh.sigmax; sig>0; sig--)
	{
		flag = sh.sigflag[sig];
		if((flag&(SH_SIGDONE|SH_SIGIGNORE|SH_SIGINTERACTIVE)) && !(flag&(SH_SIGFAULT|SH_SIGOFF)))
			sh_sigtrap(sig);
	}
}

/*
 * Restore to default signals
 * Free the trap strings if mode is non-zero
 * If mode>1 then ignored traps cause signal to be ignored
 * If mode==-1 we're entering a new function scope in sh_funscope()
 */
void	sh_sigreset(int mode)
{
	char	*trap;
	int 	flag, sig=sh.st.trapmax;
	/* do not reset sh.st.trapdontexec in a new ksh function scope as parent traps will still be active */
	if(mode < 0)
		mode = 0;
	else
		sh.st.trapdontexec = 0;
	while(sig-- > 0)
	{
		if(trap=sh.st.trapcom[sig])
		{
			flag  = sh.sigflag[sig]&~(SH_SIGTRAP|SH_SIGSET);
			if(*trap)
			{
				if(mode)
					free(trap);
				sh.st.trapcom[sig] = 0;
			}
			else if(sig && mode>1)
			{
				if(sig!=SIGCHLD)
					signal(sig,SIG_IGN);
				flag &= ~SH_SIGFAULT;
				flag |= SH_SIGOFF;
			}
			sh.sigflag[sig] = flag;
		}
	}
	for(sig=SH_DEBUGTRAP; sig>=0; sig--)
	{
		if(trap=sh.st.trap[sig])
		{
			if(mode)
				free(trap);
			sh.st.trap[sig] = 0;
		}
	}
	if(sh.st.trapcom[0] && sh.st.trapcom[0] != Empty)
		free(sh.st.trapcom[0]);
	sh.st.trapcom[0] = 0;
	if(mode)
		sh.st.trapmax = 0;
	sh.trapnote=0;
}

/*
 * free up trap if set and restore signal handler if modified
 */
void	sh_sigclear(int sig)
{
	int flag = sh.sigflag[sig];
	char *trap;
	sh.st.otrapcom=0;
	if(!(flag&SH_SIGFAULT))
		return;
	flag &= ~(SH_SIGTRAP|SH_SIGSET);
	if(trap=sh.st.trapcom[sig])
	{
		if(!sh.subshell)
			free(trap);
		sh.st.trapcom[sig]=0;
	}
	sh.sigflag[sig] = flag;
}

/*
 * check for traps
 */
void	sh_chktrap(void)
{
	int	sig=sh.st.trapmax;
	char	*trap;
	if(sh.trapnote&SH_SIGALRM)
		sh_timetraps();
	if(!sh.trapnote)
		sig=0;
	sh.trapnote &= ~SH_SIGTRAP;
	/* execute errexit trap first */
	if(sh_isstate(SH_ERREXIT) && sh.exitval)
	{
		int	sav_trapnote = sh.trapnote;
		sh.trapnote &= ~SH_SIGSET;
		if(sh.st.trap[SH_ERRTRAP])
		{
			trap = sh.st.trap[SH_ERRTRAP];
			sh.st.trap[SH_ERRTRAP] = 0;
			sh_trap(trap,0);
			sh.st.trap[SH_ERRTRAP] = trap;
		}
		sh.trapnote = sav_trapnote;
		if(sh_isoption(SH_ERREXIT))
		{
			struct checkpt	*pp = (struct checkpt*)sh.jmplist;
			pp->mode = SH_JMPEXIT;
			sh_exit(sh.exitval);
		}
	}
#if SHOPT_BGX
	if((sh.sigflag[SIGCHLD]&SH_SIGTRAP) && sh.st.trapcom[SIGCHLD])
		job_chldtrap(1);
#endif /* SHOPT_BGX */
	while(--sig>=0)
	{
		if(sig==cursig)
			continue;
#if SHOPT_BGX
		if(sig==SIGCHLD)
			continue;
#endif /* SHOPT_BGX */
		if(sh.sigflag[sig]&SH_SIGTRAP)
		{
			sh.sigflag[sig] &= ~SH_SIGTRAP;
			if(trap=sh.st.trapcom[sig])
			{
				cursig = sig;
 				sh_trap(trap,0);
				cursig = -1;
				/* If we're in a PS2 prompt, then we just parsed and executed a trap in the middle of parsing
				 * another command, so the lexer state is overwritten. Escape to avoid crashing the lexer. */
				if(sh.nextprompt == 2)
				{
					fcclose();		/* force lexer to abort partial command */
					sh.nextprompt = 1;	/* next display prompt is PS1 */
					sh.lastsig = sig;	/* make sh_exit() set $? to signal exit status */
					sh_exit(SH_EXITSIG);	/* start a new command line */
				}
 			}
		}
	}
}


/*
 * parse and execute the given trap string, stream or tree depending on mode
 * mode==0 for string, mode==1 for stream, mode==2 for parse tree
 * The return value is the exit status of the trap action.
 */
int sh_trap(const char *trap, int mode)
{
	int	jmpval, savxit = sh.exitval, savxit_return;
	int	was_history = sh_isstate(SH_HISTORY);
	int	was_verbose = sh_isstate(SH_VERBOSE);
	char	was_no_trapdontexec = !sh.st.trapdontexec;
	char	save_chldexitsig = sh.chldexitsig;
	int	staktop = stktell(sh.stk);
	void	*savptr = stkfreeze(sh.stk,0);
	struct	checkpt buff;
	Fcin_t	savefc;
	fcsave(&savefc);
	sh_offstate(SH_HISTORY);
	sh_offstate(SH_VERBOSE);
	sh.intrap++;
	/* disable last-command exec optimisation so the caller gets to complete execution */
	if(was_no_trapdontexec)
		sh.st.trapdontexec = 's';  /* special value for direct sh_trap() call */
	sh_pushcontext(&buff,SH_JMPTRAP);
	jmpval = sigsetjmp(buff.buff,0);
	if(jmpval == 0)
	{
		if(mode==2)
			sh_exec((Shnode_t*)trap,sh_isstate(SH_ERREXIT));
		else
		{
			Sfio_t *sp;
			if(mode)
				sp = (Sfio_t*)trap;
			else
				sp = sfopen(NULL,trap,"s");
			sh_eval(sp,0);
		}
	}
	else if(indone)
	{
		if(jmpval==SH_JMPSCRIPT)
			indone=0;
		else
			jmpval=SH_JMPTRAP;
	}
	sh_popcontext(&buff);
	/* re-allow last-command exec optimisation unless the command we executed set a trap */
	if(was_no_trapdontexec && sh.st.trapdontexec=='s')
		sh.st.trapdontexec = 0;
	sh.intrap--;
	sfsync(sh.outpool);
	savxit_return = sh.exitval;
	if(sh.intrap_exit_n)
		sh.intrap_exit_n = 0;
	else
		sh.exitval = savxit;
	stkset(sh.stk,savptr,staktop);
	fcrestore(&savefc);
	if(was_history)
		sh_onstate(SH_HISTORY);
	if(was_verbose)
		sh_onstate(SH_VERBOSE);
	sh.chldexitsig = save_chldexitsig;
	exitset();
	if(jmpval>SH_JMPTRAP && (((struct checkpt*)sh.jmpbuffer)->prev || ((struct checkpt*)sh.jmpbuffer)->mode==SH_JMPSCRIPT))
		siglongjmp(*sh.jmplist,jmpval);
	return savxit_return;
}

/*
 * exit the current scope and jump to an earlier one based on pp->mode
 */
void sh_exit(int xno)
{
	struct checkpt	*pp = (struct checkpt*)sh.jmplist;
	int		sig=0;
	Sfio_t		*pool;
	/* POSIX requires exit status >= 2 for error in 'test'/'[' */
	if(xno==1 && sh.bltinfun==b_test)
		sh.exitval = 2;
	else
		sh.exitval = xno;
	if(xno==SH_EXITSIG)
		sh.exitval |= (sig=sh.lastsig);
	if(pp && pp->mode>1)
		cursig = -1;
	sh_offstate(SH_EXEC);
	if((sh.trapnote&SH_SIGTSTP) && job.jobcontrol)
	{
		/* ^Z detected by the shell */
		sh.trapnote = 0;
		sh.sigflag[SIGTSTP] = 0;
		if(!sh.subshell && sh_isstate(SH_MONITOR) && !sh_isstate(SH_STOPOK))
			return;
		if(sh_isstate(SH_TIMING))
			return;
		/* Handles ^Z for shell builtins, subshells, and functs */
		sh.lastsig = 0;
		sh_onstate(SH_MONITOR);
		sh_offstate(SH_STOPOK);
		sh.trapnote = 0;
		sh.forked = 1;
		if(sh_isstate(SH_INTERACTIVE) && (sig=sh_fork(0,NULL)))
		{
			job.curpgid = 0;
			job.parent = (pid_t)-1;
			job_wait(sig);
			sh.forked = 0;
			job.parent = 0;
			sh.sigflag[SIGTSTP] = 0;
			/* wait for child to stop */
			sh.exitval = (SH_EXITSIG|SIGTSTP);
			/* return to prompt mode */
			pp->mode = SH_JMPERREXIT;
		}
		else
		{
			if(sh.subshell)
				sh_subfork();
			/* script or child process; put to sleep */
			sh_offstate(SH_STOPOK);
			sh_offstate(SH_MONITOR);
			sh.sigflag[SIGTSTP] = 0;
			/* stop child job */
			killpg(job.curpgid,SIGTSTP);
			/* child resumes */
			job_clear();
			sh.exitval = (xno&SH_EXITMASK);
			return;
		}
	}
	/* unlock output pool */
	sh_offstate(SH_NOTRACK);
	if(!(pool=sfpool(NULL,sh.outpool,SFIO_WRITE)))
		pool = sh.outpool; /* can't happen? */
	sfclrlock(pool);
	if(sh.lastsig==SIGPIPE)
		sfpurge(pool);
	sfclrlock(sfstdin);
	if(!pp)
		sh_done(sig);
	sh.arithrecursion = 0;
	sh.intrace = 0;
	sh.prefix = 0;
	sh.mktype = 0;
	sh.invoc_local = 0;
	sh.tilde_block = 0;
	if(job.in_critical)
		job_unlock();
	if(pp->mode == SH_JMPSCRIPT && !pp->prev)
		sh_done(sig);
	if(pp->mode)
		siglongjmp(pp->buff,pp->mode);
}

static void array_notify(Namval_t *np, void *data)
{
	Namarr_t	*ap = nv_arrayptr(np);
	NOT_USED(data);
	if(ap && ap->fun)
		(*ap->fun)(np, 0, NV_AFREE);
}

/*
 * This is the exit routine for the shell
 */
noreturn void sh_done(int sig)
{
	char *t;
	int savxit = sh.exitval;
	unsigned char savlastsig = sh.lastsig;
	sh.trapnote = 0;
	indone=1;
	if(sig)
		savxit = SH_EXITSIG|sig;
	if(sh.userinit)
		(*sh.userinit)(&sh, -1);
	if(t=sh.st.trapcom[0])
	{
		sh.st.trapcom[0]=0; /* should free but not long */
		sh_trap(t,0);
		savxit = sh.exitval;
	}
	else
	{
		/* avoid recursive call for set -e */
		sh_offstate(SH_ERREXIT);
		sh_chktrap();
	}
	nv_scan(sh.var_tree,array_notify,NULL,NV_ARRAY,NV_ARRAY);
	sh_freeup();
#if SHOPT_ACCT
	sh_accend();
#endif	/* SHOPT_ACCT */
	if(mbwide() && sh_editor_active())
		tty_cooked(-1);
	if((sh_isoption(SH_INTERACTIVE) && sh_isoption(SH_LOGIN_SHELL)) || (!sh_isoption(SH_INTERACTIVE) && (sig==SIGHUP)))
		job_walk(sfstderr, job_hup, SIGHUP, NULL);
	job_close();
	sfsync((Sfio_t*)sfstdin);
	sfsync((Sfio_t*)sh.outpool);
	sfsync((Sfio_t*)sfstdout);
	if((sh.chldexitsig && sh.realsubshell) || (savxit&SH_EXITSIG && (savxit&SH_EXITMASK) == savlastsig))
		sig = savxit&SH_EXITMASK;
	if(sig)
	{
		/* generate fault termination code */
		if(RLIMIT_CORE!=RLIMIT_UNKNOWN)
		{
#if _lib_getrlimit
			struct rlimit rlp;
			getrlimit(RLIMIT_CORE,&rlp);
			rlp.rlim_cur = 0;
			setrlimit(RLIMIT_CORE,&rlp);
#else
			vlimit(RLIMIT_CORE,0);
#endif
		}
		signal(sig,SIG_DFL);
		sigrelease(sig);
		kill(sh.current_pid,sig);
		pause();
	}
#if SHOPT_KIA
	if(sh_isoption(SH_NOEXEC))
		kiaclose((Lex_t*)sh.lex_context);
#endif /* SHOPT_KIA */
	/* Exit with portable 8-bit status (128 + signum) if last child process exits due to signal */
	if(sh.chldexitsig)
		savxit = savxit & ~SH_EXITSIG | 0200;
	exit(savxit&SH_EXITMASK);
}

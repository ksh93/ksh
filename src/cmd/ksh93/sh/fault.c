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
/*
 * Fault handling routines
 *
 *   David Korn
 *   AT&T Labs
 *
 */

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

#if !_std_malloc
#   include	<vmalloc.h>
#endif
#if  defined(VMFL)
    /*
     * This exception handler is called after vmalloc() unlocks the region
     */
    static int malloc_done(Vmalloc_t* vm, int type, void* val, Vmdisc_t* dp)
    {
	dp->exceptf = 0;
	sh_exit(SH_EXITSIG);
	return(0);
    }
#endif

/*
 * Most signals caught or ignored by the shell come here
*/
void	sh_fault(register int sig)
{
	register int 		flag=0;
	register char		*trap;
	register struct checkpt	*pp = (struct checkpt*)sh.jmplist;
	int	action=0;
	/* reset handler */
	if(!(sig&SH_TRAP))
		signal(sig, sh_fault);
	sig &= ~SH_TRAP;
#ifdef SIGWINCH
	if(sig==SIGWINCH)
	{
		int rows=0, cols=0;
		int32_t v;
		astwinsize(2,&rows,&cols);
		if(v = cols)
			nv_putval(COLUMNS, (char*)&v, NV_INT32|NV_RDONLY);
		if(v = rows)
			nv_putval(LINES, (char*)&v, NV_INT32|NV_RDONLY);
		sh.winch++;
	}
#endif  /* SIGWINCH */
	trap = sh.st.trapcom[sig];
	if(sh.savesig)
	{
		/* critical region, save and process later */
		if(!(sh.sigflag[sig]&SH_SIGIGNORE))
			sh.savesig = sig;
		return;
	}
	if(sig==SIGALRM && sh.bltinfun==b_sleep)
	{
		if(trap && *trap)
		{
			sh.trapnote |= SH_SIGTRAP;
			sh.sigflag[sig] |= SH_SIGTRAP;
		}
		return;
	}
	if(sh.subshell && trap && sig!=SIGINT && sig!=SIGQUIT && sig!=SIGWINCH && sig!=SIGCONT)
	{
		sh.exitval = SH_EXITSIG|sig;
		sh_subfork();
		sh.exitval = 0;
		return;
	}
	/* handle ignored signals */
	if(trap && *trap==0)
		return;
	flag = sh.sigflag[sig]&~SH_SIGOFF;
	if(!trap)
	{
		if(sig==SIGINT && (sh.trapnote&SH_SIGIGNORE))
			return;
		if(flag&SH_SIGIGNORE)
		{
			if(sh.subshell)
				sh.ignsig = sig;
			sigrelease(sig);
			return;
		}
		if(flag&SH_SIGDONE)
		{
			void *ptr=0;
			if((flag&SH_SIGINTERACTIVE) && sh_isstate(SH_INTERACTIVE) && !sh_isstate(SH_FORKED) && ! sh.subshell)
			{
				/* check for TERM signal between fork/exec */
				if(sig==SIGTERM && job.in_critical)
					sh.trapnote |= SH_SIGTERM;
				return;
			}
			sh.lastsig = sig;
			sigrelease(sig);
			if(pp->mode != SH_JMPSUB)
			{
				if(pp->mode < SH_JMPSUB)
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
#if  defined(VMFL)
			if(abortsig(sig))
			{
				/* abort inside malloc, process when malloc returns */
				/* VMFL defined when using vmalloc() */
				Vmdisc_t* dp = vmdisc(Vmregion,0);
				if(dp)
					dp->exceptf = malloc_done;
			}
#endif
			return;
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
#ifdef SIGTSTP
		if(sig==SIGTSTP)
		{
			sh.trapnote |= SH_SIGTSTP;
			if(pp->mode==SH_JMPCMD && sh_isstate(SH_STOPOK))
			{
				sigrelease(sig);
				sh_exit(SH_EXITSIG);
				return;
			}
		}
#endif /* SIGTSTP */
	}
#ifdef ERROR_NOTIFY
	if((error_info.flags&ERROR_NOTIFY) && sh.bltinfun)
		action = (*sh.bltinfun)(-sig,(char**)0,(void*)0);
	if(action>0)
		return;
#endif
	if(sh.bltinfun && sh.bltindata.notify)
	{
		sh.bltindata.sigset = 1;
		return;
	}
	sh.trapnote |= flag;
	if(sig <= sh.sigmax)
		sh.sigflag[sig] |= flag;
	if(pp->mode==SH_JMPCMD && sh_isstate(SH_STOPOK))
	{
		if(action<0)
			return;
		sigrelease(sig);
		sh_exit(SH_EXITSIG);
	}
}

/*
 * initialize signal handling
 */
void sh_siginit(void)
{
	register int sig, n;
	register const struct shtable2	*tp = shtab_signals;
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
void	sh_sigtrap(register int sig)
{
	register int flag;
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
	register int 	flag, sig = sh.sigmax;
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
 */
void	sh_sigreset(register int mode)
{
	register char	*trap;
	register int 	flag, sig=sh.st.trapmax;
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
	sh.st.trapcom[0] = 0;
	if(mode)
		sh.st.trapmax = 0;
	sh.trapnote=0;
}

/*
 * free up trap if set and restore signal handler if modified
 */
void	sh_sigclear(register int sig)
{
	register int flag = sh.sigflag[sig];
	register char *trap;
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
	register int 	sig=sh.st.trapmax;
	register char *trap;
	if(!(sh.trapnote&~SH_SIGIGNORE))
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
	if(sh.sigflag[SIGALRM]&SH_SIGALRM)
		sh_timetraps();
#if SHOPT_BGX
	if((sh.sigflag[SIGCHLD]&SH_SIGTRAP) && sh.st.trapcom[SIGCHLD])
		job_chldtrap(sh.st.trapcom[SIGCHLD],1);
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
	int	staktop = staktell();
	char	*savptr = stakfreeze(0);
	struct	checkpt buff;
	Fcin_t	savefc;
	fcsave(&savefc);
	sh_offstate(SH_HISTORY);
	sh_offstate(SH_VERBOSE);
	sh.intrap++;
	sh_pushcontext(&sh,&buff,SH_JMPTRAP);
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
				sp = sfopen(NIL(Sfio_t*),trap,"s");
			sh_eval(sp,0);
		}
	}
	else if(indone)
	{
		if(jmpval==SH_JMPSCRIPT)
			indone=0;
		else
		{
			if(jmpval==SH_JMPEXIT)
				savxit = sh.exitval;
			jmpval=SH_JMPTRAP;
		}
	}
	sh_popcontext(&sh,&buff);
	sh.intrap--;
	sfsync(sh.outpool);
	savxit_return = sh.exitval;
	if(jmpval!=SH_JMPEXIT && jmpval!=SH_JMPFUN)
		sh.exitval=savxit;
	stakset(savptr,staktop);
	fcrestore(&savefc);
	if(was_history)
		sh_onstate(SH_HISTORY);
	if(was_verbose)
		sh_onstate(SH_VERBOSE);
	exitset();
	if(jmpval>SH_JMPTRAP && (((struct checkpt*)sh.jmpbuffer)->prev || ((struct checkpt*)sh.jmpbuffer)->mode==SH_JMPSCRIPT))
		siglongjmp(*sh.jmplist,jmpval);
	return(savxit_return);
}

/*
 * exit the current scope and jump to an earlier one based on pp->mode
 */
void sh_exit(register int xno)
{
	register struct checkpt	*pp = (struct checkpt*)sh.jmplist;
	register int		sig=0;
	register Sfio_t*	pool;
	/* POSIX requires exit status >= 2 for error in 'test'/'[' */
	if(xno == 1 && (sh.bltindata.bnode==SYSTEST || sh.bltindata.bnode==SYSBRACKET))
		sh.exitval = 2;
	else
		sh.exitval = xno;
	if(xno==SH_EXITSIG)
		sh.exitval |= (sig=sh.lastsig);
	if(pp && pp->mode>1)
		cursig = -1;
#ifdef SIGTSTP
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
		if(sh_isstate(SH_INTERACTIVE) && (sig=sh_fork(0,NIL(int*))))
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
#endif /* SIGTSTP */
	/* unlock output pool */
	sh_offstate(SH_NOTRACK);
	if(!(pool=sfpool(NIL(Sfio_t*),sh.outpool,SF_WRITE)))
		pool = sh.outpool; /* can't happen? */
	sfclrlock(pool);
#ifdef SIGPIPE
	if(sh.lastsig==SIGPIPE)
		sfpurge(pool);
#endif /* SIGPIPE */
	sfclrlock(sfstdin);
	if(!pp)
		sh_done(sig);
	sh.arithrecursion = 0;
	sh.prefix = 0;
#if SHOPT_TYPEDEF
	sh.mktype = 0;
#endif /* SHOPT_TYPEDEF */
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

noreturn void sh_done(register int sig)
{
	register char *t;
	register int savxit = sh.exitval;
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
	nv_scan(sh.var_tree,array_notify,(void*)0,NV_ARRAY,NV_ARRAY);
	sh_freeup();
#if SHOPT_ACCT
	sh_accend();
#endif	/* SHOPT_ACCT */
#if SHOPT_VSH || SHOPT_ESH
	if(mbwide()
#if SHOPT_ESH
	|| sh_isoption(SH_EMACS)
	|| sh_isoption(SH_GMACS)
#endif
#if SHOPT_VSH
	|| sh_isoption(SH_VI)
#endif
	)
		tty_cooked(-1);
#endif /* SHOPT_VSH || SHOPT_ESH */
#ifdef JOBS
	if((sh_isoption(SH_INTERACTIVE) && sh.login_sh) || (!sh_isoption(SH_INTERACTIVE) && (sig==SIGHUP)))
		job_walk(sfstderr, job_hup, SIGHUP, NIL(char**));
#endif	/* JOBS */
	job_close();
	if(nv_search("VMTRACE", sh.var_tree,0))
		strmatch((char*)0,(char*)0);
	sfsync((Sfio_t*)sfstdin);
	sfsync((Sfio_t*)sh.outpool);
	sfsync((Sfio_t*)sfstdout);
	if(savxit&SH_EXITSIG && (savxit&SH_EXITMASK) == sh.lastsig)
		sig = savxit&SH_EXITMASK;
	if(sig)
	{
		/* generate fault termination code */
		if(RLIMIT_CORE!=RLIMIT_UNKNOWN)
		{
#ifdef _lib_getrlimit
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
	if (savxit & SH_EXITSIG)
		savxit = savxit & ~SH_EXITSIG | 0200;

	exit(savxit&SH_EXITMASK);
}

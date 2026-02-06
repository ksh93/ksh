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
*               Vincent Mihalkovic <vmihalko@redhat.com>               *
*                                                                      *
***********************************************************************/
/*
 *  Job control for UNIX Shell
 *
 *   David Korn
 *   AT&T Labs
 *
 *  Written October, 1982
 *  Rewritten April, 1988
 *  Revised January, 1992
 *  Mended February, 2021
 *  Corrected March, 2025
 *
 *  Aspects of job control are (de)activated using a global flag variable,
 *  a state bit, and a shell option bit. It is important to understand the
 *  difference and set/check them in a manner consistent with their purpose.
 *
 *  1. The job.jobcontrol flag is for job control on interactive shells.
 *     It is set to nonzero by job_init() if, and only if, the shell is
 *     interactive *and* managed to get control of the terminal. Therefore,
 *     any changing of terminal settings (tcsetpgrp(3), tty_set()) should
 *     only be done if job.jobcontrol is nonzero.
 *
 *  2. The state flag, sh_isstate(SH_MONITOR), determines whether the bits
 *     of job control that are relevant for both scripts and interactive
 *     shells are active, which is mostly making sure that a background job
 *     gets its own process group (setpgid(3)).
 *
 *  3. The -m (-o monitor) shell option, sh_isoption(SH_MONITOR), is just
 *     that. When the user turns it on or off, the state flag is synched
 *     with it. It should usually not be directly checked for, as the state
 *     may be temporarily turned off without turning off the option.
 */

#include	"shopt.h"
#include	"defs.h"
#include	<wait.h>
#include	"io.h"
#include	"jobs.h"
#include	"history.h"

#if !defined(WCONTINUED) || !defined(WIFCONTINUED)
#   undef  WCONTINUED
#   define WCONTINUED	0
#   undef  WIFCONTINUED
#   define WIFCONTINUED(wstat)	(0)
#endif

#define NJOB_SAVELIST	4

/*
 * temporary hack to get W* macros to work
 */
#undef wait
#define wait    ______wait
/*
 * This struct saves a link list of processes that have non-zero exit
 * status, have had $! saved, but haven't been waited for
 */
struct jobsave
{
	struct jobsave	*next;
	pid_t		pid;
	unsigned short	exitval;
};

static struct jobsave *job_savelist;
static int njob_savelist;
static struct process *pwfg;
static int jobfork;

pid_t	pid_fromstring(char *str)
{
	pid_t	pid;
	char	*last;
	errno = 0;
	if(sizeof(pid)==sizeof(Sflong_t))
		pid = (pid_t)strtoll(str, &last, 10);
	else
		pid = (pid_t)strtol(str, &last, 10);
	if(errno==ERANGE || *last)
	{
		errormsg(SH_DICT,ERROR_exit(1),"%s: invalid process ID",str);
		UNREACHABLE();
	}
	return pid;
}

static void init_savelist(void)
{
	struct jobsave *jp;
	while(njob_savelist < NJOB_SAVELIST)
	{
		jp = sh_newof(0,struct jobsave,1,0);
		jp->next = job_savelist;
		job_savelist = jp;
		njob_savelist++;
	}
}

struct back_save
{
	int		count;
	struct jobsave	*list;
	struct back_save *prev;
};

#define BYTE(n)		(((n)+CHAR_BIT-1)/CHAR_BIT)
#define MAXMSG	25
#define SH_STOPSIG	(SH_EXITSIG<<1)

#ifdef VSUSP
#   ifndef CNSUSP
#	ifdef _POSIX_VDISABLE
#	   define CNSUSP	_POSIX_VDISABLE
#	else
#	   define CNSUSP	0
#	endif /* _POSIX_VDISABLE */
#   endif /* CNSUSP */
#   ifndef CSWTCH
#	ifdef CSUSP
#	    define CSWTCH	CSUSP
#	else
#	    define CSWTCH	('z'&037)
#	endif /* CSUSP */
#   endif /* CSWTCH */
#endif /* VSUSP */

/* Process states */
#define P_EXITSAVE	01
#define P_STOPPED	02
#define P_NOTIFY	04
#define P_SIGNALLED	010
#define P_STTY		020
#define P_DONE		040
#define P_COREDUMP	0100
#define P_DISOWN	0200
#define P_MOVED2FG	0400	/* set if the process was moved to the foreground by job_switch() */
#define P_BG		01000	/* set if the process is running in the background */

static int		job_chksave(pid_t);
static struct process	*job_bypid(pid_t);
static struct process	*job_byjid(int);
static char		*job_sigmsg(int);
static int		job_alloc(void);
static void		job_free(int);
static struct process	*job_unpost(struct process*,int);
static void		job_unlink(struct process*);
static void		job_prmsg(struct process*);
static struct process	*freelist;
static char		beenhere;
static char		possible;
static struct process	dummy;
static char		by_number;
static Sfio_t		*outfile;
static pid_t		lastpid;
static struct back_save	bck;

static void		job_set(struct process*);
static void		job_reset(struct process*);
static void		job_waitsafe(int);
static struct process	*job_byname(char*);
static struct process	*job_bystring(char*);
static struct termios	my_stty;  /* terminal state for shell */
static char		*job_string;

    static void		job_unstop(struct process*, int);
    static void		job_fgrp(struct process*, int);

typedef int (*Waitevent_f)(int,long,int);

#if SHOPT_BGX
void job_chldtrap(int unpost)
{
	struct process *pw,*pwnext;
	pid_t bckpid;
	int oldexit,trapnote;
	job_lock();
	sh.sigflag[SIGCHLD] &= ~SH_SIGTRAP;
	trapnote = sh.trapnote;
	sh.trapnote = 0;
	for(pw=job.pwlist;pw;pw=pwnext)
	{
		pwnext = pw->p_nxtjob;
		if((pw->p_flag&(P_BG|P_DONE)) != (P_BG|P_DONE))
			continue;
		pw->p_flag &= ~P_BG;
		bckpid = sh.bckpid;
		oldexit = sh.savexit;
		sh.bckpid = pw->p_pid;
		sh.savexit = pw->p_exit;
		if(pw->p_flag&P_SIGNALLED)
			sh.savexit |= SH_EXITSIG;
		/* The trap handler for SIGCHLD may change after sh_trap() because of
		   'trap - CHLD', so it's obtained for each iteration of the loop. */
		sh_trap(sh.st.trapcom[SIGCHLD],0);
		if(pw->p_pid==bckpid && unpost)
			job_unpost(pw,0);
		sh.savexit = oldexit;
		sh.bckpid = bckpid;
	}
	sh.trapnote = trapnote;
	job_unlock();
}
#endif /* SHOPT_BGX */

/*
 * return next on link list of jobsave free list
 */
static struct jobsave *jobsave_create(pid_t pid)
{
	struct jobsave *jp = job_savelist;
	job_chksave(pid);
	if(++bck.count > sh.lim.child_max)
		job_chksave(0);
	if(jp)
	{
		njob_savelist--;
		job_savelist = jp->next;
	}
	else
		jp = sh_newof(0,struct jobsave,1,0);
	if(jp)
	{
		jp->pid = pid;
		jp->next = bck.list;
		bck.list = jp;
		jp->exitval = 0;
	}
	return jp;
}

/*
 * Reap one job
 * When called with sig==0, it does a blocking wait
 */
int job_reap(int sig)
{
	pid_t pid;
	struct process *pw = NULL;
	struct process *px;
	int flags;
	struct jobsave *jp;
	int nochild = 0, oerrno = errno, wstat;
	Waitevent_f waitevent = sh.waitevent;
	static int wcontinued = WCONTINUED;
#ifdef DEBUG
	if(sfprintf(sfstderr,"ksh: job line %4d: reap PID=%jd critical=%d signal=%d\n",__LINE__,(Sflong_t)sh.current_pid,job.in_critical,sig) <=0)
		write(2,"waitsafe\n",9);
	sfsync(sfstderr);
#endif /* DEBUG */
	job.savesig = 0;
	if(sig)
		flags = WNOHANG|WUNTRACED|wcontinued;
	else
		flags = WUNTRACED|wcontinued;
	sh.waitevent = 0;
	while(1)
	{
		if(!(flags&WNOHANG) && !sh.intrap && job.pwlist)
		{
			if(waitevent && (*waitevent)(-1,-1L,0))
				flags |= WNOHANG;
		}
		while(1)
		{
			pid = waitpid((pid_t)-1,&wstat,flags);
			/* some systems (Linux 2.6) may return EINVAL when there are no continued children */
			if (pid<0 && errno==EINVAL && (flags&WCONTINUED))
				pid = waitpid((pid_t)-1,&wstat,flags&=~WCONTINUED);
			/* run any alarm traps triggered while waiting */
			if (pid<0 && errno==EINTR && (sh.trapnote&SH_SIGALRM))
			{
				sh_timetraps();
				continue;
			}
			break;
		}
		sh_sigcheck();
		if(pid<0)
		{
			if(errno==EINTR && (sig||job.savesig))
			{
				errno = 0;
				continue;
			}
			if(errno==ECHILD)
			{
#if SHOPT_BGX
				job.numbjob = 0;
#endif /* SHOPT_BGX */
				nochild = 1;
			}
		}
		if(pid<=0)
			break;
		if(wstat==0)
			job_chksave(pid);
		flags |= WNOHANG;
		job.waitsafe++;
		jp = 0;
		lastpid = pid;
		if(!(pw=job_bypid(pid)))
		{
#ifdef DEBUG
			sfprintf(sfstderr,"ksh: job line %4d: reap PID=%jd critical=%d unknown job PID=%jd pw=%x\n",__LINE__,(Sflong_t)sh.current_pid,job.in_critical,(Sflong_t)pid,pw);
#endif /* DEBUG */
			if (WIFCONTINUED(wstat) && wcontinued)
				continue;
			pw = &dummy;
			pw->p_exit = 0;
			pw->p_pgrp = 0;
			pw->p_exitmin = 0;
			if(job.toclear)
				job_clear();
			jp = jobsave_create(pid);
			pw->p_flag = 0;
			lastpid = pw->p_pid = pid;
			px = 0;
			if(jp && WIFSTOPPED(wstat))
			{
				jp->exitval = SH_STOPSIG;
				continue;
			}
		}
		else
			px=job_byjid(pw->p_job);
		if (WIFCONTINUED(wstat) && wcontinued)
			pw->p_flag &= ~(P_NOTIFY|P_SIGNALLED|P_STOPPED);
		else if(WIFSTOPPED(wstat))
		{
			pw->p_flag |= (P_NOTIFY|P_SIGNALLED|P_STOPPED|P_BG);
			pw->p_exit = WSTOPSIG(wstat);
			if(pw->p_pgrp && pw->p_pgrp==job.curpgid && sh_isstate(SH_STOPOK))
				kill(sh.current_pid,pw->p_exit);
			if(px)
			{
				/* move to top of job list */
				job_unlink(px);
				px->p_nxtjob = job.pwlist;
				job.pwlist = px;
			}
			continue;
		}
		else
		{
			/* check for coprocess completion */
			if(pid==sh.cpid)
			{
				if(sh.coutpipe > -1)
					sh_close(sh.coutpipe);
				if(sh.cpipe[1] > -1)
					sh_close(sh.cpipe[1]);
				sh.coutpipe = sh.cpipe[1] = -1;
			}
			else if(sh.subshell)
				sh_subjobcheck(pid);

			pw->p_flag &= ~(P_STOPPED|P_SIGNALLED);
			pw->p_flag |= P_DONE;
			if(pw->p_flag&P_BG)
				pw->p_flag |= P_NOTIFY;
			if (WIFSIGNALED(wstat))
			{
				pw->p_flag |= P_SIGNALLED;
				if (WTERMCORE(wstat))
					pw->p_flag |= P_COREDUMP;
				pw->p_exit = WTERMSIG(wstat);
				/* if process in current jobs terminates from
				 * an interrupt, propagate to parent shell
				 */
				if(pw->p_pgrp && pw->p_pgrp==job.curpgid && pw->p_exit==SIGINT && sh_isstate(SH_STOPOK))
				{
					pw->p_flag &= ~P_NOTIFY;
					sh_offstate(SH_STOPOK);
					kill(sh.current_pid,SIGINT);
					sh_onstate(SH_STOPOK);
				}
			}
			else
			{
				pw->p_exit =  pw->p_exitmin;
				if(WEXITSTATUS(wstat) > pw->p_exitmin)
					pw->p_exit = WEXITSTATUS(wstat);
			}
#if SHOPT_BGX
			if(pw->p_flag&P_BG)
			{
				job.numbjob--;
				if(sh.st.trapcom[SIGCHLD])
				{
					sh.sigflag[SIGCHLD] |= SH_SIGTRAP;
					if(sig==0)
						job_chldtrap(0);
					else
						sh.trapnote |= SH_SIGTRAP;
				}
				else
					pw->p_flag &= ~P_BG;
			}
#endif /* SHOPT_BGX */
			if(pw->p_pgrp==0)
				pw->p_flag &= ~P_NOTIFY;
		}
		if(jp && pw== &dummy)
		{
			jp->exitval = pw->p_exit;
			if(pw->p_flag&P_SIGNALLED)
				jp->exitval |= SH_EXITSIG;
		}
#ifdef DEBUG
		sfprintf(sfstderr,"ksh: job line %4d: reap PID=%jd critical=%d job %d with PID %jd flags=%o complete with status=%x exit=%d\n",__LINE__,(Sflong_t)sh.current_pid,job.in_critical,pw->p_job,(Sflong_t)pid,pw->p_flag,wstat,pw->p_exit);
		sfsync(sfstderr);
#endif /* DEBUG */
		/* only top-level process in job should have notify set */
		if(px && pw != px)
			pw->p_flag &= ~P_NOTIFY;
		if(job.jobcontrol && pid==pw->p_fgrp && pid==tcgetpgrp(JOBTTY))
		{
			px = job_byjid(pw->p_job);
			for(; px && (px->p_flag&P_DONE); px=px->p_nxtproc);
			if(!px)
				tcsetpgrp(JOBTTY,job.mypid);
		}
#if !SHOPT_BGX
		if(!sh.intrap && sh.st.trapcom[SIGCHLD] && pid>0 && (pwfg!=job_bypid(pid)))
		{
			sh.sigflag[SIGCHLD] |= SH_SIGTRAP;
			sh.trapnote |= SH_SIGTRAP;
		}
#endif /* !SHOPT_BGX */
		/* Handle -b/--notify while waiting for command line input */
		if(sh_isoption(SH_NOTIFY) && sh_isstate(SH_TTYWAIT))
		{
			if(sh_editor_active())
			{
				/* Buffer the notification for ed_read() to show */
				if(!sh.notifybuf)
					sh.notifybuf = sfstropen();
				outfile = sh.notifybuf;
				job_list(pw,JOB_NFLAG);
				sh.winch = 1;
			}
			else
			{
				outfile = sfstderr;
				job_list(pw,JOB_NFLAG|JOB_NLFLAG);
				sfsync(sfstderr);
			}
			job_unpost(pw,1);
		}
	}
	sh.waitevent = waitevent;
	if(sig)
		signal(sig, job_waitsafe);
	/*
	 * Always restore errno, because this code is run during signal handling which may interrupt loops like:
	 *	while((fd = open(path, flags, mode)) < 0)
	 *		if(errno!=EINTR)
	 *			<throw error>;
	 * otherwise that may fail if SIGCHLD is handled between the open() call and the errno!=EINTR check.
	 */
	errno = oerrno;
	return nochild;
}

/*
 * This is the SIGCHLD interrupt routine
 */
static void job_waitsafe(int sig)
{
	if(job.in_critical)
	{
		job.savesig = sig;
		job.waitsafe++;
	}
	else
		job_reap(sig);
}

/*
 * initialize job control if possible
 */
void job_init(void)
{
	int ntry=0;
	job.fd = JOBTTY;
	signal(SIGCHLD,job_waitsafe);
	if(njob_savelist < NJOB_SAVELIST)
		init_savelist();
	if(!sh_isoption(SH_INTERACTIVE))
		return;
	job.mypgid = getpgrp();
	/* some systems have job control, but not initialized */
	if(job.mypgid<=0)
	{
		/* Get a controlling terminal and set process group */
		/* This should have already been done by rlogin */
		int fd;
		char *ttynam;
		if(job.mypgid<0 || !(ttynam=ttyname(JOBTTY)))
			return;
		ast_close(JOBTTY);
		if((fd = open(ttynam,O_RDWR)) <0)
			return;
		if(fd!=JOBTTY)
			sh_iorenumber(fd,JOBTTY);
		tcsetpgrp(JOBTTY,sh.pid);
		job.mypgid = sh.pid;
	}
	possible = (setpgid(0,job.mypgid) >= 0) || errno==EPERM;
	if(possible)
	{
		/* wait until we are in the foreground */
		while((job.mytgid=tcgetpgrp(JOBTTY)) != job.mypgid)
		{
			if(job.mytgid <= 0)
				return;
			/* Stop this shell until continued */
			signal(SIGTTIN,SIG_DFL);
			kill(sh.pid,SIGTTIN);
			/* resumes here after continue tries again */
			if(ntry++ > IOMAXTRY)
			{
				errormsg(SH_DICT,0,e_no_start);
				return;
			}
		}
	}
	else
		return;
	/* make sure that we are a process group leader */
	setpgid(0,sh.pid);
	job.mypid = sh.pid;
#if defined(SA_NOCLDSTOP) || defined(SA_NOCLDWAIT)
#   	if !defined(SA_NOCLDSTOP)
#	    define SA_NOCLDSTOP	0
#   	endif
#   	if !defined(SA_NOCLDWAIT)
#	    define SA_NOCLDWAIT	0
#   	endif
	sigflag(SIGCHLD, SA_NOCLDSTOP|SA_NOCLDWAIT, 0);
#endif /* SA_NOCLDSTOP || SA_NOCLDWAIT */
	signal(SIGTTIN,SIG_IGN);
	signal(SIGTTOU,SIG_IGN);
	/* The shell now handles ^Z */
	signal(SIGTSTP,sh_fault);
	tcsetpgrp(JOBTTY,sh.pid);
#ifdef CNSUSP
	/* set the switch character */
	tty_get(JOBTTY,&my_stty);
	job.suspend = (unsigned)my_stty.c_cc[VSUSP];
	if(job.suspend == (unsigned char)CNSUSP)
	{
		my_stty.c_cc[VSUSP] = CSWTCH;
		tty_set(JOBTTY,TCSAFLUSH,&my_stty);
	}
#endif /* CNSUSP */
	sh_onoption(SH_MONITOR);
	job.jobcontrol++;
	return;
}


/*
 * see if there are any stopped jobs
 * restore tty driver and pgrp
 */
int job_close(void)
{
	struct process *pw;
	int count = 0, running = 0;
	if(possible && !job.jobcontrol)
		return 0;
	else if(!possible && (!sh_isstate(SH_MONITOR) || sh_isstate(SH_FORKED)))
		return 0;
	else if(sh.current_pid != job.mypid)
		return 0;
	job_lock();
	if(!tty_check(0))
		beenhere++;
	for(pw=job.pwlist;pw;pw=pw->p_nxtjob)
	{
		if(!(pw->p_flag&P_STOPPED))
		{
			if(!(pw->p_flag&P_DONE))
				running++;
			continue;
		}
		if(beenhere)
			killpg(pw->p_pgrp,SIGTERM);
		count++;
	}
	if(beenhere++ == 0 && job.pwlist)
	{
		if(count)
		{
			errormsg(SH_DICT,0,e_terminate);
			return -1;
		}
		else if(running && sh_isoption(SH_LOGIN_SHELL))
		{
			errormsg(SH_DICT,0,e_jobsrunning);
			return -1;
		}
	}
	job_unlock();
	if(job.jobcontrol && setpgid(0,job.mypgid)>=0)
		tcsetpgrp(job.fd,job.mypgid);
#   ifdef CNSUSP
	if(possible && job.suspend==CNSUSP)
	{
		tty_get(job.fd,&my_stty);
		my_stty.c_cc[VSUSP] = CNSUSP;
		tty_set(job.fd,TCSAFLUSH,&my_stty);
	}
#   endif /* CNSUSP */
	job.jobcontrol = 0;
	return 0;
}

static void job_set(struct process *pw)
{
	if(!job.jobcontrol)
		return;
	/* save current terminal state */
	tty_get(job.fd,&my_stty);
	if(pw->p_flag&P_STTY)
	{
		/* restore terminal state for job */
		tty_set(job.fd,TCSAFLUSH,&pw->p_stty);
	}
	if((pw->p_flag&P_STOPPED) || tcgetpgrp(job.fd) == sh.pid)
		tcsetpgrp(job.fd,pw->p_fgrp);
	/* if job is stopped, resume it in the background */
	if(!sh.forked)
		job_unstop(pw,1);
	sh.forked = 0;
}

static void job_reset(struct process *pw)
{
	/* save the terminal state for current job */
	pid_t tgrp;
	if(!job.jobcontrol)
		return;
	if((tgrp=tcgetpgrp(job.fd))!=job.mypid)
		job_fgrp(pw,tgrp);
	if(tcsetpgrp(job.fd,job.mypid) !=0)
		return;
	/* force the following tty_get() to do a tcgetattr() unless fg */
	if(!(pw->p_flag&P_MOVED2FG))
		tty_set(-1, 0, NULL);
	if(pw && (pw->p_flag&P_SIGNALLED) && pw->p_exit!=SIGHUP)
	{
		if(tty_get(job.fd,&pw->p_stty) == 0)
			pw->p_flag |= P_STTY;
		/* restore terminal state for job */
		tty_set(job.fd,TCSAFLUSH,&my_stty);
	}
	beenhere = 0;
}

/*
 * wait built-in command
 */
void job_bwait(char **jobs)
{
	char *jp;
	struct process *pw;
	pid_t pid;
	if(*jobs==0)
		job_wait((pid_t)-1);
	else while(jp = *jobs++)
	{
		if(*jp == '%')
		{
			job_lock();
			pw = job_bystring(jp);
			job_unlock();
			if(pw)
				pid = pw->p_pid;
			else
				return;
		}
		else
			pid = pid_fromstring(jp);
		job_wait(-pid);
	}
}

/*
 * execute function <fun> for each job
 */
int job_walk(Sfio_t *file,int (*fun)(struct process*,int),int arg,char *joblist[])
{
	struct process *pw;
	int r = 0;
	char *jobid, **jobs=joblist;
	struct process *px;
	job_string = 0;
	outfile = file;
	by_number = 0;
	job_lock();
	pw = job.pwlist;
	job_waitsafe(SIGCHLD);
	if(jobs==0)
	{
		/* do all jobs */
		for(;pw;pw=px)
		{
			px = pw->p_nxtjob;
			if(pw->p_env != sh.jobenv)
				continue;
			if((*fun)(pw,arg))
				r = 2;
		}
	}
	else if(*jobs==0)	/* current job */
	{
		/* skip over non-stop jobs */
		while(pw && (pw->p_env!=sh.jobenv || pw->p_pgrp==0))
			pw = pw->p_nxtjob;
		if((*fun)(pw,arg))
			r = 2;
	}
	else while(jobid = *jobs++)
	{
		job_string = jobid;
		if(*jobid==0)
		{
			errormsg(SH_DICT,ERROR_exit(1),e_jobusage,job_string);
			UNREACHABLE();
		}
		if(*jobid == '%')
			pw = job_bystring(jobid);
		else
		{
			int pid = pid_fromstring(jobid);
			if(!(pw = job_bypid(pid)))
			{
				pw = &dummy;
				pw->p_pid = pid;
				pw->p_pgrp = pid;
			}
			by_number = 1;
		}
		if((*fun)(pw,arg))
			r = 2;
		by_number = 0;
	}
	job_unlock();
	return r;
}

/*
 * list the given job
 * flag JOB_LFLAG for long listing
 * flag JOB_NFLAG to list only jobs marked for notification
 * flag JOB_NLFLAG to print an initial newline
 * flag JOB_PFLAG for process ID(s) only
 */
int job_list(struct process *pw,int flag)
{
	struct process *px = pw;
	int  n;
	const char *msg;
	int msize;
	if(!pw || pw->p_job<=0)
		return 1;
	if(pw->p_env != sh.jobenv)
		return 0;
	if((flag&JOB_NFLAG) && (!(px->p_flag&P_NOTIFY)||px->p_pgrp==0))
		return 0;
	if((flag&JOB_PFLAG))
	{
		sfprintf(outfile,"%jd\n",(Sflong_t)(px->p_pgrp?px->p_pgrp:px->p_pid));
		return 0;
	}
	if((px->p_flag&P_DONE) && job.waitall && !(flag&JOB_LFLAG))
		return 0;
	job_lock();
	n = px->p_job;
	if(px==job.pwlist)
		msize = '+';
	else if(px==job.pwlist->p_nxtjob)
		msize = '-';
	else
		msize = ' ';
	if(flag&JOB_NLFLAG)
		sfputc(outfile,'\n');
	sfprintf(outfile,"[%d] %c ",n, msize);
	do
	{
		n = 0;
		if(flag&JOB_LFLAG)
			sfprintf(outfile,"%jd\t",(Sflong_t)px->p_pid);
		if(px->p_flag&P_SIGNALLED)
			msg = job_sigmsg((int)(px->p_exit));
		else if(px->p_flag&P_NOTIFY)
		{
			msg = sh_translate(e_done);
			n = px->p_exit;
		}
		else
			msg = sh_translate(e_running);
		px->p_flag &= ~P_NOTIFY;
		sfputr(outfile,msg,-1);
		msize = strlen(msg);
		if(n)
		{
			sfprintf(outfile,"(%d)",n);
			msize += (3+(n>10)+(n>100));
		}
		if(px->p_flag&P_COREDUMP)
		{
			msg = sh_translate(e_coredump);
			sfputr(outfile, msg, -1);
			msize += strlen(msg);
		}
		sfnputc(outfile,' ',MAXMSG>msize?MAXMSG-msize:1);
		if(flag&JOB_LFLAG)
			px = px->p_nxtproc;
		else
		{
			while(px=px->p_nxtproc)
				px->p_flag &= ~P_NOTIFY;
			px = 0;
		}
		if(!px)
			hist_list(sh.hist_ptr,outfile,pw->p_name,0,";");
		else
			sfputr(outfile, e_nlspace, -1);
	}
	while(px);
	job_unlock();
	return 0;
}

/*
 * get the process group given the job number
 * This routine returns the process group number or -1
 */
static struct process *job_bystring(char *ajob)
{
	struct process *pw=job.pwlist;
	int c;
	if(*ajob++ != '%' || !pw)
		return NULL;
	c = *ajob;
	if(isdigit(c))
		pw = job_byjid((int)strtol(ajob, NULL, 10));
	else if(c=='+' || c=='%')
		;
	else if(c=='-')
	{
		if(pw)
			pw = job.pwlist->p_nxtjob;
	}
	else
		pw = job_byname(ajob);
	if(pw && pw->p_flag)
		return pw;
	return NULL;
}

/*
 * Helper function for job_kill().
 * sh.1: "If the signal being sent is TERM (terminate) or HUP (hangup), then
 * the job or process will be sent a CONT (continue) signal if it is stopped."
 * As this is not specified anywhere in POSIX, this is disabled for POSIX mode.
 */
static int also_send_sigcont(struct process *pw,int sig)
{
	return !sh_isoption(SH_POSIX) && (sig==SIGHUP || sig==SIGTERM) && pw && (pw->p_flag & P_STOPPED);
}

/*
 * Kill a job or process
 */
int job_kill(struct process *pw,int sig)
{
	pid_t pid;
	int r = -1;
	const char *msg;
	job_lock();
	errno = ECHILD;
	if(!pw)
		goto error;  /* not an actual shell job */
	pid = pw->p_pid;
	if(by_number)
	{
		if(pid==0 && job.jobcontrol)
			r = job_walk(outfile, job_kill,sig, NULL);
		if(sig==SIGSTOP && pid==sh.pid && sh_isoption(SH_LOGIN_SHELL))
		{
			/* can't stop login shell */
			errno = EPERM;
			r = -1;
		}
		else if(pid>=0)
		{
			r = kill(pid,sig);
			if(r>=0)
			{
				if(also_send_sigcont(pw,sig))
					kill(pid,sig = SIGCONT);
				if(sig==SIGCONT && (pw->p_flag&P_STOPPED))
					pw->p_flag &= ~(P_STOPPED|P_SIGNALLED|P_NOTIFY);
			}
		}
		else
		{
			pid = -pid;
			pw = job_bypid(pid);
			r = killpg(pid,sig);
			if(r>=0)
			{
				if(sig==SIGCONT)
					job_unstop(pw,0);
				else if(also_send_sigcont(pw,sig))
					job_unstop(pw,1);
			}
		}
	}
	else
	{
		if(pid = pw->p_pgrp)
		{
			r = killpg(pid,sig);
			if(r>=0)
			{
				if(sig==SIGCONT)
					job_unstop(pw,0);
				else if(also_send_sigcont(pw,sig))
					job_unstop(pw,1);
				sh_delay(.05,0);
			}
		}
		while(pw && pw->p_pgrp==0 && (r=kill(pw->p_pid,sig))>=0)
		{
			if(also_send_sigcont(pw,sig))
			{
				kill(pw->p_pid,SIGCONT);
				pw->p_flag &= ~(P_STOPPED|P_SIGNALLED|P_NOTIFY);
			}
			pw = pw->p_nxtproc;
		}
	}
	if(r<0 && job_string)
	{
	error:
		if(pw && by_number)
			msg = sh_translate(e_no_proc);
		else
			msg = sh_translate(e_no_job);
		if(errno == EPERM)
			msg = sh_translate(e_access);
		sfprintf(sfstderr,"kill: %s: %s\n",job_string, msg);
		r = 2;
	}
	sh_delay(.001,0);
	job_unlock();
	return r;
}

/*
 * Kill process group with SIGHUP when the session is being disconnected.
 * (As this is called via job_walk(), it must accept the 'sig' argument.)
 */
int job_hup(struct process *pw, int sig)
{
	struct process	*px;
	NOT_USED(sig);
	if(pw->p_pgrp == 0 || (pw->p_flag & P_DISOWN))
		return 0;
	job_lock();
	/*
	 * Only kill process group if we still have at least one process. If all the processes are P_DONE,
	 * then our process group is already gone and its p_pgrp may now be used by an unrelated process.
	 */
	for(px = pw; px; px = px->p_nxtproc)
	{
		if(!(px->p_flag & P_DONE))
		{
			if(killpg(pw->p_pgrp, SIGHUP) >= 0)
				job_unstop(pw,1);
			break;
		}
	}
	job_unlock();
	return 0;
}

/*
 * Get process structure from first letters of jobname
 */
static struct process *job_byname(char *name)
{
	struct process *pw = job.pwlist;
	struct process *pz = 0;
	int *flag = 0;
	char *cp = name;
	int offset;
	if(!sh.hist_ptr)
		return NULL;
	if(*cp=='?')
		cp++,flag= &offset;
	{
		if(hist_match(sh.hist_ptr,pw->p_name,cp,flag)>=0)
		{
			if(pz)
			{
				errormsg(SH_DICT,ERROR_exit(1),e_jobusage,name-1);
				UNREACHABLE();
			}
			pz = pw;
		}
	}
	return pz;
}

/*
 * Initialize the process posting array
 */
void	job_clear(void)
{
	struct process *pw, *px;
	struct process *pwnext;
	int j = BYTE(sh.lim.child_max);
	struct jobsave *jp,*jpnext;
	job_lock();
	for(pw=job.pwlist; pw; pw=pwnext)
	{
		pwnext = pw->p_nxtjob;
		while(px=pw)
		{
			pw = pw->p_nxtproc;
			free(px);
		}
	}
	for(jp=bck.list; jp;jp=jpnext)
	{
		jpnext = jp->next;
		free(jp);
	}
	bck.list = 0;
	if(njob_savelist < NJOB_SAVELIST)
		init_savelist();
	job.pwlist = NULL;
	job.numpost=0;
#if SHOPT_BGX
	job.numbjob = 0;
#endif /* SHOPT_BGX */
	job.waitall = 0;
	job.curpgid = 0;
	job.toclear = 0;
	if(!job.freejobs)
		job.freejobs = (unsigned char*)sh_malloc((unsigned)(j+1));
	while(j >=0)
		job.freejobs[j--]  = 0;
	job_unlock();
}

/*
 * Put the process <pid> on the process list and return the job number.
 * If <join> is 1, the job is marked as a background job (P_BG);
 * otherwise, if non-zero, <join> is the process ID of the job to join.
 */
int job_post(pid_t pid, pid_t join)
{
	struct process *pw;
	History_t *hp = sh.hist_ptr;
	int val;
	char bg = 0;
	sh.jobenv = sh.curenv;
	if(job.toclear)
	{
		job_clear();
		return 0;
	}
	job_lock();
	if(join==1)
	{
		join = 0;
		bg++;
#if SHOPT_BGX
		job.numbjob++;
#endif /* SHOPT_BGX */
	}
	if(njob_savelist < NJOB_SAVELIST)
		init_savelist();
	if(pw = job_bypid(pid))
		job_unpost(pw,0);
	if(join)
	{
		if(pw=job_bypid(join))
			val = pw->p_job;
		else
			val = job.curjobid;
		/* if job to join is not first move it to front */
		if(val && (pw=job_byjid(val)) != job.pwlist)
		{
			job_unlink(pw);
			pw->p_nxtjob = job.pwlist;
			job.pwlist = pw;
		}
	}
	if(pw=freelist)
		freelist = pw->p_nxtjob;
	else
		pw = new_of(struct process,0);
	pw->p_flag = 0;
	job.numpost++;
	if(join && job.pwlist)
	{
		/* join existing current job */
		pw->p_nxtjob = job.pwlist->p_nxtjob;
		pw->p_nxtproc = job.pwlist;
		pw->p_job = job.pwlist->p_job;
	}
	else
	{
		/* create a new job */
		while((pw->p_job = job_alloc()) < 0)
			job_wait((pid_t)1);
		pw->p_nxtjob = job.pwlist;
		pw->p_nxtproc = 0;
	}
	pw->p_exitval = job.exitval;
	job.pwlist = pw;
	pw->p_env = sh.curenv;
	pw->p_pid = pid;
	if(!sh.outpipe || sh.cpid==pid)
		pw->p_flag = P_EXITSAVE;
	pw->p_exitmin = sh.xargexit;
	pw->p_exit = 0;
	if(sh_isstate(SH_MONITOR))
	{
		if(killpg(job.curpgid,0)<0 && errno==ESRCH)
			job.curpgid = pid;
		pw->p_fgrp = job.curpgid;
	}
	else
		pw->p_fgrp = 0;
	pw->p_pgrp = pw->p_fgrp;
#ifdef DEBUG
	sfprintf(sfstderr,"ksh: job line %4d: post PID=%jd critical=%d job=%d PID=%jd PGID=%jd savesig=%d join=%d\n",__LINE__,(Sflong_t)sh.current_pid,job.in_critical,pw->p_job,
		(Sflong_t)pw->p_pid,(Sflong_t)pw->p_pgrp,job.savesig,join);
	sfsync(sfstderr);
#endif /* DEBUG */
	if(hp && !sh_isstate(SH_PROFILE) && !sh.realsubshell)
		pw->p_name=hist_tell(sh.hist_ptr,(int)hp->histind-1);
	else
		pw->p_name = -1;
	if ((val = job_chksave(pid))>=0 && !jobfork)
	{
		pw->p_exit = val;
		if(pw->p_exit==SH_STOPSIG)
		{
			pw->p_flag |= (P_SIGNALLED|P_STOPPED);
			pw->p_exit = 0;
		}
		else if(pw->p_exit >= SH_EXITSIG)
		{
			pw->p_flag |= P_DONE|P_SIGNALLED;
			pw->p_exit &= SH_EXITMASK;
		}
		else
			pw->p_flag |= (P_DONE|P_NOTIFY);
	}
	if(bg)
	{
		if(!(pw->p_flag&P_DONE))
			pw->p_flag |= P_BG;
#if SHOPT_BGX
		else
			job.numbjob--;
#endif /* SHOPT_BGX */
	}
	lastpid = 0;
	job_unlock();
	return pw->p_job;
}

/*
 * Returns a process structure give a process ID
 */
static struct process *job_bypid(pid_t pid)
{
	struct process  *pw, *px;
	for(pw=job.pwlist; pw; pw=pw->p_nxtjob)
		for(px=pw; px; px=px->p_nxtproc)
		{
			if(px->p_pid==pid)
				return px;
		}
	return NULL;
}

/*
 * return a pointer to a job given the job ID
 */
static struct process *job_byjid(int jobid)
{
	struct process *pw;
	for(pw=job.pwlist;pw; pw = pw->p_nxtjob)
	{
		if(pw->p_job==jobid)
			break;
	}
	return pw;
}

/*
 * print a signal message
 */
static void job_prmsg(struct process *pw)
{
	if(pw->p_exit!=SIGINT && pw->p_exit!=SIGPIPE)
	{
		const char *msg, *dump;
		msg = job_sigmsg((int)(pw->p_exit));
		msg = sh_translate(msg);
		if(pw->p_flag&P_COREDUMP)
			dump =  sh_translate(e_coredump);
		else
			dump = "";
		if(sh_isstate(SH_INTERACTIVE))
			sfprintf(sfstderr,"%s%s\n",msg,dump);
		else
			errormsg(SH_DICT,2,"%jd: %s%s",(Sflong_t)pw->p_pid,msg,dump);
	}
}

/*
 * Wait for process to complete
 * If pid < -1, then wait can be interrupted, -pid is waited for (wait builtin)
 * pid=0 to unpost all done processes
 * pid=1 to wait for at least one process to complete
 * pid=-1 to wait for all running processes
 */
int	job_wait(pid_t pid)
{
	struct process	*pw=0,*px;
	int		jobid = 0;
	int		nochild = 1;
	char		intr = 0;
	if(pid < 0)
	{
		pid = -pid;
		intr = 1;
	}
	job_lock();
	if(pid==0)
	{
		if(!job.waitall || !job.curjobid || !(pw = job_byjid(job.curjobid)))
		{
			job_unlock();
			goto done;
		}
		jobid = pw->p_job;
		job.curjobid = 0;
		if(!(pw->p_flag&(P_DONE|P_STOPPED)))
			job_reap(job.savesig);
	}
	if(pid > 1)
	{
		if(pid==sh.spid)
			sh.spid = 0;
		if(!(pw=job_bypid(pid)))
		{
			/* check to see whether job status has been saved */
			if((sh.exitval = job_chksave(pid)) < 0)
				sh.exitval = ERROR_NOENT;
			exitset();
			job_unlock();
			return nochild;
		}
		else if(intr && pw->p_env!=sh.curenv)
		{
			sh.exitval = ERROR_NOENT;
			job_unlock();
			return nochild;
		}
		jobid = pw->p_job;
		if(!intr)
			pw->p_flag &= ~P_EXITSAVE;
		if(pw->p_pgrp && job.parent!= (pid_t)-1)
			job_set(job_byjid(jobid));
	}
	pwfg = pw;
#ifdef DEBUG
	sfprintf(sfstderr,"ksh: job line %4d: wait PID=%jd critical=%d job=%d PID=%jd\n",__LINE__,(Sflong_t)sh.current_pid,job.in_critical,jobid,(Sflong_t)pid);
	if(pw)
		sfprintf(sfstderr,"ksh: job line %4d: wait PID=%jd critical=%d flags=%o\n",__LINE__,(Sflong_t)sh.current_pid,job.in_critical,pw->p_flag);
#endif /* DEBUG */
	errno = 0;
	if(sh.coutpipe>=0 && lastpid && sh.cpid==lastpid)
	{
		sh_close(sh.coutpipe);
		sh_close(sh.cpipe[1]);
		sh.cpipe[1] = sh.coutpipe = -1;
	}
	while(1)
	{
		if(job.waitsafe)
		{
			for(px=job.pwlist;px; px = px->p_nxtjob)
			{
				if(px!=pw && (px->p_flag&P_NOTIFY))
				{
					if(sh_isoption(SH_NOTIFY))
					{
						outfile = sfstderr;
						job_list(px,JOB_NFLAG|JOB_NLFLAG);
						sfsync(sfstderr);
					}
					else if(!sh_isoption(SH_INTERACTIVE) && (px->p_flag&P_SIGNALLED))
					{
						job_prmsg(px);
						px->p_flag &= ~P_NOTIFY;
					}
				}
			}
		}
		if(pw && (pw->p_flag&(P_DONE|P_STOPPED)))
		{
			if(pw->p_flag&P_STOPPED)
			{
				pw->p_flag |= P_EXITSAVE;
				if(job.jobcontrol && !sh_isstate(SH_FORKED))
				{
					if( pw->p_exit!=SIGTTIN && pw->p_exit!=SIGTTOU)
						break;

					tcsetpgrp(JOBTTY,pw->p_pgrp);
					killpg(pw->p_pgrp,SIGCONT);
				}
				else /* ignore stop when non-interactive */
					pw->p_flag &= ~(P_NOTIFY|P_SIGNALLED|P_STOPPED|P_EXITSAVE);
			}
			else
			{
				if(pw->p_flag&P_SIGNALLED)
				{
					pw->p_flag &= ~P_NOTIFY;
					job_prmsg(pw);
				}
				else if(pw->p_flag&P_DONE)
					pw->p_flag &= ~P_NOTIFY;
				if(pw->p_job==jobid)
				{
					px = job_byjid(jobid);
					/* last process in job */
					if(px!=pw)
						px = 0;
					if(px)
					{
						sh.exitval=px->p_exit;
						if(px->p_flag&P_SIGNALLED)
						{
							sh.exitval |= SH_EXITSIG;
							sh.chldexitsig = 1;
						}
						if(intr)
							px->p_flag &= ~P_EXITSAVE;
					}
				}
				px = job_unpost(pw,1);
				if(!px || !job.waitall)
					break;
				pw = px;
				continue;
			}
		}
		sfsync(sfstderr);
		job.waitsafe = 0;
		nochild = job_reap(job.savesig);
		if(job.waitsafe)
			continue;
		if(nochild)
			break;
		if((intr && sh.trapnote) || (pid==1 && !intr))
			break;
	}
	if(intr && sh.trapnote)
		sh.exitval = 1;
	pwfg = 0;
	job_unlock();
	if(pid==1)
		return nochild;
	exitset();
	if(pid==0)
		goto done;
	if(pw->p_pgrp)
	{
		job_reset(pw);
		/* propagate keyboard interrupts to parent */
		if((pw->p_flag&P_SIGNALLED) && pw->p_exit==SIGINT && !(sh.sigflag[SIGINT]&SH_SIGOFF))
			kill(sh.current_pid,SIGINT);
		else if((pw->p_flag&P_STOPPED) && pw->p_exit==SIGTSTP)
		{
			job.parent = 0;
			kill(sh.current_pid,SIGTSTP);
		}
	}
	else
	{
		if(job.jobcontrol && pw->p_pid == tcgetpgrp(JOBTTY))
		{
			if(pw->p_pgrp==0)
				pw->p_pgrp = pw->p_pid;
			job_reset(pw);
		}
		tty_set(-1, 0, NULL);
	}
done:
	if(!job.waitall && sh_isoption(SH_PIPEFAIL))
		return nochild;
	if(!sh.intrap)
	{
		job_lock();
		for(pw=job.pwlist; pw; pw=px)
		{
			px = pw->p_nxtjob;
			job_unpost(pw,0);
		}
		job_unlock();
	}
	return nochild;
}

/*
 * move job to foreground if bgflag == 'f'
 * move job to background if bgflag == 'b'
 * disown job if bgflag == 'd'
 */
int job_switch(struct process *pw,int bgflag)
{
	const char *msg;
	job_lock();
	if(!pw || !(pw=job_byjid(pw->p_job)))
	{
		job_unlock();
		return 1;
	}
	if(bgflag=='d')
	{
		for(; pw; pw=pw->p_nxtproc)
			pw->p_flag |= P_DISOWN;
		job_unlock();
		return 0;
	}
	if(bgflag=='b')
	{
		sfprintf(outfile,"[%d]\t",pw->p_job);
		sh.bckpid = pw->p_pid;
		pw->p_flag |= P_BG;
		msg = "&";
	}
	else
	{
		job_unlink(pw);
		pw->p_nxtjob = job.pwlist;
		job.pwlist = pw;
		msg = "";
	}
	hist_list(sh.hist_ptr,outfile,pw->p_name,'&',";");
	sfputr(outfile,msg,'\n');
	sfsync(outfile);
	if(bgflag=='f')
	{
		if(!(pw=job_unpost(pw,1)))
		{
			job_unlock();
			return 1;
		}
		job.waitall = 1;
		pw->p_flag |= P_MOVED2FG;
		pw->p_flag &= ~P_BG;
		job_wait(pw->p_pid);
		job.waitall = 0;
	}
	else if(pw->p_flag&P_STOPPED)
		job_unstop(pw,1);
	job_unlock();
	return 0;
}

/*
 * Set the foreground group associated with a job
 */
static void job_fgrp(struct process *pw, int newgrp)
{
	for(; pw; pw=pw->p_nxtproc)
		pw->p_fgrp = newgrp;
}

/*
 * turn off STOP state of a process group and send CONT signals
 */
static void job_unstop(struct process *px, int send_sigcont)
{
	struct process *pw;
	int num = 0;
	for(pw=px ;pw ;pw=pw->p_nxtproc)
	{
		if(pw->p_flag&P_STOPPED)
		{
			num++;
			pw->p_flag &= ~(P_STOPPED|P_SIGNALLED|P_NOTIFY);
		}
	}
	if(num && send_sigcont)
	{
		if(px->p_fgrp != px->p_pgrp)
			killpg(px->p_fgrp,SIGCONT);
		killpg(px->p_pgrp,SIGCONT);
	}
}

/*
 * remove a job from table
 * If all the processes have not completed, unpost first non-completed process
 * Otherwise the job is removed and job_unpost returns NULL.
 * pwlist is reset if the first job is removed
 * if <notify> is non-zero, then jobs with pending notifications are unposted
 */
static struct process *job_unpost(struct process *pwtop,int notify)
{
	struct process *pw;
	/* make sure all processes are done */
#ifdef DEBUG
	sfprintf(sfstderr,"ksh: job line %4d: drop PID=%jd critical=%d PID=%jd env=%u\n",__LINE__,(Sflong_t)sh.current_pid,job.in_critical,(Sflong_t)pwtop->p_pid,pwtop->p_env);
	sfsync(sfstderr);
#endif /* DEBUG */
	pwtop = pw = job_byjid(pwtop->p_job);
	if(!pw)
		return NULL;
#if SHOPT_BGX
	if(pw->p_flag&P_BG)
		return pw;
#endif /* SHOPT_BGX */
	for(; pw && (pw->p_flag&P_DONE)&&(notify||!(pw->p_flag&P_NOTIFY)||pw->p_env); pw=pw->p_nxtproc);
	if(pw)
		return pw;
	if(pwtop->p_job == job.curjobid)
		return NULL;
	/* all processes complete, unpost job */
	job_unlink(pwtop);
	for(pw=pwtop; pw; pw=pw->p_nxtproc)
	{
		/* save the exit status for the pipefail option */
		if(pw && pw->p_exitval)
		{
			*pw->p_exitval = pw->p_exit;
			if(pw->p_flag&P_SIGNALLED)
				*pw->p_exitval |= SH_EXITSIG;
		}
		/* save the exit status for background jobs */
		if((pw->p_flag&P_EXITSAVE) ||  pw->p_pid==sh.spid)
		{
			struct jobsave *jp;
			/* save status for future wait */
			if(jp = jobsave_create(pw->p_pid))
			{
				jp->exitval = pw->p_exit;
				if(pw->p_flag&P_SIGNALLED)
					jp->exitval |= SH_EXITSIG;
			}
			pw->p_flag &= ~P_EXITSAVE;
		}
		pw->p_flag &= ~P_DONE;
		job.numpost--;
		pw->p_nxtjob = freelist;
		freelist = pw;
	}
	pwtop->p_pid = 0;
#ifdef DEBUG
	sfprintf(sfstderr,"ksh: job line %4d: free PID=%jd critical=%d job=%d\n",__LINE__,(Sflong_t)sh.current_pid,job.in_critical,pwtop->p_job);
	sfsync(sfstderr);
#endif /* DEBUG */
	job_free(pwtop->p_job);
	return NULL;
}

/*
 * unlink a job form the job list
 */
static void job_unlink(struct process *pw)
{
	struct process *px;
	if(pw==job.pwlist)
	{
		job.pwlist = pw->p_nxtjob;
		job.curpgid = 0;
		return;
	}
	for(px=job.pwlist;px;px=px->p_nxtjob)
		if(px->p_nxtjob == pw)
		{
			px->p_nxtjob = pw->p_nxtjob;
			return;
		}
}

/*
 * get an unused job number
 * freejobs is a bit vector, 0 is unused
 */
static int job_alloc(void)
{
	int j=0;
	unsigned mask = 1;
	unsigned char *freeword;
	int jmax = BYTE(sh.lim.child_max);
	/* skip to first word with a free slot */
	for(j=0;job.freejobs[j] == UCHAR_MAX; j++);
	if(j >= jmax)
	{
		struct process *pw;
		for(j=1; j < sh.lim.child_max; j++)
		{
			if((pw=job_byjid(j))&& !job_unpost(pw,0))
				break;
		}
		j /= CHAR_BIT;
		if(j >= jmax)
			return -1;
	}
	freeword = &job.freejobs[j];
	j *= CHAR_BIT;
	for(j++;mask&(*freeword);j++,mask <<=1);
	*freeword  |= mask;
	return j;
}

/*
 * return a job number
 */
static void job_free(int n)
{
	int j = (--n)/CHAR_BIT;
	unsigned mask;
	n -= j*CHAR_BIT;
	mask = 1 << n;
	job.freejobs[j]  &= ~mask;
}

static char *job_sigmsg(int sig)
{
	static char signo[40];
	if(sig<=sh.sigmax && sh.sigmsg[sig])
		return sh.sigmsg[sig];
#if defined(SIGRTMIN) && defined(SIGRTMAX)
	if(sig>=sh.sigruntime[SH_SIGRTMIN] && sig<=sh.sigruntime[SH_SIGRTMAX])
	{
		static char sigrt[20];
		if(sig>sh.sigruntime[SH_SIGRTMIN]+(sh.sigruntime[SH_SIGRTMAX]-sig<=sh.sigruntime[SH_SIGRTMIN])/2)
			sfsprintf(sigrt,sizeof(sigrt),"SIGRTMAX-%d",sh.sigruntime[SH_SIGRTMAX]-sig);
		else
			sfsprintf(sigrt,sizeof(sigrt),"SIGRTMIN+%d",sig-sh.sigruntime[SH_SIGRTMIN]);
		return sigrt;
	}
#endif
	sfsprintf(signo,sizeof(signo),sh_translate(e_signo),sig);
	return signo;
}

/*
 * see whether exit status has been saved and delete it
 * if pid==0, then oldest saved process is deleted
 * If pid is not found a -1 is returned.
 */
static int job_chksave(pid_t pid)
{
	struct jobsave *jp = bck.list, *jpold=0;
	int r= -1;
	int count=bck.count;
	struct back_save *bp= &bck;
again:
	while(jp && count-->0)
	{
		if(jp->pid==pid)
			break;
		if(pid==0 && !jp->next)
			break;
		jpold = jp;
		jp = jp->next;
	}
	if(!jp && pid && (bp=bp->prev))
	{
		count = bp->count;
		jp = bp->list;
		jpold = 0;
		goto again;
	}
	if(jp)
	{
		r = 0;
		if(pid)
			r = jp->exitval;
		if(jpold)
			jpold->next = jp->next;
		else
			bp->list = jp->next;
		bp->count--;
		if(njob_savelist < NJOB_SAVELIST)
		{
			njob_savelist++;
			jp->next = job_savelist;
			job_savelist = jp;
		}
		else
			free(jp);
	}
	return r;
}

void *job_subsave(void)
{
	struct back_save *bp = new_of(struct back_save,0);
	job_lock();
	*bp = bck;
	bp->prev = bck.prev;
	bck.count = 0;
	bck.list = 0;
	bck.prev = bp;
	job_unlock();
	return bp;
}

void job_subrestore(void* ptr)
{
	struct jobsave *jp;
	struct back_save *bp = (struct back_save*)ptr;
	struct process *pw, *px, *pwnext;
	struct jobsave *end=NULL;
	job_lock();
	for(jp=bck.list; jp; jp=jp->next)
	{
		if (!jp->next)
			end = jp;
	}
	if(end)
		end->next = bp->list;
	else
		bck.list = bp->list;
	bck.count += bp->count;
	bck.prev = bp->prev;
	while(bck.count > sh.lim.child_max)
		job_chksave(0);
	for(pw=job.pwlist; pw; pw=pwnext)
	{
		pwnext = pw->p_nxtjob;
		if(pw->p_env != sh.curenv || pw->p_pid==sh.pipepid)
			continue;
		for(px=pw; px; px=px->p_nxtproc)
			px->p_flag |= P_DONE;
		job_unpost(pw,0);
	}

	free(bp);
	job_unlock();
}

void job_fork(pid_t parent)
{
#ifdef DEBUG
	sfprintf(sfstderr,"ksh: job line %4d: fork PID=%jd critical=%d parent=%d\n",__LINE__,(Sflong_t)sh.current_pid,job.in_critical,parent);
#endif /* DEBUG */
	switch (parent)
	{
	case -1:
		job_lock();
		jobfork++;
		break;
	case -2:
		jobfork--;
		job_unlock();
		break;
	case 0:
		jobfork=0;
		job_unlock();
		job.waitsafe = 0;
		job.in_critical = 0;
		break;
	default:
		job_chksave(parent);
		jobfork=0;
		job_unlock();
		break;
	}
}

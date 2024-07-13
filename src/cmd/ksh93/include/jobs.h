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
*               Anuradha Weeraman <anuradha@debian.org>                *
*               Vincent Mihalkovic <vmihalko@redhat.com>               *
*                                                                      *
***********************************************************************/
#ifndef JOB_NFLAG
/*
 *	Interface to job control for shell
 *	written by David Korn
 *
 */

#define JOBTTY	2

#include	<ast.h>
#include	<sfio.h>
#ifndef SIGINT
#   include	<signal.h>
#endif /* !SIGINT */
#include	<aso.h>
#include	"terminal.h"

#ifndef SIGCHLD
#   error ksh 93u+m requires SIGCHLD
#endif
#ifdef FIOLOOKLD
	/* Ninth edition */
	extern int tty_ld, ntty_ld;
#	define OTTYDISC	tty_ld
#	define NTTYDISC	ntty_ld
#endif /* FIOLOOKLD */

struct process
{
	struct process *p_nxtjob;	/* next job structure */
	struct process *p_nxtproc;	/* next process in current job */
	int		*p_exitval;	/* place to store the exitval */
	pid_t		p_pid;		/* process ID */
	pid_t		p_pgrp;		/* process group */
	pid_t		p_fgrp;		/* process group when stopped */
	int		p_job;		/* job number of process */
	unsigned short	p_exit;		/* exit value or signal number */
	unsigned short	p_exitmin;	/* minimum exit value for xargs */
	unsigned short	p_flag;		/* flags - see below */
	unsigned int	p_env;		/* subshell environment number */
	off_t		p_name;		/* history file offset for command */
	struct termios	p_stty;		/* terminal state for job */
};

struct jobs
{
	struct process	*pwlist;	/* head of process list */
	int		*exitval;	/* pipe exit values */
	pid_t		curpgid;	/* current process GID */
	pid_t		parent;		/* set by fork() */
	pid_t		mypid;		/* process ID of shell */
	pid_t		mypgid;		/* process group ID of shell */
	pid_t		mytgid;		/* terminal group ID of shell */
	int		curjobid;
	unsigned int	in_critical;	/* >0 => in critical region */
	int		savesig;	/* active signal */
	int		numpost;	/* number of posted jobs */
#if SHOPT_BGX
	int		numbjob;	/* number of background jobs */
#endif /* SHOPT_BGX */
	short		fd;		/* tty descriptor number */
	int		suspend;	/* suspend character */
	int		linedisc;	/* line discipline */
	char		jobcontrol;	/* turned on for interactive shell with control of terminal */
	char		waitsafe;	/* wait will not block */
	char		waitall;	/* wait for all jobs in pipe */
	char		toclear;	/* job table needs clearing */
	unsigned char	*freejobs;	/* free jobs numbers */
};

/* flags for joblist */
#define JOB_LFLAG	1
#define JOB_NFLAG	2
#define JOB_PFLAG	4
#define JOB_NLFLAG	8

extern struct jobs job;

#if !_std_malloc
#include <vmalloc.h>
#ifdef vmlocked
#define vmbusy()	vmlocked(Vmregion)
#else
#define vmbusy()	(vmstat(0,0)!=0)
#endif
#else
#define vmbusy()	0
#endif

#define job_lock()	asoincint(&job.in_critical)
#define job_unlock()	\
	do { \
		int	_sig; \
		if (asogetint(&job.in_critical) == 1 && (_sig = job.savesig) && !vmbusy()) \
		    job_reap(_sig); \
		asodecint(&job.in_critical); \
	} while(0)

extern const char	e_jobusage[];
extern const char	e_done[];
extern const char	e_running[];
extern const char	e_coredump[];
extern const char	e_no_proc[];
extern const char	e_no_job[];
extern const char	e_jobsrunning[];
extern const char	e_nlspace[];
extern const char	e_access[];
extern const char	e_terminate[];
extern const char	e_no_jctl[];
extern const char	e_signo[];
extern const char	e_no_start[];
#ifdef NTTYDISC
   extern const char	e_newtty[];
   extern const char	e_oldtty[];
#endif /* NTTYDISC */

/*
 * The following are defined in jobs.c
 */

extern void	job_clear(void);
extern void	job_bwait(char**);
extern int	job_walk(Sfio_t*,int(*)(struct process*,int),int,char*[]);
extern int	job_kill(struct process*,int);
extern int	job_wait(pid_t);
extern int	job_post(pid_t,pid_t);
extern void	*job_subsave(void);
extern void	job_subrestore(void*);
#if SHOPT_BGX
extern void	job_chldtrap(int);
#endif /* SHOPT_BGX */
extern void	job_init(int);
extern int	job_close(void);
extern int	job_list(struct process*,int);
extern int	job_hup(struct process *, int);
extern int	job_switch(struct process*,int);
extern void	job_fork(pid_t);
extern int	job_reap(int);

#endif /* !JOB_NFLAG */

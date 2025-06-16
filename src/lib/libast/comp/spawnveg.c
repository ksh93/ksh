/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2024 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  David Korn <dgk@research.att.com>                   *
*                   Phong Vo <kpv@research.att.com>                    *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/

/*
 * spawnveg -- spawnve with process group or session control
 *
 *	pgid	<0	setsid()	[session group leader]
 *		 0	nothing		[retain session and process group]
 *		 1	setpgid(0,0)	[process group leader]
 *		>1	setpgid(0,pgid)	[join process group]
 */

#include <ast.h>
#include <error.h>
#include <wait.h>
#include <sig.h>
#include <ast_tty.h>
#include <ast_fcntl.h>

#if _lib_posix_spawn > 1	/* reports underlying exec() errors */
#define _fast_spawnveg 1

#include <spawn.h>

static pid_t
spawnveg_fast(const char* path, char* const argv[], char* const envv[], pid_t pgid, int tcfd)
{
	int				err;
	short				flags = 0;
	pid_t				pid;
	posix_spawnattr_t		attr;
	sigset_t			mask, tcmask;
#if _lib_posix_spawn_file_actions_addtcsetpgrp_np
	posix_spawn_file_actions_t	actions;
#else
	NOT_USED(tcfd);
#endif

	if (err = posix_spawnattr_init(&attr))
		goto nope;
#ifdef POSIX_SPAWN_SETSID
	if (pgid == -1)
		flags |= POSIX_SPAWN_SETSID;
#endif
	if (pgid && pgid != -1)
		flags |= POSIX_SPAWN_SETPGROUP;
	if (tcfd >= 0)
		flags |= POSIX_SPAWN_SETSIGDEF;
	if (flags && (err = posix_spawnattr_setflags(&attr, flags)))
		goto bad;
	if (pgid && pgid != -1)
	{
		if (pgid <= 1)
			pgid = 0;
		if (err = posix_spawnattr_setpgroup(&attr, pgid))
			goto bad;
	}
#if _lib_posix_spawn_file_actions_addtcsetpgrp_np
	if (tcfd >= 0)
	{
		/* set the terminal signals to SIG_DFL in the child */
		sigemptyset(&tcmask);
		sigaddset(&tcmask, SIGTTIN);
		sigaddset(&tcmask, SIGTTOU);
		sigaddset(&tcmask, SIGTSTP);
		if (err = posix_spawnattr_setsigdefault(&attr, &tcmask))
			goto bad;
		/* set the child's terminal process group */
		if (err = posix_spawn_file_actions_init(&actions))
			goto bad;
		if (err = posix_spawn_file_actions_addtcsetpgrp_np(&actions, tcfd))
			goto fail;
	}
	/* spawn the process to run the given command */
	if (err = posix_spawn(&pid, path, (tcfd >= 0) ? &actions : NULL, &attr, argv, envv ? envv : environ))
#else
	if (err = posix_spawn(&pid, path, NULL, &attr, argv, envv ? envv : environ))
#endif
	{
		if ((err != EPERM) || (err = posix_spawn(&pid, path, NULL, NULL, argv, envv ? envv : environ)))
			goto fail;
	}
#if _lib_posix_spawn_file_actions_addtcsetpgrp_np
	if (tcfd >= 0)
		posix_spawn_file_actions_destroy(&actions);
#endif
	posix_spawnattr_destroy(&attr);
	return pid;
	/* cleanup for different fail states */
 fail:
#if _lib_posix_spawn_file_actions_addtcsetpgrp_np
	if (tcfd >= 0)
		posix_spawn_file_actions_destroy(&actions);
#endif
 bad:
	posix_spawnattr_destroy(&attr);
 nope:
	errno = err;
	return -1;
}

#elif _lib_spawn_mode
#define _fast_spawnveg 1

#include <process.h>

#ifndef P_NOWAIT
#define P_NOWAIT	_P_NOWAIT
#endif
#if !defined(P_DETACH) && defined(_P_DETACH)
#define P_DETACH	_P_DETACH
#endif

static pid_t
spawnveg_fast(const char* path, char* const argv[], char* const envv[], pid_t pgid, int tcfd)
{
	NOT_USED(tcfd);
#if defined(P_DETACH)
	return spawnve(pgid ? P_DETACH : P_NOWAIT, path, argv, envv ? envv : environ);
#else
	return spawnve(P_NOWAIT, path, argv, envv ? envv : environ);
#endif
}

#elif _lib_spawn && _hdr_spawn && _mem_pgroup_inheritance
#define _fast_spawnveg 1

#include <spawn.h>

/*
 * MVS OpenEdition / z/OS fork+exec+(setpgid)
 */

static pid_t
spawnveg_fast(const char* path, char* const argv[], char* const envv[], pid_t pgid, int tcfd)
{
	struct inheritance	inherit;

	NOT_USED(tcfd);
	inherit.flags = 0;
	if (pgid)
	{
		inherit.flags |= SPAWN_SETGROUP;
		inherit.pgroup = (pgid > 1) ? pgid : SPAWN_NEWPGROUP;
	}
	return spawn(path, 0, NULL, &inherit, (const char**)argv, (const char**)envv);
}

#else
#define _fast_spawnveg 0
#endif  /* _lib_posix_spawn */

#if _lib_spawnve && _hdr_process
#include <process.h>
#if defined(P_NOWAIT) || defined(_P_NOWAIT)
#undef	_lib_spawnve
#endif
#endif

#if _lib_pipe2 && O_cloexec
#define pipe(a)  pipe2(a,O_cloexec)
#endif

/*
 * fork+exec+(setsid|setpgid)
 */

static pid_t
spawnveg_slow(const char* path, char* const argv[], char* const envv[], pid_t pgid, int tcfd)
{
	int			n;
	int			m;
	pid_t			pid;
	pid_t			rid;
	int			err[2];

	if (!envv)
		envv = environ;
#if _lib_spawnve
	if (!pgid && tcfd < 0)
		return spawnve(path, argv, envv);
#endif /* _lib_spawnve */
	n = errno;
	if (pipe(err) < 0)
		err[0] = -1;
#if !(_lib_pipe2 && O_cloexec)
	else
	{
		fcntl(err[0], F_SETFD, FD_CLOEXEC);
		fcntl(err[1], F_SETFD, FD_CLOEXEC);
	}
#endif
	sigcritical(SIG_REG_EXEC|SIG_REG_PROC|(tcfd>=0?SIG_REG_TERM:0));
	pid = fork();
	if (pid == -1)
		n = errno;
	else if (!pid)
	{
		int ret;
		sigcritical(0);
		if (pgid == -1)
			setsid();
		else if (pgid)
		{
			if (pgid <= 1)
				pgid = getpid();
			if (setpgid(0, pgid) < 0 && errno == EPERM)
				setpgid(pgid, 0);
		}
		if (tcfd >= 0)
		{
			if(pgid == -1)
				pgid = getpid();
			tcsetpgrp(tcfd, pgid);
			signal(SIGTTIN,SIG_DFL);
			signal(SIGTTOU,SIG_DFL);
			signal(SIGTSTP,SIG_DFL);
		}
		execve(path, argv, envv);
		if (err[0] != -1)
		{
			m = errno;
			write(err[1], &m, sizeof(m));
		}
		if(errno == ENOENT)
			ret = EXIT_NOTFOUND;
#ifdef ENAMETOOLONG
		else if(errno == ENAMETOOLONG)
			ret = EXIT_NOTFOUND;
#endif
		else
			ret = EXIT_NOEXEC;
		_exit(ret);
	}
	rid = pid;
	if (err[0] != -1)
	{
		close(err[1]);
		if (pid != -1)
		{
			m = 0;
			while (read(err[0], &m, sizeof(m)) == -1)
				if (errno != EINTR)
				{
					m = errno;
					break;
				}
			if (m)
			{
				while (waitpid(pid, &n, 0) && errno == EINTR);
				rid = pid = -1;
				n = m;
			}
		}
		close(err[0]);
	}
	sigcritical(0);
	if (pid != -1 && pgid > 0)
	{
		/*
		 * parent and child are in a race here
		 */

		if (pgid == 1)
			pgid = pid;
		if (setpgid(pid, pgid) < 0 && pid != pgid && errno == EPERM)
			setpgid(pid, pid);
	}
	errno = n;
	return rid;
}


pid_t
spawnveg(const char* path, char* const argv[], char* const envv[], pid_t pgid, int tcfd)
{
#if !_lib_posix_spawn_file_actions_addtcsetpgrp_np
	if(tcfd >= 0)
		return spawnveg_slow(path, argv, envv, pgid, tcfd);
#endif
#ifndef POSIX_SPAWN_SETSID
	if(pgid == -1)
		return spawnveg_slow(path, argv, envv, pgid, tcfd);
#endif
#if _fast_spawnveg
	return spawnveg_fast(path, argv, envv, pgid, tcfd);
#else
	return spawnveg_slow(path, argv, envv, pgid, tcfd);
#endif
}

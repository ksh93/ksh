/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2026 Contributors to ksh 93u+m           *
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

#if _lib_posix_spawn > 1	/* reports underlying exec() errors */

#include <spawn.h>
#include <error.h>
#include <wait.h>

pid_t
spawnveg(const char* path, char* const argv[], char* const envv[], pid_t pgid, int tcfd)
{
	int				err, flags = 0;
	pid_t				pid;
	posix_spawnattr_t		attr;
#if _lib_posix_spawn_file_actions_addtcsetpgrp_np
	posix_spawn_file_actions_t	actions;
#else
	NOT_USED(tcfd);
#endif

	if (err = posix_spawnattr_init(&attr))
		goto nope;
#if POSIX_SPAWN_SETSID
	if (pgid == -1)
		flags |= POSIX_SPAWN_SETSID;
#endif
	if (pgid && pgid != -1)
		flags |= POSIX_SPAWN_SETPGROUP;
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
		if (err = posix_spawn_file_actions_init(&actions))
			goto bad;
		if (err = posix_spawn_file_actions_addtcsetpgrp_np(&actions, tcfd))
			goto fail;
	}
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

#else

#if _lib_spawn_mode

#include <process.h>

#ifndef P_NOWAIT
#define P_NOWAIT	_P_NOWAIT
#endif
#if !defined(P_DETACH) && defined(_P_DETACH)
#define P_DETACH	_P_DETACH
#endif

pid_t
spawnveg(const char* path, char* const argv[], char* const envv[], pid_t pgid, int tcfd)
{
	NOT_USED(tcfd);
#if defined(P_DETACH)
	return spawnve(pgid ? P_DETACH : P_NOWAIT, path, argv, envv ? envv : environ);
#else
	return spawnve(P_NOWAIT, path, argv, envv ? envv : environ);
#endif
}

#else

#if _lib_spawn && _hdr_spawn && _mem_pgroup_inheritance

#include <spawn.h>

/*
 * MVS OpenEdition / z/OS fork+exec+(setpgid)
 */

pid_t
spawnveg(const char* path, char* const argv[], char* const envv[], pid_t pgid, int tcfd)
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

#include <error.h>
#include <wait.h>
#include <sig.h>
#include <ast_tty.h>

#if _lib_spawnve && _hdr_process
#include <process.h>
#if defined(P_NOWAIT) || defined(_P_NOWAIT)
#undef	_lib_spawnve
#endif
#endif

/*
 * fork+exec+(setsid|setpgid)
 */

pid_t
spawnveg(const char* path, char* const argv[], char* const envv[], pid_t pgid, int tcfd)
{
	int			n;
	int			m;
	pid_t			pid;
	pid_t			rid;
	int			err[2];

	NOT_USED(tcfd);
	if (!envv)
		envv = environ;
#if _lib_spawnve
	if (!pgid)
		return spawnve(path, argv, envv);
#endif /* _lib_spawnve */
	n = errno;
	if (pipe(err) < 0)
		err[0] = -1;
	else
	{
		fcntl(err[0], F_SETFD, FD_CLOEXEC);
		fcntl(err[1], F_SETFD, FD_CLOEXEC);
	}
	sigcritical(SIG_REG_EXEC|SIG_REG_PROC);
	pid = fork();
	if (pid == -1)
		n = errno;
	else if (!pid)
	{
		sigcritical(0);
		if (pgid == -1)
			setsid();
		else if (pgid)
		{
			m = 0;
			if (pgid == 1 || pgid == -2 && (m = 1))
				pgid = getpid();
			if (setpgid(0, pgid) < 0 && errno == EPERM)
				setpgid(pgid, 0);
			if (m)
				tcsetpgrp(2, pgid);
		}
		execve(path, argv, envv);
		if (err[0] != -1)
		{
			m = errno;
			write(err[1], &m, sizeof(m));
		}
		_exit(errno == ENOENT ? EXIT_NOTFOUND : EXIT_NOEXEC);
	}
	rid = pid;
	if (err[0] != -1)
	{
		ast_close(err[1]);
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
		ast_close(err[0]);
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

#endif

#endif

#endif

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
#include <error.h>
#include <wait.h>
#include <sig.h>
#include <ast_tty.h>
#include <ast_fcntl.h>

/*
 * Set the SID, PGID and TCPGRP in the child process
 * after forking.
 */
static void setup_child(pid_t pgid, int tcfd)
{
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
		if (pgid == -1)
			pgid = getpid();
		tcsetpgrp(tcfd, pgid);
		signal(SIGTTIN,SIG_DFL);
		signal(SIGTTOU,SIG_DFL);
		signal(SIGTSTP,SIG_DFL);
	}
}

static void fork_cleanup(pid_t pid, pid_t pgid, int err)
{
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
	errno = err;
}


static noreturn void exit_child(void)
{
	if (errno == ENOENT)
		_exit(EXIT_NOTFOUND);
#ifdef ENAMETOOLONG
	if (errno == ENAMETOOLONG)
		_exit(EXIT_NOTFOUND);
#endif
	_exit(EXIT_NOEXEC);
}

#if _lib_clone
#define _fast_spawnveg 1
#define STACK_SIZE 1024*64
#include <sched.h>

/*
 * This version of spawnveg uses the Linux clone(2) syscall via the
 * frontend wrapper provided by the libc. Using clone directly is
 * more portable than posix_spawn_file_actions_addtcsetpgrp_np().
 * This implementation works on Linux (glibc and musl) and NetBSD.
 *
 * This function does a few things to attain better performance
 * than the glibc and musl implementations of posix_spawn:
 *   - The child stack is allocated via a function local 'char stack[]'
 *     like in musl, which is faster than using mmap ala glibc.
 *   - The errno from a failed execve is merely stored in the
 *     args->err variable, which is accessible by both the parent
 *     and child thanks to CLONE_VM. This behavior matches
 *     glibc and 93u+'s _real_vfork spawnveg, and is faster than musl
 *     (which opens a pipe for interprocess communication; we don't
 *     need that).
 *
 * Additionally, unlike with posix_spawn we don't pay attention to
 * error conditions from setpgid, tcsetpgrp, or setsid. For ksh93
 * it's preferable we spawn a process when possible, rather than
 * abort prematurely. As of 7d2bb8fd the posix_spawn implementation
 * will try again without POSIX_SPAWN_SETPGROUP (posix_spawn fails
 * without spawning if any of the previous syscalls failed). In the
 * clone(2) version we don't need to abort our spawn attempt if
 * the process group couldn't be set.
 *
 * We also avoid cruft by assuming ksh93 is single-threaded.
 * Implementations of posix_spawn try to be thread-safe, which we
 * don't care about.
 */

struct cargs
{
	const char	*path;
	char		**argv;
	char		**envv;
	volatile int	err;
	pid_t		pgid;
	int		tcfd;
};

static noreturn int exec_process(void *data)
{
	struct cargs *args = (struct cargs*)data;
	setup_child(args->pgid, args->tcfd);
	execve(args->path, args->argv, args->envv);
	args->err = errno;
	exit_child();
}

pid_t
spawnveg_fast(const char* path, char* const argv[], char* const envv[], pid_t pgid, int tcfd)
{
	pid_t		pid;
	char		stack[STACK_SIZE];
	struct cargs	args;
#if defined(__MACHINE_STACK_GROWS_UP) || defined(__hppa__) || defined(__metag__)
	void		*stack_top = stack;
#else
	void		*stack_top = stack+STACK_SIZE;
#endif

	args.path = path;
	args.argv = (char**)argv;
	args.envv = (char**)(envv ? envv : environ);
	args.err = 0;
	args.pgid = pgid;
	args.tcfd = tcfd;
	sigcritical(SIG_REG_EXEC|SIG_REG_PROC|(tcfd>=0?SIG_REG_TERM:0));
	pid = clone(exec_process, stack_top, CLONE_VM|CLONE_VFORK|SIGCHLD, &args);
	if (pid == -1)
		args.err = errno;
	else if (args.err)
	{
		while (waitpid(pid, NULL, 0) == -1 && errno == EINTR);
		pid = -1;
	}
	fork_cleanup(pid, pgid, args.err);
	return pid;
}

#elif _lib_posix_spawn > 1	/* reports underlying exec() errors */
#define _fast_spawnveg 1

/*
 * This version runs commands via posix_spawn(3) when possible
 * for better performance than we'd get with fork(3).
 */

#include <spawn.h>

static pid_t
spawnveg_fast(const char* path, char* const argv[], char* const envv[], pid_t pgid, int tcfd)
{
	int				err;
	short				flags = 0;
	pid_t				pid;
	posix_spawnattr_t		attr;
	NOT_USED(tcfd);

	if (err = posix_spawnattr_init(&attr))
		goto nope;
#ifdef POSIX_SPAWN_SETSID
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
	if (err = posix_spawn(&pid, path, NULL, &attr, argv, envv ? envv : environ))
	{
		if ((err != EPERM) || (err = posix_spawn(&pid, path, NULL, NULL, argv, envv ? envv : environ)))
			goto bad;
	}
	posix_spawnattr_destroy(&attr);
	return pid;
	/* cleanup for different fail states */
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

#if !_lib_clone

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
		setup_child(pgid, tcfd);
		execve(path, argv, envv);
		if (err[0] != -1)
		{
			m = errno;
			write(err[1], &m, sizeof(m));
		}
		exit_child();
	}
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
				pid = -1;
				n = m;
			}
		}
		ast_close(err[0]);
	}
	fork_cleanup(pid, pgid, n);
	return pid;
}

#endif /* !_lib_clone */

pid_t
spawnveg(const char* path, char* const argv[], char* const envv[], pid_t pgid, int tcfd)
{
#if !_lib_clone
	if (tcfd >= 0)
		return spawnveg_slow(path, argv, envv, pgid, tcfd);
#endif
#if !_lib_clone && !defined(POSIX_SPAWN_SETSID)
	if (pgid == -1)
		return spawnveg_slow(path, argv, envv, pgid, tcfd);
#endif
#if _fast_spawnveg
	return spawnveg_fast(path, argv, envv, pgid, tcfd);
#else
	return spawnveg_slow(path, argv, envv, pgid, tcfd);
#endif
}

/***********************************************************************
*                                                                      *
*              This file is part of the ksh 93u+m package              *
*             Copyright (c) 2026 Contributors to ksh 93u+m             *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*            Johnothan King <johnothanking@protonmail.com>             *
*                  Martijn Dekker <martijn@inlv.org>                   *
*                                                                      *
***********************************************************************/

#include <ast.h>
#include <error.h>

/*
 * Depending on the implementation, close(2) must either:
 *   - always be repeated after EINTR (HP-UX and AIX), or
 *   - never be repreated after EINTR (most systems).
 *
 * What follows is a function that attempts to conform to the
 * required behavior for the operating systems ksh supports.
 */

int ast_close(int fd)
{
	int r, save_errno = errno;

#if _lib_posix_close

	/*
	 * This function, standardized as of POSIX Issue 8 (2024), is the best
	 * option if available. The 0 argument means it never throws EINTR.
	 * https://pubs.opengroup.org/onlinepubs/9799919799/functions/close.html
	 */

	r = posix_close(fd, 0);

#elif defined(__hpux) || defined(__hpux__)

	/*
	 * HP-UX: "[EINTR] An attempt to close a slow device or connection or
	 * file with pending aio requests was interrupted by a signal. The file
	 * descriptor still points to an open device or connection or file."
	 * https://support.hpe.com/hpesc/public/docDisplay?docId=c01922498
	 */

	while ((r = close(fd)) != 0 && errno == EINTR)
		errno = save_errno;

#elif defined(_AIX)

	/*
	 * AIX: "[EINTR] The state of the FileDescriptor is undetermined. Retry
	 * the close routine to ensure that the FileDescriptor is closed."
	 * https://www.ibm.com/docs/en/aix/7.2.0?topic=c-close-subroutine
	 *
	 * So, avoid potentially setting EBADF from the second attempt on.
	 */

	if ((r = close(fd)) != 0 && errno == EINTR)
	{
		errno = save_errno;
		while ((r = close(fd)) != 0 && errno == EINTR)
			errno = save_errno;
		if (r != 0 && errno == EBADF)
			r = 0, errno = save_errno;
	}

#else

	/*
	 * Never try again after EINTR (most systems).
	 *
	 * Quoting the STANDARDS section in NetBSD's close(2):
	 * "The finality of close(), even on error, is not specified by
	 * POSIX, but most operating systems, including FreeBSD, OpenBSD,
	 * Linux, and Solaris, implement the same semantics."
	 * https://man.netbsd.org/close.2
	 */

	if ((r = close(fd)) != 0 && errno == EINTR)
		r = 0, errno = save_errno;

#endif /* _lib_posix_close */

	/*
	 * POSIX-1.2024: "[EINPROGRESS] The function was interrupted by a
	 * signal and fildes was closed but the close operation is
	 * continuing asynchronously." (same URL as for posix_close)
	 *
	 * POSIX says the FD has been closed, so there's nothing the caller
	 * can do. Treat this as a non-error and let the OS do its thing.
	 */

#ifdef EINPROGRESS
	if (r != 0 && errno == EINPROGRESS)
		r = 0, errno = save_errno;
#endif

	/*
	 * All done.
	 */

	return r;
}

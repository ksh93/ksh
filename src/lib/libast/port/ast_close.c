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

#if _lib_close_range

#define QUEUE_MAX_FD		1024	/* for ksh93's needs we don't need an infinite queue */
#define QUEUE_SAFETY_MARGIN	7
#define QUEUE_EMPTY		(-1)
#define QUEUE_NOT_EMPTY		(-2)
static int batch_start = QUEUE_EMPTY;
static int batch_end = QUEUE_EMPTY;
static int queue_largest_fd = -1;
static int queue_smallest_fd = -1;
static int open_max_margin = QUEUE_MAX_FD - 7;
static uint8_t fd_queue[QUEUE_MAX_FD+1] = {0};

#endif /* _lib_close_range */

/*
 * Depending on the implementation, after EINTR, close(2) must either
 * always, or never be repeated. And if it must be repeated, then the FD
 * state is indeterminate on some systems, so the repeated close(2) call may
 * or may not return -1 and set errno to EBADF.
 *
 * posix_close(2) was introduced to fix this mess, but it introduced a
 * complication of its own: EINPROGRESS, a non-error returned as an error.
 *
 * What follows is a function that attempts to conform to the
 * required behavior for the operating systems ksh supports.
 */

static int close_single(int fd)
{
	int r, save_errno = errno;

#if _lib_posix_close

	/*
	 * This function, standardized as of POSIX Issue 8 (2024), is the best
	 * option if available. The 0 argument means it never throws EINTR.
	 * https://pubs.opengroup.org/onlinepubs/9799919799/functions/close.html
	 */

	r = posix_close(fd, 0);

#elif defined(hpux) || defined(__hpux) || defined(__QNX__)

	/*
	 * HP-UX: "[EINTR] An attempt to close a slow device or connection or
	 * file with pending aio requests was interrupted by a signal. The file
	 * descriptor still points to an open device or connection or file."
	 * https://support.hpe.com/hpesc/public/docDisplay?docId=c01922498
	 *
	 * QNX: "[EINTR] The close() call was interrupted by a signal. In the
	 * QNX Neutrino implementation, the file descriptor remains open."
	 * https://www.qnx.com/developers/docs/7.1/com.qnx.doc.neutrino.lib_ref/topic/c/close.html
	 * https://www.qnx.com/developers/docs/6.5.0SP1.update/#./com.qnx.doc.neutrino_lib_ref/c/close.html
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
	 * Linux close(2): "[...] the Linux kernel /always/ releases the file
	 * descriptor early in the close operation, freeing it for reuse; the
	 * steps that may return an error, such as flushing data to the
	 * filesystem or device, occur only later in the close operation."
	 *
	 * FreeBSD close(2): "In case of any error except EBADF, the supplied
	 * file descriptor is deallocated and therefore is no longer valid."
	 *
	 * NetBSD, OpenBSD, DragonflyBSD, and Solaris/illumos have been
	 * confirmed to work the same way by a reading of their kernel sources.
	 * The Solaris/illumos close(2) manual is highly misleading on this.
	 *
	 * There's nothing for the caller to do, so treat this as a non-error.
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

/*
 * Set errno, return -1 for bad file descriptors, and mark the codepath cold.
 */
static COLD int bad_fd(int err)
{
	errno = err;
	return -1;
}

#if _lib_close_range

/*
 * File descriptors are placed in a batching queue for close_range(2),
 * which is cheaper than calling close(2) multiple times.
 * The queueing system itself must have low overhead for this to be
 * worth bothering with for small batches (ksh most commonly batches
 * two at a time).
 */

static int close_queue(int new_fd)
{
	int r;
	if(batch_start == batch_end)
		r = close_single(batch_start);	/* close(2) is faster for closing just one fd */
	else
		r = close_range((unsigned)batch_start, (unsigned)batch_end, 0);
	batch_start = batch_end = new_fd;	/* New batch */
	return r;
}

static int clear_queue(void)
{
	int got_failure = 0;
	if(batch_start != QUEUE_NOT_EMPTY)
		return 0;
	for(int fd = queue_smallest_fd; fd <= queue_largest_fd; fd++)
	{
		if(fd_queue[fd])
		{
			if(batch_start < 0)
				batch_start = batch_end = fd;
			else if(batch_end == fd - 1)
				batch_end++;
			else if(close_queue(fd) < 0)
				got_failure = errno;
			fd_queue[fd] = 0;
		}
	}
	close_queue(QUEUE_EMPTY);
	queue_largest_fd = queue_smallest_fd = QUEUE_EMPTY;
	if(got_failure)
		return bad_fd(got_failure);
	return 0;
}

static int add_to_queue(int fd)
{
	if(UNLIKELY(QUEUE_MAX_FD < fd))
		return close_single(fd);	/* The fd will not fit in the queue */
	fd_queue[fd] = 1;
	batch_start = batch_end = QUEUE_NOT_EMPTY;
	if(queue_largest_fd < fd)
		queue_largest_fd = fd;
	if(queue_smallest_fd > fd || queue_smallest_fd == QUEUE_EMPTY)
		queue_smallest_fd = fd;
	if(UNLIKELY(open_max_margin < fd))
		return clear_queue();
	return 0;
}

int ast_close_batch(int fd, close_mode_t mode)
{
	if(mode != ENACT && fd < 0)
		return bad_fd(EBADF);
	switch(mode)
	{
	    case NOW:
		return close_single(fd);
	    case AUXILIARY:
		return 0;  /* no-op */
	    case QUEUE:
		return add_to_queue(fd);
	    case ENACT:
		if(fd > -1)
			add_to_queue(fd);
		return clear_queue();
	    case LIMIT:
		open_max_margin = fd - 7;
		return 0;
	}
	UNREACHABLE();
}

#else

int ast_close_batch(int fd, close_mode_t mode)
{
	if(mode != ENACT && fd < 0)
		return bad_fd(EBADF);
	if(mode == AUXILIARY || mode == LIMIT || (mode == ENACT && fd < 0))
		return 0;
	return close_single(fd);
}

#endif /* _lib_close_range */

int ast_close(int fd)
{
	return close_single(fd);
}

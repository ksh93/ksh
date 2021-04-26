/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2013 AT&T Intellectual Property          *
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
*               Glenn Fowler <glenn.s.fowler@gmail.com>                *
*                    David Korn <dgkorn@gmail.com>                     *
*                     Phong Vo <phongvo@gmail.com>                     *
*                                                                      *
***********************************************************************/
#pragma prototyped

#include <assert.h>
#include <tv.h>
#include <tm.h>
#include <error.h>

#include "FEATURE/tvlib"

#if !_lib_nanosleep
# if _lib_select && !_prefer_poll
#  if _sys_select
#   include <sys/select.h>
#  else
#   include <sys/socket.h>
#  endif
# elif _lib_poll_notimer
#  undef _lib_poll
# elif _lib_poll
#  include <poll.h>
# endif
#endif

#define NANOSECONDS  1000000000L
#define MILLISECONDS 1000

/*
 * sleep for tv
 * non-zero exit if sleep did not complete
 * with remaining time in rv
 *
 * NOTE: some systems hide nanosleep() outside of libc
 */

int
tvsleep(register const Tv_t* tv, register Tv_t* rv)
{
	assert(tv); /* Validate argument */

	/* Return immediately if asked to sleep for no duration. */
	if (!tv->tv_sec && !tv->tv_nsec)
		return 0;

	{

#if _lib_nanosleep

	/* PRECISION: nanoseconds */
	struct timespec	stv = { tv->tv_sec, tv->tv_nsec };
	struct timespec	srv;
	int r;

	if ((r = nanosleep(&stv, &srv)) && errno == EINTR && rv)
	{
		rv->tv_sec = srv.tv_sec;
		rv->tv_nsec = srv.tv_nsec;
	}
	return r;

	}

#else

	Tv_t tvBefore;

	tvgettime(&tvBefore);
	{

#if _lib_select && !_prefer_poll

	/* PRECISION: microseconds */
	struct timeval tvSleep = { tv->tv_sec, tv->tv_nsec / 1000 };
	if (tv->tv_nsec % 1000)
		++tvSleep.tv_usec;
	(void)select(0, NiL, NiL, NiL, &tvSleep);

#elif _lib_poll

	/* PRECISION: milliseconds
	 *
	 * We can sleep for up to 24 days with a single call to poll
	 * on systems with 32-bit integers, so calling sleep first
	 * only worsens precision with little gain. For example, the
	 * UnixWare manual page for usleep warns that a single call of
	 * that function requires eight system calls, but poll only
	 * requires one.
	 */
	struct pollfd	dummy;
	int		timeout = INT_MAX;  /* expose bugs */

	if (tv->tv_sec <= (INT_MAX / MILLISECONDS))
	{
		timeout = tv->tv_sec  * MILLISECONDS +
			  tv->tv_nsec / 1000000;
		if (timeout < INT_MAX && tv->tv_nsec % 1000000)
			++timeout;
	}
	(void)poll(&dummy, 0, timeout);

#else

	uint32_t	s = tv->tv_sec;
	uint32_t	n = tv->tv_nsec;
	unsigned int	t;

#if _lib_usleep

	if (s < (0x7fffffff / 1000000))
	{
		int	oerrno;

		oerrno = errno;
		errno = 0;
		usleep(s * 1000000 + n / 1000);
		if (!errno)
		{
			errno = oerrno;
			return 0;
		}
	}
	else
	{

#endif

		if (s += (n + 999999999L) / 1000000000L)
		{
			while (s)
			{
				if (s > UINT_MAX)
				{
					t = UINT_MAX;
					s -= UINT_MAX;
				}
				else
				{
					t = s;
					s = 0;
				}
				if (t = sleep(t))
				{
					if (rv)
					{
						rv->tv_sec = s + t;
						rv->tv_nsec = 0;
					}
					return -1;
				}
			}
			return 0;
		}

#if _lib_usleep

		if (t = (n + 999L) / 1000L)
		{
			int	oerrno;

			oerrno = errno;
			errno = 0;
			usleep(t);
			if (!errno)
			{
				errno = oerrno;
				return 0;
			}
		}
	}

#endif
#endif

	}

/* Unfortunately, some operating systems return success for select
 * or poll without having slept for the specified duration, so check
 * the clock.
 *
 * Although time discrepancies when sleeping are inevitable, tvsleep
 * can guarantee that they always lie on or after the time specified,
 * which is much more useful from the point of view of predictability
 * than if they could also occur before.
 */
	{
		Tv_t tvAfter;

		tvgettime(&tvAfter);

		if (tvAfter.tv_nsec < tvBefore.tv_nsec)
		{
			if (!tvAfter.tv_sec)
				return 0;
			--tvAfter.tv_sec;
			tvAfter.tv_nsec += (NANOSECONDS - tvBefore.tv_nsec);
		}
		else
		{
		    tvAfter.tv_nsec -= tvBefore.tv_nsec;
		}
		if (tvAfter.tv_sec < tvBefore.tv_sec)
			return 0;
		tvAfter.tv_sec -= tvBefore.tv_sec;
		/* tvAfter now holds the non-negative time slept */

		tvBefore = *tv;
		/* Normalize the time to sleep so that ns < 10e9 */
		tvBefore.tv_sec  += tvBefore.tv_nsec / NANOSECONDS;
		tvBefore.tv_nsec %= NANOSECONDS;

		if (tvBefore.tv_nsec < tvAfter.tv_nsec)
		{
			if (!tvBefore.tv_sec)
				return 0;
			--tvBefore.tv_sec;
			tvBefore.tv_nsec += (NANOSECONDS - tvAfter.tv_nsec);
		}
		else
		{
			tvBefore.tv_nsec -= tvAfter.tv_nsec;
		}
		if (tvBefore.tv_sec < tvAfter.tv_sec)
			return 0;
		tvBefore.tv_sec -= tvAfter.tv_sec;

		if (tvBefore.tv_sec > 0 || tvBefore.tv_nsec > 0)
		{
			if (rv)
				*rv = tvBefore;
			return -1;
		}
	}
	}

	return 0;

#endif

}

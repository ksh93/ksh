/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
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

#include <tv.h>
#include <tm.h>

#include "FEATURE/tvlib"

int
tvsettime(const Tv_t* tv)
{

#if _lib_clock_settime && defined(CLOCK_REALTIME)

	struct timespec			s;

	s.tv_sec = tv->tv_sec;
	s.tv_nsec = tv->tv_nsec;
	return clock_settime(CLOCK_REALTIME, &s);

#else

#if defined(tmsettimeofday)

	struct timeval			v;

	v.tv_sec = tv->tv_sec;
	v.tv_usec = tv->tv_nsec / 1000;
	return tmsettimeofday(&v);

#else

#if _lib_stime

	static time_t			s;

	s = tv->tv_sec + (tv->tv_nsec != 0);
	return stime(s);

#else

	errno = EPERM;
	return -1;

#endif

#endif

#endif

}

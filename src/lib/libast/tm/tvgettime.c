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
tvgettime(Tv_t* tv)
{

#if _lib_clock_gettime && defined(CLOCK_REALTIME)

	struct timespec			s;

	clock_gettime(CLOCK_REALTIME, &s);
	tv->tv_sec = s.tv_sec;
	tv->tv_nsec = (uint32_t)s.tv_nsec;

#else

#if defined(tmgettimeofday)

	struct timeval			v;

	tmgettimeofday(&v);
	tv->tv_sec = v.tv_sec;
	tv->tv_nsec = v.tv_usec * 1000;

#else

	static time_t			s;
	static uint32_t			n;

	if ((tv->tv_sec = time(NULL)) != s)
	{
		s = tv->tv_sec;
		n = 0;
	}
	else
		n += 1000;
	tv->tv_nsec = n;

#endif

#endif

	return 0;
}

/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
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
 * <debug.h> support
 */

#include <ast.h>
#include <error.h>
#include <debug.h>
#include "FEATURE/time"

void
debug_fatal(const char* file, int line)
{
	error(2, "%s:%d: debug error", file, line);
	abort();
}

#if _sys_times && _lib_getrusage

#include <times.h>
#ifndef RUSAGE_SELF
#include <sys/resource.h>
#endif
double
debug_elapsed(int set)
{
	double		tm;
	struct rusage	ru;

	static double	prev;

	getrusage(RUSAGE_SELF, &ru);
	tm = (double)ru.ru_utime.tv_sec  + (double)ru.ru_utime.tv_usec/1000000.0;
	if (set)
		return prev = tm;
	return tm - prev;
}

#else

double
debug_elapsed(int set)
{
	return 0;
}

#endif

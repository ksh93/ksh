/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2023 Contributors to ksh 93u+m           *
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
 * used to test if -last requires -lm
 *
 *	arch		-last			-lm
 *	----		-----			---
 *	linux.sparc	sfdlen,sfputd		frexp,ldexp	
 */

#if N >= 8
#define _ISOC99_SOURCE	1
#endif

#include <math.h>

int
main(void)
{
#if N & 1
	long double	value = 0;
#else
	double		value = 0;
#endif
#if N < 5
	int		exp = 0;
#endif

#if N == 1
	return ldexpl(value, exp) != 0;
#endif
#if N == 2
	return ldexp(value, exp) != 0;
#endif
#if N == 3
	return frexpl(value, &exp) != 0;
#endif
#if N == 4
	return frexp(value, &exp) != 0;
#endif
#if N == 5
	return isnan(value);
#endif
#if N == 6
	return isnan(value);
#endif
#if N == 7
	return copysign(1.0, value) < 0;
#endif
#if N == 8
	return signbit(value);
#endif
}

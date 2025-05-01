/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1994-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2025 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
/*
 * -lm test #4
 *
 * This program is compiled and linked by mkreq-maplib.sh via INIT/Mamfile
 * but never actually run. It is only used to check if linking succeeds
 * without or with -lm.
 *
 * For that test to work correctly, we must work around compiler optimization.
 * The rand() call is to stop the result from being considered known at
 * compile time, which would cause modern compilers to optimize out the probe
 * calls, which would in turn cause linking to succeed where it shouldn't.
 */

#include <stdlib.h>
#include <math.h>

int
main(void)
{
	double	value = (double)rand();

	return isnan(value);
}

/* (obsolete test #5 for isnanl deleted; C99 specifies isnan for all types) */

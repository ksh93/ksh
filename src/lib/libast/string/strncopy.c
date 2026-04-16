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

#include <ast.h>

/*
 * copy up to n bytes of string f into t
 * trailing 0 always added to t, even if n==0
 * pointer to the copied 0 returned
 * overlapping buffers are permitted
 */

char*
strncopy(char* t, const char* f, size_t n)
{
	char*	e = t + n - 1;

	do
	{
		if (t >= e)
		{
			*t = 0;
			return t;
		}
	} while (*t++ = *f++);
	return t - 1;
}

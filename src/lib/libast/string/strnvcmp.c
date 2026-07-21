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
#include <ctype.h>

/*
 * version strncmp(3)
 */

int
strnvcmp(const char* a, const char* b, size_t n)
{
	const char*	ae;
	const char*	be;
	unsigned long	na;
	unsigned long	nb;

	ae = a + n;
	be = b + n;
	for (;;)
	{
		if (a >= ae)
		{
			if (b >= be)
				return 0;
			return 1;
		}
		else if (b >= be)
			return -1;
		if (isdigit(*a) && isdigit(*b))
		{
			na = nb = 0;
			while (a < ae && isdigit(*a))
				na = na * 10 + (unsigned)*a++ - '0';
			while (b < be && isdigit(*b))
				nb = nb * 10 + (unsigned)*b++ - '0';
			if (na < nb)
				return -1;
			if (na > nb)
				return 1;
		}
		else if (*a != *b)
			break;
		else if (!*a)
			return 0;
		else
		{
			a++;
			b++;
		}
	}
	if (*a == 0)
		return -1;
	if (*b == 0)
		return 1;
	if (*a == '.')
		return -1;
	if (*b == '.')
		return 1;
	if (*a == '-')
		return -1;
	if (*b == '-')
		return 1;
	return *a < *b ? -1 : 1;
}

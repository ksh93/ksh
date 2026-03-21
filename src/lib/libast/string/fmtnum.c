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
/*
 * Glenn Fowler
 * AT&T Research
 *
 * return scaled number n
 * string width is 5 chars or less
 * if m>1 then n divided by m before scaling
 */

#include <ast.h>

char*
fmtnum(unsigned long n, int m)
{
	int		i;
	unsigned long	r;
	char*		buf;
	size_t		z;

	char		suf[2];

	if (m > 1)
	{
		r = n;
		n /= (unsigned long)m;
		r -= n;
	}
	else
		r = 0;
	suf[1] = 0;
	if (n < 1024)
		suf[0] = 0;
	else if (n < 1024 * 1024)
	{
		suf[0] = 'k';
		r = ((n % 1024) * 100) / 1024;
		n /= 1024;
	}
	else if (n < 1024 * 1024 * 1024)
	{
		suf[0] = 'm';
		r = ((n % (1024 * 1024)) * 100) / (1024 * 1024);
		n /= 1024 * 1024;
	}
	else
	{
		suf[0] = 'g';
		r = ((n % (1024 * 1024 * 1024)) * 100) / (1024 * 1024 * 1024);
		n /= 1024 * 1024 * 1024;
	}
	if (r)
	{
		if (n >= 100)
			r = 0;
		else if (n >= 10)
		{
			i = 1;
			if (r >= 10)
				r /= 10;
		}
		else
			i = 2;
	}
	buf = fmtbuf(z = 8);
	if (r)
		sfsprintf(buf, z, "%lu.%0*lu%s", n, i, r, suf);
	else
		sfsprintf(buf, z, "%lu%s", n, suf);
	return buf;
}

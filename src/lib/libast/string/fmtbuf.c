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
 * return small format buffer chunk of size n
 * format buffers are short lived
 * only one concurrent buffer with size > sizeof(buf)
 */

static char		buf[16 * 1024];
static char*		nxt = buf;

static char*		big;
static size_t		bigsiz;

char*
fmtbuf(size_t n)
{
	char*	cur;

	if (n > (size_t)(&buf[elementsof(buf)] - nxt))
	{
		if (n > elementsof(buf))
		{
			if (n > bigsiz)
			{
				bigsiz = roundof(n, 8U * 1024U);
				if (!(big = newof(big, char, bigsiz, 0)))
					return NULL;
			}
			return big;
		}
		nxt = buf;
	}
	cur = nxt;
	nxt += n;
	return cur;
}

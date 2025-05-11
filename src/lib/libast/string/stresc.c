/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2025 Contributors to ksh 93u+m           *
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
*                                                                      *
***********************************************************************/
/*
 * Glenn Fowler
 * AT&T Research
 *
 * convert \X character constants in s in place
 * the length of the converted s is returned (may have embedded \0's)
 * strexp() FMT_EXP_* flags passed to chrexp() for selective conversion
 */

#include <ast.h>

int
strexp(char* s, int flags)
{
	char*		t;
	int		c;
	char*		b;
	char*		e;
	int		w;

	b = t = s;
	while (c = *s++)
	{
		if (c == '\\')
		{
			c = chrexp(s - 1, &e, &w, flags);
			s = e;
			if (c < 0)
				continue;
			if (w)
			{
				t += mbconv(t, c);
				continue;
			}
		}
		*t++ = c;
	}
	*t = 0;
	return t - b;
}

int
stresc(char* s)
{
	return strexp(s, FMT_EXP_CHAR|FMT_EXP_LINE|FMT_EXP_WIDE);
}

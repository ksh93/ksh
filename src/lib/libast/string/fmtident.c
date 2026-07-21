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

#define IDENT	01
#define USAGE	02

/*
 * format what(1) and/or ident(1) string a
 */

char*
fmtident(const char* a)
{
	char*	s = (char*)a;
	char*	t;
	char*	buf;
	size_t	i;

	i = 0;
	for (;;)
	{
		while (isspace(*s))
			s++;
		if (s[0] == '[')
		{
			while (*++s && *s != '\n');
			i |= USAGE;
		}
		else if (s[0] == '@' && s[1] == '(' && s[2] == '#' && s[3] == ')')
			s += 4;
		else if (s[0] == '$' && s[1] == 'I' && s[2] == 'd' && s[3] == ':' && isspace(s[4]))
		{
			s += 5;
			i |= IDENT;
		}
		else
			break;
	}
	if (i)
	{
		i &= IDENT;
		for (t = s; isprint(*t) && *t != '\n'; t++)
			if (i && t[0] == ' ' && t[1] == '$')
				break;
		while (t > s && isspace(t[-1]))
			t--;
		i = (size_t)(t - s);
		buf = fmtbuf(i + 1);
		memcpy(buf, s, i);
		s = buf;
		s[i] = 0;
	}
	return s;
}

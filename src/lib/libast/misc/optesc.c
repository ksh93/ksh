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
 * escape optget() special chars in s and write to sp
 * esc == '?' or ':' also escaped
 */

#include <optlib.h>
#include <ctype.h>

int
optesc(Sfio_t* sp, const char* s, int esc)
{
	const char*	m;
	int		c;

	if (*s == '[' && *(s + 1) == '+' && *(s + 2) == '?')
	{
		c = (int)strlen(s);
		if (s[c - 1] == ']')
		{
			sfprintf(sp, "%-.*s", c - 4, s + 3);
			return 0;
		}
	}
	if (esc != '?' && esc != ':')
		esc = 0;
	while (c = *s++)
	{
		if (isalnum(c))
		{
			for (m = s - 1; isalnum(*s); s++);
			if (isalpha(c) && *s == '(' && isdigit(*(s + 1)) && *(s + 2) == ')')
			{
				sfputc(sp, '\b');
				sfwrite(sp, m, (size_t)(s - m));
				sfputc(sp, '\b');
				sfwrite(sp, s, 3);
				s += 3;
			}
			else
				sfwrite(sp, m, (size_t)(s - m));
		}
		else if (c == '-' && *s == '-' || c == '<')
		{
			m = s - 1;
			if (c == '-')
				s++;
			else if (*s == '/')
				s++;
			while (isalnum(*s))
				s++;
			if (c == '<' && *s == '>' || isspace(*s) || *s == 0 || *s == '=' || *s == ':' || *s == ';' || *s == '.' || *s == ',')
			{
				sfputc(sp, '\b');
				sfwrite(sp, m, (size_t)(s - m));
				sfputc(sp, '\b');
			}
			else
				sfwrite(sp, m, (size_t)(s - m));
		}
		else
		{
			if (c == ']' || c == esc)
				sfputc(sp, c);
			sfputc(sp, c);
		}
	}
	return 0;
}

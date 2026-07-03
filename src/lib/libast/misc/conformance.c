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

static char**		ids;

static const char*	dflt[] = { "ast", "standard", 0 };

/*
 * initialize the conformance() ID list
 */

static char**
initconformance(void)
{
	char*			m;
	char**			p;
	char*			t;
	size_t			h;
	size_t			i;
	size_t			j;
	int			c;
	Sfio_t*			sp;

	static const char*	conf[] = { "CONFORMANCE", "HOSTTYPE", "UNIVERSE" };

	p = 0;
	if (sp = sfstropen())
	{
		for (i = h = 0, j = 1; i < elementsof(conf); i++)
			if (*(m = astconf(conf[i], NULL, NULL)) && (h |= (1<<i)) || !i && (m = "ast"))
			{
				ptrdiff_t d;
				t = m;
				while ((c = *m++) && c != '.')
				{
					if (isupper(c))
						c = tolower(c);
					sfputc(sp, c);
				}
				sfputc(sp, 0);
				j++;
				if ((d = (m - t)) == 6 && strneq(t, "linux", 5))
				{
					sfputr(sp, "gnu", 0);
					j++;
				}
				else if (d > 3 && strneq(t, "bsd", 3) || d == 7 && strneq(t, "debian", 7))
				{
					sfputr(sp, "bsd", 0);
					j++;
				}
				if (h & 1)
					break;
			}
		i = (size_t)sfstrtell(sp);
		sfstrseek(sp, 0, SEEK_SET);
		if (p = newof(0, char*, j, i))
		{
			m = (char*)(p + j--);
			memcpy(m, sfstrbase(sp), i);
			i = 0;
			p[i++] = m;
			while (i < j)
			{
				while (*m++);
				p[i++] = m;
			}
			p[i] = 0;
		}
		sfstrclose(sp);
	}
	if (!p)
		p = (char**)dflt;
	return ids = p;
}

/*
 * return conformance ID if s size n is in conformance
 * prefix match of s on the conformance ID table
 * s==0 => "standard"
 */

char*
conformance(const char* s, size_t n)
{
	char**		p;
	char**		q;
	char*		m;
	const char*	e;
	const char*	t;

	static uint64_t	serial = ~(uint64_t)0;

	if (!(p = ids) || serial != ast.env_serial)
	{
		serial = ast.env_serial;
		if (ids)
		{
			if (ids != (char**)dflt)
				free(ids);
			ids = 0;
		}
		p = initconformance();
	}
	if (!s)
		s = dflt[1];
	if (!n)
		n = strlen(s);
	e = s + n;
	if (*s == '(')
		s++;
	do
	{
		while (s < e && (isspace(*s) || *s == ',' || *s == '|'))
			s++;
		if (*s == ')')
			break;
		for (t = s; s < e && !isspace(*s) && *s != ',' && *s != '|' && *s != ')'; s++);
		if (s == t)
			break;
		q = p;
		while (m = *q++)
			if (strneq(t, m, (size_t)(s - t)))
				return m;
		if (s < e)
			s++;
	} while (s < e);
	return NULL;
}

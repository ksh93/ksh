/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
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

#include "intercepts.h"

/*
 * put name=value in the environment
 * pointer to value returned
 * environ==0 is ok
 *
 *	setenviron("N=V")	add N=V
 *	setenviron("N")		delete N
 *	setenviron(0)		expect more (pre-fork optimization)
 *
 * _ always placed at the top
 */

#define INCREMENT	16		/* environ increment		*/

char*
setenviron(const char* akey)
{
#undef	setenviron
	static char**	envv;		/* recorded environ		*/
	static char**	next;		/* next free slot		*/
	static char**	last;		/* last free slot (0)		*/
	static char	ok[] = "";	/* delete/optimization ok return*/

	char*		key = (char*)akey;
	char**		v = environ;
	char**		p = envv;
	char*		s;
	char*		t;
	ptrdiff_t	n;

	ast.env_serial++;
	if (intercepts.intercept_setenviron)
		return (*intercepts.intercept_setenviron)(akey);
	if (p && !v)
	{
		environ = next = p;
		*++next = 0;
	}
	else if (p != v || !v)
	{
		if (v)
		{
			while (*v++);
			n = v - environ + INCREMENT;
			v = environ;
		}
		else
			n = INCREMENT;
		if (!p || (last - p + 1) < n)
		{
			if (!(p = newof(p, char*, (size_t)n, 0)))
				return NULL;
			last = p + n - 1;
		}
		envv = environ = p;
		if (v && v[0] && v[0][0] == '_' && v[0][1] == '=')
			*p++ = *v++;
		else
			*p++ = "_=";
		if (!v)
			*p = 0;
		else
			while (*p = *v++)
				if (p[0][0] == '_' && p[0][1] == '=')
					envv[0] = *p;
				else
					p++;
		next = p;
		p = envv;
	}
	else if (next == last)
	{
		n = last - v + INCREMENT + 1;
		if (!(p = newof(p, char*, (size_t)n, 0)))
			return NULL;
		last = p + n - 1;
		next = last - INCREMENT;
		envv = environ = p;
	}
	if (!key)
		return ok;
	for (; s = *p; p++)
	{
		t = key;
		do
		{
			if (!*t || *t == '=')
			{
				if (*s == '=')
				{
					if (!*t)
					{
						v = p++;
						while (*v++ = *p++);
						next--;
						return ok;
					}
					*p = key;
					return (s = strchr(key, '=')) ? s + 1 : NULL;
				}
				break;
			}
		} while (*t++ == *s++);
	}
	if (!(s = strchr(key, '=')))
		return ok;
	p = next;
	*++next = 0;
	*p = key;
	return s + 1;
}

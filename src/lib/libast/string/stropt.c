/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2013 AT&T Intellectual Property          *
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
 */

#include <ast.h>
#include <ctype.h>

/*
 * parse option expression in s using options in tab with element size siz
 * first element in tab must be a char*
 * options match
 *
 *	[no]name[[:]=['"]value["']][, ]...
 *
 * f is called for each option
 *
 *	(*f)(void* a, char* p, int n, char* v)
 *
 *	a	from stropt
 *	p	matching tab entry, or name if no table
 *	n	0 if option had ``no'' prefix, -1 if :=, 1 otherwise
 *	v	option value pointer
 *
 * for unmatched options p is 0 and v is the offending option
 *
 * names in s may be shorter than tab names
 * longer names must have a prefix that matches a tab name
 * the first match is returned
 * \ escapes value using chresc()
 */

int
stropt(const char* as, const void* tab, int siz, int(*f)(void*, const void*, int, const char*), void* a)
{
	int	c;
	char*	s;
	char*	v;
	char*	t;
	char**	p;
	char*	u = 0;
	char*	x;
	char*	e;
	int	n;
	int	ql;
	int	qr;
	int	qc = 0;

	if (!as) n = 0;
	else if (!(x = s = strdup(as))) n = -1;
	else
	{
		for (;;)
		{
			while (isspace(*s) || *s == ',') s++;
			if (*s == 'n' && *(s + 1) == 'o')
			{
				s += 2;
				n = 0;
			}
			else n = 1;
			if (!*s)
			{
				n = 0;
				break;
			}
			if (tab)
			{
				for (p = (char**)tab; t = *p; p = (char**)((char*)p + siz))
				{
					for (v = s; *t && *t == *v; t++, v++);
					if (!*t || isspace(*v) || *v == ',' || *v == '=')
						break;
					if (*v == ':' && *(v + 1) == '=')
					{
						v++;
						n = -1;
						break;
					}
				}
				if (!t)
				{
					u = v = s;
					p = 0;
				}
			}
			else
			{
				p = (char**)(v = s);
				t = 0;
			}
			while (*v && !isspace(*v) && *v != '=' && *v != ',')
				if (*v++ == ':' && *v == '=')
				{
					if (!t)
						*(v - 1) = 0;
					n = -n;
					break;
				}
			if (*v == '=')
			{
				if (!t)
					*v = 0;
				t = s = ++v;
				ql = qr = 0;
				while (c = *s++)
				{
					if (c == '\\')
					{
						*t++ = (char)chresc(s - 1, &e);
						s = e;
					}
					else if (c == qr)
					{
						if (qr != ql)
							*t++ = (char)c;
						if (--qc <= 0)
							qr = ql = 0;
					}
					else if (c == ql)
					{
						*t++ = (char)c;
						qc++;
					}
					else if (qr)
						*t++ = (char)c;
					else if (c == ',' || isspace(c))
						break;
					else if (c == '"' || c == '\'')
					{
						ql = qr = c;
						qc = 1;
					}
					else
					{
						*t++ = (char)c;
						if (c == '{')
						{
							ql = c;
							qr = '}';
							qc = 1;
						}
						else if (c == '(')
						{
							ql = c;
							qr = ')';
							qc = 1;
						}
					}
				}
				*t = 0;
			}
			else
			{
				s = v;
				c = *s;
				*s++ = 0;
			}
			n = p ? (*f)(a, p, n, v) : (*f)(a, p, (int)(v - u), u);
			if (n || !c)
				break;
		}
		free(x);
	}
	return n;
}

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
 * return RE expression given strmatch() pattern
 * 0 returned for invalid RE
 */

#include <ast.h>

typedef struct Stack_s
{
	char*		beg;
	ptrdiff_t	dlen;
	char		min;
} Stack_t;

char*
fmtre(const char* as)
{
	char*		s = (char*)as;
	char		c;
	ssize_t		i;
	char*		t;
	Stack_t*	p;
	char*		x;
	char		n;
	char		end;
	char*		buf;
	Stack_t		stack[32];

	end = 1;
	t = buf = fmtbuf(2 * strlen(s) + 1);
	p = stack;
	if (*s != '*' || *(s + 1) == '(' || *(s + 1) == '-' && *(s + 2) == '(')
		*t++ = '^';
	else
		s++;
	for (;;)
	{
		switch (c = *s++)
		{
		case 0:
			break;
		case '\\':
			if (!(c = *s++) || c == '{' || c == '}')
				return NULL;
			*t++ = '\\';
			if ((*t++ = c) == '(' && *s == '|')
			{
				*t++ = *s++;
				goto logical;
			}
			continue;
		case '[':
			*t++ = c;
			n = 0;
			if ((c = *s++) == '!')
			{
				*t++ = '^';
				c = *s++;
			}
			else if (c == '^')
			{
				if ((c = *s++) == ']')
				{
					*(t - 1) = '\\';
					*t++ = '^';
					continue;
				}
				n = '^';
			}
			for (;;)
			{
				if (!(*t++ = c))
					return NULL;
				if ((c = *s++) == ']')
				{
					if (n)
						*t++ = n;
					*t++ = c;
					break;
				}
			}
			continue;
		case '{':
			for (x = s; *x && *x != '}'; x++);
			if (*x++ && (*x == '(' || *x == '-' && *(x + 1) == '('))
			{
				if (p >= &stack[elementsof(stack)])
					return NULL;
				p->beg = s - 1;
				s = x;
				p->dlen = s - p->beg;
				if (p->min = *s == '-')
					s++;
				p++;
				*t++ = *s++;
			}
			else
				*t++ = c;
			continue;
		case '*':
			if (!*s)
			{
				end = 0;
				break;
			}
			/* FALLTHROUGH */
		case '?':
		case '+':
		case '@':
		case '!':
		case '~':
			if (*s == '(' || c != '~' && *s == '-' && *(s + 1) == '(')
			{
				if (p >= &stack[elementsof(stack)])
					return NULL;
				p->beg = s - 1;
				if (c == '~')
				{
					if (*(s + 1) == 'E' && *(s + 2) == ')')
					{
						for (s += 3; *t = *s; t++, s++);
						continue;
					}
					p->dlen = 0;
					p->min = 0;
					*t++ = *s++;
					*t++ = '?';
				}
				else
				{
					p->dlen = c != '@';
					if (p->min = *s == '-')
						s++;
					*t++ = *s++;
				}
				p++;
			}
			else
			{
				switch (c)
				{
				case '*':
					*t++ = '.';
					break;
				case '?':
					c = '.';
					break;
				case '+':
				case '!':
					*t++ = '\\';
					break;
				}
				*t++ = c;
			}
			continue;
		case '(':
			if (p >= &stack[elementsof(stack)])
				return NULL;
			p->beg = s - 1;
			p->dlen = 0;
			p->min = 0;
			p++;
			*t++ = c;
			continue;
		case ')':
			if (p == stack)
				return NULL;
			*t++ = c;
			p--;
			for (i = 0; i < p->dlen; i++)
				*t++ = p->beg[i];
			if (p->min)
				*t++ = '?';
			continue;
		case '^':
		case '.':
		case '$':
			*t++ = '\\';
			*t++ = c;
			continue;
		case '|':
			if (t == buf || *(t - 1) == '(')
				return NULL;
		logical:
			if (!*s || *s == ')')
				return NULL;
			/* FALLTHROUGH */
		default:
			*t++ = c;
			continue;
		}
		break;
	}
	if (p != stack)
		return NULL;
	if (end)
		*t++ = '$';
	*t = 0;
	return buf;
}

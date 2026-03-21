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
 * return printf(3) format signature given format string
 * the format signature contains one char per format optionally preceded
 * by the number of `*' args
 *	c	char
 *	d	double
 *	D	long double
 *	f	float
 *	h	short
 *	i	int
 *	j	long long
 *	l	long
 *	p	void*
 *	s	string
 *	t	ptrdiff_t
 *	z	size_t
 *	?	unknown
 */

#include <ast.h>
#include <ctype.h>

char*
fmtfmt(const char* as)
{
	char*		s = (char*)as;
	char*		buf;
	int		c;
	int		a;
	int		q;
	int		x;
	int		t;
	int		n;
	ssize_t		i;
	ssize_t		m;
	ssize_t		z;
	char		formats[256];
	unsigned int	extra[elementsof(formats)];

	z = 1;
	i = m = 0;
	for (;;)
	{
		switch (*s++)
		{
		case 0:
			break;
		case '%':
			if (*s == '%')
				continue;
			n = 0;
			a = 0;
			q = 0;
			t = '?';
			x = 0;
			for (;;)
			{
				switch (c = *s++)
				{
				case 0:
					s--;
					break;
				case '(':
					q++;
					continue;
				case ')':
					if (--q <= 0)
						n = 0;
					continue;
				case '0': case '1': case '2': case '3':
				case '4': case '5': case '6': case '7':
				case '8': case '9':
					n = n * 10 + (c - '0');
					continue;
				case '$':
					a = n;
					n = 0;
					continue;
				case '*':
					x++;
					n = 0;
					continue;
				case 'h':
					if (!q)
						t = t == 'h' ? 'c' : 'h';
					continue;
				case 'l':
					if (!q)
						t = t == 'l' ? 'j' : 'l';
					continue;
				case 'j':
				case 't':
				case 'z':
					if (!q)
						t = c;
					continue;
				case 'c':
				case 'p':
				case 's':
					if (!q)
					{
						t = c;
						break;
					}
					continue;
				case 'e':
				case 'g':
					if (!q)
					{
						switch (t)
						{
						case 'j':
							t = 'D';
							break;
						default:
							t = 'd';
							break;
						}
						break;
					}
					continue;
				case 'f':
					if (!q)
					{
						switch (t)
						{
						case 'j':
							t = 'D';
							break;
						case 'l':
							t = 'd';
							break;
						default:
							t = c;
							break;
						}
						break;
					}
					continue;
				default:
					if (!q && isalpha(c))
					{
						if (t == '?')
							t = 'i';
						break;
					}
					n = 0;
					continue;
				}
				break;
			}
			if (a)
				i = a;
			else
				i++;
			if (i < (ssize_t)elementsof(formats))
			{
				formats[i] = (char)t;
				if (extra[i] = (unsigned)x)
					do z++; while (x /= 10);
				if (m < i)
					m = i;
			}
			continue;
		default:
			continue;
		}
		break;
	}
	s = buf = fmtbuf((size_t)(m + z));
	for (i = 1; i <= m; i++)
	{
		if (extra[i])
			s += sfsprintf(s, 10, "%d", extra[m]);
		*s++ = formats[i];
	}
	*s = 0;
	return buf;
}

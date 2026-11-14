/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2023 Contributors to ksh 93u+m           *
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
 * AT&T Bell Laboratories
 *
 * time conversion support
 */

#include <ast.h>
#include <tm.h>
#include <ctype.h>

/*
 * match s against t ignoring case and .'s
 *
 * suf is an n element table of suffixes that may trail s
 * if all isalpha() chars in s match then 1 is returned
 * and if e is non-null it will point to the first unmatched
 * char in s, otherwise 0 is returned
 */

int
tmword(const char* s, char** e, const char* t, char** suf, int n)
{
	int		c;
	const char*	b;

	if (*s && *t)
	{
		b = s;
		while (c = *s++)
		{
			if (c != '.')
			{
				if (!isalpha((unsigned char)c) || c != *t && (islower((unsigned char)c) ? toupper((unsigned char)c) : tolower((unsigned char)c)) != *t)
					break;
				t++;
			}
		}
		s--;
		if (!isalpha((unsigned char)c))
		{
			if (c == '_')
				s++;
			if (e)
				*e = (char*)s;
			return s > b;
		}
		if (!*t && s > (b + 1))
		{
			b = s;
			while (n-- && (t = *suf++))
			{
				s = b;
				while (isalpha((unsigned char)(c = *s++)) && (c == *t || (islower((unsigned char)c) ? toupper((unsigned char)c) : tolower((unsigned char)c)) == *t)) t++;
				if (!*t && !isalpha((unsigned char)c))
				{
					if (c != '_')
						s--;
					if (e)
						*e = (char*)s;
					return 1;
				}
			}
		}
	}
	return 0;
}

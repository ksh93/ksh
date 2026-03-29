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

#include <tmx.h>
#include <ctype.h>

/*
 * parse duration expression in s and return Time_t value
 * if non-null, e points to the first unused char in s
 * returns 0 with *e==s on error
 */

Time_t
tmxduration(const char* s, char** e)
{
	Time_t		ns;
	Time_t		ts;
	Time_t		now;
	char*		last;
	char*		t;
	char*		x;
	Sfio_t*		f;
	ptrdiff_t	i;

	now = TMX_NOW;
	while (isspace(*s))
		s++;
	if (*s == 'P' || *s == 'p')
		ns = tmxdate(s, &last, now) - now;
	else
	{
		ns = strtod(s, &last) * TMX_RESOLUTION;
		if (*last && (f = sfstropen()))
		{
			sfprintf(f, "exact %s", s);
			t = sfstruse(f);
			ts = tmxdate(t, &x, now);
			if ((i = x - t - 6) > (last - s))
			{
				last = (char*)s + i;
				ns = ts - now;
			}
			else
			{
				sfprintf(f, "p%s", s);
				t = sfstruse(f);
				ts = tmxdate(t, &x, now);
				if ((i = x - t - 1) > (last - s))
				{
					last = (char*)s + i;
					ns = ts - now;
				}
			}
			sfstrclose(f);
		}
	}
	if (e)
		*e = last;
	return ns;
}

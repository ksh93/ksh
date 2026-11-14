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
 * return minutes offset from absolute timezone expression
 *
 *	[[-+]hh[:mm[:ss]]]
 *	[-+]hhmm
 *
 * if e is non-null then it points to the first unrecognized char in s
 * d returned if no offset in s
 */

int
tmgoff(const char* s, char** e, int d)
{
	int		n = d;
	int		east;
	const char*	t = s;

	if ((east = *s == '+') || *s == '-')
	{
		s++;
		if (isdigit((unsigned char)*s) && isdigit((unsigned char)*(s + 1)))
		{
			n = ((*s - '0') * 10 + (*(s + 1) - '0')) * 60;
			s += 2;
			if (*s == ':')
				s++;
			if (isdigit((unsigned char)*s) && isdigit((unsigned char)*(s + 1)))
			{
				n += ((*s - '0') * 10 + (*(s + 1) - '0'));
				s += 2;
				if (*s == ':')
					s++;
				if (isdigit((unsigned char)*s) && isdigit((unsigned char)*(s + 1)))
					s += 2;
			}
			if (east)
				n = -n;
			t = s;
		}
	}
	if (e)
		*e = (char*)t;
	return n;
}

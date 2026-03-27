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
 * return pointer to formatted clock() tics t
 * return value length is at most 6
 */

#include <ast.h>
#include <tm.h>

char*
fmtclock(Sfulong_t t)
{
	int		u;
	char*		buf;
	size_t		z;

	static clock_t	clk_tck;

	if (!clk_tck)
	{
#ifdef CLOCKS_PER_SEC
		clk_tck = (clock_t)CLOCKS_PER_SEC;
#else
		if (!(clk_tck = (clock_t)astconf_ulong(CONF_CLK_TCK)))
			clk_tck = 60;
#endif /* CLOCKS_PER_SEC */
	}
	if (t == 0)
		return "0";
	if (t == ((Sfulong_t)~0))
		return "%";
	t = (t * 1000000) / (Sfulong_t)clk_tck;
	if (t < 1000)
		u = 'u';
	else if ((t /= 1000) < 1000)
		u = 'm';
	else
		return fmtelapsed((unsigned long)t / 10, 100);
	buf = fmtbuf(z = 7);
	sfsprintf(buf, z, "%I*u%cs", sizeof(t), t, u);
	return buf;
}

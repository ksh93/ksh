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
 * AT&T Bell Laboratories
 *
 * if sig>=0 then return signal text for signal sig
 * otherwise return signal name for signal -sig
 */

#include <ast.h>
#include <sig.h>

char*
fmtsignal(int sig)
{
	char*	buf;
	size_t	z;

	if (sig >= 0)
	{
		if (sig <= sig_info.sigmax)
			buf = sig_info.text[sig];
		else
		{
			buf = fmtbuf(z = 20);
			sfsprintf(buf, z, "Signal %d", sig);
		}
	}
	else
	{
		sig = -sig;
		if (sig <= sig_info.sigmax)
			buf = sig_info.name[sig];
		else
		{
			buf = fmtbuf(z = 20);
			sfsprintf(buf, z, "%d", sig);
		}
	}
	return buf;
}

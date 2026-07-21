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

/*
 * format 4 byte local byte order ip address
 * and optional prefix bits (if 0 <= bits <= 32)
 */

char*
fmtip4(uint32_t addr, int bits)
{
	char*	buf;
	size_t	z;
	ssize_t	i;

	buf = fmtbuf(z = 20);
	i = sfsprintf(buf, z, "%d.%d.%d.%d", (addr>>24)&0xff, (addr>>16)&0xff, (addr>>8)&0xff, (addr)&0xff);
	if (bits >= 0 && bits <= 32)
		sfsprintf(buf + i, z - (size_t)i, "/%d", bits);
	return buf;
}

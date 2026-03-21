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
 * internal representation conversion support
 */

#include <ast.h>
#include <swap.h>

/*
 * return the swap operation for external to internal conversion
 * if size<0 then (-size) used and (-size==4)&&(op==3) => op=7
 * this is a workaround for 4 byte magic predicting 8 byte swap
 */

ssize_t
swapop(const void* internal, const void* external, ssize_t size)
{
	int	op;
	ssize_t	z;
	char	tmp[sizeof(intmax_t)];

	if ((z = size) < 0)
		z = -z;
	if (z <= 1)
		return 0;
	if (z <= (ssize_t)sizeof(intmax_t))
		for (op = 0; op < z; op++)
			if (!memcmp(internal, swapmem(op, external, tmp, (size_t)z), (size_t)z))
			{
				if (size < 0 && z == 4 && op == 3)
					op = 7;
				return op;
			}
	return -1;
}

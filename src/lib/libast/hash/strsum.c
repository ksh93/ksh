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
 * hash table library
 */

#include "hashlib.h"

/*
 * return a running 32 bit checksum of string s
 *
 * c is the return value from a previous
 * memsum() or strsum() call, 0 on the first call
 *
 * the result is the same on all implementations
 */

unsigned long
strsum(const char* as, unsigned long c)
{
	const unsigned char*	s = (const unsigned char*)as;
	int			n;

	while (n = *s++) HASHPART(c, (size_t)n);
#if LONG_MAX > 2147483647
	return c & 0xffffffff;
#else
	return c;
#endif
}

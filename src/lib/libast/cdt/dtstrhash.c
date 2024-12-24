/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2013 AT&T Intellectual Property          *
*          Copyright (c) 2020-2024 Contributors to ksh 93u+m           *
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
#include	"dthdr.h"

/* Hashing a string into an unsigned integer.
** This is the FNV (Fowler-Noll-Vo) hash function.
** Written by Kiem-Phong Vo (01/10/2012)
*/

uint dtstrhash(uint h, void* args, ssize_t n)
{
	unsigned char	*s = (unsigned char*)args;

#if _ast_sizeof_int == 8 /* 64-bit hash */
#define FNV_PRIME	((1<<40) + (1<<8) + 0xb3)
#define FNV_OFFSET	14695981039346656037U
#else /* 32-bit hash */
#define FNV_PRIME	((1<<24) + (1<<8) + 0x93)
#define FNV_OFFSET	2166136261U
#endif
	h = (h == 0 || h == ~0) ? FNV_OFFSET : h;
	if(n <= 0) /* see discipline key definition for == 0 */
	{	for(; *s != 0; ++s )
			h = (h ^ s[0]) * FNV_PRIME;
	}
	else
	{	unsigned char*	ends;
		for(ends = s+n; s < ends; ++s)
			h = (h ^ s[0]) * FNV_PRIME;
	}

	return h;
}

/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2021 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 1.0                  *
*                    by AT&T Intellectual Property                     *
*                                                                      *
*                A copy of the License is available at                 *
*          http://www.eclipse.org/org/documents/epl-v10.html           *
*         (with md5 checksum b35adb5213ca9657e911e9befb180842)         *
*                                                                      *
*              Information and Software Systems Research               *
*                            AT&T Research                             *
*                           Florham Park NJ                            *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  David Korn <dgk@research.att.com>                   *
*                   Phong Vo <kpv@research.att.com>                    *
*                                                                      *
***********************************************************************/
#include	"dthdr.h"

/* Hashing a string into an unsigned integer.
** This is the FNV (Fowler-Noll-Vo) hash function.
** Written by Kiem-Phong Vo (01/10/2012)
*/

#if __STD_C
uint dtstrhash(uint h, Void_t* args, ssize_t n)
#else
uint dtstrhash(h,args,n)
uint	h;
Void_t*		args;
ssize_t		n;
#endif
{
	unsigned char	*s = (unsigned char*)args;

#if _ast_sizeof_int == 8 /* 64-bit hash */
#define	FNV_PRIME	((1<<40) + (1<<8) + 0xb3)
#define FNV_OFFSET	14695981039346656037
#else /* 32-bit hash */
#define	FNV_PRIME	((1<<24) + (1<<8) + 0x93)
#define FNV_OFFSET	2166136261
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

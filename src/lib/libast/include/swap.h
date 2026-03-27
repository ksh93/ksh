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
 * integral representation conversion support definitions
 * supports sizeof(integral_type)<=sizeof(intmax_t)
 */

#ifndef _SWAP_H
#define _SWAP_H

#include <ast_common.h>

#define int_swap	_ast_intswap

#define SWAP_MAX	8

#define SWAPOP(n)	(((n)&int_swap)^(n))

extern void*		swapmem(int, const void*, void*, size_t);
extern intmax_t		swapget(int, const void*, size_t);
extern void*		swapput(int, void*, size_t, intmax_t);
extern ssize_t		swapop(const void*, const void*, ssize_t);

#endif

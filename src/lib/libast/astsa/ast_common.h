/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2022 Contributors to ksh 93u+m           *
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
#ifndef _AST_COMMON_H
#define _AST_COMMON_H	1

#include <ast_sa.h>
#include <sys/types.h>

/* AST backwards compatibility macros */
#undef	_NIL_
#define	_NIL_(x)	((x)0)
#undef	_STD_
#define	_STD_		1
#undef	_ARG_
#define	_ARG_(x)	x
#undef	_VOID_
#define	_VOID_		void
#undef	_BEGIN_EXTERNS_
#define	_BEGIN_EXTERNS_
#undef	_END_EXTERNS_
#define	_END_EXTERNS_
#undef	__STD_C
#define	__STD_C		1
#undef	Void_t
#define	Void_t		void

#if _hdr_stdint
#include <stdint.h>
#else
#include <inttypes.h>
#endif

#include <unistd.h>

#define _typ_int32_t	1
#ifdef _ast_int8_t
#define _typ_int64_t	1
#endif

#endif

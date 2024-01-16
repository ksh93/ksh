/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
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
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
/*
 * strtoull() implementation
 */

#define strtoull	______strtoull

#include <ast.h>

#undef	strtoull

#undef	_def_map_ast

#include <ast_map.h>

#define S2I_function	strtoull
#define S2I_number	intmax_t
#define S2I_unumber	uintmax_t
#define S2I_unsigned	1

/* on macOS arm64, long == long long, causing a false-positive "incompatible library declaration" warning on clang */
#if _ast_intmax_long && __clang__
#pragma clang diagnostic ignored "-Wincompatible-library-redeclaration"
#endif

#include "strtoi.h"

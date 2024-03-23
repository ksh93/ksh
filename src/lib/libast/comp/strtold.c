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
 * strtold() implementation
 */

#define S2F_function	strtold
#define S2F_type	2

/*
 * AST strtold() => strtod() when double == long double
 */

#define _AST_STD_H	1

#include <ast_common.h>

#if _ast_fltmax_double
#define strtold		______strtold
#endif

#include <ast_lib.h>

#if __ANDROID_API__
#define _STDLIB_H	1
#endif

#include <ast_sys.h>

#if __ANDROID_API__
#undef _STDLIB_H
#endif

#if _ast_fltmax_double
#undef	strtold
#endif

#undef	_AST_STD_H

#include <ast.h>

/* on macOS arm64, double == long double, causing a false-positive "incompatible library declaration" warning on clang */
#if _ast_fltmax_double && __clang__
#pragma clang diagnostic ignored "-Wincompatible-library-redeclaration"
#endif

#include "sfstrtof.h"

/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
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
 * AT&T Research
 *
 * AST <windows.h> wrapper
 * for libast only
 * include after AST headers
 */

#ifndef _AST_WINDOWS_H
#define _AST_WINDOWS_H		1

/*
 * For some reason, DECLSPEC_NORETURN breaks when compiling with
 * -std=c99. C11 does not have this problem, so for C99 and below
 * avoid this problem by avoiding use of __declspec().
 */
#if __STDC_VERSION__ < 201112L
#if defined(__GNUC__) && (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4))
#define DECLSPEC_NORETURN __attribute__((__noreturn__))
#else
#define DECLSPEC_NORETURN
#endif
#endif /* __STDC_VERSION__ */

#include <windows.h>

#endif

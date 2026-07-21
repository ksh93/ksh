/***********************************************************************
*                                                                      *
*              This file is part of the ksh 93u+m package              *
*          Copyright (c) 2024-2026 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/

/*
 * Since native OS assert.h headers often don't play well with libast,
 * here is a simple AST implementation of assert.h as defined in C90.
 * It may be included more than once with or without NDEBUG defined.
 *
 * In addition, static_assert is defined as the C11 keyword
 * _Static_assert, if available. Portable programs should use the
 * preprocessor to check that static_assert is defined before using it.
 */

#include <ast_common.h>

#undef assert
#ifdef NDEBUG
#  define	assert(e)	((void)0)
#elif _has___func__
#  define	assert(e)	((e) ? (void)0 : _ast_assertfail(#e, __func__, __FILE__, __LINE__))
#elif _has___FUNCTION__
#  define	assert(e)	((e) ? (void)0 : _ast_assertfail(#e, __FUNCTION__, __FILE__, __LINE__))
#else
#  define	assert(e)	((e) ? (void)0 : _ast_assertfail(#e, NULL, __FILE__, __LINE__))
#endif

#ifndef _ASSERT_H
#  define _ASSERT_H
extern noreturn void _ast_assertfail(const char*, const char*, const char*, int);
#  if _has__Static_assert && !defined(static_assert)
#    define static_assert	_Static_assert
#  endif
#endif

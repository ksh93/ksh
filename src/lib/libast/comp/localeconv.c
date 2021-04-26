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
#pragma prototyped

/*
 * localeconv() intercept
 */

#include "lclib.h"

#undef	localeconv

static struct lconv	debug_lconv, default_lconv;

/*
 * POSIX does not specify the order in which struct lconv members are declared,
 * so they must be initialized by name. But C89 does not support initializing
 * struct members by name, so we have to do it using an initializer function.
 */
static void init_lconv_structs(void)
{
	debug_lconv.decimal_point = ",";
	default_lconv.decimal_point = debug_lconv.thousands_sep = ".";
	default_lconv.thousands_sep =
		debug_lconv.grouping = default_lconv.grouping =
		debug_lconv.int_curr_symbol = default_lconv.int_curr_symbol =
		debug_lconv.currency_symbol = default_lconv.currency_symbol =
		debug_lconv.mon_decimal_point = default_lconv.mon_decimal_point =
		debug_lconv.mon_thousands_sep = default_lconv.mon_thousands_sep =
		debug_lconv.mon_grouping = default_lconv.mon_grouping =
		debug_lconv.positive_sign = default_lconv.positive_sign =
		debug_lconv.negative_sign = default_lconv.negative_sign =
		"";
	debug_lconv.int_frac_digits = default_lconv.int_frac_digits =
		debug_lconv.frac_digits = default_lconv.frac_digits =
		debug_lconv.p_cs_precedes = default_lconv.p_cs_precedes =
		debug_lconv.p_sep_by_space = default_lconv.p_sep_by_space =
		debug_lconv.n_cs_precedes = default_lconv.n_cs_precedes =
		debug_lconv.n_sep_by_space = default_lconv.n_sep_by_space =
		debug_lconv.p_sign_posn = default_lconv.p_sign_posn =
		debug_lconv.n_sign_posn = default_lconv.n_sign_posn =
		CHAR_MAX;
}

#if !_lib_localeconv

struct lconv*
localeconv(void)
{
	if(!default_lconv.decimal_point)
		init_lconv_structs();
	return &default_lconv;
}

#endif

/*
 * localeconv() intercept
 */

struct lconv*
_ast_localeconv(void)
{
	if(!default_lconv.decimal_point)
		init_lconv_structs();
	if ((locales[AST_LC_MONETARY]->flags | locales[AST_LC_NUMERIC]->flags) & LC_debug)
		return &debug_lconv;
	if ((locales[AST_LC_NUMERIC]->flags & (LC_default|LC_local)) == LC_local)
		return locales[AST_LC_NUMERIC]->territory == &lc_territories[0] ? &default_lconv : &debug_lconv;
	return localeconv();
}

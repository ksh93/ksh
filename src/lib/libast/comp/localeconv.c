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
*                                                                      *
***********************************************************************/

/*
 * localeconv() intercept
 */

#include "lclib.h"

#undef	localeconv

static struct lconv	comma_lconv, dot_lconv;

/*
 * POSIX does not specify the order in which struct lconv members are declared,
 * so they must be initialized by name. But C89 does not support initializing
 * struct members by name, so we have to do it using an initializer function.
 */
static void init_lconv_structs(void)
{
	comma_lconv.decimal_point = ",";
	dot_lconv.decimal_point = comma_lconv.thousands_sep = ".";
	dot_lconv.thousands_sep =
		comma_lconv.grouping = dot_lconv.grouping =
		comma_lconv.int_curr_symbol = dot_lconv.int_curr_symbol =
		comma_lconv.currency_symbol = dot_lconv.currency_symbol =
		comma_lconv.mon_decimal_point = dot_lconv.mon_decimal_point =
		comma_lconv.mon_thousands_sep = dot_lconv.mon_thousands_sep =
		comma_lconv.mon_grouping = dot_lconv.mon_grouping =
		comma_lconv.positive_sign = dot_lconv.positive_sign =
		comma_lconv.negative_sign = dot_lconv.negative_sign =
		"";
	comma_lconv.int_frac_digits = dot_lconv.int_frac_digits =
		comma_lconv.frac_digits = dot_lconv.frac_digits =
		comma_lconv.p_cs_precedes = dot_lconv.p_cs_precedes =
		comma_lconv.p_sep_by_space = dot_lconv.p_sep_by_space =
		comma_lconv.n_cs_precedes = dot_lconv.n_cs_precedes =
		comma_lconv.n_sep_by_space = dot_lconv.n_sep_by_space =
		comma_lconv.p_sign_posn = dot_lconv.p_sign_posn =
		comma_lconv.n_sign_posn = dot_lconv.n_sign_posn =
		CHAR_MAX;
}

#if !_lib_localeconv

struct lconv*
localeconv(void)
{
	if(!dot_lconv.decimal_point)
		init_lconv_structs();
	return &dot_lconv;
}

#endif

/*
 * localeconv() intercept
 */

struct lconv*
_ast_localeconv(void)
{
	if(!dot_lconv.decimal_point)
		init_lconv_structs();
	if ((locales[AST_LC_NUMERIC]->flags & (LC_default|LC_local)) == LC_local)
		return locales[AST_LC_NUMERIC]->territory == &lc_territories[0] ? &dot_lconv : &comma_lconv;
	return localeconv();
}

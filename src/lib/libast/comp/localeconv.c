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
 * localeconv() intercept
 */

#include "lclib.h"

#undef	localeconv

static struct lconv debug_lconv = {
	.decimal_point		= ",",
	.thousands_sep		= ".",
	.grouping		= "",
	.int_curr_symbol	= "",
	.currency_symbol	= "",
	.mon_decimal_point	= "",
	.mon_thousands_sep	= "",
	.mon_grouping		= "",
	.positive_sign		= "",
	.negative_sign		= "",
	.int_frac_digits	= CHAR_MAX,
	.frac_digits		= CHAR_MAX,
	.p_cs_precedes		= CHAR_MAX,
	.p_sep_by_space		= CHAR_MAX,
	.n_cs_precedes		= CHAR_MAX,
	.n_sep_by_space		= CHAR_MAX,
	.p_sign_posn		= CHAR_MAX,
	.n_sign_posn		= CHAR_MAX
};

static struct lconv default_lconv = {
	.decimal_point		= ".",
	.thousands_sep		= ".",
	.grouping		= "",
	.int_curr_symbol	= "",
	.currency_symbol	= "",
	.mon_decimal_point	= "",
	.mon_thousands_sep	= "",
	.mon_grouping		= "",
	.positive_sign		= "",
	.negative_sign		= "",
	.int_frac_digits	= CHAR_MAX,
	.frac_digits		= CHAR_MAX,
	.p_cs_precedes		= CHAR_MAX,
	.p_sep_by_space		= CHAR_MAX,
	.n_cs_precedes		= CHAR_MAX,
	.n_sep_by_space		= CHAR_MAX,
	.p_sign_posn		= CHAR_MAX,
	.n_sign_posn		= CHAR_MAX
};


/*
 * localeconv() intercept
 */

struct lconv*
_ast_localeconv(void)
{
	if ((locales[AST_LC_MONETARY]->flags | locales[AST_LC_NUMERIC]->flags) & LC_debug)
		return &debug_lconv;
	if ((locales[AST_LC_NUMERIC]->flags & (LC_default|LC_local)) == LC_local)
		return locales[AST_LC_NUMERIC]->territory == &lc_territories[0] ? &default_lconv : &debug_lconv;
	return localeconv();
}

/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
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

#include <ast.h>

#undef	strcmp

/*
 * Initial ast.* values (really _ast_info.*)
 *
 * The _Ast_info_t definition is in include/ast_std.h.
 * Values not set here are implicitly initialized to zero.
 */

_Ast_info_t	_ast_info =
{
	.id = "libast",
	.locale =
	{
		.collate = strcmp,
	},
#if !AST_NOMULTIBYTE
	.mb =
	{
		.cur_max = 1,
	},
#endif
};

extern _Ast_info_t	_ast_info;

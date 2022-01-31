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

static const char id[] = "\n@(#)$Id: ast (ksh 93u+m) $\0\n";

#include <ast.h>

#undef	strcmp

_Ast_info_t	_ast_info =
{
	"libast",	/* ID */
	{ 0 },
	0,0,0,0,0,
	strcmp,		/* collate */
	0,0,
	1,		/* mb_cur_max */
	0,0,0,0,0,0,0,
	AST_VERSION	/* version */
};

__EXTERN__(_Ast_info_t, _ast_info);

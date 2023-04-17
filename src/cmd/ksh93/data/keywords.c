/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2023 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                  David Korn <dgk@research.att.com>                   *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
#include	"shopt.h"
#include	"shell.h"
#include	"shlex.h"

/*
 * table of reserved words in shell language
 * This list must be in ASCII sorted order
 */

const Shtable_t shtab_reserved[] =
{
		"!",		NOTSYM,
		"[[",		BTESTSYM,
		"case",		CASESYM,
		"do",		DOSYM,
		"done",		DONESYM,
		"elif",		ELIFSYM,
		"else",		ELSESYM,
		"esac",		ESACSYM,
		"fi",		FISYM,
		"for",		FORSYM,
		"function",	FUNCTSYM,
		"if",		IFSYM,
		"in",		INSYM,
#if SHOPT_NAMESPACE
		"namespace",	NSPACESYM,
#endif /* SHOPT_NAMESPACE */
		"select",	SELECTSYM,
		"then",		THENSYM,
		"time",		TIMESYM,
		"until",	UNTILSYM,
		"while",	WHILESYM,
		"{",		LBRACE,
		"}",		RBRACE,
		"",		0,
};

const char	e_unexpected[]	= "unexpected";
const char	e_unmatched[]	= "unmatched";
const char	e_endoffile[]	= "end of file";
const char	e_newline[]	= "newline";


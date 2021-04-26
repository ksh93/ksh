/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
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
*                  David Korn <dgk@research.att.com>                   *
*                                                                      *
***********************************************************************/
#pragma prototyped
#include	"defs.h"
#include	<signal.h>
#include	"FEATURE/options"
#include	"FEATURE/dynamic"

/*
 * Table of built-in aliases for interactive shells.
 */

const struct shtable2 shtab_aliases[] =
{
	"history",	NV_NOFREE,		"hist -l",
	"r",		NV_NOFREE,		"hist -s",
	"",		0,			(char*)0
};

/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
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

#define SH_RELEASE_FORK	"93u+m"		/* only change if you develop a new ksh93 fork */
#define SH_RELEASE_SVER	"1.0.0-alpha"	/* semantic version number: https://semver.org */
#define SH_RELEASE_DATE	"2021-04-09"	/* must be in this format for $((.sh.version)) */
#define SH_RELEASE_CPYR	"(c) 2020-2021 Contributors to ksh " SH_RELEASE_FORK

/* Scripts sometimes field-split ${.sh.version}, so don't change amount of whitespace. */
/* Arithmetic $((.sh.version)) uses the last 10 chars, so the date must be at the end. */
#if _AST_ksh_release
#  define SH_RELEASE	SH_RELEASE_FORK "/" SH_RELEASE_SVER " " SH_RELEASE_DATE
#else
#  ifdef _AST_git_commit
#    define SH_RELEASE	SH_RELEASE_FORK "/" SH_RELEASE_SVER "+" _AST_git_commit " " SH_RELEASE_DATE
#  else
#    define SH_RELEASE	SH_RELEASE_FORK "/" SH_RELEASE_SVER "+dev " SH_RELEASE_DATE
#  endif
#endif

/*
 * For shcomp: the version number (0-255) for the binary bytecode header.
 * Only increase very rarely, i.e.: if incompatible changes are made that
 * cause bytecode from newer versions to fail on older versions of ksh.
 *
 * The version number was last increased in 2021 for ksh 93u+m because
 * most of the predefined aliases were converted to builtin commands.
 */
#define SHCOMP_HDR_VERSION	4

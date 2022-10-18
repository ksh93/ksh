/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2022 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                  David Korn <dgk@research.att.com>                   *
*                                                                      *
***********************************************************************/

#include <releaseflags.h>

#define SH_RELEASE_FORK	"93u+m"		/* only change if you develop a new ksh93 fork */
#define SH_RELEASE_SVER	"1.1.0-alpha"	/* semantic version number: https://semver.org */
#define SH_RELEASE_DATE	"2022-10-18"	/* must be in this format for $((.sh.version)) */
#define SH_RELEASE_CPYR	"(c) 2020-2022 Contributors to ksh " SH_RELEASE_FORK

/* Scripts sometimes field-split ${.sh.version}, so don't change amount of whitespace. */
/* Arithmetic $((.sh.version)) uses the last 10 chars, so the date must be at the end. */
#if _AST_release
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
 */
#define SHCOMP_HDR_VERSION	5

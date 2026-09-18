/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1992-2012 AT&T Intellectual Property          *
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
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
/*
 * David Korn
 * Glenn Fowler
 * AT&T Research
 */

static const char usage[] =
"[-s8?\n@(#)$Id: sync (ksh 93u+m) 2026-09-12 $\n]"
"[--catalog?" ERROR_CATALOG "]"
"[+NAME?sync - schedule file system updates]"
"[+DESCRIPTION?\bsync\b calls \bsync\b(2), which causes all information "
    "in memory that updates file systems to be scheduled for writing out to "
    "all file systems. The writing, although scheduled, is not necessarily "
    "complete upon return.]"
"[+EXIT STATUS?]"
    "{"
	"[+0?\bsync\b(2) was called.]"
	"[+>0?Option/operand syntax error.]"
    "}"
"[+SEE ALSO?\bshutdown\b(8)]"
;

#include <cmd.h>
#include <ls.h>

int
b_sync(int argc, char** argv, Shbltin_t* context)
{
	cmdinit(argc, argv, context, ERROR_CATALOG, 0);
	for (;;)
	{
		switch (optget(argv, usage))
		{
		case ':':
			error(2, "%s", opt_info.arg);
			break;
		case '?':
			return optselfdoc();
		}
		break;
	}
	argv += opt_info.index;
	if (error_info.errors || *argv)
	{
		error(ERROR_usage(2), "%s", optusage(NULL));
		UNREACHABLE();
	}
#if _lib_sync
	sync();
	return 0;
#else
	error(2, "not supported -- the native system does not provide a sync(2) call");
	return 1;
#endif
}

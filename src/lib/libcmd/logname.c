/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1992-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2024 Contributors to ksh 93u+m           *
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
 * AT&T Research
 *
 * logname
 */

static const char usage[] =
"[-?\n@(#)$Id: logname (AT&T Research) 1999-04-30 $\n]"
"[--catalog?" ERROR_CATALOG "]"
"[+NAME?logname - return the user's login name]"
"[+DESCRIPTION?\blogname\b writes the user's login name to standard "
	"output.  The login name is the string that is returned by the "
	"\bgetlogin\b(2) function.  If \bgetlogin\b(2) does not return "
	"successfully, the name corresponding to the real user ID of the "
	"calling process is used instead.]"

"\n"
"\n\n"
"\n"
"[+EXIT STATUS?]{"
	"[+0?Successful completion.]"
	"[+>0?An error occurred.]"
"}"
"[+SEE ALSO?\bgetlogin\b(2)]"
;


#include <cmd.h>

int
b_logname(int argc, char** argv, Shbltin_t* context)
{
	char*	logname;

	cmdinit(argc, argv, context, ERROR_CATALOG, 0);
	for (;;)
	{
		switch (optget(argv, usage))
		{
		case ':':
			error(2, "%s", opt_info.arg);
			continue;
		case '?':
			error(ERROR_usage(2), "%s", opt_info.arg);
			UNREACHABLE();
		}
		break;
	}
	if (error_info.errors)
	{
		error(ERROR_usage(2), "%s", optusage(NULL));
		UNREACHABLE();
	}
	if (!(logname = getlogin()))
		logname = fmtuid(getuid());
	sfputr(sfstdout, logname, '\n');
	return 0;
}

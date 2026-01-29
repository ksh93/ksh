/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2026 Contributors to ksh 93u+m           *
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
/*
 * umask [-pS] [mask]
 *
 *   David Korn
 *   AT&T Labs
 *   research!dgk
 *
 */

#include	"shopt.h"
#include	<ast.h>
#include	<sfio.h>
#include	<error.h>
#include	<ctype.h>
#include	<ls.h>
#include	<shell.h>
#include	"builtins.h"
#ifndef SH_DICT
#   define SH_DICT	"libshell"
#endif

int	b_umask(int argc,char *argv[],Shbltin_t *context)
{
	char *mask;
	int flag = 0, pflag = 0, sflag = 0;
	NOT_USED(context);
	while((argc = optget(argv,sh_optumask))) switch(argc)
	{
		case 'p':
			pflag = 1;
			break;
		case 'S':
			sflag = 1;
			break;
		case ':':
			errormsg(SH_DICT,2, "%s", opt_info.arg);
			break;
		case '?':
			/* self-doc: write to standard output */
			error(ERROR_USAGE|ERROR_OUTPUT, STDOUT_FILENO, "%s", opt_info.arg);
			return 0;
	}
	if(error_info.errors)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NULL));
		UNREACHABLE();
	}
	argv += opt_info.index;
	if(mask = *argv)
	{
		int c;
		if(isdigit(*mask))
		{
			while(c = *mask++)
			{
				if (c>='0' && c<='7')
					flag = (flag<<3) + (c-'0');
				else
				{
					errormsg(SH_DICT,ERROR_exit(1),e_number,*argv);
					UNREACHABLE();
				}
			}
		}
		else
		{
			char *cp = mask;
			flag = umask(0);
			c = strperm(cp,&cp,~flag&0777);
			if(*cp)
			{
				umask(flag);
				errormsg(SH_DICT,ERROR_exit(1),e_format,mask);
				UNREACHABLE();
			}
			flag = (~c&0777);
		}
		umask(flag);
	}
	else
	{
		char *prefix = pflag ? "umask " : "";
		umask(flag=umask(0));
		if(sflag)
			sfprintf(sfstdout,"%s%s\n",prefix,fmtperm(~flag&0777));
		else
			sfprintf(sfstdout,"%s%0#4o\n",prefix,flag);
	}
	return 0;
}

/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2024 Contributors to ksh 93u+m           *
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
 * break [n]
 * continue [n]
 * return [n]
 * exit [n]
 *
 *   David Korn
 *   AT&T Labs
 *   dgk@research.att.com
 *
 */

#include	"shopt.h"
#include	"defs.h"
#include	<ast.h>
#include	<error.h>
#include	"shnodes.h"
#include	"builtins.h"

/*
 * return and exit
 */
#if 0
    /* for the dictionary generator */
    int	b_exit(int n, char *argv[],Shbltin_t *context){}
#endif
int	b_return(int n, char *argv[],Shbltin_t *context)
{
	/* 'return' outside of function, dotscript and profile behaves like 'exit' */
	char do_exit = **argv=='e' || sh.fn_depth==0 && sh.dot_depth==0 && !sh_isstate(SH_PROFILE);
	NOT_USED(context);
	while((n = optget(argv, **argv=='e' ? sh_optexit : sh_optreturn))) switch(n)
	{
	    case ':':
		if(!strmatch(argv[opt_info.index],"[+-]+([0-9])"))
			errormsg(SH_DICT,2, "%s", opt_info.arg);
		goto done;
	    case '?':
		errormsg(SH_DICT,ERROR_usage(0), "%s", opt_info.arg);
		return 2;
	}
done:
	if(error_info.errors)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NULL));
		UNREACHABLE();
	}
	argv += opt_info.index;
	if(*argv)
	{
		int r;
		intmax_t l = strtoll(*argv, NULL, 10);
		if(do_exit)
		{
			n = (int)(l & SH_EXITMASK);	/* exit: apply bitmask before conversion to avoid undefined int overflow */
			if (sh.intrap)
				sh.intrap_exit_n = 1;
		}
		else if(r = (int)l, l != (intmax_t)r)	/* return: convert to int and check for overflow (should be safe enough) */
		{
			errormsg(SH_DICT,ERROR_warn(0),"%s: out of range",*argv);
			n = 128;			/* overflow is undefined, so use a consistent status for this */
		}
		else
			n = r;
	}
	else
	{
		n = sh.savexit;				/* no argument: pass down $? */
		if(do_exit)
			n &= SH_EXITMASK;
	}
	((struct checkpt*)sh.jmplist)->mode = do_exit ? SH_JMPEXIT : SH_JMPFUN;
	sh_exit(sh.savexit = n);
	UNREACHABLE();
}


/*
 * break and continue
 */
#if 0
    /* for the dictionary generator */
    int	b_continue(int n, char *argv[],Shbltin_t *context){}
#endif
int	b_break(int n, char *argv[],Shbltin_t *context)
{
	char *arg;
	int cont= **argv=='c';
	NOT_USED(context);
	while((n = optget(argv,cont?sh_optcont:sh_optbreak))) switch(n)
	{
	    case ':':
		errormsg(SH_DICT,2, "%s", opt_info.arg);
		break;
	    case '?':
		errormsg(SH_DICT,ERROR_usage(0), "%s", opt_info.arg);
		return 2;
	}
	if(error_info.errors)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NULL));
		UNREACHABLE();
	}
	argv += opt_info.index;
	n=1;
	if(arg= *argv)
	{
		n = (int)strtol(arg,&arg,10);
		if(n<=0 || *arg)
		{
			errormsg(SH_DICT,ERROR_exit(1),e_number,*argv);
			UNREACHABLE();
		}
	}
	if(sh.st.loopcnt)
	{
		sh.st.breakcnt = n;
		if(sh.st.breakcnt > sh.st.loopcnt)
			sh.st.breakcnt = sh.st.loopcnt;
		if(cont)
			sh.st.breakcnt = -sh.st.breakcnt;
	}
	return 0;
}

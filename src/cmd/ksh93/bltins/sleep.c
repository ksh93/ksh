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
*                  Lev Kujawski <int21h@mailbox.org>                   *
*                                                                      *
***********************************************************************/
/*
 * sleep [-s] duration
 *
 *   David Korn
 *   AT&T Labs
 *
 */

#include	"shopt.h"
#include	"defs.h"
#include	<tmx.h>
#include	<ast_float.h>
#include	"builtins.h"
#include	"FEATURE/time"
#include	"FEATURE/poll"

int	b_sleep(int argc,char *argv[],Shbltin_t *context)
{
	char *cp;
	double d=0;
	int sflag=0;
	time_t tloc = 0;
	char *last;
	NOT_USED(context);
	if(!(sh.sigflag[SIGALRM]&(SH_SIGFAULT|SH_SIGOFF)))
		sh_sigtrap(SIGALRM);
	while((argc = optget(argv,sh_optsleep))) switch(argc)
	{
		case 's':
			sflag=1;
			break;
		case ':':
			errormsg(SH_DICT,2, "%s", opt_info.arg);
			break;
		case '?':
			errormsg(SH_DICT,ERROR_usage(2), "%s", opt_info.arg);
			UNREACHABLE();
	}
	if(error_info.errors)
	{
		errormsg(SH_DICT, ERROR_usage(2), "%s", optusage(NULL));
		UNREACHABLE();
	}
	argv += opt_info.index;
	if(cp = *argv)
	{
		d = strtod(cp, &last);
#if _lib_isnan
		if (isnan(d))
			last = cp;  /* trigger error */
#endif /* _lib_isnan */
		if(*last)
		{
			Time_t now,ns;
			char* pp;
			now = TMX_NOW;
			ns = 0;
			if(*cp == 'P' || *cp == 'p')
				ns = tmxdate(cp, &last, now);
			else if(*last=='.' && sh.radixpoint!='.' && d==(unsigned long)d)
			{
				*(pp=last) = sh.radixpoint;
				if(!strchr(cp,'.'))
					d = strtod(cp,&last);
				*pp = '.';
				if(*last==0)
					goto skip;
			}
			else if(*last!='.' && *last!=sh.radixpoint)
			{
				if(pp = sfprints("exact %s", cp))
					ns = tmxdate(pp, &last, now);
				if(*last && (pp = sfprints("p%s", cp)))
					ns = tmxdate(pp, &last, now);
			}
			if(*last)
			{
				errormsg(SH_DICT,ERROR_exit(1),e_number,*argv);
				UNREACHABLE();
			}
			d = ns - now;
			d /= TMX_RESOLUTION;
		}
skip:
		if(argv[1])
		{
			errormsg(SH_DICT,ERROR_exit(1),e_oneoperand);
			UNREACHABLE();
		}
	}
	else if(!sflag)
	{
		errormsg(SH_DICT,ERROR_exit(1),e_oneoperand);
		UNREACHABLE();
	}
	if(d > .10)
	{
		time(&tloc);
		tloc += (time_t)(d+.5);
	}
	if(sflag && d==0)
		pause();  /* 'sleep -s' waits until a signal is sent */
	else while(1)
	{
		time_t now;
		errno = 0;
		sh.lastsig=0;
		sh_delay(d,sflag);
		if(sflag || tloc==0 || errno!=EINTR || sh.lastsig)
			break;
		sh_sigcheck();
		if(tloc < (now=time(NULL)))
			break;
		d = (double)(tloc-now);
		if(sh.sigflag[SIGALRM]&SH_SIGTRAP)
			sh_timetraps();
	}
	return 0;
}

/*
 * Delay execution for time <t>.
 * If sflag==1, stop sleeping when any signal is received
 * (such as SIGWINCH in an interactive shell).
 */
void sh_delay(double t, int sflag)
{
	uint32_t n;
	Tv_t ts, tx;
#if _lib_isinf
	if (isinf(t))
	{
		while (1)
		{
			pause();
			if ((sh.trapnote & (SH_SIGSET | SH_SIGTRAP)) || sflag)
				return;
		}
	}
#endif /* _lib_isinf */
	n = (uint32_t)t;
	ts.tv_sec = n;
	ts.tv_nsec = 1000000000 * (t - (double)n);
	while(tvsleep(&ts, &tx) < 0)
	{
		if ((sh.trapnote & (SH_SIGSET | SH_SIGTRAP)) || sflag)
			return;
		ts = tx;
	}
}

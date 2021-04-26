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
/*
 * sleep [-s] duration
 *
 *   David Korn
 *   AT&T Labs
 *
 */

#include	"defs.h"
#include	<error.h>
#include	<errno.h>
#include	<tmx.h>
#include	"builtins.h"
#include	"FEATURE/time"
#include	"FEATURE/poll"
#ifdef _NEXT_SOURCE
#   define sleep	_ast_sleep
#endif /* _NEXT_SOURCE */
#ifdef _lib_poll_notimer
#   undef _lib_poll
#endif /* _lib_poll_notimer */

int	b_sleep(register int argc,char *argv[],Shbltin_t *context)
{
	register char *cp;
	register double d=0;
	register Shell_t *shp = context->shp;
	int sflag=0;
	time_t tloc = 0;
	char *last;
	if(!(shp->sigflag[SIGALRM]&(SH_SIGFAULT|SH_SIGOFF)))
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
		if(*last)
		{
			Time_t now,ns;
			char* pp;
			now = TMX_NOW;
			ns = 0;
			if(*cp == 'P' || *cp == 'p')
				ns = tmxdate(cp, &last, now);
			else if(*last=='.' && shp->decomma && d==(unsigned long)d)
			{
				*(pp=last) = ',';
				if(!strchr(cp,'.'))
					d = strtod(cp,&last);
				*pp = '.';
				if(*last==0)
					goto skip;
			}
			else if(*last!='.' && *last!=',')
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
		pause();
	else while(1)
	{
		time_t now;
		errno = 0;
		shp->lastsig=0;
		sh_delay(d,sflag);
		if(sflag || tloc==0 || errno!=EINTR || shp->lastsig)
			break;
		sh_sigcheck(shp);
		if(tloc < (now=time(NIL(time_t*))))
			break;
		d = (double)(tloc-now);
		if(shp->sigflag[SIGALRM]&SH_SIGTRAP)
			sh_timetraps(shp);
	}
	return(0);
}

/*
 * delay execution for time <t>
 */

void sh_delay(double t, int sflag)
{
	Shell_t *shp = sh_getinterp();
	int n = (int)t;
	Tv_t ts, tx;

	ts.tv_sec = n;
	ts.tv_nsec = 1000000000 * (t - (double)n);
	while(tvsleep(&ts, &tx) < 0)
	{
		if ((shp->trapnote & (SH_SIGSET | SH_SIGTRAP)) || sflag)
			return;
		ts = tx;
	}
}

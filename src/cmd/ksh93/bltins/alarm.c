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
 * alarm [-r] [varname [+]when]
 *
 *   David Korn
 *   AT&T Labs
 *
 */

/*
 * TODO: 2014 email from David Korn cited at <https://bugzilla.redhat.com/1176670>:
 *
 * > I never documented the alarm builtin because it is problematic.  The
 * > problem is that traps can't safely be handled asynchronously.  What should
 * > happen is that the trap is marked for execution (sh.trapnote) and run after
 * > the current command completes.  The time trap should wake up the shell if
 * > it is blocked and it should return and then handle the trap.
 */

#include	"shopt.h"
#include	"defs.h"
#include	<error.h>
#include	"builtins.h"
#include	"FEATURE/time"

#define R_FLAG	1
#define L_FLAG	2

struct	tevent
{
	Namfun_t	fun;
	Namval_t	*node;
	Namval_t	*action;
	struct tevent	*next;
	long		milli;
	int		flags;
	void            *timeout;
};

static const char ALARM[] = "alarm";

static void	trap_timeout(void*);

/*
 * insert timeout item on current given list in sorted order
 */
static void *time_add(struct tevent *item, void *list)
{
	struct tevent *tp = (struct tevent*)list;
	if(!tp || item->milli < tp->milli)
	{
		item->next = tp;
		list = item;
	}
	else
	{
		while(tp->next && item->milli > tp->next->milli)
			tp = tp->next;
		item->next = tp->next;
		tp->next = item;
	}
	tp = item;
	tp->timeout = sh_timeradd(tp->milli,tp->flags&R_FLAG,trap_timeout,tp);
	return list;
}

/*
 * delete timeout item from current given list, delete timer
 */
static 	void *time_delete(struct tevent *item, void *list)
{
	struct tevent *tp = (struct tevent*)list;
	if(item==tp)
		list = tp->next;
	else
	{
		while(tp && tp->next != item)
			tp = tp->next;
		if(tp)
			tp->next = item->next;
	}
	if(item->timeout)
		sh_timerdel(item->timeout);
	return list;
}

static void	print_alarms(void *list)
{
	struct tevent *tp = (struct tevent*)list;
	while(tp)
	{
		if(tp->timeout)
		{
			char *name = nv_name(tp->node);
			if(tp->flags&R_FLAG)
			{
				double d = tp->milli;
				sfprintf(sfstdout,e_alrm1,name,d/1000.);
			}
			else
				sfprintf(sfstdout,e_alrm2,name,nv_getnum(tp->node));
		}
		tp = tp->next;
	}
}

static void	trap_timeout(void* handle)
{
	struct tevent *tp = (struct tevent*)handle;
	sh.trapnote |= SH_SIGALRM;
	if(!(tp->flags&R_FLAG))
		tp->timeout = 0;
	tp->flags |= L_FLAG;
	sh.sigflag[SIGALRM] |= SH_SIGALRM;
	if(sh_isstate(SH_TTYWAIT))
		sh_timetraps();
}

void	sh_timetraps(void)
{
	struct tevent *tp, *tpnext;
	struct tevent *tptop;
	while(1)
	{
		sh.sigflag[SIGALRM] &= ~SH_SIGALRM;
		tptop= (struct tevent*)sh.st.timetrap;
		for(tp=tptop;tp;tp=tpnext)
		{
			tpnext = tp->next;
			if(tp->flags&L_FLAG)
			{
				tp->flags &= ~L_FLAG;
				if(tp->action)
					sh_fun(tp->action,tp->node,NULL);
				tp->flags &= ~L_FLAG;
				if(!tp->flags)
					nv_unset(tp->node);
			}
		}
		if(!(sh.sigflag[SIGALRM]&SH_SIGALRM))
			break;
	}
}


/*
 * This trap function catches "alarm" actions only
 */
static char *setdisc(Namval_t *np, const char *event, Namval_t* action, Namfun_t *fp)
{
	struct tevent *tp = (struct tevent*)fp;
	if(!event)
		return action ? Empty : (char*)ALARM;
	if(strcmp(event,ALARM)!=0)
	{
		/* try the next level */
		return nv_setdisc(np, event, action, fp);
	}
	if(action==np)
		action = tp->action;
	else
		tp->action = action;
	return action ? (char*)action : Empty;
}

/*
 * catch assignments and set alarm traps
 */
static void putval(Namval_t* np, const char* val, int flag, Namfun_t* fp)
{
	struct tevent	*tp = (struct tevent*)fp;
	double d;
	if(val)
	{
		double now;
		struct timeval tmp;
		timeofday(&tmp);
		now = tmp.tv_sec + 1.e-6*tmp.tv_usec;
		nv_putv(np,val,flag,fp);
		d = nv_getnum(np);
		if(*val=='+')
		{
			double x = d + now;
			nv_putv(np,(char*)&x,NV_INTEGER|NV_DOUBLE,fp);
		}
		else
			d -= now;
		tp->milli = 1000*(d+.0005);
		if(tp->timeout)
			sh.st.timetrap = time_delete(tp,sh.st.timetrap);
		if(tp->milli > 0)
			sh.st.timetrap = time_add(tp,sh.st.timetrap);
	}
	else
	{
		tp = (struct tevent*)nv_stack(np, NULL);
		sh.st.timetrap = time_delete(tp,sh.st.timetrap);
		nv_unset(np);
		free(fp);
	}
}

static const Namdisc_t alarmdisc =
{
	sizeof(struct tevent),
	putval,
	0,
	0,
	setdisc,
};

int	b_alarm(int argc,char *argv[],Shbltin_t *context)
{
	int n,rflag=0;
	Namval_t *np;
	struct tevent *tp;
	while (n = optget(argv, sh_optalarm)) switch (n)
	{
	    case 'r':
		rflag = R_FLAG;
		break;
	    case ':':
		errormsg(SH_DICT,2, "%s", opt_info.arg);
		break;
	    case '?':
		errormsg(SH_DICT,ERROR_usage(2), "%s", opt_info.arg);
		UNREACHABLE();
	}
	argc -= opt_info.index;
	argv += opt_info.index;
	if(error_info.errors)
	{
		errormsg(SH_DICT,ERROR_usage(2),optusage(NULL));
		UNREACHABLE();
	}
	if(argc==0)
	{
		print_alarms(sh.st.timetrap);
		return 0;
	}
	if(argc!=2)
	{
		errormsg(SH_DICT,ERROR_usage(2),optusage(NULL));
		UNREACHABLE();
	}
	np = nv_open(argv[0],sh.var_tree,NV_NOARRAY|NV_VARNAME);
	if(!nv_isnull(np))
		nv_unset(np);
	nv_setattr(np, NV_DOUBLE);
	tp = sh_newof(NULL,struct tevent,1,0);
	tp->fun.disc = &alarmdisc;
	tp->flags = rflag;
	tp->node = np;
	nv_stack(np,(Namfun_t*)tp);
	nv_putval(np, argv[1], 0);
	return 0;
}

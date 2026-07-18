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
 * alarm [-r] [varname [+]when]
 *
 *   David Korn
 *   AT&T Labs
 *
 */

#include	"FEATURE/options"
#include	"defs.h"
#include	<error.h>
#include	<tmx.h>
#include	"builtins.h"
#include	"fcin.h"
#include	"shlex.h"
#include	"jobs.h"
#include	"FEATURE/time"

#define R_FLAG	1
#define L_FLAG	2

struct	tevent
{
	Namfun_t	fun;
	Namval_t	*node;
	Namval_t	*action;
	struct tevent	*next;
	Sflong_t	milli;
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
	tp->timeout = sh_timeradd((Sfulong_t)tp->milli,tp->flags&R_FLAG,trap_timeout,tp);
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

static Time_t getnow(void)
{
	struct timeval tmp;
	timeofday(&tmp);
	return tmp.tv_sec + 1.e-6 * tmp.tv_usec;
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
				Sfdouble_t d = tp->milli;
				sfprintf(sfstdout,e_alrm1,name,d/1000.);
			}
			else
			{
				Sfdouble_t d = tp->milli;
				sfprintf(sfstdout,e_alrm2,name,d/1000.);
			}
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
	if(sh_isstate(SH_TTYWAIT))
		sh_timetraps();
}

void	sh_timetraps(void)
{
	struct tevent *tp, *tpnext;
	struct tevent *tptop;
	while(1)
	{
		sh.trapnote &= ~SH_SIGALRM;
		tptop= (struct tevent*)sh.st.timetrap;
		for(tp=tptop;tp;tp=tpnext)
		{
			tpnext = tp->next;
			if(tp->flags&L_FLAG)
			{
				if(tp->action)
				{
					/* Call the alarm discipline function. This may occur at any time including parse time,
					 * so save the lexer state and push/pop context to make sure we can restore it. */
					struct checkpt	checkpoint;
					int		jmpval;
					int		exitval = sh.exitval, savexit = sh.savexit;
					Shopt_t		opts = sh.options;
					int		states = sh.st.states;
					char		*dbg = sh.st.trap[SH_DEBUGTRAP];
					Lex_t		*lexp = sh.lex_context, savelex = *lexp;
					char		jc = job.jobcontrol;
					int		savesig = job.savesig;
					struct process	*pw = job.pwlist;
					Fcin_t		savefc;
					int		oerrno = errno;
					fcsave(&savefc);
					job.jobcontrol = 0;
					job.pwlist = NULL;	/* avoid external commands in the disc funct affecting job list */
					sh_lexopen(lexp,0);	/* fully reset lexer state */
					sh_offoption(SH_XTRACE);
					sh_offoption(SH_VERBOSE);
					sh_offstate(SH_INTERACTIVE);
					sh_offstate(SH_TTYWAIT);
					sh.st.trap[SH_DEBUGTRAP] = NULL;
					sh_pushcontext(&checkpoint,SH_JMPTRAP);
					jmpval = sigsetjmp(checkpoint.buff,0);
					if(!jmpval)
						sh_fun(tp->action,tp->node,NULL);
					sh_popcontext(&checkpoint);
					*lexp = savelex;
					sh.exitval = exitval;
					sh.savexit = savexit;
					sh.st.trap[SH_DEBUGTRAP] = dbg;
					sh.options = opts;
					sh.st.states = states;
					job.pwlist = pw;
					job.savesig = savesig;
					job.jobcontrol = jc;
					fcrestore(&savefc);
					errno = oerrno;
					if(jmpval>SH_JMPTRAP)
						siglongjmp(*sh.jmplist,jmpval);
				}
				tp->flags &= ~L_FLAG;
				if(!tp->flags)
					nv_unset(tp->node,0);
			}
		}
		if(!(sh.trapnote&SH_SIGALRM))
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
static void putval(Namval_t* np, const char* val, nvflag_t flag, Namfun_t* fp)
{
	struct tevent	*tp = (struct tevent*)fp;
	double		d, x;
	char		*pp;
	if(val)
	{
		Time_t now = getnow();
		char *last;
		if(*val=='+')
		{
			d = strtod(val+1, &last);
			x = d + now;
			nv_putv(np,val,flag,fp);
		}
		else
		{
			d = strtod(val, &last);
			if(*last)
			{
				if(pp = sfprints("exact %s", val))
					d = tmxdate(pp, &last, TMX_NOW);
				if(*last && (pp = sfprints("p%s", val)))
					d = tmxdate(pp, &last, TMX_NOW);
				d /= 1000000000;
				x = d;
				d -= now;
			}
		}
		nv_putv(np,(char*)&x,NV_INTEGER|NV_DOUBLE,fp);
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
		nv_unset(np,0);
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
	NOT_USED(context);
	while (n = optget(argv, sh_optalarm)) switch (n)
	{
	    case 'r':
		rflag = R_FLAG;
		break;
	    case ':':
		errormsg(SH_DICT,2, "%s", opt_info.arg);
		break;
	    case '?':
		return optselfdoc();
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
		nv_unset(np,0);
	nv_setattr(np, NV_DOUBLE);
	tp = sh_newof(NULL,struct tevent,1,0);
	tp->fun.disc = &alarmdisc;
	tp->flags = rflag;
	tp->node = np;
	nv_stack(np,(Namfun_t*)tp);
	nv_putval(np, argv[1], 0);
	return 0;
}

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

#include	"shopt.h"
#include	<ast.h>
#include	<sig.h>
#include	<error.h>
#include	"fault.h"
#include	"defs.h"
#include	"FEATURE/time"

typedef struct _timer
{
	Sfdouble_t	wakeup;
	Sfdouble_t	incr;
	struct _timer	*next;
	void 		(*action)(void*);
	void		*handle;
} Timer_t;

#define IN_ADDTIMEOUT	1
#define IN_SIGALRM	2
#define DEFER_SIGALRM	4
#define SIGALRM_CALL	8

static Timer_t *tptop, *tpmin, *tpfree;
static char time_state;

static Sfdouble_t getnow(void)
{
	Sfdouble_t now;
	struct timeval tp;
	timeofday(&tp);
	now = tp.tv_sec + 1.e-6*tp.tv_usec;
	return now+.001;
}

/*
 * set an alarm for <t> seconds
 */
static Sfdouble_t setalarm(Sfdouble_t t)
{
#if defined(_lib_setitimer) && defined(ITIMER_REAL)
	struct itimerval tnew, told;
	tnew.it_value.tv_sec = t;
	tnew.it_value.tv_usec = 1.e6*(t- (Sfdouble_t)tnew.it_value.tv_sec);
	if(t && tnew.it_value.tv_sec==0 && tnew.it_value.tv_usec<1000)
		tnew.it_value.tv_usec = 1000;
	tnew.it_interval.tv_sec = 0;
	tnew.it_interval.tv_usec = 0;
	if(setitimer(ITIMER_REAL,&tnew,&told) < 0)
	{
		errormsg(SH_DICT,ERROR_system(1),e_alarm);
		UNREACHABLE();
	}
	t = told.it_value.tv_sec + 1.e-6*told.it_value.tv_usec;
#else
	unsigned seconds = (unsigned)t;
	if(t && seconds<1)
		seconds=1;
	t = (Sfdouble_t)alarm(seconds);
#endif
	return t;
}

/* signal handler for alarm call */
static void sigalrm(int sig)
{
	Timer_t *tp, *tplast, *tpold, *tpnext;
	Sfdouble_t now;
	static Sfdouble_t left;
	NOT_USED(sig);
	left = 0;
	if(time_state&SIGALRM_CALL)
		time_state &= ~SIGALRM_CALL;
	else if(alarm(0))
		kill(sh.current_pid,SIGALRM|SH_TRAP);
	if(time_state)
	{
		if(time_state&IN_ADDTIMEOUT)
			time_state |= DEFER_SIGALRM;
		errno = EINTR;
		return;
	}
	time_state |= IN_SIGALRM;
	sigrelease(SIGALRM);
	while(1)
	{
		now = getnow();
		tpold = tpmin = 0;
		for(tplast=0,tp=tptop; tp; tp=tpnext)
		{
			tpnext = tp->next;
			if(tp->action)
			{
				if(tp->wakeup <=now)
				{
					if(!tpold || tpold->wakeup>tp->wakeup)
						tpold = tp;
				}
				else
				{
					if(!tpmin || tpmin->wakeup>tp->wakeup)
						tpmin=tp;
				}
				tplast = tp;
			}
			else
			{
				if(tplast)
					tplast->next = tp->next;
				else
					tptop = tp->next;
				tp->next = tpfree;
				tpfree = tp;
			}
		}
		if((tp=tpold) && tp->incr)
		{
			while((tp->wakeup += tp->incr) <= now);
			if(!tpmin || tpmin->wakeup>tp->wakeup)
				tpmin=tp;
		}
		if(tpmin && (left==0 || (tp && tpmin->wakeup < (now+left))))
		{
			if(left==0)
				signal(SIGALRM,sigalrm);
			left = setalarm(tpmin->wakeup-now);
			if(left && (now+left) < tpmin->wakeup)
				setalarm(left);
			else
				left=tpmin->wakeup-now;
		}
		if(tp)
		{
			void	(*action)(void*);
			action = tp->action;
			if(!tp->incr)
				tp->action = 0;
			errno = EINTR;
			time_state &= ~IN_SIGALRM;
			(*action)(tp->handle);
			time_state |= IN_SIGALRM;
		}
		else
			break;
	}
	if(!tpmin)
		signal(SIGALRM,(sh.sigflag[SIGALRM]&SH_SIGFAULT)?sh_fault:SIG_DFL);
	time_state &= ~IN_SIGALRM;
	errno = EINTR;
}

static void oldalrm(void *handle)
{
	Handler_t fn = *(Handler_t*)handle;
	free(handle);
	(*fn)(SIGALRM);
}

void *sh_timeradd(Sfulong_t msec,int flags,void (*action)(void*),void *handle)
{
	Timer_t *tp;
	Sfdouble_t t;
	Handler_t fn;
	t = ((Sfdouble_t)msec)/1000.;
	if(t<=0 || !action)
		return NULL;
	if(tp=tpfree)
		tpfree = tp->next;
	else
		tp = (Timer_t*)sh_malloc(sizeof(Timer_t));
	tp->wakeup = getnow() + t;
	tp->incr = (flags?t:0);
	tp->action = action;
	tp->handle = handle;
	time_state |= IN_ADDTIMEOUT;
	tp->next = tptop;
	tptop = tp;
	if(!tpmin || tp->wakeup < tpmin->wakeup)
	{
		tpmin = tp;
		fn = (Handler_t)signal(SIGALRM,sigalrm);
		if((t= setalarm(t))>0 && fn  && fn!=(Handler_t)sigalrm)
		{
			Handler_t *hp = (Handler_t*)sh_malloc(sizeof(Handler_t));
			*hp = fn;
			sh_timeradd((Sflong_t)(1000*t), 0, oldalrm, hp);
		}
		tp = tptop;
	}
	else if(tpmin && !tpmin->action)
		time_state |= DEFER_SIGALRM;
	time_state &= ~IN_ADDTIMEOUT;
	if(time_state&DEFER_SIGALRM)
	{
		time_state=SIGALRM_CALL;
		sigalrm(SIGALRM);
		if(tp!=tptop)
			tp=0;
	}
	return tp;
}

/*
 * delete timer <tp>.  If <tp> is NULL, all timers are deleted
 */
void	sh_timerdel(void *handle)
{
	Timer_t *tp = (Timer_t*)handle;
	if(tp)
		tp->action = 0;
	else
	{
		for(tp=tptop; tp; tp=tp->next)
			tp->action = 0;
		if(tpmin)
		{
			tpmin = 0;
			setalarm((Sfdouble_t)0);
		}
		signal(SIGALRM,(sh.sigflag[SIGALRM]&SH_SIGFAULT)?sh_fault:SIG_DFL);
	}
}

/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
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
*                   Phong Vo <kpv@research.att.com>                    *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
/*
 * Glenn Fowler
 * AT&T Research
 *
 * signal critical region support
 */

#include <ast.h>
#include <sig.h>

static struct
{
	int	sig;
	int	op;
}
signals[] =		/* held inside critical region	*/
{
	SIGINT,		SIG_REG_EXEC,
	SIGPIPE,	SIG_REG_EXEC,
	SIGQUIT,	SIG_REG_EXEC,
	SIGHUP,		SIG_REG_EXEC,
	SIGCHLD,	SIG_REG_PROC,
	SIGTSTP,	SIG_REG_TERM,
	SIGTTIN,	SIG_REG_TERM,
	SIGTTOU,	SIG_REG_TERM,
};

/*
 * critical signal region handler
 *
 * op>0		new region according to SIG_REG_*, return region level
 * op==0	pop region, return region level
 * op<0		return non-zero if any signals held in current region
 *
 * signals[] held until region popped
 */

int
sigcritical(int op)
{
	int			i;
	static int		region;
	static int		level;
	static sigset_t		mask;
	sigset_t		nmask;

	if (op > 0)
	{
		if (!level++)
		{
			region = op;
			if (op & SIG_REG_SET)
				level--;
			sigemptyset(&nmask);
			for (i = 0; i < elementsof(signals); i++)
				if (op & signals[i].op)
					sigaddset(&nmask, signals[i].sig);
			sigprocmask(SIG_BLOCK, &nmask, &mask);
		}
		return level;
	}
	else if (op < 0)
	{
		sigpending(&nmask);
		for (i = 0; i < elementsof(signals); i++)
			if (region & signals[i].op)
			{
				if (sigismember(&nmask, signals[i].sig))
					return 1;
			}
		return 0;
	}
	else
	{
		/*
		 * A vfork via clone(2) may have intervened so we
		 * allow apparent nesting mismatches. The child
		 * shares memory and will decrease the level to 0,
		 * which is then decreased again to -1 by the parent
		 * once the parent's execution resumes.
		 * (This assumes both the child and parent processes
		 * invoke sigcritical(0).)
		 */

		if (--level <= 0)
		{
			level = 0;
			sigprocmask(SIG_SETMASK, &mask, NULL);
		}
		return level;
	}
}

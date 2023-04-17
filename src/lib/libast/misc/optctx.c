/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2023 Contributors to ksh 93u+m           *
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
*                                                                      *
***********************************************************************/
/*
 * Glenn Fowler
 * AT&T Research
 *
 * _opt_infop_ context control
 *
 * allocate new context:
 *	new_context = optctx(0, 0);
 * free new context:
 *	optctx(0, new_context);
 * switch to new_context:
 *	old_context = optctx(new_context, 0);
 * switch to old_context and free new_context:
 *	optctx(old_context, new_context);
 */

#include <optlib.h>

static Opt_t*	freecontext;

Opt_t*
optctx(Opt_t* p, Opt_t* o)
{
	if (o)
	{
		if (freecontext)
			free(o);
		else
			freecontext = o;
		if (!p)
			return NULL;
	}
	if (p)
	{
		o = _opt_infop_;
		_opt_infop_ = p;
	}
	else
	{
		if (o = freecontext)
			freecontext = 0;
		else if (!(o = newof(0, Opt_t, 1, 0)))
			return NULL;
		memset(o, 0, sizeof(Opt_t));
		o->state = _opt_infop_->state;
	}
	return o;
}

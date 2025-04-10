/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2025 Contributors to ksh 93u+m           *
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
 * return p stat info
 */

#include "reglib.h"

regstat_t*
regstat(const regex_t* p)
{
	Rex_t*	e;

	p->env->stats.re_flags = p->env->flags;
	p->env->stats.re_info = 0;
	e = p->env->rex;
	if (e && e->type == REX_BEG)
		e = e->next;
	if (e && e->type == REX_STRING)
		e = e->next;
	if (!e || e->type == REX_END && !e->next)
		p->env->stats.re_info |= REG_LITERAL;
	return &p->env->stats;
}

/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1997-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2026 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
/*
 * Glenn Fowler
 * AT&T Research
 */

#include "dlllib.h"

Dllstate_t	state;

/*
 * return error message from last failed dl*() call
 * retain==0 resets the last dl*() error
 */

extern cold char*
dllerror(int retain)
{
	char*	s;

	if (state.error)
	{
		state.error = retain;
		return state.errorbuf;
	}
	s = dlerror();
	if (retain)
	{
		state.error = retain;
		sfsprintf(state.errorbuf, sizeof(state.errorbuf), "%s", s);
	}
	return s;
}

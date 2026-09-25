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

#include "intercepts.h"

/*
 * NOTE: the "intercepts" definition is here instead of astintercept.c because some
 *	 static linkers miss lone references to "intercepts" without "astintercept()"
 * ALSO: { 0 } definition required by some dynamic linkers averse to common symbols
 */

Intercepts_t	intercepts
		= { 0 };

/*
 * get name from the environment
 */

extern char*
getenv(const char* name)
{
#undef	getenv
	return intercepts.intercept_getenv ? (*intercepts.intercept_getenv)(name) : getenv(name);
}

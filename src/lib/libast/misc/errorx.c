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

#include "lclib.h"

/*
 * low level for ERROR_translate()
 * this fills in NULL arg defaults and calls error_info.translate
 */

cold char*
errorx(const char* loc, const char* cmd, const char* cat, const char* msg)
{
	char*	s;

	if (!error_info.translate)
		error_info.translate = translate; /* 2007-03-19 OLD_Error_info_t workaround */
	if (ERROR_translating())
	{
		if (!loc)
			loc = (const char*)locales[AST_LC_MESSAGES]->code;
		if (!cmd)
			cmd = (const char*)error_info.id;
		if (!cat)
			cat = (const char*)error_info.catalog;
		if (s = (*error_info.translate)(loc, cmd, cat, msg))
			return s;
	}
	return (char*)msg;
}

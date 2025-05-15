/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2013 AT&T Intellectual Property          *
*             Copyright (c) 2025 Contributors to ksh 93u+m             *
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
 * Return the codeset name for the current locale
 */

#include "lclib.h"
#include <ast_nl_types.h>

#if !_hdr_langinfo
#undef	_lib_nl_langinfo
#endif
#if _lib_nl_langinfo
#include <langinfo.h>
#endif

char*
getcodeset(void)
{
	char	*s;

	if (ast.locale.set & AST_LC_utf8)
		return "UTF-8";
#if _lib_nl_langinfo
	s = nl_langinfo(CODESET);
#else
	if ((locales[AST_LC_CTYPE]->flags & LC_default) || (s = setlocale(LC_CTYPE, 0)) && (s = strchr(s, '.')) && !*++s)
		s = 0;
#endif
	if (!s || strmatch(s, "~(i)@(ansi*3.4*|?(us)*ascii|?(iso)*646*)"))
		return "US-ASCII";
	return s;
}

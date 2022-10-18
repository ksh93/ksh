/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2022 Contributors to ksh 93u+m           *
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

#define setenv		______setenv

#include <ast.h>

#undef	setenv

#if _lib_setenv

NoN(setenv)

#else

#undef	_def_map_ast
#include <ast_map.h>

extern int
setenv(const char* name, const char* value, int overwrite)
{
	char*	s;

	if (overwrite || !getenv(name))
	{
		if (!(s = sfprints("%s=%s", name, value)) || !(s = strdup(s)))
			return -1;
		return setenviron(s) ? 0 : -1;
	}
	return 0;
}

#endif

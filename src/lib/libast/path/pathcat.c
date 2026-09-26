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
 * AT&T Bell Laboratories
 *
 * single dir support for pathaccess()
 */

#define _AST_API_H	1

#include <ast.h>

char*
pathcat(char* path, const char* dirs, int sep, const char* a, const char* b)
{
	return pathcat_20100601(dirs, sep, a, b, path, PATH_MAX);
}

#undef	_AST_API

#include <ast_api.h>

char*
pathcat_20100601(const char* dirs, int sep, const char* a, const char* b, char* path, size_t size)
{
	char*	s;
	char*	e;

	s = path;
	e = path + size;
	while (*dirs && *dirs != sep)
	{
		if (s >= e)
			return NULL;
		*s++ = *dirs++;
	}
	if (s != path)
	{
		if (s >= e)
			return NULL;
		*s++ = '/';
	}
	if (a)
	{
		while (*s = *a++)
			if (++s >= e)
				return NULL;
		if (b)
		{
			if (s >= e)
				return NULL;
			*s++ = '/';
		}
	}
	else if (!b)
		b = ".";
	if (b)
		do
		{
			if (s >= e)
				return NULL;
		} while (*s++ = *b++);
	return *dirs ? (char*)++dirs : NULL;
}

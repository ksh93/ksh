/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2024 Contributors to ksh 93u+m           *
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
 * AT&T Bell Laboratories
 *
 * in place replace of first occurrence of /match/ with /replace/ in path
 * end of path returned
 */

#define _AST_API_H	1

#include <ast.h>

char*
pathrepl(char* path, const char* match, const char* replace)
{
	return pathrepl_20100601(path, PATH_MAX, match, replace);
}

#undef	_AST_API_H

#include <ast_api.h>

char*
pathrepl_20100601(char* path, size_t size, const char* match, const char* replace)
{
	const char*	m = match;
	const char*	r;
	char*		t;

	if (!match)
		match = "";
	if (!replace)
		replace = "";
	if (streq(match, replace))
		return path + strlen(path);
	if (!size)
		size = strlen(path) + 1;
	for (;;)
	{
		while (*path && *path++ != '/');
		if (!*path) break;
		if (*path == *m)
		{
			t = path;
			while (*m && *m++ == *path) path++;
			if (!*m && *path == '/')
			{
				char*	p;

				p = t;
				r = replace;
				while (p < path && *r) *p++ = *r++;
				if (p < path) while (*p++ = *path++);
				else if (*r && p >= path)
				{
					char*	u;

					t = path + strlen(path);
					u = t + strlen(r);
					while (t >= path) *u-- = *t--;
					while (*r) *p++ = *r++;
				}
				else p += strlen(p) + 1;
				return p - 1;
			}
			path = t;
			m = match;
		}
	}
	return path;
}

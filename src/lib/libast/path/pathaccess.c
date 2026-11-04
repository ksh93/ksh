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
*                                                                      *
***********************************************************************/
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * return path to file a/b with access mode using : separated dirs
 * both a and b may be 0
 * if a==".." then relative paths in dirs are ignored
 * if (mode&PATH_REGULAR) then path must not be a directory
 * if (mode&PATH_ABSOLUTE) then path must be rooted
 * path returned in path buffer
 */

#define _AST_API_H	1

#include <ast.h>

char*
pathaccess(char* path, const char* dirs, const char* a, const char* b, int mode)
{
	return pathaccess_20100601(dirs, a, b, mode, path, PATH_MAX);
}

#undef	_AST_API_H

#include <ast_api.h>

char*
pathaccess_20100601(const char* dirs, const char* a, const char* b, int mode, char* path, size_t size)
{
	int		sib = a && a[0] == '.' && a[1] == '.' && a[2] == 0;
	int		sep = ':';
	char		cwd[PATH_MAX];

	do
	{
		dirs = pathcat(dirs, sep, a, b, path, size);
		if(!memchr(path, 0, size))
		{
			/* pathcat() could not fit the result: path is not null-terminated */
			errno = ENAMETOOLONG;
			return NULL;
		}
		pathcanon(path, size, 0);
		if ((!sib || *path == '/') && pathexists(path, mode))
		{
			if (*path == '/' || !(mode & PATH_ABSOLUTE))
				return path;
			dirs = getcwd(cwd, sizeof(cwd));
			sep = 0;
		}
	} while (dirs);
	return NULL;
}

/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2013 AT&T Intellectual Property          *
*          Copyright (c) 2025-2026 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*               Glenn Fowler <glenn.s.fowler@gmail.com>                *
*                    David Korn <dgkorn@gmail.com>                     *
*                     Phong Vo <phongvo@gmail.com>                     *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
/*
 * stpcpy implementation
 */

#include <ast.h>

#undef	_def_map_ast
#include <ast_map.h>

#if _lib_stpcpy

NoN(stpcpy)

#else

/*
 * copy f into t, return a pointer to the end of t ('\0')
 * the buffers cannot overlap
 */

extern char*
stpcpy(char *restrict t, const char *restrict f)
{
	if (!f)
		return t;
	while (*t++ = *f++);
	return t - 1;
}

#endif

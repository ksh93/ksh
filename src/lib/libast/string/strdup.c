/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
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
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/

#include <ast.h>

/*
 * return a copy of s using malloc
 */

/*
 * Avoid a null-test optimization bug caused by glibc's headers
 * by naming this function '_ast_strdup' instead of 'strdup'.
 * https://bugzilla.redhat.com/1221766
 */
extern char*
_ast_strdup(const char* s)
{
	char*	t;
	size_t	n;

	if (s)
	{
		n = strlen(s) + 1;
		t = malloc(n);
		if (t)
			return memcpy(t, s, n);
	}
	return NULL;
}

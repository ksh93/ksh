/***********************************************************************
*                                                                      *
*              This file is part of the ksh 93u+m package              *
*          Copyright (c) 2024-2026 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/

#include <ast.h>

noreturn void _ast_assertfail(const char *restrict a, const char *restrict fun, const char *restrict file, int line)
{
	sfprintf(sfstderr,"\n*** assertion %s failed in %s(), %s:%d\n", a, fun, file, line);
	sfsync(NULL);
	abort();
}

/***********************************************************************
*                                                                      *
*              This file is part of the ksh 93u+m package              *
*             Copyright (c) 2024 Contributors to ksh 93u+m             *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                  Martijn Dekker <martijn@inlv.org>                   *
*                                                                      *
***********************************************************************/

#include <ast.h>

void noreturn _ast_assertfail(const char *a, const char *fun, const char *file, int line)
{
#if _has___func__ || _has___FUNCTION__
	sfprintf(sfstderr,"\n*** assertion %s failed in %s(), %s:%d\n", a, fun, file, line);
#else
	NOT_USED(fun);
	sfprintf(sfstderr,"\n*** assertion %s failed in %s:%d\n", a, file, line);
#endif
	sfsync(NULL);
	abort();
}

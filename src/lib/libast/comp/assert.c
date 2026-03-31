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

#if _hdr_execinfo
#include <execinfo.h>
#endif

noreturn void _ast_assertfail(const char *a, const char *fun, const char *file, int line)
{
#if _has___func__ || _has___FUNCTION__
	sfprintf(sfstderr,"\n*** assertion %s failed in %s(), %s:%d\n", a, fun, file, line);
#else
	NOT_USED(fun);
	sfprintf(sfstderr,"\n*** assertion %s failed in %s:%d\n", a, file, line);
#endif
#if _lib_backtrace && _lib_backtrace_symbols
	{
		void* callstack[200];
		int i, frames = backtrace(callstack, 200);
		char** strs = backtrace_symbols(callstack, frames);
		if(strs)
		{
			for (i = 0; i < frames; ++i)
				sfprintf(sfstderr, "%s\n", strs[i]);
			free(strs);
		}
		else
			sfprintf(sfstderr, "Could not obtain a backtrace (%s)\n", strerror(errno));
	}
#endif
	sfsync(NULL);
	abort();
}

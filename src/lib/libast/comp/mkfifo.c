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
*                                                                      *
***********************************************************************/

#include <ast.h>

#if _lib_mkfifo

NoN(mkfifo)

#else

#include <ls.h>
#include <error.h>

int
mkfifo(const char* path, mode_t mode)
{
#ifdef S_IFIFO
	return mknod(path, S_IFIFO|(mode & ~S_IFMT), 0);
#else
	errno = ENOSYS;
	return -1;
#endif
}

#endif

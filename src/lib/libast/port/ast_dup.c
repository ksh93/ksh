/***********************************************************************
*                                                                      *
*              This file is part of the ksh 93u+m package              *
*             Copyright (c) 2026 Contributors to ksh 93u+m             *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/

#include <ast.h>
#include <error.h>

/*
 * Wrapper functions for handling EINTR when calling dup2 and dup3.
 */

int ast_dup2(int fd1, int fd2)
{
	int	r, err = errno;
	while((r = dup2(fd1, fd2)) < 0 && errno == EINTR)
		errno = err;
	return r;
}

#if _lib_dup3

int ast_dup3(int fd1, int fd2, int flags)
{
	int	r, err = errno;
	while((r = dup3(fd1, fd2, flags)) < 0 && errno == EINTR)
		errno = err;
	return r;
}

#endif

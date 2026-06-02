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
 * seekdir
 *
 * seek on directory stream
 * this is not optimal because there aren't portable
 * semantics for directory seeks
 */

#include "dirlib.h"

#if !_dir_ok

void
seekdir(DIR* dirp, long loc)
{
	off_t	base;		/* file location of block */
	off_t	offset; 	/* offset within block */

	if (telldir(dirp) != loc)
	{
		lseek(dirp->dd_fd, 0L, SEEK_SET);
		dirp->dd_loc = dirp->dd_size = 0;
		while (telldir(dirp) != loc)
			if (!readdir(dirp))
				break; 	/* "can't happen" */
	}
}

#else

NoN(seekdir)

#endif /*  !_dir_ok */

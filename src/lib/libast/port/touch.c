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
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
/*
 * Glenn Fowler
 * AT&T Research
 *
 * touch file access and modify times of file
 * if flags&PATH_TOUCH_CREATE then file will be created if it doesn't exist
 * if flags&PATH_TOUCH_VERBATIM then times are taken verbatim
 * times have one second granularity
 *
 *	(time_t)(-1)	retain old time
 *	0		use current time
 *
 * the old interface flag values were:
 *	 1	PATH_TOUCH_CREATE
 *	-1	PATH_TOUCH_CREATE|PATH_TOUCH_VERBATIM
 *		PATH_TOUCH_VERBATIM -- not supported
 */

#include <ast.h>
#include <times.h>
#include <tv.h>

int
touch(const char* path, time_t at, time_t mt, int flags)
{
	Tv_t	av;
	Tv_t	mv;
	Tv_t*	ap;
	Tv_t*	mp;

	if (at == (time_t)(-1) && !(flags & PATH_TOUCH_VERBATIM))
		ap = TV_TOUCH_RETAIN;
	else if (!at && !(flags & PATH_TOUCH_VERBATIM))
		ap = NULL;
	else
	{
		av.tv_sec = at;
		av.tv_nsec = 0;
		ap = &av;
	}
	if (mt == (time_t)(-1) && !(flags & PATH_TOUCH_VERBATIM))
		mp = TV_TOUCH_RETAIN;
	else if (!mt && !(flags & PATH_TOUCH_VERBATIM))
		mp = NULL;
	else
	{
		mv.tv_sec = mt;
		mv.tv_nsec = 0;
		mp = &mv;
	}
	return tvtouch(path, ap, mp, NULL, flags & 1);
}

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
 * Time_t conversion support
 */

#include <tmx.h>
#include <tv.h>

/*
 * touch path <atime,mtime,ctime>
 * (flags&PATH_TOUCH_VERBATIM) treats times verbatim, otherwise:
 * Time_t==0		current time
 * Time_t==TMX_NOTIME	retains path value
 */

int
tmxtouch(const char* path, Time_t at, Time_t mt, Time_t ct, int flags)
{
	Tv_t	av;
	Tv_t	mv;
	Tv_t	cv;
	Tv_t*	ap;
	Tv_t*	mp;
	Tv_t*	cp;

	if (at == TMX_NOTIME && !(flags & PATH_TOUCH_VERBATIM))
		ap = TV_TOUCH_RETAIN;
	else if (!at && !(flags & PATH_TOUCH_VERBATIM))
		ap = 0;
	else
	{
		av.tv_sec = (time_t)tmxsec(at);
		av.tv_nsec = tmxnsec(at);
		ap = &av;
	}
	if (mt == TMX_NOTIME && !(flags & PATH_TOUCH_VERBATIM))
		mp = TV_TOUCH_RETAIN;
	else if (!mt && !(flags & PATH_TOUCH_VERBATIM))
		mp = 0;
	else
	{
		mv.tv_sec = (time_t)tmxsec(mt);
		mv.tv_nsec = tmxnsec(mt);
		mp = &mv;
	}
	if (ct == TMX_NOTIME && !(flags & PATH_TOUCH_VERBATIM))
		cp = TV_TOUCH_RETAIN;
	else if (!ct && !(flags & PATH_TOUCH_VERBATIM))
		cp = 0;
	else
	{
		cv.tv_sec = (time_t)tmxsec(ct);
		cv.tv_nsec = tmxnsec(ct);
		cp = &cv;
	}
	return tvtouch(path, ap, mp, cp, flags & 1);
}

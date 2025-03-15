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
*                  Martijn Dekker <martijn@inlv.org>                   *
*                                                                      *
***********************************************************************/
/*
 * Glenn Fowler
 * AT&T Research
 *
 * convert path to native fs representation in <buf,siz>
 * length of converted path returned
 * if return length >= siz then buf is indeterminate, but another call
 * with siz=length+1 would work
 * if buf==0 then required size is returned
 */

#include <ast.h>

#if __CYGWIN__

#include <sys/cygwin.h>

ssize_t cygwin_conv_path(cygwin_conv_path_t what, const void * from, void * to, size_t size);

size_t
pathnative(const char* path, char* buf, size_t siz)
{
	size_t		n;

	if (!buf || siz < PATH_MAX)
	{
		char	tmp[PATH_MAX];

		cygwin_conv_path(CCP_POSIX_TO_WIN_W | CCP_ABSOLUTE, path, tmp, 0);
		if ((n = strlen(tmp)) < siz && buf)
			memcpy(buf, tmp, n + 1);
		return n;
	}
	cygwin_conv_path(CCP_POSIX_TO_WIN_W | CCP_ABSOLUTE, path, buf, 0);
	return strlen(buf);
}



#elif __INTERIX

#include <interix/interix.h>

size_t
pathnative(const char* path, char* buf, size_t siz)
{
	*buf = 0;
	if (path[1] == ':')
		strlcpy(buf, path, siz);
	else
		unixpath2win(path, 0, buf, siz);
	return strlen(buf);
}

#else

size_t
pathnative(const char* path, char* buf, size_t siz)
{
	size_t		n;

	if ((n = strlen(path)) < siz && buf)
		memcpy(buf, path, n + 1);
	return n;
}

#endif

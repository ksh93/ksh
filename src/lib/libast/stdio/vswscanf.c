/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
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
*                                                                      *
***********************************************************************/

#include "stdhdr.h"

#if !_has_multibyte

NoN(vswscanf)

#else

int
vswscanf(const wchar_t* s, const wchar_t* fmt, va_list args)
{
	Sfio_t	f;

	if (!s)
		return -1;

	/*
	 * make a fake stream
	 */

	SFCLEAR(&f);
	f.flags = SFIO_STRING|SFIO_READ;
	f.bits = SFIO_PRIVATE;
	f.mode = SFIO_READ;
	f.size = wcslen(s) * sizeof(wchar_t);
	f.data = f.next = f.endw = (uchar*)s;
	f.endb = f.endr = f.data + f.size;

	/*
	 * sfio does the rest
	 */

	return vfwscanf(&f, fmt, args);
}

#endif /* !_has_multibyte */

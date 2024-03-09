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

NoN(vswprintf)

#else

int
vswprintf(wchar_t* s, size_t n, const wchar_t* fmt, va_list args)
{
	Sfio_t	f;
	int	v;

	if (!s)
		return -1;

	/*
	 * make a fake stream
	 */

	SFCLEAR(&f);
	f.flags = SFIO_STRING|SFIO_WRITE;
	f.bits = SFIO_PRIVATE;
	f.mode = SFIO_WRITE;
	f.size = n - 1;
	f.data = f.next = f.endr = (uchar*)s;
	f.endb = f.endw = f.data + f.size;

	/*
	 * call and adjust
	 */

	v = vfwprintf(&f, fmt, args);
	*f.next = 0;
	_Sfi = f.next - f.data;
	return v;
}

#endif /* !_has_multibyte */

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

#include "stdhdr.h"

#if !_has_multibyte

NoN(fgetws)

#else

wchar_t*
fgetws(wchar_t* s, int n, Sfio_t* f)
{
	wchar_t*	p = s;
	wchar_t*	e = s + n - 1;
	wint_t		c;

	FWIDE(f, NULL);
	while (p < e && (c = fgetwc(f)) != WEOF && (*p++ = (wchar_t)c) != '\n');
	*p = 0;
	return s;
}

wchar_t*
getws(wchar_t* s)
{
	wchar_t*	p = s;
	wchar_t*	e = s + BUFSIZ - 1;
	wint_t		c;

	FWIDE(sfstdin, NULL);
	while (p < e && (c = fgetwc(sfstdin)) != WEOF && (*p++ = (wchar_t)c) != '\n');
	*p = 0;
	return s;
}

#endif /* !_has_multibyte */

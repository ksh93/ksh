/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2022 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 1.0                  *
*                    by AT&T Intellectual Property                     *
*                                                                      *
*                A copy of the License is available at                 *
*          http://www.eclipse.org/org/documents/epl-v10.html           *
*         (with md5 checksum b35adb5213ca9657e911e9befb180842)         *
*                                                                      *
*              Information and Software Systems Research               *
*                            AT&T Research                             *
*                           Florham Park NJ                            *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  David Korn <dgk@research.att.com>                   *
*                   Phong Vo <kpv@research.att.com>                    *
*                                                                      *
***********************************************************************/

#include "stdhdr.h"

#if !_has_multibyte

NoN(vfwprintf)

#else

int
vfwprintf(Sfio_t* f, const wchar_t* fmt, va_list args)
{
	char*	m;
	char*	x;
	wchar_t*w;
	size_t	n;
	int	v;
	Sfio_t*	t;

	FWIDE(f, WEOF);
	n = wcstombs(NiL, fmt, 0);
	if (m = malloc(n + 1))
	{
		if (t = sfstropen())
		{
			wcstombs(m, fmt, n + 1);
			sfvprintf(t, m, args);
			free(m);
			if (!(x = sfstruse(t)))
				v = -1;
			else
			{
				n = mbstowcs(NiL, x, 0);
				if (w = (wchar_t*)sfreserve(f, n * sizeof(wchar_t) + 1, 0))
					v = mbstowcs(w, x, n + 1);
				else
					v = -1;
			}
			sfstrclose(t);
		}
		else
		{
			free(m);
			v = -1;
		}
	}
	else
		v = -1;
	return v;
}

#endif /* !_has_multibyte */

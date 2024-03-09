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
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/

#include "stdhdr.h"

#if !_has_multibyte

NoN(vfwscanf)

#else

typedef struct
{
	Sfdisc_t	sfdisc;		/* sfio discipline		*/
	Sfio_t*		f;		/* original wide stream		*/
	char		fmt[1];		/* mb fmt			*/
} Wide_t;

/*
 * wide exception handler
 * free on close
 */

static int
wideexcept(Sfio_t* f, int op, void* val, Sfdisc_t* dp)
{
	if (sffileno(f) >= 0)
		return -1;
	switch (op)
	{
	case SFIO_ATEXIT:
		sfdisc(f, SFIO_POPDISC);
		break;
	case SFIO_CLOSING:
	case SFIO_DPOP:
	case SFIO_FINAL:
		if (op != SFIO_CLOSING)
			free(dp);
		break;
	}
	return 0;
}

/*
 * sfio wide discipline read
 * 1 wchar_t at a time
 * go pure multibyte for best performance
 */

static ssize_t
wideread(Sfio_t* f, void* buf, size_t size, Sfdisc_t* dp)
{
	Wide_t*	w = (Wide_t*)dp;
	wchar_t			wuf[2];
	ssize_t	r;

	r = sfread(w->f, wuf, sizeof(wuf[0]));
	if (r != sizeof(wuf[0]))
		return -1;
	wuf[1] = 0;
	r = wcstombs(buf, wuf, size);
	return r;
}

int
vfwscanf(Sfio_t* f, const wchar_t* fmt, va_list args)
{
	size_t	n;
	int	v;
	Sfio_t*	t;
	Wide_t*	w;
	char	buf[1024];

	FWIDE(f, WEOF);
	n = wcstombs(NULL, fmt, 0);
	if (w = newof(0, Wide_t, 1, n))
	{
		if (t = sfnew(NULL, buf, sizeof(buf), (int)astconf_long(CONF_OPEN_MAX)+1, SFIO_READ))
		{
			w->sfdisc.exceptf = wideexcept;
			w->sfdisc.readf = wideread;
			w->f = f;
			if (sfdisc(t, &w->sfdisc) == &w->sfdisc)
			{
				wcstombs(w->fmt, fmt, n + 1);
				v = sfvscanf(t, w->fmt, args);
			}
			else
			{
				free(w);
				v = -1;
			}
			sfsetfd(t, -1);
			sfclose(t);
		}
		else
		{
			free(w);
			v = -1;
		}
	}
	else
		v = -1;
	return v;
}

#endif /* !_has_multibyte */

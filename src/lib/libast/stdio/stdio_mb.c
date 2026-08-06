/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2013 AT&T Intellectual Property          *
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
#include "ast_wchar.h"

#if _has_multibyte

typedef struct
{
	Sfdisc_t	sfdisc;		/* sfio discipline		*/
	Sfio_t*		f;		/* original wide stream		*/
	char		fmt[1];		/* mb fmt			*/
} Wide_t;

wint_t
fgetwc(Sfio_t* f)
{
	wchar_t	c;

	FWIDE(f, WEOF);
	return (sfread(f, &c, sizeof(c)) == sizeof(c)) ? (wint_t)c : WEOF;
}

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

wint_t
fputwc(wchar_t c, Sfio_t* f)
{
	FWIDE(f, WEOF);
	return (sfwrite(f, &c, sizeof(c)) == sizeof(c)) ? (wint_t)c : WEOF;
}

int
fputws(const wchar_t* s, Sfio_t* f)
{
	size_t	n;

	FWIDE(f, -1);
	n = wcslen(s) * sizeof(wchar_t);
	return (sfwrite(f, s, n) == (ssize_t)n) ? 0 : -1;
}

int
fwide(Sfio_t* f, int mode)
{
	if (mode > 0)
	{
		f->bits &= ~SFIO_MB;
		f->bits |= SFIO_WC;
	}
	else if (mode < 0)
	{
		f->bits &= ~SFIO_WC;
		f->bits |= SFIO_MB;
	}
	if (f->bits & SFIO_MB)
		return -1;
	if (f->bits & SFIO_WC)
		return 1;
	if ((f->flags & SFIO_SYNCED) || f->next > f->data)
	{
		f->bits |= SFIO_MB;
		return -1;
	}
	return 0;
}

int
fwprintf(Sfio_t* f, const wchar_t* fmt, ...)
{
	va_list	args;
	int	v;

	va_start(args, fmt);
	v = vfwprintf(f, fmt, args);
	va_end(args);
	return v;
}

int
fwscanf(Sfio_t* f, const wchar_t* fmt, ...)
{
	va_list		args;
	int		v;

	va_start(args, fmt);

	v = vfwscanf(f, fmt, args);
	va_end(args);
	return v;
}

wint_t
getwc(Sfio_t* f)
{
	return fgetwc(f);
}


wint_t
getwchar(void)
{
	return fgetwc(sfstdin);
}

wint_t
putwc(wchar_t c, Sfio_t* f)
{
	return fputwc(c, f);
}

wint_t
putwchar(wchar_t c)
{
	return fputwc(c, sfstdout);
}

int
swprintf(wchar_t* s, size_t size, const wchar_t* fmt, ...)
{
	va_list	args;
	int	v;

	va_start(args, fmt);
	v = vswprintf(s, size, fmt, args);
	va_end(args);
	return v;
}

int
swscanf(const wchar_t* s, const wchar_t* fmt, ...)
{
	va_list	args;
	int	v;

	va_start(args, fmt);
	v = vswscanf(s, fmt, args);
	va_end(args);
	return v;
}

wint_t
ungetwc(wint_t c, Sfio_t* f)
{
	unsigned char*	s = (unsigned char*)&c;
	unsigned char*	e = s + sizeof(c);

	FWIDE(f, WEOF);
	while (s < e)
		if (sfungetc(f, *s++) == EOF)
			return WEOF;
	return c;
}

int
vfwprintf(Sfio_t* f, const wchar_t* fmt, va_list args)
{
	char*	m;
	char*	x;
	wchar_t*w;
	size_t	n;
	int	v;
	Sfio_t*	t;

	FWIDE(f, -1);
	n = wcstombs(NULL, fmt, 0);
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
				n = mbstowcs(NULL, x, 0);
				if (w = (wchar_t*)sfreserve(f, (ssize_t)(n * sizeof(wchar_t) + 1), 0))
					v = (int)mbstowcs(w, x, n + 1);
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

/*
 * wide exception handler
 * free on close
 */

static int
wideexcept(Sfio_t* f, int op, void* val, Sfdisc_t* dp)
{
	NOT_USED(val);
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
	wchar_t	wuf[2];
	ssize_t	r;

	NOT_USED(f);
	r = sfread(w->f, wuf, sizeof(wuf[0]));
	if (r != sizeof(wuf[0]))
		return -1;
	wuf[1] = 0;
	r = (ssize_t)wcstombs(buf, wuf, size);
	return r;
}

int
vfwscanf(Sfio_t* f, const wchar_t* fmt, va_list args)
{
	size_t	n;
	int	v;
	int	d;
	Sfio_t*	t;
	Wide_t*	w;
	char	buf[1024];

	FWIDE(f, EOF);
	n = wcstombs(NULL, fmt, 0);
	if (w = newof(0, Wide_t, 1, n))
	{
		d = dup(0);
		if (t = sfnew(NULL, buf, sizeof(buf), d, SFIO_READ))
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
		ast_close(d);
	}
	else
		v = -1;
	return v;
}

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
	f.size = (ssize_t)n - 1;
	f.data = f.next = f.endr = (uchar*)s;
	f.endb = f.endw = f.data + f.size;

	/*
	 * call and adjust
	 */

	v = vfwprintf(&f, fmt, args);
	*f.next = 0;
	_Sfi = (ssize_t)(f.next - f.data);
	return v;
}

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
	f.size = (ssize_t)(wcslen(s) * sizeof(wchar_t));
	f.data = f.next = f.endw = (uchar*)s;
	f.endb = f.endr = f.data + f.size;

	/*
	 * sfio does the rest
	 */

	return vfwscanf(&f, fmt, args);
}

int
vwprintf(const wchar_t* fmt, va_list args)
{
	return vfwprintf(sfstdout, fmt, args);
}

int
vwscanf(const wchar_t* fmt, va_list args)
{
	return vfwscanf(sfstdin, fmt, args);
}

int
wprintf(const wchar_t* fmt, ...)
{
	va_list	args;
	int	v;

	va_start(args, fmt);
	v = vfwprintf(sfstdout, fmt, args);
	va_end(args);
	return v;
}

int
wscanf(const wchar_t* fmt, ...)
{
	va_list	args;
	int	v;

	va_start(args, fmt);
	v = vfwscanf(sfstdin, fmt, args);
	va_end(args);
	return v;
}

#else
NoN(stdio_mb)
#endif /* !_has_multibyte */

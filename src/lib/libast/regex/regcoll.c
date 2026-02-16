/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
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
 * regex collation symbol support
 */

#include "reglib.h"

/*
 * return the collating symbol delimited by [c c], where c is either '=' or '.'
 * s points to the first char after the initial [
 * if e!=0 it is set to point to the next char in s on return
 *
 * the collating symbol is converted to multibyte in <buf,size>
 * the return value is:
 *	-1	syntax error / invalid collating element
 *	>=0	size with 0-terminated mb character (*wc != 0)
 *		or collating element (*wc == 0) in buf
 */

int
regcollate(const char* s, char** e, char* buf, size_t size, wchar_t* wc)
{
	int			c;
	char*			b;
	char*			x;
	const char*		t;
	int			r;
	int			term;
	wchar_t			w;

	if (size < 2 || (term = *s) != '.' && term != '=' || !*++s || *s == term && *(s + 1) == ']')
		goto nope;
	t = s;
	w = mbchar(s);
	if ((r = (s - t)) > 1)
	{
		if (*s++ != term || *s++ != ']')
			goto oops;
		goto done;
	}
	if (*s == term && *(s + 1) == ']')
	{
		s += 2;
		goto done;
	}
	b = buf;
	x = buf + size - 2;
	s = t;
	for (;;)
	{
		if (!(c = *s++))
			goto oops;
		if (c == term)
		{
			if (!(c = *s++))
				goto oops;
			if (c != term)
			{
				if (c != ']')
					goto oops;
				break;
			}
		}
		if (b < x)
			*b++ = c;
	}
	r = s - t - 2;
	w = 0;
	if (b >= x)
		goto done;
	*b = 0;
	if(!ast.locale.transform)
		goto nope;
	{
		char	tmp[256];
		int	i;
		for (i = 0; i < r && i < sizeof(tmp) - 1; i++)
			tmp[i] = '0';
		tmp[i] = 0;
		if (ast.locale.transform(NULL, buf, 0) >= ast.locale.transform(NULL, tmp, 0))
			goto nope;
	}
	t = (const char*)buf;
 done:
	if (r <= size && (char*)t != buf)
	{
		memcpy(buf, t, r);
		if (r < size)
			buf[r] = 0;
	}
	if (wc)
		*wc = w;
	if (e)
		*e = (char*)s;
	return r;
 oops:
 	s--;
 nope:
	if (e)
		*e = (char*)s;
	return -1;
}

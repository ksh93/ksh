/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
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
#include	"sfhdr.h"

/*	Print data with a given format
**
**	Written by Kiem-Phong Vo.
*/

int sfprintf(Sfio_t* f, const char* form, ...)
{
	va_list	args;
	int	rv;
	va_start(args,form);
	rv = sfvprintf(f,form,args);
	va_end(args);
	return rv;
}

ssize_t sfvsprintf(char* s, size_t n, const char* form, va_list args)
{
	Sfio_t		*f;
	ssize_t		rv;
	Sfnotify_f	notify = _Sfnotify;

	/* make a temp stream */
	_Sfnotify = 0;
	f = sfnew(NULL,NULL,(size_t)SFIO_UNBOUND, -1,SFIO_WRITE|SFIO_STRING);
	_Sfnotify = notify;
	if(!f)
		return -1;

	if((rv = sfvprintf(f,form,args)) < 0 )
		return -1;
	if(s && n > 0)
	{	if((rv+1) >= n)
			n--;
		else
			n = rv;
		memcpy(s, f->data, n);
		s[n] = 0;
	}

	sfclose(f);

	_Sfi = rv;

	return rv;
}

ssize_t sfsprintf(char* s, size_t n, const char* form, ...)
{
	va_list	args;
	ssize_t	rv;
	va_start(args,form);
	rv = sfvsprintf(s,n,form,args);
	va_end(args);
	return rv;
}

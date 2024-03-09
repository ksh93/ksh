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

/*	Construct a string with the given format and data.
**	These functions allocate space as necessary to store the string.
**	This avoids overflow problems typical with sprintf() in stdio.
**
**	Written by Kiem-Phong Vo.
*/

char* sfvprints(const char* form, va_list args)
{
	int		rv;
	Sfnotify_f	notify = _Sfnotify;
	static Sfio_t*	f;

	if(!f) /* make a string stream to write into */
	{	_Sfnotify = 0;
		f = sfnew(NULL,NULL,(size_t)SFIO_UNBOUND, -1,SFIO_WRITE|SFIO_STRING);
		_Sfnotify = notify;
		if(!f)
			return NULL;
	}

	sfseek(f,0,SEEK_SET);
	rv = sfvprintf(f,form,args);

	if(rv < 0 || sfputc(f,'\0') < 0)
		return NULL;

	_Sfi = (f->next - f->data) - 1;
	return (char*)f->data;
}

char* sfprints(const char* form, ...)
{
	char*	s;
	va_list	args;
	va_start(args,form);
	s = sfvprints(form, args);
	va_end(args);
	return s;
}

ssize_t sfvaprints(char** sp, const char* form, va_list args)
{
	char	*s;
	ssize_t	n;

	if(!sp || !(s = sfvprints(form,args)) )
		return -1;
	else
	{	if(!(*sp = (char*)malloc(n = strlen(s)+1)) )
			return -1;
		memcpy(*sp, s, n);
		return n-1;
	}
}

ssize_t sfaprints(char** sp, const char* form, ...)
{
	ssize_t	n;
	va_list	args;
	va_start(args,form);
	n = sfvaprints(sp, form, args);
	va_end(args);
	return n;
}

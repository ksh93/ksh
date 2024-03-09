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

int
vsnprintf(char* s, int n, const char* form, va_list args)
{
	Sfio_t*	f;
	ssize_t	rv;

	/* make a temp stream */
	if(!(f = sfnew(NULL,NULL,(size_t)SFIO_UNBOUND,
                        -1,SFIO_WRITE|SFIO_STRING)) )
		return -1;

	if((rv = sfvprintf(f,form,args)) >= 0 )
	{	if(s && n > 0)
		{	if((rv+1) >= n)
				n--;
			else
				n = rv;
			memcpy(s, f->data, n);
			s[n] = 0;
		}
		_Sfi = rv;
	}

	sfclose(f);

	return rv;
}

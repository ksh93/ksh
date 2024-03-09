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
#include	"sfhdr.h"

/*	Write out a long value in a portable format
**
**	Written by Kiem-Phong Vo.
*/

int _sfputl(Sfio_t*	f,	/* write a portable long to this stream */
	    Sflong_t	v)	/* the value to be written */
{
#define N_ARRAY		(2*sizeof(Sflong_t))
	uchar	*s, *ps;
	ssize_t	n, p;
	uchar		c[N_ARRAY];

	if(!f || (f->mode != SFIO_WRITE && _sfmode(f,SFIO_WRITE,0) < 0))
		return -1;
	SFLOCK(f,0);

	s = ps = &(c[N_ARRAY-1]);
	if(v < 0)
	{	/* add 1 to avoid 2-complement problems with -SFIO_MAXINT */
		v = -(v+1);
		*s = (uchar)(SFSVALUE(v) | SFIO_SIGN);
	}
	else	*s = (uchar)(SFSVALUE(v));
	v = (Sfulong_t)v >> SFIO_SBITS;

	while(v > 0)
	{	*--s = (uchar)(SFUVALUE(v) | SFIO_MORE);
		v = (Sfulong_t)v >> SFIO_UBITS;
	}
	n = (ps-s)+1;

	if(n > 8 || SFWPEEK(f,ps,p) < n)
		n = SFWRITE(f,s,n); /* write the hard way */
	else
	{	switch(n)
		{
		case 8 : *ps++ = *s++;
			 /* FALLTHROUGH */
		case 7 : *ps++ = *s++;
			 /* FALLTHROUGH */
		case 6 : *ps++ = *s++;
			 /* FALLTHROUGH */
		case 5 : *ps++ = *s++;
			 /* FALLTHROUGH */
		case 4 : *ps++ = *s++;
			 /* FALLTHROUGH */
		case 3 : *ps++ = *s++;
			 /* FALLTHROUGH */
		case 2 : *ps++ = *s++;
			 /* FALLTHROUGH */
		case 1 : *ps++ = *s++;
		}
		f->next = ps;
	}

	SFOPEN(f,0);
	return n;
}

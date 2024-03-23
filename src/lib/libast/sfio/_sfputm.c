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

/*	Write out an unsigned long value in a portable format.
**
**	Written by Kiem-Phong Vo.
*/

int _sfputm(Sfio_t*	f,	/* write a portable ulong to this stream */
	    Sfulong_t	v,	/* the unsigned value to be written */
	    Sfulong_t	m)	/* the max value of the range */
{
#define N_ARRAY		(2*sizeof(Sfulong_t))
	uchar	*s, *ps;
	ssize_t	n, p;
	uchar		c[N_ARRAY];

	if(!f || v > m || (f->mode != SFIO_WRITE && _sfmode(f,SFIO_WRITE,0) < 0))
		return -1;
	SFLOCK(f,0);

	/* code v as integers in base SFIO_UBASE */
	s = ps = &(c[N_ARRAY-1]);
	*s = (uchar)SFBVALUE(v);
	while((m >>= SFIO_BBITS) > 0 )
	{	v >>= SFIO_BBITS;
		*--s = (uchar)SFBVALUE(v);
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
	return (int)n;
}

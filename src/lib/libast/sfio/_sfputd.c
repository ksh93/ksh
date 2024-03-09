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
#include	"sfhdr.h"

/*	Write out a floating point value in a portable format
**
**	Written by Kiem-Phong Vo.
*/

int _sfputd(Sfio_t* f, Sfdouble_t v)
{
#define N_ARRAY		(16*sizeof(Sfdouble_t))
	ssize_t		n, w;
	uchar		*s, *ends;
	int		exp;
	uchar		c[N_ARRAY];
	Sfdouble_t	x;

	if(!f || (f->mode != SFIO_WRITE && _sfmode(f,SFIO_WRITE,0) < 0))
		return -1;
	SFLOCK(f,0);

	/* get the sign of v */
	if(v < 0.)
	{	v = -v;
		n = 1;
	}
	else	n = 0;

	/* make the magnitude of v < 1 */
	if(v != 0.)
		v = frexpl(v,&exp);
	else	exp = 0;

	/* code the sign of v and exp */
	if((w = exp) < 0)
	{	n |= 02;
		w = -w;
	}

	/* write out the signs and the exp */
	SFOPEN(f,0);
	if(sfputc(f,n) < 0 || (w = sfputu(f,w)) < 0)
		return -1;
	SFLOCK(f,0);
	w += 1;

	s = (ends = &c[0])+sizeof(c);
	while(s > ends)
	{	/* get 2^SFIO_PRECIS precision at a time */
		n = (int)(x = ldexpl(v,SFIO_PRECIS));
		*--s = n|SFIO_MORE;
		v = x-n;
		if(v <= 0.)
			break;
	}

	/* last byte is not SFIO_MORE */
	ends = &c[0] + sizeof(c) -1;
	*ends &= ~SFIO_MORE;

	/* write out coded bytes */
	n = ends - s + 1;
	w = SFWRITE(f,s,n) == n ? w+n : -1;

	SFOPEN(f,0);
	return w;
}

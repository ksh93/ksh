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

/*	Read a portably coded double value
**
**	Written by Kiem-Phong Vo
*/

Sfdouble_t sfgetd(Sfio_t* f)
{
	uchar		*s, *ends, c;
	int		p, sign, exp;
	Sfdouble_t	v;

	if(!f || (sign = sfgetc(f)) < 0 || (exp = (int)sfgetu(f)) < 0)
		return -1.;

	if(f->mode != SFIO_READ && _sfmode(f,SFIO_READ,0) < 0)
		return -1.;

	SFLOCK(f,0);

	v = 0.;
	for(;;)
	{	/* fast read for data */
		if(SFRPEEK(f,s,p) <= 0)
		{	f->flags |= SFIO_ERROR;
			v = -1.;
			goto done;
		}

		for(ends = s+p; s < ends; )
		{	c = *s++;
			v += SFUVALUE(c);
			v = ldexpl(v,-SFIO_PRECIS);
			if(!(c&SFIO_MORE))
			{	f->next = s;
				goto done;
			}
		}
		f->next = s;
	}

done:
	v = ldexpl(v,(sign&02) ? -exp : exp);
	if(sign&01)
		v = -v;

	SFOPEN(f,0);
	return v;
}

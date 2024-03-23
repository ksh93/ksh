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

/*	Invoke event handlers for a stream
**
**	Written by Kiem-Phong Vo.
*/

static int _sfraiseall(int	type,	/* type of event	*/
		       void*	data)	/* associated data	*/
{
	Sfio_t		*f;
	Sfpool_t	*p, *next;
	int		n, rv;

	rv = 0;
	for(p = &_Sfpool; p; p = next)
	{
		for(next = p->next; next; next = next->next)
			if(next->n_sf > 0)
				break;
		for(n = 0; n < p->n_sf; ++n)
		{	f = p->sf[n];
			if(sfraise(f, type, data) < 0)
				rv -= 1;
		}
	}
	return rv;
}

int sfraise(Sfio_t*	f,	/* stream		*/
	    int		type,	/* type of event	*/
	    void*	data)	/* associated data	*/
{
	Sfdisc_t	*disc, *next, *d;
	int		local, rv;

	if(!f)
		return _sfraiseall(type,data);

	GETLOCAL(f,local);
	if(!SFKILLED(f) &&
	   !(local &&
	     (type == SFIO_NEW || type == SFIO_CLOSING ||
	      type == SFIO_FINAL || type == SFIO_ATEXIT)) &&
	   SFMODE(f,local) != (f->mode&SFIO_RDWR) && _sfmode(f,0,local) < 0)
		return -1;
	SFLOCK(f,local);

	for(disc = f->disc; disc; )
	{	next = disc->disc;
		if(type == SFIO_FINAL)
			f->disc = next;

		if(disc->exceptf)
		{	SFOPEN(f,0);
			if((rv = (*disc->exceptf)(f,type,data,disc)) != 0 )
				return rv;
			SFLOCK(f,0);
		}

		if((disc = next) )
		{	/* make sure that "next" hasn't been popped */
			for(d = f->disc; d; d = d->disc)
				if(d == disc)
					break;
			if(!d)
				disc = f->disc;
		}
	}

	SFOPEN(f,local);
	return 0;
}

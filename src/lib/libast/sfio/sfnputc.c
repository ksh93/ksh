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

/*	Write out a character n times
**
**	Written by Kiem-Phong Vo.
*/

ssize_t sfnputc(Sfio_t*		f,	/* file to write */
		int		c,	/* char to be written */
		size_t		n)	/* number of time to repeat */
{
	uchar*	ps;
	ssize_t	p, w;
	uchar	buf[128];
	int	local;

	if(!f)
		return -1;

	GETLOCAL(f,local);
	if(SFMODE(f,local) != SFIO_WRITE && _sfmode(f,SFIO_WRITE,local) < 0)
		return -1;

	SFLOCK(f,local);

	/* write into a suitable buffer */
	if((size_t)(p = (f->endb-(ps = f->next))) < n)
		{ ps = buf; p = sizeof(buf); }
	if((size_t)p > n)
		p = n;
	MEMSET(ps,c,p);
	ps -= p;

	w = n;
	if(ps == f->next)
	{	/* simple sfwrite */
		f->next += p;
		if(c == '\n')
			(void)SFFLSBUF(f,-1);
		goto done;
	}

	for(;;)
	{	/* hard write of data */
		if((p = SFWRITE(f,ps,p)) <= 0 || (n -= p) <= 0)
		{	w -= n;
			goto done;
		}
		if((size_t)p > n)
			p = n;
	}
done :
	SFOPEN(f,local);
	return w;
}

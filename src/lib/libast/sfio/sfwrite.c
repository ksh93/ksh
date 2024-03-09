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

/*	Write data out to the file system
**
**	Written by Kiem-Phong Vo.
*/

ssize_t sfwrite(Sfio_t*		f,	/* write to this stream. 	*/
		const void*	buf,	/* buffer to be written.	*/
		size_t		n)	/* number of bytes. 		*/
{
	uchar		*s, *begs, *next;
	ssize_t		w;
	int		local;

	if(!f)
		return (ssize_t)(-1);

	GETLOCAL(f,local);

	if(!buf)
		return (ssize_t)(n == 0 ? 0 : -1) ;

	/* release peek lock */
	if(f->mode&SFIO_PEEK)
	{	if(!(f->mode&SFIO_WRITE) && (f->flags&SFIO_RDWR) != SFIO_RDWR)
			return (ssize_t)(-1);

		if((uchar*)buf != f->next &&
		   (!f->rsrv || f->rsrv->data != (uchar*)buf) )
			return (ssize_t)(-1);

		f->mode &= ~SFIO_PEEK;

		if(f->mode&SFIO_PKRD)
		{	/* read past peeked data */
			char		buf[16];
			ssize_t	r;

			for(w = n; w > 0; )
			{	if((r = w) > sizeof(buf))
					r = sizeof(buf);
				if((r = read(f->file,buf,r)) <= 0)
				{	n -= w;
					break;
				}
				else	w -= r;
			}

			f->mode &= ~SFIO_PKRD;
			f->endb = f->data + n;
			f->here += n;
		}

		if((f->mode&SFIO_READ) && f->proc)
			f->next += n;
	}

	s = begs = (uchar*)buf;
	for(;; f->mode &= ~SFIO_LOCK)
	{	/* check stream mode */
		if(SFMODE(f,local) != SFIO_WRITE && _sfmode(f,SFIO_WRITE,local) < 0 )
		{	w = s > begs ? s-begs : -1;
			return w;
		}

		SFLOCK(f,local);

		w = f->endb - f->next;

		if(s == f->next && s < f->endb) /* after sfreserve */
		{	if(w > (ssize_t)n)
				w = (ssize_t)n;
			f->next = (s += w);
			n -= w;
			break;
		}

		/* attempt to create space in buffer */
		if(w == 0 || ((f->flags&SFIO_WHOLE) && w < (ssize_t)n) )
		{	if(f->flags&SFIO_STRING) /* extend buffer */
			{	(void)SFWR(f, s, n-w, f->disc);
				if((w = f->endb - f->next) < (ssize_t)n)
				{	if(!(f->flags&SFIO_STRING)) /* maybe sftmp */
					{	if(f->next > f->data)
							goto fls_buf;
					}
					else if(w == 0)
						break;
				}
			}
			else if(f->next > f->data)
			{ fls_buf:
				(void)SFFLSBUF(f, -1);
				if((w = f->endb - f->next) < (ssize_t)n &&
				   (f->flags&SFIO_WHOLE) && f->next > f->data )
						break;
			}
		}

		if(!(f->flags&SFIO_STRING) && f->next == f->data &&
		   (((f->flags&SFIO_WHOLE) && w <= n) || SFDIRECT(f,n)) )
		{	/* bypass buffering */
			if((w = SFWR(f,s,n,f->disc)) <= 0 )
				break;
		}
		else
		{	if(w > (ssize_t)n)
				w = (ssize_t)n;
			if(w <= 0) /* no forward progress possible */
				break;
			memmove(f->next, s, w);
			f->next += w;
		}

		s += w;
		if((n -= w) <= 0)
			break;
	}

	/* always flush buffer for share streams */
	if(f->extent < 0 && (f->flags&SFIO_SHARE) && !(f->flags&SFIO_PUBLIC) )
		(void)SFFLSBUF(f,-1);

	/* check to see if buffer should be flushed */
	else if(n == 0 && (f->flags&SFIO_LINE) && !(f->flags&SFIO_STRING))
	{	if((ssize_t)(n = f->next-f->data) > (w = s-begs))
			n = w;
		if(n > 0 && n < HIFORLINE)
		{	for(next = f->next-1; n > 0; --n, --next)
			{	if(*next == '\n')
				{	n = HIFORLINE;
					break;
				}
			}
		}
		if(n >= HIFORLINE)
			(void)SFFLSBUF(f,-1);
	}

	SFOPEN(f,local);

	w = s-begs;
	return w;
}

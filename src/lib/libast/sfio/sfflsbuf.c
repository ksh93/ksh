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

/*	Write a buffer out to a file descriptor or
**	extending a buffer for a SFIO_STRING stream.
**
**	Written by Kiem-Phong Vo
*/

int _sfflsbuf(Sfio_t*	f,	/* write out the buffered content of this stream */
	      int	c)	/* if c>=0, c is also written out */
{
	ssize_t		n, w, written;
	uchar*		data;
	uchar		outc;
	int		local, isall;
	int		inpc = c;

	if(!f)
		return -1;

	GETLOCAL(f,local);

	for(written = 0;; f->mode &= ~SFIO_LOCK)
	{	/* check stream mode */
		if(SFMODE(f,local) != SFIO_WRITE && _sfmode(f,SFIO_WRITE,local) < 0)
			return -1;
		SFLOCK(f,local);

		/* current data extent */
		n = f->next - (data = f->data);

		if(n == (f->endb-data) && (f->flags&SFIO_STRING))
		{	/* call sfwr() to extend string buffer and process events */
			w = ((f->bits&SFIO_PUTR) && f->val > 0) ? f->val : 1;
			(void)SFWR(f, data, w, f->disc);

			/* !(f->flags&SFIO_STRING) is required because exception
			   handlers may turn a string stream to a file stream */
			if(f->next < f->endb || !(f->flags&SFIO_STRING) )
				n = f->next - (data = f->data);
			else
			{	SFOPEN(f,local);
				return -1;
			}
		}

		if(c >= 0)
		{	/* write into buffer */
			if(n < (f->endb - (data = f->data)))
			{	*f->next++ = c;
				if(c == '\n' &&
				   (f->flags&SFIO_LINE) && !(f->flags&SFIO_STRING))
				{	c = -1;
					n += 1;
				}
				else	break;
			}
			else if(n == 0)
			{	/* unbuffered io */
				outc = (uchar)c;
				data = &outc;
				c = -1;
				n = 1;
			}
		}

		if(n == 0 || (f->flags&SFIO_STRING))
			break;

		isall = SFISALL(f,isall);
		if((w = SFWR(f,data,n,f->disc)) > 0)
		{	if((n -= w) > 0) /* save unwritten data, then resume */
				memmove((char*)f->data,(char*)data+w,n);
			written += w;
			f->next = f->data+n;
			if(c < 0 && (!isall || n == 0))
				break;
		}
		else if(w == 0)
		{	if(written > 0) /* some buffer was cleared */
				break; /* do normal exit below */
			else /* nothing was done, returning failure */
			{	SFOPEN(f,local);
				return -1;
			}
		}
		else /* w < 0 means SFIO_EDISC or SFIO_ESTACK in sfwr() */
		{	if(c < 0) /* back to the calling write operation */
				break;
			else	continue; /* try again to write out c */
		}
	}

	SFOPEN(f,local);

	if(inpc < 0)
		inpc = f->endb-f->next;

	return inpc;
}

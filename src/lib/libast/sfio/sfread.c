/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2026 Contributors to ksh 93u+m           *
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

/*	Read n bytes from a stream into a buffer
**
**	Written by Kiem-Phong Vo.
*/

ssize_t sfread(Sfio_t*	f,	/* read from this stream. 	*/
	       void*	buf,	/* buffer to read into		*/
	       size_t	n)	/* number of bytes to be read. 	*/
{
	uchar		*s, *begs;
	ssize_t		r;
	int		local, justseek;

	if(!f)
		return -1;

	GETLOCAL(f,local);
	justseek = f->bits&SFIO_JUSTSEEK; f->bits &= ~SFIO_JUSTSEEK;

	if(!buf)
		return n == 0 ? 0 : -1;

	/* release peek lock */
	if(f->mode&SFIO_PEEK)
	{	if(!(f->mode&SFIO_READ) )
			return -1;

		if(f->mode&SFIO_GETR)
		{	if(((uchar*)buf + f->val) != f->next &&
			   (!f->rsrv || f->rsrv->data != (uchar*)buf) )
				return -1;
			f->mode &= (uint32_t)~SFIO_PEEK;
			return 0;
		}
		else
		{	if((uchar*)buf != f->next)
				return -1;
			f->mode &= (uint32_t)~SFIO_PEEK;
			if(f->mode&SFIO_PKRD)
			{	/* actually read the data now */
				f->mode &= (uint32_t)~SFIO_PKRD;
				if(n > 0)
					n = (r = read(f->file,f->data,n)) < 0 ? 0 : (size_t)r;
				f->endb = f->data+n;
				f->here += n;
			}
			f->next += n;
			f->endr = f->endb;
			return (ssize_t)n;
		}
	}

	s = begs = (uchar*)buf;
	for(;; f->mode &= (uint32_t)~SFIO_LOCK)
	{	/* check stream mode */
		if(SFMODE(f,local) != SFIO_READ && _sfmode(f,SFIO_READ,local) < 0)
		{	n = s > begs ? (size_t)(s-begs) : (size_t)(-1);
			return (ssize_t)n;
		}

		SFLOCK(f,local);

		if((r = f->endb - f->next) > 0) /* has buffered data */
		{	if(r > (ssize_t)n)
				r = (ssize_t)n;
			if(s != f->next)
				memmove(s, f->next, (size_t)r);
			f->next += r;
			s += r;
			n -= (size_t)r;
		}

		if((ssize_t)n <= 0)	/* all done */
			break;

		if(!(f->flags&SFIO_STRING) && !(f->bits&SFIO_MMAP) )
		{	f->next = f->endb = f->data;

			/* exact IO is desirable for these cases */
			if(SFDIRECT(f,n) ||
			   ((f->flags&SFIO_SHARE) && f->extent < 0) )
				r = (ssize_t)n;
			else if(justseek && n <= f->iosz && f->iosz <= (size_t)f->size)
				r = (ssize_t)f->iosz;	/* limit buffering */
			else	r = f->size;	/* full buffering */

			/* if read almost full size, then just do it direct */
			if(r > (ssize_t)n && (r - r/8) <= (ssize_t)n)
				r = (ssize_t)n;

			/* read directly to user's buffer */
			if(r == (ssize_t)n && (r = SFRD(f,s,(size_t)r,f->disc)) >= 0)
			{	s += r;
				n -= (size_t)r;
				if(r == 0 || n == 0) /* eof or eob */
					break;
			}
			else	goto do_filbuf;
		}
		else
		{ do_filbuf:
			if(justseek)
				f->bits |= SFIO_JUSTSEEK;
			if(SFFILBUF(f,-1) <= 0)
				break;
		}
	}

	SFOPEN(f,local);
	r = s-begs;
	return r;
}

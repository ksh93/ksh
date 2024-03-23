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

/*	Safe access to the internal stream buffer.
**	This function is obsolete. sfreserve() should be used.
**
**	Written by Kiem-Phong Vo (06/27/90).
*/

extern ssize_t sfpeek(Sfio_t*	f,	/* file to peek */
		      void**		bp,	/* start of data area */
		      size_t	size)	/* size of peek */
{	ssize_t	n, sz;
	int	mode;

	/* query for the extent of the remainder of the buffer */
	if((sz = size) == 0 || !bp)
	{	if(f->mode&SFIO_INIT)
			(void)_sfmode(f,0,0);

		if((f->flags&SFIO_RDWRSTR) == SFIO_RDWRSTR)
		{	SFSTRSIZE(f);
			n = (f->data+f->here) - f->next;
		}
		else	n = f->endb - f->next;

		if(!bp)
			return n;
		else if(n > 0)	/* size == 0 */
		{	*bp = f->next;
			return 0;
		}
		/* else fall down and fill buffer */
	}

	if(!(mode = f->flags&SFIO_READ) )
		mode = SFIO_WRITE;
	if((int)f->mode != mode && _sfmode(f,mode,0) < 0)
		return -1;

	*bp = sfreserve(f, sz <= 0 ? 0 : sz > f->size ? f->size : sz, 0);

	if(*bp && sz >= 0)
		return sz;

	if((n = sfvalue(f)) > 0)
	{	*bp = f->next;
		if(sz < 0)
		{	f->mode |= SFIO_PEEK;
			f->endr = f->endw = f->data;
		}
		else
		{	if(sz > n)
				sz = n;
			f->next += sz;
		}
	}

	return (sz >= 0 && n >= sz) ? sz : n;
}

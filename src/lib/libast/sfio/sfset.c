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

/*	Set some control flags or file descriptor for the stream
**
**	Written by Kiem-Phong Vo.
*/

int sfset(Sfio_t* f, int flags, int set)
{
	int	oflags, tflags, rv;

	if(!f)
		return 0;

	if(flags == 0 && set == 0)
		return f->flags&SFIO_FLAGS;

	if((oflags = (f->mode&SFIO_RDWR)) != (int)f->mode)
	{	/* avoid sfsetbuf() isatty() call if user sets (SFIO_LINE|SFIO_WCWIDTH) */
		if(set && (flags & (SFIO_LINE|SFIO_WCWIDTH)) && !(f->flags & (SFIO_LINE|SFIO_WCWIDTH)))
		{	tflags = (SFIO_LINE|SFIO_WCWIDTH);
			f->flags |= tflags;
		}
		else	tflags = 0;
		rv = _sfmode(f,oflags,0);
		if(tflags)
			f->flags &= ~tflags;
		if(rv < 0)
			return 0;
	}
	if(flags == 0)
		return f->flags&SFIO_FLAGS;

	SFLOCK(f,0);

	/* preserve at least one rd/wr flag */
	oflags = f->flags;
	if(!(f->bits&SFIO_BOTH) || (flags&SFIO_RDWR) == SFIO_RDWR )
		flags &= ~SFIO_RDWR;

	/* set the flag */
	if(set)
		f->flags |=  (flags&SFIO_SETS);
	else	f->flags &= ~(flags&SFIO_SETS);

	/* must have at least one of read/write */
	if(!(f->flags&SFIO_RDWR))
		f->flags |= (oflags&SFIO_RDWR);

	if(f->extent < 0)
		f->flags &= ~SFIO_APPENDWR;

	/* turn to appropriate mode as necessary */
	if((flags &= SFIO_RDWR) )
	{	if(!set)
		{	if(flags == SFIO_READ)
				flags = SFIO_WRITE;
			else	flags = SFIO_READ;
		}
		if((flags == SFIO_WRITE && !(f->mode&SFIO_WRITE)) ||
		   (flags == SFIO_READ && !(f->mode&(SFIO_READ|SFIO_SYNCED))) )
			(void)_sfmode(f,flags,1);
	}

	/* if not shared or unseekable, public means nothing */
	if(!(f->flags&SFIO_SHARE) || f->extent < 0)
		f->flags &= ~SFIO_PUBLIC;

	SFOPEN(f,0);
	return oflags&SFIO_FLAGS;
}

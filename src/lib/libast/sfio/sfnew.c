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

/*	Fundamental function to create a new stream.
**	The argument flags defines the type of stream and the scheme
**	of buffering.
**
**	Written by Kiem-Phong Vo.
*/

Sfio_t* sfnew(Sfio_t*	oldf,	/* old stream to be reused */
	      void*	buf,	/* a buffer to read/write, if NULL, will be allocated */
	      size_t	size,	/* buffer size if buf is given or desired buffer size */
	      int	file,	/* file descriptor to read/write from */
	      int	flags)	/* type of file stream */
{
	Sfio_t*		f;
	int		sflags;


	if(!(flags&SFIO_RDWR))
		return NULL;

	sflags = 0;
	if((f = oldf) )
	{	if(flags&SFIO_EOF)
		{	SFCLEAR(f);
			oldf = NULL;
		}
		else if(f->mode&SFIO_AVAIL)
		{	/* only allow SFIO_STATIC to be already closed */
			if(!(f->flags&SFIO_STATIC) )
				return NULL;
			sflags = f->flags;
			oldf = NULL;
		}
		else
		{	/* reopening an open stream, close it first */
			sflags = f->flags;

			if(((f->mode&SFIO_RDWR) != f->mode && _sfmode(f,0,0) < 0) ||
			   SFCLOSE(f) < 0 )
				return NULL;

			if(f->data && ((flags&SFIO_STRING) || size != (size_t)SFIO_UNBOUND) )
			{	if(sflags&SFIO_MALLOC)
					free(f->data);
				f->data = NULL;
			}
			if(!f->data)
				sflags &= ~SFIO_MALLOC;
		}
	}

	if(!f)
	{	/* reuse a standard stream structure if possible */
		if(!(flags&SFIO_STRING) && file >= 0 && file <= 2)
		{	f = file == 0 ? sfstdin : file == 1 ? sfstdout : sfstderr;
			if(f)
			{	if(f->mode&SFIO_AVAIL)
				{	sflags = f->flags;
					SFCLEAR(f);
				}
				else	f = NULL;
			}
		}

		if(!f)
		{	if(!(f = (Sfio_t*)malloc(sizeof(Sfio_t))) )
				return NULL;
			SFCLEAR(f);
		}
	}

	/* stream type */
	f->mode = (flags&SFIO_READ) ? SFIO_READ : SFIO_WRITE;
	f->flags = (flags&SFIO_FLAGS) | (sflags&(SFIO_MALLOC|SFIO_STATIC));
	f->bits = (flags&SFIO_RDWR) == SFIO_RDWR ? SFIO_BOTH : 0;
	f->file = file;
	f->here = f->extent = 0;
	f->getr = f->tiny[0] = 0;

	f->mode |= SFIO_INIT;
	if(size != (size_t)SFIO_UNBOUND)
	{	f->size = size;
		f->data = size <= 0 ? NULL : (uchar*)buf;
	}
	f->endb = f->endr = f->endw = f->next = f->data;

	if(_Sfnotify)
		(*_Sfnotify)(f, SFIO_NEW, (void*)((long)f->file));

	if(f->flags&SFIO_STRING)
		(void)_sfmode(f,f->mode&SFIO_RDWR,0);

	return f;
}

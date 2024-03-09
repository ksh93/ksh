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
#define getpagesize	______getpagesize

#include	"sfhdr.h"

#undef	getpagesize

#if _lib_getpagesize
extern int	getpagesize(void);
#endif

/*	Set a (new) buffer for a stream.
**	If size < 0, it is assigned a suitable value depending on the
**	kind of stream. The actual buffer size allocated is dependent
**	on how much memory is available.
**
**	Written by Kiem-Phong Vo.
*/

#if !_sys_stat
struct stat
{	int	st_mode;
	int	st_size;
};
#undef fstat
#define fstat(fd,st)	(-1)
#endif /*_sys_stat*/

#if !defined(SFSETLINEMODE)
#define SFSETLINEMODE		1
#endif

#if SFSETLINEMODE

static int sfsetlinemode(void)
{	char*			astsfio;
	char*			endw;

	static int		modes = -1;
	static const char	sf_line[] = "SFIO_LINE";
	static const char	sf_maxr[] = "SFIO_MAXR=";
	static const char	sf_wcwidth[] = "SFIO_WCWIDTH";

#define ISSEPAR(c)	((c) == ',' || (c) == ' ' || (c) == '\t')
	if (modes < 0)
	{	modes = 0;
		if(astsfio = getenv("SFIO_OPTIONS"))
		{	for(; *astsfio != 0; astsfio = endw)
			{	while(ISSEPAR(*astsfio) )
					++astsfio;
				for(endw = astsfio; *endw && !ISSEPAR(*endw); ++endw)
					;
				if((endw-astsfio) > (sizeof(sf_line)-1) &&
				   strncmp(astsfio,sf_line,sizeof(sf_line)-1) == 0)
					modes |= SFIO_LINE;
				else if((endw-astsfio) > (sizeof(sf_maxr)-1) &&
				   strncmp(astsfio,sf_maxr,sizeof(sf_maxr)-1) == 0)
					_Sfmaxr = (ssize_t)strtonll(astsfio+sizeof(sf_maxr)-1,NULL,NULL,0);
				else if((endw-astsfio) > (sizeof(sf_wcwidth)-1) &&
				   strncmp(astsfio,sf_wcwidth,sizeof(sf_wcwidth)-1) == 0)
					modes |= SFIO_WCWIDTH;
			}
		}
	}
	return modes;
}

#endif

void* sfsetbuf(Sfio_t*	f,	/* stream to be buffered */
	       void*	buf,	/* new buffer */
	       size_t	size)	/* buffer size, -1 for default size */
{
	int		sf_malloc, oflags, init, local;
	ssize_t		bufsize, blksz;
	Sfdisc_t*	disc;
	struct stat	st;
	uchar*		obuf = NULL;
	ssize_t		osize = 0;
#ifdef MAP_TYPE
	int		okmmap;
#endif

	if(!f)
		return NULL;

	GETLOCAL(f,local);

	if(size == 0 && buf)
	{	/* special case to get buffer info */
		_Sfi = f->val = (f->bits&SFIO_MMAP) ? (f->endb-f->data) : f->size;
		return f->data;
	}

	/* cleanup actions already done, don't allow write buffering any more */
	if(_Sfexiting && !(f->flags&SFIO_STRING) && (f->mode&SFIO_WRITE))
	{	buf = NULL;
		size = 0;
	}

	if((init = f->mode&SFIO_INIT) )
	{	if(!f->pool && _sfsetpool(f) < 0)
			return NULL;
	}
	else if((f->mode&SFIO_RDWR) != SFMODE(f,local) && _sfmode(f,0,local) < 0)
		return NULL;

	if(init)
		f->mode = (f->mode&SFIO_RDWR)|SFIO_LOCK;
	else
	{	int	rv;

		/* make sure there is no hidden read data */
		if(f->proc && (f->flags&SFIO_READ) && (f->mode&SFIO_WRITE) &&
		   _sfmode(f,SFIO_READ,local) < 0)
			return NULL;

		/* synchronize first */
		SFLOCK(f,local); rv = SFSYNC(f); SFOPEN(f,local);
		if(rv < 0)
			return NULL;

		/* turn off the SFIO_SYNCED bit because buffer is changing */
		f->mode &= ~SFIO_SYNCED;
	}

	SFLOCK(f,local);

	if((Sfio_t*)buf != f)
		blksz = -1;
	else /* setting alignment size only */
	{	blksz = (ssize_t)size;

		if(!init) /* stream already initialized */
		{	obuf = f->data;
			osize = f->size;
			goto done;
		}
		else /* initialize stream as if in the default case */
		{	buf = NULL;
			size = (size_t)SFIO_UNBOUND;
		}
	}

	bufsize = 0;
	oflags = f->flags;

#ifdef MAP_TYPE
	/* see if memory mapping is possible (see sfwrite for SFIO_BOTH) */
	okmmap = (buf || (f->flags&SFIO_STRING) || (f->flags&SFIO_RDWR) == SFIO_RDWR) ? 0 : 1;

	/* save old buffer info */
	if(f->bits&SFIO_MMAP)
	{	if(f->data)
		{	SFMUNMAP(f,f->data,f->endb-f->data);
			f->data = NULL;
		}
	} else
#endif
	if(f->data == f->tiny)
	{	f->data = NULL;
		f->size = 0;
	}
	obuf  = f->data;
	osize = f->size;

	f->flags &= ~SFIO_MALLOC;
	f->bits  &= ~SFIO_MMAP;

	/* pure read/string streams must have a valid string */
	if((f->flags&(SFIO_RDWR|SFIO_STRING)) == SFIO_RDSTR &&
	   (size == (size_t)SFIO_UNBOUND || !buf))
		size = 0;

	/* set disc to the first discipline with a seekf */
	for(disc = f->disc; disc; disc = disc->disc)
		if(disc->seekf)
			break;

	if((init || local) && !(f->flags&SFIO_STRING))
	{	/* ASSERT(f->file >= 0) */
		st.st_mode = 0;

		/* if has discipline, set size by discipline if possible */
		if(!_sys_stat || disc)
		{	if((f->here = SFSK(f,0,SEEK_CUR,disc)) < 0)
				goto unseekable;
			else
			{	Sfoff_t	e;
				if((e = SFSK(f,0,SEEK_END,disc)) >= 0)
					f->extent = e > f->here ? e : f->here;
				(void)SFSK(f,f->here,SEEK_SET,disc);
				goto setbuf;
			}
		}

		/* get file descriptor status */
		if(fstat((int)f->file,&st) < 0)
			f->here = -1;
		else
		{
#if _sys_stat && _stat_blksize	/* preferred io block size */
			f->blksz = (size_t)st.st_blksize;
#endif
			bufsize = 64 * 1024;
#ifdef MAP_TYPE
			if(S_ISDIR(st.st_mode) || (Sfoff_t)st.st_size < (Sfoff_t)SFIO_GRAIN)
				okmmap = 0;
#endif
			if(S_ISREG(st.st_mode) || S_ISDIR(st.st_mode))
				f->here = SFSK(f,0,SEEK_CUR,f->disc);
			else	f->here = -1;

#ifdef MAP_TYPE
#if O_TEXT /* no memory mapping with O_TEXT because read()/write() alter data stream */
			if(okmmap && f->here >= 0 &&
			   (fcntl((int)f->file,F_GETFL,0) & O_TEXT) )
				okmmap = 0;
#endif
#endif
		}

		/* set page size, this is also the desired default buffer size */
		if(_Sfpage <= 0)
		{
#if _lib_getpagesize
			if((_Sfpage = (size_t)getpagesize()) <= 0)
#endif
				_Sfpage = SFIO_PAGE;
		}

#if SFSETLINEMODE
		if(init)
			f->flags |= sfsetlinemode();
#endif

		if(f->here >= 0)
		{	f->extent = (Sfoff_t)st.st_size;

			/* seekable std-devices are share-public by default */
			if(f == sfstdin || f == sfstdout || f == sfstderr)
				f->flags |= SFIO_SHARE|SFIO_PUBLIC;
		}
		else
		{
		unseekable:
			f->extent = -1;
			f->here = 0;

			if(init)
			{	if(S_ISCHR(st.st_mode) )
				{	int oerrno = errno;

					bufsize = SFIO_GRAIN;

					/* set line mode for terminals */
					if(!(f->flags&(SFIO_LINE|SFIO_WCWIDTH)) && isatty(f->file))
						f->flags |= SFIO_LINE|SFIO_WCWIDTH;
#if _sys_stat
					else	/* special case /dev/null */
					{	int	dev, ino;
						static int null_checked, null_dev, null_ino;
						dev = (int)st.st_dev;	
						ino = (int)st.st_ino;	
						if(!null_checked)
						{	if(stat(DEVNULL,&st) < 0)
								null_checked = -1;
							else
							{	null_checked = 1;
								null_dev = (int)st.st_dev;	
								null_ino = (int)st.st_ino;	
							}
						}
						if(null_checked >= 0 && dev == null_dev && ino == null_ino)
							SFSETNULL(f);
					}
#endif
					errno = oerrno;
				}

				/* initialize side buffer for r+w unseekable streams */
				if(!f->proc && (f->bits&SFIO_BOTH) )
					(void)_sfpopen(f,-1,-1,1);
			}
		}
	}

#ifdef MAP_TYPE
	if(okmmap && size && (f->mode&SFIO_READ) && f->extent >= 0 )
	{	/* see if we can try memory mapping */
		if(!disc)
			for(disc = f->disc; disc; disc = disc->disc)
				if(disc->readf)
					break;
		if(!disc)
		{	f->bits |= SFIO_MMAP;
			if(size == (size_t)SFIO_UNBOUND)
			{	if(bufsize > _Sfpage)
					size = bufsize * SFIO_NMAP;
				else	size = _Sfpage * SFIO_NMAP;
				if(size > 256*1024)
					size = 256*1024;
			}
		}
	}
#endif

	/* get buffer space */
setbuf:
	if(size == (size_t)SFIO_UNBOUND)
	{	/* define a default size suitable for block transfer */
		if(init && osize > 0)
			size = osize;
		else if(f == sfstderr && (f->mode&SFIO_WRITE))
			size = 0;
		else if(f->flags&SFIO_STRING )
			size = SFIO_GRAIN;
		else if((f->flags&SFIO_READ) && !(f->bits&SFIO_BOTH) &&
			f->extent > 0 && f->extent < (Sfoff_t)_Sfpage )
			size = (((size_t)f->extent + SFIO_GRAIN-1)/SFIO_GRAIN)*SFIO_GRAIN;
		else if((ssize_t)(size = _Sfpage) < bufsize)
			size = bufsize;

		buf = NULL;
	}

	sf_malloc = 0;
	if(size > 0 && !buf && !(f->bits&SFIO_MMAP))
	{	/* try to allocate a buffer */
		if(obuf && size == (size_t)osize && init)
		{	buf = obuf;
			obuf = NULL;
			sf_malloc = (oflags&SFIO_MALLOC);
		}
		if(!buf)
		{	/* do allocation */
			while(!buf && size > 0)
			{	if((buf = malloc(size)) )
					break;
				else	size /= 2;
			}
			if(size > 0)
				sf_malloc = SFIO_MALLOC;
		}
	}

	if(size == 0 && !(f->flags&SFIO_STRING) && !(f->bits&SFIO_MMAP) && (f->mode&SFIO_READ))
	{	/* use the internal buffer */
		size = sizeof(f->tiny);
		buf = f->tiny;
	}

	/* set up new buffer */
	f->size = size;
	f->next = f->data = f->endr = f->endw = (uchar*)buf;
	f->endb = (f->mode&SFIO_READ) ? f->data : f->data+size;
	if(f->flags&SFIO_STRING)
	{	/* these fields are used to test actual size - see sfseek() */
		f->extent = (!sf_malloc &&
			     ((f->flags&SFIO_READ) || (f->bits&SFIO_BOTH)) ) ? size : 0;
		f->here = 0;

		/* read+string stream should have all data available */
		if((f->mode&SFIO_READ) && !sf_malloc)
			f->endb = f->data+size;
	}

	f->flags = (f->flags & ~SFIO_MALLOC)|sf_malloc;

	if(obuf && obuf != f->data && osize > 0 && (oflags&SFIO_MALLOC))
	{	free(obuf);
		obuf = NULL;
	}

done:
	_Sfi = f->val = obuf ? osize : 0;

	/* blksz is used for aligning disk block boundary while reading data to
	** optimize data transfer from disk (e.g., via direct I/O). blksz can be
	** at most f->size/2 so that data movement in buffer can be optimized.
	** blksz should also be a power-of-2 for optimal disk seeks.
	*/
	if(blksz <= 0 || (blksz & (blksz-1)) != 0 )
		blksz = SFIO_GRAIN;
	while(blksz > f->size/2)
		blksz /= 2;
	f->blksz = blksz;

	SFOPEN(f,local);

	return obuf;
}

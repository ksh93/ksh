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

/*	Get the size of a stream.
**
**	Written by Kiem-Phong Vo.
*/
Sfoff_t sfsize(Sfio_t* f)
{
	Sfdisc_t*	disc;
	int		mode;
	Sfoff_t		s;

	if(!f || ((mode = f->mode&SFIO_RDWR) != (int)f->mode && _sfmode(f,mode,0) < 0))
		return (Sfoff_t)(-1);

	if(f->flags&SFIO_STRING)
	{	SFSTRSIZE(f);
		return f->extent;
	}

	SFLOCK(f,0);

	s = f->here;

	if(f->extent >= 0)
	{	if(f->flags&(SFIO_SHARE|SFIO_APPENDWR))
		{	for(disc = f->disc; disc; disc = disc->disc)
				if(disc->seekf)
					break;
			if(!_sys_stat || disc)
			{	Sfoff_t	e;
				if((e = SFSK(f,0,SEEK_END,disc)) >= 0)
					f->extent = e;
				if(SFSK(f,f->here,SEEK_SET,disc) != f->here)
					f->here = SFSK(f,0,SEEK_CUR,disc);
			}
#if _sys_stat
			else
			{	struct stat	st;
				if(fstat(f->file,&st) < 0)
					f->extent = -1;
				else if((f->extent = st.st_size) < f->here)
					f->here = SFSK(f,0,SEEK_CUR,disc);
			}
#endif
		}

		if((f->flags&(SFIO_SHARE|SFIO_PUBLIC)) == (SFIO_SHARE|SFIO_PUBLIC))
			f->here = SFSK(f,0,SEEK_CUR,f->disc);
	}

	if(f->here != s && (f->mode&SFIO_READ) )
	{	/* buffered data is known to be invalid */
#ifdef MAP_TYPE
		if((f->bits&SFIO_MMAP) && f->data)
		{	SFMUNMAP(f,f->data,f->endb-f->data);
			f->data = NULL;
		}
#endif
		f->next = f->endb = f->endr = f->endw = f->data;
	}

	if(f->here < 0)
		f->extent = -1;
	else if(f->extent < f->here)
		f->extent = f->here;

	if((s = f->extent) >= 0)
	{	if(f->flags&SFIO_APPENDWR)
			s += (f->next - f->data);
		else if(f->mode&SFIO_WRITE)
		{	s = f->here + (f->next - f->data);
			if(s < f->extent)
				s = f->extent;
		}
	}

	SFOPEN(f,0);
	return s;
}

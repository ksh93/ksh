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

/*	Seek function that knows discipline
**
**	Written by Kiem-Phong Vo.
*/
Sfoff_t sfsk(Sfio_t* f, Sfoff_t addr, int type, Sfdisc_t* disc)
{
	Sfoff_t		p;
	Sfdisc_t*	dc;
	ssize_t		s;
	int		local, mode;

	if(!f)
		return (Sfoff_t)(-1);

	GETLOCAL(f,local);
	if(!local && !(f->bits&SFIO_DCDOWN))
	{	if((mode = f->mode&SFIO_RDWR) != (int)f->mode && _sfmode(f,mode,0) < 0)
			return (Sfoff_t)(-1);
		if(SFSYNC(f) < 0)
			return (Sfoff_t)(-1);
#ifdef MAP_TYPE
		if(f->mode == SFIO_READ && (f->bits&SFIO_MMAP) && f->data)
		{	SFMUNMAP(f, f->data, f->endb-f->data);
			f->data = NULL;
		}
#endif
		f->next = f->endb = f->endr = f->endw = f->data;
	}

	if((type &= (SEEK_SET|SEEK_CUR|SEEK_END)) > SEEK_END)
		return (Sfoff_t)(-1);

	for(;;)
	{	dc = disc;
		if(f->flags&SFIO_STRING)
		{	SFSTRSIZE(f);
			if(type == SEEK_SET)
				s = (ssize_t)addr;
			else if(type == SEEK_CUR)
				s = (ssize_t)(addr + f->here);
			else	s = (ssize_t)(addr + f->extent);
		}
		else
		{	SFDISC(f,dc,seekf);
			if(dc && dc->seekf)
			{	SFDCSK(f,addr,type,dc,p);
			}
			else
			{	p = lseek(f->file,(off_t)addr,type);
			}
			if(p >= 0)
				return p;
			s = -1;
		}

		if(local)
			SETLOCAL(f);
		switch(_sfexcept(f,SFIO_SEEK,s,dc))
		{
		case SFIO_EDISC:
		case SFIO_ECONT:
			if(f->flags&SFIO_STRING)
				return (Sfoff_t)s;
			goto do_continue;
		default:
			return (Sfoff_t)(-1);
		}

	do_continue:
		for(dc = f->disc; dc; dc = dc->disc)
			if(dc == disc)
				break;
		disc = dc;
	}
}

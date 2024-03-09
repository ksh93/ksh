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

/*	Delete all pending data in the buffer
**
**	Written by Kiem-Phong Vo.
*/

int sfpurge(Sfio_t* f)
{
	int	mode;

	if(!f || (mode = f->mode&SFIO_RDWR) != (int)f->mode && _sfmode(f,mode|SFIO_SYNCED,0) < 0)
		return -1;

	if((f->flags&SFIO_IOCHECK) && f->disc && f->disc->exceptf)
		(void)(*f->disc->exceptf)(f,SFIO_PURGE,(void*)((int)1),f->disc);

	if(f->disc == _Sfudisc)
		(void)sfclose((*_Sfstack)(f,NULL));

	/* cannot purge read string streams */
	if((f->flags&SFIO_STRING) && (f->mode&SFIO_READ) )
		goto done;

	SFLOCK(f,0);

	/* if memory map must be a read stream, pretend data is gone */
#ifdef MAP_TYPE
	if(f->bits&SFIO_MMAP)
	{	f->here -= f->endb - f->next;
		if(f->data)
		{	SFMUNMAP(f,f->data,f->endb-f->data);
			(void)SFSK(f,f->here,SEEK_SET,f->disc);
		}
		SFOPEN(f,0);
		return 0;
	}
#endif

	switch(f->mode&~SFIO_LOCK)
	{
	default :
		SFOPEN(f,0);
		return -1;
	case SFIO_WRITE :
		f->next = f->data;
		if(!f->proc || !(f->flags&SFIO_READ) || !(f->mode&SFIO_WRITE) )
			break;

		/* 2-way pipe, must clear read buffer */
		(void)_sfmode(f,SFIO_READ,1);
		/* FALLTHROUGH */
	case SFIO_READ:
		if(f->extent >= 0 && f->endb > f->next)
		{	f->here -= f->endb-f->next;
			(void)SFSK(f,f->here,SEEK_SET,f->disc);
		}
		f->endb = f->next = f->data;
		break;
	}

	SFOPEN(f,0);

done:
	if((f->flags&SFIO_IOCHECK) && f->disc && f->disc->exceptf)
		(void)(*f->disc->exceptf)(f,SFIO_PURGE,(void*)((int)0),f->disc);

	return 0;
}

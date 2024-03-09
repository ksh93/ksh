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

/*	Tell the current location in a given stream
**
**	Written by Kiem-Phong Vo.
*/

Sfoff_t sftell(Sfio_t* f)
{	
	int	mode;
	Sfoff_t	p;

	/* set the stream to the right mode */
	if(!f || ((mode = f->mode&SFIO_RDWR) != (int)f->mode && _sfmode(f,mode,0) < 0))
		return (Sfoff_t)(-1);

	/* throw away ungetc data */
	if(f->disc == _Sfudisc)
		(void)sfclose((*_Sfstack)(f,NULL));

	if(f->flags&SFIO_STRING)
		return (Sfoff_t)(f->next-f->data);

	/* let sfseek() handle the hard case */
	if(f->extent >= 0 && (f->flags&(SFIO_SHARE|SFIO_APPENDWR)) )
		p = sfseek(f,0,SEEK_CUR);
	else	p = f->here + ((f->mode&SFIO_WRITE) ? f->next-f->data : f->next-f->endb);

	return p;
}

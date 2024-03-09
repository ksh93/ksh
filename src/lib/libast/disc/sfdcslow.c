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
#include "sfdchdr.h"

/*	Make a stream op return immediately on interrupts.
**	This is useful on slow streams (hence the name).
**
**	Written by Glenn Fowler (03/18/1998).
*/

static int slowexcept(Sfio_t* f, int type, void* v, Sfdisc_t* disc)
{
	NOT_USED(f);
	NOT_USED(v);
	NOT_USED(disc);

	switch (type)
	{
	case SFIO_FINAL:
	case SFIO_DPOP:
		free(disc);
		break;
	case SFIO_READ:
	case SFIO_WRITE:
		if (errno == EINTR)
			return -1;
		break;
	}

	return 0;
}

int sfdcslow(Sfio_t* f)
{
	Sfdisc_t*	disc;

	if(!(disc = (Sfdisc_t*)malloc(sizeof(Sfdisc_t))) )
		return -1;

	disc->readf = NULL;
	disc->writef = NULL;
	disc->seekf = NULL;
	disc->exceptf = slowexcept;

	if(sfdisc(f,disc) != disc)
	{	free(disc);
		return -1;
	}
	sfset(f,SFIO_IOINTR,1);

	return 0;
}

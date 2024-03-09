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

/*	External variables and functions used only by Sfio
**	Written by Kiem-Phong Vo
*/

/* global variables used internally to the package */
Sfextern_t _Sfextern =
{	0,						/* _Sfpage	*/
	{ NULL, 0, 0, 0, NULL },			/* _Sfpool	*/
	NULL,						/* _Sfpmove	*/
	NULL,						/* _Sfstack	*/
	NULL,						/* _Sfnotify	*/
	NULL,						/* _Sfstdsync	*/
	{ NULL,						/* _Sfudisc	*/
	  NULL,
	  NULL,
	  NULL,
	  NULL
	},
	NULL,						/* _Sfcleanup	*/
	0,						/* _Sfexiting	*/
	0,						/* _Sfdone	*/
};

ssize_t	_Sfi = -1;		/* value for a few fast macro functions	*/
ssize_t	_Sfmaxr = 0;		/* default (unlimited) max record size	*/

Sfio_t	_Sfstdin  = SFNEW(NULL,-1,0,(SFIO_READ |SFIO_STATIC),NULL);
Sfio_t	_Sfstdout = SFNEW(NULL,-1,1,(SFIO_WRITE|SFIO_STATIC),NULL);
Sfio_t	_Sfstderr = SFNEW(NULL,-1,2,(SFIO_WRITE|SFIO_STATIC),NULL);

#undef	sfstdin
#undef	sfstdout
#undef	sfstderr

Sfio_t*	sfstdin  = &_Sfstdin;
Sfio_t*	sfstdout = &_Sfstdout;
Sfio_t*	sfstderr = &_Sfstderr;

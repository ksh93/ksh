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

/* Walk streams and run operations on them
**
** Written by Kiem-Phong Vo.
*/

int sfwalk(Sfwalk_f	walkf,	/* return <0: stop, >=0: continue	*/
	   void*	data,
	   int		type)	/* walk streams with all given flags	*/
{
	Sfpool_t	*p;
	Sfio_t		*f;
	int		n, rv;

	/* truly initializing std-streams before walking */
	if(sfstdin->mode & SFIO_INIT)
		_sfmode(sfstdin, (sfstdin->mode & SFIO_RDWR), 0);
	if(sfstdout->mode & SFIO_INIT)
		_sfmode(sfstdout, (sfstdout->mode & SFIO_RDWR), 0);
	if(sfstderr->mode & SFIO_INIT)
		_sfmode(sfstderr, (sfstderr->mode & SFIO_RDWR), 0);

	for(rv = 0, p = &_Sfpool; p; p = p->next)
	{	for(n = 0; n < p->n_sf; )
		{	f = p->sf[n];

			if(type != 0 && (f->_flags&type) != type )
				continue; /* not in the interested set */

			if((rv = (*walkf)(f, data)) < 0)
				return rv;

			if(p->sf[n] == f) /* move forward to next stream */
				n += 1;
			/* else - a sfclose() was done on current stream */
		}
	}

	return rv;
}

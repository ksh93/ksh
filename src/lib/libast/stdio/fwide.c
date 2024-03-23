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

#include "stdhdr.h"

#if !_has_multibyte

NoN(fwide)

#else

int
fwide(Sfio_t* f, int mode)
{
	if (mode > 0)
	{
		f->bits &= ~SFIO_MB;
		f->bits |= SFIO_WC;
	}
	else if (mode < 0)
	{
		f->bits &= ~SFIO_WC;
		f->bits |= SFIO_MB;
	}
	if (f->bits & SFIO_MB)
		return -1;
	if (f->bits & SFIO_WC)
		return 1;
	if ((f->flags & SFIO_SYNCED) || f->next > f->data)
	{
		f->bits |= SFIO_MB;
		return -1;
	}
	return 0;
}

#endif /* !_has_multibyte */

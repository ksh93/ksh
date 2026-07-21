/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2026 Contributors to ksh 93u+m           *
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
/*
 * G. S. Fowler
 * AT&T Bell Laboratories
 *
 * return modex canonical representation of file mode bits
 * given ls -l style file mode string
 */

#include "modelib.h"

mode_t
strmode(const char* s)
{
	int		c;
	char*		t;
	struct modeop*	p;
	mode_t		mode;

	mode = 0;
	for (p = modetab; (c = *s++) && p < &modetab[MODELEN]; p++)
		for (t = p->name; *t; t++)
			if (*t == c)
			{
				c = (int)(t - p->name);
				mode |= (p->mask1 & ((mode_t)c << p->shift1)) | (p->mask2 & ((mode_t)c << p->shift2));
				break;
			}
	return mode;
}

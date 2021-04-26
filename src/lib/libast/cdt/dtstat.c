/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2021 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 1.0                  *
*                    by AT&T Intellectual Property                     *
*                                                                      *
*                A copy of the License is available at                 *
*          http://www.eclipse.org/org/documents/epl-v10.html           *
*         (with md5 checksum b35adb5213ca9657e911e9befb180842)         *
*                                                                      *
*              Information and Software Systems Research               *
*                            AT&T Research                             *
*                           Florham Park NJ                            *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  David Korn <dgk@research.att.com>                   *
*                   Phong Vo <kpv@research.att.com>                    *
*                                                                      *
***********************************************************************/
#include	"dthdr.h"

/* Get statistics for a dictionary
**
** Written by Kiem-Phong Vo
*/

ssize_t dtstat(Dt_t* dt, Dtstat_t* dtst)
{
	ssize_t	sz, k, maxk;
	char	*str;
	char	*end;

	sz = (ssize_t)(*dt->meth->searchf)(dt, (Void_t*)dtst, DT_STAT);

	str = dtst->mesg;
	end = &dtst->mesg[sizeof(dtst->mesg)] - 1;
	str += sfsprintf(str, end - str, "Objects=%d Levels=%d(Largest:", dtst->size, dtst->mlev+1);

	/* print top 3 levels */
	for(k = maxk = 0; k <= dtst->mlev; ++k)
		if(dtst->lsize[k] > dtst->lsize[maxk])
			maxk = k;
	if(maxk > 0)
		maxk -= 1;
	for(k = 0; k < 3 && maxk <= dtst->mlev; ++k, ++maxk)
		str += sfsprintf(str, end - str, " lev[%d]=%d", maxk, dtst->lsize[maxk] );
	if (str < end)
		*str++ = ')';
	*str = 0;

	return sz;
}

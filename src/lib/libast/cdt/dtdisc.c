/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2025 Contributors to ksh 93u+m           *
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
#include	"dthdr.h"

/*	Change discipline.
**	dt :	dictionary
**	disc :	discipline
**
**	Written by Kiem-Phong Vo (5/26/96)
*/

static void* dtmemory(Dt_t* 	dt,	/* dictionary			*/
		      void* 	addr,	/* address to be manipulate	*/
		      size_t	size,	/* size to obtain		*/
		      Dtdisc_t* disc)	/* discipline			*/
{
	NOT_USED(dt);
	NOT_USED(disc);
	if(addr)
	{	if(size == 0)
		{	free(addr);
			return NULL;
		}
		return realloc(addr,size);
	}
	return size > 0 ? malloc(size) : NULL;
}

Dtdisc_t* dtdisc(Dt_t* dt, Dtdisc_t* disc, int type)
{
	Dtdisc_t	*old;
	Dtlink_t	*list;

	if(!(old = dt->disc) )	/* initialization call from dtopen() */
	{	dt->disc = disc;
		if(!(dt->memoryf = disc->memoryf) )
			dt->memoryf = dtmemory;
		return disc;
	}

	if(!disc) /* only want to know current discipline */
		return old;

	if(old->eventf && (*old->eventf)(dt,DT_DISC,disc,old) < 0)
		return NULL;

	if((type & (DT_SAMEHASH|DT_SAMECMP)) != (DT_SAMEHASH|DT_SAMECMP) )
		list = dtextract(dt); /* grab the list of objects if any */
	else	list = NULL;

	dt->disc = disc;
	if(!(dt->memoryf = disc->memoryf) )
		dt->memoryf = dtmemory;

	if(list ) /* reinsert extracted objects (with new discipline) */
		dtrestore(dt, list);

	return old;
}

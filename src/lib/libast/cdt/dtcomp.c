/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
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
/*
 * backwards binary compatibility
 */

#include <cdt.h>

#if defined(__EXPORT__)
#define extern	__EXPORT__
#endif

#undef dtflatten
extern Dtlink_t* dtflatten(Dt_t* d)
{
	return (Dtlink_t*)(*(_DT(d)->searchf))((d),(void*)(0),DT_FLATTEN);
}

#undef dtextract
extern Dtlink_t* dtextract(Dt_t* d)
{
	return (Dtlink_t*)(*(_DT(d)->searchf))((d),(void*)(0),DT_EXTRACT);
}

#undef dtrestore
extern Dtlink_t* dtrestore(Dt_t* d, void* l)
{
	return (Dtlink_t*)(*(_DT(d)->searchf))((d),(l),DT_RESTORE);
}

#undef dtsize
extern ssize_t dtsize(Dt_t* d)
{
	return (ssize_t)(*(_DT(d)->searchf))((d),(void*)(0),DT_STAT);
}

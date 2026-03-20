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
#include	"dthdr.h"

/*	Close a dictionary
**
**	Written by Kiem-Phong Vo (11/15/2010)
*/
int dtclose(Dt_t* dt)
{
	int		ev;
	unsigned int	type;
	Dt_t		pdt;
	Dtdisc_t	*disc = dt->disc;

	if(!dt || dt->nview > 0 ) /* can't close if being viewed */
		return -1;

	if(disc && disc->eventf) /* announce closing event */
		ev = (*disc->eventf)(dt, DT_CLOSE, (void*)1, disc);
	else	ev = 0;
	if(ev < 0) /* cannot close */
		return -1;

	if(dt->view) /* turn off viewing at this point */
		dtview(dt,NULL);

	type = dt->data->type; /* save before memory is freed */
	memcpy(&pdt, dt, sizeof(Dt_t));

	if(ev == 0 ) /* release all allocated data */
	{	(void)(*(dt->meth->searchf))(dt,NULL,DT_CLEAR);
		(void)(*dt->meth->eventf)(dt, DT_CLOSE, NULL);
		/**/DEBUG_ASSERT(!dt->data);
	}
	if(!(type&DT_INDATA) )
		(void)free(dt);

	if(disc && disc->eventf) /* announce end of closing activities */
		(void)(*disc->eventf)(&pdt, DT_ENDCLOSE, NULL, disc);

	return 0;
}

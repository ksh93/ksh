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


/*	Push/pop streams
**
**	Written by Kiem-Phong Vo.
*/

Sfio_t* sfstack(Sfio_t*	f1,	/* base of stack	*/
	        Sfio_t*	f2)	/* top of stack		*/
{
	int		n;
	Sfio_t*		rf;
	Sfrsrv_t*	rsrv;

	if(f1 && (f1->mode&SFIO_RDWR) != f1->mode && _sfmode(f1,0,0) < 0)
		return NULL;
	if(f2 && (f2->mode&SFIO_RDWR) != f2->mode && _sfmode(f2,0,0) < 0)
		return NULL;
	if(!f1)
		return f2;

	/* give access to other internal functions */
	_Sfstack = sfstack;

	if(f2 == SFIO_POPSTACK)
	{	if(!(f2 = f1->push))
			return NULL;
		f2->mode &= ~SFIO_PUSH;
	}
	else
	{	if(f2->push)
			return NULL;
		if(f1->pool && f1->pool != &_Sfpool && f1->pool != f2->pool &&
		   f1 == f1->pool->sf[0])
		{	/* get something else to pool front since f1 will be locked */
			for(n = 1; n < f1->pool->n_sf; ++n)
			{	if(SFFROZEN(f1->pool->sf[n]) )
					continue;
				(*_Sfpmove)(f1->pool->sf[n],0);
				break;
			}
		}
	}

	if(f2->pool && f2->pool != &_Sfpool && f2 != f2->pool->sf[0])
		(*_Sfpmove)(f2,0);

	/* swap streams */
	sfswap(f1,f2);

	/* but the reserved buffer must remain the same */
	rsrv = f1->rsrv; f1->rsrv = f2->rsrv; f2->rsrv = rsrv;

	SFLOCK(f1,0);
	SFLOCK(f2,0);

	if(f2->push != f2)
	{	/* freeze the pushed stream */
		f2->mode |= SFIO_PUSH;
		f1->push = f2;
		rf = f1;
	}
	else
	{	/* unfreeze the just exposed stream */
		f1->mode &= ~SFIO_PUSH;
		f2->push = NULL;
		rf = f2;
	}

	SFOPEN(f1,0);
	SFOPEN(f2,0);

	return rf;
}

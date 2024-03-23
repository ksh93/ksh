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
#include	"sfhdr.h"

/*	Synchronize data in buffers with the file system.
**	If f is nil, all streams are sync-ed
**
**	Written by Kiem-Phong Vo.
*/

static int _sfall(void)
{
	Sfpool_t	*p, *next;
	Sfio_t*		f;
	int		n, rv;
	int		nsync, count, loop;
#define MAXLOOP 3

	for(loop = 0; loop < MAXLOOP; ++loop)
	{	rv = nsync = count = 0;
		for(p = &_Sfpool; p; p = next)
		{	/* find the next legitimate pool */
			for(next = p->next; next; next = next->next)
				if(next->n_sf > 0)
					break;

			/* walk the streams for _Sfpool only */
			for(n = 0; n < ((p == &_Sfpool) ? p->n_sf : 1); ++n)
			{	count += 1;
				f = p->sf[n];

				if(f->flags&SFIO_STRING )
					goto did_sync;
				if(SFFROZEN(f))
					continue;
				if((f->mode&SFIO_READ) && (f->mode&SFIO_SYNCED) )
					goto did_sync;
				if((f->mode&SFIO_READ) && !(f->bits&SFIO_MMAP) &&
				   f->next == f->endb)
					goto did_sync;
				if((f->mode&SFIO_WRITE) && !(f->bits&SFIO_HOLE) &&
				   f->next == f->data)
					goto did_sync;

				if(sfsync(f) < 0)
					rv = -1;

			did_sync:
				nsync += 1;
			}
		}

		if(nsync == count)
			break;
	}
	return rv;
}

int sfsync(Sfio_t* f)
{
	int	local, rv, lock;
	Sfio_t*	origf;

	if(!(origf = f) )
		return _sfall();

	if(!(origf)) return -1;

	GETLOCAL(origf,local);

	if(origf->disc == _Sfudisc)	/* throw away ungetc */
		(void)sfclose((*_Sfstack)(origf,NULL));

	rv = 0;

	lock = origf->mode&SFIO_LOCK;
	if(origf->mode == (SFIO_SYNCED|SFIO_READ) ) /* already synced */
		goto done;

	if((origf->mode&SFIO_RDWR) != SFMODE(origf,local) && _sfmode(origf,0,local) < 0)
	{	rv = -1;
		goto done;
	}

	for(; f; f = f->push)
	{
		unsigned int mode;

		if((f->flags&SFIO_IOCHECK) && f->disc && f->disc->exceptf)
			(void)(*f->disc->exceptf)(f,SFIO_SYNC,(void*)((int)1),f->disc);

		SFLOCK(f,local);

		/* pretend that this stream is not on a stack */
		mode = f->mode&SFIO_PUSH;
		f->mode &= ~SFIO_PUSH;

		/* these streams do not need synchronization */
		if((f->flags&SFIO_STRING) || (f->mode&SFIO_SYNCED))
			goto next;

		if((f->mode&SFIO_WRITE) && (f->next > f->data || (f->bits&SFIO_HOLE)) )
		{	/* sync the buffer, make sure pool doesn't move */
			unsigned int pool = f->mode&SFIO_POOL;
			f->mode &= ~SFIO_POOL;
			if(f->next > f->data && (SFWRALL(f), SFFLSBUF(f,-1)) < 0)
				rv = -1;
			if(!SFISNULL(f) && (f->bits&SFIO_HOLE) )
			{	/* realize a previously created hole of 0's */
				if(SFSK(f,(Sfoff_t)(-1),SEEK_CUR,f->disc) >= 0)
					(void)SFWR(f,"",1,f->disc);
				f->bits &= ~SFIO_HOLE;
			}
			f->mode |= pool;
		}

		if((f->mode&SFIO_READ) && f->extent >= 0 &&
		   ((f->bits&SFIO_MMAP) || f->next < f->endb) )
		{	/* make sure the file pointer is at the right place */
			f->here -= (f->endb-f->next);
			f->endr = f->endw = f->data;
			f->mode = SFIO_READ|SFIO_SYNCED|lock;
			(void)SFSK(f,f->here,SEEK_SET,f->disc);

			if((f->flags&SFIO_SHARE) && !(f->flags&SFIO_PUBLIC) &&
			   !(f->bits&SFIO_MMAP) )
			{	f->endb = f->next = f->data;
				f->mode &= ~SFIO_SYNCED;
			}
		}

	next:
		f->mode |= mode;
		SFOPEN(f,local);

		if((f->flags&SFIO_IOCHECK) && f->disc && f->disc->exceptf)
			(void)(*f->disc->exceptf)(f,SFIO_SYNC,(void*)((int)0),f->disc);
	}

done:
	if(!local && f && (f->mode&SFIO_POOL) && f->pool && f != f->pool->sf[0])
		SFSYNC(f->pool->sf[0]);

	return rv;
}

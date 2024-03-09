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

/*	Add a new discipline to the discipline stack. Each discipline
**	provides alternative I/O functions that are analogues of the
**	system calls.
**
**	When the application fills or flushes the stream buffer, data
**	will be processed through discipline functions. A case deserving
**	consideration is stacking a discipline onto a read stream. Each
**	discipline operation implies buffer synchronization so the stream
**	buffer should be empty. However, a read stream representing an
**	unseekable device (e.g., a pipe) may not be synchronizable. In that
**	case, any buffered data must then be fed to the new discipline
**	to preserve data processing semantics. This is done by creating
**	a temporary discipline to cache such buffered data and feed
**	them to the new discipline when its readf() asks for new data.
**	Care must then be taken to remove this temporary discipline
**	when it runs out of cached data.
**
**	Written by Kiem-Phong Vo
*/

typedef struct _dccache_s
{	Sfdisc_t	disc;
	uchar*		data;
	uchar*		endb;
} Dccache_t;

static int _dccaexcept(Sfio_t* f, int type, void* val, Sfdisc_t* disc)
{
	if(disc && type == SFIO_FINAL)
		free(disc);
	return 0;
}

static ssize_t _dccaread(Sfio_t* f, void* buf, size_t size, Sfdisc_t* disc)
{
	ssize_t		sz;
	Sfdisc_t	*prev;
	Dccache_t	*dcca;

	if(!f) /* bad stream */
		return -1;

	/* make sure that this is on the discipline stack */
	for(prev = f->disc; prev; prev = prev->disc)
		if(prev->disc == disc)
			break;
	if(!prev)
		return -1;

	if(size <= 0) /* nothing to do */
		return size;

	/* read from available data */
	dcca = (Dccache_t*)disc;
	if((sz = dcca->endb - dcca->data) > (ssize_t)size)
		sz = (ssize_t)size;
	memcpy(buf, dcca->data, sz);

	if((dcca->data += sz) >= dcca->endb) /* free empty cache */
	{	prev->disc = disc->disc;
		free(disc);
	}

	return sz;
}

Sfdisc_t* sfdisc(Sfio_t* f, Sfdisc_t* disc)
{
	Sfdisc_t	*d, *rdisc;
	Sfread_f	oreadf;
	Sfwrite_f	owritef;
	Sfseek_f	oseekf;
	ssize_t		n;
	Dccache_t	*dcca = NULL;

	if(!f)
		return NULL;

	if((Sfio_t*)disc == f) /* special case to get the top discipline */
		return f->disc;

	if((f->flags&SFIO_READ) && f->proc && (f->mode&SFIO_WRITE) )
	{	/* make sure in read mode to check for read-ahead data */
		if(_sfmode(f,SFIO_READ,0) < 0)
			return NULL;
	}
	else
	{	if((f->mode&SFIO_RDWR) != f->mode && _sfmode(f,0,0) < 0)
			return NULL;
	}

	SFLOCK(f,0);
	rdisc = NULL;

	/* disallow popping while there is cached data */
	if(!disc && f->disc && f->disc->disc && f->disc->disc->readf == _dccaread )
		goto done;

	/* synchronize before switching to a new discipline */
	if(!(f->flags&SFIO_STRING))
	{	(void)SFSYNC(f); /* do a silent buffer synch */
		if((f->mode&SFIO_READ) && (f->mode&SFIO_SYNCED) )
		{	f->mode &= ~SFIO_SYNCED;
			f->endb = f->next = f->endr = f->endw = f->data;
		}

		/* if there is buffered data, ask app before proceeding */
		if(((f->mode&SFIO_WRITE) && (n = f->next-f->data) > 0) ||
		   ((f->mode&SFIO_READ) && (n = f->endb-f->next) > 0) )
		{	int	rv = 0;
			if(rv == 0 && f->disc && f->disc->exceptf) /* ask current discipline */
			{	SFOPEN(f,0);
				rv = (*f->disc->exceptf)(f, SFIO_DBUFFER, &n, f->disc);
				SFLOCK(f,0);
			}
			if(rv == 0 && disc && disc->exceptf) /* ask discipline being pushed */
			{	SFOPEN(f,0);
				rv = (*disc->exceptf)(f, SFIO_DBUFFER, &n, disc);
				SFLOCK(f,0);
			}
			if(rv < 0)
				goto done;
		}

		/* trick the new discipline into processing already buffered data */
		if((f->mode&SFIO_READ) && n > 0 && disc && disc->readf )
		{	if(!(dcca = (Dccache_t*)malloc(sizeof(Dccache_t)+n)) )
				goto done;
			memclear(dcca, sizeof(Dccache_t));

			dcca->disc.readf = _dccaread;
			dcca->disc.exceptf = _dccaexcept;

			/* move buffered data into the temp discipline */
			dcca->data = ((uchar*)dcca) + sizeof(Dccache_t);
			dcca->endb = dcca->data + n;
			memcpy(dcca->data, f->next, n);
			f->endb = f->next = f->endr = f->endw = f->data;
		}
	}

	/* save old readf, writef, and seekf to see if stream need reinit */
#define GETDISCF(func,iof) \
	{ \
	  for(d = f->disc; d && !d->iof; d = d->disc) ; \
	  func = d ? d->iof : NULL; \
	}
	GETDISCF(oreadf,readf);
	GETDISCF(owritef,writef);
	GETDISCF(oseekf,seekf);

	if(disc == SFIO_POPDISC)
	{	/* popping, warn the being popped discipline */
		if(!(d = f->disc) )
			goto done;
		disc = d->disc;
		if(d->exceptf)
		{	SFOPEN(f,0);
			if((*(d->exceptf))(f,SFIO_DPOP,disc,d) < 0 )
				goto done;
			SFLOCK(f,0);
		}
		f->disc = disc;
		rdisc = d;
	}
	else
	{	/* pushing, warn being pushed discipline */
		do
		{	/* loop to handle the case where d may pop itself */
			d = f->disc;
			if(d && d->exceptf)
			{	SFOPEN(f,0);
				if( (*(d->exceptf))(f,SFIO_DPUSH,disc,d) < 0 )
					goto done;
				SFLOCK(f,0);
			}
		} while(d != f->disc);

		/* make sure we are not creating an infinite loop */
		for(; d; d = d->disc)
			if(d == disc)
				goto done;

		/* set new disc */
		if(dcca) /* insert the discipline with cached data */
		{	dcca->disc.disc = f->disc;
			disc->disc = &dcca->disc;
		}
		else	disc->disc = f->disc;
		f->disc = disc;
		rdisc = disc;
	}

	if(!(f->flags&SFIO_STRING) )
	{	/* this stream may have to be reinitialized */
		int	reinit = 0;
#define DISCF(dst,iof)	(dst ? dst->iof : NULL)
#define REINIT(oiof,iof) \
		if(!reinit) \
		{ \
			for(d = f->disc; d && !d->iof; d = d->disc) ; \
			if(DISCF(d,iof) != oiof) \
				reinit = 1; \
		}

		REINIT(oreadf,readf);
		REINIT(owritef,writef);
		REINIT(oseekf,seekf);

		if(reinit)
		{	SETLOCAL(f);
			f->bits &= ~SFIO_NULL;	/* turn off /dev/null handling */
			if((f->bits&SFIO_MMAP) || (f->mode&SFIO_INIT))
				sfsetbuf(f,NULL,(size_t)SFIO_UNBOUND);
			else if(f->data == f->tiny)
				sfsetbuf(f,NULL,0);
			else
			{	int	flags = f->flags;
				sfsetbuf(f,f->data,f->size);
				f->flags |= (flags&SFIO_MALLOC);
			}
		}
	}

done :
	SFOPEN(f,0);
	return rdisc;
}

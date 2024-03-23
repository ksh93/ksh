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

/*	Reserve a segment of data or buffer.
**
**	Written by Kiem-Phong Vo.
*/

void* sfreserve(Sfio_t*	f,	/* file to peek */
		ssize_t	size,	/* size of peek */
		int	type)	/* LOCKR: lock stream, LASTR: last record */
{
	ssize_t		n, now, sz, iosz;
	Sfrsrv_t*	rsrv;
	void*		data;
	int		mode, local;

	if(!f)
		return NULL;

	sz = size < 0 ? -size : size;

	/* see if we need to bias toward SFIO_WRITE instead of the default SFIO_READ */
	if(type < 0)
		mode = 0;
	else if((mode = type&SFIO_WRITE) )
		type &= ~SFIO_WRITE;

	/* return the last record */
	if(type == SFIO_LASTR )
	{	if((n = f->endb - f->next) > 0 && n == f->val )
		{	data = f->next;
			f->next += n;
		}
		else if((rsrv = f->rsrv) && (n = -rsrv->slen) > 0)
		{	rsrv->slen = 0;
			_Sfi = f->val = n;
			data = rsrv->data;
		}
		else
		{	_Sfi = f->val = -1;
			data = NULL;
		}

		return data;
	}

	if(type > 0)
	{	if(type == 1 ) /* upward compatibility mode */
			type = SFIO_LOCKR;
		else if(type != SFIO_LOCKR)
			return NULL;
	}

	if(size == 0 && (type < 0 || type == SFIO_LOCKR) )
	{	if((f->mode&SFIO_RDWR) != f->mode && _sfmode(f,0,0) < 0)
			return NULL;

		SFLOCK(f,0);
		if((n = f->endb - f->next) < 0)
			n = 0;

		goto done;
	}

	/* iterate until get to a stream that has data or buffer space */
	for(local = 0;; local = SFIO_LOCAL)
	{	_Sfi = f->val = -1;

		if(!mode && !(mode = f->flags&SFIO_READ) )
			mode = SFIO_WRITE;
		if((int)f->mode != mode && _sfmode(f,mode,local) < 0)
		{	SFOPEN(f,0);
			return NULL;
		}

		SFLOCK(f,local);

		if((n = now = f->endb - f->next) < 0)
			n = 0;
		if(n > 0 && n >= sz) /* all done */
			break;

		/* set amount to perform IO */
		if(size == 0 || (f->mode&SFIO_WRITE))
			iosz = -1;
		else if(size < 0 && n == 0 && f->push) /* maybe stack-pop */
		{	if((iosz = f->push->endb - f->push->next) == 0)
				iosz = f->push->size;
			if(iosz < sz)
				iosz = sz; /* so only get what is asked for */
		}
		else
		{	iosz = sz - n; /* get enough to fulfill requirement */
			if(size < 0 && iosz < (f->size - n) )
				iosz = f->size - n; /* get as much as possible */
			if(iosz <= 0) /* nothing to do */
				break;
		}

		/* do a buffer refill or flush */
		now = n;
		if(f->mode&SFIO_WRITE)
			(void)SFFLSBUF(f, iosz);
		else if(type == SFIO_LOCKR && f->extent < 0 && (f->flags&SFIO_SHARE) )
		{	if(n == 0) /* peek-read only if there is no buffered data */
			{	f->mode |= SFIO_RV;
				(void)SFFILBUF(f, iosz );
			}
			if((n = f->endb - f->next) < sz)
			{	if(f->mode&SFIO_PKRD)
				{	f->endb = f->endr = f->next;
					f->mode &= ~SFIO_PKRD;
				}
				break;
			}
		}
		else
		{	/* sfreserve(f,0,0) == sfread(f, sfreserve(f,-1,SFIO_LOCKR), 0) */
			if(size == 0 && type == 0)
				f->mode |= SFIO_RV;

			(void)SFFILBUF(f, iosz );
		}

		if((n = f->endb - f->next) <= 0)
			n = 0;

		if(n >= sz) /* got it */
			break;

		if(n == now || sferror(f) || sfeof(f)) /* no progress */
			break;

		/* request was only to assess data availability */
		if(type == SFIO_LOCKR && size > 0 && n > 0 )
			break;
	}

done:	/* compute the buffer to be returned */
	data = NULL;
	if(size == 0 || n == 0)
	{	if(n > 0) /* got data */
			data = f->next;
		else if(type == SFIO_LOCKR && size == 0 && (rsrv = _sfrsrv(f,0)) )
			data = rsrv->data;
	}
	else if(n >= sz) /* got data */
		data = f->next;
	else if(f->flags&SFIO_STRING) /* try extending string buffer */
	{	if((f->mode&SFIO_WRITE) && (f->flags&SFIO_MALLOC) )
		{	(void)SFWR(f,f->next,sz,f->disc);
			if((n = f->endb - f->next) >= sz )
				data = f->next;
		}
	}
	else if(f->mode&SFIO_WRITE) /* allocate side buffer */
	{	if(type == SFIO_LOCKR && (rsrv = _sfrsrv(f, sz)) )
			data = rsrv->data;
	}
	else if(type != SFIO_LOCKR && sz > f->size && (rsrv = _sfrsrv(f,sz)) )
	{	if((n = SFREAD(f,rsrv->data,sz)) >= sz) /* read side buffer */
			data = rsrv->data;
		else	rsrv->slen = -n;
	}

	SFOPEN(f,0);

	if(data)
	{	if(type == SFIO_LOCKR)
		{	f->mode |= SFIO_PEEK;
			if((f->mode & SFIO_READ) && size == 0 && data != f->next)
				f->mode |= SFIO_GETR; /* so sfread() will unlock */
			f->endr = f->endw = f->data;
		}
		else
		{	if(data == f->next)
				f->next += (size >= 0 ? size : n);
		}
	}

	_Sfi = f->val = n; /* return true buffer size */

	return data;
}

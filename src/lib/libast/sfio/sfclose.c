/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
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
#include	"sfhdr.h"

/*	Close a stream. A file stream is synced before closing.
**
**	Written by Kiem-Phong Vo
*/

int sfclose(Sfio_t* f)
{
	int		local, ex, rv;
	void*		data = NULL;

	if(!f)
		return -1;

	GETLOCAL(f,local);

	if(!(f->mode&SFIO_INIT) &&
	   SFMODE(f,local) != (f->mode&SFIO_RDWR) &&
	   SFMODE(f,local) != (f->mode&(SFIO_READ|SFIO_SYNCED)) &&
	   _sfmode(f,SFIO_SYNCED,local) < 0)
		return -1;

	/* closing a stack of streams */
	while(f->push)
	{	Sfio_t*	pop;

		if(!(pop = (*_Sfstack)(f,NULL)) )
			return -1;

		if(sfclose(pop) < 0)
		{	(*_Sfstack)(f,pop);
			return -1;
		}
	}

	rv = 0;
	if(f->disc == _Sfudisc)	/* closing the ungetc stream */
		f->disc = NULL;
	else if(f->file >= 0)	/* sync file pointer */
	{	f->bits |= SFIO_ENDING;
		rv = sfsync(f);
	}

	SFLOCK(f,0);

	/* raise discipline exceptions */
	if(f->disc && (ex = SFRAISE(f,local ? SFIO_NEW : SFIO_CLOSING,NULL)) != 0)
		return ex;

	if(!local && f->pool)
	{	/* remove from pool */
		if(f->pool == &_Sfpool)
		{	int	n;

			for(n = 0; n < _Sfpool.n_sf; ++n)
			{	if(_Sfpool.sf[n] != f)
					continue;
				/* found it */
				_Sfpool.n_sf -= 1;
				for(; n < _Sfpool.n_sf; ++n)
					_Sfpool.sf[n] = _Sfpool.sf[n+1];
				break;
			}
		}
		else
		{	f->mode &= ~SFIO_LOCK;	/**/ASSERT(_Sfpmove);
			if((*_Sfpmove)(f,-1) < 0)
			{	SFOPEN(f,0);
				return -1;
			}
			f->mode |= SFIO_LOCK;
		}
		f->pool = NULL;
	}

	if(f->data && (!local || (f->flags&SFIO_STRING) || (f->bits&SFIO_MMAP) ) )
	{	/* free buffer */
#ifdef MAP_TYPE
		if(f->bits&SFIO_MMAP)
			SFMUNMAP(f,f->data,f->endb-f->data);
		else
#endif
		if(f->flags&SFIO_MALLOC)
			data = f->data;

		f->data = NULL;
		f->size = -1;
	}

	/* zap the file descriptor */
	if(_Sfnotify)
		(*_Sfnotify)(f, SFIO_CLOSING, (void*)((long)f->file));
	if(f->file >= 0 && !(f->flags&SFIO_STRING))
	{	errno = 0;
		ast_close(f->file);
		if(errno)
			rv = -1;
	}
	f->file = -1;

	SFKILL(f);
	f->flags &= SFIO_STATIC;
	f->here = 0;
	f->extent = -1;
	f->endb = f->endr = f->endw = f->next = f->data;

	/* zap any associated auxiliary buffer */
	if(f->rsrv)
	{	free(f->rsrv);
		f->rsrv = NULL;
	}

	/* delete any associated sfpopen-data */
	if(f->proc)
		rv = _sfpclose(f);

	if(!local)
	{	if(f->disc && (ex = SFRAISE(f,SFIO_FINAL,NULL)) != 0 )
		{	rv = ex;
			goto done;
		}

		if(!(f->flags&SFIO_STATIC) )
			free(f);
		else
		{	f->disc = NULL;
			f->stdio = NULL;
			f->mode = SFIO_AVAIL;
		}
	}

done:
	if(data)
		free(data);
	return rv;
}

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
static char*	Version = "\n@(#)$Id: sfio (AT&T Labs - Research) 2009-09-15 $\0\n";

/*	Functions to set a given stream to some desired mode
**
**	Written by Kiem-Phong Vo.
**
**	Modifications:
**		06/27/1990 (first version)
**		01/06/1991
**		07/08/1991
**		06/18/1992
**		02/02/1993
**		05/25/1993
**		02/07/1994
**		05/21/1996
**		08/01/1997
**		08/01/1998 (extended formatting)
**		09/09/1999 (thread-safe)
**		02/01/2001 (adaptive buffering)
**		05/31/2002 (multi-byte handling in sfvprintf/vscanf)
**		09/06/2002 (SFIO_IOINTR flag)
**		11/15/2002 (%#c for sfvprintf)
**		05/31/2003 (sfsetbuf(f,f,align_size) to set alignment for data)
**			   (%I1d is fixed to handle "signed char" correctly)
**		01/01/2004 Porting issues to various platforms resolved.
**		06/01/2008 Allowing notify() at entering/exiting thread-safe routines.
**		09/15/2008 Add sfwalk().
*/

/* the below is for protecting the application from SIGPIPE */
#include		<sig.h>
#include		<wait.h>
#define Sfsignal_f	Sig_handler_t
static int		_Sfsigp = 0; /* # of streams needing SIGPIPE protection */

/* done at exiting time */
static void _sfcleanup(void)
{
	Sfpool_t*	p;
	Sfio_t*		f;
	int		n;
	int		pool;

	f = (Sfio_t*)Version; /* shut compiler warning */

	/* set this so that no more buffering is allowed for write streams */
	_Sfexiting = 1001;

	sfsync(NULL);

	for(p = &_Sfpool; p; p = p->next)
	{	for(n = 0; n < p->n_sf; ++n)
		{	if(!(f = p->sf[n]) || SFFROZEN(f) )
				continue;

			SFLOCK(f,0);

			/* let application know that we are leaving */
			(void)SFRAISE(f, SFIO_ATEXIT, NULL);

			if(f->flags&SFIO_STRING)
				continue;

			/* from now on, write streams are unbuffered */
			pool = f->mode&SFIO_POOL;
			f->mode &= ~SFIO_POOL;
			if((f->flags&SFIO_WRITE) && !(f->mode&SFIO_WRITE))
				(void)_sfmode(f,SFIO_WRITE,1);
			if(f->data &&
			   ((f->bits&SFIO_MMAP) ||
			    ((f->mode&SFIO_WRITE) && f->next == f->data) ) )
				(void)SFSETBUF(f,NULL,0);
			f->mode |= pool;

			SFOPEN(f,0);
		}
	}
}

/* put into discrete pool */
int _sfsetpool(Sfio_t* f)
{
	Sfpool_t*	p;
	Sfio_t**	array;
	int		n, rv;

	if(!_Sfcleanup)
	{	_Sfcleanup = _sfcleanup;
		(void)atexit(_sfcleanup);
	}

	if(!(p = f->pool) )
		p = f->pool = &_Sfpool;


	rv = -1;

	if(p->n_sf >= p->s_sf)
	{	if(p->s_sf == 0) /* initialize pool array */
		{	p->s_sf = sizeof(p->array)/sizeof(p->array[0]);
			p->sf = p->array;
		}
		else	/* allocate a larger array */
		{	n = (p->sf != p->array ? p->s_sf : (p->s_sf/4 + 1)*4) + 4;
			if(!(array = (Sfio_t**)malloc(n*sizeof(Sfio_t*))) )
				goto done;

			/* move old array to new one */
			memcpy(array,p->sf,p->n_sf*sizeof(Sfio_t*));
			if(p->sf != p->array)
				free(p->sf);

			p->sf = array;
			p->s_sf = n;
		}
	}

	/* always add at end of array because if this was done during some sort
	   of walk through all streams, we'll want the new stream to be seen.
	*/
	p->sf[p->n_sf++] = f;
	rv = 0;

done:
	return rv;
}

/* create an auxiliary buffer for sfgetr/sfreserve/sfputr */
Sfrsrv_t* _sfrsrv(Sfio_t* f, ssize_t size)
{
	Sfrsrv_t	*rsrv, *rs;

	/* make buffer if nothing yet */
	size = ((size + SFIO_GRAIN-1)/SFIO_GRAIN)*SFIO_GRAIN;
	if(!(rsrv = f->rsrv) || size > rsrv->size)
	{	if(!(rs = (Sfrsrv_t*)malloc(size+sizeof(Sfrsrv_t))))
			size = -1;
		else
		{	if(rsrv)
			{	if(rsrv->slen > 0)
					memcpy(rs,rsrv,sizeof(Sfrsrv_t)+rsrv->slen);
				free(rsrv);
			}
			f->rsrv = rsrv = rs;
			rsrv->size = size;
			rsrv->slen = 0;
		}
	}

	if(rsrv && size > 0)
		rsrv->slen = 0;

	return size >= 0 ? rsrv : NULL;
}

int _sfpopen(Sfio_t* f, int fd, int pid, int stdio)	/* stdio popen() does not reset SIGPIPE handler */
{
	Sfproc_t*	p;

	if(f->proc)
		return 0;

	if(!(p = f->proc = (Sfproc_t*)malloc(sizeof(Sfproc_t))) )
		return -1;

	p->pid = pid;
	p->size = p->ndata = 0;
	p->rdata = NULL;
	p->file = fd;
	p->sigp = (!stdio && pid >= 0 && (f->flags&SFIO_WRITE)) ? 1 : 0;

	/* protect from broken pipe signal */
	if(p->sigp)
	{	Sfsignal_f	handler;

		if((handler = signal(SIGPIPE, SIG_IGN)) != SIG_DFL && handler != SIG_IGN)
			signal(SIGPIPE, handler); /* honor user handler */
		_Sfsigp += 1;
	}

	return 0;
}

int _sfpclose(Sfio_t* f)
{
	Sfproc_t*	p;
	int		status;

	if(!(p = f->proc))
		return -1;
	f->proc = NULL;

	if(p->rdata)
		free(p->rdata);

	if(p->pid < 0)
		status = 0;
	else
	{	/* close the associated stream */
		if(p->file >= 0)
			ast_close(p->file);

		/* wait for process termination */
		sigcritical(SIG_REG_EXEC|SIG_REG_PROC);
		status = -1;
		while (waitpid(p->pid,&status,0) == -1 && errno == EINTR)
			;
		status = status == -1 ?
			 EXIT_QUIT :
			 WIFSIGNALED(status) ?
			 EXIT_TERM(WTERMSIG(status)) :
			 EXIT_CODE(WEXITSTATUS(status));
		sigcritical(0);
		if(p->sigp && (_Sfsigp -= 1) <= 0)
		{	Sfsignal_f	handler;
			if((handler = signal(SIGPIPE,SIG_DFL)) != SIG_DFL && handler != SIG_IGN)
				signal(SIGPIPE,handler); /* honor user handler */
			_Sfsigp = 0;
		}
	}

	free(p);
	return status;
}

static int _sfpmode(Sfio_t* f, int type)
{
	Sfproc_t*	p;

	if(!(p = f->proc) )
		return -1;

	if(type == SFIO_WRITE)
	{	/* save unread data */
		p->ndata = f->endb-f->next;
		if(p->ndata > p->size)
		{	if(p->rdata)
				free(p->rdata);
			if((p->rdata = (uchar*)malloc(p->ndata)) )
				p->size = p->ndata;
			else
			{	p->size = 0;
				return -1;
			}
		}
		if(p->ndata > 0)
			memcpy(p->rdata,f->next,p->ndata);
		f->endb = f->data;
	}
	else
	{	/* restore read data */
		if(p->ndata > f->size)	/* may lose data!!! */
			p->ndata = f->size;
		if(p->ndata > 0)
		{	memcpy(f->data,p->rdata,p->ndata);
			f->endb = f->data+p->ndata;
			p->ndata = 0;
		}
	}

	/* switch file descriptor */
	if(p->pid >= 0)
	{	type = f->file;
		f->file = p->file;
		p->file = type;
	}

	return 0;
}

int _sfmode(Sfio_t*	f,	/* change r/w mode and sync file pointer for this stream */
	    int	wanted,	/* desired mode */
	    int	local)	/* a local call */
{
	int	n;
	Sfoff_t	addr;
	int	rv = 0;


	if(wanted&SFIO_SYNCED) /* for (SFIO_SYNCED|SFIO_READ) stream, just junk data */
	{	wanted &= ~SFIO_SYNCED;
		if((f->mode&(SFIO_SYNCED|SFIO_READ)) == (SFIO_SYNCED|SFIO_READ) )
		{	f->next = f->endb = f->endr = f->data;
			f->mode &= ~SFIO_SYNCED;
		}
	}

	if((!local && SFFROZEN(f)) || (!(f->flags&SFIO_STRING) && f->file < 0))
	{	if(local || !f->disc || !f->disc->exceptf)
		{	local = 1;
			goto err_notify;
		}

		for(;;)
		{	if((rv = (*f->disc->exceptf)(f,SFIO_LOCKED,0,f->disc)) < 0)
				return rv;
			if((!local && SFFROZEN(f)) ||
			   (!(f->flags&SFIO_STRING) && f->file < 0) )
			{	if(rv == 0)
				{	local = 1;
					goto err_notify;
				}
				else	continue;
			}
			else	break;
		}
	}

	if(f->mode&SFIO_GETR)
	{	f->mode &= ~SFIO_GETR;
#ifdef MAP_TYPE
		if(f->bits&SFIO_MMAP)
		{
			if (!++f->ngetr)
				f->tiny[0]++;
			if(((f->tiny[0]<<8)|f->ngetr) >= (4*SFIO_NMAP) )
			{	/* turn off mmap to avoid page faulting */
				sfsetbuf(f,f->tiny,(size_t)SFIO_UNBOUND);
				f->ngetr = f->tiny[0] = 0;
			}
		}
#endif
		if(f->getr)
		{	f->next[-1] = f->getr;
			f->getr = 0;
		}
	}

	if(f->mode&SFIO_STDIO) /* synchronizing with stdio pointers */
		(*_Sfstdsync)(f);

	if(f->disc == _Sfudisc && wanted == SFIO_WRITE &&
	   sfclose((*_Sfstack)(f,NULL)) < 0 )
	{	local = 1;
		goto err_notify;
	}

	if(f->mode&SFIO_POOL)
	{	/* move to head of pool */
		if(f == f->pool->sf[0] || (*_Sfpmove)(f,0) < 0 )
		{	local = 1;
			goto err_notify;
		}
		f->mode &= ~SFIO_POOL;
	}

	SFLOCK(f,local);

	/* buffer initialization */
	wanted &= SFIO_RDWR;
	if(f->mode&SFIO_INIT)
	{
		if(!f->pool && _sfsetpool(f) < 0)
		{	rv = -1;
			goto done;
		}

		if(wanted == 0)
			goto done;

		if(wanted != (int)(f->mode&SFIO_RDWR) && !(f->flags&wanted) )
			goto err_notify;

		if((f->flags&SFIO_STRING) && f->size >= 0 && f->data)
		{	f->mode &= ~SFIO_INIT;
			f->extent = ((f->flags&SFIO_READ) || (f->bits&SFIO_BOTH)) ?
					f->size : 0;
			f->here = 0;
			f->endb = f->data + f->size;
			f->next = f->endr = f->endw = f->data;
			if(f->mode&SFIO_READ)
				f->endr = f->endb;
			else	f->endw = f->endb;
		}
		else
		{	n = f->flags;
			(void)SFSETBUF(f,f->data,f->size);
			f->flags |= (n&SFIO_MALLOC);
		}
	}

	if(wanted == (int)SFMODE(f,1))
		goto done;

	switch(SFMODE(f,1))
	{
	case SFIO_WRITE: /* switching to SFIO_READ */
		if(wanted == 0 || wanted == SFIO_WRITE)
			break;
		if(!(f->flags&SFIO_READ) )
			goto err_notify;
		else if(f->flags&SFIO_STRING)
		{	SFSTRSIZE(f);
			f->endb = f->data+f->extent;
			f->mode = SFIO_READ;
			break;
		}

		/* reset buffer */
		if(f->next > f->data && SFFLSBUF(f,-1) < 0)
			goto err_notify;

		if(f->size == 0)
		{	/* unbuffered */
			f->data = f->tiny;
			f->size = sizeof(f->tiny);
		}
		f->next = f->endr = f->endw = f->endb = f->data;
		f->mode = SFIO_READ|SFIO_LOCK;

		/* restore saved read data for coprocess */
		if(f->proc && _sfpmode(f,wanted) < 0)
			goto err_notify;

		break;

	case (SFIO_READ|SFIO_SYNCED): /* a previously sync-ed read stream */
		if(wanted != SFIO_WRITE)
		{	/* just reset the pointers */
			f->mode = SFIO_READ|SFIO_LOCK;

			/* see if must go with new physical location */
			if((f->flags&(SFIO_SHARE|SFIO_PUBLIC)) == (SFIO_SHARE|SFIO_PUBLIC) &&
			   (addr = SFSK(f,0,SEEK_CUR,f->disc)) != f->here)
			{
#ifdef MAP_TYPE
				if((f->bits&SFIO_MMAP) && f->data)
				{	SFMUNMAP(f,f->data,f->endb-f->data);
					f->data = NULL;
				}
#endif
				f->endb = f->endr = f->endw = f->next = f->data;
				f->here = addr;
			}
			else
			{	addr = f->here + (f->endb - f->next);
				if(SFSK(f,addr,SEEK_SET,f->disc) < 0)
					goto err_notify;
				f->here = addr;
			}

			break;
		}
		/* FALLTHROUGH */

	case SFIO_READ: /* switching to SFIO_WRITE */
		if(wanted != SFIO_WRITE)
			break;
		else if(!(f->flags&SFIO_WRITE))
			goto err_notify;
		else if(f->flags&SFIO_STRING)
		{	f->endb = f->data+f->size;
			f->mode = SFIO_WRITE|SFIO_LOCK;
			break;
		}

		/* save unread data before switching mode */
		if(f->proc && _sfpmode(f,wanted) < 0)
			goto err_notify;

		/* reset buffer and seek pointer */
		if(!(f->mode&SFIO_SYNCED) )
		{	intptr_t nn = f->endb - f->next;
			if(f->extent >= 0 && (nn > 0 || (f->data && (f->bits&SFIO_MMAP))) )
			{	/* reset file pointer */
				addr = f->here - nn;
				if(SFSK(f,addr,SEEK_SET,f->disc) < 0)
					goto err_notify;
				f->here = addr;
			}
		}

		f->mode = SFIO_WRITE|SFIO_LOCK;
#ifdef MAP_TYPE
		if(f->bits&SFIO_MMAP)
		{	if(f->data)
				SFMUNMAP(f,f->data,f->endb-f->data);
			(void)SFSETBUF(f,f->tiny,(size_t)SFIO_UNBOUND);
		}
#endif
		if(f->data == f->tiny)
		{	f->endb = f->data = f->next = NULL;
			f->size = 0;
		}
		else	f->endb = (f->next = f->data) + f->size;

		break;

	default: /* unknown case */
	err_notify:
		if((wanted &= SFIO_RDWR) == 0 && (wanted = f->flags&SFIO_RDWR) == SFIO_RDWR)
			wanted = SFIO_READ;

		/* set errno for operations that access wrong stream type */
		if(wanted != (f->mode&SFIO_RDWR) && f->file >= 0)
			errno = EBADF;

		if(_Sfnotify) /* notify application of the error */
			(*_Sfnotify)(f, wanted, (void*)((long)f->file));

		rv = -1;
		break;
	}

done:
	SFOPEN(f,local);
	return rv;
}

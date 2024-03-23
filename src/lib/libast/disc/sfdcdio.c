/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
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
#include	"sfdchdr.h"

/*	Discipline to turn on direct IO capability.
**	This currently only works for XFS on SGI's.
**
**	Written by Kiem-Phong Vo, kpv@research.att.com, 03/18/1998.
*/

#ifndef FDIRECT
#undef F_DIOINFO
#endif

typedef struct _direct_s
{	Sfdisc_t	disc;	/* Sfio discipline	*/
	int		cntl;	/* file control flags	*/
#ifdef F_DIOINFO
	struct dioattr	dio;	/* direct IO params	*/
#endif
} Direct_t;

/* convert a pointer to an int */
#define P2I(p)	(Sfulong_t)((char*)(p) - (char*)0)

#ifdef F_DIOINFO

static ssize_t diordwr(Sfio_t* f, void* buf, size_t n, Direct_t* di, int type)
{
	size_t	rw, done;
	ssize_t	rv;

	done = 0;	/* amount processed by direct IO */
	rv = 0;

	if((P2I(buf)%di->dio.d_mem) == 0 &&
	   (f->here%di->dio.d_miniosz) == 0 && n >= di->dio.d_miniosz )
	{	/* direct IO ok, make sure we're in the right mode */
		if(!(di->cntl & FDIRECT) )
		{	di->cntl |= FDIRECT;
			(void)fcntl(f->file, F_SETFL, di->cntl);
		}

		for(rw = (n/di->dio.d_miniosz)*di->dio.d_miniosz;; )
		{	size_t	io;

			if((io = rw) > di->dio.d_maxiosz )
				io = di->dio.d_maxiosz;
			if(type == SFIO_READ)
				rv = read(f->file,buf,io);
			else	rv = write(f->file,buf,io);

			if(rv > 0)
			{	rw -= rv; done += rv;
				buf = ((char*)buf + rv);
			}

			if(rv < io || rw < di->dio.d_miniosz)
				break;
		}
	}

	if(done < n && (di->cntl & FDIRECT) )
	{	/* turn off direct IO for remaining IO operation */
		di->cntl &= ~FDIRECT;
		(void)fcntl(f->file, F_SETFL, di->cntl);
	}

	if((rw = n-done) > 0 &&
	   (rv = type == SFIO_READ ? read(f->file,buf,rw) : write(f->file,buf,rw)) > 0 )
		done += rv;

	return done ? done : rv;
}

static ssize_t dioread(Sfio_t* f, void* buf, size_t n, Sfdisc_t* disc)
{
	return diordwr(f, buf, n, (Direct_t*)disc, SFIO_READ);
}

static ssize_t diowrite(Sfio_t* f, const void* buf, size_t n, Sfdisc_t* disc)
{
	return diordwr(f, buf, n, (Direct_t*)disc, SFIO_WRITE);
}

static int dioexcept(Sfio_t* f, int type, void* data, Sfdisc_t* disc)
{
	Direct_t*	di = (Direct_t*)disc;

	if(type == SFIO_FINAL || type == SFIO_DPOP)
	{
		if(di->cntl&FDIRECT)
		{	di->cntl &= ~FDIRECT;
			(void)fcntl(f->file,F_SETFL,di->cntl);
		}
		free(disc);
	}

	return 0;
}

#endif /* F_DIOINFO */

int sfdcdio(Sfio_t* f, size_t bufsize)
{
#ifndef F_DIOINFO
	return -1;
#else
	int		cntl;
	struct dioattr	dio;
	void*		buf;
	Direct_t*	di;

	if(f->extent < 0 || (f->flags&SFIO_STRING))
		return -1;

	if((cntl = fcntl(f->file,F_GETFL,0)) < 0)
		return -1;

	if(!(cntl&FDIRECT) )
	{	cntl |= FDIRECT;
		if(fcntl(f->file,F_SETFL,cntl) < 0)
			return -1;
	}

	if(fcntl(f->file,F_DIOINFO,&dio) < 0)
		goto no_direct;

	if(bufsize > 0)
		bufsize = (bufsize/dio.d_miniosz)*dio.d_miniosz;
	if(bufsize <= 0)
		bufsize = dio.d_miniosz*64;
	if(bufsize > dio.d_maxiosz)
		bufsize = dio.d_maxiosz;

	if(!(di = (Direct_t*)malloc(sizeof(Direct_t))) )
		goto no_direct;

	if(!(buf = memalign(dio.d_mem,bufsize)) )
	{	free(di);
		goto no_direct;
	}

	sfsetbuf(f,buf,bufsize);
	if(sfsetbuf(f,buf,0) == buf)
		sfset(f,SFIO_MALLOC,1);
	else
	{	free(buf);
		free(di);
		goto no_direct;
	}

	di->disc.readf = dioread;
	di->disc.writef = diowrite;
	di->disc.seekf = NULL;
	di->disc.exceptf = dioexcept;
	di->cntl = cntl;
	di->dio = dio;

	if(sfdisc(f,(Sfdisc_t*)di) != (Sfdisc_t*)di)
	{	free(di);
	no_direct:
		cntl &= ~FDIRECT;
		(void)fcntl(f->file,F_SETFL,cntl);
		return -1;
	}

	return 0;

#endif /*F_DIOINFO*/
}

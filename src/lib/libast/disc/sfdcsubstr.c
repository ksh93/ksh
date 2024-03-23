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
#include	"sfdchdr.h"


/*	Discipline to treat a contiguous segment of a stream as a stream
**	in its own right. The hard part in all this is to allow multiple
**	segments of the stream to be used as substreams at the same time.
**
**	Written by David G. Korn and Kiem-Phong Vo (03/18/1998)
*/

typedef struct _subfile_s
{
	Sfdisc_t	disc;	/* sfio discipline */
	Sfio_t*		parent;	/* parent stream */
	Sfoff_t		offset;	/* starting offset */
	Sfoff_t		extent;	/* size wanted */
	Sfoff_t		here;	/* current seek location */
} Subfile_t;

static ssize_t streamio(Sfio_t* f, void* buf, size_t n, Sfdisc_t* disc, int type)
{
	Subfile_t	*su;
	Sfoff_t	here, parent;
	ssize_t	io;

	su = (Subfile_t*)disc;

	/* read just what we need */
	if(su->extent >= 0 && (ssize_t)n > (io = (ssize_t)(su->extent - su->here)) )
		n = io;
	if(n <= 0)
		return n;

	/* save current location in parent stream */
	parent = sfsk(f,0,SEEK_CUR,disc);

	/* read data */
	here = su->here + su->offset;
	if(sfsk(f,here,SEEK_SET,disc) != here)
		io = 0;
	else
	{	if(type == SFIO_WRITE) 
			io = sfwr(f,buf,n,disc);
		else	io = sfrd(f,buf,n,disc);
		if(io > 0)
			su->here += io;
	}

	/* restore parent current position */
	sfsk(f,parent,SEEK_SET,disc);

	return io;
}

static ssize_t streamwrite(Sfio_t* f, const void* buf, size_t n, Sfdisc_t* disc)
{
	return streamio(f,(void*)buf,n,disc,SFIO_WRITE);
}

static ssize_t streamread(Sfio_t* f, void* buf, size_t n, Sfdisc_t* disc)
{
	return streamio(f,buf,n,disc,SFIO_READ);
}

static Sfoff_t streamseek(Sfio_t* f, Sfoff_t pos, int type, Sfdisc_t* disc)
{
	Subfile_t*	su;
	Sfoff_t	here, parent;

	su = (Subfile_t*)disc;

	switch(type)
	{
	case SEEK_SET:
		here = 0;
		break;
	case SEEK_CUR:
		here = su->here;
		break;
	case SEEK_END:
		if(su->extent >= 0)
			here = su->extent;
		else
		{	parent = sfsk(f,0,SEEK_CUR,disc);
			if((here = sfsk(f,0,SEEK_END,disc)) < 0)
				return -1;
			else	here -= su->offset;
			sfsk(f,parent,SEEK_SET,disc);
		}
		break;
	default:
		return -1;
	}

	pos += here;
	if(pos < 0 || (su->extent >= 0 && pos >= su->extent))
		return -1;

	return su->here = pos;
}

static int streamexcept(Sfio_t* f, int type, void* data, Sfdisc_t* disc)
{
	if(type == SFIO_FINAL || type == SFIO_DPOP)
		free(disc);
	return 0;
}

Sfio_t* sfdcsubstream(Sfio_t*	f,	/* stream */
		      Sfio_t*	parent,	/* parent stream */
		      Sfoff_t	offset,	/* offset in f */
		      Sfoff_t	extent)	/* desired size */
{
	Sfio_t*	sp;
	Subfile_t*	su;
	Sfoff_t	here;

	/* establish that we can seek to offset */
	if((here = sfseek(parent,0,SEEK_CUR)) < 0 || sfseek(parent,offset,SEEK_SET) < 0)
		return NULL;
	else	sfseek(parent,here,SEEK_SET);
	sfpurge(parent);

	if (!(sp = f) && !(sp = sfnew(NULL, NULL, (size_t)SFIO_UNBOUND, dup(sffileno(parent)), parent->flags)))
		return NULL;

	if(!(su = (Subfile_t*)malloc(sizeof(Subfile_t))))
	{	if(sp != f)
			sfclose(sp);
		return NULL;
	}
	memset(su, 0, sizeof(*su));

	su->disc.readf = streamread;
	su->disc.writef = streamwrite;
	su->disc.seekf = streamseek;
	su->disc.exceptf = streamexcept;
	su->parent = parent;
	su->offset = offset;
	su->extent = extent;

	if(sfdisc(sp, (Sfdisc_t*)su) != (Sfdisc_t*)su)
	{	free(su);
		if(sp != f)
			sfclose(sp);
		return NULL;
	}

	return sp;
}

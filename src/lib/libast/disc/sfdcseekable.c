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

/*	Discipline to make an unseekable read stream seekable
**
**	sfraise(f,SFSK_DISCARD,0) discards previous seek data
**	but seeks from current offset on still allowed
**
**	Written by Kiem-Phong Vo, kpv@research.att.com, 03/18/1998.
*/

typedef struct _skable_s
{	Sfdisc_t	disc;	/* sfio discipline */
	Sfio_t*		shadow;	/* to shadow data */
	Sfoff_t		discard;/* sfseek(f,-1,SEEK_SET) discarded data */
	Sfoff_t		extent; /* shadow extent */
	int		eof;	/* if eof has been reached */
} Seek_t;

static ssize_t skwrite(Sfio_t*		f,	/* stream involved */
		       const void*	buf,	/* buffer to read into */
		       size_t		n,	/* number of bytes to read */
		       Sfdisc_t*	disc)	/* discipline */
{
	return (ssize_t)(-1);
}

static ssize_t skread(Sfio_t*	f,	/* stream involved */
		      void*	buf,	/* buffer to read into */
		      size_t	n,	/* number of bytes to read */
		      Sfdisc_t*	disc)	/* discipline */
{
	Seek_t*		sk;
	Sfio_t*		sf;
	Sfoff_t		addr;
	ssize_t		r, w, p;

	sk = (Seek_t*)disc;
	sf = sk->shadow;
	if(sk->eof)
		return sfread(sf,buf,n);

	addr = sfseek(sf,0,SEEK_CUR);

	if(addr+n <= sk->extent)
		return sfread(sf,buf,n);

	if((r = (ssize_t)(sk->extent-addr)) > 0)
	{	if((w = sfread(sf,buf,r)) != r)
			return w;
		buf = (char*)buf + r;
		n -= r;
	}

	/* do a raw read */
	if((w = sfrd(f,buf,n,disc)) <= 0)
	{	sk->eof = 1;
		w = 0;
	}
	else
	{
		if((p = sfwrite(sf,buf,w)) != w)
			sk->eof = 1;
		if(p > 0)
			sk->extent += p;
	}

	return r+w;
}

static Sfoff_t skseek(Sfio_t* f, Sfoff_t addr, int type, Sfdisc_t* disc)
{
	Seek_t*		sk;
	Sfio_t*		sf;
	char		buf[SFIO_BUFSIZE];
	ssize_t		r, w;

	sk = (Seek_t*)disc;
	sf = sk->shadow;

	switch (type)
	{
	case SEEK_SET:
		addr -= sk->discard;
		break;
	case SEEK_CUR:
		addr += sftell(sf);
		break;
	case SEEK_END:
		addr += sk->extent;
		break;
	default:
		return -1;
	}

	if(addr < 0)
		return (Sfoff_t)(-1);
	else if(addr > sk->extent)
	{	if(sk->eof)
			return (Sfoff_t)(-1);

		/* read enough to reach the seek point */
		while(addr > sk->extent)
		{	if(addr > sk->extent+sizeof(buf) )
				w = sizeof(buf);
			else	w = (int)(addr-sk->extent);
			if((r = sfrd(f,buf,w,disc)) <= 0)
				w = r-1;
			else if((w = sfwrite(sf,buf,r)) > 0)
				sk->extent += w;
			if(w != r)
			{	sk->eof = 1;
				break;
			}
		}

		if(addr > sk->extent)
			return (Sfoff_t)(-1);
	}

	return sfseek(sf,addr,SEEK_SET) + sk->discard;
}

/* on close, remove the discipline */
static int skexcept(Sfio_t* f, int type, void* data, Sfdisc_t* disc)
{
	Seek_t*		sk;

	sk = (Seek_t*)disc;

	switch (type)
	{
	case SFIO_FINAL:
	case SFIO_DPOP:
		sfclose(sk->shadow);
		free(disc);
		break;
	case SFSK_DISCARD:
		sk->eof = 0;
		sk->discard += sk->extent;
		sk->extent = 0;
		sfseek(sk->shadow,0,SEEK_SET);
		break;
	}
	return 0;
}

int sfdcseekable(Sfio_t* f)
{
	Seek_t*	sk;

	/* see if already seekable */
	if(sfseek(f,0,SEEK_CUR) >= 0)
		return 0;

	if(!(sk = (Seek_t*)malloc(sizeof(Seek_t))) )
		return -1;
	memset(sk, 0, sizeof(*sk));

	sk->disc.readf = skread;
	sk->disc.writef = skwrite;
	sk->disc.seekf = skseek;
	sk->disc.exceptf = skexcept;
	sk->shadow = sftmp(SFIO_BUFSIZE);
	sk->discard = 0;
	sk->extent = 0;
	sk->eof = 0;

	if(sfdisc(f, (Sfdisc_t*)sk) != (Sfdisc_t*)sk)
	{	sfclose(sk->shadow);
		free(sk);
		return -1;
	}

	return 0;
}

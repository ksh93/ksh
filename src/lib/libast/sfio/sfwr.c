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

/*	Write data with discipline.
**	In the case of a string stream, this is used mainly to extend
**	the buffer. However, this is done here so that exception handling
**	is done uniformly across all stream types.
**
**	Written by Kiem-Phong Vo.
*/

/* hole preserving writes */
static ssize_t sfoutput(Sfio_t* f, char* buf, size_t n)
{	char	*sp, *wbuf, *endbuf;
	ssize_t	s, w, wr;

	s = w = 0;
	wbuf = buf;
	endbuf = buf+n;
	while(n > 0)
	{	if((ssize_t)n < _Sfpage) /* no hole possible */
		{	buf += n;
			s = n = 0;
		}
		else while((ssize_t)n >= _Sfpage)
		{	/* see if a hole of 0's starts here */
			sp = buf+1;
			if(buf[0] == 0 && buf[_Sfpage-1] == 0)
			{	/* check byte at a time until int-aligned */
				while((uintptr_t)sp % sizeof(int))
				{	if(*sp != 0)
						goto chk_hole;
					sp += 1;
				}

				/* check using int to speed up */
				while(sp < endbuf)
				{	if(*((int*)sp) != 0)
						goto chk_hole;
					sp += sizeof(int);
				}

				/* check the remaining bytes */
				if(sp > endbuf)
				{	sp -= sizeof(int);
					while(sp < endbuf)
					{	if(*sp != 0)
							goto chk_hole;
						sp += 1;
					}
				}
			}

		chk_hole:
			if((s = sp-buf) >= _Sfpage) /* found a hole */
				break;

			/* skip a dirty page */
			n -= _Sfpage;
			buf += _Sfpage;
		}

		/* write out current dirty pages */
		if(buf > wbuf)
		{	if((ssize_t)n < _Sfpage)
			{	buf = endbuf;
				n = s = 0;
			}
			if((wr = write(f->file,wbuf,buf-wbuf)) > 0)
			{	w += wr;
				f->bits &= ~SFIO_HOLE;
			}
			if(wr != (buf-wbuf))
				break;
			wbuf = buf;
		}

		/* seek to a rounded boundary within the hole */
		if(s >= _Sfpage)
		{	s = (s/_Sfpage)*_Sfpage;
			if(SFSK(f,(Sfoff_t)s,SEEK_CUR,NULL) < 0)
				break;
			w += s;
			n -= s;
			wbuf = (buf += s);
			f->bits |= SFIO_HOLE;

			if(n > 0)
			{	/* next page must be dirty */
				s = (ssize_t)n <= _Sfpage ? 1 : _Sfpage;
				buf += s;
				n -= s;
			}
		}
	}

	return w > 0 ? w : -1;
}

ssize_t sfwr(Sfio_t* f, const void* buf, size_t n, Sfdisc_t* disc)
{
	ssize_t		w;
	Sfdisc_t*	dc;
	int		local, oerrno;

	if(!f)
		return (ssize_t)(-1);

	GETLOCAL(f,local);
	if(!local && !(f->bits&SFIO_DCDOWN)) /* an external user's call */
	{	if(f->mode != SFIO_WRITE && _sfmode(f,SFIO_WRITE,0) < 0 )
			return (ssize_t)(-1);
		if(f->next > f->data && SFSYNC(f) < 0 )
			return (ssize_t)(-1);
	}

	for(;;)
	{	/* stream locked by sfsetfd() */
		if(!(f->flags&SFIO_STRING) && f->file < 0)
			return 0;

		/* clear current error states */
		f->flags &= ~(SFIO_EOF|SFIO_ERROR);

		dc = disc;
		if(f->flags&SFIO_STRING)	/* just asking to extend buffer */
			w = n + (f->next - f->data);
		else
		{	/* warn that a write is about to happen */
			SFDISC(f,dc,writef);
			if(dc && dc->exceptf && (f->flags&SFIO_IOCHECK) )
			{	int	rv;
				if(local)
					SETLOCAL(f);
				if((rv = _sfexcept(f,SFIO_WRITE,n,dc)) > 0)
					n = rv;
				else if(rv < 0)
				{	f->flags |= SFIO_ERROR;
					return rv;
				}
			}

			if(f->extent >= 0)
			{	/* make sure we are at the right place to write */
				if(f->flags&SFIO_APPENDWR)
				{	if(f->here != f->extent || (f->flags&SFIO_SHARE))
					{	f->here = SFSK(f,0,SEEK_END,dc);
						f->extent = f->here;
					}
				}
				else if((f->flags&SFIO_SHARE) && !(f->flags&SFIO_PUBLIC))
					f->here = SFSK(f,f->here,SEEK_SET,dc);
			}

			oerrno = errno;
			errno = 0;

			if(dc && dc->writef)
			{	SFDCWR(f,buf,n,dc,w);
			}
			else if(SFISNULL(f))
				w = n;
			else if(f->flags&SFIO_WHOLE)
				goto do_write;
			else if((ssize_t)n >= _Sfpage &&
				!(f->flags&(SFIO_SHARE|SFIO_APPENDWR)) &&
				f->here == f->extent && (f->here%_Sfpage) == 0)
			{	if((w = sfoutput(f,(char*)buf,n)) <= 0)
					goto do_write;
			}
			else
			{
			do_write:
				if((w = write(f->file,buf,n)) > 0)
					f->bits &= ~SFIO_HOLE;
			}

			if(errno == 0)
				errno = oerrno;

			if(w > 0)
			{	if(!(f->bits&SFIO_DCDOWN) )
				{	if((f->flags&(SFIO_APPENDWR|SFIO_PUBLIC)) && f->extent >= 0 )
						f->here = SFSK(f,0,SEEK_CUR,dc);
					else	f->here += w;
					if(f->extent >= 0 && f->here > f->extent)
						f->extent = f->here;
				}

				return (ssize_t)w;
			}
		}

		if(local)
			SETLOCAL(f);
		switch(_sfexcept(f,SFIO_WRITE,w,dc))
		{
		case SFIO_ECONT :
			goto do_continue;
		case SFIO_EDONE :
			w = local ? 0 : w;
			return (ssize_t)w;
		case SFIO_EDISC :
			if(!local && !(f->flags&SFIO_STRING))
				goto do_continue;
			/* FALLTHROUGH */
		case SFIO_ESTACK :
			return (ssize_t)(-1);
		}

	do_continue:
		for(dc = f->disc; dc; dc = dc->disc)
			if(dc == disc)
				break;
		disc = dc;
	}
}

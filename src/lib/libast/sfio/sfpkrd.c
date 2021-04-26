/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2021 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 1.0                  *
*                    by AT&T Intellectual Property                     *
*                                                                      *
*                A copy of the License is available at                 *
*          http://www.eclipse.org/org/documents/epl-v10.html           *
*         (with md5 checksum b35adb5213ca9657e911e9befb180842)         *
*                                                                      *
*              Information and Software Systems Research               *
*                            AT&T Research                             *
*                           Florham Park NJ                            *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  David Korn <dgk@research.att.com>                   *
*                   Phong Vo <kpv@research.att.com>                    *
*                                                                      *
***********************************************************************/
#include	"sfhdr.h"

/*
 * The preferred method is POSIX recv(2) with MSG_PEEK, which is detected as 'socket_peek'.
 * On Solaris/Illumos (__sun), _stream_peek and _lib_select are needed, as _socket_peek doesn't work correctly.
 * On at least macOS and Linux, sfpkrd() runs significantly faster if we disable these. However,
 * ed_read() still needs to use select to intercept SIGWINCH, so if the last argument given
 # to sfpkrd is '2' select is always used when available.
 */
#if _socket_peek && !__sun
#undef _stream_peek
#endif

#if __APPLE__ && !_socket_peek
#error The socket_peek feature is required. (Hey Apple, revert your src__lib__libast__features__lib.diff patch; it caused multiple regressions, and the hanging bug it fixed is now fixed correctly. See <https://github.com/ksh93/ksh/issues/118>.)
#endif

/*	Read/Peek a record from an unseekable device
**
**	Written by Kiem-Phong Vo.
*/

#define STREAM_PEEK	001
#define SOCKET_PEEK	002

#if __STD_C
ssize_t sfpkrd(int fd, Void_t* argbuf, size_t n, int rc, long tm, int action)
#else
ssize_t sfpkrd(fd, argbuf, n, rc, tm, action)
int	fd;	/* file descriptor */
Void_t*	argbuf;	/* buffer to read data */
size_t	n;	/* buffer size */
int	rc;	/* record character */
long	tm;	/* time-out */
int	action;	/* >0: peeking, if rc>=0, get action records,
		   <0: no peeking, if rc>=0, get action records,
		   =0: no peeking, if rc>=0, must get a single record
		   =2: same as >0, but always use select(2)
		*/
#endif
{
	reg ssize_t	r;
	reg int		ntry, t;
	reg char	*buf = (char*)argbuf, *endbuf;

	if(rc < 0 && tm < 0 && action <= 0)
		return sysreadf(fd,buf,n);

	t = (action > 0 || rc >= 0) ? (STREAM_PEEK|SOCKET_PEEK) : 0;
#if !_stream_peek
	t &= ~STREAM_PEEK;
#endif
#if !_socket_peek
	t &= ~SOCKET_PEEK;
#endif

	for(ntry = 0; ntry < 2; ++ntry)
	{
		r = -1;
#if _stream_peek
		if((t&STREAM_PEEK) && (ntry == 1 || tm < 0) )
		{	struct strpeek	pbuf;
			pbuf.flags = 0;
			pbuf.ctlbuf.maxlen = -1;
			pbuf.ctlbuf.len = 0;
			pbuf.ctlbuf.buf = NIL(char*);
			pbuf.databuf.maxlen = n;
			pbuf.databuf.buf = buf;
			pbuf.databuf.len = 0;

			if((r = ioctl(fd,I_PEEK,&pbuf)) < 0)
			{	if(errno == EINTR)
					return -1;
				t &= ~STREAM_PEEK;
			}
			else
			{	t &= ~SOCKET_PEEK;
				if(r > 0 && (r = pbuf.databuf.len) <= 0)
				{	if(action <= 0)	/* read past eof */
						r = sysreadf(fd,buf,1);
					return r;
				}
				if(r == 0)
					r = -1;
				else if(r > 0)
					break;
			}
		}
#endif /* _stream_peek */

		if(ntry == 1)
			break;

		/* use select to see if data is present */
		while(tm >= 0 || action > 0 ||
			/* block until there is data before peeking again */
			((t&STREAM_PEEK) && rc >= 0) ||
			/* let select be interrupted instead of recv which autoresumes */
			(t&SOCKET_PEEK) )
		{	r = -2;
#if _lib_select
			if(r == -2
#if !__sun /* select(2) is always used on Solaris or if action == 2 on other OSes */
				&& action == 2
#endif
				)
			{	fd_set		rd;
				struct timeval	tmb, *tmp;
				FD_ZERO(&rd);
				FD_SET(fd,&rd);
				if(tm < 0)
					tmp = NIL(struct timeval*);
				else
				{	tmp = &tmb;
					tmb.tv_sec = tm/SECOND;
					tmb.tv_usec = (tm%SECOND)*SECOND;
				}
				r = select(fd+1,&rd,NIL(fd_set*),NIL(fd_set*),tmp);
				if(r < 0)
				{	if(errno == EINTR)
						return -1;
					else if(errno == EAGAIN)
					{	errno = 0;
						continue;
					}
					else	r = -2;
				}
				else	r = FD_ISSET(fd,&rd) ? 1 : -1;
			}
#endif /*_lib_select*/
			if(r == -2)
			{
			}

			if(r > 0)		/* there is data now */
			{	if(action <= 0 && rc < 0)
					return sysreadf(fd,buf,n);
				else	r = -1;
			}
			else if(tm >= 0)	/* timeout exceeded */
				return -1;
			else	r = -1;
			break;
		}

#if _socket_peek
		if(t&SOCKET_PEEK)
		{
			while((t&SOCKET_PEEK) && (r = recv(fd,(char*)buf,n,MSG_PEEK)) < 0)
			{	if(errno == EINTR)
					return -1;
				else if(errno == EAGAIN)
					errno = 0;
				else	t &= ~SOCKET_PEEK;
			}
			if(r >= 0)
			{	t &= ~STREAM_PEEK;
				if(r > 0)
					break;
				else	/* read past eof */
				{	if(action <= 0)
						r = sysreadf(fd,buf,1);
					return r;
				}
			}
		}
#endif
	}

	if(r < 0)
	{	if(tm >= 0 || action > 0)
			return -1;
		else /* get here means: tm < 0 && action <= 0 && rc >= 0 */
		{	/* number of records read at a time */
			if((action = action ? -action : 1) > (int)n)
				action = n;
			r = 0;
			while((t = sysreadf(fd,buf,action)) > 0)
			{	r += t;
				for(endbuf = buf+t; buf < endbuf;)
					if(*buf++ == rc)
						action -= 1;
				if(action == 0 || (int)(n-r) < action)
					break;
			}
			return r == 0 ? t : r;
		}
	}

	/* successful peek, find the record end */
	if(rc >= 0)
	{	reg char*	sp;	

		t = action == 0 ? 1 : action < 0 ? -action : action;
		for(endbuf = (sp = buf)+r; sp < endbuf; )
			if(*sp++ == rc)
				if((t -= 1) == 0)
					break;
		r = sp - buf;
	}

	/* advance */
	if(action <= 0)
		r = sysreadf(fd,buf,r);

	return r;
}

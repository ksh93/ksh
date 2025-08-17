/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2025 Contributors to ksh 93u+m           *
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

/*	Change the file descriptor
**
**	Written by Kiem-Phong Vo.
*/

static int _sfdup(int fd, int newfd, int cloexec)
{
	int	dupfd;

#if F_dupfd_cloexec == F_DUPFD
	while((dupfd = fcntl(fd,F_DUPFD,newfd)) < 0 && errno == EINTR)
		errno = 0;
	if(cloexec && dupfd > -1)
		fcntl(dupfd,F_SETFD,FD_CLOEXEC);
#else
	while((dupfd = fcntl(fd,cloexec?F_dupfd_cloexec:F_DUPFD,newfd)) < 0 && errno == EINTR)
		errno = 0;
#endif
	return dupfd;
}

static int sfsetfd_internal(Sfio_t* f, int newfd, int cloexec)
{
	int		oldfd;

	if(!f)
		return -1;

	if(f->flags&SFIO_STRING)
		return -1;

	if((f->mode&SFIO_INIT) && f->file < 0)
	{	/* restoring file descriptor after a previous freeze */
		if(newfd < 0)
			return -1;
	}
	else
	{	/* change file descriptor */
		if((f->mode&SFIO_RDWR) != f->mode && _sfmode(f,0,0) < 0)
			return -1;
		SFLOCK(f,0);

		oldfd = f->file;
		if(oldfd >= 0)
		{	if(newfd >= 0)
			{	if((newfd = _sfdup(oldfd,newfd,cloexec)) < 0)
				{	SFOPEN(f,0);
					return -1;
				}
				CLOSE(oldfd);
			}
			else
			{	/* sync stream if necessary */
				if(((f->mode&SFIO_WRITE) && f->next > f->data) ||
				   (f->mode&SFIO_READ) || f->disc == _Sfudisc)
				{	if(SFSYNC(f) < 0)
					{	SFOPEN(f,0);
						return -1;
					}
				}

				if(((f->mode&SFIO_WRITE) && f->next > f->data) ||
				   ((f->mode&SFIO_READ) && f->extent < 0 &&
				    f->next < f->endb) )
				{	SFOPEN(f,0);
					return -1;
				}

#ifdef MAP_TYPE
				if((f->bits&SFIO_MMAP) && f->data)
				{	SFMUNMAP(f,f->data,f->endb-f->data);
					f->data = NULL;
				}
#endif

				/* make stream appears uninitialized */
				f->endb = f->endr = f->endw = f->data;
				f->extent = f->here = 0;
				f->mode = (f->mode&SFIO_RDWR)|SFIO_INIT;
				f->bits &= ~SFIO_NULL;	/* off /dev/null handling */
			}
		}

		SFOPEN(f,0);
	}

	/* notify changes */
	if(_Sfnotify)
		(*_Sfnotify)(f, SFIO_SETFD, (void*)((long)newfd));

	f->file = newfd;

	return newfd;
}

int sfsetfd(Sfio_t* f, int newfd)
{
	return sfsetfd_internal(f, newfd, 0);
}

int sfsetfd_cloexec(Sfio_t* f, int newfd)
{
	return sfsetfd_internal(f, newfd, 1);
}

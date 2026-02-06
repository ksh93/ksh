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
#if defined(__linux__) && _lib_statfs
#  include <sys/statfs.h>
#  ifndef  TMPFS_MAGIC
#   define TMPFS_MAGIC	0x01021994
#  endif
#endif

/*	Create a temporary stream for read/write.
**	The stream is originally created as a memory-resident stream.
**	When this memory is exceeded, a real temp file will be created.
**	The temp file creation sequence is somewhat convoluted so that
**	pool/stack/discipline will work correctly.
**
**	Written by David Korn and Kiem-Phong Vo.
*/

#if _tmp_rmfail

/* File not removable while there is an open file descriptor.
** To ensure that temp files are properly removed, we need:
** 1. A discipline to remove a file when the corresponding stream is closed.
**    Care must be taken to close the file descriptor before removing the
**    file because systems such as NT do not allow file removal while
**    there is an open file handle.
** 2. An atexit() function is set up to close temp files when process exits.
** 3. On systems with O_TEMPORARY (e.g., NT), this is used to further ensure
**    that temp files will be removed after the last handle is closed.
*/

typedef struct _file_s		File_t;
struct _file_s
{	File_t*	next;		/* link list		*/
	Sfio_t*	f;		/* associated stream	*/
	char	name[1];	/* temp file name	*/
};

static File_t*	File;		/* list pf temp files	*/

static int _tmprmfile(Sfio_t* f, int type, void* val, Sfdisc_t* disc)
{
	File_t	*ff, *last;

	NOT_USED(val);

	if(type == SFIO_DPOP)	/* don't allow this to pop */
		return -1;

	if(type == SFIO_CLOSING)
	{
		for(last = NULL, ff = File; ff; last = ff, ff = ff->next)
			if(ff->f == f)
				break;
		if(ff)
		{	if(!last)
				File = ff->next;
			else	last->next = ff->next;

			if(_Sfnotify)
				(*_Sfnotify)(f,SFIO_CLOSING,f->file);
			ast_close(f->file);
			f->file = -1;
			while(remove(ff->name) < 0 && errno == EINTR)
				errno = 0;

			free(ff);
		}
	}

	return 0;
}

static void _rmfiles(void)
{	File_t	*ff, *next;

	for(ff = File; ff; ff = next)
	{	next = ff->next;
		_tmprmfile(ff->f, SFIO_CLOSING, NULL, ff->f->disc);
	}
}

static Sfdisc_t	Rmdisc =
	{ NULL, NULL, NULL, _tmprmfile, NULL };

#endif /*_tmp_rmfail*/

static int _rmtmp(char* file)
{
#if _tmp_rmfail	/* remove only when stream is closed */
	File_t*	ff;

	if(!File)
		atexit(_rmfiles);

	if(!(ff = (File_t*)malloc(sizeof(File_t)+strlen(file))) )
		return -1;
	ff->f = f;
	strcpy(ff->name,file);
	ff->next = File;
	File = ff;

#else	/* can remove now */
	while(remove(file) < 0 && errno == EINTR)
		errno = 0;
#endif

	return 0;
}

static int _tmpfd(void)
{
	char*	file;
	int	fd;

#if defined(__linux__) && _lib_statfs
	/*
	 * Use the area of POSIX shared memory objects for the new temporary file descriptor
	 * that is do not access the HDD or SSD but only the memory based tmpfs of the POSIX SHM
	 */
	static int doshm;
	static char *shm = "/dev/shm";
	if (!doshm)
	{
		struct statfs fs;
		if (statfs(shm, &fs) < 0 || fs.f_type != TMPFS_MAGIC || eaccess(shm, W_OK|X_OK))
			shm = NULL;
		doshm++;
	}
	if(!(file = pathtemp(NULL,PATH_MAX,shm,"sf",&fd)))
#else
	if(!(file = pathtemp(NULL,PATH_MAX,NULL,"sf",&fd)))
#endif
		return -1;
	_rmtmp(file);
	free(file);
	return fd;
}

static int _tmpexcept(Sfio_t* f, int type, void* val, Sfdisc_t* disc)
{
	int		fd, m;
	Sfio_t*		sf;
	Sfio_t		newf, savf;
	Sfnotify_f	notify = _Sfnotify;

	NOT_USED(val);
	NOT_USED(disc);

	/* the discipline needs to change only under the following exceptions */
	if(type != SFIO_WRITE && type != SFIO_SEEK &&
	   type != SFIO_DPUSH && type != SFIO_DPOP && type != SFIO_DBUFFER)
		return 0;

	/* try to create the temp file */
	SFCLEAR(&newf);
	newf.flags = SFIO_STATIC;
	newf.mode = SFIO_AVAIL;

	if((fd = _tmpfd()) < 0 )
		return -1;

	/* make sure that the notify function won't be called here since
	   we are only interested in creating the file, not the stream */
	_Sfnotify = 0;
	sf = sfnew(&newf,NULL,(size_t)SFIO_UNBOUND,fd,SFIO_READ|SFIO_WRITE);
	_Sfnotify = notify;
	if(!sf)
		return -1;

	/* make sure that new stream has the same mode */
	if((m = f->flags&(SFIO_READ|SFIO_WRITE)) != (SFIO_READ|SFIO_WRITE))
		sfset(sf, ((~m)&(SFIO_READ|SFIO_WRITE)), 0);
	sfset(sf, (f->mode&(SFIO_READ|SFIO_WRITE)), 1);

	/* now remake the old stream into the new image */
	memcpy(&savf, f, sizeof(Sfio_t));
	memcpy(f, sf, sizeof(Sfio_t));
	f->push = savf.push;
	f->pool = savf.pool;
	f->rsrv = savf.rsrv;
	f->proc = savf.proc;
	f->stdio = savf.stdio;

	/* remove the SFIO_STATIC bit if it was only set above in making newf */
	if(!(savf.flags&SFIO_STATIC) )
		f->flags &= ~SFIO_STATIC;

	if(savf.data)
	{	SFSTRSIZE(&savf);
		if(!(savf.flags&SFIO_MALLOC) )
			(void)sfsetbuf(f,savf.data,savf.size);
		if(savf.extent > 0)
			(void)sfwrite(f,savf.data,(size_t)savf.extent);
		(void)sfseek(f,(Sfoff_t)(savf.next - savf.data),SEEK_SET);
		if((savf.flags&SFIO_MALLOC) )
			free(savf.data);
	}

	/* announce change of status */
	f->disc = NULL;
	if(_Sfnotify)
		(*_Sfnotify)(f, SFIO_SETFD, (void*)((long)f->file));

	/* erase all traces of newf */
	newf.data = newf.endb = newf.endr = newf.endw = NULL;
	newf.file = -1;
	_Sfnotify = 0;
	sfclose(&newf);
	_Sfnotify = notify;

	return 1;
}

Sfio_t* sftmp(size_t s)
{
	Sfio_t		*f;
	int		rv;
	Sfnotify_f	notify = _Sfnotify;
	static Sfdisc_t	Tmpdisc =
			{ NULL, NULL, NULL, _tmpexcept,
#if _tmp_rmfail
			  &Rmdisc
#else
			NULL
#endif
			};

	/* start with a memory resident stream */
	_Sfnotify = 0; /* local computation so no notification */
	f = sfnew(NULL,NULL,s,-1,SFIO_STRING|SFIO_READ|SFIO_WRITE);
	_Sfnotify = notify;
	if(!f)
		return NULL;

	if(s != (size_t)SFIO_UNBOUND)	/* set up a discipline for out-of-bound, etc. */
		f->disc = &Tmpdisc;

	if(s == 0) /* make the file now */
	{	_Sfnotify = 0; /* local computation so no notification */
		rv =  _tmpexcept(f,SFIO_DPOP,NULL,f->disc);
		_Sfnotify = notify;
		if(rv < 0)
		{	sfclose(f);
			return NULL;
		}
	}

	if(_Sfnotify)
		(*_Sfnotify)(f, SFIO_NEW, (void*)((long)f->file));

	return f;
}

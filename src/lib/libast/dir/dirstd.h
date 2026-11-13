/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2022 Contributors to ksh 93u+m           *
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
/*
 * AT&T Bell Laboratories
 *
 * <dirent.h> for systems with no opendir()
 */

#ifndef _DIRENT_H
#define _DIRENT_H

typedef struct
{
	int		dd_fd;		/* file descriptor		*/
#ifdef _DIR_PRIVATE_
	_DIR_PRIVATE_
#endif
} DIR;

struct dirent
{
	long		d_fileno;	/* entry serial number		*/
	int		d_reclen;	/* entry length			*/
	int		d_namlen;	/* entry name length		*/
	char		d_name[];	/* entry name			*/
};

#ifdef	rewinddir
#undef	rewinddir
#define rewinddir(p)	seekdir(p,0L)
#endif

extern DIR*		opendir(const char*);
extern void		closedir(DIR*);
extern struct dirent*	readdir(DIR*);
extern void		seekdir(DIR*, long);
extern long		telldir(DIR*);

#endif

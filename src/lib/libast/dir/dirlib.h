/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
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
/*
 * AT&T Research
 *
 * directory stream access library private definitions
 * library routines should include this file rather than <dirent.h>
 */

#ifndef _DIRLIB_H
#define _DIRLIB_H

#undef	getdents
#define getdents	______getdents
#undef	getdirentries
#define getdirentries	______getdirentries

#include <ast.h>

#if _lib_opendir && ( _hdr_dirent || _hdr_ndir || _sys_dir )

#define _dir_ok		1

#include <ls.h>

#ifndef _DIRENT_H
#if _hdr_dirent
#include <dirent.h>
#else
#if _hdr_ndir
#include <ndir.h>
#else
#include <sys/dir.h>
#endif
#ifndef dirent
#define dirent	direct
#endif
#endif
#endif

#define DIRdirent	dirent

#else

#define dirent	DIRdirent

#undef	DIR
#define DIR		______DIR
#undef	closedir
#define closedir	______closedir
#undef	opendir
#define opendir		______opendir
#undef	readdir
#define readdir		______readdir
#undef	seekdir
#define seekdir		______seekdir
#undef	telldir
#define telldir		______telldir

#include <ast_param.h>

#include <ls.h>
#include <limits.h>

#ifndef _DIRENT_H
#if _hdr_dirent
#include <dirent.h>
#else
#if _hdr_direntry
#include <direntry.h>
#else
#include <sys/dir.h>
#endif
#endif
#endif

#undef	dirent
#undef	DIR
#undef	closedir
#undef	opendir
#undef	readdir
#undef	seekdir
#undef	telldir

#define _DIR_PRIVATE_ \
	int		dd_loc;		/* offset in block		*/ \
	int		dd_size;	/* valid data in block		*/ \
	char*		dd_buf;		/* directory block		*/

#undef	_DIRENT_H
#include "dirstd.h"
#ifndef _DIRENT_H
#define _DIRENT_H	1
#endif

#ifndef DIRBLKSIZ
#ifdef	DIRBLK
#define DIRBLKSIZ	DIRBLK
#else
#ifdef	DIRBUF
#define DIRBLKSIZ	DIRBUF
#else
#define DIRBLKSIZ	8192
#endif
#endif
#endif

#endif

#undef	getdents
#undef	getdirentries

#ifndef errno
extern int	errno;
#endif

extern ssize_t		getdents(int, void*, size_t);

#endif

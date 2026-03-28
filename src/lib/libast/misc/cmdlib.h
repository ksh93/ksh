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
/*
 * cmdarg library private definitions
 */

#ifndef _CMDLIB_H
#define _CMDLIB_H	1

#define _CMDARG_PRIVATE_ \
	struct \
	{ \
	size_t		args;		/* total args			*/ \
	size_t		commands;	/* total commands		*/ \
	}		total; \
	Error_f		errorf;		/* optional error callback	*/ \
	Cmdrun_f	runf;		/* exec function		*/ \
	int		argcount;	/* current arg count		*/ \
	int		argmax;		/* max # args			*/ \
	int		echo;		/* just an echo			*/ \
	uint32_t	flags;		/* CMD_* flags			*/ \
	size_t		insertlen;	/* strlen(insert)		*/ \
	ptrdiff_t	offset;		/* post arg offset		*/ \
	Cmddisc_t*	disc;		/* discipline			*/ \
	char**		argv;		/* exec argv			*/ \
	char**		firstarg;	/* first argv file arg		*/ \
	char**		insertarg;	/* argv before insert		*/ \
	char**		postarg;	/* start of post arg list	*/ \
	char**		nextarg;	/* next argv file arg		*/ \
	char*		nextstr;	/* next string ends before here	*/ \
	char*		laststr;	/* last string ends before here	*/ \
	char*		insert;		/* replace with current arg	*/ \
	char		buf[1];		/* argv and arg buffer		*/

#include <cmdarg.h>

#endif

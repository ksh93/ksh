/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2023 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                  David Korn <dgk@research.att.com>                   *
*                  Martijn Dekker <martijn@inlv.org>                   *
*                                                                      *
***********************************************************************/

#include	"shopt.h"
#include	"defs.h"
/*
 *  This installs a hook to allow the processing of events when
 *  the shell is waiting for input and when the shell is
 *  waiting for job completion.
 *  The previous waitevent hook function is returned
 */
void	*sh_waitnotify(int(*newevent)(int,long,int))
{
	int (*old)(int,long,int);
	old = sh.waitevent;
	sh.waitevent = newevent;
	return old;
}

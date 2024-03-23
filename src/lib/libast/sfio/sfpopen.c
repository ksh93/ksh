/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
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

/*	Create a coprocess.
**	Written by Kiem-Phong Vo.
*/

#include	<proc.h>

Sfio_t*	sfpopen(Sfio_t*		f,
		const char*	command,	/* command to execute */
		const char*	mode)		/* mode of the stream */
{
	Proc_t*		proc;
	int		sflags;
	long		flags;
	int		pflags;
	char*		av[4];

	if (!command || !command[0] || !mode)
		return NULL;
	sflags = _sftype(mode, NULL, NULL);

	if(f == (Sfio_t*)(-1))
	{	/* stdio compatibility mode */
		f = NULL;
		pflags = 1;
	}
	else	pflags = 0;

	flags = 0;
	if (sflags & SFIO_READ)
		flags |= PROC_READ;
	if (sflags & SFIO_WRITE)
		flags |= PROC_WRITE;
	av[0] = "sh";
	av[1] = "-c";
	av[2] = (char*)command;
	av[3] = 0;
	if (!(proc = procopen(0, av, 0, 0, flags)))
		return NULL;
	if (!(f = sfnew(f, NULL, (size_t)SFIO_UNBOUND,
	       		(sflags&SFIO_READ) ? proc->rfd : proc->wfd, sflags|((sflags&SFIO_RDWR)?0:SFIO_READ))) ||
	    _sfpopen(f, (sflags&SFIO_READ) ? proc->wfd : -1, proc->pid, pflags) < 0)
	{
		if (f) sfclose(f);
		procclose(proc);
		return NULL;
	}
	procfree(proc);
	return f;
}

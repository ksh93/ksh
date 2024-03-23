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

#ifndef _STDHDR_H
#define _STDHDR_H	1

#include <ast_standards.h>

#define _ast_fseeko	______fseeko
#define _ast_ftello	______ftello
#include "sfhdr.h"
#undef	_ast_fseeko
#undef	_ast_ftello

#include "stdio.h"

#define SFIO_MB		010000
#define SFIO_WC		020000

#define FWIDE(f,r) \
	do \
	{ \
		if (fwide(f, 0) < 0) \
			return r; \
		f->bits |= SFIO_WC; \
	} while (0)

extern int		sfdcwide(Sfio_t*);

#endif

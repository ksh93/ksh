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
 * sfio discipline interface definitions
 */

#ifndef _SFDISC_H
#define _SFDISC_H

#include <ast.h>

#define SFDCEVENT(a,b,n)	((((a)-'A'+1)<<11)^(((b)-'A'+1)<<6)^(n))

#define SFSK_DISCARD		SFDCEVENT('S','K',1)

/*
 * %(...) printf support
 */

typedef int (*Sf_key_lookup_t)(void*, Sffmt_t*, const char*, char**, Sflong_t*);
typedef char* (*Sf_key_convert_t)(void*, Sffmt_t*, const char*, char*, Sflong_t);

extern ssize_t		sfkeyprintf(Sfio_t*, void*, const char*, Sf_key_lookup_t, Sf_key_convert_t);

/*
 * pure sfio read and/or write disciplines
 */

extern int		sfdcdos(Sfio_t*);
extern int		sfdcfilter(Sfio_t*, const char*);
extern int		sfdcmore(Sfio_t*, const char*, int, int);
extern int		sfdcprefix(Sfio_t*, const char*);
extern int		sfdcseekable(Sfio_t*);
extern int		sfdcslow(Sfio_t*);
extern int		sfdctee(Sfio_t*, Sfio_t*);
extern int		sfdcunion(Sfio_t*, Sfio_t**, int);

extern Sfio_t*		sfdcsubstream(Sfio_t*, Sfio_t*, Sfoff_t, Sfoff_t);

#endif

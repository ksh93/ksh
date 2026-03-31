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
 * Glenn Fowler
 * AT&T Research
 *
 * fast find interface definitions
 */

#ifndef _FIND_H
#define _FIND_H

#define FIND_VERSION	19980301L

#ifndef FIND_CODES
#define FIND_CODES	"lib/find/codes"
#endif

#define FIND_CODES_ENV	"FINDCODES"

#define FIND_GENERATE	(1<<0)		/* generate new codes		*/
#define FIND_ICASE	(1<<1)		/* ignore case in match		*/
#define FIND_GNU	(1<<2)		/* generate GNU format codes	*/
#define FIND_OLD	(1<<3)		/* generate old format codes	*/
#define FIND_TYPE	(1<<4)		/* generate type with codes	*/
#define FIND_VERIFY	(1<<5)		/* verify the dir hierarchy	*/

#define FIND_USER	(1L<<16)	/* first user flag bit		*/

struct Find_s;
struct Finddisc_s;

typedef int (*Findverify_f)(struct Find_s*, const char*, size_t, struct Finddisc_s*);

typedef struct Finddisc_s
{
	unsigned long	version;	/* interface version		*/
	unsigned long	flags;		/* FIND_* flags			*/
	Error_f		errorf;		/* error function		*/
	Findverify_f	verifyf;	/* dir verify function		*/
	char**		dirs;		/* dir prefixes to search	*/
} Finddisc_t;

typedef struct Find_s
{
	const char*	id;		/* library ID string		*/
	time_t		stamp;		/* codes time stamp		*/

#ifdef _FIND_PRIVATE_
	_FIND_PRIVATE_
#endif

} Find_t;

extern Find_t*		findopen(const char*, const char*, const char*, Finddisc_t*);
extern char*		findread(Find_t*);
extern int		findwrite(Find_t*, const char*, size_t, const char*);
extern int		findclose(Find_t*);

#endif

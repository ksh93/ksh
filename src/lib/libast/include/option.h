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
 * command line option parse interface
 */

#ifndef _OPTION_H
#define _OPTION_H

#include <ast.h>

#define OPT_VERSION	20070319L

#define OPT_USER	(1L<<16)	/* first user flag bit		*/

struct Opt_s;
struct Optdisc_s;

typedef int (*Optinfo_f)(struct Opt_s*, Sfio_t*, const char*, struct Optdisc_s*);

typedef struct Optdisc_s
{
	unsigned long	version;	/* OPT_VERSION			*/
	unsigned long	flags;		/* OPT_* flags			*/
	char*		catalog;	/* error catalog ID		*/
	Optinfo_f	infof;		/* runtime info function	*/
} Optdisc_t;

#ifndef _OPT_PRIVATE_
#define _OPT_PRIVATE_
#endif

typedef struct Opt_s
{
	int		again;		/* see optjoin()		*/
	char*		arg;		/* {:,#} string argument	*/
	char**		argv;		/* most recent argv		*/
	int		index;		/* argv index			*/
	char*		msg;		/* error/usage message buffer	*/
	long		num;		/* OBSOLETE -- use number	*/
	int		offset;		/* char offset in argv[index]	*/
	char		option[8];	/* current flag {-,+} + option  */
	char		name[64];	/* current long name or flag	*/
	Optdisc_t*	disc;		/* user discipline		*/
	intmax_t	number;		/* # numeric argument		*/
	unsigned char	assignment;	/* option arg assignment op	*/
	char		posix;		/* set for POSIX compliance	*/
	_OPT_PRIVATE_
} Opt_t;

extern Opt_t*		_opt_infop_;

#define opt_info	(*_opt_infop_)

#define optinit(d,f)	(memset(d,0,sizeof(*(d))),(d)->version=OPT_VERSION,(d)->infof=(f),opt_info.disc=(d))

extern int		optget(char**, const char*);
extern int		optjoin(char**, ...);
extern char*		opthelp(const char*, const char*);
extern char*		optusage(const char*);
extern int		optselfdoc(void);
extern int		optstr(const char*, const char*);
extern int		optesc(Sfio_t*, const char*, int);
extern Opt_t*		optctx(Opt_t*, Opt_t*);

#endif

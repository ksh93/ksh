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
 * Glenn Fowler
 * AT&T Research
 *
 * xargs/tw command arg list interface definitions
 */

#ifndef _CMDARG_H
#define _CMDARG_H	1

#include <error.h>

#define CMD_VERSION	20120411L

#define CMD_CHECKED	(1U<<9)		/* cmdopen() argv[0] ok		*/
#define CMD_EMPTY	(1U<<0)		/* run once, even if no args	*/
#define CMD_EXACT	(1U<<1)		/* last command must have argmax*/
#define CMD_EXIT	(1U<<11)	/* fatal error_info.exit()	*/
#define CMD_IGNORE	(1U<<2)		/* ignore EXIT_QUIT exit	*/
#define CMD_INSERT	(1U<<3)		/* argpat for insertion		*/
#define CMD_MINIMUM	(1U<<4)		/* argmax is a minimum		*/
#define CMD_NEWLINE	(1U<<5)		/* echo separator is newline	*/
#define CMD_POST	(1U<<6)		/* argpat is post arg position	*/
#define CMD_QUERY	(1U<<7)		/* trace and query each command	*/
#define CMD_SILENT	(1U<<10)	/* no error messages		*/
#define CMD_TRACE	(1U<<8)		/* trace each command		*/

#define CMD_USER	(1U<<12)

#define CMDDISC(d,f,e)	(memset(d,0,sizeof(*(d))),(d)->version=CMD_VERSION,(d)->flags=(f),(d)->errorf=(e))

struct Cmddisc_s;
typedef struct Cmddisc_s Cmddisc_t;

typedef int (*Cmdrun_f)(int, char**, Cmddisc_t*);

struct Cmddisc_s
{
	uint32_t	version;	/* CMD_VERSION			*/
	uint32_t	flags;		/* CMD_* flags			*/
	Error_f		errorf;		/* optional error function	*/
	Cmdrun_f	runf;		/* optional exec function	*/
};

typedef struct Cmdarg_s			/* cmdopen() handle		*/
{
	const char*	id;		/* library ID string		*/

#ifdef _CMDARG_PRIVATE_
	_CMDARG_PRIVATE_
#endif

} Cmdarg_t;

#ifndef cmdopen
extern Cmdarg_t*	cmdopen(char**, int, ssize_t, const char*, uint32_t);
#endif
extern Cmdarg_t*	cmdopen_20120411(char**, int, ssize_t, const char*, Cmddisc_t*);
extern int		cmdflush(Cmdarg_t*);
extern int		cmdarg(Cmdarg_t*, const char*, int);
extern int		cmdclose(Cmdarg_t*);

#endif

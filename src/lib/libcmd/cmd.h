/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1992-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2025 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  David Korn <dgk@research.att.com>                   *
*                  Martijn Dekker <martijn@inlv.org>                   *
*                                                                      *
***********************************************************************/
/*
 * AT&T Research
 *
 * builtin cmd definitions
 */

#ifndef _CMD_H
#define _CMD_H

#include <ast.h>
#include <error.h>
#include <stk.h>
#include <shcmd.h>

#define cmdinit			_cmd_init

#include <cmdext.h>

#if defined(CMD_BUILTIN) && !defined(CMD_STANDALONE)
#define CMD_STANDALONE	CMD_BUILTIN
#endif

#ifdef CMD_STANDALONE

#define CMD_CONTEXT(c)		(NULL)

#if CMD_DYNAMIC

#include <dlldefs.h>

#else

extern int CMD_STANDALONE(int, char**, Shbltin_t*);

#endif

#ifndef CMD_BUILTIN

/*
 * command initialization
 */

static int
cmdinit(int argc, char** argv, Shbltin_t* context, const char* catalog, int flags)
{
	char*	cp;
	char*	pp;

	NOT_USED(argc);
	if (cp = strrchr(argv[0], '/'))
		cp++;
	else
		cp = argv[0];
	if (pp = strrchr(cp, '_'))
		cp = pp + 1;
	error_info.id = cp;
	if (!error_info.catalog)
		error_info.catalog = (char*)catalog;
	opt_info.index = 0;
	if (context)
		error_info.flags |= flags & ~(ERROR_CALLBACK|ERROR_NOTIFY);
	return 0;
}

#endif

int
main(int argc, char** argv)
{
#if CMD_DYNAMIC
	char*	s;
	char*	t;
	void*		dll;
	Shbltin_f	fun;
	char		buf[64];

	if (s = strrchr(argv[0], '/'))
		s++;
	else if (!(s = argv[0]))
		return 127;
	if ((t = strrchr(s, '_')) && *++t)
		s = t;
	buf[0] = '_';
	buf[1] = 'b';
	buf[2] = '_';
	strncpy(buf + 3, s, sizeof(buf) - 4);
	buf[sizeof(buf) - 1] = 0;
	if (t = strchr(buf, '.'))
		*t = 0;
	for (;;)
	{
		if (dll = dlopen(NULL, RTLD_LAZY))
		{
			if (fun = (Shbltin_f)dlsym(dll, buf + 1))
				break;
			if (fun = (Shbltin_f)dlsym(dll, buf))
				break;
		}
		if (dll = dllplug(NULL, "cmd", NULL, RTLD_LAZY, NULL, 0))
		{
			if (fun = (Shbltin_f)dlsym(dll, buf + 1))
				break;
			if (fun = (Shbltin_f)dlsym(dll, buf))
				break;
		}
		return 127;
	}
	return (*fun)(argc, argv, NULL);
#else
	return CMD_STANDALONE(argc, argv, NULL);
#endif
}

#else

#undef	cmdinit
#ifdef _MSC_VER
#define CMD_CONTEXT(p)		((Shbltin_t*)(p))
#define cmdinit(a,b,c,d,e)	do{if(_cmd_init(a,b,c,d,e))return -1;}while(0)
#else
#define CMD_CONTEXT(p)		(((p)&&((Shbltin_t*)(p))->version>=20071012&&((Shbltin_t*)(p))->version<20350101)?((Shbltin_t*)(p)):0)
#define cmdinit(a,b,c,d,e)	do{if((c)&&!CMD_CONTEXT(c))c=0;if(_cmd_init(a,b,c,d,e))return -1;}while(0)
#endif

extern int	_cmd_init(int, char**, Shbltin_t*, const char*, int);

#endif

#endif

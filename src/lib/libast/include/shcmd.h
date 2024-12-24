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

/*
 * ksh builtin command API
 */

#ifndef _SHCMD_H
#define _SHCMD_H	1

#ifndef AST_PLUGIN_VERSION
#define AST_PLUGIN_VERSION(v)	(v)
#endif
#define SH_PLUGIN_VERSION	AST_PLUGIN_VERSION(20111111L)

#define SHLIB(m)	unsigned long	plugin_version(void) { return SH_PLUGIN_VERSION; }

#ifndef SH_VERSION
#   define Shell_t	void
#endif
#ifndef NV_DEFAULT
#   define Namval_t	void
#endif

#undef Shbltin_t
struct Shbltin_s;
typedef struct Shbltin_s Shbltin_t;

typedef int (*Shbltin_f)(int, char**, Shbltin_t*);

struct Shbltin_s
{
	Shell_t*	shp;
	void*		ptr;
	int		version;
	int		(*shrun)(int, char**);
	int		(*shtrap)(const char*, int);
	void		(*shexit)(int);
	Namval_t*	(*shbltin)(const char*, Shbltin_f, void*);
	unsigned char	notify;
	unsigned char	sigset;
	unsigned char	nosfio;
	Namval_t*	bnode;
	Namval_t*	vnode;
	char*		data;
	int		flags;
	char*		(*shgetenv)(const char*);
	char*		(*shsetenv)(const char*);
	int		invariant;
};

#if defined(shell_h_defined) || defined(defs_h_defined)
#   undef Shell_t
#   undef Namval_t
#else
#   define sh_context(c)	((Shbltin_t*)(c))
#   define sh_run(c, ac, av)	((c)?(*sh_context(c)->shrun)(ac,av):-1)
#   define sh_system(c,str)	((c)?(*sh_context(c)->shtrap)(str,0):system(str))
#   define sh_exit(c,n)		((c)?(*sh_context(c)->shexit)(n):exit(n))
#   define sh_checksig(c)	((c) && sh_context(c)->sigset)
#   define sh_builtin(c,n,f,p)	((c)?(*sh_context(c)->shbltin)(n,(Shbltin_f)(f),sh_context(p)):0)
#   if defined(SFIO_VERSION) || defined(_AST_H)
#	define LIB_INIT(c)
#   else
#	define LIB_INIT(c)	((c) && (sh_context(c)->nosfio = 1))
#   endif
#   ifndef _CMD_H
#     ifndef ERROR_NOTIFY
#       define ERROR_NOTIFY	1
#     endif
#     define cmdinit(ac,av,c,cat,flg)		do { if((ac)<=0) return 0; \
	(sh_context(c)->notify = ((flg)&ERROR_NOTIFY)?1:0);} while(0)
#   endif
#endif

extern int		astintercept(Shbltin_t*, int);

#endif

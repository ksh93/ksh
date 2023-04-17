/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
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
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
#ifndef PATH_OFFSET

/*
 *	UNIX shell path handling interface
 *	Written by David Korn
 *	These are the definitions for the lexical analyzer
 */

#include	<nval.h>
#include	"defs.h"

#if !defined(SHOPT_SPAWN)
#   if _use_spawnveg
#	define  SHOPT_SPAWN  1
#   endif
#endif /* !SHOPT_SPAWN */

#define PATH_PATH		0001
#define PATH_FPATH		0002
#define PATH_CDPATH		0004
#define PATH_BFPATH		0010
#define PATH_SKIP		0020
#define PATH_BUILTIN_LIB	0040
#define PATH_STD_DIR		0100	/* directory is on  $(getconf PATH) */

#define PATH_OFFSET	2		/* path offset for path_join */
#define MAXDEPTH	1024		/* maximum shell function recursion depth */

/*
 * path component structure for path searching
 */
typedef struct pathcomp
{
	struct pathcomp *next;
	int		refcount;
	dev_t		dev;
	ino_t		ino;
	time_t		mtime;
	char		*name;
	char		*lib;
	char		*bbuf;
	char		*blib;
	unsigned short	len;
	unsigned short	flags;
} Pathcomp_t;

#ifndef ARG_RAW
    struct argnod;
#endif /* !ARG_RAW */

/* pathname handling routines */
extern void		path_newdir(Pathcomp_t*);
extern Pathcomp_t	*path_dirfind(Pathcomp_t*,const char*,int);
extern Pathcomp_t	*path_unsetfpath(void);
extern Pathcomp_t	*path_addpath(Pathcomp_t*,const char*,int);
extern Pathcomp_t	*path_dup(Pathcomp_t*);
extern void		path_delete(Pathcomp_t*);
extern void 		path_settrackedalias(const char*,Pathcomp_t*);
extern Namval_t		*path_gettrackedalias(const char*);
extern Pathcomp_t 	*path_absolute(const char*, Pathcomp_t*, int);
extern char 		*path_basename(const char*);
extern char 		*path_fullname(const char*);
extern int 		path_expand(const char*, struct argnod**);
extern noreturn void 	path_exec(const char*,char*[],struct argnod*);
extern pid_t		path_spawn(const char*,char*[],char*[],Pathcomp_t*,int);
extern int		path_open(const char*,Pathcomp_t*);
extern Pathcomp_t 	*path_get(const char*);
extern char 		*path_pwd(void);
extern Pathcomp_t	*path_nextcomp(Pathcomp_t*,const char*,Pathcomp_t*);
extern int		path_search(const char*,Pathcomp_t**,int);
extern char		*path_relative(const char*);
extern int		path_complete(const char*, const char*,struct argnod**);
#if SHOPT_BRACEPAT
    extern int 		path_generate(struct argnod*,struct argnod**);
#endif /* SHOPT_BRACEPAT */

#if SHOPT_DYNAMIC
/* builtin/plugin routines */
extern int		sh_addlib(void*,char*,Pathcomp_t*);
extern Shbltin_f	sh_getlib(char*,Pathcomp_t*);
#endif /* SHOPT_DYNAMIC */

/* constant strings needed for whence */
extern const char e_timeformat[];
extern const char e_badtformat[];
extern const char e_funload[];
extern const char e_pwd[];
extern const char e_logout[];
extern const char e_alphanum[];
extern const char e_mailmsg[];
extern const char e_suidprofile[];
extern const char e_sysprofile[];
extern const char e_traceprompt[];
extern const char is_alias[];
extern const char is_builtin[];
extern const char is_spcbuiltin[];
extern const char is_builtver[];
extern const char is_reserved[];
extern const char is_talias[];
extern const char is_function[];
extern const char is_ufunction[];
extern const char e_autoloadfrom[];

#if SHOPT_ACCT
#   include	"FEATURE/acct"
#   ifdef	_sys_acct
	extern void sh_accinit(void);
	extern void sh_accbegin(const char*);
	extern void sh_accend(void);
	extern void sh_accsusp(void);
#   else
#	undef	SHOPT_ACCT
#   endif	/* _sys_acct */
#endif /* SHOPT_ACCT */

#endif /*! PATH_OFFSET */

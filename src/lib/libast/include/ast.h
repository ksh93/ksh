/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
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
*                   Phong Vo <kpv@research.att.com>                    *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/

/*
 * Advanced Software Technology Library
 * AT&T Research
 *
 * std + POSIX + AST
 */

#ifndef _AST_H
#define _AST_H

#ifndef _AST_STD_H
#include <ast_std.h>
#endif

#ifndef _SFIO_H
#include <sfio.h>
#endif

/*
 * All the ast.* global state variables are actually stored in _ast_info.
 * It's defined/initialized in misc/state.c and the struct is defined in include/ast_std.h.
 */
#define ast		_ast_info

#ifndef PATH_MAX
#define PATH_MAX	1024
#endif

/*
 * work around botched headers that assume <stdio.h>
 */

#ifndef FILE
#define FILE		struct _sfio_s
#ifndef __FILE_typedef
#define __FILE_typedef	1
#endif
#ifndef _FILE_DEFINED
#define _FILE_DEFINED   1
#endif
#ifndef _FILE_defined
#define _FILE_defined   1
#endif
#ifndef _FILEDEFED
#define _FILEDEFED	1
#endif
#ifndef __FILE_defined
#define __FILE_defined  1
#endif
#ifndef ____FILE_defined
#define ____FILE_defined  1
#endif
#endif

/*
 * tcc on FreeBSD: Avoid using nonexistent math
 * builtins by pretending to be an ancient gcc.
 */
#if __TINYC__ && __GNUC__ >= 3 && __FreeBSD__
#undef __GNUC__
#undef __GNUC_MINOR__
#undef __GNUC_PATCHLEVEL__
#define __GNUC__ 2
#define __GNUC_MINOR__ 95
#define __GNUC_PATCHLEVEL__ 3
#endif

/*
 * exit() support -- this matches shell exit codes
 */

#define EXIT_BITS	8			/* # exit status bits	*/

#define EXIT_USAGE	2			/* usage exit code	*/
#define EXIT_QUIT	((1<<(EXIT_BITS))-1)	/* parent should quit	*/
#define EXIT_NOTFOUND	((1<<(EXIT_BITS-1))-1)	/* command not found	*/
#define EXIT_NOEXEC	((1<<(EXIT_BITS-1))-2)	/* other exec error	*/

#define EXIT_CODE(x)	((x)&((1<<EXIT_BITS)-1))
#define EXIT_CORE(x)	(EXIT_CODE(x)|(1<<EXIT_BITS)|(1<<(EXIT_BITS-1)))
#define EXIT_TERM(x)	(EXIT_CODE(x)|(1<<EXIT_BITS))

/*
 * NOTE: for compatibility the following work for EXIT_BITS={7,8}
 */

#define EXIT_STATUS(x)	(((x)&((1<<(EXIT_BITS-2))-1))?(x):EXIT_CODE((x)>>EXIT_BITS))

#define EXITED_CORE(x)	(((x)&((1<<EXIT_BITS)|(1<<(EXIT_BITS-1))))==((1<<EXIT_BITS)|(1<<(EXIT_BITS-1)))||((x)&((1<<(EXIT_BITS-1))|(1<<(EXIT_BITS-2))))==((1<<(EXIT_BITS-1))|(1<<(EXIT_BITS-2))))
#define EXITED_TERM(x)	((x)&((1<<EXIT_BITS)|(1<<(EXIT_BITS-1))))

/*
 * astconflist() flags
 */

#define ASTCONF_parse		0x0001
#define ASTCONF_write		0x0002
#define ASTCONF_read		0x0004
#define ASTCONF_lower		0x0008
#define ASTCONF_base		0x0010
#define ASTCONF_defined		0x0020
#define ASTCONF_quote		0x0040
#define ASTCONF_table		0x0080
#define ASTCONF_matchcall	0x0100
#define ASTCONF_matchname	0x0200
#define ASTCONF_matchstandard	0x0400
#define ASTCONF_error		0x0800
#define ASTCONF_system		0x1000
#define ASTCONF_AST		0x2000

/*
 * pathcanon() flags
 */

#define PATH_PHYSICAL	01
#define PATH_DOTDOT	02
#define PATH_EXISTS	04
#define PATH_VERIFIED(n) (((n)&01777)<<5)

/*
 * pathaccess() flags
 */

#define PATH_READ	004
#define PATH_WRITE	002
#define PATH_EXECUTE	001
#define PATH_REGULAR	010
#define PATH_ABSOLUTE	020

/*
 * touch() flags
 */

#define PATH_TOUCH_CREATE	01
#define PATH_TOUCH_VERBATIM	02

/*
 * pathcheck() info
 */

typedef struct
{
	unsigned long	date;
	char*		feature;
	char*		host;
	char*		user;
} Pathcheck_t;

/*
 * strgrpmatch() flags
 */

#define STR_MAXIMAL	0x01		/* maximal match		*/
#define STR_LEFT	0x02		/* implicit left anchor		*/
#define STR_RIGHT	0x04		/* implicit right anchor	*/
#define STR_ICASE	0x08		/* ignore case			*/
#define STR_GROUP	0x10		/* (|&) inside [@|&](...) only	*/
#define STR_INT		0x20		/* int* match array		*/

/*
 * fmtquote() flags
 */

#define FMT_ALWAYS	0x01		/* always quote			*/
#define FMT_ESCAPED	0x02		/* already escaped		*/
#define FMT_SHELL	0x04		/* escape $ ` too		*/
#define FMT_WIDE	0x08		/* don't escape 8-bit chars	*/
#define FMT_PARAM	0x10		/* disable FMT_SHELL ${$( quote	*/

/*
 * chrexp() flags
 */

#define FMT_EXP_CHAR	0x020		/* expand single byte chars	*/
#define FMT_EXP_LINE	0x040		/* expand \n and \r		*/
#if !AST_NOMULTIBYTE
#define FMT_EXP_WIDE	0x080		/* expand \u \U \x wide chars	*/
#else
#define FMT_EXP_WIDE	0
#endif /* !AST_NOMULTIBYTE */
#define FMT_EXP_NOCR	0x100		/* skip \r			*/
#define FMT_EXP_NONL	0x200		/* skip \n			*/

/*
 * Define inline as an empty macro if we are
 * compiling with C89.
 */
#if __STDC_VERSION__ < 199901L
#define inline
#endif

/*
 * multibyte macros
 */

#if !AST_NOMULTIBYTE

#define mbmax()		( ast.mb.cur_max )
#define mberr()		( ast.mb.tmp_i < 0 )

#define mbcoll()	( ast.mb.xfrm != 0 )
#define mbwide()	( mbmax() > 1 )

#define mb2wc(w,p,n)	( (*ast.mb.towc)(&w, (char*)(p), n) )
#define mbchar(p)	mbnchar(p, mbmax())
#define mbnchar(p,n)	( mbwide() ? ( (ast.mb.tmp_i = (*ast.mb.towc)(&ast.mb.tmp_w, (char*)(p), n)) > 0 ? \
			( (p+=ast.mb.tmp_i),ast.mb.tmp_w) : (p+=ast.mb.sync+1,ast.mb.tmp_i) ) : (*(unsigned char*)(p++)) )
#define mbsize(p)	mbnsize(p, mbmax())
#define mbnsize(p,n)	( mbwide() ? (*ast.mb.len)((char*)(p), n) : ((p), 1) )
#define mbconv(s,w)	( ast.mb.conv ? (*ast.mb.conv)(s,w) : ((*(s)=(w)), 1) )
#define mbwidth(w)	( ast.mb.width ? (*ast.mb.width)(w) : (w >= 0 && w <= 255 && !iscntrl(w) ? 1 : -1) )
#define mbxfrm(t,f,n)	( mbcoll() ? (*ast.mb.xfrm)((char*)(t), (char*)(f), n) : 0 )
#define mbalpha(w)	( ast.mb.alpha ? (*ast.mb.alpha)(w) : isalpha((w) & 0xff) )

#else

#define mbmax()		1
#define mberr()		0

#define mbcoll()	0
#define mbwide()	0

#define mb2wc(w,p,n)	( (w) = *(unsigned char*)(p), 1 )
#define mbchar(p)	( *(unsigned char*)(p++) )
#define mbnchar(p,n)	mbchar(p)
#define mbsize(p)	1
#define mbnsize(p,n)	1
#define mbconv(s,w)	( (*(s)=(w)), 1 )
#define mbwidth(w)	( w >= 0 && w <= 255 && !iscntrl(w) ? 1 : -1 )
#define mbxfrm(t,f,n)	0
#define mbalpha(w)	( isalpha((w) & 0xff) )

#endif /* !AST_NOMULTIBYTE */

#define UTF8_LEN_MAX	6	/* UTF-8 only uses 5 */

/*
 * common macros
 */

#define elementsof(x)	(sizeof(x)/sizeof(x[0]))
#define integralof(x)	((intptr_t)(x))
#define newof(p,t,n,x)	((p)?(t*)realloc((char*)(p),sizeof(t)*(n)+(x)):(t*)calloc(1,sizeof(t)*(n)+(x)))
#define oldof(p,t,n,x)	((p)?(t*)realloc((char*)(p),sizeof(t)*(n)+(x)):(t*)malloc(sizeof(t)*(n)+(x)))
#define pointerof(x)	((void*)((uintptr_t)(x)))
#define roundof(x,y)	(((x)+(y)-1)&~((y)-1))
#define ssizeof(x)	((int)sizeof(x))

#define streq(a,b)	(!strcmp(a,b))
#define strneq(a,b,n)	(!strncmp(a,b,n))
#define strsignal(s)	fmtsignal(s)

#define NiL		NULL			/* for backward compatibility */
#define NoP(x)		do (void)(x); while(0)	/* for silencing "unused parameter" warnings */

#if !defined(NoF)
#define NoF(x)		void _DATA_ ## x (void) {}
#if !defined(_DATA_)
#define _DATA_
#endif
#endif

#if !defined(NoN)
#define NoN(x)		void _STUB_ ## x (void) {}
#if !defined(_STUB_)
#define _STUB_
#endif
#endif

#define NOT_USED(x)	NoP(x)

typedef int (*Error_f)(void*, void*, int, ...);

typedef int (*Ast_confdisc_f)(const char*, const char*, const char*);
typedef int (*Strcmp_context_f)(const char*, const char*, void*);
typedef int (*Strcmp_f)(const char*, const char*);

extern char*		astgetconf(const char*, const char*, const char*, int, Error_f);
extern char*		astconf(const char*, const char*, const char*);
extern Ast_confdisc_f	astconfdisc(Ast_confdisc_f);
extern void		astconflist(Sfio_t*, const char*, int, const char*);
extern int		astquery(int, const char*, ...);
extern void		astwinsize(int, int*, int*);
#if _lib_sysconf
/* prefer sysconf for astconf_long and astconf_ulong to improve performance */
#define CONF_ARG_MAX		_SC_ARG_MAX
#define CONF_CHILD_MAX		_SC_CHILD_MAX
#define CONF_CLK_TCK		_SC_CLK_TCK
#define CONF_NGROUPS_MAX	_SC_NGROUPS_MAX
#define CONF_OPEN_MAX		_SC_OPEN_MAX
#define CONF_PAGESIZE		_SC_PAGESIZE
#define astconf_long(x)		sysconf(x)
#define astconf_ulong(x)	(unsigned long)sysconf(x)
#else
/* fallback in case sysconf isn't available */
#define CONF_ARG_MAX		"ARG_MAX"
#define CONF_CHILD_MAX		"CHILD_MAX"
#define CONF_CLK_TCK		"CLK_TCK"
#define CONF_NGROUPS_MAX	"NGROUPS_MAX"
#define CONF_OPEN_MAX		"OPEN_MAX"
#define CONF_PAGESIZE		"PAGESIZE"
#define astconf_long(x)		strtol(astconf(x,NULL,NULL),NULL,0)
#define astconf_ulong(x)	strtoul(astconf(x,NULL,NULL),NULL,0)
#endif

extern ssize_t		base64encode(const void*, size_t, void**, void*, size_t, void**);
extern ssize_t		base64decode(const void*, size_t, void**, void*, size_t, void**);
extern int		chresc(const char*, char**);
extern int		chrexp(const char*, char**, int*, int);
extern int		chrtoi(const char*);
extern char*		conformance(const char*, size_t);
extern char*		getcodeset(void);
extern char*		fmtbuf(size_t);
extern char*		fmtclock(Sfulong_t);
extern char*		fmtelapsed(unsigned long, int);
extern char*		fmtesc(const char*);
extern char*		fmtesq(const char*, const char*);
extern char*		fmtident(const char*);
extern char*		fmtip4(uint32_t, int);
extern char*		fmtfmt(const char*);
extern char*		fmtgid(int);
extern char*		fmtint(intmax_t, int);
extern char*		fmtmatch(const char*);
extern char*		fmtmode(int, int);
extern char*		fmtnesq(const char*, const char*, size_t);
extern char*		fmtnum(unsigned long, int);
extern char*		fmtperm(int);
extern char*		fmtquote(const char*, const char*, const char*, size_t, int);
extern char*		fmtre(const char*);
extern char*		fmtscale(Sfulong_t, int);
extern char*		fmtsignal(int);
extern char*		fmttime(const char*, time_t);
extern char*		fmtuid(int);
extern void*		memdup(const void*, size_t);
extern unsigned int	memhash(const void*, int);
extern unsigned long	memsum(const void*, int, unsigned long);
extern char*		pathaccess(char*, const char*, const char*, const char*, int);
extern char*		pathaccess_20100601(const char*, const char*, const char*, int, char*, size_t);
extern char*		pathbin(void);
extern char*		pathcanon(char*, int);
extern char*		pathcanon_20100601(char*, size_t, int);
extern char*		pathcat(char*, const char*, int, const char*, const char*);
extern char*		pathcat_20100601(const char*, int, const char*, const char*, char*, size_t);
extern int		pathcd(const char*, const char*);
extern int		pathcheck(const char*, const char*, Pathcheck_t*);
extern int		pathexists(char*, int);
extern char*		pathfind(const char*, const char*, const char*, char*, size_t);
extern int		pathgetlink(const char*, char*, int);
extern int		pathicase(const char*);
extern int		pathinclude(const char*);
extern size_t		pathnative(const char*, char*, size_t);
extern char*		pathpath(char*, const char*, const char*, int);
extern char*		pathpath_20100601(const char*, const char*, int, char*, size_t);
extern size_t		pathposix(const char*, char*, size_t);
extern char*		pathrepl(char*, const char*, const char*);
extern char*		pathrepl_20100601(char*, size_t, const char*, const char*);
extern int		pathsetlink(const char*, const char*);
extern char*		pathtemp(char*, size_t, const char*, const char*, int*);
extern char*		pathtmp(char*, const char*, const char*, int*);
extern char*		setenviron(const char*);
extern pid_t		spawnveg(const char*, char* const[], char* const[], pid_t, int);
extern int		stracmp(const char*, const char*);
extern char*		strcopy(char*, const char*);
extern unsigned long	strelapsed(const char*, char**, int);
extern int		stresc(char*);
extern int		strexp(char*, int);
extern long		streval(const char*, char**, long(*)(const char*, char**));
extern long		strexpr(const char*, char**, long(*)(const char*, char**, void*), void*);
extern int		strgid(const char*);
extern int		strgrpmatch(const char*, const char*, ssize_t*, int, int);
extern int		strngrpmatch(const char*, size_t, const char*, ssize_t*, int, int);
extern unsigned int	strhash(const char*);
extern void*		strlook(const void*, size_t, const char*);
extern int		strmatch(const char*, const char*);
extern int		strmode(const char*);
extern int		strnacmp(const char*, const char*, size_t);
extern char*		strncopy(char*, const char*, size_t);
extern int		strnpcmp(const char*, const char*, size_t);
extern double		strntod(const char*, size_t, char**);
extern _ast_fltmax_t	strntold(const char*, size_t, char**);
extern long		strntol(const char*, size_t, char**, int);
extern intmax_t		strntoll(const char*, size_t, char**, int);
extern long		strnton(const char*, size_t, char**, char*, int);
extern unsigned long	strntoul(const char*, size_t, char**, int);
extern intmax_t		strntonll(const char*, size_t, char**, char*, int);
extern uintmax_t	strntoull(const char*, size_t, char**, int);
extern int		strnvcmp(const char*, const char*, size_t);
extern int		stropt(const char*, const void*, int, int(*)(void*, const void*, int, const char*), void*);
extern int		strpcmp(const char*, const char*);
extern int		strperm(const char*, char**, int);
extern void*		strpsearch(const void*, size_t, size_t, const char*, char**);
extern void*		strsearch(const void*, size_t, size_t, Strcmp_f, const char*, void*);
extern void		strsort(char**, int, int(*)(const char*, const char*));
extern char*		strsubmatch(const char*, const char*, int);
extern unsigned long	strsum(const char*, unsigned long);
extern char*		strtape(const char*, char**);
extern int		strtoip4(const char*, char**, uint32_t*, unsigned char*);
extern long		strton(const char*, char**, char*, int);
extern intmax_t		strtonll(const char*, char**, char*, int);
extern int		struid(const char*);
extern int		struniq(char**, int);
extern int		strvcmp(const char*, const char*);
#if !AST_NOMULTIBYTE
extern size_t		utf32toutf8(char*, uint32_t);
#endif /* !AST_NOMULTIBYTE */

/*
 * backward compat
 */
#define fmterror(e)	strerror(e)

/*
 * C library global data symbols not prototyped by <unistd.h>
 */

#if !defined(environ) && defined(__DYNAMIC__)
#define environ		__DYNAMIC__(environ)
#else
extern char**		environ;
#endif

#include <ast_api.h>

#undef	AST_PLUGIN_VERSION
#define AST_PLUGIN_VERSION(v)	((v)>AST_VERSION?(v):AST_VERSION)

extern unsigned long	plugin_version(void);

#endif

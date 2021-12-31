/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2021 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 1.0                  *
*                    by AT&T Intellectual Property                     *
*                                                                      *
*                A copy of the License is available at                 *
*          http://www.eclipse.org/org/documents/epl-v10.html           *
*         (with md5 checksum b35adb5213ca9657e911e9befb180842)         *
*                                                                      *
*              Information and Software Systems Research               *
*                            AT&T Research                             *
*                           Florham Park NJ                            *
*                                                                      *
*                  David Korn <dgk@research.att.com>                   *
*                                                                      *
***********************************************************************/
/*
 * David Korn
 * AT&T Labs
 *
 * Shell interface private definitions
 *
 */
#ifndef defs_h_defined
#define defs_h_defined

#include	<ast.h>
#if !defined(AST_VERSION) || AST_VERSION < 20111111L
#error libast version 20111111 or later is required
#endif
#if !_lib_fork
#error In 2021, ksh joined the 21st century and started requiring fork(2).
#endif
#if !SHOPT_MULTIBYTE
    /*
     * Disable multibyte without need for excessive '#if SHOPT_MULTIBYTE' preprocessor conditionals.
     * If we redefine the maximum character size mbmax() as 1 byte, the mbwide() macro will always
     * evaluate to 0. All the other multibyte macros have multibtye code conditional upon mbwide(),
     * so the compiler should optimize all of that code away. See src/lib/libast/include/ast.h
     */
#   undef mbmax
#   define mbmax()	1
#endif

#include	<sfio.h>
#include	<error.h>
#include	"FEATURE/externs"
#include	"FEATURE/options"
#include	<cdt.h>
#include	"argnod.h"
#include	"name.h"
#include	<ctype.h>

#ifndef pointerof
#define pointerof(x)		((void*)((char*)0+(x)))
#endif

#define Empty			((char*)(e_sptbnl+3))

#define	env_change()		(++ast.env_serial)
#define sh_envput(e,p)	env_change()
#define env_delete(e,p)	env_change()

extern char*	sh_getenv(const char*);
extern char*	sh_setenviron(const char*);

#ifndef SH_wait_f_defined
    typedef int (*Shwait_f)(int, long, int);
#   define     SH_wait_f_defined
#endif

#include	<shell.h>

#include	"shtable.h"
#include	"regress.h"

/* error exits from various parts of shell */
#define	NIL(type)	((type)0)

#define exitset()	(sh.savexit=sh.exitval)

#ifndef SH_DICT
#define SH_DICT		(void*)e_dict
#endif

#ifndef SH_CMDLIB_DIR
#define SH_CMDLIB_DIR	"/opt/ast/bin"
#endif

#define SH_ID			"ksh"	/* ksh id */
#define SH_STD			"sh"	/* standard sh id */

/* defines for sh_type() */

#define SH_TYPE_SH		001
#define SH_TYPE_KSH		002
#define SH_TYPE_POSIX		004
#define SH_TYPE_LOGIN		010
#define SH_TYPE_PROFILE		020
#define SH_TYPE_RESTRICTED	040

#if SHOPT_HISTEXPAND
#   define SH_HISTAPPEND	60
#   define SH_HISTEXPAND	43
#   define SH_HISTORY2		44
#   define SH_HISTREEDIT	61
#   define SH_HISTVERIFY	62
#endif

#ifndef PIPE_BUF
#   define PIPE_BUF		512
#endif

#if SHOPT_PFSH && ( !_lib_getexecuser || !_lib_free_execattr )
#   undef SHOPT_PFSH
#endif

#define MATCH_MAX		64

#define SH_READEVAL		0x4000	/* for sh_eval */
#define SH_FUNEVAL		0x10000	/* for sh_eval for function load */

extern void		sh_applyopts(Shell_t*,Shopt_t);
extern char 		**sh_argbuild(Shell_t*,int*,const struct comnod*,int);
extern struct dolnod	*sh_argfree(Shell_t *, struct dolnod*,int);
extern struct dolnod	*sh_argnew(Shell_t*,char*[],struct dolnod**);
extern void 		*sh_argopen(Shell_t*);
extern struct argnod	*sh_argprocsub(Shell_t*,struct argnod*);
extern void 		sh_argreset(Shell_t*,struct dolnod*,struct dolnod*);
extern Namval_t		*sh_assignok(Namval_t*,int);
extern struct dolnod	*sh_arguse(Shell_t*);
extern char		*sh_checkid(char*,char*);
extern void		sh_chktrap(Shell_t*);
extern void		sh_deparse(Sfio_t*,const Shnode_t*,int);
extern int		sh_debug(Shell_t *shp,const char*,const char*,const char*,char *const[],int);
extern int 		sh_echolist(Shell_t*,Sfio_t*, int, char**);
extern struct argnod	*sh_endword(Shell_t*,int);
extern char 		**sh_envgen(void);
extern void 		sh_envnolocal(Namval_t*,void*);
extern Sfdouble_t	sh_arith(Shell_t*,const char*);
extern void		*sh_arithcomp(Shell_t *,char*);
extern pid_t 		sh_fork(Shell_t*,int,int*);
extern pid_t		_sh_fork(Shell_t*,pid_t, int ,int*);
extern char 		*sh_mactrim(Shell_t*,char*,int);
extern int 		sh_macexpand(Shell_t*,struct argnod*,struct argnod**,int);
extern int		sh_macfun(Shell_t*,const char*,int);
extern void 		sh_machere(Shell_t*,Sfio_t*, Sfio_t*, char*);
extern void 		*sh_macopen(Shell_t*);
extern char 		*sh_macpat(Shell_t*,struct argnod*,int);
extern Sfdouble_t	sh_mathfun(Shell_t*, void*, int, Sfdouble_t*);
extern int		sh_outtype(Shell_t*, Sfio_t*);
extern char 		*sh_mactry(Shell_t*,char*);
extern int		sh_mathstd(const char*);
extern void		sh_printopts(Shopt_t,int,Shopt_t*);
extern int 		sh_readline(Shell_t*,char**,volatile int,int,ssize_t,long);
extern Sfio_t		*sh_sfeval(char*[]);
extern void		sh_setmatch(Shell_t*,const char*,int,int,int[],int);
extern void             sh_scope(Shell_t*, struct argnod*, int);
extern Namval_t		*sh_scoped(Shell_t*, Namval_t*);
extern Dt_t		*sh_subtracktree(int);
extern Dt_t		*sh_subfuntree(int);
extern void		sh_subjobcheck(pid_t);
extern int		sh_subsavefd(int);
extern void		sh_subtmpfile(Shell_t*);
extern char 		*sh_substitute(const char*,const char*,char*);
extern void		sh_timetraps(Shell_t*);
extern const char	*_sh_translate(const char*);
extern int		sh_trace(Shell_t*,char*[],int);
extern void		sh_trim(char*);
extern int		sh_type(const char*);
extern void             sh_unscope(Shell_t*);
extern void		sh_utol(const char*, char*);
extern int 		sh_whence(char**,int);
#if SHOPT_NAMESPACE
    extern Namval_t	*sh_fsearch(Shell_t*,const char *,int);
#endif /* SHOPT_NAMESPACE */

/* malloc related wrappers */
extern void		*sh_malloc(size_t size);
extern void		*sh_realloc(void *ptr, size_t size);
extern void		*sh_calloc(size_t nmemb, size_t size);
extern char		*sh_strdup(const char *s);
extern void		*sh_memdup(const void *s, size_t n);
extern char		*sh_getcwd(void);
#define new_of(type,x)	((type*)sh_malloc((unsigned)sizeof(type)+(x)))
#define sh_newof(p,t,n,x)	((p)?(t*)sh_realloc((char*)(p),sizeof(t)*(n)+(x)):(t*)sh_calloc(1,sizeof(t)*(n)+(x)))

#define URI_RFC3986_UNRESERVED "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~"

#ifndef ERROR_dictionary
#   define ERROR_dictionary(s)	(s)
#endif
#define sh_translate(s)	_sh_translate(ERROR_dictionary(s))

#define WBITS		(sizeof(long)*8)
#define WMASK		(0xff)

#define is_option(s,x)	((s)->v[((x)&WMASK)/WBITS] & (1L << ((x) % WBITS)))
#define on_option(s,x)	((s)->v[((x)&WMASK)/WBITS] |= (1L << ((x) % WBITS)))
#define off_option(s,x)	((s)->v[((x)&WMASK)/WBITS] &= ~(1L << ((x) % WBITS)))
#define sh_isoption(x)	is_option(&sh.options,x)
#define sh_onoption(x)	on_option(&sh.options,x)
#define sh_offoption(x)	off_option(&sh.options,x)


#define sh_state(x)	( 1<<(x))
#define	sh_isstate(x)	(sh.st.states&sh_state(x))
#define	sh_onstate(x)	(sh.st.states |= sh_state(x))
#define	sh_offstate(x)	(sh.st.states &= ~sh_state(x))
#define	sh_getstate()	(sh.st.states)
#define	sh_setstate(x)	(sh.st.states = (x))

#define sh_sigcheck(shp) do{if((shp)->trapnote&SH_SIGSET)sh_exit(SH_EXITSIG);} while(0)

extern int32_t		sh_mailchk;
extern const char	e_dict[];

/* sh_printopts() mode flags -- set --[no]option by default */

#define PRINT_VERBOSE	0x01	/* option on|off list		*/
#define PRINT_ALL	0x02	/* list unset options too	*/
#define PRINT_NO_HEADER	0x04	/* omit listing header		*/
#define PRINT_TABLE	0x10	/* table of all options		*/

#if SHOPT_STATS
    /* performance statistics */
#   define	STAT_ARGHITS	0
#   define	STAT_ARGEXPAND	1
#   define	STAT_COMSUB	2
#   define	STAT_FORKS	3
#   define	STAT_FUNCT	4
#   define	STAT_GLOBS	5
#   define	STAT_READS	6
#   define	STAT_NVHITS	7
#   define	STAT_NVOPEN	8
#   define	STAT_PATHS	9
#   define	STAT_SVFUNCT	10
#   define	STAT_SCMDS	11
#   define	STAT_SPAWN	12
#   define	STAT_SUBSHELL	13
    extern const Shtable_t shtab_stats[];
#   define sh_stats(x)	(shgd->stats[(x)]++)
#else
#   define sh_stats(x)
#endif /* SHOPT_STATS */

#endif /* !defs_h_defined */

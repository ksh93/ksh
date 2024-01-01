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
*         hyenias <58673227+hyenias@users.noreply.github.com>          *
*                 Harald van Dijk <harald@gigawatt.nl>                 *
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

/* In case multibyte support was disabled for ksh only (SHOPT_MULTIBYTE==0) and not for libast */
#if !SHOPT_MULTIBYTE && !AST_NOMULTIBYTE
#    undef AST_NOMULTIBYTE
#    define AST_NOMULTIBYTE	1
#endif

#include	<ast.h>
#if !defined(AST_VERSION) || AST_VERSION < 20220801
#error libast version 20220801 or later is required
#endif
#if !_lib_fork
#error In 2021, ksh joined the 21st century and started requiring fork(2).
#endif

#include	<sfio.h>
#include	<error.h>
#include	"FEATURE/externs"
#include	<cdt.h>
#include	"argnod.h"
#include	"name.h"
#include	<ctype.h>

#ifndef pointerof
#define pointerof(x)		((void*)((char*)0+(x)))
#endif

#define Empty			((char*)(e_sptbnl+3))
#define AltEmpty		((char*)(e_dot+1))	/* alternative pointer to empty string */

#define	env_change()		(++ast.env_serial)

extern char*	sh_getenv(const char*);
extern char*	sh_setenviron(const char*);

#ifndef SH_wait_f_defined
    typedef int (*Shwait_f)(int, long, int);
#   define     SH_wait_f_defined
#endif

#include	<shell.h>

#include	"shtable.h"
#include	"regress.h"

#define	NIL(type)	NULL		/* for backward compatibility */

#define exitset()	(sh.savexit=sh.exitval)

#ifndef SH_DICT
#define SH_DICT		e_dict
#endif

#ifndef SH_CMDLIB_DIR
#define SH_CMDLIB_DIR	"/opt/ast/bin"
#endif

#define SH_ID			"ksh"	/* ksh ID */
#define SH_STD			"sh"	/* standard sh ID */

/* defines for sh_type() */

#define SH_TYPE_SH		001
#define SH_TYPE_KSH		002
#define SH_TYPE_POSIX		004
#define SH_TYPE_LOGIN		010
#define SH_TYPE_RESTRICTED	040

#ifndef PIPE_BUF
#   define PIPE_BUF		512
#endif

#define MATCH_MAX		64

#define SH_READEVAL		0x4000	/* for sh_eval */
#define SH_FUNEVAL		0x10000	/* for sh_eval for function load */

extern char 		**sh_argbuild(int*,const struct comnod*,int);
extern struct dolnod	*sh_argfree(struct dolnod*,int);
extern struct dolnod	*sh_argnew(char*[],struct dolnod**);
extern void 		*sh_argopen(void);
extern struct argnod	*sh_argprocsub(struct argnod*);
extern void 		sh_argreset(struct dolnod*,struct dolnod*);
extern void		sh_assignok(Namval_t*,int);
extern struct dolnod	*sh_arguse(void);
extern char		*sh_checkid(char*,char*);
extern void		sh_chktrap(void);
extern void		sh_deparse(Sfio_t*,const Shnode_t*,int,int);
extern int		sh_debug(const char*,const char*,const char*,char *const[],int);
extern char 		**sh_envgen(void);
extern void 		sh_envnolocal(Namval_t*,void*);
extern Sfdouble_t	sh_arith(const char*);
extern void		*sh_arithcomp(char*);
extern pid_t 		sh_fork(int,int*);
extern pid_t		_sh_fork(pid_t, int ,int*);
extern void		sh_invalidate_ifs(void);
extern char 		*sh_mactrim(char*,int);
extern int 		sh_macexpand(struct argnod*,struct argnod**,int);
extern int		sh_macfun(const char*,int);
extern void 		sh_machere(Sfio_t*, Sfio_t*, char*);
extern void 		*sh_macopen(void);
extern char 		*sh_macpat(struct argnod*,int);
extern Sfdouble_t	sh_mathfun(void*, int, Sfdouble_t*);
extern int		sh_outtype(Sfio_t*);
extern char 		*sh_mactry(char*);
extern int		sh_mathstd(const char*);
extern void		sh_printopts(Shopt_t,int,Shopt_t*);
extern int 		sh_readline(char**,volatile int,int,ssize_t,long);
extern Sfio_t		*sh_sfeval(char*[]);
extern void		sh_setmatch(const char*,int,int,int[],int);
extern void             sh_scope(struct argnod*, int);
extern Namval_t		*sh_scoped(Namval_t*);
extern Dt_t		*sh_subtracktree(int);
extern Dt_t		*sh_subfuntree(int);
extern void		sh_subjobcheck(pid_t);
extern int		sh_subsavefd(int);
extern void		sh_subtmpfile(void);
extern char 		*sh_substitute(const char*,const char*,char*);
extern void		sh_timetraps(void);
extern const char	*_sh_translate(const char*);
extern int		sh_trace(char*[],int);
extern void		sh_trim(char*);
extern int		sh_type(const char*);
extern void             sh_unscope(void);
#if SHOPT_NAMESPACE
    extern Namval_t	*sh_fsearch(const char *,int);
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

#if SHOPT_SCRIPTONLY
#define is_option(s,x)	((x)==SH_INTERACTIVE || (x)==SH_HISTORY ? 0 : ((s)->v[((x)&WMASK)/WBITS] & (1L << ((x) % WBITS))) )
#define on_option(s,x)	( (x)==SH_INTERACTIVE || (x)==SH_HISTORY ? errormsg(SH_DICT,ERROR_exit(1),e_scriptonly) : ((s)->v[((x)&WMASK)/WBITS] |= (1L << ((x) % WBITS))) )
#define off_option(s,x)	((x)==SH_INTERACTIVE || (x)==SH_HISTORY ? 0 : ((s)->v[((x)&WMASK)/WBITS] &= ~(1L << ((x) % WBITS))) )
#else
#define is_option(s,x)	((s)->v[((x)&WMASK)/WBITS] & (1L << ((x) % WBITS)))
#define on_option(s,x)	((s)->v[((x)&WMASK)/WBITS] |= (1L << ((x) % WBITS)))
#define off_option(s,x)	((s)->v[((x)&WMASK)/WBITS] &= ~(1L << ((x) % WBITS)))
#endif /* SHOPT_SCRIPTONLY */
#define sh_isoption(x)	is_option(&sh.options,x)
#define sh_onoption(x)	on_option(&sh.options,x)
#define sh_offoption(x)	off_option(&sh.options,x)


#define sh_state(x)	( 1<<(x))
#if SHOPT_SCRIPTONLY
#define	sh_isstate(x)	( (x)==SH_INTERACTIVE || (x)==SH_HISTORY ? 0 : (sh.st.states&sh_state(x)) )
#define	sh_onstate(x)	( (x)==SH_INTERACTIVE || (x)==SH_HISTORY ? 0 : (sh.st.states |= sh_state(x)) )
#define	sh_offstate(x)	( (x)==SH_INTERACTIVE || (x)==SH_HISTORY ? 0 : (sh.st.states &= ~sh_state(x)) )
#else
#define	sh_isstate(x)	(sh.st.states&sh_state(x))
#define	sh_onstate(x)	(sh.st.states |= sh_state(x))
#define	sh_offstate(x)	(sh.st.states &= ~sh_state(x))
#endif /* SHOPT_SCRIPTONLY */
#define	sh_getstate()	(sh.st.states)
#define	sh_setstate(x)	(sh.st.states = (x))

#define sh_sigcheck()	do { if(sh.trapnote & SH_SIGSET) sh_exit(SH_EXITSIG); } while(0)

extern int32_t		sh_mailchk;
extern const char	e_dict[];	/* error message catalog */
extern const char	e_sptbnl[];	/* default IFS: " \t\n" */
extern const char	e_dot[];	/* default path & name of dot command: "." */
#if SHOPT_SCRIPTONLY
extern const char	e_scriptonly[];
#endif

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
#   define sh_stats(x)	(sh.stats[(x)]++)
#else
#   define sh_stats(x)
#endif /* SHOPT_STATS */

#endif /* !defs_h_defined */

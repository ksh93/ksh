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
#pragma prototyped
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
#include	<history.h>
#include	"fault.h"
#include	"argnod.h"
#include	"name.h"
#include	<ctype.h>

#ifndef pointerof
#define pointerof(x)		((void*)((char*)0+(x)))
#endif

#define Empty			((char*)(e_sptbnl+3))

#define	env_change()		(++ast.env_serial)
#define Env_t		void
#define sh_envput(e,p)	env_change()
#define env_delete(e,p)	env_change()

extern char*	sh_getenv(const char*);
extern char*	sh_setenviron(const char*);

struct sh_scoped
{
	struct sh_scoped *prevst;	/* pointer to previous state */
	int		dolc;
	char		**dolv;
	char		*cmdname;
	char		*filename;
	char		*funname;
	int		lineno;
	Dt_t		*save_tree;	/* var_tree for calling function */
	struct sh_scoped *self;		/* pointer to copy of this scope */
	Dt_t		*var_local;	/* local level variables for name() */
	struct slnod	*staklist;	/* link list of function stacks */
	int		states;		/* shell state bits used by sh_isstate(), etc. */
	int		breakcnt;
	int		execbrk;
	int		loopcnt;
	int		firstline;
	int32_t		optindex;
	int32_t		optnum;
	int32_t		tmout;		/* value for TMOUT */ 
	short		optchar;
	short		opterror;
	int		ioset;
	unsigned short	trapmax;
	char		*trap[SH_DEBUGTRAP+1];
	char		**otrap;
	char		**trapcom;
	char		**otrapcom;
	void		*timetrap;
	struct Ufunction *real_fun;	/* current 'function name' function */
	int             repl_index;
	char            *repl_arg;
};

struct limits
{
	long		arg_max;	/* max arg+env exec() size */
	int		open_max;	/* maximum number of file descriptors */
	int		clk_tck;	/* number of ticks per second */
	int		child_max;	/* maximum number of children */
	int		ngroups_max;	/* maximum number of process groups */
	unsigned char	posix_version;	/* posix version number */
	unsigned char	posix_jobcontrol;/* non-zero for job control systems */
};

#ifndef SH_wait_f_defined
    typedef int (*Shwait_f)(int, long, int);
#   define     SH_wait_f_defined
#endif


struct shared
{
	struct limits	lim;
	uid_t		userid;
	uid_t		euserid;
	gid_t		groupid;
	gid_t		egroupid;
	pid_t		pid;		/* $$, the main shell's PID (invariable) */
	pid_t		ppid;		/* $PPID, the main shell's parent's PID */
	pid_t		current_pid;	/* ${.sh.pid}, PID of current ksh process (updates when subshell forks) */
	int		realsubshell;	/* ${.sh.subshell}, actual subshell level (including virtual and forked) */
	unsigned char	sigruntime[2];
	Namval_t	*bltin_nodes;
	Namval_t	*bltin_cmds;
	History_t	*hist_ptr;
	char		*shpath;
	char		*user;
	char		**sigmsg;
	char		*rcfile;
	char		**login_files;
	void		*ed_context;
	void		*init_context;
	void		*job_context;
	int		*stats;
	int		bltin_nnodes;	/* number of bltins nodes */ 
	int		sigmax;
	int		nforks;
	Shwait_f	waitevent;
};

#define __SH_PRIVATE_1 \
	struct shared	*gd;		/* global data */ \
	struct sh_scoped st;		/* scoped information */ \
	Stk_t		*stk;		/* stack pointer */ \
	Sfio_t		*heredocs;	/* current here-doc temp file */ \
	Sfio_t		*funlog;	/* for logging function definitions */ \
	int		**fdptrs;	/* pointer to file numbers */ \
	char		*lastarg; \
	char		*lastpath;	/* last absolute path found */ \
	int		path_err;	/* last error on path search */ \
	Dt_t		*track_tree;	/* for tracked aliases */ \
	Dt_t		*var_base;	/* global level variables */ \
	Dt_t		*fun_base;	/* global level functions */ \
	Dt_t		*openmatch; \
	Namval_t	*namespace;	/* current active namespace */ \
	Namval_t	*last_table;	/* last table used in last nv_open */ \
	Namval_t	*prev_table;	/* previous table used in nv_open */ \
	Sfio_t		*outpool;	/* output stream pool */ \
	long		timeout;	/* read timeout */ \
	unsigned int	curenv;		/* current subshell number */ \
	unsigned int	jobenv;		/* subshell number for jobs */ \
	int		infd;		/* input file descriptor */ \
	short		nextprompt;	/* next prompt is PS<nextprompt> */ \
	short		poolfiles; \
	Namval_t	*posix_fun;	/* points to last name() function */ \
	char		*outbuff;	/* pointer to output buffer */ \
	char		*errbuff;	/* pointer to stderr buffer */ \
	char		*prompt;	/* pointer to prompt string */ \
	char		*shname;	/* shell name */ \
	char		*comdiv;	/* points to sh -c argument */ \
	char		*prefix;	/* prefix for compound assignment */ \
	sigjmp_buf	*jmplist;	/* longjmp return stack */ \
	pid_t		bckpid;		/* background process id */ \
	pid_t		cpid; \
	pid_t		spid; 		/* subshell process id */ \
	pid_t		pipepid; \
	pid_t		outpipepid; \
	int		topfd; \
	int		savesig; \
	unsigned char	*sigflag;	/* pointer to signal states */ \
	char		intrap; \
	char		login_sh; \
	char		lastbase; \
	char		forked;	\
	char		binscript; \
	char		deftype; \
	char		funload; \
	char		used_pos;	/* used positional parameter */\
	char		universe; \
	char		winch; \
	char		inarith; 	/* set when in ((...)) */ \
	short		arithrecursion;	/* current arithmetic recursion level */ \
	char		indebug; 	/* set when in debug trap */ \
	unsigned char	ignsig;		/* ignored signal in subshell */ \
	unsigned char	lastsig;	/* last signal received */ \
	char		pathinit;	/* pathinit called from subshell */ \
	char		comsub;		/* set to 1 when in `...`, 2 when in ${ ...; }, 3 when in $(...) */ \
	char		subshare;	/* set when comsub==2 (shared-state ${ ...; } command substitution) */ \
	char		toomany;	/* set when out of fd's */ \
	char		instance;	/* in set_instance */ \
	char		decomma;	/* decimal_point=',' */ \
	char		redir0;		/* redirect of 0 */ \
	char		*readscript;	/* set before reading a script */ \
	int		subdup;		/* bitmask for dups of 1 */ \
	int		*inpipe;	/* input pipe pointer */ \
	int		*outpipe;	/* output pipe pointer */ \
	int		cpipe[3]; \
	int		coutpipe; \
	int		inuse_bits; \
	struct argnod	*envlist; \
	struct dolnod	*arglist; \
	int		fn_depth; \
	int		fn_reset; \
	int		dot_depth; \
	int		hist_depth; \
	int		xargmin; \
	int		xargmax; \
	int		xargexit; \
	int		nenv; \
	mode_t		mask; \
	Env_t		*env; \
	void		*init_context; \
	void		*mac_context; \
	void		*lex_context; \
	void		*arg_context; \
	void		*job_context; \
	void		*pathlist; \
	void		*defpathlist; \
	void		*cdpathlist; \
	char		**argaddr; \
	void		*optlist; \
	struct sh_scoped global; \
	struct checkpt	checkbase; \
	Shinit_f	userinit; \
	Shbltin_f	bltinfun; \
	Shbltin_t	bltindata; \
	char		*cur_line; \
	int		offsets[10]; \
	Sfio_t		**sftable; \
	unsigned char	*fdstatus; \
	const char	*pwd; \
	void		*jmpbuffer; \
	void		*mktype; \
	Sfio_t		*strbuf; \
	Sfio_t		*strbuf2; \
	Dt_t		*first_root; \
	Dt_t		*prefix_root; \
	Dt_t		*last_root; \
	Dt_t		*prev_root; \
	Dt_t		*fpathdict; \
	Dt_t		*typedict; \
	Dt_t		*inpool; \
	Dt_t		*transdict; \
	char		ifstable[256]; \
	unsigned long	test; \
	Shopt_t		offoptions;	/* options that were explicitly disabled by the user on the command line */ \
	Shopt_t		glob_options; \
	Namval_t	*typeinit; \
	Namfun_t	nvfun; \
	char		*mathnodes; \
	char		*bltin_dir; \
	struct Regress_s*regress; \
	char 		exittrap; \
	char 		errtrap; \
	char 		end_fn;
#if !SHOPT_DEVFD
#define __SH_PRIVATE_2 \
	char		*fifo;		/* FIFO name for current process substitution */ \
	Dt_t		*fifo_tree;	/* for cleaning up process substitution FIFOs */
#else
#define	__SH_PRIVATE_2
#endif
#define _SH_PRIVATE	__SH_PRIVATE_1 __SH_PRIVATE_2

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

extern struct shared	*shgd;
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
extern void		sh_subtmpfile(char);
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

#define sh_sigcheck(shp) do{if(shp->trapnote&SH_SIGSET)sh_exit(SH_EXITSIG);} while(0)

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

extern int		_ERROR_exit_b_test(int);

#endif

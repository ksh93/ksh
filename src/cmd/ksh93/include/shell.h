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
#ifndef shell_h_defined
#define shell_h_defined
/*
 * David Korn
 * AT&T Labs
 *
 * Interface definitions for shell command language
 *
 */

#define SH_VERSION	20211229

#include	<ast.h>
#include	<cdt.h>
#include	<history.h>
#include	<stk.h>
#ifdef defs_h_defined
#   include	"name.h"
#else
#   include	<nval.h>
#endif /* defs_h_defined */
#include	"fault.h"

/* options */
typedef struct
{
	unsigned long v[4];
}
Shopt_t;

typedef struct Shell_s Shell_t;

#include	<shcmd.h>

typedef void	(*Shinit_f)(Shell_t*, int);
#ifndef SH_wait_f_defined
    typedef int	(*Shwait_f)(int, long, int);
#   define SH_wait_f_defined
#endif

union Shnode_u;
typedef union Shnode_u Shnode_t;

/*
 * Shell state flags. Used with sh_isstate(), sh_onstate(), sh_offstate().
 * See also shell options below. States 0-5 are also used as shell options.
 */
#define SH_NOFORK	0	/* set when fork not necessary */
#define	SH_FORKED	7	/* set when process has been forked */
#define	SH_PROFILE	8	/* set when processing profiles */
#define SH_NOALIAS	9	/* do not expand non-exported aliases */
#define SH_NOTRACK	10	/* set to disable sftrack() function */
#define SH_STOPOK	11	/* set for stopable builtins */
#define SH_GRACE	12	/* set for timeout grace period */
#define SH_TIMING	13	/* set while timing pipelines */
#define SH_DEFPATH	14	/* set when using default path */
#define SH_INIT		15	/* set when initializing the shell */
#define SH_TTYWAIT	16	/* waiting for keyboard input */
#define	SH_FCOMPLETE	17	/* set for filename completion */
#define	SH_PREINIT	18	/* set with SH_INIT before parsing options */
#define SH_COMPLETE	19	/* set for command completion */
#define SH_XARG		21	/* set while in xarg (command -x) mode */

/*
 * Shell options (set -o). Used with sh_isoption(), sh_onoption(), sh_offoption().
 * There can be a maximum of 256 (0..0xFF) shell options.
 * The short option letters are defined in optksh[] and flagval[] in sh/args.c.
 * The long option names are defined in shtab_options[] in data/options.c.
 */
#define SH_CFLAG	0
#define SH_HISTORY	1	/* used also as a state */
#define	SH_ERREXIT	2	/* used also as a state */
#define	SH_VERBOSE	3	/* used also as a state */
#define SH_MONITOR	4	/* used also as a state */
#define	SH_INTERACTIVE	5	/* used also as a state */
#define	SH_RESTRICTED	6
#define	SH_XTRACE	7
#define	SH_KEYWORD	8
#define SH_NOUNSET	9
#define SH_NOGLOB	10
#define SH_ALLEXPORT	11
#if SHOPT_PFSH
#define SH_PFSH		12
#endif
#define SH_IGNOREEOF	13
#define SH_NOCLOBBER	14
#define SH_MARKDIRS	15
#define SH_BGNICE	16
#if SHOPT_VSH
#define SH_VI		17
#define SH_VIRAW	18
#endif
#define	SH_TFLAG	19
#define SH_TRACKALL	20
#define	SH_SFLAG	21
#define	SH_NOEXEC	22
#if SHOPT_ESH
#define SH_GMACS	24
#define SH_EMACS	25
#endif
#define SH_PRIVILEGED	26
#define SH_NOLOG	28
#define SH_NOTIFY	29
#define SH_DICTIONARY	30
#define SH_PIPEFAIL	32
#define SH_GLOBSTARS	33
#if SHOPT_GLOBCASEDET
#define SH_GLOBCASEDET	34
#endif
#define SH_RC		35
#define SH_SHOWME	36
#define SH_LETOCTAL	37
#if SHOPT_BRACEPAT
#define SH_BRACEEXPAND	42
#endif
#define SH_POSIX	46
#define SH_MULTILINE	47
#define SH_NOBACKSLCTRL	48
#define SH_LOGIN_SHELL	67
#define SH_NOUSRPROFILE	79	/* internal use only */
#define SH_COMMANDLINE	0x100	/* bit flag for invocation-only options ('set -o' cannot change them) */

/*
 * passed as flags to builtins in Nambltin_t struct when BLT_OPTIM is on
 */
#define SH_BEGIN_OPTIM	0x1
#define SH_END_OPTIM	0x2

/* The following type is used for error messages */

/* error messages */
extern const char	e_found[];
#ifdef ENAMETOOLONG
extern const char	e_toolong[];
#endif
extern const char	e_format[];
extern const char 	e_number[];
extern const char	e_restricted[];
extern const char	e_recursive[];
extern char		e_version[];

typedef struct sh_scope
{
	struct sh_scope	*par_scope;
	int		argc;
	char		**argv;
	char		*cmdname;
	char		*filename;
	char		*funname;
	int		lineno;
	Dt_t		*var_tree;
	struct sh_scope	*self;
} Shscope_t;

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
};

/*
 * Saves the state of the shell
 */

struct Shell_s
{
	Shopt_t		options;	/* set -o options */
	Dt_t		*var_tree;	/* for shell variables */
	Dt_t		*fun_tree;	/* for shell functions */
	Dt_t		*alias_tree;	/* for alias names */
	Dt_t		*bltin_tree;    /* for builtin commands */
	Shscope_t	*topscope;	/* pointer to top-level scope */
	int		inlineno;	/* line number of current input file */
	int		exitval;	/* exit status of the command currently being run */
	int		savexit;	/* $? == exit status of the last command executed */
	unsigned char	trapnote;	/* set when trap/signal is pending */
	char		shcomp;		/* set when running shcomp */
	unsigned int	subshell;	/* set for virtual subshell */

	/* These are the former 'struct shared' (shgd) members. */
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
	char		**login_files;
	void		*ed_context;
	int		*stats;
	int		sigmax;
	Shwait_f	waitevent;

	/* These are the members formerly defined via the _SH_PRIVATE macro.
	 * Programs using libshell should not rely on them as they may change. */
	Shell_t		*gd;		/* pointer to self for backwards compatibility (was: global data) */
	struct sh_scoped st;		/* scoped information */
	Stk_t		*stk;		/* stack pointer */
	Sfio_t		*heredocs;	/* current here-doc temp file */
	Sfio_t		*funlog;	/* for logging function definitions */
	int		**fdptrs;	/* pointer to file numbers */
	char		*lastarg;
	char		*lastpath;	/* last absolute path found */
	int		path_err;	/* last error on path search */
	Dt_t		*track_tree;	/* for tracked aliases */
	Dt_t		*var_base;	/* global level variables */
	Dt_t		*fun_base;	/* global level functions */
	Dt_t		*openmatch;
	Namval_t	*namespace;	/* current active namespace */
	Namval_t	*last_table;	/* last table used in last nv_open */
	Namval_t	*prev_table;	/* previous table used in nv_open */
	Sfio_t		*outpool;	/* output stream pool */
	long		timeout;	/* read timeout */
	unsigned int	curenv;		/* current subshell number */
	unsigned int	jobenv;		/* subshell number for jobs */
	int		infd;		/* input file descriptor */
	short		nextprompt;	/* next prompt is PS<nextprompt> */
	Namval_t	*posix_fun;	/* points to last name() function */
	char		*outbuff;	/* pointer to output buffer */
	char		*errbuff;	/* pointer to stderr buffer */
	char		*prompt;	/* pointer to prompt string */
	char		*shname;	/* shell name */
	char		*comdiv;	/* points to sh -c argument */
	char		*prefix;	/* prefix for compound assignment */
	sigjmp_buf	*jmplist;	/* longjmp return stack */
	pid_t		bckpid;		/* background process id */
	pid_t		cpid;
	pid_t		spid; 		/* subshell process id */
	pid_t		pipepid;
	pid_t		outpipepid;
	int		topfd;
	int		savesig;
	unsigned char	*sigflag;	/* pointer to signal states */
	char		intrap;
	char		login_sh;
	char		lastbase;
	char		forked;
	char		binscript;
	char		funload;
	char		used_pos;	/* used positional parameter */
	char		universe;
	char		winch;
	short		arithrecursion;	/* current arithmetic recursion level */
	char		indebug; 	/* set when in debug trap */
	unsigned char	ignsig;		/* ignored signal in subshell */
	unsigned char	lastsig;	/* last signal received */
	char		pathinit;	/* pathinit called from subshell */
	char		comsub;		/* set to 1 when in `...`, 2 when in ${ ...; }, 3 when in $(...) */
	char		subshare;	/* set when comsub==2 (shared-state ${ ...; } command substitution) */
	char		toomany;	/* set when out of fd's */
	char		instance;	/* in set_instance */
	char		decomma;	/* decimal_point=',' */
	char		redir0;		/* redirect of 0 */
	char		*readscript;	/* set before reading a script */
	int		subdup;		/* bitmask for dups of 1 */
	int		*inpipe;	/* input pipe pointer */
	int		*outpipe;	/* output pipe pointer */
	int		cpipe[3];
	int		coutpipe;
	int		inuse_bits;
	struct argnod	*envlist;
	struct dolnod	*arglist;
	int		fn_depth;
	int		fn_reset;
	int		dot_depth;
	int		hist_depth;
	int		xargmin;
	int		xargmax;
	int		xargexit;
	int		nenv;
	mode_t		mask;
	void		*env;		/* environment */
	void		*init_context;
	void		*mac_context;
	void		*lex_context;
	void		*arg_context;
	void		*pathlist;
	void		*defpathlist;
	void		*cdpathlist;
	char		**argaddr;
	void		*optlist;
	struct sh_scoped global;
	struct checkpt	checkbase;
	Shinit_f	userinit;
	Shbltin_f	bltinfun;
	Shbltin_t	bltindata;
	char		*cur_line;
	int		offsets[10];
	Sfio_t		**sftable;
	unsigned char	*fdstatus;
	const char	*pwd;
	void		*jmpbuffer;
	void		*mktype;
	Sfio_t		*strbuf;
	Sfio_t		*strbuf2;
	Dt_t		*first_root;
	Dt_t		*prefix_root;
	Dt_t		*last_root;
	Dt_t		*prev_root;
	Dt_t		*fpathdict;
	Dt_t		*typedict;
	Dt_t		*inpool;
	char		ifstable[256];
	unsigned long	test;
	Shopt_t		offoptions;	/* options that were explicitly disabled by the user on the command line */
	Shopt_t		glob_options;
	Namval_t	*typeinit;
	Namfun_t	nvfun;
	char		*mathnodes;
	char		*bltin_dir;
	struct Regress_s*regress;
	char 		exittrap;
	char 		errtrap;
	char 		end_fn;
#if !SHOPT_DEVFD
	char		*fifo;		/* FIFO name for current process substitution */
	Dt_t		*fifo_tree;	/* for cleaning up process substitution FIFOs */
#endif /* !SHOPT_DEVFD */
};

/* used for builtins */
typedef struct Libcomp_s
{
	void*		dll;
	char*		lib;
	dev_t		dev;
	ino_t		ino;
	unsigned int	attr;
} Libcomp_t;
extern Libcomp_t *liblist;

/* flags for sh_parse */
#define SH_NL		1	/* Treat new-lines as ; */
#define SH_EOF		2	/* EOF causes syntax error */

/* symbolic values for sh_iogetiop */
#define SH_IOCOPROCESS	(-2)
#define SH_IOHISTFILE	(-3)

#include	<cmd.h>

/* symbolic value for sh_fdnotify */
#define SH_FDCLOSE	(-1)

#undef getenv			/* -lshell provides its own */

#if defined(__EXPORT__) && defined(_DLL)
#	define extern __EXPORT__
#endif /* _DLL */

extern Dt_t		*sh_bltin_tree(void);
extern void		sh_subfork(void);
extern Shell_t		*sh_init(int,char*[],Shinit_f);
extern int		sh_reinit(char*[]);
extern int 		sh_eval(Sfio_t*,int);
extern void 		sh_delay(double,int);
extern void		*sh_parse(Shell_t*, Sfio_t*,int);
extern int 		sh_trap(const char*,int);
extern int 		sh_fun(Namval_t*,Namval_t*, char*[]);
extern int 		sh_funscope(int,char*[],int(*)(void*),void*,int);
extern Sfio_t		*sh_iogetiop(int,int);
extern int		sh_main(int, char*[], Shinit_f);
extern int		sh_run(int, char*[]);
extern void		sh_menu(Sfio_t*, int, char*[]);
extern Namval_t		*sh_addbuiltin(const char*, int(*)(int, char*[],Shbltin_t*), void*);
extern char		*sh_fmtq(const char*);
extern char		*sh_fmtqf(const char*, int, int);
extern Sfdouble_t	sh_strnum(const char*, char**, int);
extern int		sh_access(const char*,int);
extern int 		sh_close(int);
extern int		sh_chdir(const char*);
extern int 		sh_dup(int);
extern void 		sh_exit(int);
extern int		sh_fchdir(int);
extern int		sh_fcntl(int, int, ...);
extern Sfio_t		*sh_fd2sfio(int);
extern int		(*sh_fdnotify(int(*)(int,int)))(int,int);
extern int		sh_open(const char*, int, ...);
extern int		sh_openmax(void);
extern Sfio_t		*sh_pathopen(const char*);
extern ssize_t 		sh_read(int, void*, size_t);
extern ssize_t 		sh_write(int, const void*, size_t);
extern off_t		sh_seek(int, off_t, int);
extern int 		sh_pipe(int[]);
extern mode_t 		sh_umask(mode_t);
extern void		*sh_waitnotify(Shwait_f);
extern Shscope_t	*sh_getscope(int,int);
extern Shscope_t	*sh_setscope(Shscope_t*);
extern void		sh_sigcheck(Shell_t*);
extern unsigned long	sh_isoption(int);
extern unsigned long	sh_onoption(int);
extern unsigned long	sh_offoption(int);
extern int 		sh_waitsafe(void);
extern int		sh_exec(const Shnode_t*,int);

/*
 * As of 93u+m, direct access to sh is no longer obsolete, and
 * shgd ("global data") is no longer a separately allocated struct;
 * sh_getinterp() and shgd are provided here for compatibility.
 */
extern Shell_t		sh;
extern Shell_t		*sh_getinterp(void);	/* for libshell ABI compatibility */
#define	sh_getinterp()	(&sh)
#define shgd		(&sh)

#ifdef _DLL
#   undef extern
#endif /* _DLL */

#define chdir(a)	sh_chdir(a)
#define fchdir(a)	sh_fchdir(a)
#ifndef defs_h_defined
#   define access(a,b)	sh_access(a,b)
#   define close(a)	sh_close(a)
#   define exit(a)	sh_exit(a)
#   define fcntl(a,b,c)	sh_fcntl(a,b,c)
#   define pipe(a)	sh_pipe(a)
#   define read(a,b,c)	sh_read(a,b,c)
#   define write(a,b,c)	sh_write(a,b,c)
#   define umask(a)	sh_umask(a)
#   define dup		sh_dup
#   if _lib_lseek64
#	define open64	sh_open
#	define lseek64	sh_seek
#   else
#	define open	sh_open
#	define lseek	sh_seek
#   endif
#endif /* !defs_h_defined */

#define SH_SIGSET	4
#define SH_EXITSIG	0400	/* signal exit bit */
#define SH_EXITMASK	(SH_EXITSIG-1)	/* normal exit status bits */
#define SH_RUNPROG	-1022	/* needs to be negative and < 256 */

#endif /* !shell_h_defined */

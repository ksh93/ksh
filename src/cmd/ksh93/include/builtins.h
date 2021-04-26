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

#ifndef SYSDECLARE

#include	<option.h>
#include	"FEATURE/options"
#include	"FEATURE/dynamic"
#include	"shtable.h"

/*
 * IDs for the parser (parse.c) and parse tree executer (xec.c)
 * to implement special handling for the corresponding builtins.
 * IMPORTANT: The offsets on these macros must be synchronous
 * with the order of shtab_builtins[] in data/builtins.c!
 */
#define SYSEXEC		(shgd->bltin_cmds)	/* exec */
#define SYSREDIR	(shgd->bltin_cmds+1)	/* redirect */
#define SYSSET		(shgd->bltin_cmds+2)	/* set */
						/* : */
#define SYSTRUE		(shgd->bltin_cmds+4)	/* true */
#define SYSCOMMAND	(shgd->bltin_cmds+5)	/* command */
#define SYSCD		(shgd->bltin_cmds+6)	/* cd */
#define SYSBREAK	(shgd->bltin_cmds+7)	/* break */
#define SYSCONT		(shgd->bltin_cmds+8)	/* continue */

#define SYSTYPESET	(shgd->bltin_cmds+9)	/* typeset     \		*/
						/* autoload	|		*/
#define SYSCOMPOUND	(shgd->bltin_cmds+11)	/* compound	|		*/
						/* float	 >typeset range	*/
						/* functions	|		*/
						/* integer	|		*/
#define SYSNAMEREF	(shgd->bltin_cmds+15)	/* nameref      |		*/
#define SYSTYPESET_END	(shgd->bltin_cmds+15)	/*	       /		*/

#define SYSTEST		(shgd->bltin_cmds+16)	/* test */
#define SYSBRACKET	(shgd->bltin_cmds+17)	/* [ */
#define SYSLET		(shgd->bltin_cmds+18)	/* let */
#define SYSEXPORT	(shgd->bltin_cmds+19)	/* export */
#define SYSDOT		(shgd->bltin_cmds+20)	/* . */
#define SYSSOURCE	(shgd->bltin_cmds+21)	/* source */
#define SYSRETURN	(shgd->bltin_cmds+22)	/* return */

/* entry point for shell special builtins */

#if _BLD_shell && defined(__EXPORT__)
#	define extern	__EXPORT__
#endif

extern int b_alias(int, char*[],Shbltin_t*);
extern int b_break(int, char*[],Shbltin_t*);
extern int b_dot_cmd(int, char*[],Shbltin_t*);
extern int b_enum(int, char*[],Shbltin_t*);
extern int b_exec(int, char*[],Shbltin_t*);
extern int b_eval(int, char*[],Shbltin_t*);
extern int b_return(int, char*[],Shbltin_t*);
extern int B_login(int, char*[],Shbltin_t*);
extern int b_true(int, char*[],Shbltin_t*);
extern int b_false(int, char*[],Shbltin_t*);
extern int b_readonly(int, char*[],Shbltin_t*);
extern int b_set(int, char*[],Shbltin_t*);
extern int b_shift(int, char*[],Shbltin_t*);
extern int b_trap(int, char*[],Shbltin_t*);
extern int b_typeset(int, char*[],Shbltin_t*);
extern int b_unset(int, char*[],Shbltin_t*);
extern int b_unalias(int, char*[],Shbltin_t*);

/* The following are for job control */
#if defined(SIGCLD) || defined(SIGCHLD)
    extern int b_jobs(int, char*[],Shbltin_t*);
    extern int b_kill(int, char*[],Shbltin_t*);
#   ifdef SIGTSTP
	extern int b_bg(int, char*[],Shbltin_t*);
#   endif	/* SIGTSTP */
#   ifdef SIGSTOP
	extern int b_suspend(int, char*[],Shbltin_t*);
#   endif	/* SIGSTOP */
#endif

/* The following utilities are built-in because of side-effects */
extern int b_builtin(int, char*[],Shbltin_t*);
extern int b_cd(int, char*[],Shbltin_t*);
extern int b_command(int, char*[],Shbltin_t*);
extern int b_getopts(int, char*[],Shbltin_t*);
extern int b_hist(int, char*[],Shbltin_t*);
extern int b_let(int, char*[],Shbltin_t*);
extern int b_read(int, char*[],Shbltin_t*);
extern int b_ulimit(int, char*[],Shbltin_t*);
extern int b_umask(int, char*[],Shbltin_t*);
#ifdef _cmd_universe
    extern int b_universe(int, char*[],Shbltin_t*);
#endif /* _cmd_universe */
extern int b_wait(int, char*[],Shbltin_t*);
extern int b_whence(int, char*[],Shbltin_t*);

extern int b_alarm(int, char*[],Shbltin_t*);
extern int b_print(int, char*[],Shbltin_t*);
extern int b_printf(int, char*[],Shbltin_t*);
extern int b_pwd(int, char*[],Shbltin_t*);
extern int b_sleep(int, char*[],Shbltin_t*);
extern int b_test(int, char*[],Shbltin_t*);
extern int b_times(int, char*[],Shbltin_t*);
#if !SHOPT_ECHOPRINT
    extern int B_echo(int, char*[],Shbltin_t*);
#endif /* SHOPT_ECHOPRINT */

#undef	extern

extern const char	e_alrm1[];
extern const char	e_alrm2[];
extern const char	e_badfun[];
extern const char	e_baddisc[];
extern const char	e_nofork[];
extern const char	e_nosignal[];
extern const char	e_nolabels[];
extern const char	e_notimp[];
extern const char	e_nosupport[];
extern const char	e_badbase[];
extern const char	e_overlimit[];

extern const char	e_eneedsarg[];
extern const char	e_oneoperand[];
extern const char	e_toomanyops[];
extern const char	e_toodeep[];
extern const char	e_badname[];
extern const char	e_badsyntax[];
#ifdef _cmd_universe
    extern const char	e_nouniverse[];
#endif /* _cmd_universe */
extern const char	e_histopen[];
extern const char	e_condition[];
extern const char	e_badrange[];
extern const char	e_trap[];
extern const char	e_direct[];
extern const char	e_defedit[];
extern const char	e_cneedsarg[];
extern const char	e_defined[];

/* for option parsing */
extern const char sh_set[];
extern const char sh_optalarm[];
extern const char sh_optalias[];
extern const char sh_optbreak[];
extern const char sh_optbuiltin[];
extern const char sh_optcd[];
extern const char sh_optcommand[];
extern const char sh_optcont[];
extern const char sh_optdot[];
#ifndef ECHOPRINT
    extern const char sh_optecho[];
#endif /* !ECHOPRINT */
extern const char sh_opteval[];
extern const char sh_optexec[];
extern const char sh_optredirect[];
extern const char sh_optexit[];
extern const char sh_optexport[];
extern const char sh_optgetopts[];
extern const char sh_optbg[];
extern const char sh_optdisown[];
extern const char sh_optfg[];
extern const char sh_opthash[];
extern const char sh_opthist[];
extern const char sh_optjobs[];
extern const char sh_optkill[];
#if defined(JOBS) && defined(SIGSTOP)
extern const char sh_optstop[];
extern const char sh_optsuspend[];
#endif /* defined(JOBS) && defined(SIGSTOP) */
extern const char sh_optksh[];
extern const char sh_optlet[];
extern const char sh_optprint[];
extern const char sh_optprintf[];
extern const char sh_optpwd[];
extern const char sh_optread[];
extern const char sh_optreadonly[];
extern const char sh_optreturn[];
extern const char sh_optset[];
extern const char sh_optshift[];
extern const char sh_optsleep[];
extern const char sh_opttrap[];
extern const char sh_opttypeset[];
extern const char sh_optulimit[];
extern const char sh_optumask[];
extern const char sh_optunalias[];
extern const char sh_optwait[];
#ifdef _cmd_universe
    extern const char sh_optuniverse[];
#endif /* _cmd_universe */
extern const char sh_optunset[];
extern const char sh_optwhence[];
#endif /* SYSDECLARE */
extern const char sh_opttimes[];

extern const char e_dict[];

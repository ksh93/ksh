/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2025 Contributors to ksh 93u+m           *
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

#ifndef __builtins_h_defined
#define __builtins_h_defined

#include	<option.h>
#include	"shtable.h"

/*
 * IDs for the parser (parse.c) and parse tree executer (xec.c)
 * to implement special handling for the corresponding builtins.
 * IMPORTANT: The offsets on these macros must be synchronous
 * with the order of shtab_builtins[] in data/builtins.c!
 */
#define SYSEXEC		(sh.bltin_cmds)		/* exec */
#define SYSREDIR	(sh.bltin_cmds+1)	/* redirect */
#define SYSSET		(sh.bltin_cmds+2)	/* set */
						/* : */
#define SYSTRUE		(sh.bltin_cmds+4)	/* true */
#define SYSCOMMAND	(sh.bltin_cmds+5)	/* command */
#define SYSCD		(sh.bltin_cmds+6)	/* cd */
#define SYSBREAK	(sh.bltin_cmds+7)	/* break */
#define SYSCONT		(sh.bltin_cmds+8)	/* continue */

#define SYSTYPESET	(sh.bltin_cmds+9)	/* typeset     \		*/
						/* autoload	|		*/
#define SYSCOMPOUND	(sh.bltin_cmds+11)	/* compound	|		*/
						/* float	 >typeset range	*/
						/* functions	|		*/
						/* integer	|		*/
#define SYSNAMEREF	(sh.bltin_cmds+15)	/* nameref      |		*/
#define SYSTYPESET_END	(sh.bltin_cmds+15)	/*	       /		*/

#define SYSTEST		(sh.bltin_cmds+16)	/* test */
#define SYSBRACKET	(sh.bltin_cmds+17)	/* [ */
#define SYSLET		(sh.bltin_cmds+18)	/* let */
#define SYSEXPORT	(sh.bltin_cmds+19)	/* export */
#define SYSDOT		(sh.bltin_cmds+20)	/* . */
#define SYSSOURCE	(sh.bltin_cmds+21)	/* source */
#define SYSRETURN	(sh.bltin_cmds+22)	/* return */
#define SYSENUM		(sh.bltin_cmds+23)	/* enum */

/* entry point for shell special builtins */

extern int b_alias(int, char*[],Shbltin_t*);
extern int b_break(int, char*[],Shbltin_t*);
extern int b_dot_cmd(int, char*[],Shbltin_t*);
extern int b_enum(int, char*[],Shbltin_t*);
extern int b_exec(int, char*[],Shbltin_t*);
extern int b_eval(int, char*[],Shbltin_t*);
extern int b_return(int, char*[],Shbltin_t*);
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
extern int b_jobs(int, char*[],Shbltin_t*);
extern int b_kill(int, char*[],Shbltin_t*);
extern int b_bg(int, char*[],Shbltin_t*);
extern int b_suspend(int, char*[],Shbltin_t*);

/* The following utilities are built-in because of side-effects */
extern int b_builtin(int, char*[],Shbltin_t*);
extern int b_cd(int, char*[],Shbltin_t*);
extern int b_command(int, char*[],Shbltin_t*);
extern int b_getopts(int, char*[],Shbltin_t*);
#if SHOPT_MKSERVICE
extern int b_mkservice(int, char*[],Shbltin_t*);
extern int b_eloop(int, char*[],Shbltin_t*);
#endif /* SHOPT_MKSERVICE */
extern int b_hist(int, char*[],Shbltin_t*);
extern int b_let(int, char*[],Shbltin_t*);
extern int b_read(int, char*[],Shbltin_t*);
extern int b_ulimit(int, char*[],Shbltin_t*);
extern int b_umask(int, char*[],Shbltin_t*);
#if _cmd_universe
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

extern short		b_enum_nelem(Namfun_t*);

extern const char	e_alrm1[];
extern const char	e_alrm2[];
extern const char	e_badfun[];
extern const char	e_baddisc[];
extern const char	e_nofork[];
extern const char	e_nosignal[];
extern const char	e_notimp[];
extern const char	e_nosupport[];
extern const char	e_limit[];
extern const char	e_overlimit[];
extern const char	e_internal[];

extern const char	e_eneedsarg[];
extern const char	e_oneoperand[];
extern const char	e_toomanyops[];
extern const char	e_toodeep[];
extern const char	e_badname[];
extern const char	e_badsyntax[];
#if _cmd_universe
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
#if !SHOPT_ECHOPRINT
    extern const char sh_optecho[];
#endif
extern const char sh_optenum[];
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
#if !SHOPT_SCRIPTONLY
extern const char sh_opthist[];
#endif /* !SHOPT_SCRIPTONLY */
extern const char sh_optjobs[];
extern const char sh_optkill[];
extern const char sh_optstop[];
extern const char sh_optsuspend[];
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
#if _cmd_universe
    extern const char sh_optuniverse[];
#endif /* _cmd_universe */
extern const char sh_optunset[];
extern const char sh_optwhence[];
extern const char sh_opttimes[];

extern const char e_dict[];

#endif /* __builtins_h_defined */

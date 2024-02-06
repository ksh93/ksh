/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2024 Contributors to ksh 93u+m           *
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
*                                                                      *
***********************************************************************/

#include	"shopt.h"
#include	<ast.h>
#include	"FEATURE/dynamic"
#include	<shell.h>
#include	"shtable.h"
#include	"name.h"
#include	"defs.h"
#include	"variables.h"
#include	"builtins.h"

/*
 * This is the list of built-in shell variables and default values
 * and default attributes.
 *
 * The order must be kept synchronous with the defines in include/variables.h.
 */

const struct shtable2 shtab_variables[] =
{
	"PATH",		0,				NULL,
	"PS1",		0,				NULL,
	"PS2",		NV_NOFREE, 			"> ",
	"IFS",		NV_NOFREE, 			" \t\n",
	"PWD",		0,				NULL,
	"HOME",		0,				NULL,
	"MAIL",		0,				NULL,
	"REPLY",	0,				NULL,
	"SHELL",	NV_NOFREE,			"/bin/" SH_STD,
	"EDITOR",	0,				NULL,
	"MAILCHECK",	NV_NOFREE|NV_INTEGER,		NULL,
	"RANDOM",	NV_NOFREE|NV_INTEGER,		NULL,
	"ENV",		NV_NOFREE,			NULL,
	"HISTFILE",	0,				NULL,
	"HISTSIZE",	0,				NULL,
	"HISTEDIT",	NV_NOFREE,			NULL,
	"HISTCMD",	NV_NOFREE|NV_INTEGER,		NULL,
	"FCEDIT",	NV_NOFREE,			&e_defedit[0],
	"CDPATH",	0,				NULL,
	"MAILPATH",	0,				NULL,
	"PS3",		NV_NOFREE, 			"#? ",
	"OLDPWD",	0,				NULL,
	"VISUAL",	0,				NULL,
	"COLUMNS",	0,				NULL,
	"LINES",	0,				NULL,
	"PPID",		NV_NOFREE|NV_PID,		NULL,
	"_",		NV_EXPORT,			NULL,
	"TMOUT",	NV_NOFREE|NV_INTEGER,		NULL,
	"SECONDS",	NV_NOFREE|NV_INTEGER|NV_DOUBLE,	NULL,
	"LINENO",	NV_NOFREE|NV_INTEGER,		NULL,
	"OPTARG",	0,				NULL,
	"OPTIND",	NV_NOFREE|NV_INTEGER,		NULL,
	"PS4",		0,				NULL,
	"FPATH",	0,				NULL,
	"LANG",		0,				NULL,
	"LC_ALL",	0,				NULL,
	"LC_COLLATE",	0,				NULL,
	"LC_CTYPE",	0,				NULL,
	"LC_MESSAGES",	0,				NULL,
	"LC_NUMERIC",	0,				NULL,
	"LC_TIME",	0,				NULL,
	"FIGNORE",	0,				NULL,
	"KSH_VERSION",	0,				NULL,
	"JOBMAX",	NV_NOFREE|NV_INTEGER,		NULL,
	".sh",		NV_TABLE|NV_NOFREE|NV_NOPRINT,	NULL,
	".sh.edchar",	0,				NULL,
	".sh.edcol",	0,				NULL,
	".sh.edtext",	0,				NULL,
	".sh.edmode",	0,				NULL,
	".sh.name",	0,				NULL,
	".sh.subscript",0,				NULL,
	".sh.value",	0,				NULL,
	".sh.version",	NV_NOFREE,			(char*)(&e_version[10]),
	".sh.match",	0,				NULL,
	".sh.command",	0,				NULL,
	".sh.file",	0,				NULL,
	".sh.fun",	0,				NULL,
	".sh.subshell",	NV_INTEGER|NV_NOFREE,		NULL,
	".sh.level",	NV_INT16|NV_NOFREE|NV_RDONLY,	NULL,
	".sh.lineno",	NV_INTEGER|NV_NOFREE,		NULL,
	".sh.stats",	0,				NULL,
	".sh.math",	0,				NULL,
	".sh.pid",	NV_PID|NV_NOFREE,		NULL,
	".sh.ppid",	NV_PID|NV_NOFREE,		NULL,
	".sh.tilde",	0,				NULL,
	"SHLVL",	NV_INTEGER|NV_NOFREE|NV_EXPORT,	NULL,
	"SRANDOM",	NV_NOFREE|NV_INTEGER|NV_UNSIGN,	NULL,
	"",	0,					NULL
};

const char *nv_discnames[] = { "get", "set", "append", "unset", "getn", 0 };

#if SHOPT_STATS
const Shtable_t shtab_stats[] =
{
	"arg_cachehits",	STAT_ARGHITS,
	"arg_expands",		STAT_ARGEXPAND,
	"comsubs",		STAT_COMSUB,
	"forks",		STAT_FORKS,
	"funcalls",		STAT_FUNCT,
	"globs",		STAT_GLOBS,
	"linesread",		STAT_READS,
	"nv_cachehit",		STAT_NVHITS,
	"nv_opens",		STAT_NVOPEN,
	"pathsearch",		STAT_PATHS,
	"posixfuncall",		STAT_SVFUNCT,
	"simplecmds",		STAT_SCMDS,
	"spawns",		STAT_SPAWN,
	"subshell",		STAT_SUBSHELL
};
#endif /* SHOPT_STATS */


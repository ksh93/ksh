/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2026 Contributors to ksh 93u+m           *
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
/*
 * David Korn
 * AT&T Labs
 *
 * shell script to shell binary converter
 *
 */

#include "shopt.h"
#include "version.h"

static const char usage[] =
"[-?\n@(#)$Id: shcomp (AT&T Research) " SH_RELEASE " $\n]"
"[-author?David Korn <dgk@research.att.com>]"
"[-copyright?(c) 1982-2012 AT&T Intellectual Property]"
"[-copyright?" SH_RELEASE_CPYR "]"
"[-license?https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html]"
"[--catalog?" SH_DICT "]"
"[+NAME?shcomp - compile a shell script]"
"[+DESCRIPTION?Unless \b-D\b is specified, \b\f?\f\b takes a shell script, "
	"\ainfile\a, and creates a binary format file, \aoutfile\a, that "
	"\bksh\b can read and execute with the same effect as the original "
	"script.]"
"[+?Since aliases are processed as the script is read, alias definitions "
	"whose value requires variable expansion will not work correctly.]"
"[+?If \b-D\b is specified, all double quoted strings that are preceded by "
	"\b$\b are output. These are the messages that need to be "
	"translated to locale specific versions for internationalization.]"
"[+?If \ainfile\a is a simple command name, the shell script will be searched "
	"on \b$PATH\b. It does not need execute permission to be found.]"
"[+?If \aoutfile\a is omitted, then the results will be written to "
	"standard output. If \ainfile\a is also omitted, the shell script "
	"will be read from standard input. However, \b\f?\f\b will not read "
	"a script from your keyboard unless \ainfile\a is given as "
	"\b/dev/stdin\b, and will refuse to write binary data to a terminal "
	"in any case.]"
"[D:dictionary?Generate a list of strings that need to be placed in a message "
	"catalog for internationalization.]"
"[n:noexec?Displays warning messages for obsolete or non-conforming "
	"constructs.] "
"[v:verbose?Displays input from \ainfile\a onto standard error as it "
	"reads it.]"
"\n"
"\n[infile [outfile]]\n"
"\n"
"[+EXIT STATUS?]{"
	"[+0?Successful completion.]"
	"[+>0?An error occurred.]"
"}"
"[+SEE ALSO?\bksh\b(1)]"
;

#include	<shell.h>
#include	"defs.h"
#include	"path.h"
#include	"shnodes.h"
#include	"sys/stat.h"
#include	"terminal.h"
#include	"builtins.h"

#define CNTL(x)	((x)&037)
static const char header[6] = { CNTL('k'),CNTL('s'),CNTL('h'),0,SHCOMP_HDR_VERSION,0 };

int main(int argc, char *argv[])
{
	Sfio_t *in, *out;
	Namval_t *np;
	Shnode_t *t;
	char *cp, *shcomp_id, *script_id;
	int n, nflag=0, vflag=0, dflag=0;
	shcomp_id = error_info.id = path_basename(argv[0]);
	while(n = optget(argv, usage )) switch(n)
	{
	    case 'D':
		dflag=1;
		break;
	    case 'v':
		vflag=1;
		break;
	    case 'n':
		nflag=1;
		break;
	    case ':':
		errormsg(SH_DICT,2,"%s",opt_info.arg);
		break;
	    case '?':
		/* self-doc: write to standard output */
		error(ERROR_USAGE|ERROR_OUTPUT, STDOUT_FILENO, "%s", opt_info.arg);
		return 0;
	}
	sh_init(argc,argv,NULL);
	script_id = error_info.id;  /* set by sh_init() */
	error_info.id = shcomp_id;
	sh.shcomp = 1;
	argv += opt_info.index;
	argc -= opt_info.index;
	if(argc==0 && tty_check(0))
	{
		errormsg(SH_DICT,ERROR_exit(0),"refusing to read script from terminal");
		error_info.errors++;
	}
	if(error_info.errors || argc>2)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NULL));
		UNREACHABLE();
	}
	if(cp= *argv)
	{
		argv++;
		in = sh_pathopen(cp);
	}
	else
	{
		script_id = "(stdin)";
		in = sfstdin;
	}
	if(cp= *argv)
	{
		struct stat statb;
		if(!(out = sfopen(NULL,cp,"w")))
		{
			errormsg(SH_DICT,ERROR_system(1),"%s: cannot create",cp);
			UNREACHABLE();
		}
		if(fstat(sffileno(out),&statb) >=0)
			chmod(cp,(statb.st_mode&~S_IFMT)|S_IXUSR|S_IXGRP|S_IXOTH);
	}
	else
		out = sfstdout;
	if(tty_check(sffileno(out)))
	{
		errormsg(SH_DICT,ERROR_exit(1),"refusing to write binary data to terminal",cp);
		UNREACHABLE();
	}
	if(dflag)
	{
		sh_onoption(SH_DICTIONARY);
		sh_onoption(SH_NOEXEC);
	}
	if(nflag)
		sh_onoption(SH_NOEXEC);
	if(vflag)
		sh_onoption(SH_VERBOSE);
	if(!dflag)
		sfwrite(out,header,sizeof(header));  /* write binary shcomp header */
#if SHOPT_ESH || SHOPT_VSH
	sh_offoption(SH_MULTILINE);
#endif
	sh.inlineno = 1;
#if SHOPT_BRACEPAT
	sh_onoption(SH_BRACEEXPAND);
#endif
	error_info.id = script_id;
	while(1)
	{
		stkset(sh.stk,NULL,0);
		if(t = (Shnode_t*)sh_parse(in,0))
		{
			if((t->tre.tretyp&(COMMSK|COMSCAN))==0 && t->com.comnamp && (Namval_t*)t->com.comnamp==SYSALIAS)
				/* Create aliases found in the script to prevent syntax errors */
				sh_exec(t,0);
			if(!dflag && sh_tdump(out,t) < 0)
			{
				error_info.id = shcomp_id;
				errormsg(SH_DICT,ERROR_exit(1),"dump failed");
				UNREACHABLE();
			}
		}
		else if(sfeof(in))
			break;
		if(sferror(in))
		{
			error_info.id = shcomp_id;
			errormsg(SH_DICT,ERROR_system(1),"I/O error");
			UNREACHABLE();
		}
		if(t && ((t->tre.tretyp&COMMSK)==TCOM) && (np=t->com.comnamp) && (cp=nv_name(np)))
		{
			if(strcmp(cp,"exit")==0)
				break;
			/* check for exec of a command */
			if(strcmp(cp,"exec")==0)
			{
				if(t->com.comtyp&COMSCAN)
				{
					if(t->com.comarg.ap->argnxt.ap)
						break;
				}
				else if(t->com.comarg.dp->dolnum > 1)
					break;
			}
		}
	}
	/* copy any remaining input */
	if(!sfeof(in))
		sfmove(in,out,SFIO_UNBOUND,-1);
	if(in!=sfstdin)
		sfclose(in);
	if(out!=sfstdout)
		sfclose(out);
	return 0;
}

/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1992-2013 AT&T Intellectual Property          *
*          Copyright (c) 2020-2024 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*               Glenn Fowler <glenn.s.fowler@gmail.com>                *
*                    David Korn <dgkorn@gmail.com>                     *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * output the beginning portion of one or more files
 */

static const char usage[] =
"[-n?\n@(#)$Id: head (AT&T Research) 2013-09-19 $\n]"
"[--catalog?" ERROR_CATALOG "]"
"[+NAME?head - output beginning portion of one or more files ]"
"[+DESCRIPTION?\bhead\b copies one or more input files to standard "
    "output stopping at a designated point for each file or to the end of "
    "the file whichever comes first. Copying ends at the point indicated by "
    "the options. By default a header of the form \b==> \b\afilename\a\b "
    "<==\b is output before all but the first file but this can be changed "
    "with the \b-q\b and \b-v\b options.]"
"[+?If no \afile\a is given, or if the \afile\a is \b-\b, \bhead\b "
    "copies from standard input starting at the current location.]"
"[+?The option argument for \b-n\b, \b-c\b and \b-s\b may end in one of the following "
	"case-insensitive unit suffixes, with an optional trailing \bB\b ignored:]{"
		"[+b?512 (block)]"
		"[+k?1000 (kilo)]"
		"[+Ki?1024 (kibi)]"
		"[+M?1000*1000 (mega)]"
		"[+Mi?1024*1024 (mebi)]"
		"[+G?1000*1000*1000 (giga)]"
		"[+Gi?1024*1024*1024 (gibi)]"
		"[+...?and so on for T, Ti, P, Pi, E, and Ei.]"
	"}"
"[+?For backward compatibility, \b-\b\anumber\a is equivalent to \b-n\b "
    "\anumber\a.]"
"[n:lines?Copy \alines\a lines from each file.]#[lines:=10]"
"[c:bytes?Copy \achars\a bytes from each file.]#[chars]"
"[q:quiet|silent?Never output filename headers.]"
"[s:skip?Skip \askip\a characters or lines from each file before "
    "copying.]#[skip]"
"[v:verbose?Always output filename headers.]"
    "\n\n"
"[ file ... ]"
    "\n\n"
"[+EXIT STATUS?]"
    "{"
        "[+0?All files copied successfully.]"
        "[+>0?One or more files did not copy.]"
    "}"
"[+SEE ALSO?\bcat\b(1), \btail\b(1)]"
;

#include <cmd.h>

int
b_head(int argc, char** argv, Shbltin_t* context)
{
	static const char	header_fmt[] = "\n==> %s <==\n";

	Sfio_t*		fp;
	char*		cp;
	off_t		keep = 10;
	off_t		skip = 0;
	int		delim = '\n';
	off_t		moved;
	int		header = 1;
	char*		format = (char*)header_fmt+1;

	cmdinit(argc, argv, context, ERROR_CATALOG, 0);
	for (;;)
	{
		switch (optget(argv, usage))
		{
		case 'c':
			delim = -1;
			/* FALLTHROUGH */
		case 'n':
			if (opt_info.offset && argv[opt_info.index][opt_info.offset] == 'c')
			{
				delim = -1;
				opt_info.offset++;
			}
			if ((keep = opt_info.number) <=0)
				error(2, "%s: %I*d: positive numeric option argument expected", opt_info.name, sizeof(keep), keep);
			continue;
		case 'q':
			header = argc;
			continue;
		case 'v':
			header = 0;
			continue;
		case 's':
			skip = opt_info.number;
			continue;
		case '?':
			error(ERROR_usage(2), "%s", opt_info.arg);
			UNREACHABLE();
		case ':':
			error(2, "%s", opt_info.arg);
			continue;
		}
		break;
	}
	argv += opt_info.index;
	argc -= opt_info.index;
	if (error_info.errors)
	{
		error(ERROR_usage(2), "%s", optusage(NULL));
		UNREACHABLE();
	}
	if (cp = *argv)
		argv++;
	do
	{
		if (!cp || streq(cp, "-"))
		{
			cp = "/dev/stdin";
			fp = sfstdin;
			sfset(fp, SFIO_SHARE, 1);
		}
		else if (!(fp = sfopen(NULL, cp, "r")))
		{
			error(ERROR_system(0), "%s: cannot open", cp);
			continue;
		}
		if (argc > header)
			sfprintf(sfstdout, format, cp);
		format = (char*)header_fmt;
		if (skip > 0)
		{
			if ((moved = sfmove(fp, NULL, skip, delim)) < 0 && !ERROR_PIPE(errno) && errno != EINTR)
				error(ERROR_system(0), "%s: skip error", cp);
			if (delim >= 0 && moved < skip)
				goto next;
		}
		if ((moved = sfmove(fp, sfstdout, keep, delim)) < 0 && !ERROR_PIPE(errno) && errno != EINTR ||
			delim >= 0 && moved < keep && sfmove(fp, sfstdout, SFIO_UNBOUND, -1) < 0 && !ERROR_PIPE(errno) && errno != EINTR)
			error(ERROR_system(0), "%s: read error", cp);
	next:
		if (fp != sfstdin)
			sfclose(fp);
	} while (cp = *argv++);
	if (sfsync(sfstdout))
		error(ERROR_system(0), "write error");
	return error_info.errors != 0;
}

/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1992-2012 AT&T Intellectual Property          *
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
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  David Korn <dgk@research.att.com>                   *
*                                                                      *
***********************************************************************/
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Research
 *
 * getconf - get configuration values
 */

static const char usage[] =
"[-?\n@(#)$Id: getconf (ksh 93u+m) 2021-04-20 $\n]"
"[--catalog?" ERROR_CATALOG "]"
"[+NAME?getconf - get configuration values]"
"[+DESCRIPTION?\bgetconf\b displays the system configuration value for"
"	\aname\a. If \aname\a is a filesystem specific variable then"
"	the value is determined relative to \apath\a or the current"
"	directory if \apath\a is omitted. If \avalue\a is specified then"
"	\bgetconf\b attempts to change the process local value to \avalue\a."
"	\b-\b may be used in place of \apath\a when it is not relevant."
"	If \apath\a is \b=\b then the \avalue\a is cached and used"
"	for subsequent tests in the calling and all child processes."
"	Only \bwritable\b variables may be set; \breadonly\b variables"
"	cannot be changed.]"
"[+?The current value for \aname\a is written to the standard output. If"
"	\aname\a is valid but undefined then \bundefined\b is written to"
"	the standard output. If \aname\a is invalid or an error occurs in"
"	determining its value, then a diagnostic written to the standard error"
"	and \bgetconf\b exits with a non-zero exit status.]"
"[+?More than one variable may be set or queried by providing the \aname\a"
"	\apath\a \avalue\a 3-tuple for each variable, specifying \b-\b for"
"	\avalue\a when querying.]"
"[+?If no operands are specified then all known variables are written in"
"	\aname\a=\avalue\a form to the standard output, one per line."
"	Only one of \b--call\b, \b--name\b or \b--standard\b may be specified.]"
"[+?This implementation uses the \bastgetconf\b(3) string interface to the native"
"	\bsysconf\b(3), \bconfstr\b(3), \bpathconf\b(2), and \bsysinfo\b(2)"
"	system calls."
"	Invalid options and/or names not supported by \bastgetconf\b(3) cause"
"	the default native \bgetconf\b, named by \b$(getconf GETCONF)\b, to"
"	be executed (unless the shell is in restricted mode, in which case"
"	an error will occur).]"

"[a:all?Call the native \bgetconf\b(1) with option \b-a\b.]"
"[b:base?List base variable name sans call and standard prefixes.]"
"[c:call?Display variables with call prefix that matches \aRE\a. The call"
"	prefixes are:]:[RE]{"
"		[+CS?\bconfstr\b(3)]"
"		[+PC?\bpathconf\b(2)]"
"		[+SC?\bsysconf\b(3)]"
"		[+SI?\bsysinfo\b(2)]"
"		[+XX?Constant value.]"
"}"
"[d:defined?Only display defined values when no operands are specified.]"
"[l:lowercase?List variable names in lower case.]"
"[n:name?Display variables with name that match \aRE\a.]:[RE]"
"[p:portable?Display the named \bwritable\b variables and values in a form that"
"	can be directly executed by \bsh\b(1) to set the values. If \aname\a"
"	is omitted then all \bwritable\b variables are listed.]"
"[q:quote?\"...\" quote string values.]"
"[r:readonly?Display the named \breadonly\b variables in \aname\a=\avalue\a form."
"	If \aname\a is omitted then all \breadonly\b variables are listed.]"
"[s:standard?Display variables with standard prefix that matches \aRE\a."
"	Use the \b--table\b option to view all standard prefixes, including"
"	local additions. The standard prefixes available on all systems"
"	are:]:[RE]{"
"		[+AES]"
"		[+AST]"
"		[+C]"
"		[+GNU]"
"		[+POSIX]"
"		[+SVID]"
"		[+XBS5]"
"		[+XOPEN]"
"		[+XPG]"
"}"
"[t:table?Display the internal table that contains the name, standard,"
"	standard section, and system call symbol prefix for each variable.]"
"[w:writable?Display the named \bwritable\b variables in \aname\a=\avalue\a"
"	form. If \aname\a is omitted then all \bwritable\b variables are"
"	listed.]"
"[v:specification?Call the native \bgetconf\b(1) with option"
"	\b-v\b \aname\a.]:[name]"

"\n"
"\n[ name [ path [ value ] ] ... ]\n"
"\n"

"[+ENVIRONMENT]"
    "{"
        "[+_AST_FEATURES?Process local writable values that are "
            "different from the default are stored in the \b_AST_FEATURES\b "
            "environment variable. The \b_AST_FEATURES\b value is a "
            "space-separated list of \aname\a \apath\a \avalue\a 3-tuples, "
            "where \aname\a is the system configuration name, \apath\a is "
            "the corresponding path, \b-\b if no path is applicable, and "
            "\avalue\a is the system configuration value. \b_AST_FEATURES\b "
            "is an implementation detail of process inheritance; it may "
            "change or vanish in the future; don't rely on it.]"
    "}"
"[+SEE ALSO?\bpathchk\b(1), \bconfstr\b(3), \bpathconf\b(2),"
"	\bsysconf\b(3), \bastgetconf\b(3)]"
;

#include <cmd.h>
#include <proc.h>
#include <ls.h>

typedef struct Path_s
{
	const char*	path;
	int		len;
} Path_t;

int
b_getconf(int argc, char** argv, Shbltin_t* context)
{
	register char*		name;
	register char*		path;
	register char*		value;
	register const char*	s;
	char*			pattern;
	char*			native;
	int			flags;
	int			n;
	char**			oargv;
	static const char	empty[] = "-";

	cmdinit(argc, argv, context, ERROR_CATALOG, 0);
	oargv = argv;
	if (*(native = astconf("GETCONF", NiL, NiL)) != '/')
		native = 0;
	flags = 0;
	name = 0;
	pattern = 0;
	for (;;)
	{
		switch (optget(argv, usage))
		{
		case 'a':
			if (native)
				goto defer;
			continue;
		case 'b':
			flags |= ASTCONF_base;
			continue;
		case 'c':
			flags |= ASTCONF_matchcall;
			pattern = opt_info.arg;
			continue;
		case 'd':
			flags |= ASTCONF_defined;
			continue;
		case 'l':
			flags |= ASTCONF_lower;
			continue;
		case 'n':
			flags |= ASTCONF_matchname;
			pattern = opt_info.arg;
			continue;
		case 'p':
			flags |= ASTCONF_parse;
			continue;
		case 'q':
			flags |= ASTCONF_quote;
			continue;
		case 'r':
			flags |= ASTCONF_read;
			continue;
		case 's':
			flags |= ASTCONF_matchstandard;
			pattern = opt_info.arg;
			continue;
		case 't':
			flags |= ASTCONF_table;
			continue;
		case 'v':
			if (native)
				goto defer;
			continue;
		case 'w':
			flags |= ASTCONF_write;
			continue;
		case ':':
			if (native)
				goto defer;
			error(2, "%s", opt_info.arg);
			break;
		case '?':
			error(ERROR_usage(2), "%s", opt_info.arg);
			UNREACHABLE();
		}
		break;
	}
	argv += opt_info.index;
	if (!(name = *argv))
		path = 0;
	else if (streq(name, empty))
	{
		name = 0;
		if (path = *++argv)
		{
			argv++;
			if (streq(path, empty))
				path = 0;
		}
	}
	if (error_info.errors || !name && *argv)
	{
		error(ERROR_usage(2), "%s", optusage(NiL));
		UNREACHABLE();
	}
	if (!name)
		astconflist(sfstdout, path, flags, pattern);
	else
	{
		if (native)
			flags |= (ASTCONF_system|ASTCONF_error);
		do
		{
			if (!(path = *++argv))
				value = 0;
			else
			{
				if (streq(path, empty))
				{
					path = 0;
					flags = 0;
				}
				if ((value = *++argv) && (streq(value, empty)))
				{
					value = 0;
					flags = 0;
				}
			}
			s = astgetconf(name, path, value, flags, errorf);
			if (error_info.errors)
				break;
			if (!s)
			{
				if (native)
					goto defer;
				error(2, "%s: unknown name", name);
				break;
			}
			if (!value)
			{
				if (flags & ASTCONF_write)
				{
					sfputr(sfstdout, name, ' ');
					sfputr(sfstdout, path ? path : empty, ' ');
				}
				sfputr(sfstdout, s, '\n');
			}
		} while (*argv && (name = *++argv));
	}
	return error_info.errors != 0;

 defer:
	/*
	 * Run the external getconf command
	 */
	oargv[0] = native;
	if ((n = sh_run(context, argc, oargv)) >= EXIT_NOEXEC)
		error(ERROR_SYSTEM|2, "%s: exec error [%d]", native, n);
	return n;
}

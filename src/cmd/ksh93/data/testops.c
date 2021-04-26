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
 * tables for the test builtin [[ ... ]] and [ ... ]
 */

#include	<ast.h>

#include	"defs.h"
#include	"test.h"

/*
 * This is the list of binary test and [[ ... ]] operators
 */

const Shtable_t shtab_testops[] =
{
		"!=",		TEST_SNE,
		"-a",		TEST_AND,
		"-ef",		TEST_EF,
		"-eq",		TEST_EQ,
		"-ge",		TEST_GE,
		"-gt",		TEST_GT,
		"-le",		TEST_LE,
		"-lt",		TEST_LT,
		"-ne",		TEST_NE,
		"-nt",		TEST_NT,
		"-o",		TEST_OR,
		"-ot",		TEST_OT,
		"=",		TEST_SEQ,
		"==",		TEST_SEQ,
		"=~",           TEST_REP,
		"<",		TEST_SLT,
		">",		TEST_SGT,
		"]]",		TEST_END,
		"",		0
};

const char sh_opttest[] =
"[-1c?\n@(#)$Id: test (ksh 93u+m) 2020-08-31 $\n]"
"[--catalog?" SH_DICT "]"
"[+NAME?test, [ - evaluate expression]"
"[+DESCRIPTION?\btest\b evaluates expressions and returns its result using the "
	"exit status. Option parsing is not performed; all arguments, "
	"including \b--\b, are processed as operands. It is essential to quote "
	"expression arguments to suppress empty removal, field splitting, file "
	"name generation, and constructs such as redirection. If the command "
	"is invoked as \b[\b, a final \b]]\b argument is required and "
	"discarded. The evaluation of the expression depends on the number of "
	"operands, as follows:]"
"{"
	"[+0?Evaluates to false.]"
	"[+1?True if argument is not an empty string.]"
	"[+2?If first operand is \b!\b, the result is True if the second "
		"operand is an empty string. Otherwise, it is evaluated "
		"as one of the unary expressions defined below. If the "
		"unary operator is invalid and the second argument is \b--\b, "
		"then the first argument is processed as a \bgetopts\b(1) "
		"help option.]"
	"[+3?If first operand is \b!\b, the result is True if the second "
		"and third operand evaluated as a unary expression is False. "
		"Otherwise, the three operands are evaluated as one of the "
		"binary expressions listed below.]"
	"[+4?If first operand is \b!\b, the result is True if the next "
		"three operands are a valid binary expression that is False.]"
"}"
"[+?For the following unary and binary operators:]"
"{"
	"[+?If any \afile\a is a symlink, it is followed before testing, "
		"except for \b-L\b/\b-h\b"
#if SHOPT_TEST_L
		"/\b-l\b"
#endif
		".]"
	"[+?If any \afile\a is of the form \b/dev/fd/\b\an\a, "
		"then file descriptor \an\a is checked.]"
	"[+?Operators marked with a * are not part of the POSIX standard.]"
"}"
"[+UNARY OPERATORS?These evaluate as True if:]{"
	"[+-a \afile\a *?Same as \b-e\b.]"
	"[+-b \afile\a?\afile\a is a block special file.]"
	"[+-c \afile\a?\afile\a is a character special file.]"
	"[+-d \afile\a?\afile\a is a directory.]"
	"[+-e \afile\a?\afile\a exists and is not a broken symlink.]"
	"[+-f \afile\a?\afile\a is a regular file.]"
	"[+-g \afile\a?\afile\a has its set-group-id bit set.]"
	"[+-h \afile\a?Same as \b-L\b.]"
	"[+-k \afile\a *?\afile\a has its sticky bit on.]"
#if SHOPT_TEST_L
	"[+-l \afile\a *?Same as \b-L\b.]"
#endif
	"[+-n \astring\a?Length of \astring\a is non-zero.]"
	"[+-o \aoption\a *?Shell option \aoption\a is enabled.]"
	"[+-p \afile\a?\afile\a is a FIFO.]"
	"[+-r \afile\a?\afile\a is readable.]"
	"[+-s \afile\a?\afile\a has size > 0.]"
	"[+-t \anum\a?File descriptor number \anum\a is "
		"open and associated with a terminal.]"
	"[+-u \afile\a?\afile\a has its set-user-id bit set.]"
	"[+-v \avarname\a *?The variable \avarname\a is set.]"
	"[+-w \afile\a?\afile\a is writable.]"
	"[+-x \afile\a?\afile\a is executable, or if directory, searchable.]"
	"[+-z \astring\a?\astring\a is a zero-length string.]"
	"[+-G \afile\a *?Group of \afile\a is the effective "
		"group ID of the current process.]"
	"[+-L \afile\a?\afile\a is a symbolic link.]"
	"[+-N \afile\a *?\afile\a has been modified since it was last read.]"
	"[+-O \afile\a *?\afile\a exists and owner is the effective "
		"user ID of the current process.]"
	"[+-R \avarname\a *?\avarname\a is a name reference.]"
	"[+-S \afile\a?\afile\a is a socket.]"
"}"
"[+BINARY OPERATORS?These evaluate as True if:]{"
	"[+\astring1\a = \astring2\a?\astring1\a is equal to \astring2\a.]"
	"[+\astring1\a == \astring2\a *?Same as \b=\b.]"
	"[+\astring1\a != \astring2\a?\astring1\a is not equal to \astring2\a.]"
	"[+\anum1\a -eq \anum2\a?\anum1\a is numerically equal to \anum2\a.]"
	"[+\anum1\a -ne \anum2\a?\anum1\a is not numerically equal to \anum2\a.]"
	"[+\anum1\a -lt \anum2\a?\anum1\a is less than \anum2\a.]"
	"[+\anum1\a -le \anum2\a?\anum1\a is less than or equal to \anum2\a.]"
	"[+\anum1\a -gt \anum2\a?\anum1\a is greater than \anum2\a.]"
	"[+\anum1\a -ge \anum2\a?\anum1\a is greater than or equal to \anum2\a.]"
	"[+\afile1\a -nt \afile2\a *?\afile1\a is newer than \afile2\a, "
		"or \afile1\a exists and \afile2\a does not.]"
	"[+\afile1\a -ot \afile2\a *?\afile1\a is older than \afile2\a, "
		"or \afile1\a does not exist and \afile2\a does.]"
	"[+\afile1\a -ef \afile2\a *?\afile1\a is a hard link or "
		"symbolic link to \afile2\a.]"
"}"
"\n"
"\nexpression\n"
"\n"
"[+EXIT STATUS?]{"
	"[+0?The expression evaluated as True.]"
	"[+1?The expression evaluated as False.]"
	"[+>1?An error occurred.]"
"}"

"[+SEE ALSO?\blet\b(1), \bexpr\b(1)]"
;

const char test_opchars[]	= "HLNRSVOGCaeohrwxdcbfugkv"
#if SHOPT_TEST_L
	"l"
#endif
				"psnzt";
const char e_argument[]		= "argument expected";
const char e_missing[]		= "%s missing";
const char e_badop[]		= "%s: unknown operator";
const char e_unsupported_op[]	= "%s: operator not supported; use [[ ... ]]";
const char e_tstbegin[]		= "[[ ! ";
const char e_tstend[]		= " ]]\n";

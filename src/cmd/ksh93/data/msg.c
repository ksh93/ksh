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
*                                                                      *
***********************************************************************/
/*
 *	UNIX shell
 *	S. R. Bourne
 *	Rewritten by David Korn
 *
 *	AT&T Labs
 *
 */

#include	"shopt.h"
#include	<ast.h>
#include	<errno.h>
#include	"defs.h"
#include	"path.h"
#include	"io.h"
#include	"shlex.h"
#include	"timeout.h"
#include	"history.h"
#include	"builtins.h"
#include	"jobs.h"
#include	"edit.h"

/* error messages */
const char e_timewarn[]		= "\r\n\ashell will timeout in 60 seconds due to inactivity";
const char e_runvi[]		= "\\hist -e \"${VISUAL:-${EDITOR:-vi}}\" ";
const char e_timeout[]		= "timed out waiting for input";
const char e_mailmsg[]		= "you have mail in $_";
const char e_query[]		= "no query process";
#if !SHOPT_SCRIPTONLY
const char e_history[]		= "no history file";
const char e_histopen[]		= "cannot open history file";
#endif /* !SHOPT_SCRIPTONLY */
const char e_optincompat1[]	= "%s cannot be used with other options";
const char e_optincompat2[]	= "%s cannot be used with %s";
const char e_toomany[]		= "open file limit exceeded";
const char e_argtype[]		= "invalid argument of type %c";
const char e_oneoperand[]	= "one operand expected";
const char e_toomanyops[]	= "too many operands";
const char e_formspec[]		= "%c: unknown format specifier";
const char e_badregexp[]	= "%s: invalid regular expression";
const char e_number[]		= "%s: bad number";
const char e_badlocale[]	= "%s: unknown locale";
const char e_nullset[]		= "%s: parameter null";
const char e_notset[]		= "%s: parameter not set";
const char e_noparent[]		= "%s: no parent";
const char e_subst[]		= "%s: bad substitution";
const char e_create[]		= "%s: cannot create";
const char e_tmpcreate[]	= "cannot create temporary file";
const char e_restricted[]	= "%s: restricted";
const char e_copexists[]	= "coprocess is running; cannot create a new coprocess";
const char e_exists[]		= "%s: file already exists";
const char e_pipe[]		= "cannot create pipe";
const char e_alarm[]		= "cannot set alarm";
const char e_open[]		= "%s: cannot open";
const char e_notseek[]		= "%s: not seekable";
const char e_readscript[]	= "read from script failed";
const char e_badseek[]		= "%s: invalid seek offset";
const char e_badpattern[]	= "%s: invalid shell pattern";
const char e_noread[]		= "%s: pattern seek requires read access";
const char e_logout[]		= "Use 'exit' to terminate this shell";
const char e_exec[]		= "%s: cannot execute";
const char e_pwd[]		= "cannot determine present working directory";
const char e_found[]		= "%s: not found";
#ifdef ENAMETOOLONG
const char e_toolong[]		= "%s: file name too long";
#endif
const char e_defined[]		= "%s: function not defined";
const char e_subscript[]	= "%s: subscript out of range";
const char e_toodeep[]		= "%s: recursion too deep";
const char e_access[]		= "permission denied";
#ifdef _cmd_universe
    const char e_nouniverse[]	= "universe not accessible";
#endif /* _cmd_universe */
const char e_direct[]		= "bad directory";
const char e_file[]		= "%s: bad file unit number";
const char e_redirect[]		= "redirection failed";
const char e_trap[]		= "%s: bad trap";
const char e_readonly[]		= "%s: is read only";
const char e_badfield[]		= "%d: negative field size";
const char e_ident[]		= "%s: is not an identifier";
const char e_badname[]		= "%s: invalid name";
const char e_varname[]		= "%s: invalid variable name";
const char e_badfun[]		= "%s: invalid function name";
const char e_aliname[]		= "%s: invalid alias name";
const char e_badexport[]	= "%s: only simple variables can be exported";
const char e_badref[]		= "%s: reference variable cannot be an array";
const char e_badsubscript[]	= "%c: invalid subscript in assignment";
const char e_noarray[]		= "%s: cannot be an array";
const char e_badappend[]	= "%s: invalid append to associative array";
const char e_noref[]		= "%s: no reference name";
const char e_nounattr[]		= "cannot unset attribute C or A or a";
const char e_selfref[]		= "%s: invalid self reference";
const char e_globalref[]	= "%s: global reference cannot refer to local variable";
const char e_noalias[]		= "%s: alias not found\n";
const char e_notrackedalias[]	= "%s: tracked alias not found\n";
const char e_format[]		= "%s: bad format";
const char e_redef[]		= "%s: type cannot be redefined";
const char e_required[]		= "%s: is a required element of %s";
const char e_badtformat[]	= "%c: bad format character in time format";
const char e_notimp[]		= "%s: not implemented";
const char e_notelem[]		= "%.*s: is not an element of %s";
const char e_notenum[]		= "%s: not an enumeration type";
const char e_unknowntype[]	= "%.*s: unknown type";
const char e_unknownmap[]	= "%s: unknown mapping name";
const char e_mapchararg[]	= "-M requires argument when operands are specified";
const char e_subcomvar[]	= "%s: compound assignment requires sub-variable name";
const char e_badtypedef[]	= "%s: type definition requires compound assignment";
const char e_typecompat[]	= "%s: array instance incompatible with type assignment";
const char e_nosupport[]	= "not supported";
const char e_badrange[]		= "%d-%d: invalid range";
const char e_eneedsarg[]	= "-e - requires single argument";
const char e_limit[]		= "%s: could not get limit";
const char e_overlimit[]	= "%s: limit exceeded";
const char e_badsyntax[]	= "incorrect syntax";
const char e_badwrite[]		= "write to %d failed";
const char e_staticfun[]	= "%s: defined as a static function in type %s and cannot be redefined";
const char is_reserved[]	= " is a keyword";
const char is_builtin[]		= " is a shell builtin";
const char is_spcbuiltin[]	= " is a special shell builtin";
const char is_builtver[]	= "is a shell builtin version of";
const char is_alias[]		= "%s is an alias for ";
const char is_talias[]		= "is a tracked alias for";
const char is_function[]	= " is a function";
const char is_ufunction[]	= " is an undefined function";
const char e_autoloadfrom[]	= " (autoload from %s)";
const char e_newtty[]		= "Switching to new tty driver...";
const char e_oldtty[]		= "Reverting to old tty driver...";
const char e_no_start[]		= "Cannot start job control";
const char e_no_jctl[]		= "No job control";
const char e_terminate[]	= "You have stopped jobs";
const char e_done[]		= " Done";
const char e_nlspace[]		= "\n      ";
const char e_running[]		= " Running";
const char e_ambiguous[]	= "%s: Ambiguous";
const char e_jobsrunning[]	= "You have running jobs";
const char e_no_job[]		= "no such job";
const char e_no_proc[]		= "no such process";
const char e_jobusage[]		= "%s: Arguments must be %%job or process IDs";
const char e_coredump[]		= "(coredump)";
const char e_alphanum[]		= "[_[:alpha:]]*([_[:alnum:]])";
const char e_devfdNN[]		= "/dev/fd/+([0-9])";
const char e_signo[]		= "Signal %d";

/* string constants */
const char e_heading[]		= "Current option settings";
const char e_sptbnl[]		= " \t\n";
const char e_tolower[]		= "tolower";
const char e_toupper[]		= "toupper";
const char e_defedit[]		= _pth_ed;
const char e_unknown []		= "<command unknown>";
const char e_devnull[]		= "/dev/null";
const char e_traceprompt[]	= "+ ";
const char e_supprompt[]	= "# ";
const char e_stdprompt[]	= "$ ";
const char e_profile[]		= "$HOME/.profile";
#ifdef BUILD_DTKSH
const char e_sysprofile[] = PROFILEPATH;
const char e_suidprofile[] = SUIDPROFILEPATH;
#else
const char e_sysprofile[]	= "/etc/profile";
const char e_suidprofile[]	= "/etc/suid_profile";
#endif
#if SHOPT_SYSRC
const char e_sysrc[]		= "/etc/ksh.kshrc";
#endif
#if !SHOPT_SCRIPTONLY
const char hist_fname[]		= "/.sh_history";
#endif /* !SHOPT_SCRIPTONLY */
const char e_dot[]		= ".";
const char e_timeformat[]	= "\nreal\t%3lR\nuser\t%3lU\nsys\t%3lS";
const char e_dict[]		= "libshell";
const char e_funload[]		= "function, built-in or type definition for %s not found in %s";
#if SHOPT_SCRIPTONLY
const char e_scriptonly[]	= "script-only shell";
#endif /* SHOPT_SCRIPTONLY */

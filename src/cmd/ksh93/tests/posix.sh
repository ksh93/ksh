########################################################################
#                                                                      #
#              This file is part of the ksh 93u+m package              #
#          Copyright (c) 2022-2022 Contributors to ksh 93u+m           #
#                    <https://github.com/ksh93/ksh>                    #
#                      and is licensed under the                       #
#                 Eclipse Public License, Version 2.0                  #
#                                                                      #
#                A copy of the License is available at                 #
#      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      #
#         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         #
#                                                                      #
#                  Martijn Dekker <martijn@inlv.org>                   #
#                                                                      #
########################################################################

. "${SHTESTS_COMMON:-${0%/*}/_common}"

# Tests for the --posix compatibility mode. The comments are quotes from the manual page.
# Some of these tests need to use 'eval' to avoid the parser parsing ahead and/or to work with shcomp.

set -o noglob
typeset -xi testint=123

if ! (set -o posix) 2>/dev/null
then
	warning "$SHELL does not have a posix option -- skipping tests"
	exit 0
fi
command set --noposix 2>/dev/null && [[ ! -o posix ]] || err_exit "set --noposix does not work"
command set --posix 2>/dev/null && [[ -o posix ]] || err_exit "set --posix does not work"
((Errors)) && exit "$Errors"  # no sense in continuing if the above fail

# The --posix option enables the POSIX standard mode for maximum compatibility with other compliant shells. At the
# moment that the posix option is turned on, it also turns on letoctal and turns off -B/braceexpand; the reverse
# is done when posix is turned back off. (These options can still be controlled independently in between.)
let "017 == 15" || err_exit "--posix does not turn on letoctal"
(set --noletoctal; let "017 == 17") || err_exit "--posix does not allow independent control of letoctal"
(set --noposix; let "017 == 17") || err_exit "--noposix does not turn off letoctal"
(set --noposix; set --letoctal; let "017 == 15") || err_exit "--noposix does not allow independent control of letoctal"
(set {1..4}; let "$# == 1") || err_exit "--posix does not turn off braceexpand"
if ((SHOPT_BRACEPAT)); then
	(set -B; eval 'set {1..4}'; let "$# == 4") || err_exit "--posix does not allow independent control of braceexpand"
	(set --noposix; eval 'set {1..4}'; let "$# == 4") || err_exit "--noposix does not turn on braceexpand"
fi
(set --noposix; ((SHOPT_BRACEPAT)) && set +B; eval 'set {1..4}'; let "$# == 1") || err_exit "--noposix does not allow independent control of braceexpand"

# When brace expansion is turned back on in POSIX mode, the values of unquoted variable expansions are not subject to it.
# (note that, in typical quirky ksh fashion, this only happens if pathname expansion is also on, i.e. -f/--noglob is off.)
if	((SHOPT_BRACEPAT))
then
	got=$(set -B +f; a='x{1,2}'; print $a)
	exp='x{1,2}'
	[[ $got == "$exp" ]] || err_exit "posix braceexpand expands unquoted variable expansions" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
	got=$(set --noposix +f; a='x{1,2}'; print $a)
	exp='x1 x2'
	[[ $got == "$exp" ]] || err_exit "non-posix braceexpand does not expand unquoted variable expansions" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
fi

# Furthermore, the posix option is automatically turned on upon invocation if the shell is invoked as sh or rsh,
# or if -o posix or --posix is specified on the shell invocation command line, or when executing scripts
# without a #! path with this option active in the invoking shell.
# In that case, the invoked shell will not set the preset aliases even if interactive, and will not import
# type attributes for variables (such as integer or left/right justify) from the environment.
set --noposix
ln -s "$SHELL" sh
ln -s "$SHELL" rsh
script=$'[[ -o posix ]]\necho $?\ntypeset -p testint'
exp=$'0\ntypeset -x testint=123'
got=$(./sh -c "$script")
[[ $got == "$exp" ]] || err_exit "incorrect --posix settings on invocation as sh" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(./rsh -c "$script")
[[ $got == "$exp" ]] || err_exit "incorrect --posix settings on invocation as rsh" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
echo "$script" >script
chmod +x script
set --posix
./script >|out
got=$(<out)
[[ $got == "$exp" ]] || err_exit "incorrect --posix settings on invoking hashbangless script from posix shell (direct)" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

: <<\DISABLED  # TODO: these do not pass yet, unless SHOPT_SPAWN is disabled.
(./script) >|out
got=$(<out)
[[ $got == "$exp" ]] || err_exit "incorrect --posix settings on invoking hashbangless script from posix shell (subshell)" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(./script)
[[ $got == "$exp" ]] || err_exit "incorrect --posix settings on invoking hashbangless script from posix shell (comsub)" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
DISABLED

got=$("$SHELL" --posix -c "$(<script)")
[[ $got == "$exp" ]] || err_exit "incorrect --posix settings on invoking -c script from posix shell" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
set --noposix
exp=$'1\ntypeset -x -i testint=123'
got=$(./script)
[[ $got == "$exp" ]] || err_exit "incorrect --posix settings on invoking hashbangless script from noposix shell" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
set --posix

# In addition, while on, the posix option:
#
# disables exporting variable type attributes to the environment for other ksh processes to import;
exp='typeset -x testint=123'
got=$("$SHELL" -c 'typeset -p testint')
[[ $got == "$exp" ]] || err_exit "variable attributes incorrectly exported in --posix mode" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
set --noposix
exp='typeset -x -i testint=123'
got=$("$SHELL" -c 'typeset -p testint')
[[ $got == "$exp" ]] || err_exit "variable attributes not exported in --noposix mode" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
set --posix

# disables the special handling of repeated isspace class characters in the IFS variable;
IFS=$'x\t\ty' val=$'\tun\t\tduo\ttres\t'
got=$(set $val; echo "$#")
exp=3
[[ $got == "$exp" ]] || err_exit "repeated IFS whitespace char (posix): incorrect number of fields" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(set --noposix; set $val; echo "$#")
exp=5
[[ $got == "$exp" ]] || err_exit "repeated IFS whitespace char (noposix): incorrect number of fields" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(set --default; set $val; echo "$#")
[[ $got == "$exp" ]] || err_exit "repeated IFS whitespace char (default): incorrect number of fields" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
IFS=$' \t\n' # default

# causes file descriptors > 2 to be left open when invoking another program;
exp='ok'
got=$(redirect 3>&1; "$SHELL" -c 'echo ok >&3' 2>/dev/null)
[[ $got == "$exp" ]] || err_exit "file descriptor 3 not left open in --posix mode" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
exp=''
got=$(set --noposix; redirect 3>&1; "$SHELL" -c 'echo ok >&3' 2>/dev/null)
[[ $got == "$exp" ]] || err_exit "file descriptor 3 left open in --noposix mode" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# disables the &> redirection shorthand;
exp=''
(set --posix; eval 'echo output &>out') >/dev/null
got=$(<out)
[[ $got == "$exp" ]] || err_exit "&> redirection shorthand not disabled in --posix mode" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
(set --noposix; eval 'echo output &>out') >/dev/null
exp='output'
got=$(<out)
[[ $got == "$exp" ]] || err_exit "&> redirection shorthand disabled in --noposix mode" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# disables fast filescan loops of type 'while inputredirection; do list; done';
printf '%s\n' "un duo tres" >out
set --
exp='0'
got=$(while <out; do echo "$#" "$@"; break; done)
[[ $got == "$exp" ]] || err_exit "filescan loop not disabled in --posix mode" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
if ((SHOPT_FILESCAN))
then
	exp='3 un duo tres'
	got=$(set --noposix; while <out; do echo "$#" "$@"; break; done)
	[[ $got == "$exp" ]] || err_exit "filescan loop disabled in --noposix mode" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
fi

# makes the <> redirection operator default to redirecting standard input if no file descriptor number precedes it;
: >|out
(set --posix; eval 'echo foo <>out') >/dev/null
exp=''
got=$(<out)
[[ $got == "$exp" ]] || err_exit "<> redirects standard output in --posix mode" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
: >|out
(set --noposix; eval 'echo foo <>out') >/dev/null
exp='foo'
got=$(<out)
[[ $got == "$exp" ]] || err_exit "<> does not redirect standard output in --posix mode" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# disables the special floating point constants Inf and NaN in arithmetic evaluation so that, e.g., $((inf))
# and $((nan)) refer to the variables by those names;
inf=1 nan=2
let "inf==1 && nan==2" || err_exit "inf and nan are not variables in arithmetic expressions in --posix mode"
(set --noposix; let "inf==1 || nan==2") && err_exit "inf and nan are variables in arithmetic expressions in --noposix mode"

# enables the recognition of a leading zero as introducing an octal number in all arithmetic evaluation
# contexts, except in the let built-in while letoctal is off;
let "017 == 15" || err_exit "leading octal zero not recognised in 'let' in --posix"
(set --noletoctal; let "017 == 17") || err_exit "leading octal zero erroneously recognised in --posix --noletoctal"
(set --noposix; let "017 == 17") || err_exit "leading octal zero erroneously recognised in --noposix"
(set --noposix --letoctal; let "017 == 15") || err_exit "leading octal zero not recognised in --noposix --letoctal (1)"
(set --noposix; set --letoctal; let "017 == 15") || err_exit "leading octal zero not recognised in --noposix --letoctal (2)"
test 010 -eq 10 || err_exit "'test' not ignoring leading octal zero in --posix"
[ 010 -eq 10 ] || err_exit "'[' not ignoring leading octal zero in --posix"
[[ 010 -eq 8 ]] || err_exit "'[[' ignoring leading octal zero in --posix"
(set --noposix; [[ 010 -eq 10 ]]) || err_exit "'[[' not ignoring leading octal zero in --noposix"

# disables zero-padding of seconds in the output of the time and times built-ins;
case ${.sh.version} in
*93u+m/1.0.*)	exp=$'^user\t0m0.[0-9]{2}s\nsys\t0m0.[0-9]{2}s\n0m0.[0-9]{3}s 0m0.[0-9]{3}s\n0m0.000s 0m0.000s$' ;;
*)		exp=$'^user\t0m0.[0-9]{3}s\nsys\t0m0.[0-9]{3}s\n0m0.[0-9]{3}s 0m0.[0-9]{3}s\n0m0.000s 0m0.000s$' ;;
esac
got=$("$SHELL" --posix -c '{ time; } 2>&1; times')
[[ $got =~ $exp ]] || err_exit "POSIX time/times output: expected match of $(printf %q "$exp"), got $(printf %q "$got")"

# stops the . command (but not source) from looking up functions defined with the function syntax;
echo 'echo SCRIPT' >scrunction
function scrunction { echo FUNCTION; }
got=$(PATH=.:$PATH; . scrunction)
exp='SCRIPT'
[[ $got == "$exp" ]] || err_exit "'.' finds ksh function in --posix mode" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(set --noposix; PATH=.:$PATH; . scrunction)
exp='FUNCTION'
[[ $got == "$exp" ]] || err_exit "'.' does not find ksh function in --noposix mode" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(PATH=.:$PATH; source scrunction)
[[ $got == "$exp" ]] || err_exit "'source' does not find ksh function in --posix mode" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(set --noposix; PATH=.:$PATH; source scrunction)
[[ $got == "$exp" ]] || err_exit "'source' does not find ksh function in --noposix mode" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# changes the test/[ built-in command to make its deprecated expr1 -a expr2 and expr1 -o expr2 operators work
# even if expr1 equals "!" or "(" (which means the nonstandard unary -a file and -o option operators cannot
# be directly negated using ! or wrapped in parentheses);
# https://github.com/ksh93/ksh/issues/330
test ! -a "" && err_exit "POSIX test/[: binary -a operator does not work with '!' as left-hand expression"
test \( -a \) 2>/dev/null || err_exit "POSIX test/[: binary -a operator does not work with '(' as left-hand expression"
(set --trackall; test ! -o trackall) || err_exit "POSIX test/[: binary -o operator does not work with '!' as left-hand expression"
(set --noposix --trackall; test ! -o trackall) && err_exit "ksh test/[: unary -o operator does not work with '!' negator"
test \( -o \) 2>/dev/null || err_exit "POSIX test/[: binary -o operator does not work with '(' as left-hand expression"

# disables a hack that makes test -t ([ -t ]) equivalent to test -t 1 ([ -t 1 ]).
# ...simple 'test -t' is hacked in the parser (so we need 'eval')...
eval 'test -t' >/dev/null 2>&1 || err_exit "'test -t' does not test for nonemptiness of string '-t' in --posix mode"
eval '[ -t ]' >/dev/null 2>&1 || err_exit "'[ -t ]' does not test for nonemptiness of string '-t' in --posix mode"
(set --noposix; eval 'test -t') >/dev/null 2>&1 && err_exit "'test -t' is not equivalent to 'test -t 1' in --noposix mode"
(set --noposix; eval '[ -t ]') >/dev/null 2>&1 && err_exit "'[ -t ]' is not equivalent to '[ -t 1 ]' in --noposix mode"
# ...whereas complex expressions with bare '-t' are hacked in the test builtin itself
test X -a -t >/dev/null 2>&1 || err_exit "'test X -a -t' does not test for nonemptiness of string '-t' in --posix mode"
[ X -a -t ] >/dev/null 2>&1 || err_exit "'[ X -a -t ]' does not test for nonemptiness of string '-t' in --posix mode"
(set --noposix; test X -a -t) >/dev/null 2>&1 && err_exit "'test X -a -t' is not equivalent to 'test X -a -t 1' in --noposix mode"
(set --noposix; [ X -a -t ]) >/dev/null 2>&1 && err_exit "'[ X -a -t ]' is not equivalent to '[ X -a -t 1 ]' in --noposix mode"

# ======
exit $((Errors<125?Errors:125))

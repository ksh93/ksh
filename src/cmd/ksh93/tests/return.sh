########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1982-2011 AT&T Intellectual Property          #
#          Copyright (c) 2020-2024 Contributors to ksh 93u+m           #
#                      and is licensed under the                       #
#                 Eclipse Public License, Version 2.0                  #
#                                                                      #
#                A copy of the License is available at                 #
#      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      #
#         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         #
#                                                                      #
#                  David Korn <dgk@research.att.com>                   #
#                  Martijn Dekker <martijn@inlv.org>                   #
#            Johnothan King <johnothanking@protonmail.com>             #
#                                                                      #
########################################################################
# test the behavior of return and exit with functions

. "${SHTESTS_COMMON:-${0%/*}/_common}"

unset HISTFILE

foo=NOVAL bar=NOVAL
file=$tmp/test
function foo
{
	typeset foo=NOEXIT
	trap "foo=EXIT;rm -f $file" EXIT
	> $file
	if	(( $1 == 0 ))
	then	return $2
	elif	(( $1 == 1 ))
	then	exit $2
	else	bar "$@"
	fi
}

function bar
{
	typeset bar=NOEXIT
	trap 'bar=EXIT' EXIT
	if	(( $1 == 2 ))
	then	return $2
	elif	(( $1 == 3 ))
	then	exit $2
	fi
}

function funcheck
{
	[[ $foo == EXIT ]] || err_exit "foo "$@" : exit trap not set"
	if	[[ -f $file ]]
	then	rm -r $file
		err_exit "foo $@: doesn't remove $file"
	fi
	foo=NOVAL bar=NOVAL
}

(exit 0) || err_exit "exit 0 is not zero"
(return 0) || err_exit "return 0 is not zero"
(exit) || err_exit "default exit value is not zero"
(return) || err_exit "default return value is not zero"
(exit 35)
ret=$?
if	(( $ret != 35 ))
then	err_exit "exit 35 is $ret not 35"
fi
(return 35)
ret=$?
if	(( $ret != 35 ))
then	err_exit "return 35 is $ret not 35"
fi

foo 0 0 || err_exit "foo 0 0: incorrect return"
funcheck 0 0
foo 0 3
ret=$?
if	(( $ret != 3 ))
then	err_exit "foo 0 3: return is $ret not 3"
fi
funcheck 0 3
foo 2 0 || err_exit "foo 2 0: incorrect return"
[[ $bar == EXIT ]] || err_exit "foo 2 0: bar exit trap not set"
funcheck 2 0
foo 2 3
ret=$?
if	(( $ret != 3 ))
then	err_exit "foo 2 3: return is $ret not 3"
fi
[[ $bar == EXIT ]] || err_exit "foo 2 3: bar exit trap not set"
funcheck 2 3
(foo 3 3)
ret=$?
if	(( $ret != 3 ))
then	err_exit "foo 3 3: return is $ret not 3"
fi
foo=EXIT
funcheck 3 3
cat > $file <<!
return 3
exit 4
!
( . $file )
ret=$?
if	(( $ret != 3 ))
then	err_exit "return in dot script is $ret should be 3"
fi
chmod 755 $file
(  $file )
ret=$?
if	(( $ret != 3 ))
then	err_exit "return in script is $ret should be 3"
fi
cat > $file <<!
: line 1
# next line should fail and cause an exit
: > /
exit 4
!
( . $file ; exit 5 ) 2> /dev/null
ret=$?
if	(( $ret != 1 ))
then	err_exit "error in dot script is $ret should be 1"
fi
(  $file; exit 5 ) 2> /dev/null
ret=$?
if	(( $ret != 5 ))
then	err_exit "error in script is $ret should be 5"
fi
cat > $file <<\!
print -r -- "$0"
!
x=$( . $file)
if	[[ $x != $0 ]]
then	err_exit "\$0 in a dot script is $x. Should be $0"
fi

if((!SHOPT_SCRIPTONLY));then
x=$($SHELL -i --norc 2> /dev/null <<\!
typeset -i x=1/0
print hello
!
)
if	[[ $x != hello ]]
then	err_exit "interactive shell terminates with error in bltin"
fi
fi # !SHOPT_SCRIPTONLY

x=$( set -e
	false
	print bad
	)
if	[[ $x != '' ]]
then	err_exit "set -e doesn't terminate script on error"
fi
x=$( set -e
	trap 'exit 0' EXIT
	false
	print bad
	)
if	(( $? != 0 ))
then	err_exit "exit 0 in trap should doesn't set exit value to 0"
fi
$SHELL <<\!
trap 'exit 8' EXIT
exit 1
!
if	(( $? != 8 ))
then	err_exit "exit 8 in trap should set exit value to 8"
fi

# ======
# Tests for 'return' and 'exit' without argument: they should pass down the previous exit status
#
# Some of these tests use 'function foo' instead of foo() to locally turn off
# xtrace; otherwise the 2>&1 redirection would write xtrace to standard output.

foo() { return; }
false
foo && err_exit "'return' within function does not preserve exit status"
false
foo & wait "$!" && err_exit "'return' within function does not preserve exit status (bg job)"

foo() { false; return || :; }
foo && err_exit "'return ||' does not preserve exit status"

foo() { false; return && :; }
foo && err_exit "'return &&' does not preserve exit status"

foo() { false; while return; do true; done; }
foo && err_exit "'while return' does not preserve exit status"

function foo { false; while return; do true; done 2>&1; }
foo && err_exit "'while return' with redirection does not preserve exit status"

foo() { false; until return; do true; done; }
foo && err_exit "'until return' does not preserve exit status"

function foo { false; until return; do true; done 2>&1; }
foo && err_exit "'until return' with redirection does not preserve exit status"

foo() { false; for i in 1; do return; done; }
foo && err_exit "'return' within 'for' does not preserve exit status"

function foo { false; for i in 1; do return; done 2>&1; }
foo && err_exit "'return' within 'for' with redirection does not preserve exit status"

function foo { false; { return; } 2>&1; }
foo && err_exit "'return' within { block; } with redirection does not preserve exit status"

# Subshell functions. These have no ksh variant, but we can just turn off xtrace directly in them (set +x).

foo() ( exit )
false
foo && err_exit "'exit' within function does not preserve exit status"
false
foo & wait "$!" && err_exit "'exit' within function does not preserve exit status (bg job)"

foo() ( false; exit || : )
foo && err_exit "'exit ||' does not preserve exit status"

foo() ( false; exit && : )
foo && err_exit "'exit &&' does not preserve exit status"

foo() ( false; while exit; do true; done )
foo && err_exit "'while exit' does not preserve exit status"

foo() ( set +x; false; while exit; do true; done 2>&1 )
foo && err_exit "'while exit' with redirection does not preserve exit status"

foo() ( false; until exit; do true; done )
foo && err_exit "'until exit' does not preserve exit status"

foo() ( set +x; false; until exit; do true; done 2>&1 )
foo && err_exit "'until exit' with redirection does not preserve exit status"

foo() ( false; for i in 1; do exit; done )
foo && err_exit "'exit' within 'for' does not preserve exit status"

foo() ( set +x; false; for i in 1; do exit; done 2>&1 )
foo && err_exit "'exit' within 'for' with redirection does not preserve exit status"

foo() ( set +x; false; { exit; } 2>&1 )
foo && err_exit "'exit' within { block; } with redirection does not preserve exit status"

foo() { false; (exit); }
foo && err_exit "'exit' within subshell does not preserve exit status"

# ======
if	let ".sh.version >= 20211208" && builtin getconf 2>/dev/null
then	max=$(getconf INT_MAX) min=$(getconf INT_MIN) err=$tmp/stderr
	exp=': out of range'
	foo() { return $((max+1)); }
	foo 2>$err
	[[ e=$? -eq 128 && $(<$err) == *"$exp" ]] || err_exit 'return fails to warn for INT_MAX+1' \
		"(expected status 128 and *$(printf %q "$exp"), got status $e and $(printf %q "$(<$err)"))"
	foo() { return $((min-1)); }
	foo 2>$err
	[[ e=$? -eq 128 && $(<$err) == *"$exp" ]] || err_exit 'return fails to warn for INT_MIN-1' \
		"(expected status 128 and *$(printf %q "$exp"), got status $e and $(printf %q "$(<$err)"))"
	foo() { return $max; }
	foo 2>$err
	[[ e=$? -eq max && -z $got ]] || err_exit 'return fails for INT_MAX' \
		"(expected status 128 and '', got status $e and $(printf %q "$(<$err)"))"
	foo() { return $min; }
	foo 2>$err
	[[ e=$? -eq min && -z $got ]] || err_exit 'return fails for INT_MIN' \
		"(expected status 128 and '', got status $e and $(printf %q "$(<$err)"))"
fi

# ======
# old AT&T bug reintroduced in v1.0.8 (commit aea99158)
f() { true; return; }
trap 'f; echo $? >out' USR1
(exit 13)
kill -s USR1 ${.sh.pid}
trap - USR1
unset -f f
[[ $(<out) == 0 ]] || err_exit "default return status in traps is broken (expected 0, got $(<out))"

# ======
print 'print "BUG TRIGGERED"' >eval
chmod +x eval
PATH=$PWD:$PATH
foo() { return -1; }
got=$(eval foo)
[[ -z $got ]] || err_exit "negative return status from 'eval' triggers external 'eval' command (got $(printf %q "$got"))"
PATH=${PATH%"$PWD:"}
unset -f foo

# ======
exit $((Errors<125?Errors:125))

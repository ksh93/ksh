########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1982-2012 AT&T Intellectual Property          #
#          Copyright (c) 2020-2021 Contributors to ksh 93u+m           #
#                      and is licensed under the                       #
#                 Eclipse Public License, Version 1.0                  #
#                    by AT&T Intellectual Property                     #
#                                                                      #
#                A copy of the License is available at                 #
#          http://www.eclipse.org/org/documents/epl-v10.html           #
#         (with md5 checksum b35adb5213ca9657e911e9befb180842)         #
#                                                                      #
#              Information and Software Systems Research               #
#                            AT&T Research                             #
#                           Florham Park NJ                            #
#                                                                      #
#                  David Korn <dgk@research.att.com>                   #
#                                                                      #
########################################################################

. "${SHTESTS_COMMON:-${0%/*}/_common}"

unset n s t
typeset -A SIG
for s in $(kill -l)
do	if	! n=$(kill -l $s 2>/dev/null)
	then	err_exit "'kill -l $s' failed"
	elif	! t=$(kill -l $n 2>/dev/null)
	then	err_exit "'kill -l $n' failed"
	elif	[[ $s == ?(SIG)$t ]]
	then	SIG[${s#SIG}]=1
	elif	! m=$(kill -l $t 2>/dev/null)
	then	err_exit "'kill -l $t' failed"
	elif	[[ $m != $n ]]
	then	err_exit "'kill -l $s' => $n, 'kill -l $n' => $t, kill -l $t => $m -- expected $n"
	fi
done

(
	: disabling xtrace for this test
	set +x --pipefail
	{
		$SHELL 2> out2 <<- \EOF
			g=false
			trap 'print -u2 PIPED; $g && exit 0;g=true' PIPE
			while :
			do 	print hello
			done
		EOF
	} | head > /dev/null
	(( $? == 0)) ||   err_exit "SIGPIPE with wrong error code $?"
	# The below is kind of bogus as the err_exit from a bg job is never counted. But see extra check below.
	[[ $(<out2) == $'PIPED\nPIPED' ]] || err_exit 'SIGPIPE output on standard error is not correct'
) &
cop=$!
{ sleep .4; kill $cop; } 2>/dev/null &
spy=$!
if	wait $cop 2>/dev/null
then	kill $spy 2>/dev/null
else	err_exit "pipe with --pipefail PIPE trap hangs or produced an error"
fi
wait
rm -f out2

actual=$( trap 'print -n got_child' SIGCHLD
	sleep .4 &
	for	((i=0; i < 4; i++))
	do 	sleep .15
		print -n $i
	done)
expect=01got_child23
[[ $actual == "$expect" ]] || err_exit 'SIGCHLD not working' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

# begin standalone SIGINT test generation

cat > tst <<'!'
# shell trap tests
#
#    tst  control script that calls tst-1, must be run by ksh
#  tst-1  calls tst-2
#  tst-2  calls tst-3
#  tst-3  defaults or handles and discards/propagates SIGINT
#
# initial -v option lists script entry and SIGINT delivery
#
# three test options
#
#     d call next script directly, otherwise via $SHELL -c
#     t trap, echo, and kill self on SIGINT, otherwise x or SIGINT default if no x
#     x trap, echo on SIGINT, and tst-3 exit 0, tst-2 exit, otherwise SIGINT default
#     z trap, echo on SIGINT, and tst-3 exit 0, tst-2 exit 0, otherwise SIGINT default
#
# Usage: tst [-v] [-options] shell-to-test ...

# "trap + sig" is an unadvertized extension for this test
# if run from nmake SIGINT is set to SIG_IGN
# this call sets it back to SIG_DFL
# semantics w.r.t. function scope must be worked out before
# making it public
trap + INT

set -o monitor

function gen
{
	typeset o t x d
	for x in - x z
	do	case $x in
		[$1])	for t in - t
			do	case $t in
				[$1])	for d in - d
					do	case $d in
						[$1])	o="$o $x$t$d"
						esac
					done
				esac
			done
		esac
	done
	echo '' $o
}

case $1 in
-v)	v=v; shift ;;
-*v*)	v=v ;;
*)	v= ;;
esac
case $1 in
*' '*)	o=$1; shift ;;
-*)	o=$(gen $1); shift ;;
*)	o=$(gen -txd) ;;
esac
case $# in
0)	set ksh bash ksh88 pdksh ash zsh ;;
esac
for f in $o
do	case $# in
	1)	;;
	*)	echo ;;
	esac
	for sh
	do	if	$sh -c 'exit 0' > /dev/null 2>&1
		then	case $# in
			1)	printf '%3s ' "$f" ;;
			*)	printf '%16s %3s ' "$sh" "$f" ;;
			esac
			$sh tst-1 $v$f $sh > tst.out &
			wait
			echo $(cat tst.out)
		fi
	done
done
case $# in
1)	;;
*)	echo ;;
esac
!
cat > tst-1 <<'!'
exec 2>/dev/null
case $1 in
*v*)	echo 1-main ;;
esac
{
	sleep .2
	case $1 in
	*v*)	echo "SIGINT" ;;
	esac
	kill -s INT 0
} &
case $1 in
*t*)	trap '
		echo 1-intr
		trap - INT
		# omitting the self kill exposes shells that deliver
		# the SIGINT trap but exit 0 for -xt
		# kill -s INT $$
	' INT
	;;
esac
case $1 in
*d*)	tst-2 $1 $2; status=$? ;;
*)	$2 -c "tst-2 $1 $2"; status=$? ;;
esac
printf '1-%04d\n' $status
sleep .2
!
cat > tst-2 <<'!'
case $1 in
*z*)	trap '
		echo 2-intr
		exit 0
	' INT
	;;
*x*)	trap '
		echo 2-intr
		exit
	' INT
	;;
*t*)	trap '
		echo 2-intr
		trap - INT
		kill -s INT $$
	' INT
	;;
esac
case $1 in
*v*)	echo 2-main ;;
esac
case $1 in
*d*)	tst-3 $1 $2; status=$? ;;
*)	$2 -c "tst-3 $1 $2"; status=$? ;;
esac
printf '2-%04d\n' $status
!
cat > tst-3 <<'!'
case $1 in
*[xz]*)	trap '
		sleep .2
		echo 3-intr
		exit 0
	' INT
	;;
*)	trap '
		sleep .2
		echo 3-intr
		trap - INT
		kill -s INT $$
	' INT
	;;
esac
case $1 in
*v*)	echo 3-main ;;
esac
sleep .5
printf '3-%04d\n' $?
!
chmod +x tst tst-?

# end standalone test generation

export PATH=$PATH:
typeset -A expected
expected[---]="3-intr"
expected[--d]="3-intr"
expected[-t-]="3-intr 2-intr 1-intr 1-0258"
expected[-td]="3-intr 2-intr 1-intr 1-0258"
expected[x--]="3-intr 2-intr 1-0000"
expected[x-d]="3-intr 2-intr 1-0000"
expected[xt-]="3-intr 2-intr 1-intr 1-0000"
expected[xtd]="3-intr 2-intr 1-intr 1-0000"
expected[z--]="3-intr 2-intr 1-0000"
expected[z-d]="3-intr 2-intr 1-0000"
expected[zt-]="3-intr 2-intr 1-intr 1-0000"
expected[ztd]="3-intr 2-intr 1-intr 1-0000"

tst $SHELL > tst.got

while	read ops out
do	[[ $out == ${expected[$ops]} ]] || err_exit "interrupt $ops test failed -- expected '${expected[$ops]}', got '$out'"
done < tst.got

if	[[ ${SIG[USR1]} ]]
then	float s=$SECONDS
	exp=SIGUSR1
	got=$(LC_ALL=C $SHELL -c '
		trap "print SIGUSR1 ; exit 0" USR1
		(trap "" USR1 ; exec kill -USR1 $$ & sleep .5)
		print done')
	[[ $got == "$exp" ]] || err_exit 'subshell ignoring signal does not send signal to parent' \
		"(expected '$exp', got '$got')"
	(( (SECONDS-s) < .4 )) && err_exit 'parent does not wait for child to complete before handling signal'
	((s = SECONDS))
	exp=SIGUSR1
	got=$(LC_ALL=C $SHELL -c '
		trap "print SIGUSR1 ; exit 0" USR1
		(trap "exit" USR1 ; exec kill -USR1 $$ & sleep .5)
		print done')
	[[ $got == "$exp" ]] || err_exit 'subshell catching signal does not send signal to parent' \
		"(expected '$exp', got '$got')"
	(( SECONDS-s < .4 )) && err_exit 'parent completes early'
fi

yes() for ((;;)); do print y; done

	for exp in TERM VTALRM PIPE
	do	if	[[ ${SIG[$exp]} ]]
		then	{
				bindate=$(whence -p date) "$SHELL" <<- EOF
				foo() { return 0; }
				trap foo EXIT
				{ sleep .2; kill -$exp \$\$; sleep .3; kill -0 \$\$ && kill -KILL \$\$; } &
				yes |
				while read yes
				do	("\$bindate"; sleep .01)
				done > /dev/null
				EOF
    			} 2>> /dev/null
    			got=$(kill -l $?)
    			[[ $exp == $got ]] || err_exit "kill -$exp \$\$ failed, required termination by signal '$got'"
		fi
	done

SECONDS=0
$SHELL 2> /dev/null -c 'sleep .2 && kill $$ & trap "print done; exit 3" EXIT; (sleep .5); print finished' > $tmp/sig
e=$?
[[ $e == 3 ]] || err_exit "exit status failed -- expected 3, got $e"
x=$(<$tmp/sig)
[[ $x == done ]] || err_exit "output failed -- expected 'done', got '$x'"
(( SECONDS > .35 )) && err_exit "took $SECONDS seconds, expected around .2"

SECONDS=0
$SHELL 2> /dev/null -c 'sleep .2 && kill $$ & trap "print done; exit 3" EXIT; sleep .5; print finished' > $tmp/sig
e=$?
[[ $e == 3 ]] || err_exit "exit status failed -- expected 3, got $e"
x=$(<$tmp/sig)
[[ $x == done ]] || err_exit "output failed -- expected 'done', got '$x'"
(( SECONDS > .35 )) && err_exit "took $SECONDS seconds, expected around .2"

SECONDS=0
{ $SHELL 2> /dev/null -c 'sleep .2 && kill $$ & trap "print done; exit 3" EXIT; (sleep .5); print finished' > $tmp/sig ;} 2> /dev/null
e=$?
[[ $e == 3 ]] || err_exit "exit status failed -- expected 3, got $e"
x=$(<$tmp/sig)
[[ $x == done ]] || err_exit "output failed -- expected 'done', got '$x'"
(( SECONDS > .35 )) && err_exit "took $SECONDS seconds, expected around .2"

SECONDS=0
{ $SHELL 2> /dev/null -c 'sleep .2 && kill $$ & trap "print done; exit 3" EXIT; sleep .5; print finished' > $tmp/sig ;} 2> /dev/null
e=$?
[[ $e == 3 ]] || err_exit "exit status failed -- expected 3, got $e"
x=$(<$tmp/sig)
[[ $x == done ]] || err_exit "output failed -- expected 'done', got '$x'"
(( SECONDS > .35 )) && err_exit "took $SECONDS seconds, expected around .2"

SECONDS=0
x=$($SHELL 2> /dev/null -c 'sleep .2 && kill $$ & trap "print done; exit 3" EXIT; (sleep .5); print finished')
e=$?
[[ $e == 3 ]] || err_exit "exit status failed -- expected 3, got $e"
[[ $x == done ]] || err_exit "output failed -- expected 'done', got '$x'"
(( SECONDS > .35 )) && err_exit "took $SECONDS seconds, expected around .2"

SECONDS=0
x=$($SHELL 2> /dev/null -c 'sleep .2 && kill $$ & trap "print done; exit 3" EXIT; sleep .5; print finished')
e=$?
[[ $e == 3 ]] || err_exit "exit status failed -- expected 3, got $e"
[[ $x == done ]] || err_exit "output failed -- expected 'done', got '$x'"
(( SECONDS > .35 )) && err_exit "took $SECONDS seconds, expected around .2"

trap '' SIGBUS
[[ $($SHELL -c 'trap date SIGBUS; trap -p SIGBUS') ]] && err_exit 'SIGBUS should not have a trap'
trap -- - SIGBUS

{
    x=$(
    $SHELL   <<- \++EOF
	timeout() 
	{
		trap 'trap - TERM; return' TERM
		( sleep $1; kill -TERM $$ ) >/dev/null 2>&1 &
		sleep .3
	}
	timeout .1
	print ok
++EOF
    )
} 2> /dev/null
[[ $x == ok ]] || err_exit 'return without arguments in trap not preserving exit status'

x=$(
    $SHELL  <<- \++EOF
	set -o pipefail
        foobar()
        {
		for ((i=0; i < 10000; i++))
		do	print abcdefghijklmnopqrstuvwxyz
		done | head > /dev/null
        }
        foobar
        print ok
	++EOF
)
[[ $x == ok ]] || err_exit 'SIGPIPE exit status causes PIPE signal to be propagated'

x=$(
    $SHELL <<- \EOF
	trap "print GNAW" URG
	print 1
	( sleep .1 ; kill -URG $$ ; sleep .1 ; print S1 ; )
	print 2
EOF
)
[[ $x == $'1\nS1\nGNAW\n2' ]] || err_exit 'signal ignored in subshell not propagated to parent'

if	[[ ${SIG[RTMIN]} ]]
then	{
	$SHELL <<- \EOF
		trap : RTMIN
		for ((i=0 ; i < 3 ; i++))
		do	sleep .1
			kill -RTMIN $$ 2> /dev/null
		done &
		wait
	EOF
	} 2> /dev/null
	[[ $? == 0 ]] && err_exit 'wait interrupted by caught signal should have non-zero exit status'
	{
	$SHELL <<- \EOF
		for ((i=0 ; i < 3 ; i++))
		do	sleep .1
			kill -RTMIN $$ 2> /dev/null
		done &
		wait
	EOF
	} 2> /dev/null
	[[ $(kill -l $?) == RTMIN ]] || err_exit 'wait interrupted by signal not caught should exit with the value of that signal+256'
fi

# sh.1: "A trap condition that is not caught or ignored by the function causes
# the function to terminate and the condition to be passed on to the caller."
function b
{
	sleep .3
	endb=1
}

function a
{
	trap 'endc=1' TERM
	b
	enda=1
}

{ sleep .1;kill -s TERM $$;}&
unset enda endb endc
a
[[ $endb ]] &&  err_exit 'TERM signal did not kill function b'
[[ $enda == 1 ]] || err_exit 'TERM signal killed function a'
[[ $endc == 1 ]] || err_exit 'TERM trap not triggered in function a'

# ======
# Exit status checks

# Verify that 'exit x' for x > 256 does not make the shell send a signal to itself
"$SHELL" -c 'exit $((256+9))'
let "$? == 256+9" && err_exit 'exit with status > 256 makes shell kill itself'

# Verify that the shell does not kill itself after detecting that a child process is killed by a signal,
# and that a signal still causes the exit status to be set to a value > 128
cat >"$tmp/sigtest.sh" <<\EOF
echo begin
"$1" -c 'kill -9 "$$"'
# this extra comment disables an exec optimization
EOF
expect=$'^begin\n/.*/sigtest.sh: line 2: [1-9][0-9]*: Killed\n[1-9][0-9]{1,2}$'
actual=$(LANG=C "$SHELL" -c '"$1" "$2" "$1"; echo "$?"' x "$SHELL" "$tmp/sigtest.sh" 2>&1)
if	! [[ $actual =~ $expect ]]
then	[[ $actual == *Killed*Killed* ]] && msg='ksh killed itself' || msg='unexpected output'
	err_exit "$msg after child process signal (expected match to $(printf %q "$expect"); got $(printf %q "$actual"))"
fi
let "${actual##*$'\n'} > 128" || err_exit "child process signal did not cause exit status > 128" \
	"(got ${actual##*$'\n'})"

# ======
# Killing a non-existent job shouldn't cause a segfault. Note that `2> /dev/null` has no effect when
# there is a segfault.
$SHELL -c 'kill %% 2> /dev/null'; [[ $? == 1 ]] || err_exit $'`kill` doesn\'t handle a non-existent job correctly when passed \'%%\''
$SHELL -c 'kill %+ 2> /dev/null'; [[ $? == 1 ]] || err_exit $'`kill` doesn\'t handle a non-existent job correctly when passed \'%+\''
$SHELL -c 'kill %- 2> /dev/null'; [[ $? == 1 ]] || err_exit $'`kill` doesn\'t handle a non-existent job correctly when passed \'%-\''

# ======
# SIGINFO should be supported by the kill builtin on platforms that have it.
if "$(whence -p kill)" -INFO $$ 2> /dev/null
then
	kill -INFO $$ || err_exit '`kill` cannot send SIGINFO to processes when passed `-INFO`'
fi

# ======
# Due to an off-by-one error, the last signal in 'kill -l' output wasn't treated properly and could crash.

sig=$(kill -l | tail -n 1)
exp="OK: $sig"

got=$(export sig; "$SHELL" -c '
	trap "print '\''OK: $sig'\''" "$sig"
	(kill -s "$sig" "$$")
	trap - "$sig"
' 2>&1)
((!(e = $?))) && [[ $got == "$exp" ]] || err_exit "failed to handle SIG$sig from subshell" \
	"(got status $e$( ((e>128)) && print -n / && kill -l "$e"), $(printf %q "$got"))"

got=$(export sig; "$SHELL" -c '
	function tryTrap
	{
		kill -s "$1" "$$"
	}
	trap "print '\''OK: $sig'\''" "$sig"
	tryTrap "$sig"
	trap - "$sig"
' 2>&1)
((!(e = $?))) && [[ $got == "$exp" ]] || err_exit "failed to handle SIG$sig from ksh function" \
	"(got status $e$( ((e>128)) && print -n / && kill -l "$e"), $(printf %q "$got"))"

# ======
# ksh-style functions didn't handle signals other than SIGINT and SIGQUIT (rhbz#1454804)
exp="HUP INT PIPE QUIT TERM USR1 USR2"
got=$(export exp; "$SHELL" -c '
	function tryTrap
	{
		kill -s "$1" "$$"
	}
	for sig in $exp			# split
	do	trap "print -n '\''$sig '\''" "$sig"
		tryTrap "$sig"
		trap - "$sig"
	done
' 2>&1)
got=${got% }	# rm final space
((!(e = $?))) && [[ $got == "$exp" ]] || err_exit "ksh function ignores global signal traps" \
	"(got status $e$( ((e>128)) && print -n / && kill -l "$e"), $(printf %q "$got"))"

# ======
exit $((Errors<125?Errors:125))

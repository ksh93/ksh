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

typeset -F SECONDS  # for fractional seconds in PS4

builtin getconf
bincat=$(PATH=$(getconf PATH) whence -p cat)
binecho=$(PATH=$(getconf PATH) whence -p echo)
# make an external 'sleep' command that supports fractional seconds
binsleep=$tmp/.sleep.sh  # hide to exclude from simple wildcard expansion
cat >"$binsleep" <<EOF
#!$SHELL
sleep "\$@"
EOF
chmod +x "$binsleep"

z=()
z.foo=( [one]=hello [two]=(x=3 y=4) [three]=hi)
z.bar[0]=hello
z.bar[2]=world
z.bar[1]=(x=4 y=5)
val='(
	typeset -a bar=(
		[0]=hello
		[2]=world
		[1]=(
			x=4
			y=5
		)
	)
	typeset -A foo=(
		[one]=hello
		[three]=hi
		[two]=(
			x=3
			y=4
		)
	)
)'
[[ $z == "$val" ]] || err_exit 'compound variable with mixed arrays not working'
z.bar[1]=yesyes
[[ ${z.bar[1]} == yesyes ]] || err_exit 'reassign of indexed array compound variable fails'
z.bar[1]=(x=12 y=5)
[[ ${z.bar[1]} == $'(\n\tx=12\n\ty=5\n)' ]] || err_exit 'reassign array simple to compound variable fails'
eval val="$z"
(
	z.foo[three]=good
	[[ ${z.foo[three]} == good ]] || err_exit 'associative array assignment in subshell not working'
)
[[ $z == "$val" ]] || err_exit 'compound variable changes after associative array assignment'
eval val="$z"
(
	z.foo[two]=ok
	[[ ${z.foo[two]} == ok ]] || err_exit 'associative array assignment to compound variable in subshell not working'
	z.bar[1]=yes
	[[ ${z.bar[1]} == yes ]] || err_exit 'indexed array assignment to compound variable in subshell not working'
)
[[ $z == "$val" ]] || err_exit 'compound variable changes after associative array assignment'

x=(
	foo=( qqq=abc rrr=def)
	bar=( zzz=no rst=fed)
)
eval val="$x"
(
	unset x.foo
	[[ ${x.foo.qqq} ]] && err_exit 'x.foo.qqq should be unset'
	x.foo=good
	[[ ${x.foo} == good ]] || err_exit 'x.foo should be good'
)
[[ $x == "$val" ]] || err_exit 'compound variable changes after unset leaves'
unset l
(
	l=( a=1 b="BE" )
)
[[ ${l+foo} != foo ]] || err_exit 'l should be unset'

TEST_notfound=notfound
while	whence $TEST_notfound >/dev/null 2>&1
do	TEST_notfound=notfound-$RANDOM
done


integer BS=1024 nb=64 ss=60 bs no
for bs in $BS 1
do	$SHELL -c '
		{
			sleep '$ss'
			kill -KILL $$
		} &
		set -- $(printf %.'$(($BS*$nb))'c x | dd bs='$bs')
		print ${#1}
		kill $!
	' > $tmp/sub 2>/dev/null
	no=$(<$tmp/sub)
	(( no == (BS * nb) )) || err_exit "shell hangs on command substitution output size >= $BS*$nb with write size $bs -- expected $((BS*nb)), got ${no:-0}"
done
# this time with redirection on the trailing command
for bs in $BS 1
do	$SHELL -c '
		{
			sleep 2
			sleep '$ss'
			kill -KILL $$
		} &
		set -- $(printf %.'$(($BS*$nb))'c x | dd bs='$bs' 2>/dev/null)
		print ${#1}
		kill $!
	' > $tmp/sub 2>/dev/null
	no=$(<$tmp/sub)
	(( no == (BS * nb) )) || err_exit "shell hangs on command substitution output size >= $BS*$nb with write size $bs and trailing redirection -- expected $((BS*nb)), got ${no:-0}"
done

# exercise command substitution trailing newline logic w.r.t. pipe vs. tmp file io

set -- \
	'post-line print'								\
	'$TEST_unset; ($TEST_fork; print 1); print'					\
	1										\
	'pre-line print'								\
	'$TEST_unset; ($TEST_fork; print); print 1'					\
	$'\n1'										\
	'multiple pre-line print'							\
	'$TEST_unset; ($TEST_fork; print); print; ($TEST_fork; print 1); print'		\
	$'\n\n1'									\
	'multiple post-line print'							\
	'$TEST_unset; ($TEST_fork; print 1); print; ($TEST_fork; print); print'		\
	1										\
	'intermediate print'								\
	'$TEST_unset; ($TEST_fork; print 1); print; ($TEST_fork; print 2); print'	\
	$'1\n\n2'									\
	'simple variable'								\
	'$TEST_unset; ($TEST_fork; l=2; print "$l"); print $l'				\
	2										\
	'compound variable'								\
	'$TEST_unset; ($TEST_fork; l=(a=2 b="BE"); print "$l"); print $l'		\
	$'(\n\ta=2\n\tb=BE\n)'								\

export TEST_fork TEST_unset

while	(( $# >= 3 ))
do	txt=$1
	cmd=$2
	exp=$3
	shift 3
	for TEST_unset in '' 'unset var'
	do	for TEST_fork in '' 'ulimit -c 0'
		do	for TEST_shell in "eval" "$SHELL -c"
			do	if	! got=$($TEST_shell "$cmd")
				then	err_exit "${TEST_shell/*-c/\$SHELL -c} ${TEST_unset:+unset }${TEST_fork:+fork }$txt print failed"
				elif	[[ "$got" != "$exp" ]]
				then	EXP=$(printf %q "$exp")
					GOT=$(printf %q "$got")
					err_exit "${TEST_shell/*-c/\$SHELL -c} ${TEST_unset:+unset }${TEST_fork:+fork }$txt command substitution failed -- expected $EXP, got $GOT"
				fi
			done
		done
	done
done

r=$( ($SHELL -c '
	{
		sleep 32
		kill -KILL $$
	} &
	for v in $(set | sed "s/=.*//")
	do	command unset $v
	done
	typeset -Z5 I
	for ((I = 0; I < 1024; I++))
	do	eval A$I=1234567890
	done
	a=$(set 2>&1)
	print ok
	kill -KILL $!
') 2>/dev/null)
[[ $r == ok ]] || err_exit "large subshell command substitution hangs"

for TEST_command in '' $TEST_notfound
do	for TEST_exec in '' 'exec'
	do	for TEST_fork in '' 'ulimit -c 0;'
		do	for TEST_redirect in '' '>/dev/null'
			do	for TEST_substitute in '' ': $'
				do

	TEST_test="$TEST_substitute($TEST_fork $TEST_exec $TEST_command $TEST_redirect)"
	[[ $TEST_test == '('*([[:space:]])')' ]] && continue
	r=$($SHELL -c '
		{
			sleep 2
			kill -KILL $$
		} &
		{ '"$TEST_test"'; } 2>/dev/null
		kill $!
		print ok
		')
	[[ $r == ok ]] || err_exit "shell hangs on $TEST_test"

				done
			done
		done
	done
done

$SHELL -c '( autoload xxxxx);print -n' ||  err_exit 'autoloaded functions in subshells can cause failure'
foo=$($SHELL  <<- ++EOF++
	(trap 'print bar' EXIT;print -n foo)
	++EOF++
)
[[ $foo == foobar ]] || err_exit 'trap on exit when last commands is subshell is not triggered'

err=$(
	ulimit -n 1024
	$SHELL  2>&1  <<- \EOF
	        date=$(whence -p date)
	        function foo
	        {
	                x=$( $date > /dev/null 2>&1 ;:)
	        }
		# consume almost all fds to push the test to the fd limit #
		integer max=$(ulimit --nofile)
		(( max -= 6 ))
		for ((i=20; i < max; i++))
		do	exec {i}>&1
		done
	        for ((i=0; i < 20; i++))
	        do      y=$(foo)
	        done
	EOF
) || {
	err=${err%%$'\n'*}
	err=${err#*:}
	err=${err##[[:space:]]}
	err_exit "nested command substitution with redirections failed -- $err"
}

exp=0
$SHELL -c $'
	function foobar
	{
		print "hello world"
	}
	[[ $(getopts \'[+?X\ffoobar\fX]\' v --man 2>&1) == *"Xhello worldX"* ]]
	exit '$exp$'
'
got=$?
[[ $got == $exp ]] || err_exit "getopts --man runtime callout with nonzero exit terminates shell -- expected '$exp', got '$got'"
exp=ok
got=$($SHELL -c $'
	function foobar
	{
		print "hello world"
	}
	[[ $(getopts \'[+?X\ffoobar\fX]\' v --man 2>&1) == *"Xhello worldX"* ]]
	print '$exp$'
')
[[ $got == $exp ]] || err_exit "getopts --man runtime callout with nonzero exit terminates shell -- expected '$exp', got '$got'"

# command substitution variations #
set -- \
	'$('			')'		\
	'${ '			'; }'		\
	'$(ulimit -c 0; '	')'		\
	'$( ('			') )'		\
	'${ ('			'); }'		\
	'`'			'`'		\
	'`('			')`'		\
	'`ulimit -c 0; '	'`'		\
	# end of table #
exp=ok
testcase[1]='
	if	%sexpr "NOMATCH" : ".*Z" >/dev/null%s
	then	print error
	else	print ok
	fi
	exit %s
'
testcase[2]='
	function bar
	{
		pipeout=%1$sprintf Ok | tr O o%2$s
		print $pipeout
		return 0
	}
	foo=%1$sbar%2$s || foo="exit status $?"
	print $foo
	exit %3$s
'
while	(( $# >= 2 ))
do	for ((TEST=1; TEST<=${#testcase[@]}; TEST++))
	do	body=${testcase[TEST]}
		for code in 0 2
		do	got=${ printf "$body" "$1" "$2" "$code" | $SHELL 2>&1 }
			status=$?
			if	(( status != code ))
			then	err_exit "test $TEST '$1...$2 exit $code' failed -- exit status $status, expected $code"
			elif	[[ $got != $exp ]]
			then	err_exit "test $TEST '$1...$2 exit $code' failed -- got '$got', expected '$exp'"
			fi
		done
	done
	shift 2
done

# Regression introduced in 93v- beta; let's make sure not to backport it to 93u+m
# Ref.: https://github.com/att/ast/issues/478
expect='foo=bar'
actual=$(
	bin_echo=${ whence -p echo; } || bin_echo=echo
	foo=$(print `"$bin_echo" bar`) # should print nothing
	print foo=$foo                 # should print "foo=bar"
)
[[ $actual == "$expect" ]] \
|| err_exit 'Backticks nested in $( ) result in misdirected output' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

# the next tests loop on all combinations of
#	{ SUB CAT INS TST APP } X { file-sizes }
# where the file size starts at 1Ki and doubles up to and including 1Mi
#
# the tests and timeouts are done in async subshells to prevent
# the test harness from hanging

SUB=(
	( BEG='$( '	END=' )'	)
	( BEG='${ '	END='; }'	)
)
CAT=(  cat  $bincat  )
INS=(  ""  "builtin cat; "  "builtin -d cat $bincat; "  ": > /dev/null; "  )
APP=(  ""  "; :"  )
TST=(
	( CMD='print foo | $cat'			EXP=3		)
	( CMD='$cat < $tmp/lin'						)
	( CMD='cat $tmp/lin | $cat'					)
	( CMD='read v < $tmp/buf; print $v'		LIM=4*1024	)
	( CMD='cat $tmp/buf | read v; print $v'		LIM=4*1024	)
)

if	cat /dev/fd/3 3</dev/null >/dev/null 2>&1 || whence mkfifo > /dev/null
then	T=${#TST[@]}
	TST[T].CMD='$cat <(print foo)'
	TST[T].EXP=3
fi

# prime the two data files to 512 bytes each
# $tmp/lin has newlines every 16 bytes and $tmp/buf has no newlines
# the outer loop doubles the file size at top

buf=$'1234567890abcdef'
lin=$'\n1234567890abcde'
for ((i=0; i<5; i++))
do	buf=$buf$buf
	lin=$lin$lin
done
print -n "$buf" > $tmp/buf
print -n "$lin" > $tmp/lin

unset SKIP
for ((n=1024; n<=1024*1024; n*=2))
do	cat $tmp/buf $tmp/buf > $tmp/tmp
	mv $tmp/tmp $tmp/buf
	cat $tmp/lin $tmp/lin > $tmp/tmp
	mv $tmp/tmp $tmp/lin
	for ((S=0; S<${#SUB[@]}; S++))
	do	for ((C=0; C<${#CAT[@]}; C++))
		do	cat=${CAT[C]}
			for ((I=0; I<${#INS[@]}; I++))
			do	for ((A=0; A<${#APP[@]}; A++))
				do	for ((T=0; T<${#TST[@]}; T++))
					do	#undent...#

	if	[[ ! ${SKIP[S][C][I][A][T]} ]]
	then	eval "{ x=${SUB[S].BEG}${INS[I]}${TST[T].CMD}${APP[A]}${SUB[S].END}; print \${#x}; } >\$tmp/out &"
		m=$!
		{ sleep 4; kill -9 $m; } &
		k=$!
		wait $m
		h=$?
		kill -9 $k
		{ wait $k; } 2>/dev/null  # suppress 'wait: $pid: Killed'
		got=$(<$tmp/out)
		if	[[ ! $got ]] && (( h ))
		then	got=HUNG
		fi
		if	[[ ${TST[T].EXP} ]]
		then	exp=${TST[T].EXP}
		else	exp=$n
		fi
		if	[[ $got != $exp ]]
		then	# on failure skip similar tests on larger files sizes #
			SKIP[S][C][I][A][T]=1
			siz=$(printf $'%#i' $exp)
			cmd=${TST[T].CMD//\$cat/$cat}
			cmd=${cmd//\$tmp\/buf/$siz.buf}
			cmd=${cmd//\$tmp\/lin/$siz.lin}
			err_exit "'x=${SUB[S].BEG}${INS[I]}${cmd}${APP[A]}${SUB[S].END} && print \${#x}' failed -- expected '$exp', got '$got'"
		elif	[[ ${TST[T].EXP} ]] || (( TST[T].LIM >= n ))
		then	SKIP[S][C][I][A][T]=1
		fi
	fi

						#...indent#
					done
				done
			done
		done
	done
done

# specifics -- there's more?

{
	cmd='{ exec 5>/dev/null; print "$(set +x; eval ls -d . 2>&1 1>&5)"; } >$tmp/out &'
	eval $cmd
	m=$!
	{ sleep 4; kill -9 $m; } &
	k=$!
	wait $m
	h=$?
	kill -9 $k
	wait $k
	got=$(<$tmp/out)
} 2>/dev/null
exp=''
if	[[ ! $got ]] && (( h ))
then	got=HUNG
fi
if	[[ $got != $exp ]]
then	err_exit "eval '$cmd' failed -- expected '$exp', got '$got'"
fi

float t1=$SECONDS

$SHELL -c '( "$1" 5 </dev/null >/dev/null 2>&1 & );exit 0' x "$binsleep" | cat 
(( (SECONDS-t1) > 4 )) && err_exit '/bin/sleep& in subshell hanging'
((t1=SECONDS))

$SHELL -c '( sleep 5 </dev/null >/dev/null 2>&1 & );exit 0' | cat 
(( (SECONDS-t1) > 4 )) && err_exit 'sleep& in subshell hanging'

exp=HOME=$HOME
( HOME=/bin/sh )
got=$(env | grep ^HOME=)
[[ $got == "$exp" ]] ||  err_exit "( HOME=/bin/sh ) cleanup failed -- expected '$exp', got '$got'"

cmd='echo $((case x in x)echo ok;esac);:)'
exp=ok
got=$($SHELL -c "$cmd" 2>&1)
[[ $got == "$exp" ]] ||  err_exit "'$cmd' failed -- expected '$exp', got '$got'"

cmd='eval '\''for i in 1 2; do eval "\"\$binecho\" x"; done'\'
exp=$'x\nx'
got=$(export binecho; $SHELL -c "$cmd")
if	[[ $got != "$exp" ]]
then	EXP=$(printf %q "$exp")
	GOT=$(printf %q "$got")
	err_exit "'$cmd' failed -- expected $EXP, got $GOT"
fi

("$SHELL" -c '
	sleep 20 & pid=$!
	{
		x=$( ( "$SHELL" -c "integer i; for((i=1;i<=60000;i++)); do print \$i; done" ) )
		kill -9 $pid
	} &
	wait $pid
') 2> /dev/null
(( $? )) ||  err_exit 'nested command substitution with large output hangs'

(.sh.foo=foobar)
[[ ${.sh.foo} == foobar ]] && err_exit '.sh subvariables in subshells remain set'
[[ $(export bincat; $SHELL -c 'print 1 | : "$("$bincat" <("$bincat"))"') ]] && err_exit 'process substitution not working correctly in subshells'

# config hang bug
integer i
for ((i=1; i < 1000; i++))
do	typeset foo$i=$i
done
{
    : $( (ac_space=' '; set | grep ac_space) 2>&1) 
} < /dev/null | cat > /dev/null &
sleep  1.5
if	kill -KILL $! 2> /dev/null
then	err_exit 'process timed out with hung comsub'
fi
wait $! 2> /dev/null
(( $? > 128 )) && err_exit 'incorrect exit status with comsub' 

$SHELL 2> /dev/null -c '[[ ${ print foo },${ print bar } == foo,bar ]]' || err_exit  '${ print foo },${ print bar } not working'
$SHELL 2> /dev/null -c '[[ ${ print foo; },${ print bar } == foo,bar ]]' || err_exit  '${ print foo; },${ print bar } not working'

src=$'true 2>&1\n: $(true | true)\n: $(true | true)\n: $(true | true)\n'$(whence -p true)
exp=ok
got=$( $SHELL -c "(eval '$src'); echo $exp" )
[[ $got == "$exp" ]] || err_exit 'subshell eval of pipeline clobbers stdout'

x=$( { time $SHELL -c date >| /dev/null;} 2>&1)
[[ $x == *real*user*sys* ]] || err_exit 'time { ...;} 2>&1 in $(...) fails'

x=$($SHELL -c '( function fx { export X=123;  } ; fx; ); echo $X')
[[ $x == 123 ]] && err_exit 'global variables set from with functions inside a
subshell can leave side effects in parent shell'

date=$(whence -p date)
err() { return $1; }
( err 12 ) & pid=$!
: $( $date)
wait $pid
exit_status=$?
[[ $exit_status == 12 ]] || err_exit "exit status from subshells not being preserved (expected 12, got $exit_status)"

if	cat /dev/fd/3 3</dev/null >/dev/null 2>&1 || whence mkfifo > /dev/null
then	x="$(sed 's/^/Hello /' <(print "Fred" | sort))"
	[[ $x == 'Hello Fred' ]] || err_exit  "process substitution of pipeline in command substitution not working"
fi

{
$SHELL <<- \EOF
	function foo
	{
		integer i
		print -u2 foobar
		for	((i=0; i < 8000; i++))
		do	print abcdefghijk
		done
		print -u2 done
	}
	out=$(eval "foo | cat" 2>&1)
	(( ${#out} == 96011 )) || err_exit "\${#out} is ${#out} should be 96011"
EOF
} & pid=$!
$SHELL -c "{ sleep .4 && kill $pid ;}" 2> /dev/null
(( $? == 0 )) &&  err_exit 'process has hung'

{
x=$( $SHELL  <<- \EOF
	function func1 { typeset IFS; : $(func2); print END ;}
	function func2 { IFS="BAR"; }
	func1
	func1
EOF
)
} 2> /dev/null
[[ $x == $'END\nEND' ]] || err_exit 'bug in save/restore of IFS in subshell'

true=$(whence -p true)
date=$(whence -p date)
tmpf=$tmp/foo
function fun1
{
	$true
	cd - >/dev/null 2>&1
	print -u2 -- "$($date) SUCCESS"
}

print -n $(fun1 2> $tmpf)
[[  $(< $tmpf) == *SUCCESS ]] || err_exit 'standard error output lost with command substitution'


tmpfile=$tmp/foo
cat > $tmpfile <<-\EOF
	$SHELL -c 'function g { IFS= ;};function f { typeset IFS;(g);: $V;};f;f'
	EOF
$SHELL 2> /dev/null "$tmpfile" || err_exit 'IFS in subshell causes core dump'

unset i
if      [[ -d /dev/fd ]]
then    integer i
        for ((i=11; i < 29; i++))
        do      if      ! [[ -r /dev/fd/$i  || -w /dev/fd/$i ]]
                then    a=$($SHELL -c "[[ -r /dev/fd/$i || -w /dev/fd/$i ]]")
                        (( $? )) || err_exit "file descriptor $i not close on exec"
                fi
        done
fi

trap USR1 USR1
trap ERR ERR
[[ $(trap -p USR1) == USR1 ]] || err_exit 'trap -p USR1 in subshell not working'
[[ $(trap -p ERR) == ERR ]] || err_exit 'trap -p ERR in subshell not working'
[[ $(trap -p) == *USR* ]] || err_exit 'trap -p in subshell does not contain USR'
[[ $(trap -p) == *ERR* ]] || err_exit 'trap -p in subshell does not contain ERR'
trap - USR1 ERR

( builtin getconf && PATH=$(getconf PATH)
dot=$(cat <<-EOF
		$(ls -d .)
	EOF
) ) & sleep 1
if      kill -0 $! 2> /dev/null
then    err_exit  'command substitution containing here-doc with command substitution fails'
fi

printf=$(whence -p printf)
[[ $( { trap "echo foobar" EXIT; ( $printf ""); } & wait) == foobar ]] || err_exit  'exit trap not being invoked'

$SHELL 2> /dev/null -c '( PATH=/bin; set -o restricted) ; exit 0'  || err_exit 'restoring PATH when a subshell enables restricted exits not working'

$SHELL <<- \EOF
	print > /dev/null  $( ( dd if=/dev/zero bs=1 count=1 2>/dev/null | ( wc -c) 3>&1 ) 3>&1) &
	pid=$!
	sleep .2
	kill -9 $! 2> /dev/null && err_exit '/dev/zero in command substitution hangs'
	wait $!
EOF

for f in /dev/stdout /dev/fd/1
do	if	"${ whence -p test; }" -e "$f"
	then	$SHELL -c "x=\$(command -p tee $f </dev/null 2>/dev/null)" || err_exit "$f in command substitution fails"
	fi
done

# ======
# Unsetting or redefining functions within subshells

# ...function can be unset in subshell

func() { echo mainfunction; }
(unset -f func; typeset -f func >/dev/null 2>&1) && err_exit 'function fails to be unset in subshell'
v=$(unset -f func; typeset -f func >/dev/null 2>&1) && err_exit 'function fails to be unset in comsub'
v=${ unset -f func 2>&1; } && ! typeset -f func >/dev/null 2>&1 || err_exit 'function unset fails to survive ${ ...; }'

# ...function can be redefined in subshell
func() { echo mainfunction; }
(func() { echo sub; }; [[ ${ func; } == sub ]]) || err_exit 'function fails to be redefined in subshell'
v=$(func() { echo sub; }; func) && [[ $v == sub ]] || err_exit 'function fails to be redefined in comsub'
v=${ { func() { echo sub; }; } 2>&1; } && [[ $(PATH=/dev/null func) == sub ]] \
	|| err_exit 'function redefine fails to survive ${ ...; }'

# ...some more fun from Stéphane Chazelas: https://github.com/att/ast/issues/73#issuecomment-384178095
a=$tmp/morefun.sh
cat >| "$a" <<EOF
true() { echo WRONG; }
(true() { echo ok; } && true && unset -f true && true) || false
# the extra '|| false' avoids optimising out the subshell
EOF
v=$("$SHELL" "$a") && [[ $v == ok ]] || err_exit 'fail: more fun 1'
v=$("$SHELL" -c "$(cat "$a")") && [[ $v == ok ]] || err_exit 'fail: more fun 2'
v=$("$SHELL" -c 'eval "$(cat "$1")"' x "$a") && [[ $v == ok ]] || err_exit "fail: more fun 3"
v=$("$SHELL" -c '. "$1"' x "$a") && [[ $v == ok ]] || err_exit "fail: more fun 4"

# ...multiple levels of subshell
func() { echo mainfunction; }
v=$(
	set +x
	(
		func() { echo sub1; }
		(
			func() { echo sub2; }
			(
				func() { echo sub3; }
				func
				PATH=/dev/null
				dummy=${ dummy=${ dummy=${ dummy=${ unset -f func; }; }; }; };  # test subshare within subshell
				func 2>/dev/null
				(($? == 127)) && echo ok_nonexistent || echo fail_zombie
			)
			func
		)
		func
	) 2>&1
	func 2>&1
)
expect=$'sub3\nok_nonexistent\nsub2\nsub1\nmainfunction'
[[ $v == "$expect" ]] \
|| err_exit "multi-level subshell function failure (expected $(printf %q "$expect"), got $(printf %q "$v"))"

# ... https://github.com/ksh93/ksh/issues/228
fail() {
	echo 'Failure'
}
exp=Success
got=$(
	foo() { true; }			# Define function before forking
	ulimit -t unlimited 2>/dev/null	# Fork the subshell
	unset -f fail
	PATH=/dev/null fail 2>/dev/null || echo "$exp"
)
[[ $got == "$exp" ]] || err_exit 'unset -f fails in forked subshells if a function is defined before forking' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# function set in subshell, unset in subshell of that subshell
exp='*: f: not found'
got=$( f() { echo WRONG; }; ( unset -f f; PATH=/dev/null f 2>&1 ) )
[[ $got == $exp ]] || err_exit 'unset -f fails in sub-subshell on function set in subshell' \
	"(expected match of $(printf %q "$exp"), got $(printf %q "$got"))"

# Functions unset in a subshell shouldn't be detected by type (whence -v)
# https://github.com/ksh93/ksh/pull/287
notafunc() {
	echo 'Failure'
}
exp=Success
got=$(unset -f notafunc; type notafunc 2>/dev/null || echo Success)
[[ $got == "$exp" ]] || err_exit "type/whence -v finds function in virtual subshell after it's unset with 'unset -f'" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# Unsetting or redefining aliases within subshells

# ...alias can be unset in subshell

alias al="echo 'mainalias'"

(unalias al; alias al 2>/dev/null) && err_exit 'alias fails to be unset in subshell'

v=$(unalias al; alias al 2>/dev/null) && err_exit 'alias fails to be unset in comsub'

[[ $(eval 'al') == 'mainalias' ]] || err_exit 'main alias fails to survive unset in subshell(s)'

v=${ eval 'al'; unalias al 2>&1; } && [[ $v == 'mainalias' ]] && ! alias al 2>/dev/null \
|| err_exit 'main shell alias wrongly survives unset within ${ ...; }'

# ...alias can be redefined in subshell

alias al="echo 'mainalias'"

(alias al='echo sub'; [[ $(eval 'al') == sub ]]) || err_exit 'alias fails to be redefined in subshell'

v=$(alias al='echo sub'; eval 'al') && [[ $v == sub ]] || err_exit 'alias fails to be redefined in comsub'

[[ $(eval 'al') == 'mainalias' ]] || err_exit 'main alias fails to survive redefinition in subshell(s)'

v=${ eval 'al'; alias al='echo subshare'; } && [[ $v == 'mainalias' && $(eval 'al') == subshare ]] \
|| err_exit 'alias redefinition fails to survive ${ ...; }'

# Resetting a subshell's hash table should not affect the parent shell
check_hash_table()
{
	[[ -n ${ hash; } ]] || err_\exit $1 "resetting the hash table in a subshell affects the parent shell's hash table"
	# Ensure the hash table isn't empty before the next test is run
	hash -r chmod
}
(hash -r); check_hash_table $LINENO					# err_exit (count me)
(PATH="$PATH"); check_hash_table $LINENO				# err_exit (count me)
(unset PATH); check_hash_table $LINENO					# err_exit (count me)
(nameref PATH_TWO=PATH; unset PATH_TWO); check_hash_table $LINENO	# err_exit (count me)

# Adding a utility to a subshell's hash table should not affect the parent shell
hash -r
(hash ls)
[[ -z ${ hash; } ]] || err_exit "changes to a subshell's hash table affect the parent shell"

# Running a command in a subshell should not affect the parent shell's hash table
hash -r
(ls /dev/null >/dev/null)
[[ -z ${ hash; } ]] || err_exit "command run in subshell added to parent shell's hash table"

# ======
# Variables set in functions inside of a virtual subshell should not affect the
# outside environment. This regression test must be run from the disk.
testvars=$tmp/testvars.sh
cat >| "$testvars" << 'EOF'
c=0
function set_ac { a=1; c=1; }
function set_abc { ( set_ac ; b=1 ) }
set_abc
echo "a=$a b=$b c=$c"
EOF
v=$($SHELL $testvars) && [[ "$v" == "a= b= c=0" ]] || err_exit 'variables set in subshells are not confined to the subshell'

# ======
got=$("$SHELL" -c '
	PATH=/dev/null/1
	(
		echo "DEBUG ${.sh.pid}"
		PATH=/dev/null/2
		readonly PATH
		echo "DEBUG ${.sh.pid}"
	)
	[[ $PATH == /dev/null/1 ]] && echo "DEBUG ${.sh.pid}" || echo "DEBUG -1 == wrong"
	: extra command to disable "-c" exec optimization
' 2>&1) \
&& awk '/^DEBUG/ { pid[NR] = $2; }  END { exit !(pid[1] == pid[2] && pid[2] == pid[3]); }' <<<"$got" \
|| err_exit "setting PATH to a readonly value in a subshell either fails or forks (got $(printf %q "$got"))"

# ======
# Test command substitution with external command in here-document
# https://github.com/ksh93/ksh/issues/104
expect=$'/dev/null\n/dev/null'
actual=$(
	cat <<-EOF
		$(ls /dev/null)
		`ls /dev/null`
	EOF
)
[[ $actual == "$expect" ]] || err_exit 'Command substitution in here-document fails' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

# ...and in pipeline (rhbz#994251)
expect=/dev/null
actual=$(cat /dev/null | "$binecho" `ls /dev/null`)
[[ $actual == "$expect" ]] || err_exit 'Command substitution in pipeline fails (1)' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

# ...and in pipeline again (rhbz#1036802: standard error was misdirected)
expect=$'END2\naaa\nEND1\nEND3'
actual=$(export bincat binecho; "$SHELL" 2>&1 -c \
	'function foo
	{
		"$binecho" hello >/dev/null 2>&1
		"$binecho" aaa | "$bincat"
		echo END1
		echo END2 >&2
	}
	echo "$(foo)" >&2
	echo END3 >&2
	exit')
[[ $actual == "$expect" ]] || err_exit 'Command substitution in pipeline fails (2)' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

# ======
# Crash in job handling code when running backtick-style command substitutions (rhbz#825520)
# The regression sometimes doesn't just crash, but freezes hard, so requires special handling.
cat >$tmp/backtick_crash.ksh <<'EOF'
binfalse=$(whence -p false) || exit
for ((i=0; i<250; i++))
do	test -z `"$binfalse" | "$binfalse" | /dev/null/nothing`
done
EOF
"$SHELL" -i "$tmp/backtick_crash.ksh" 2>/dev/null &	# run test as bg job
test_pid=$!
(sleep 10; kill -s KILL "$test_pid" 2>/dev/null) &	# another bg job to kill frozen test job
sleep_pid=$!
{ wait "$test_pid"; } 2>/dev/null			# get job's exit status, suppressing signal messages
((!(e = $?))) || err_exit "backtick comsub crash/freeze (got status $e$( ((e>128)) && print -n / && kill -l "$e"))"
kill "$sleep_pid" 2>/dev/null

# ======
# Backtick command substitution hangs when filling out pipe buffer (rhbz#1062296)
"$SHELL" -c 'HANG=`dd if=/dev/zero bs=1k count=117 2>/dev/null`' &
test_pid=$!
(sleep 2; kill -s KILL "$test_pid" 2>/dev/null) &
sleep_pid=$!
{ wait "$test_pid"; } 2>/dev/null
((!(e = $?))) || err_exit "backtick comsub hang (got status $e$( ((e>128)) && print -n / && kill -l "$e"))"
kill "$sleep_pid" 2>/dev/null

# Backtick command substitution with pipe hangs when filling out pipe buffer (rhbz#1138751)
"$SHELL" -c 'HANG=`dd if=/dev/zero bs=1k count=117 2>/dev/null | cat`' &
test_pid=$!
(sleep 2; kill -s KILL "$test_pid" 2>/dev/null) &
sleep_pid=$!
{ wait "$test_pid"; } 2>/dev/null
((!(e = $?))) || err_exit "backtick comsub with pipe hangs (got status $e$( ((e>128)) && print -n / && kill -l "$e"))"
kill "$sleep_pid" 2>/dev/null

# ======
# https://bugzilla.redhat.com/1116072

expect='return-value'
function get_value
{
	/dev/null/foo 2>/dev/null	# trigger part 1: external command (even nonexistent)
	exec 3>&-			# trigger part 2: close file descriptor 3
	print return-value		# with the bug, the comsub won't output this
}
actual=$(get_value)
[[ $actual == "$expect" ]] || err_exit "\$(Comsub) failed to return output (expected '$expect', got '$actual')"
actual=${ get_value; }
[[ $actual == "$expect" ]] || err_exit "\${ Comsub; } failed to return output (expected '$expect', got '$actual')"
actual=`get_value`
[[ $actual == "$expect" ]] || err_exit "\`Comsub\` failed to return output (expected '$expect', got '$actual')"

# more tests from https://github.com/oracle/solaris-userland/blob/master/components/ksh93/patches/285-30771135.patch
tmpfile=$tmp/1116072.dummy
touch "$tmpfile"
exp='return value'
function get_value
{
	case=$1
	(( case >= 1 )) && exec 3< $tmpfile
	(( case >= 2 )) && exec 4< $tmpfile
	(( case >= 3 )) && exec 6< $tmpfile
	# To trigger the bug we have to spawn an external command.
	$(whence -p true)
	(( case >= 1 )) && exec 3<&-
	(( case >= 2 )) && exec 4<&-
	(( case >= 3 )) && exec 6<&-
	print "$exp"
}
got=$(get_value 0)
[[ $got == "$exp" ]] || err_exit "failed to capture subshell output when closing fd: case 0"
got=$(get_value 1)
[[ $got == "$exp" ]] || err_exit "failed to capture subshell output when closing fd: case 1"
got=$(get_value 2)
[[ $got == "$exp" ]] || err_exit "failed to capture subshell output when closing fd: case 2"
got=$(get_value 3)
[[ $got == "$exp" ]] || err_exit "failed to capture subshell output when closing fd: case 3"

# ======
# https://bugzilla.redhat.com/1117404

cat >$tmp/crash_rhbz1117404.ksh <<-'EOF'
	trap "" HUP			# trigger part 1: signal ignored in main shell
	for((i=0; i<2500; i++))
	do	(trap ": foo" HUP)	# trigger part 2: any trap (empty or not) on same signal in subshell
	done
EOF
got=$( { "$SHELL" "$tmp/crash_rhbz1117404.ksh"; } 2>&1)
((!(e = $?))) || err_exit 'crash while handling subshell trap' \
	"(got status $e$( ((e>128)) && print -n / && kill -l "$e"), $(printf %q "$got"))"

# ======
# Segmentation fault when using cd in a subshell, when current directory cannot be determined
# https://github.com/ksh93/ksh/issues/153
mkdir "$tmp/deleted"
cd "$tmp/deleted"
tmp=$tmp "$SHELL" -c 'cd /; rmdir "$tmp/deleted"'

exp="subPWD: ${PWD%/deleted}"$'\n'"mainPWD: $PWD"
got=$( { "$SHELL" -c '(subshfn() { bad; }; cd ..; echo "subPWD: $PWD"); typeset -f subshfn; echo "mainPWD: $PWD"'; } 2>&1 )
[[ $got == "$exp" ]] || err_exit "subshell state not restored after 'cd ..' from deleted PWD" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

exp="PWD=$PWD"
got=$( { "$SHELL" -c '(cd /; (cd /)); print -r -- "PWD=$PWD"'; } 2>&1 )
((!(e = $?))) && [[ $got == "$exp" ]] || err_exit 'failed to restore nonexistent PWD on exiting a virtual subshell' \
	"(got status $e$( ((e>128)) && print -n / && kill -l "$e"), $(printf %q "$got"))"
mkdir "$tmp/recreated"
cd "$tmp/recreated"
tmp=$tmp "$SHELL" -c 'cd /; rmdir "$tmp/recreated"; mkdir "$tmp/recreated"'
exp="PWD=$PWD"
got=$( { "$SHELL" -c '(cd /); print -r -- "PWD=$PWD"'; } 2>&1 )
((!(e = $?))) && [[ $got == "$exp" ]] || err_exit 'failed to restore re-created PWD on exiting a virtual subshell' \
	"(got status $e$( ((e>128)) && print -n / && kill -l "$e"), $(printf %q "$got"))"
cd "$tmp"

# ======
# Before 93u+m 2021-02-17, the state of ${ shared-state command substitutions; } leaked out of parent virtual subshells.
# https://github.com/ksh93/ksh/issues/143

v=main
d=${ v=shared; }
[[ $v == shared ]] || err_exit "shared comsub in main shell wrongly scoped"

v=main
(d=${ v=shared; }; [[ $v == shared ]]) || err_exit "shared comsub in subshell wrongly scoped (1)"
[[ $v == main ]] || err_exit "shared comsub leaks out of subshell (1)"

v=main
(v=sub; d=${ v=shared; }; [[ $v == shared ]]) || err_exit "shared comsub in subshell wrongly scoped (2)"
[[ $v == main ]] || err_exit "shared comsub leaks out of subshell (2)"

v=main
(v=sub; (d=${ v=shared; }); [[ $v == sub ]]) || err_exit "shared comsub leaks out of nested subshell"
[[ $v == main ]] || err_exit "shared comsub leaks out of subshell (3)"

v=main
(d=${ d=${ v=shared; }; }; [[ $v == shared ]]) || err_exit "nested shared comsub in subshell wrongly scoped (1)"
[[ $v == main ]] || err_exit "shared comsub leaks out of subshell (4)"

v=main
(v=sub; d=${ d=${ v=shared; }; }; [[ $v == shared ]]) || err_exit "nested shared comsub in subshell wrongly scoped (2)"
[[ $v == main ]] || err_exit "shared comsub leaks out of subshell (5)"

v=main
( (d=${ v=shared; }; [[ $v == shared ]]) ) || err_exit "shared comsub in nested subshell wrongly scoped (1)"
[[ $v == main ]] || err_exit "shared comsub leaks out of subshell (6)"

v=main
(v=sub; (d=${ v=shared; }; [[ $v == shared ]]) ) || err_exit "shared comsub in nested subshell wrongly scoped (2)"
[[ $v == main ]] || err_exit "shared comsub leaks out of subshell (7)"

# ...multiple levels of subshell
v=main
got=$(
	(
		v=sub1
		(
			v=sub2
			(
				v=sub3
				echo $v
				dummy=${ dummy=${ dummy=${ dummy=${ unset v; }; }; }; };  # test subshare within subshell
				[[ -n $v || -v v ]] && echo fail_zombie || echo ok_nonexistent
			)
			echo $v
		)
		echo $v
	)
	echo $v
)
exp=$'sub3\nok_nonexistent\nsub2\nsub1\nmain'
[[ $got == "$exp" ]] || err_exit "multi-level subshell function failure" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# After the memory leak patch for rhbz#982142, this minor regression was introduced:
# if a backtick command substitution expanded an alias, an extra space was inserted.
exp='word'
alias a='print -n wo\'
got=$(eval 'echo "`a`rd"')
unalias a
[[ $got == "$exp" ]] || err_exit 'backtick comsub with alias:' \
	"expected $(printf %q "$exp"), got $(printf %q "$got")"

# ======
# Redirecting standard output for a single command should not cause a subshare to fork
exp='good'
got='bad'
dummy=${ : >&2; got='good'; }
[[ $got == "$exp" ]] || err_exit 'subshare stopped sharing state after command that redirects stdout' \
	"(expected '$exp', got '$got')"

# ======
unset d x
exp='end 1'
got=$(d=${ true & x=1; echo end; }; echo $d $x)
[[ $got == "$exp" ]] || err_exit 'subshare forks when running background job' \
	"(expected '$exp', got '$got')"

# ======
# https://github.com/ksh93/ksh/issues/289
got=$(ulimit -t unlimited 2>/dev/null; (dummy=${ ulimit -t 1; }); ulimit -t)
[[ $got == 1 ]] && err_exit "'ulimit' command run in subshare leaks out of parent virtual subshell"
got=$(_AST_FEATURES="TEST_TMP_VAR - $$" "$SHELL" -c '(d=${ builtin getconf;}); getconf TEST_TMP_VAR' 2>&1)
[[ $got == $$ ]] && err_exit "'builtin' command run in subshare leaks out of parent virtual subshell"
got=$(ulimit -t unlimited 2>/dev/null; (dummy=${ exec true; }); echo ok)
[[ $got == ok ]] || err_exit "'exec' command run in subshare disregards parent virtual subshell"

# ======
# https://github.com/ksh93/ksh/pull/294#discussion_r624627501
exp='this should be run once'
$SHELL -c '( ( : & ) ); echo "this should be run once"' >r624627501.out
sleep .01
got=$(<r624627501.out)
[[ $got == "$exp" ]] || err_exit 'background job optimization within virtual subshell causes program flow corruption' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
exit $((Errors<125?Errors:125))

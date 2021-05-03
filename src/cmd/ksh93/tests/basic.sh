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

bincat=$(whence -p cat)
binecho=$(whence -p echo)
binfalse=$(whence -p false)
# make an external 'sleep' command that supports fractional seconds
binsleep=$tmp/.sleep.sh  # hide to exclude from simple wildcard expansion
cat >"$binsleep" <<EOF
#!$SHELL
sleep "\$@"
EOF
chmod +x "$binsleep"

# test basic file operations like redirection, pipes, file expansion
set -- \
	go+r	0000	\
	go-r	0044	\
	ug=r	0330	\
	go+w	0000	\
	go-w	0022	\
	ug=w	0550	\
	go+x	0000	\
	go-x	0011	\
	ug=x	0660	\
	go-rx	0055	\
	uo-wx	0303	\
	ug-rw	0660	\
	o=	0007
while	(( $# >= 2 ))
do	umask 0
	umask $1
	g=$(umask)
	[[ $g == $2 ]] || err_exit "umask 0; umask $1 failed -- expected $2, got $g"
	shift 2
done
umask u=rwx,go=rx || err_exit "umask u=rws,go=rx failed"
if	[[ $(umask -S) != u=rwx,g=rx,o=rx ]]
then	err_exit 'umask -S incorrect'
fi
um=$(umask -S)
( umask 0777; > foobar )
rm -f foobar
> foobar
[[ -r foobar ]] || err_exit 'umask not being restored after subshell'
umask "$um"
rm -f foobar
# optimizer bug test
> foobar
for i in 1 2
do      print foobar*
        rm -f foobar
done > out
if      [[ "$(<out)"  != "foobar"$'\n'"foobar*" ]]
then    print -u2 "optimizer bug with file expansion"
fi
rm -f out foobar
mkdir dir
if	[[ $(print */) != dir/ ]]
then	err_exit 'file expansion with trailing / not working'
fi
if	[[ $(print *) != dir ]]
then	err_exit 'file expansion with single file not working'
fi
print hi > .foo
if	[[ $(print *) != dir ]]
then	err_exit 'file expansion leading . not working'
fi
date > dat1 || err_exit "date > dat1 failed"
test -r dat1 || err_exit "dat1 is not readable"
x=dat1
cat <$x > dat2 || err_exit "cat < $x > dat2 failed"
cat dat1 dat2 | cat  | cat | cat > dat3 || err_exit "cat pipe failed"
cat > dat4 <<!
$(date)
!
cat dat1 dat2 | cat  | cat | cat > dat5 &
wait $!
set -- dat*
if	(( $# != 5 ))
then	err_exit "dat* matches only $# files"
fi
if	(command > foo\\abc) 2> /dev/null
then	set -- foo*
	if	[[ $1 != 'foo\abc' ]]
	then	err_exit 'foo* does not match foo\abc'
	fi
fi
if ( : > TT* && : > TTfoo ) 2>/dev/null
then	set -- TT*
	if	(( $# < 2 ))
	then	err_exit 'TT* not expanding when file TT* exists'
	fi
fi

cd /dev
cd ~- || err_exit "cd back failed"

cat > $tmp/script <<- !
	#! $SHELL
	print -r -- \$0
!
chmod 755 $tmp/script
if	[[ $($tmp/script) != "$tmp/script" ]]
then	err_exit '$0 not correct for #! script'
fi
bar=foo
eval foo=\$bar
if	[[ $foo != foo ]]
then	err_exit 'eval foo=\$bar not working'
fi
bar='foo=foo\ bar'
eval $bar
if	[[ $foo != 'foo bar' ]]
then	err_exit 'eval foo=\$bar, with bar="foo\ bar" not working'
fi
cd /tmp
cd ../../tmp || err_exit "cd ../../tmp failed"
if	[[ $PWD != /tmp ]]
then	err_exit 'cd ../../tmp is not /tmp'
fi
( sleep .2; cat <<!
foobar
!
) | cat > $tmp/foobar &
wait $!
foobar=$( < $tmp/foobar)
if	[[ $foobar != foobar ]]
then	err_exit "$foobar is not foobar"
fi
{
	print foo
	"$binecho" bar
	print bam
} > $tmp/foobar
if	[[ $( < $tmp/foobar) != $'foo\nbar\nbam' ]]
then	err_exit "output file pointer not shared correctly"
fi
cat > $tmp/foobar <<\!
	print foo
	"$binecho" bar
	print bam
!
chmod +x $tmp/foobar
if	[[ $(export binecho; $tmp/foobar) != $'foo\nbar\nbam' ]]
then	err_exit "script not working"
fi
if	[[ $(export binecho; $tmp/foobar | "$bincat") != $'foo\nbar\nbam' ]]
then	err_exit "script | cat not working"
fi
if	[[ $(export binecho; $tmp/foobar) != $'foo\nbar\nbam' ]]
then	err_exit "output file pointer not shared correctly"
fi
rm -f $tmp/foobar
x=$( (print foo) ; (print bar) )
if	[[ $x != $'foo\nbar' ]]
then	err_exit " ( (print foo);(print bar ) failed"
fi
x=$( ("$binecho" foo) ; (print bar) )
if	[[ $x != $'foo\nbar' ]]
then	err_exit " ( ("$binecho");(print bar ) failed"
fi
x=$( ("$binecho" foo) ; ("$binecho" bar) )
if	[[ $x != $'foo\nbar' ]]
then	err_exit " ( ("$binecho");("$binecho" bar ) failed"
fi
cat > $tmp/script <<\!
if	[[ -p /dev/fd/0 ]]
then	builtin cat
	cat - > /dev/null
	[[ -p /dev/fd/0 ]] && print ok
else	print no
fi
!
chmod +x $tmp/script
case $( (print) | $tmp/script;:) in
ok)	;;
no)	err_exit "[[ -p /dev/fd/0 ]] fails for standard input pipe" ;;
*)	err_exit "builtin replaces standard input pipe" ;;
esac
print 'print $0' > $tmp/script
print ". $tmp/script" > $tmp/scriptx
chmod +x $tmp/scriptx
if	[[ $($tmp/scriptx) != $tmp/scriptx ]]
then	err_exit '$0 not correct for . script'
fi
cd $tmp || { err_exit "cd $tmp failed"; exit 1; }
print ./b > ./a; print ./c > b; print ./d > c; print ./e > d; print "echo \"hello there\"" > e
chmod 755 a b c d e
x=$(./a)
if	[[ $x != "hello there" ]]
then	err_exit "nested scripts failed"
fi
x=$( (./a) | cat)
if	[[ $x != "hello there" ]]
then	err_exit "scripts in subshells fail"
fi
cd ~- || err_exit "cd back failed"
cd "$tmp"
x=$( ("$binecho" foo) 2> /dev/null )
if	[[ $x != foo ]]
then	err_exit "subshell in command substitution fails"
fi
exec 9>& 1
exec 1>&-
x=$(print hello)
if	[[ $x != hello ]]
then	err_exit "command substitution with stdout closed failed"
fi
exec >& 9
x=$(export binecho bincat; cat <<\! | $SHELL
"$binecho" | "$bincat"
"$binecho" hello
!
)
if	[[ $x != $'\n'hello ]]
then	err_exit "$SHELL not working when standard input is a pipe"
fi
x=$( ("$binecho" hello) 2> /dev/null )
if	[[ $x != hello ]]
then	err_exit "subshell in command substitution with 1 closed fails"
fi
cat > $tmp/script <<- \!
read line 2> /dev/null
print done
!
if	[[ $($SHELL $tmp/script <&-) != done ]]
then	err_exit "executing script with 0 closed fails"
fi
trap '' INT
cat > $tmp/script <<- \!
trap 'print bad' INT
kill -s INT $$
print good
!
chmod +x $tmp/script
if	[[ $($SHELL  $tmp/script) != good ]]
then	err_exit "traps ignored by parent not ignored"
fi
trap - INT
cat > $tmp/script <<- \!
read line
"$bincat"
!
if	[[ $(export bincat; $SHELL $tmp/script <<!
one
two
!
)	!= two ]]
then	err_exit "standard input not positioned correctly"
fi
word=$(print $'foo\nbar' | { read line; "$bincat";})
if	[[ $word != bar ]]
then	err_exit "pipe to { read line; $bincat;} not working"
fi
word=$(print $'foo\nbar' | ( read line; "$bincat") )
if	[[ $word != bar ]]
then	err_exit "pipe to ( read line; $bincat) not working"
fi
if	((SHOPT_BRACEPAT)) && [[ $(print x{a,b}y) != 'xay xby' ]]
then	err_exit 'brace expansion not working'
fi
if	[[ $(for i in foo bar
	  do ( tgz=$(print $i)
	  print $tgz)
	  done) != $'foo\nbar' ]]
then	err_exit 'for loop subshell optimizer bug'
fi
unset a1
function optbug
{
	set -A a1  foo bar bam
	integer i
	for ((i=0; i < 3; i++))
	do
		(( ${#a1[@]} < 2 )) && return 0
		set -- "${a1[@]}"
		shift
		set -A a1 -- "$@"
	done
	return 1
}
optbug ||  err_exit 'array size optimization bug'
wait # not running --pipefail which would interfere with subsequent tests
: $(jobs -p) # required to clear jobs for next jobs -p (interactive side effect)
sleep 2 &
pids=$!
if	[[ $(jobs -p) != $! ]]
then	err_exit 'jobs -p not reporting a background job'
fi
sleep 2 &
pids="$pids $!"
foo()
{
	set -- $(jobs -p)
	(( $# == 2 )) || err_exit "$# jobs not reported -- 2 expected"
}
foo
kill $pids

[[ $( (trap 'print alarm' ALRM; sleep .4) & sleep .2; kill -ALRM $!; sleep .2; wait) == alarm ]] || err_exit 'ALRM signal not working'
[[ $($SHELL -c 'trap "" HUP; $SHELL -c "(sleep .2;kill -HUP $$)& sleep .4;print done"') != done ]] && err_exit 'ignored traps not being ignored'
[[ $($SHELL -c 'o=foobar; for x in foo bar; do (o=save);print $o;done' 2> /dev/null ) == $'foobar\nfoobar' ]] || err_exit 'for loop optimization subshell bug'
command exec 3<> /dev/null
if	cat /dev/fd/3 >/dev/null 2>&1  || whence mkfifo > /dev/null
then	[[ $($SHELL -c 'cat <(print foo)' 2> /dev/null) == foo ]] || err_exit 'process substitution not working'
	[[ $($SHELL -c  $'tee >(grep \'1$\' > '$tmp/scriptx$') > /dev/null <<-  \!!!
	line0
	line1
	line2
	!!!
	wait
	cat '$tmp/scriptx 2> /dev/null)  == line1 ]] || err_exit '>() process substitution fails'
	> $tmp/scriptx
	[[ $($SHELL -c  $'
	for i in 1
	do	tee >(grep \'1$\' > '$tmp/scriptx$') > /dev/null  <<-  \!!!
		line0
		line1
		line2
		!!!
	done
	wait
	cat '$tmp/scriptx 2>> /dev/null) == line1 ]] || err_exit '>() process substitution fails in for loop'
	[[ $({ $SHELL -c 'cat <(for i in x y z; do print $i; done)';} 2> /dev/null) == $'x\ny\nz' ]] ||
		err_exit 'process substitution of compound commands not working'
fi
[[ $($SHELL -r 'command -p :' 2>&1) == *restricted* ]]  || err_exit 'command -p not restricted'
print cat >  $tmp/scriptx
chmod +x $tmp/scriptx
[[ $($SHELL -c "print foo | $tmp/scriptx ;:" 2> /dev/null ) == foo ]] || err_exit 'piping into script fails'
[[ $($SHELL -c 'X=1;print -r -- ${X:=$(expr "a(0)" : '"'a*(\([^)]\))')}" 2> /dev/null) == 1 ]] || err_exit 'x=1;${x:=$(..."...")} failure'
[[ $($SHELL -c 'print -r -- ${X:=$(expr "a(0)" : '"'a*(\([^)]\))')}" 2> /dev/null) == 0 ]] || err_exit '${x:=$(..."...")} failure'
if	cat /dev/fd/3 >/dev/null 2>&1  || whence mkfifo > /dev/null
then	[[ $(cat <(print hello) ) == hello ]] || err_exit "process substitution not working outside for or while loop"
	$SHELL -c '[[ $(for i in 1;do cat <(print hello);done ) == hello ]]' 2> /dev/null|| err_exit "process substitution not working in for or while loop"
fi
exec 3> /dev/null
print 'print foo "$@"' > $tmp/scriptx
[[ $( print "($tmp/scriptx bar)" | $SHELL 2>/dev/null) == 'foo bar' ]] || err_exit 'script pipe to shell fails'
print "#! $SHELL" > $tmp/scriptx
print 'print  -- $0' >> $tmp/scriptx
chmod +x $tmp/scriptx
[[ $($tmp/scriptx) == $tmp/scriptx ]] || err_exit  "\$0 is $0 instead of $tmp/scriptx"
cat > $tmp/scriptx <<- \EOF
	myfilter() { x=$(print ok | cat); print  -r -- $SECONDS;}
	set -o pipefail
	sleep .3 | myfilter
EOF
(( $($SHELL $tmp/scriptx) > .2 )) && err_exit 'command substitution causes pipefail option to hang'
exec 3<&-
( typeset -r foo=bar) 2> /dev/null || err_exit 'readonly variables set in a subshell cannot unset'
$SHELL -c 'x=${ print hello;}; [[ $x == hello ]]' 2> /dev/null || err_exit '${ command;} not supported'
$SHELL 2> /dev/null <<- \EOF || err_exit 'multiline ${...} command substitution not supported'
	x=${
		print hello
	}
	[[ $x == hello ]]
EOF
$SHELL 2> /dev/null <<- \EOF || err_exit '${...} command substitution with side effects not supported '
	y=bye
	x=${
		y=hello
		print hello
	}
	[[ $y == $x ]]
EOF
$SHELL   2> /dev/null <<- \EOF || err_exit 'nested ${...} command substitution not supported'
	x=${
		print ${ print hello;} $(print world)
	}
	[[ $x == 'hello world' ]]
EOF
$SHELL   2> /dev/null <<- \EOF || err_exit 'terminating } is not a reserved word with ${ command }'
	x=${	{ print -n } ; print -n hello ; }  ; print ' world' }
	[[ $x == '}hello world' ]]
EOF
$SHELL   2> /dev/null <<- \EOF || err_exit '${ command;}xxx not working'
	f()
	{
		print foo
	}
	[[ ${ f;}bar == foobar ]]
EOF

unset foo
[[ ! ${foo[@]} ]] || err_exit '${foo[@]} is not empty when foo is unset'
[[ ! ${foo[3]} ]] || err_exit '${foo[3]} is not empty when foo is unset'
[[ $(print  "[${ print foo }]") == '[foo]' ]] || err_exit '${...} not working when } is followed by ]'
[[ $(print  "${ print "[${ print foo }]" }") == '[foo]' ]] || err_exit 'nested ${...} not working when } is followed by ]'
unset foo
foo=$(false) > /dev/null && err_exit 'failed command substitution with redirection not returning false'
expected=foreback
got=`print -n fore; (sleep 2;print back)&`
[[ $got == $expected ]] || err_exit "\`\` command substitution background process output error (expected '$expected', got '$got')"
got=$(print -n fore; (sleep .2;print back)&)
[[ $got == $expected ]] || err_exit "\$() command substitution background process output error (expected '$expected', got '$got')"
got=${ print -n fore; (sleep 2;print back)& }
[[ $got == $expected ]] || err_exit "\${} shared-state command substitution background process output error (expected '$expected', got '$got')"
function abc { sleep 2; print back; }
function abcd { abc & }
got=$(print -n fore;abcd)
[[ $got == $expected ]] || err_exit "\$() command substitution background with function process output error (expected '$expected', got '$got')"

for false in false $binfalse
do	x=$($false) && err_exit "x=\$($false) should fail"
	$($false) && err_exit "\$($false) should fail"
	$($false) > /dev/null && err_exit "\$($false) > /dev/null should fail"
done
if	env x-a=y >/dev/null 2>&1
then	[[ $(env 'x-a=y'  $SHELL -c 'env | grep x-a') == *x-a=y* ]] || err_exit 'invalid environment variables not preserved'
fi

set --
false
for i in "$@"; do :; done || err_exit 'empty loop does not reset exit status ("$@")'
false
for i do :; done || err_exit 'empty loop does not reset exit status ("$@" shorthand)'
false
for i in; do :; done || err_exit "empty loop does not reset exit status (empty 'in' list)"

false
case 1 in 1) ;; esac || err_exit "empty case does not reset exit status"
false
case 1 in 2) echo whoa ;; esac || err_exit "non-matching case does not reset exit status"

false
2>&1 || err_exit "lone redirection does not reset exit status"

float s=SECONDS
for i in .1 .2
do      print $i
done | while read sec; do ( "$binsleep" "$sec"; "$binsleep" "$sec") done
((  (SECONDS-s) < .4)) && err_exit '"command | while read...done" finishing too fast'
s=SECONDS
set -o pipefail
for ((i=0; i < 30; i++))
do	print hello
	sleep .01
done |  "$binsleep" .1
(( (SECONDS-s) < .2 )) || err_exit 'early termination not causing broken pipe'

got=$({ trap 'print trap' 0; print -n | "$bincat"; } & wait "$!")
[[ $got == trap ]] || err_exit "trap on exit not correctly triggered (expected 'trap', got $(printf %q "$got"))"

got=$({ trap 'print trap' ERR; print -n | "$binfalse"; } & wait "$!")
[[ $got == trap ]] || err_exit "trap on ERR not correctly triggered (expected 'trap', got $(printf %q "$got"))"

exp=
got=$(
	function fun
	{
		$binfalse && echo FAILED
	}
	: works if this line deleted : |
	fun
	: works if this line deleted :
)
[[ $got == $exp ]] || err_exit "pipe to function with conditional fails -- expected '$exp', got '$got'"
got=$(
	: works if this line deleted : |
	{ $binfalse && echo FAILED; }
	: works if this line deleted :
)
[[ $got == $exp ]] || err_exit "pipe to { ... } with conditional fails -- expected '$exp', got '$got'"

got=$(
	: works if this line deleted : |
	( $binfalse && echo FAILED )
	: works if this line deleted :
)
[[ $got == $exp ]] || err_exit "pipe to ( ... ) with conditional fails -- expected '$exp', got '$got'"

( $SHELL -c 'trap : DEBUG; x=( $foo); exit 0') 2> /dev/null  || err_exit 'trap DEBUG fails'

bintrue=$(whence -p true)
set -o pipefail
float start=$SECONDS end 
for ((i=0; i < 2; i++))
do	print foo
	sleep .15
done | { read; $bintrue; end=$SECONDS ;}
(( (SECONDS-start) < .1 )) && err_exit "pipefail not waiting for pipe to finish"
set +o pipefail
(( (SECONDS-end) > .2 )) &&  err_exit "pipefail causing $bintrue to wait for other end of pipe"


{ env A__z=C+SHLVL $SHELL -c : ;} 2> /dev/null || err_exit "SHLVL with wrong attribute fails"

if [[ $bintrue ]]
then	float t0=SECONDS
	{ time sleep .15 | $bintrue ;} 2> /dev/null
	(( (SECONDS-t0) < .1 )) && err_exit 'time not waiting for pipeline to complete'
fi

cat > $tmp/foo.sh <<- \EOF
	eval "cat > /dev/null  < /dev/null"
	sleep .1
EOF
float sec=SECONDS
. $tmp/foo.sh  | cat > /dev/null
(( (SECONDS-sec) < .07 ))  && err_exit '. script does not restore output redirection with eval'

file=$tmp/foobar
builtin cat
for ((n=0; n < 1000; n++))
do
	> $file
	{ sleep .001;echo $? >$file;} | cat > /dev/null
	if	[[ !  -s $file ]]
	then	err_exit 'output from pipe is lost with pipe to builtin'
		break;
	fi
done

$SHELL -c 'kill -0 123456789123456789123456789' 2> /dev/null && err_exit 'kill not catching process id overflows'

[[ $($SHELL -c '{ cd..; print ok;}' 2> /dev/null) == ok ]] || err_exit 'command name ending in .. causes shell to abort'

$SHELL -xc '$(LD_LIBRARY_PATH=$LD_LIBRARY_PATH exec $SHELL -c :)' > /dev/null 2>&1  || err_exit "ksh -xc '(name=value exec ksh)' fails with err=$?"

$SHELL 2> /dev/null -c $'for i;\ndo :;done' || err_exit 'for i ; <newline> not vaid'

# ======
# Crash on syntax error when dotting/sourcing multiple files
# Ref.: https://www.mail-archive.com/ast-developers@lists.research.att.com/msg01943.html
(
	mkdir "$tmp/dotcrash" || exit
	cd "$tmp/dotcrash" || exit
	cat >functions.ksh <<-EOF
		function f1
		{
			echo "f1"
		}
		function f2
		{
			if	[[ $1 -eq 1 ]]:  # deliberate syntax error
			then	echo "f2"
			fi
		}
	EOF
	cat >sub1.ksh <<-EOF
		. ./functions.ksh
		echo "sub1" >tmp.out
	EOF
	cat >main.ksh <<-EOF
		. ./sub1.ksh
	EOF
	"$SHELL" main.ksh 2>/dev/null
) || err_exit "crash when sourcing multiple files (exit status $?)"

# ======
# The time keyword

# A single '%' after a format specifier should not be a syntax
# error (it should be treated as a literal '%').
expect='0%'
actual=$(
	TIMEFORMAT=$'%0S%'
	redirect 2>&1
	set +x
	time :
)
[[ $actual == "$expect" ]] || err_exit "'%' is not treated literally when placed after a format specifier" \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

# The locale's radix point shouldn't be ignored
us=$(
	LC_ALL='C.UTF-8' # radix point '.'
	TIMEFORMAT='%1U' # catch -1.99 bug as well by getting user time
	redirect 2>&1
	set +x
	time sleep 0
)
eu=$(
	LC_ALL='C_EU.UTF-8' # radix point ','
	TIMEFORMAT='%1U'
	redirect 2>&1
	set +x
	time sleep 0
)
[[ ${us:1:1} == ${eu:1:1} ]] && err_exit "The time keyword ignores the locale's radix point (both are ${eu:1:1})"

# ======
# Expansion of multibyte characters after expansion of single-character names $1..$9, $?, $!, $-, etc.
case ${LC_ALL:-${LC_CTYPE:-${LANG:-}}} in
( *[Uu][Tt][Ff]8* | *[Uu][Tt][Ff]-8* )
	eval 'function exptest { print -r "$1テスト"; print -r "$?テスト" ; print -r "$#テスト"; }'
	eval 'expect=$'\''fooテスト\n0テスト\n1テスト'\'
	actual=$(exptest foo)
	[[ $actual == "$expect" ]] || err_exit 'Corruption of multibyte char following expansion of single-char name' \
		"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
	;;
esac

# ======
# ksh didn't rewrite argv correctly (rhbz#1047506)
# When running a script without a #! hashbang path, ksh attempts to replace argv with the arguments
# of the script. However, fixargs() didn't wipe out the rest of previous arguments after the last
# \0. This caused an erroneous record in /proc/<PID>/cmdline and the output of the ps command.
cd "$tmp"
getPsOutput() {
	# UNIX95=1 makes this work on HP-UX.
	actual=$(UNIX95=1 ps -o args= -p "$1" 2>&1)
	# BSD: setproctitle(3) prepends "ksh:" and ps(1) appends " (ksh)". Remove.
	actual=${actual#'ksh: '}
	actual=${actual%' (ksh)'}
	# Some 'ps' implementations add leading and/or trailing whitespace. Remove.
	while [[ $actual == [[:space:]]* ]]; do actual=${actual#?}; done
	while [[ $actual == *[[:space:]] ]]; do actual=${actual%?}; done
}
getCmdline() {
	if [[ $(uname -s) =~ ^UnixWare$ ]]; then
		# UnixWare's ps does not trust the value stored in argv[0] as a
		# security measure. It is still accessible within cmdline.
		actual=$(< "/proc/${1}/cmdline")
	else
		getPsOutput "$1"
	fi
}
if	[[ ! $(uname -s) =~ ^SunOS$ ]] &&
	getPsOutput "$$" &&
	[[ "$SHELL $0" == "$actual"* ]]  # "$SHELL $0" is how shtests invokes this script
then	expect='./atest 1 2'
	echo 'sleep 10; exit 0' >atest
	chmod 755 atest
	./atest 1 2 &
	getCmdline "$!"
	kill "$!"
	[[ $actual == "$expect" ]] || err_exit "ksh didn't rewrite argv correctly" \
		"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
fi
unset -f getPsOutput getCmdline

# ======
# https://bugzilla.redhat.com/1241013
got=$(eval 'x=$(for i in test; do case $i in test) true;; esac; done)' 2>&1) \
|| err_exit "case in a for loop inside a \$(comsub) caused syntax error (got $(printf %q "$got"))"
got=$(eval 'x=${ for i in test; do case $i in test) true;; esac; done; }' 2>&1) \
|| err_exit "case in a for loop inside a \${ comsub; } caused syntax error (got $(printf %q "$got"))"
got=$(eval 'x=`for i in test; do case $i in test) true;; esac; done`' 2>&1) \
|| err_exit "case in a for loop inside a \`comsub\` caused syntax error (got $(printf %q "$got"))"

# ======
# Various DEBUG trap fixes: https://github.com/ksh93/ksh/issues/155
#			    https://github.com/ksh93/ksh/issues/187

# Redirecting disabled the DEBUG trap
exp=$'LINENO: 4\nfoo\nLINENO: 5\nLINENO: 6\nbar\nLINENO: 7\nbaz'
got=$({ "$SHELL" -c '
	PATH=/dev/null
	trap "echo LINENO: \$LINENO >&1" DEBUG	# 3
	echo foo				# 4
	var=$(echo)				# 5
	echo bar				# 6
	echo baz				# 7
'; } 2>&1)
((!(e = $?))) && [[ $got == "$exp" ]] || err_exit 'Redirection in DEBUG trap corrupts the trap' \
	"(got status $e$( ((e>128)) && print -n / && kill -l "$e"), $(printf %q "$got"))"

# The DEBUG trap crashed when re-trapping inside a subshell
exp=$'trap -- \': main\' EXIT\ntrap -- \': main\' ERR\ntrap -- \': main\' KEYBD\ntrap -- \': main\' DEBUG'
got=$({ "$SHELL" -c '
	PATH=/dev/null
	for sig in EXIT ERR KEYBD DEBUG
	do	trap ": main" $sig
		( trap ": LEAK : $sig" $sig )
		trap
		trap - "$sig"
	done
'; } 2>&1)
((!(e = $?))) && [[ $got == "$exp" ]] || err_exit 'Pseudosignal trap failed when re-trapping in subshell' \
	"(got status $e$( ((e>128)) && print -n / && kill -l "$e"), $(printf %q "$got"))"

# Field splitting broke upon evaluating an unquoted expansion in a DEBUG trap
exp=$'a\nb\nc'
got=$({ "$SHELL" -c '
	PATH=/dev/null
	v=""
	trap ": \$v" DEBUG
	A="a b c"
	set -- $A
	printf "%s\n" "$@"
'; } 2>&1)
((!(e = $?))) && [[ $got == "$exp" ]] || err_exit 'Field splitting broke after executing DEBUG trap' \
	"(got status $e$( ((e>128)) && print -n / && kill -l "$e"), $(printf %q "$got"))"

# The DEBUG trap had side effects on the exit status
trap ':' DEBUG
(exit 123)
(((e=$?)==123)) || err_exit "DEBUG trap run in subshell affects exit status (expected 123, got $e)"
r=$(exit 123)
(((e=$?)==123)) || err_exit "DEBUG trap run in \$(comsub) affects exit status (expected 123, got $e)"
r=`exit 123`
(((e=$?)==123)) || err_exit "DEBUG trap run in \`comsub\` affects exit status (expected 123, got $e)"
trap - DEBUG

# After fixing the previous, the DEBUG trap stopped honouring exit status 2 to skip command execution -- whoops
got=$(
	myfn()
	{
		[[ ${.sh.command} == echo* ]] && return 2
	}
	trap 'myfn' DEBUG
	print end
	echo "Hi, I'm a bug"
)
(((e=$?)==2)) && [[ $got == end ]] || err_exit 'DEBUG trap: next command not skipped upon status 2' \
	"(got status $e, $(printf %q "$got"))"

# The DEBUG trap was incorrectly inherited by subshells
exp=$'Subshell\nDebug 1\nParentshell'
got=$(
	trap 'echo Debug ${.sh.subshell}' DEBUG
	(echo Subshell)
	echo Parentshell
)
trap - DEBUG  # bug compat
[[ $got == "$exp" ]] || err_exit "DEBUG trap inherited by subshell" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# The DEBUG trap was incorrectly inherited by ksh functions
exp=$'Debug 0\nFunctionEnv\nDebug 0\nParentEnv'
got=$(
	function myfn
	{
		echo FunctionEnv
	}
	trap 'echo Debug ${.sh.level}' DEBUG
	myfn
	echo ParentEnv
)
trap - DEBUG  # bug compat
[[ $got == "$exp" ]] || err_exit "DEBUG trap inherited by ksh function" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Make sure the DEBUG trap is still inherited by POSIX functions
exp=$'Debug 0\nDebug 1\nFunction\nDebug 0\nNofunction'
got=$(
	myfn()
	{
		echo Function
	}
	trap 'echo Debug ${.sh.level}' DEBUG
	myfn
	echo Nofunction
)
trap - DEBUG  # bug compat
[[ $got == "$exp" ]] || err_exit "DEBUG trap not inherited by POSIX function" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Make sure the DEBUG trap still exits a POSIX function on exit status 255
# TODO: same test for ksh function with -o functrace, once we add that option
exp=$'one\ntwo'
got=$(
	myfn()
	{
		echo one
		echo two
		echo three
		echo four
	}
	set255()
	{
		return 255
	}
	trap '[[ ${.sh.command} == *three ]] && set255' DEBUG
	myfn
)
trap - DEBUG  # bug compat
[[ $got == "$exp" ]] || err_exit "DEBUG trap did not trigger return from POSIX function on status 255" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# In ksh93v- and ksh2020 EXIT traps don't work in forked subshells
# https://github.com/att/ast/issues/1452
exp="forked subshell EXIT trap okay"
got="$(ulimit -t unlimited 2> /dev/null; trap 'echo forked subshell EXIT trap okay' EXIT)"
[[ $got == $exp ]] || err_exit "EXIT trap did not trigger in forked subshell" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

exp="virtual subshell EXIT trap okay"
got="$(trap 'echo virtual subshell EXIT trap okay' EXIT)"
[[ $got == $exp ]] || err_exit "EXIT trap did not trigger in virtual subshell" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# Regression test for https://github.com/att/ast/issues/1403
for t in {A..Z}; do
	$SHELL -c "trap - $t 2> /dev/null"
	[[ $? == 1 ]] || err_exit "'trap' throws segfault when given invalid signal name '$t'"
done

# ======
# Is an invalid flag handled correctly?
# ksh2020 regression: https://github.com/att/ast/issues/1284
actual=$($SHELL --verson 2>&1)
actual_status=$?
expect='ksh: verson: bad option(s)'
expect_status=2
[[ "$actual" == ${expect}* ]] || err_exit "failed to get version string" \
	"(expected $(printf %q ${expect}*), got $(printf %q "$actual"))"
[[ $actual_status == $expect_status ]] ||
	err_exit "wrong exit status (expected '$expect_status', got '$actual_status')"

# ======
# Test for illegal seek error (ksh93v- regression)
# https://www.mail-archive.com/ast-users@lists.research.att.com/msg00816.html
exp='1
2'
got="$(join <(printf '%d\n' 1 2) <(printf '%d\n' 1 2))"
[[ $exp == $got ]] || err_exit "pipeline fails with illegal seek error" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
exit $((Errors<125?Errors:125))

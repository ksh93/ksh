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

[[ ${.sh.version} == "$KSH_VERSION" ]] || err_exit '.sh.version != KSH_VERSION'
unset ss
[[ ${@ss} ]] && err_exit '${@ss} should be empty string when ss is unset'
[[ ${!ss} == ss ]] ||  err_exit '${!ss} should be ss when ss is unset'
[[ ${#ss} == 0 ]] ||  err_exit '${#ss} should be 0 when ss is unset'

# RANDOM
if	(( RANDOM==RANDOM || $RANDOM==$RANDOM ))
then	err_exit RANDOM variable not working
fi
# When the $RANDOM variable is used in a forked subshell, it shouldn't
# use the same pseudorandom seed as the main shell.
# https://github.com/ksh93/ksh/issues/285
RANDOM=123
function rand_print {
	ulimit -t unlimited 2> /dev/null
	print $RANDOM
}
integer rand1=$(rand_print)
integer rand2=$(rand_print)
(( rand1 == rand2 )) && err_exit "Test 1: \$RANDOM seed in subshell doesn't change" \
	"(both results are $rand1)"
# Make sure we're actually using a different pseudorandom seed
integer rand1=$(
	ulimit -t unlimited 2> /dev/null
	test $RANDOM
	print $RANDOM
)
integer rand2=${ print $RANDOM ;}
(( rand1 == rand2 )) && err_exit "Test 2: \$RANDOM seed in subshell doesn't change" \
	"(both results are $rand1)"
# $RANDOM should be reseeded when the final command is inside of a subshell
rand1=$($SHELL -c 'RANDOM=1; (echo $RANDOM)')
rand2=$($SHELL -c 'RANDOM=1; (echo $RANDOM)')
(( rand1 == rand2 )) && err_exit "Test 3: \$RANDOM seed in subshell doesn't change" \
	"(both results are $rand1)"
# $RANDOM should be reseeded for the ( simple_command & ) optimization
( echo $RANDOM & ) >r1
( echo $RANDOM & ) >r2
integer giveup=0
trap '((giveup++))' USR1
(sleep 2; kill -s USR1 $$) &
while	[[ ! -s r1 || ! -s r2 ]]
do	((giveup)) && break
done
if	((giveup))
then	err_exit "Test 4: ( echo $RANDOM & ) does not write output"
else	[[ $(<r1) == "$(<r2)" ]] && err_exit "Test 4: \$RANDOM seed in ( simple_command & ) doesn't change" \
		"(both results are $(printf %q "$(<r1)"))"
fi
kill $! 2>/dev/null
trap - USR1
unset giveup
# Virtual subshells should not influence the parent shell's RANDOM sequence
RANDOM=456
exp="$RANDOM $RANDOM $RANDOM $RANDOM $RANDOM"
RANDOM=456
got=
for((i=0; i<5; i++))
do	: $( : $RANDOM $RANDOM $RANDOM )
	got+=${got:+ }$RANDOM
done
[[ $got == "$exp" ]] || err_exit 'Using $RANDOM in subshell influences reproducible sequence in parent environment' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# SECONDS
float secElapsed=0.0 secSleep=0.001
let SECONDS=$secElapsed
sleep $secSleep
secElapsed=SECONDS
if	(( secElapsed < secSleep ))
then	err_exit "slept ${secElapsed} seconds instead of ${secSleep}: " \
                 "either 'sleep' or \$SECONDS not working"
fi
unset -v secElapsed secSleep
# _
set abc def
if	[[ $_ != def ]]
then	err_exit _ variable not working
fi
# PWD
if	[[ !  $PWD -ef . ]]
then	err_exit PWD variable failed, not equivalent to .
fi
# PPID
exp=$$
got=${ $SHELL -c 'print $PPID'; }
if	[[ ${ $SHELL -c 'print $PPID'; } != $$ ]]
then	err_exit "PPID variable failed -- expected '$exp', got '$got'"
fi
# OLDPWD
old=$PWD
cd /
if	[[ $OLDPWD != $old ]]
then	err_exit "OLDPWD variable failed -- expected '$old', got '$OLDPWD'"
fi
cd "$old" || err_exit cd failed
# REPLY
read <<-!
	foobar
	!
if	[[ $REPLY != foobar ]]
then	err_exit REPLY variable not working
fi
integer save=$LINENO
# LINENO
LINENO=10
#
#  These lines intentionally left blank
#
if	(( LINENO != 13))
then	err_exit LINENO variable not working
fi
LINENO=save+10
IFS=:
x=a::b::c
if	[[ $x != a::b::c ]]
then	err_exit "word splitting on constants"
fi
set -- $x
if	[[ $# != 5 ]]
then	err_exit ":: doesn't separate null arguments "
fi
set x
if	x$1=0 2> /dev/null
then	err_exit "x\$1=value treated as an assignment"
fi
# check for attributes across subshells
typeset -i x=3
y=1/0
if	( x=y ) 2> /dev/null
then	err_exit "attributes not passed to subshells"
fi
unset x
function x.set
{
	nameref foo=${.sh.name}.save
	foo=${.sh.value}
	.sh.value=$0
}
x=bar
if	[[ $x != x.set ]]
then	err_exit 'x.set does not override assignment'
fi
x.get()
{
	nameref foo=${.sh.name}.save
	.sh.value=$foo
}

if	[[ $x != bar ]]
then	err_exit 'x.get does not work correctly'
fi
typeset +n foo
unset foo
foo=bar
(
	unset foo
	set +u
	if	[[ $foo != '' ]]
	then	err_exit '$foo not null after unset in subsehll'
	fi
)
if	[[ $foo != bar ]]
then	err_exit 'unset foo in subshell produces side effect '
fi
unset foo
if	[[ $( { : ${foo?hi there} ; } 2>&1) != *'hi there' ]]
then	err_exit '${foo?hi there} with foo unset does not print hi there on 2'
fi
x=$0
set foobar
if	[[ ${@:0} != "$x foobar" ]]
then	err_exit '${@:0} not expanding correctly'
fi
set --
if	[[ ${*:0:1} != "$0" ]]
then	err_exit '${@:0} not expanding correctly'
fi
ACCESS=0
function COUNT.set
{
        (( ACCESS++ ))
}
COUNT=0
(( COUNT++ ))
if	(( COUNT != 1 || ACCESS!=2 ))
then	err_exit " set discipline failure COUNT=$COUNT ACCESS=$ACCESS"
fi

save_LANG=$LANG
LANG=C > /dev/null 2>&1
if	[[ $LANG != C ]]
then	err_exit "C locale not working"
fi
LANG=$save_LANG
if	[[ $LANG != "$save_LANG" ]]
then	err_exit "$save_LANG locale not working"
fi

unset -n foo
foo=junk
function foo.get
{
	.sh.value=stuff
	unset -f foo.get
}
if	[[ $foo != stuff ]]
then	err_exit "foo.get discipline not working"
fi
if	[[ $foo != junk ]]
then	err_exit "foo.get discipline not working after unset"
fi
# special variables
set -- 1 2 3 4 5 6 7 8 9 10
sleep 1000 &
if	[[ $(print -r -- ${#10}) != 2 ]]
then	err_exit '${#10}, where ${10}=10 not working'
fi
for i in @ '*' ! '#' - '?' '$'
do	false
	eval foo='$'$i bar='$'{$i}
	if	[[ ${foo} != "${bar}" ]]
	then	err_exit "\$$i not equal to \${$i}"
	fi
	command eval bar='$'{$i%?} 2> /dev/null || err_exit "\${$i%?} gives syntax error"
	if	[[ $i != [@*] && ${foo%?} != "$bar"  ]]
	then	err_exit "\${$i%?} not correct"
	fi
	command eval bar='$'{$i#?} 2> /dev/null || err_exit "\${$i#?} gives syntax error"
	if	[[ $i != [@*] && ${foo#?} != "$bar"  ]]
	then	err_exit "\${$i#?} not correct"
	fi
	command eval foo='$'{$i} bar='$'{#$i} || err_exit "\${#$i} gives syntax error"
	if	[[ $i != @([@*]) && ${#foo} != "$bar" ]]
	then	err_exit "\${#$i} not correct"
	fi
done
kill -s 0 $! || err_exit '$! does not point to latest asynchronous process'
kill $!
unset x
cd /tmp || exit
CDPATH=/
x=$(cd ${tmp#/})
if	[[ $x != $tmp ]]
then	err_exit 'CDPATH does not display new directory'
fi
CDPATH=/:
x=$(cd ${tmp%/*}; cd ${tmp##*/})
if	[[ $x ]]
then	err_exit 'CDPATH displays new directory when not used'
fi
x=$(cd ${tmp#/})
if	[[ $x != $tmp ]]
then	err_exit "CDPATH ${tmp#/} does not display new directory"
fi
cd "$tmp" || exit
TMOUT=100
(TMOUT=20)
if	(( TMOUT !=100 ))
then	err_exit 'setting TMOUT in subshell affects parent'
fi
unset y
function setdisc # var
{
        eval function $1.get'
        {
                .sh.value=good
        }
        '
}
y=bad
setdisc y
if	[[ $y != good ]]
then	err_exit 'setdisc function not working'
fi
integer x=$LINENO
: $'\
'
if	(( LINENO != x+3  ))
then	err_exit '\<newline> gets linenumber count wrong'
fi
set --
set -- "${@-}"
if	(( $# !=1 ))
then	err_exit	'"${@-}" not expanding to null string'
fi
for i in : % + / 3b '**' '***' '@@' '{' '[' '}' !!  '*a' '$foo'
do      (eval : \${"$i"} 2> /dev/null) && err_exit "\${$i} not an syntax error"
done

# ___ begin: IFS tests ___

unset IFS
( IFS='  ' ; read -r a b c <<-!
	x  y z
	!
	if	[[ $b ]]
	then	err_exit 'IFS="  " not causing adjacent space to be null string'
	fi
)
read -r a b c <<-!
x  y z
!
if	[[ $b != y ]]
then	err_exit 'IFS not restored after subshell'
fi

# The next part generates 3428 IFS set/read tests.

unset IFS x
function split
{
	i=$1 s=$2 r=$3
	IFS=': '
	set -- $i
	IFS=' '
	g="[$#]"
	while	:
	do	case $# in
		0)	break ;;
		esac
		g="$g($1)"
		shift
	done
	case "$g" in
	"$s")	;;
	*)	err_exit "IFS=': '; set -- '$i'; expected '$s' got '$g'" ;;
	esac
	print "$i" | IFS=": " read arg rem; g="($arg)($rem)"
	case "$g" in
	"$r")	;;
	*)	err_exit "IFS=': '; read '$i'; expected '$r' got '$g'" ;;
	esac
}
for str in 	\
	'-'	\
	'a'	\
	'- -'	\
	'- a'	\
	'a -'	\
	'a b'	\
	'- - -'	\
	'- - a'	\
	'- a -'	\
	'- a b'	\
	'a - -'	\
	'a - b'	\
	'a b -'	\
	'a b c'
do
	IFS=' '
	set x $str
	shift
	case $# in
	0)	continue ;;
	esac
	f1=$1
	case $f1 in
	'-')	f1='' ;;
	esac
	shift
	case $# in
	0)	for d0 in '' ' '
		do
			for d1 in '' ' ' ':' ' :' ': ' ' : '
			do
				case $f1$d1 in
				'')	split "$d0$f1$d1" "[0]" "()()" ;;
				' ')	;;
				*)	split "$d0$f1$d1" "[1]($f1)" "($f1)()" ;;
				esac
			done
		done
		continue
		;;
	esac
	f2=$1
	case $f2 in
	'-')	f2='' ;;
	esac
	shift
	case $# in
	0)	for d0 in '' ' '
		do
			for d1 in ' ' ':' ' :' ': ' ' : '
			do
				case ' ' in
				$f1$d1|$d1$f2)	continue ;;
				esac
				for d2 in '' ' ' ':' ' :' ': ' ' : '
				do
					case $f2$d2 in
					'')	split "$d0$f1$d1$f2$d2" "[1]($f1)" "($f1)()" ;;
					' ')	;;
					*)	split "$d0$f1$d1$f2$d2" "[2]($f1)($f2)" "($f1)($f2)" ;;
					esac
				done
			done
		done
		continue
		;;
	esac
	f3=$1
	case $f3 in
	'-')	f3='' ;;
	esac
	shift
	case $# in
	0)	for d0 in '' ' '
		do
			for d1 in ':' ' :' ': ' ' : '
			do
				case ' ' in
				$f1$d1|$d1$f2)	continue ;;
				esac
				for d2 in ' ' ':' ' :' ': ' ' : '
				do
					case $f2$d2 in
					' ')	continue ;;
					esac
					case ' ' in
					$f2$d2|$d2$f3)	continue ;;
					esac
					for d3 in '' ' ' ':' ' :' ': ' ' : '
					do
						case $f3$d3 in
						'')	split "$d0$f1$d1$f2$d2$f3$d3" "[2]($f1)($f2)" "($f1)($f2)" ;;
						' ')	;;
						*)	x=$f2$d2$f3$d3
							x=${x#' '}
							x=${x%' '}
							split "$d0$f1$d1$f2$d2$f3$d3" "[3]($f1)($f2)($f3)" "($f1)($x)"
							;;
						esac
					done
				done
			done
		done
		continue
		;;
	esac
done

# BUG_KUNSETIFS: Unsetting IFS fails to activate default default field splitting if two conditions are met:
IFS=''		# condition 1: no split in main shell
: ${foo-}	# at least one expansion is also needed to trigger this
(		# condition 2: subshell (non-forked)
	unset IFS
	v="one two three"
	set -- $v
	let "$# == 3"	# without bug, should be 3
) || err_exit 'IFS fails to be unset in subshell (BUG_KUNSETIFS)'

# Test known BUG_KUNSETIFS workaround (assign to IFS before unset)
IFS= v=
: ${v:=a$'\n'bc$'\t'def\ gh}
case $(unset IFS; set -- $v; print $#) in
4 | 1)	# test if the workaround works whether we've got the bug or not
	v=$(IFS=foobar; unset IFS; set -- $v; print $#)
	[[ $v == 4 ]] || err_exit "BUG_KUNSETIFS workaround fails (expected 4, got $v)" ;;
*)	err_exit 'BUG_KUNSETIFS detection failed'
esac

# Multi-byte characters should work with $IFS
if [[ ${LC_ALL:-${LC_CTYPE:-${LANG:-}}} =~ [Uu][Tt][Ff]-?8 ]]	# The multi-byte tests are pointless without UTF-8
then
	# Test the following characters:
	# Lowercase accented e  (two bytes)
	# Roman sestertius sign (four bytes)
	for delim in é 𐆘; do
		IFS=$delim
		set : :
		expect=:$delim:
		actual=$*
		[[ $actual == "$expect" ]] || err_exit "IFS failed with multi-byte character $delim" \
			"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

		read -r first second third <<< "one${delim}two${delim}three"
		[[ $first == one ]] || err_exit "IFS failed with multi-byte character $delim (expected one, got $first)"
		[[ $second == two ]] || err_exit "IFS failed with multi-byte character $delim (expected two, got $second)"
		[[ $third == three ]] || err_exit "IFS failed with multi-byte character $delim (expected three, got $three)"

		# Ensure subshells don't get corrupted when IFS becomes a multi-byte character
		IFS=$' \t\n'
		expect=$(printf ":$delim:\\ntrap -- 'echo end' EXIT\\nend")
		actual=$(set : :; IFS=$delim; echo "$*"; trap "echo end" EXIT; trap)
		[[ $actual == "$expect" ]] || err_exit "IFS in subshell failed with multi-byte character $delim" \
			"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
	done

	# Multibyte characters with the same initial byte shouldn't be parsed as the same
	# character if they are different. The regression test below tests two characters
	# with the same initial byte (0xC2).
	IFS='£'  # £ = C2 A3
	v='abc§def ghi§jkl'  # § = C2 A7 (same initial byte)
	set -- $v
	v="${#},${1-},${2-},${3-}"
	[[ $v == '1,abc§def ghi§jkl,,' ]] || err_exit "IFS treats £ (C2 A3) and § (C2 A7) as the same character"
fi

# Ensure fallback to first byte if IFS doesn't start with a valid multibyte character
# (however, this test should pass regardless of the locale)
IFS=$'\x[A0]a'
set : :
expect=$':\x[A0]:'
actual=$*
[[ $actual == "$expect" ]] || err_exit "IFS failed with invalid multi-byte character" \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

# ^^^ end: IFS tests ^^^
# restore default split:
unset IFS

if	[[ $( (print ${12345:?}) 2>&1) != *12345* ]]
then	err_exit 'incorrect error message with ${12345?}'
fi
unset foobar
if	[[ $( (print ${foobar:?}) 2>&1) != *foobar* ]]
then	err_exit 'incorrect error message with ${foobar?}'
fi
unset bar
if	[[ $( (print ${bar:?bam}) 2>&1) != *bar*bam* ]]
then	err_exit 'incorrect error message with ${foobar?}'
fi
{ $SHELL -c '
function foo
{
	typeset SECONDS=0
	sleep 0.002
	print $SECONDS
}
x=$(foo)
(( x >= 0.002 && x < 1 ))
'
} 2> /dev/null   || err_exit 'SECONDS not working in function'
cat > $tmp/script <<-\!
	posixfun()
	{
		unset x
	 	nameref x=$1
	 	print  -r -- "$x"
	}
	function fun
	{
	 	nameref x=$1
	 	print  -r -- "$x"
	}
	if	[[ $1 ]]
	then	file=${.sh.file}
	else	print -r -- "${.sh.file}"
	fi
!
chmod +x $tmp/script
. $tmp/script  1
[[ $file == $tmp/script ]] || err_exit ".sh.file not working for dot scripts"
[[ $($SHELL $tmp/script) == $tmp/script ]] || err_exit ".sh.file not working for scripts"
[[ $(posixfun .sh.file) == $tmp/script ]] || err_exit ".sh.file not working for posix functions"
[[ $(fun .sh.file) == $tmp/script ]] || err_exit ".sh.file not working for functions"
[[ $(posixfun .sh.fun) == posixfun ]] || err_exit ".sh.fun not working for posix functions"
[[ $(fun .sh.fun) == fun ]] || err_exit ".sh.fun not working for functions"
[[ $(posixfun .sh.subshell) == 1 ]] || err_exit ".sh.subshell not working for posix functions"
[[ $(fun .sh.subshell) == 1 ]] || err_exit ".sh.subshell not working for functions"
(
    [[ $(posixfun .sh.subshell) == 2 ]]  || err_exit ".sh.subshell not working for posix functions in subshells"
    [[ $(fun .sh.subshell) == 2 ]]  || err_exit ".sh.subshell not working for functions in subshells"
    (( .sh.subshell == 1 )) || err_exit ".sh.subshell not working in a subshell"
)
TIMEFORMAT='this is a test'
[[ $(set +x; { { time :;} 2>&1;}) == "$TIMEFORMAT" ]] || err_exit 'TIMEFORMAT not working'
alias _test_alias=true
: ${.sh.version}
[[ $(alias _test_alias) == *.sh.* ]] && err_exit '.sh. prefixed to alias name'
: ${.sh.version}
[[ $(whence rm) == *.sh.* ]] && err_exit '.sh. prefixed to tracked alias name'
: ${.sh.version}
[[ $(cd /bin;env | grep PWD=) == *.sh.* ]] && err_exit '.sh. prefixed to PWD'
# unset discipline bug fix
dave=dave
function dave.unset
{
    unset dave
}
unset dave
[[ $(typeset +f) == *dave.* ]] && err_exit 'unset discipline not removed'

x=$(
	dave=dave
	function dave.unset
	{
		print dave.unset
	}
)
[[ $x == dave.unset ]] || err_exit 'unset discipline not called with subset completion'

print 'print ${VAR}' > $tmp/script
unset VAR
VAR=new $tmp/script > $tmp/out
got=$(<$tmp/out)
[[ $got == new ]] || err_exit "previously unset environment variable not passed to script, expected 'new', got '$got'"
[[ ! $VAR ]] || err_exit "previously unset environment variable set after script, expected '', got '$VAR'"
unset VAR
VAR=old
VAR=new $tmp/script > $tmp/out
got=$(<$tmp/out)
[[ $got == new ]] || err_exit "environment variable covering local variable not passed to script, expected 'new', got '$got'"
[[ $VAR == old ]] || err_exit "previously set local variable changed after script, expected 'old', got '$VAR'"
unset VAR
export VAR=old
VAR=new $tmp/script > $tmp/out
got=$(<$tmp/out)
[[ $got == new ]] || err_exit "environment variable covering environment variable not passed to script, expected 'new', got '$got'"
[[ $VAR == old ]] || err_exit "previously set environment variable changed after script, expected 'old', got '$VAR'"

(
	unset dave
	function  dave.append
	{
		.sh.value+=$dave
		dave=
	}
	dave=foo; dave+=bar
	[[ $dave == barfoo ]] || exit 2
) 2> /dev/null
case $? in
0)	 ;;
1)	 err_exit 'append discipline not implemented';;
*)	 err_exit 'append discipline not working';;
esac
.sh.foobar=hello
{
	function .sh.foobar.get
	{
		.sh.value=world
	}
} 2> /dev/null || err_exit "cannot add get discipline to .sh.foobar"
[[ ${.sh.foobar} == world ]]  || err_exit 'get discipline for .sh.foobar not working'

[[ -o xtrace ]] && opt_x=-x || opt_x=+x
x='a|b'
IFS='|'
set -- $x
[[ $2 == b ]] || err_exit '$2 should be b after set'
exec 3>&2 2> /dev/null
set -x
( IFS= ) 2> /dev/null
set "$opt_x"
exec 2>&3-
set -- $x
[[ $2 == b ]] || err_exit '$2 should be b after subshell'

: & pid=$!
( : & )
[[ $pid == $! ]] || err_exit '$! value not preserved across subshells'

pid=$!
{ : & } >&2
[[ $pid == $! ]] && err_exit '$! value not updated after bg job in braces+redir'

pid=$!
{ : |& } >&2
[[ $pid == $! ]] && err_exit '$! value not updated after co-process in braces+redir'

unset foo
typeset -A foo
function foo.set
{
	case ${.sh.subscript} in
	bar)	if	((.sh.value > 1 ))
	        then	.sh.value=5
			foo[barrier_hit]=yes
		fi
		;;
	barrier_hit)
		if	[[ ${.sh.value} == yes ]]
		then	foo[barrier_not_hit]=no
		else	foo[barrier_not_hit]=yes
		fi
		;;
	esac
}
foo[barrier_hit]=no
foo[bar]=1
(( foo[bar] == 1 )) || err_exit 'foo[bar] should be 1'
[[ ${foo[barrier_hit]} == no ]] || err_exit 'foo[barrier_hit] should be no'
[[ ${foo[barrier_not_hit]} == yes ]] || err_exit 'foo[barrier_not_hit] should be yes'
foo[barrier_hit]=no
foo[bar]=2
(( foo[bar] == 5 )) || err_exit 'foo[bar] should be 5'
[[ ${foo[barrier_hit]} == yes ]] || err_exit 'foo[barrier_hit] should be yes'
[[ ${foo[barrier_not_hit]} == no ]] || err_exit 'foo[barrier_not_hit] should be no'
unset x
typeset -i x
function x.set
{
	typeset sub=${.sh.subscript}
	(( sub > 0 )) && (( x[sub-1]= x[sub-1] + .sh.value ))
}
x[0]=0 x[1]=1 x[2]=2 x[3]=3
[[ ${x[@]} == '12 8 5 3' ]] || err_exit 'set discipline for indexed array not working correctly'
float seconds
((SECONDS=3*4))
seconds=SECONDS
(( seconds < 12 || seconds > 12.1 )) &&  err_exit "SECONDS is $seconds and should be close to 12"
unset a
function a.set
{
	print -r -- "${.sh.name}=${.sh.value}"
}
[[ $(a=1) == a=1 ]] || err_exit 'set discipline not working in subshell assignment'
[[ $(a=1 :) == a=1 ]] || err_exit 'set discipline not working in subshell command'

[[ ${.sh.subshell} == 0 ]] || err_exit '${.sh.subshell} should be 0'
(
	[[ ${.sh.subshell} == 1 ]] || err_exit '${.sh.subshell} should be 1'
	(
		[[ ${.sh.subshell} == 2 ]] || err_exit '${.sh.subshell} should be 2'
		exit $Errors
	)
)
Errors=$?	# ensure error count survives subshell

actual=$(
	{
		(
			echo ${.sh.subshell} | cat	# left element of pipe should increase ${.sh.subshell}
			echo ${.sh.subshell}
			ulimit -t unlimited 2>/dev/null	# fork
			echo ${.sh.subshell}		# should be same after forking existing virtual subshell
		)
		echo ${.sh.subshell}			# a background job should also increase ${.sh.subshell}
	} & wait "$!"
	echo ${.sh.subshell}
)
expect=$'4\n3\n3\n2\n1'
[[ $actual == "$expect" ]] || err_exit "\${.sh.subshell} failure (expected $(printf %q "$expect"), got $(printf %q "$actual"))"

# ${.sh.subshell} should increment when the final command is inside of a subshell
exp=1
got=$($SHELL -c '(echo ${.sh.subshell})')
[[ $exp == $got ]] || err_exit '${.sh.subshell} fails to increment when the final command is inside of a subshell' \
	"(expected '$exp', got '$got')"

unset IFS
if	((SHOPT_BRACEPAT)) && command set -o braceexpand
then	set -- {1..32768}
else	set -- $(awk 'BEGIN { for(i=1;i<=32768;i++) print i; }')
fi
(( $# == 32768 )) || err_exit "\$# failed -- expected 32768, got $#"
set --

unset r v x
(
	x=foo
	for v in EDITOR VISUAL OPTIND CDPATH FPATH PATH ENV RANDOM SECONDS _ LINENO
	do	nameref r=$v
		unset $v
		if	( $SHELL -c "unset $v; : \$$v" ) 2>/dev/null
		then	[[ $r ]] && err_exit "unset $v failed -- expected '', got '$r'"
			r=$x
			[[ $r == $x ]] || err_exit "$v=$x failed -- expected '$x', got '$r'"
		else	err_exit "unset $v; : \$$v failed"
		fi
	done
	exit $Errors
)
Errors=$?  # ensure error count survives subshell
(
	# $x must be an unknown locale.
	for x in x x.b@d xx_XX xx_XX.b@d
	do	errmsg=$({ LANG=$x; } 2>&1)
		[[ -n $errmsg ]] && break
	done
	if	[[ -z $errmsg ]]
	then	warning "C library does not seem to verify locales: skipping LC_* tests"
		exit $Errors
	fi

	for v in LC_ALL LC_CTYPE LC_MESSAGES LC_COLLATE LC_NUMERIC LC_TIME
	do	nameref r=$v
		unset $v
		[[ $r ]] && err_exit "unset $v failed -- expected '', got '$r'"
		d=$($SHELL -c "$v=$x" 2>&1)
		[[ $d ]] || err_exit "$v=$x failed -- expected locale diagnostic"
		{ g=$( r=$x; print -- $r ); } 2>/dev/null
		[[ $g == '' ]] || err_exit "$v=$x failed -- expected '', got '$g'"
		{ g=$( r=C; r=$x; print -- $r ); } 2>/dev/null
		[[ $g == 'C' ]] || err_exit "$v=C; $v=$x failed -- expected 'C', got '$g'"
	done
	exit $Errors
)
Errors=$?  # ensure error count survives subshell

print print -n zzz > zzz
chmod +x zzz
exp='aaazzz'
got=$($SHELL -c 'unset SHLVL; print -n aaa; ./zzz' 2>&1) >/dev/null 2>&1
[[ $got == "$exp" ]] || err_exit "unset SHLVL causes script failure -- expected '$exp', got '$got'"

mkdir glean
for cmd in date ok
do	exp="$cmd ok"
	rm -f $cmd
	print print $exp > glean/$cmd
	chmod +x glean/$cmd
	got=$(set +x; CDPATH=:.. $SHELL -c "command -p date > /dev/null; cd glean && ./$cmd" 2>&1)
	[[ $got == "$exp" ]] || err_exit "cd with CDPATH after PATH change failed -- expected '$exp', got '$got'"
done

v=LC_CTYPE
unset $v
[[ -v $v ]] && err_exit "unset $v; [[ -v $v ]] failed"
eval $v=C
[[ -v $v ]] || err_exit "$v=C; [[ -v $v ]] failed"

cmd='set --nounset; unset foo; : ${!foo*}'
$SHELL -c "$cmd" 2>/dev/null || err_exit "'$cmd' exit status $?, expected 0"

SHLVL=1
level=$($SHELL -c $'$SHELL -c \'print -r "$SHLVL"\'')
[[ $level  == 3 ]]  || err_exit "SHLVL should be 3 not $level"

[[ $($SHELL -c '{ x=1; : ${x.};print ok;}' 2> /dev/null) == ok ]] || err_exit '${x.} where x is a simple variable causes shell to abort'

$SHELL -c 'unset .sh' 2> /dev/null
[[ $? == 1 ]] || err_exit 'unset .sh should return 1'

#'

# ======
# ${var+set} within a loop.
_test_isset() { eval "
	$1=initial_value
	function _$1_test {
		typeset $1	# make local and initially unset
		for i in 1 2 3 4 5; do
			case \${$1+s} in
			( s )	print -n 'S'; unset -v $1 ;;
			( '' )	print -n 'U'; $1='' ;;
			esac
		done
	}
	_$1_test
	[[ -n \${$1+s} && \${$1} == initial_value ]] || exit
	for i in 1 2 3 4 5; do
		case \${$1+s} in
		( s )	print -n 's'; unset -v $1 ;;
		( '' )	print -n 'u'; $1='' ;;
		esac
	done
"; }
expect='USUSUsusus'
actual=$(_test_isset var)
[[ "$actual" = "$expect" ]] || err_exit "\${var+s} expansion fails in loops (expected '$expect', got '$actual')"
actual=$(_test_isset IFS)
[[ "$actual" = "$expect" ]] || err_exit "\${IFS+s} expansion fails in loops (expected '$expect', got '$actual')"

# [[ -v var ]] within a loop.
_test_v() { eval "
	$1=initial_value
	function _$1_test {
		typeset $1	# make local and initially unset
		for i in 1 2 3 4 5; do
			if	[[ -v $1 ]]
			then	print -n 'S'; unset -v $1
			else	print -n 'U'; $1=''
			fi
		done
	}
	_$1_test
	[[ -v $1 && \${$1} == initial_value ]] || exit
	for i in 1 2 3 4 5; do
		if	[[ -v $1 ]]
		then	print -n 's'; unset -v $1
		else	print -n 'u'; $1=''
		fi
	done
"; }
expect='USUSUsusus'
actual=$(_test_v var)
[[ "$actual" = "$expect" ]] || err_exit "[[ -v var ]] command fails in loops (expected '$expect', got '$actual')"
actual=$(_test_v IFS)
[[ "$actual" = "$expect" ]] || err_exit "[[ -v IFS ]] command fails in loops (expected '$expect', got '$actual')"

# ======
# Verify that importing untrusted environment variables does not allow evaluating
# arbitrary expressions, but does recognize all integer literals recognized by ksh.

expect=8
actual=$(env SHLVL='7' "$SHELL" -c 'echo $SHLVL')
[[ $actual == $expect ]] || err_exit "decimal int literal not recognized (expected '$expect', got '$actual')"

expect=14
actual=$(env SHLVL='013' "$SHELL" -c 'echo $SHLVL')
[[ $actual == $expect ]] || err_exit "leading zeros int literal not recognized (expected '$expect', got '$actual')"

expect=4
actual=$(env SHLVL='2#11' "$SHELL" -c 'echo $SHLVL')
[[ $actual == $expect ]] || err_exit "base#value int literal not recognized (expected '$expect', got '$actual')"

expect=12
actual=$(env SHLVL='16#B' "$SHELL" -c 'echo $SHLVL')
[[ $actual == $expect ]] || err_exit "base#value int literal not recognized (expected '$expect', got '$actual')"

expect=1
actual=$(env SHLVL="2#11+x[\$(env echo Exploited vuln CVE-2019-14868 >&2)0]" "$SHELL" -c 'echo $SHLVL' 2>&1)
[[ $actual == $expect ]] || err_exit "expression allowed on env var import (expected '$expect', got '$actual')"

# ======
# Check unset, attribute and cleanup/restore behavior of special variables.

# Keep the list in sync (minus ".sh") with shtab_variables[] in src/cmd/ksh93/data/variables.c
set -- \
	"PATH" \
	"PS1" \
	"PS2" \
	"IFS" \
	"PWD" \
	"HOME" \
	"MAIL" \
	"REPLY" \
	"SHELL" \
	"EDITOR" \
	"MAILCHECK" \
	"RANDOM" \
	"ENV" \
	"HISTFILE" \
	"HISTSIZE" \
	"HISTEDIT" \
	"HISTCMD" \
	"FCEDIT" \
	"CDPATH" \
	"MAILPATH" \
	"PS3" \
	"OLDPWD" \
	"VISUAL" \
	"COLUMNS" \
	"LINES" \
	"PPID" \
	"_" \
	"TMOUT" \
	"SECONDS" \
	"LINENO" \
	"OPTARG" \
	"OPTIND" \
	"PS4" \
	"FPATH" \
	"LANG" \
	"LC_ALL" \
	"LC_COLLATE" \
	"LC_CTYPE" \
	"LC_MESSAGES" \
	"LC_NUMERIC" \
	"LC_TIME" \
	"FIGNORE" \
	"KSH_VERSION" \
	"JOBMAX" \
	".sh.edchar" \
	".sh.edcol" \
	".sh.edtext" \
	".sh.edmode" \
	".sh.name" \
	".sh.subscript" \
	".sh.value" \
	".sh.version" \
	".sh.dollar" \
	".sh.match" \
	".sh.command" \
	".sh.file" \
	".sh.fun" \
	".sh.lineno" \
	".sh.subshell" \
	".sh.level" \
	".sh.stats" \
	".sh.math" \
	".sh.pool" \
	".sh.pid" \
	".sh.tilde" \
	"SHLVL" \
	"CSWIDTH"

# ... unset
$SHELL -c '
	errors=0
	unset -v "$@" || let errors++
	for var
	do	if	[[ $var != "_" ]] &&	# only makes sense that $_ is immediately set again
			{ [[ -v $var ]] || eval "[[ -n \${$var+s} ]]"; }
		then	echo "	$0: special variable $var still set" >&2
			let errors++
		elif	eval "[[ -n \${$var} ]]"
		then	echo "	$0: special variable $var has value, though unset" >&2
			let errors++
		fi
	done
	exit $((errors + 1))	# a possible erroneous asynchronous fork would cause exit status 0
' unset_test "$@"
(((e = $?) == 1)) || err_exit "Failure in unsetting one or more special variables" \
	"(exit status $e$( ((e>128)) && print -n / && kill -l "$e"))"

# ... unset in virtual subshell inside of nested function
$SHELL -c '
	errors=0
	fun1()
	{
		fun2()
		{
			(
				unset -v "$@" || let errors++
				for var
				do	if	[[ $var != "_" ]] &&	# only makes sense that $_ is immediately set again
						{ [[ -v $var ]] || eval "[[ -n \${$var+s} ]]"; }
					then	echo "	$0: special variable $var still set" >&2
						let errors++
					elif	eval "[[ -n \${$var} ]]"
					then	echo "	$0: special variable $var has value, though unset" >&2
						let errors++
					fi
				done
				exit $errors
			) || errors=$?
		}
		fun2 "$@"
	}
	fun1 "$@"
	exit $((errors + 1))	# a possible erroneous asynchronous fork would cause exit status 0
' unset_subsh_fun_test "$@"
(((e = $?) == 1)) || err_exit "Unset of special variable(s) in a virtual subshell within a nested function fails" \
	"(exit status $e$( ((e>128)) && print -n / && kill -l "$e"))"

# ... readonly in subshell
$SHELL -c '
	errors=0
	(
		readonly "$@"
		for var
		do	if	(eval "$var=") 2>/dev/null
			then	echo "	$0: special variable $var not made readonly in subshell" >&2
				let errors++
			fi
		done
		exit $errors
	) || errors=$?
	for var
	do	if	! (eval "$var=")
		then	echo "	$0: special variable $var still readonly outside subshell" >&2
			let errors++
		fi
	done
	exit $((errors + 1))	# a possible erroneous asynchronous fork would cause exit status 0
' readonly_test "$@"
(((e = $?) == 1)) || err_exit "Failure in making one or more special variables readonly in a subshell" \
	"(exit status $e$( ((e>128)) && print -n / && kill -l "$e"))"

# ... subshell leak test
$SHELL -c '
	errors=0
	for var
	do	if	[[ $var == .sh.level ]]
		then	continue	# known to fail
		fi
		if	eval "($var=bug); [[ \${$var} == bug ]]" 2>/dev/null
		then	echo "	$0: special variable $var leaks out of subshell" >&2
			let errors++
		fi
	done
	exit $((errors + 1))
' subshell_leak_test "$@"
(((e = $?) == 1)) || err_exit "One or more special variables leak out of a subshell" \
	"(exit status $e$( ((e>128)) && print -n / && kill -l "$e"))"

# ... upper/lowercase test
$SHELL -c '
	typeset -u upper
	typeset -l lower
	errors=0
	PS1=/dev/null/test_my_case_too
	PS2=$PS1 PS3=$PS1 PS4=$PS1 OPTARG=$PS1 IFS=$PS1 FPATH=$PS1 FIGNORE=$PS1 CSWIDTH=$PS1
	for var
	do	case $var in
		RANDOM | HISTCMD | _ | SECONDS | LINENO | JOBMAX | .sh.stats)
			# these are expected to fail below as their values change; just test against crashing
			typeset -u "$var"
			typeset -l "$var"
			continue ;;
		esac
		nameref val=$var
		upper=$val
		lower=$val
		typeset -u "$var"
		if	[[ $val != "$upper" ]]
		then	echo "	$0: typeset -u does not work on special variable $var" \
				"(expected $(printf %q "$upper"), got $(printf %q "$val"))" >&2
			let errors++
		fi
		typeset -l "$var"
		if	[[ $val != "$lower" ]]
		then	echo "	$0: typeset -l does not work on special variable $var" \
				"(expected $(printf %q "$lower"), got $(printf %q "$val"))" >&2
			let errors++
		fi
	done
	exit $((errors + 1))
' changecase_test "$@"
(((e = $?) == 1)) || err_exit "typeset -l/-u doesn't work on special variables" \
	"(exit status $e$( ((e>128)) && print -n / && kill -l "$e"))"

# ... unset followed by launching a forked subshell
$SHELL -c '
	errors=0
	unset -v "$@" || let errors++
	(
		ulimit -t unlimited 2>/dev/null
		for var do
			[[ $var == _ ]] && continue	# only makes sense that $_ is immediately set again
			[[ -v $var ]] && let errors++
		done
		exit $((errors + 1))
	)
	exit $?
' unset_to_fork_test "$@"
(((e = $?) == 1)) || err_exit "Failure in unsetting one or more special variables followed by launching forked subshell" \
	"(exit status $e$( ((e>128)) && print -n / && kill -l "$e"))"

# ======
# ${.sh.pid} should be the forked subshell's PID
(
	ulimit -t unlimited 2>/dev/null  # fork the subshell
	[[ ${.sh.pid} == $$ ]]
) && err_exit "\${.sh.pid} is the same as \$$ (both are $$)"

# ${.sh.pid} should be the PID of the running job
echo ${.sh.pid} > "$tmp/jobpid" &
wait
[[ $(cat "$tmp/jobpid") == ${.sh.pid} ]] && err_exit "\${.sh.pid} is not set to a job's PID (expected $!, got $(cat "$tmp/jobpid"))"

# ${.sh.pid} should be the same as $$ in the parent shell
[[ $$ == ${.sh.pid} ]] || err_exit "\${.sh.pid} and \$$ differ in the parent shell (expected $$, got ${.sh.pid})"

# ======
# Parentheses after the '-', '+', '=', and '?' expansion operators were causing syntax errors.
# Check both the unset variable case and the set variable case for each set of symbols.

unset -v foo
for op in - :- = :=
do	for word in '(word)' 'w(or)d' '(wor)d' 'w(ord)' 'w(ord' 'wor)d'
	do	exp=$(set +x; eval "echo \${foo${op}${word}}" 2>&1)
		if	[[ $exp != "$word" ]]
		then	err_exit "\${foo${op}${word}} when foo is not set: expected \"$word\", got \"$exp\""
	        fi
	done
done

foo=some_value
for op in - :- = := \? :\?
do	for word in '(word)' 'w(or)d' '(wor)d' 'w(ord)' 'w(ord' 'wor)d'
	do	exp=$(set +x; eval "echo \${foo${op}${word}}" 2>&1)
		if	[[ $exp != "$foo" ]]
		then	err_exit "\${foo${op}${word}} when foo is set: expected \"$foo\", got \"$exp\""
		fi
	done
done

unset -v foo
for op in + :+
do	for word in '(word)' 'w(or)d' '(wor)d' 'w(ord)' 'w(ord' 'wor)d'
	do	exp=$(set +x; eval "echo \${foo${op}${word}}" 2>&1)
		if	[[ $exp != "" ]]
		then	err_exit "\${foo${op}${word}} when foo is not set: expected null, got \"$exp\""
		fi
	done
done

unset -v foo
for op in \? :\?
do	for word in '(word)' 'w(or)d' '(wor)d' 'w(ord)' 'w(ord' 'wor)d'
	do	exp=$(set +x; eval "echo \${foo${op}${word}}" 2>&1)
		if	[[ $exp != *": foo: $word" ]]
		then	err_exit "\${foo${op}${word}} when foo is not set: expected *\": foo: $word\", got \"$exp\""
		fi
	done
done

# ======
# https://bugzilla.redhat.com/1147645
case $'\n'$(env 'BASH_FUNC_a%%=() { echo test; }' "$SHELL" -c set) in
*$'\nBASH_FUNC_a%%='* )
	err_exit 'ksh imports environment variables with invalid names' ;;
esac

# ======
# Autoloading a function caused $LINENO to be off by the # of lines in the function definition file.
# https://github.com/ksh93/ksh/issues/116

cd "$tmp" || exit 128

cat >lineno_autoload <<'EOF'
echo "begin: main script \$LINENO == $LINENO"
function main_script_fn
{
	lineno_autoload_fn
	(: ${bad\subst\in\main_script_fn\on\line\5})
}
main_script_fn
(eval 'syntax error(')
(: ${bad\subst\in\main\script\on\line\9})
echo "end: main script \$LINENO == $LINENO"
EOF

cat >lineno_autoload_fn <<'EOF'
function lineno_autoload_fn
{
	echo "Hi, I'm a function! On line 3, my \$LINENO is $LINENO"
	(: ${bad\subst\in\function\on\line\4})
	(eval 'syntax error(')
	echo "Hi, I'm still a function! On line 6, my \$LINENO is $LINENO"
}
echo "In definition file, outside function: \$LINENO on line 8 is $LINENO"
: ${bad\subst\in\def\file\on\line\9}
EOF

exp="begin: main script \$LINENO == 1
In definition file, outside function: \$LINENO on line 8 is 8
./lineno_autoload[7]: main_script_fn: line 9: \${bad\subst\in\def\file\on\line\9}: bad substitution
Hi, I'm a function! On line 3, my \$LINENO is 3
./lineno_autoload[7]: main_script_fn[4]: lineno_autoload_fn: line 4: \${bad\subst\in\function\on\line\4}: bad substitution
./lineno_autoload[7]: main_script_fn[4]: lineno_autoload_fn[5]: eval: syntax error at line 1: \`(' unexpected
Hi, I'm still a function! On line 6, my \$LINENO is 6
./lineno_autoload[7]: main_script_fn: line 5: \${bad\subst\in\main_script_fn\on\line\5}: bad substitution
./lineno_autoload[8]: eval: syntax error at line 1: \`(' unexpected
./lineno_autoload: line 9: \${bad\subst\in\main\script\on\line\9}: bad substitution
end: main script \$LINENO == 10"

got=$(set +x; FPATH=$tmp "$SHELL" ./lineno_autoload 2>&1)
[[ $got == "$exp" ]] || err_exit 'Regression in $LINENO and/or error messages.' \
	$'Diff follows:\n'"$(diff -u <(print -r -- "$exp") <(print -r -- "$got") | sed $'s/^/\t| /')"

# ======
# Before 2021-02-26, the DEBUG trap corrupted ${.sh.fun}
unset .sh.fun
got=$(some_func() { :; }; trap some_func DEBUG; trap - DEBUG; print -r "${.sh.fun}")
[[ -z $got ]] || err_exit "\${.sh.fun} leaks out of DEBUG trap (got $(printf %q "$got"))"

# =====
# Before 2021-03-06, ${foo=bar} and ${foo:=bar} did not work if `foo` had a numeric type
# https://github.com/ksh93/ksh/issues/157
# https://github.com/ksh93/ksh/pull/211#issuecomment-792336825

unset a b
typeset -i a
b=3+39
got=${a=b}
[[ $got == 42 ]] || err_exit "\${a=b}: expansion not working for integer type (expected '42', got '$got')"
[[ $a == 42 ]] || err_exit "\${a=b}: a was not assigned the correct integer value (expected '42', got '$a')"

unset a b
typeset -si a
b=3+39
got=${a=b}
[[ $got == 42 ]] || err_exit "\${a=b}: expansion not working for short integer type (expected '42', got '$got')"
[[ $a == 42 ]] || err_exit "\${a=b}: a was not assigned the correct short integer value (expected '42', got '$a')"

unset a b
typeset -F a
b=3.75+38.25
got=${a=b}
exp=42.0000000000
[[ $got == "$exp" ]] || err_exit "\${a=b}: expansion not working for float type (expected '$exp', got '$got')"
[[ $a == "$exp" ]] || err_exit "\${a=b}: a was not assigned the correct float value (expected '$exp', got '$a')"

unset a b
typeset -i a
b=3+39
got=${a:=b}
[[ $got == 42 ]] || err_exit "\${a:=b}: expansion not working for integer type (expected '42', got '$got')"
[[ $a == 42 ]] || err_exit "\${a:=b}: a was not assigned the correct integer value (expected '42', got '$a')"

unset a b
typeset -si a
b=3+39
got=${a:=b}
[[ $got == 42 ]] || err_exit "\${a:=b}: expansion not working for short integer type (expected '42', got '$got')"
[[ $a == 42 ]] || err_exit "\${a:=b}: a was not assigned the correct short integer value (expected '42', got '$a')"

unset a b
typeset -F a
b=3.75+38.25
got=${a:=b}
exp=42.0000000000
[[ $got == "$exp" ]] || err_exit "\${a:=b}: expansion not working for float type (expected '$exp', got '$got')"
[[ $a == "$exp" ]] || err_exit "\${a:=b}: a was not assigned the correct float value (expected '$exp', got '$a')"

# ======
# ${!FOO@} and ${!FOO*} expansions did not include FOO itself
# https://github.com/ksh93/ksh/issues/183
unset foo "${!foo@}"
exp='foo foobar fool'
got=$(IFS=/; foo=bar foobar=fbar fool=pity; print -r -- "${!foo@}")
[[ $got == "$exp" ]] || err_exit "\${!foo@}: expected $(printf %q "$exp"), got $(printf %q "$got")"
exp='foo/foobar/fool'
got=$(IFS=/; foo=bar foobar=fbar fool=pity; print -r -- "${!foo*}")
[[ $got == "$exp" ]] || err_exit "\${!foo*}: expected $(printf %q "$exp"), got $(printf %q "$got")"

# ======
# In ksh93v- ${.sh.subshell} is unset by the $PS4 prompt
# https://github.com/att/ast/issues/1092
exp='0'
got="$($SHELL -c 'PS4="${.sh.subshell}"; echo ${.sh.subshell}')"
[[ "$exp" == "$got" ]] || err_exit "\${.sh.subshell} is wrongly unset in the \$PS4 prompt" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# Corruption of SECONDS on leaving virtual subshell
# https://github.com/ksh93/ksh/issues/253#issuecomment-815191052
osec=$SECONDS
(SECONDS=20)
nsec=$SECONDS
if	((nsec<osec || nsec>osec+0.1))
then	err_exit "SECONDS corrupted after leaving virtual subshell (expected $osec, got $nsec)"
fi

# Corruption of LINENO on leaving virtual subshell
lineno_subshell=$tmp/lineno_subshell.sh
cat >| "$lineno_subshell" << 'EOF'
(
	unset LINENO
	:
)
echo $LINENO
EOF
exp=5
got=$($SHELL "$lineno_subshell")
[[ $exp == $got ]] || err_exit "LINENO corrupted after leaving virtual subshell (expected $exp, got $got)"

# ======
# Check if ${.sh.file} is set to correct value after sourcing a file
# https://github.com/att/ast/issues/472
cat > $tmp/foo.sh <<EOF
echo "foo"
EOF
. $tmp/foo.sh > /dev/null
[[ ${.sh.file} == $0 ]] || err_exit "\${.sh.file} is not set to the correct value after sourcing a file"

# ======
exit $((Errors<125?Errors:125))

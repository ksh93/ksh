########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1982-2012 AT&T Intellectual Property          #
#          Copyright (c) 2020-2023 Contributors to ksh 93u+m           #
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

# Tests for special and regular built-in commands (except those for
# libcmd path-bound built-ins; they should go into libcmd.sh instead)

. "${SHTESTS_COMMON:-${0%/*}/_common}"

bincat=$(whence -p cat)

# ======
# Test shell builtin commands
: ${foo=bar} || err_exit ": failed"
[[ $foo == bar ]] || err_exit ": side effects failed"
set -- - foobar
[[ $# == 2 && $1 == - && $2 == foobar ]] || err_exit "set -- - foobar failed"
set -- -x foobar
[[ $# == 2 && $1 == -x && $2 == foobar ]] || err_exit "set -- -x foobar failed"
getopts :x: foo || err_exit "getopts :x: returns false"
[[ $foo == x && $OPTARG == foobar ]] || err_exit "getopts :x: failed"
OPTIND=1
getopts :r:s var -r
if	[[ $var != : || $OPTARG != r ]]
then	err_exit "'getopts :r:s var -r' not working"
fi
OPTIND=1
getopts :d#u OPT -d 16177
if	[[ $OPT != d || $OPTARG != 16177 ]]
then	err_exit "'getopts :d#u OPT=d OPTARG=16177' failed -- OPT=$OPT OPTARG=$OPTARG"
fi
OPTIND=1
while getopts 'ab' option -a -b
do	[[ $OPTIND == $((OPTIND)) ]] || err_exit "OPTIND optimization bug"
done

USAGE=$'[-][S:server?Operate on the specified \asubservice\a:]:[subservice:=pmserver]
    {
        [p:pmserver]
        [r:repserver]
        [11:notifyd]
    }'
set pmser p rep r notifyd -11
while	(( $# > 1 ))
do	OPTIND=1
	getopts "$USAGE" OPT -S $1
	[[ $OPT == S && $OPTARG == $2 ]] || err_exit "OPT=$OPT OPTARG=$OPTARG -- expected OPT=S OPTARG=$2"
	shift 2
done

false ${foo=bar} &&  err_exit "false failed"
read <<!
hello world
!
[[ $REPLY == 'hello world' ]] || err_exit "read builtin failed"
print x:y | IFS=: read a b
if	[[ $a != x ]]
then	err_exit "IFS=: read ... not working"
fi
read <<!
hello \
world
!
[[ $REPLY == 'hello world' ]] || err_exit "read continuation failed"
read -d x <<!
hello worldxfoobar
!
[[ $REPLY == 'hello world' ]] || err_exit "read builtin failed"
read <<\!
hello \
	world \

!
[[ $REPLY == 'hello 	world' ]] || err_exit "read continuation 2 failed"
print "one\ntwo" | { read line
	print $line | "$bincat" > /dev/null
	read line
}
read <<\!
\
a\
\
\
b
!
if	[[ $REPLY != ab ]]
then	err_exit "read multiple continuation failed"
fi
if	[[ $line != two ]]
then	err_exit "read from pipeline failed"
fi
line=two
read line < /dev/null
if	[[ $line != "" ]]
then	err_exit "read from /dev/null failed"
fi
if	[[ $(print -R -) != - ]]
then	err_exit "print -R not working correctly"
fi
if	[[ $(print -- -) != - ]]
then	err_exit "print -- not working correctly"
fi
print -f "hello%nbar\n" size > /dev/null
if	((	size != 5 ))
then	err_exit "%n format of printf not working"
fi
print -n -u2 2>&1-
[[ -w /dev/fd/1 ]] || err_exit "2<&1- with built-ins has side effects"
x=$0
if	[[ $(eval 'print $0') != $x ]]
then	err_exit '$0 not correct for eval'
fi
$SHELL -c 'read x <<< hello' 2> /dev/null || err_exit 'syntax <<< not recognized'
($SHELL -c 'read x[1] <<< hello') 2> /dev/null || err_exit 'read x[1] not working'
unset x
readonly x
set -- $(readonly)
if      [[ " $@ " != *" x "* ]]
then    err_exit 'unset readonly variables are not displayed'
fi
if	[[ $(	for i in foo bar
		do	print $i
			continue 10
		done
	    ) != $'foo\nbar' ]]
then	err_exit 'continue breaks out of loop'
fi
(continue bad 2>/dev/null && err_exit 'continue bad should return an error')
(break bad 2>/dev/null && err_exit 'break bad should return an error')
(continue 0 2>/dev/null && err_exit 'continue 0 should return an error')
(break 0 2>/dev/null && err_exit 'break 0 should return an error')
breakfun() { break;}
continuefun() { continue;}
for fun in break continue
do	if	[[ $(	for i in foo
			do	${fun}fun
				print $i
			done
		) != foo ]]
	then	err_exit "$fun call in ${fun}fun breaks out of for loop"
	fi
done
if	[[ $(print -f "%b" "\a\n\v\b\r\f\E\03\\oo") != $'\a\n\v\b\r\f\E\03\\oo' ]]
then	err_exit 'print -f "%b" not working'
fi
if	[[ $(print -f "%P" "[^x].*b\$") != '*[!x]*b' ]]
then	err_exit 'print -f "%P" not working'
fi
if	[[ $(print -f "%(pattern)q" "[^x].*b\$") != '*[!x]*b' ]]
then	err_exit 'print -f "%(pattern)q" not working'
fi
if	[[ $(command -v if)	!= if ]]
then	err_exit	'command -v not working'
fi
read -r var <<\!

!
if	[[ $var != "" ]]
then	err_exit "read -r of blank line not working"
fi
mkdir -p $tmp/a/b/c 2>/dev/null || err_exit  "mkdir -p failed"
$SHELL -c "cd $tmp/a/b; cd c" 2>/dev/null || err_exit "initial script relative cd fails"

trap 'print TERM' TERM
exp="trap -- 'print TERM' TERM"
got=$(trap)
[[ $got == $exp ]] || err_exit "\$(trap) failed -- expected \"$exp\", got \"$got\""
exp='print TERM'
got=$(trap -p TERM)
[[ $got == $exp ]] || err_exit "\$(trap -p TERM) failed -- expected \"$exp\", got \"$got\""

[[ $($SHELL -c 'trap "print ok" SIGTERM; kill -s SIGTERM $$' 2> /dev/null) == ok ]] || err_exit 'SIGTERM not recognized'
[[ $($SHELL -c 'trap "print ok" sigterm; kill -s sigterm $$' 2> /dev/null) == ok ]] || err_exit 'SIGTERM not recognized'
[[ $($SHELL -c '( trap "" TERM);kill $$;print bad' == bad) ]] 2> /dev/null && err_exit 'trap ignored in subshell causes it to be ignored by parent'
${SHELL} -c 'kill -1 -$$' 2> /dev/null
[[ $(kill -l $?) == HUP ]] || err_exit 'kill -1 -pid not working'
${SHELL} -c 'kill -1 -$$' 2> /dev/null
[[ $(kill -l $?) == HUP ]] || err_exit 'kill -n1 -pid not working'
${SHELL} -c 'kill -s HUP -$$' 2> /dev/null
[[ $(kill -l $?) == HUP ]] || err_exit 'kill -HUP -pid not working'
n=123
typeset -A base
base[o]=8#
base[x]=16#
base[X]=16#
for i in d i o u x X
do	if	(( $(( ${base[$i]}$(printf "%$i" $n) )) != n  ))
	then	err_exit "printf %$i not working"
	fi
done
if	[[ $( trap 'print done' EXIT) != done ]]
then	err_exit 'trap on EXIT not working'
fi
if	[[ $( trap 'print done' EXIT; trap - EXIT) == done ]]
then	err_exit 'trap on EXIT not being cleared'
fi
if	[[ $(LC_MESSAGES=C type test) != 'test is a shell builtin' ]]
then	err_exit 'whence -v test not a builtin'
fi
if	[[ $(builtin -d test; type test) == *builtin* ]]
then	err_exit 'whence -v test after builtin -d incorrect'
fi
typeset -Z3 percent=$(printf '%o\n' "'%'")
forrmat=\\${percent}s
if      [[ $(printf "$forrmat") != %s ]]
then    err_exit "printf $forrmat not working"
fi
if	(( $(printf 'x\0y' | wc -c) != 3 ))
then	err_exit 'printf \0 not working'
fi
if	[[ $(printf "%bx%s\n" 'f\to\cbar') != $'f\to' ]]
then	err_exit 'printf %bx%s\n not working'
fi
alpha=abcdefghijklmnop
if	[[ $(printf "%10.*s\n" 5 $alpha) != '     abcde' ]]
then	err_exit 'printf %10.%s\n not working'
fi
float x2=.0000625
if	[[ $(printf "%10.5E\n" x2) != 6.25000E-05 ]]
then	err_exit 'printf "%10.5E" not normalizing correctly'
fi
x2=.000000001
if	[[ $(printf "%g\n" x2 2>/dev/null) != 1e-09 ]]
then	err_exit 'printf "%g" not working correctly'
fi

if((!SHOPT_SCRIPTONLY));then
(read -s foobar <<<testing_read_s) 2> /dev/null || err_exit "'read -s var' fails"
exp=$'^[[:digit:]]+\ttesting_read_s$'
got=$(fc -l -0)
[[ $got =~ $exp ]] || err_exit "'read -s' did not write to history file" \
	"(expected match of regex $(printf %q "$exp"), got $(printf %q "$got"))"
fi # !SHOPT_SCRIPTONLY

if	[[ $(printf +3 2>/dev/null) !=   +3 ]]
then	err_exit 'printf is not processing formats beginning with + correctly'
fi
if	printf "%d %d\n" 123bad 78 >/dev/null 2>/dev/null
then	err_exit "printf not exiting non-zero with conversion errors"
fi
if	[[ $(trap --version 2> /dev/null;print done) != done ]]
then	err_exit 'trap builtin terminating after --version'
fi
if	[[ $(set --version 2> /dev/null;print done) != done ]]
then	err_exit 'set builtin terminating after --version'
fi
unset -f foobar
function foobar
{
	print 'hello world'
}
OPTIND=1
if	[[ $(getopts  $'[+?X\ffoobar\fX]' v --man 2>&1) != *'Xhello world'X* ]]
then	err_exit '\f...\f not working in getopts usage strings'
fi

expect=$'&lt;&gt;&quot;&amp; &#39;\tabc'
actual=$(printf '%H\n' $'<>"& \'\tabc')
[[ $expect == "$actual" ]] || err_exit 'printf %H not working' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
actual=$(printf '%(html)q\n' $'<>"& \'\tabc')
[[ $expect == "$actual" ]] || err_exit 'printf %(html)q not working' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

expect='foo://ab_c%3C%3E%22%26%20%27%09abc'
actual=$(printf 'foo://ab_c%#H\n' $'<>"& \'\tabc')
[[ $expect == "$actual" ]] || err_exit 'printf %#H not working' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
actual=$(printf 'foo://ab_c%(url)q\n' $'<>"& \'\tabc')
[[ $expect == "$actual" ]] || err_exit 'printf %(url)q not working' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

case ${LC_ALL:-${LC_CTYPE:-${LANG:-}}} in
( *[Uu][Tt][Ff]8* | *[Uu][Tt][Ff]-8* )
	# HTML encoding UTF-8 characters
	# (UTF-8 literal characters wrapped in 'eval' to avoid syntax error on ja_JP.SJIS)
	eval 'expect='\''正常終了 正常終了'\'
	eval 'actual=$(printf %H '\''正常終了 正常終了'\'')'
	[[ $actual == "$expect" ]] || err_exit 'printf %H: Japanese UTF-8 characters' \
				"(expected $expect; got $actual)"
	expect='w?h?á?t??'
	actual=$(printf %H $'w\x80h\x81\uE1\x82t\x83?')
	[[ $actual == "$expect" ]] || err_exit 'printf %H: invalid UTF-8 characters' \
				"(expected $expect; got $actual)"
	# URL/URI encoding of UTF-8 characters
	expect='wh.at%3F'
	actual=$(printf %#H 'wh.at?')
	[[ $actual == "$expect" ]] || err_exit 'printf %H: ASCII characters' \
				"(expected $expect; got $actual)"
	expect='%D8%B9%D9%86%D8%AF%D9%85%D8%A7%20%D9%8A%D8%B1%D9%8A%D8%AF%20%D8%A7%D9%84%D8%B9%D8%A7%D9%84%D9%85%20%D8%A3%D9%86%20%E2%80%AA%D9%8A%D8%AA%D9%83%D9%84%D9%91%D9%85%20%E2%80%AC%20%D8%8C%20%D9%81%D9%87%D9%88%20%D9%8A%D8%AA%D8%AD%D8%AF%D9%91%D8%AB%20%D8%A8%D9%84%D8%BA%D8%A9%20%D9%8A%D9%88%D9%86%D9%8A%D9%83%D9%88%D8%AF.'
	actual=$(printf %#H 'عندما يريد العالم أن ‪يتكلّم ‬ ، فهو يتحدّث بلغة يونيكود.')
	[[ $actual == "$expect" ]] || err_exit 'printf %H: Arabic UTF-8 characters' \
				"(expected $expect; got $actual)"
	expect='%E6%AD%A3%E5%B8%B8%E7%B5%82%E4%BA%86%20%E6%AD%A3%E5%B8%B8%E7%B5%82%E4%BA%86'
	eval 'actual=$(printf %#H '\''正常終了 正常終了'\'')'
	[[ $actual == "$expect" ]] || err_exit 'printf %H: Japanese UTF-8 characters' \
				"(expected $expect; got $actual)"
	expect='%C2%AB%20l%E2%80%99ab%C3%AEme%20de%20mon%C2%A0m%C3%A9tier%E2%80%A6%20%C2%BB'
	actual=$(printf %#H '« l’abîme de mon métier… »')
	[[ $actual == "$expect" ]] || err_exit 'printf %H: Latin UTF-8 characters' \
				"(expected $expect; got $actual)"
	expect='%3F%C2%86%3F%3F%3F'
	actual=$(printf %#H $'\x86\u86\xF0\x96\x76\xA7\xB5')
	[[ $actual == "$expect" ]] || err_exit 'printf %H: invalid UTF-8 characters' \
				"(expected $expect; got $actual)"
	;;
esac

if	[[ $(printf '%R %R %R %R\n' 'a.b' '*.c' '^'  '!(*.*)') != '^a\.b$ \.c$ ^\^$ ^(.*\..*)!$' ]]
then	err_exit 'printf %T not working'
fi
if	[[ $(printf '%(ere)q %(ere)q %(ere)q %(ere)q\n' 'a.b' '*.c' '^'  '!(*.*)') != '^a\.b$ \.c$ ^\^$ ^(.*\..*)!$' ]]
then	err_exit 'printf %(ere)q not working'
fi
if	[[ $(printf '%..:c\n' abc) != a:b:c ]]
then	err_exit "printf '%..:c' not working"
fi
if	[[ $(printf '%..*c\n' : abc) != a:b:c ]]
then	err_exit "printf '%..*c' not working"
fi
if	[[ $(printf '%..:s\n' abc def ) != abc:def ]]
then	err_exit "printf '%..:s' not working"
fi
if	[[ $(printf '%..*s\n' : abc def) != abc:def ]]
then	err_exit "printf '%..*s' not working"
fi

# ======
# we won't get hit by the one second boundary twice, right?
expect= actual=
{ expect=$(LC_ALL=C date) && actual=$(LC_ALL=C printf '%T\n' now) && [[ ${actual/ GMT / UTC } == "${expect/ GMT / UTC }" ]]; } ||
{ expect=$(LC_ALL=C date) && actual=$(LC_ALL=C printf '%T\n' now) && [[ ${actual/ GMT / UTC } == "${expect/ GMT / UTC }" ]]; } ||
err_exit 'printf "%T" now' "(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
behead()
{
	read line
	left=$(cat)
}
print $'line1\nline2' | behead
if	[[ $left != line2 ]]
then	err_exit "read reading ahead on a pipe"
fi
read -n1 y <<!
abc
!
exp=a
if      [[ $y != $exp ]]
then    err_exit "read -n1 failed -- expected '$exp', got '$y'"
fi
print -n $'{ read -r line;print $line;}\nhello' > $tmp/script
chmod 755 $tmp/script
if	[[ $($SHELL < $tmp/script) != hello ]]
then	err_exit 'read of incomplete line not working correctly'
fi
set -f
set -- *
if      [[ $1 != '*' ]]
then    err_exit 'set -f not working'
fi
unset pid1 pid2
false &
pid1=$!
pid2=$(
	wait $pid1
	(( $? == 127 )) || err_exit "job known to subshell"
	print $!
)
wait $pid1
(( $? == 1 )) || err_exit "wait not saving exit value"
wait $pid2
(( $? == 127 )) || err_exit "subshell job known to parent"
env=
if builtin getconf 2> /dev/null; then
	v=$(getconf LIBPATH)
	for v in ${v//,/ }
	do	v=${v#*:}
		v=${v%%:*}
		eval [[ \$$v ]] && env="$env $v=\"\$$v\""
	done
	if	[[ $(foo=bar; eval foo=\$foo $env exec -c \$SHELL -c \'print \$foo\') != bar ]]
	then	err_exit '"name=value exec -c ..." not working'
	fi
fi
$SHELL -c 'OPTIND=-1000000; getopts a opt -a' 2> /dev/null
[[ $? == 1 ]] || err_exit 'getopts with negative OPTIND not working'
getopts 'n#num' opt  -n 3
[[ $OPTARG == 3 ]] || err_exit 'getopts with numerical arguments failed'
if	[[ $($SHELL -c $'printf \'%2$s %1$s\n\' world hello') != 'hello world' ]]
then	err_exit 'printf %2$s %1$s not working'
fi
val=$(( 'C' ))
set -- \
	"'C"	$val	0	\
	"'C'"	$val	0	\
	'"C'	$val	0	\
	'"C"'	$val	0	\
	"'CX"	$val	1	\
	"'CX'"	$val	1	\
	"'C'X"	$val	1	\
	'"CX'	$val	1	\
	'"CX"'	$val	1	\
	'"C"X'	$val	1
while (( $# >= 3 ))
do	arg=$1 val=$2 code=$3
	shift 3
	for fmt in '%d' '%g'
	do	out=$(printf "$fmt" "$arg" 2>/dev/null)
		err=$(set +x; printf "$fmt" "$arg" 2>&1 >/dev/null)
		printf "$fmt" "$arg" >/dev/null 2>&1
		ret=$?
		[[ $out == $val ]] || err_exit "printf $fmt $arg failed -- expected '$val', got '$out'"
		if	(( $code ))
		then	[[ $err ]] || err_exit "printf $fmt $arg failed, error message expected"
		else	[[ $err ]] && err_exit "$err: printf $fmt $arg failed, error message not expected -- got '$err'"
		fi
		(( $ret == $code )) || err_exit "printf $fmt $arg failed -- expected exit code $code, got $ret"
	done
done
((n=0))
((n++)); ARGC[$n]=1 ARGV[$n]=""
((n++)); ARGC[$n]=2 ARGV[$n]="-a"
((n++)); ARGC[$n]=4 ARGV[$n]="-a -v 2"
((n++)); ARGC[$n]=4 ARGV[$n]="-a -v 2 x"
((n++)); ARGC[$n]=4 ARGV[$n]="-a -v 2 x y"
for ((i=1; i<=n; i++))
do	set -- ${ARGV[$i]}
	OPTIND=0
	while	getopts -a tst "av:" OPT
	do	:
	done
	if	[[ $OPTIND != ${ARGC[$i]} ]]
	then	err_exit "\$OPTIND after getopts loop incorrect -- expected ${ARGC[$i]}, got $OPTIND"
	fi
done
options=ab:c
optarg=foo
set -- -a -b $optarg -c bar
while	getopts $options opt
do	case $opt in
	a|c)	[[ $OPTARG ]] && err_exit "getopts $options \$OPTARG for flag $opt failed, expected \"\", got \"$OPTARG\"" ;;
	b)	[[ $OPTARG == $optarg ]] || err_exit "getopts $options \$OPTARG failed -- \"$optarg\" expected, got \"$OPTARG\"" ;;
	*)	err_exit "getopts $options failed -- got flag $opt" ;;
	esac
done

[[ $($SHELL 2> /dev/null -c 'readonly foo; getopts a: foo -a blah; echo foo') == foo ]] || err_exit 'getopts with readonly variable causes script to abort'

unset a
{ read -N3 a; read -N1 b;}  <<!
abcdefg
!
exp=abc
[[ $a == $exp ]] || err_exit "read -N3 here-document failed -- expected '$exp', got '$a'"
exp=d
[[ $b == $exp ]] || err_exit "read -N1 here-document failed -- expected '$exp', got '$b'"
read -n3 a <<!
abcdefg
!
exp=abc
[[ $a == $exp ]] || err_exit "read -n3 here-document failed -- expected '$exp', got '$a'"
#(print -n a;sleep 1; print -n bcde) | { read -N3 a; read -N1 b;}
#[[ $a == $exp ]] || err_exit "read -N3 from pipe failed -- expected '$exp', got '$a'"
#exp=d
#[[ $b == $exp ]] || err_exit "read -N1 from pipe failed -- expected '$exp', got '$b'"
#(print -n a;sleep 1; print -n bcde) | read -n3 a
#exp=a
#[[ $a == $exp ]] || err_exit "read -n3 from pipe failed -- expected '$exp', got '$a'"
#rm -f $tmp/fifo
#if	mkfifo $tmp/fifo 2> /dev/null
#then	(print -n a; sleep 1;print -n bcde)  > $tmp/fifo &
#	{
#	read -u5 -n3 -t2 a || err_exit 'read -n3 from fifo timedout'
#	read -u5 -n1 -t2 b || err_exit 'read -n1 from fifo timedout'
#	} 5< $tmp/fifo
#	exp=a
#	[[ $a == $exp ]] || err_exit "read -n3 from fifo failed -- expected '$exp', got '$a'"
#	rm -f $tmp/fifo
#	mkfifo $tmp/fifo 2> /dev/null
#	(print -n a; sleep 1;print -n bcde) > $tmp/fifo &
#	{
#	read -u5 -N3 -t2 a || err_exit 'read -N3 from fifo timed out'
#	read -u5 -N1 -t2 b || err_exit 'read -N1 from fifo timedout'
#	} 5< $tmp/fifo
#	exp=abc
#	[[ $a == $exp ]] || err_exit "read -N3 from fifo failed -- expected '$exp', got '$a'"
#	exp=d
#	[[ $b == $exp ]] || err_exit "read -N1 from fifo failed -- expected '$exp', got '$b'"
#fi
#rm -f $tmp/fifo

function longline
{
	integer i
	for((i=0; i < $1; i++))
	do	print argument$i
	done
}
# test that 'command' can result from expansion and can be overridden by a function
expect=all\ ok
c=command
actual=$("$c" echo all ok 2>&1)
[[ $actual == "$expect" ]] || err_exit '"command" utility from "$c" expansion not executed' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
set -- command echo all ok
actual=$("$@" 2>&1)
[[ $actual == "$expect" ]] || err_exit '"command" utility from "$@" expansion not executed' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
# must use 'eval' in following test as 'command' is integrated in parser and ksh likes to parse ahead
actual=$(function command { echo all ok; }; eval 'command echo all wrong')
[[ $actual == "$expect" ]] || err_exit '"command" failed to be overridden by function' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
actual=$(function command { echo all ok; }; "$c" echo all wrong)
[[ $actual == "$expect" ]] || err_exit '"command" from "$c" expansion failed to be overridden by function' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
set -- command echo all wrong
actual=$(function command { echo all ok; }; "$@")
[[ $actual == "$expect" ]] || err_exit '"command" from "$@" expansion failed to be overridden by function' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
# test command -x option
integer sum=0 n=10000
if	! ${SHELL:-ksh} -c 'print $#' count $(longline $n) > /dev/null  2>&1
then	for i in $(command command -x ${SHELL:-ksh} -c 'print $#;[[ $1 != argument0 ]]' count $(longline $n) 2> /dev/null)
	do	((sum += $i))
	done
	(( sum == n )) || err_exit "command -x processed only $sum arguments"
	command -p command -x ${SHELL:-ksh} -c 'print $#;[[ $1 == argument0 ]]' count $(longline $n) > /dev/null  2>&1
	[[ $? != 1 ]] && err_exit 'incorrect exit status for command -x'
fi
# test command -x option with extra arguments
integer sum=0 n=10000
if      ! ${SHELL:-ksh} -c 'print $#' count $(longline $n) > /dev/null  2>&1
then    for i in $(command command -x ${SHELL:-ksh} -c 'print $#;[[ $1 != argument0 ]]' count $(longline $n) one two three) #2> /dev/null)
	do      ((sum += $i))
	done
	(( sum  > n )) || err_exit "command -x processed only $sum arguments"
	(( (sum-n)%3==0 )) || err_exit "command -x processed only $sum arguments"
	(( sum == n+3)) && err_exit "command -x processed only $sum arguments"
	command -p command -x ${SHELL:-ksh} -c 'print $#;[[ $1 == argument0 ]]' count $(longline $n) > /dev/null  2>&1
	[[ $? != 1 ]] && err_exit 'incorrect exit status for command -x'
fi
# test for debug trap
[[ $(typeset -i i=0
	trap 'print $i' DEBUG
	while (( i <2))
	do	(( i++))
	done) == $'0\n0\n1\n1\n2' ]]  || err_exit  "DEBUG trap not working"
if builtin getconf 2> /dev/null; then
	getconf UNIVERSE - ucb
	[[ $($SHELL -c 'echo -3') == -3 ]] || err_exit "echo -3 not working in ucb universe"
fi
typeset -F3 start_x=SECONDS total_t delay=0.02
typeset reps=50 leeway=5
sleep $(( 2 * leeway * reps * delay )) |
for (( i=0 ; i < reps ; i++ ))
do	read -N1 -t $delay
done
(( total_t = SECONDS - start_x ))
if	(( total_t > leeway * reps * delay ))
then	err_exit "read -t in pipe taking $total_t secs - $(( reps * delay )) minimum - too long"
elif	(( total_t < reps * delay ))
then	err_exit "read -t in pipe taking $total_t secs - $(( reps * delay )) minimum - too fast"
fi
$SHELL -c 'sleep $(printf "%a" .95)' 2> /dev/null || err_exit "sleep doesn't accept %a format constants"
$SHELL -c 'test \( ! -e \)' 2> /dev/null ; [[ $? == 1 ]] || err_exit 'test \( ! -e \) not working'
[[ $(ulimit) == "$(ulimit -fS)" ]] || err_exit 'ulimit is not the same as ulimit -fS'
tmpfile=$tmp/file.2
print $'\nprint -r -- "${.sh.file} ${LINENO} ${.sh.lineno}"' > $tmpfile
[[ $( . "$tmpfile") == "$tmpfile 2 1" ]] || err_exit 'dot command not working'
print -r -- "'xxx" > $tmpfile
[[ $($SHELL -c ". $tmpfile"$'\n print ok' 2> /dev/null) == ok ]] || err_exit 'syntax error in dot command affects next command'

float sec=$SECONDS del=4
exec 3>&2 2>/dev/null
$SHELL -c "( sleep 1; kill -ALRM \$\$ ) & sleep $del" 2> /dev/null
exitval=$?
(( sec = SECONDS - sec ))
exec 2>&3-
(( exitval )) && err_exit "sleep doesn't exit 0 with ALRM interrupt"
(( sec > (del - 1) )) || err_exit "ALRM signal causes sleep to terminate prematurely -- expected 3 sec, got $sec"
typeset -r z=3
y=5
for i in 123 z  %x a.b.c
do	( unset $i)  2>/dev/null && err_exit "unset $i should fail"
done
a=()
for i in y y  y[8] t[abc] y.d a.b  a
do	unset $i ||  print -u2  "err_exit unset $i should not fail"
done
[[ $($SHELL -c 'y=3; unset 123 y;print $?$y') == 1 ]] 2> /dev/null ||  err_exit 'y is not getting unset with unset 123 y'
[[ $($SHELL -c 'trap foo TERM; (trap;(trap) )') == 'trap -- foo TERM' ]] || err_exit 'traps not getting reset when subshell is last process'

n=$(printf "%b" 'a\0b\0c' | wc -c)
(( n == 5 )) || err_exit '\0 not working with %b format with printf'

t=$(ulimit -t)
[[ $($SHELL -c 'ulimit -v 15000 2>/dev/null; ulimit -t') == "$t" ]] || err_exit 'ulimit -v changes ulimit -t'

$SHELL 2> /dev/null -c 'cd ""' && err_exit 'cd "" not producing an error'
[[ $($SHELL 2> /dev/null -c 'cd "";print hi') != hi ]] && err_exit 'cd "" should not terminate script'

if builtin cat 2> /dev/null; then
	out=$tmp/seq.out
	for ((i=1; i<=11; i++)); do print "$i"; done >$out
	cmp -s <(print -- "$($bincat<( $bincat $out ) )") <(print -- "$(cat <( cat $out ) )") || err_exit "builtin cat differs from $bincat"
fi

[[ $($SHELL -c '{ printf %R "["; print ok;}' 2> /dev/null) == ok ]] || err_exit $'\'printf %R "["\' causes shell to abort'

exp=$'usr1\ndone'
got=$( $SHELL -c $'
	trap \'print "usr1"\' USR1
	trap exit USR2
	sleep 1 && {
		kill -USR1 $$ && sleep 1
		kill -0 $$ 2>/dev/null && kill -USR2 $$
	} &
	sleep 2 | read
	echo done
' )
[[ $got == $exp ]] || err_exit 'read not terminating when receiving USR1 signal' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

mkdir $tmp/tmpdir1
cd $tmp/tmpdir1
pwd=$PWD
cd ../tmpdir1
[[ $PWD == "$pwd" ]] || err_exit 'cd ../tmpdir1 causes directory to change'
cd "$pwd"
mv $tmp/tmpdir1 $tmp/tmpdir2
cd ..  2> /dev/null || err_exit 'cannot change directory to .. after current directory has been renamed'
[[ $PWD == "$tmp" ]] || err_exit 'after "cd $tmp/tmpdir1; cd .." directory is not $tmp'

cd "$tmp"
mkdir $tmp/tmpdir2/foo
pwd=$PWD
cd $tmp/tmpdir2/foo
mv $tmp/tmpdir2 $tmp/tmpdir1
cd ../.. 2> /dev/null || err_exit 'cannot change directory to ../.. after current directory has been renamed'
[[ $PWD == "$tmp" ]] || err_exit 'after "cd $tmp/tmpdir2; cd ../.." directory is not $tmp'
cd "$tmp"
rm -rf tmpdir1

cd /etc
cd ..
[[ $(pwd) == / ]] || err_exit 'cd /etc;cd ..;pwd is not /'
cd /etc
cd ../..
[[ $(pwd) == / ]] || err_exit 'cd /etc;cd ../..;pwd is not /'
cd /etc
cd .././..
[[ $(pwd) == / ]] || err_exit 'cd /etc;cd .././..;pwd is not /'
if [[ ! -d /usr/bin ]]; then
	dir=/system  # For Haiku, fallback to /system
else
	dir=/usr
fi
cd "$dir/bin"
cd ../..
[[ $(pwd) == / ]] || err_exit "'cd $dir/bin; cd ../..; pwd' is not /"
cd "$dir/bin"
cd ..
[[ $(pwd) == "$dir" ]] || err_exit "cd '$dir/bin; cd ..; pwd' is not $dir"
cd "$tmp"
if	mkdir $tmp/t1
then	(
		cd $tmp/t1
		> real_t1
		(
			cd ..
			mv t1 t2
			mkdir t1
		)
		[[ -f real_t1 ]]
	) || err_exit 'real_t1 not found after parent directory renamed in subshell'
fi
cd "$tmp"

if	kill -L > /dev/null 2>&1
then	[[ $(kill -l HUP) == "$(kill -L HUP)" ]] || err_exit 'kill -l and kill -L are not the same when given a signal name'
	[[ $(kill -l 9) == "$(kill -L 9)" ]] || err_exit 'kill -l and kill -L are not the same when given a signal number'
	[[ $(kill -L) == *'9) KILL'* ]] || err_exit 'kill -L output does not contain 9) KILL'
fi

if((!SHOPT_SCRIPTONLY));then
export ENV=/./dev/null
v=$($SHELL 2> /dev/null +o rc -ic $'getopts a:bc: opt --man\nprint $?')
[[ $v == 2* ]] || err_exit 'getopts --man does not exit 2 for interactive shells'
fi # !SHOPT_SCRIPTONLY

read baz <<< 'foo\\\\bar'
[[ $baz == 'foo\\bar' ]] || err_exit 'read of foo\\\\bar not getting foo\\bar'

# ======
# Check that I/O errors are detected <https://github.com/att/ast/issues/1093>
actual=$(
    set +x
    {
        (
            trap "" PIPE
            for ((i = SECONDS + 1; SECONDS < i; )); do
                print hi || {
                    print $? >&2
                    exit
                }
            done
        ) | true
    } 2>&1
)
expect='1'
if [[ $actual != "$expect" ]]
then
    err_exit "I/O error not detected: expected $(printf %q "$expect"), got $(printf %q "$actual")"
fi

# ======
# 'time' keyword and 'times' builtin

exp=$'^user\t0m00.[0-9]{3}s\nsys\t0m00.[0-9]{3}s\n0m00.[0-9]{3}s 0m00.[0-9]{3}s\n0m00.000s 0m00.000s$'
got=$("$SHELL" -c '{ time; } 2>&1; times')
[[ $got =~ $exp ]] || err_exit "times output: expected match of $(printf %q "$exp"), got $(printf %q "$got")"

expect=$'*: times: too many operands'
actual=$(set +x; eval 'times Extra Args' 2>&1)
[[ $actual == $expect ]] || err_exit "times with args: expected $(printf %q "$expect"), got $(printf %q "$actual")"

# ======
# 'whence' builtin
PATH=$tmp:$PATH $SHELL <<-\EOF || err_exit "'whence' gets wrong path on init"
	wc=$(whence wc)
	[[ -x $wc ]]
EOF

# ======
# 'builtin -d' should not delete special builtins
(builtin -d export 2> /dev/null
PATH=/dev/null
whence -q export) || err_exit "'builtin -d' deletes special builtins"

# ======
# 'read -r -d' should not ignore '-r'
printf '\\\000' | read -r -d ''
[[ $REPLY == $'\\' ]] || err_exit "read -r -d '' ignores -r"

# ======
# BUG_CMDSPASGN: Preceding a special builtin with 'command' should disable its special properties.
# Test that assignments preceding 'command' are local.
for arg in '' -v -V -p -x
do
	for cmd in '' : true ls eval 'eval :' 'eval true' 'eval ls'
	do
		[[ $arg == -x ]] && ! command -xv "${cmd% *}" >/dev/null && continue
		unset foo
		eval "foo=BUG command $arg $cmd" >/dev/null 2>&1
		got=$?
		case $arg,$cmd in
		-v, | -V, )	exp=2 ;;
		*)		exp=0 ;;
		esac
		[[ $got == "$exp" ]] || err_exit "exit status of 'command $arg $cmd' is $got, expected $exp"
		[[ -v foo ]] && err_exit "preceding assignment survives 'command $arg $cmd'"
	done
done

# Regression that occurred after fixing the bug above: the += operator didn't work correctly.
# https://www.mail-archive.com/ast-users@lists.research.att.com/msg00369.html
unset foo
integer foo=1
exp=4
got=$(foo+=3 command eval 'echo $foo')
[[ $exp == $got ]] || err_exit "Test 1: += assignment for environment variables doesn't work with 'command special_builtin'" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
foo+=3 command eval 'test $foo'
exp=1
(( foo == exp )) || err_exit "environment isn't restored after 'command special_builtin'" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(foo+=3 eval 'echo $foo')
exp=4
[[ $exp == $got ]] || err_exit "+= assignment for environment variables doesn't work with builtins" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

unset foo
exp=barbaz
got=$(foo=bar; foo+=baz command eval 'echo $foo')
[[ $exp == $got ]] || err_exit "Test 2: += assignment for environment variables doesn't work with 'command special_builtin'" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

unset y
exp='outside f, 1, 2, 3, outside f'
got=$(
	f() {
		if [ -n "${_called_f+_}" ]; then
			for y; do
				printf '%s, ' "$y"
			done
		else
			_called_f= y= command eval '{ typeset +x y; } 2>/dev/null; f "$@"'
		fi
	}
	y='outside f'
	printf "$y, "
	f 1 2 3
	echo "$y"
)
[[ $got == "$exp" ]] || err_exit 'assignments to "command special_built-in" leaving side effects' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Attempting to modify a readonly variable with the += operator should fail
exp=2
got=$(integer -r foo=2; foo+=3 command eval 'echo $foo' 2> /dev/null)
[[ $? == 0 ]] && err_exit "+= assignment modifies readonly variables" \
	"(expected $exp, got $got)"

# ======
# 'whence -f' should ignore functions
foo_bar() { true; }
actual="$(whence -f foo_bar)"
whence -f foo_bar >/dev/null && err_exit "'whence -f' doesn't ignore functions (got $(printf %q "$actual"))"

# whence -vq/type -q must be tested as well
actual="$(type -f foo_bar 2>&1)"
type -f foo_bar >/dev/null 2>&1 && err_exit "'type -f' doesn't ignore functions (got $(printf %q "$actual"))"
type -qf foo_bar && err_exit "'type -qf' doesn't ignore functions"

# Test the exit status of 'whence -q'
mkdir "$tmp/fakepath"
ln -s "${ whence -p cat ;}" "$tmp/fakepath/"
ln -s "${ whence -p ls ;}" "$tmp/fakepath/"
save_PATH=$PATH
PATH=$tmp/fakepath
whence -q cat nonexist ls && err_exit "'whence -q' has the wrong exit status"
whence -q cat nonexist && err_exit "'whence -q' has the wrong exit status"
whence -q nonexist && err_exit "'whence -q' has the wrong exit status"
PATH=$save_PATH

# ======
# These are the regression tests for the whence builtin's '-t' flag
((.sh.version >= 20211227)) &&
for w in 'whence -t' 'type -t' 'whence -t -v'; do
	exp=file
	got=$($w $SHELL)
	[[ $exp == "$got" ]] || err_exit "'$w' has the wrong output for external commands" \
		"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
	got=$(builtin -d chmod; hash chmod; $w chmod)
	[[ $exp == "$got" ]] || err_exit "'$w' has the wrong output for tracked aliases" \
		"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
	exp=keyword
	got=$($w time)
	[[ $exp == "$got" ]] || err_exit "'$w' has the wrong output for keywords" \
		"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
	exp=builtin
	got=$($w sleep)
	[[ $exp == "$got" ]] || err_exit "'$w' has the wrong output for regular builtins" \
		"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
	got=$($w export)
	[[ $exp == "$got" ]] || err_exit "'$w' has the wrong output for special builtins" \
		"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
	builtin -d cat
	if [[ $(set +x; PATH=/opt/ast/bin type cat 2>&1) != "cat is a shell builtin version of /opt/ast/bin/cat" ]]
	then	warning "/opt/ast/bin/cat isn't a builtin; skipping path-bound builtin tests"
	else
		got=$(PATH=/opt/ast/bin $w cat)
		[[ $exp == "$got" ]] || err_exit "Test A: '$w' has the wrong output for path-bound builtins" \
			"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
		got=$($w /opt/ast/bin/cat)
		[[ $exp == "$got" ]] || err_exit "Test B: '$w' has the wrong output for path-bound builtins" \
			"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
		got=$(PATH=/opt/ast/bin:$PATH; $w cat)
		[[ $exp == "$got" ]] || err_exit "Test C: '$w' has the wrong output for path-bound builtins" \
			"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
		got=$(builtin cat; $w cat)
		[[ $exp == "$got" ]] || err_exit "Test D: '$w' has the wrong output for path-bound builtins" \
			"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
	fi
	exp=alias
	got=$(alias foo=bar; $w foo)
	[[ $exp == "$got" ]] || err_exit "'$w' has the wrong output for aliases" \
		"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
	exp=function
	got=$(foo() { true; }; $w foo)
	[[ $exp == "$got" ]] || err_exit "'$w' has the wrong output for POSIX functions" \
		"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
	got=$(function foo { true; }; $w foo)
	[[ $exp == "$got" ]] || err_exit "'$w' has the wrong output for KornShell functions" \
		"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
	got=$(autoload FooBar; $w FooBar)
	[[ $exp == "$got" ]] || err_exit "'$w' has the wrong output for undefined autoloaded functions" \
		"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
	echo 'FooBar() { true; }' > FooBar
	got=$(FPATH=. autoload FooBar; $w FooBar)
	[[ $exp == "$got" ]] || err_exit "'$w' has the wrong output for autoloaded POSIX functions" \
		"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
	echo 'function FooBar { true; }' > FooBar
	got=$(FPATH=. autoload FooBar; $w FooBar)
	[[ $exp == "$got" ]] || err_exit "'$w' has the wrong output for autoloaded KornShell functions" \
		"(expected $(printf %q "$exp"); got $(printf %q "$got"))"

	# The final few tests are for '-t -a'
	builtin -d cat
	if [[ $(set +x; PATH=/opt/ast/bin type cat 2>&1) == "cat is a shell builtin version of /opt/ast/bin/cat" ]]
	then	exp="alias
function
builtin
$($w -pa cat)"
		got=$(alias cat=false
			autoload cat
			PATH=/opt/ast/bin:$PATH $w -a cat)
		[[ $exp == "$got" ]] || err_exit "'$w -a' output is incorrect (cat command)" \
			"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
	fi
	if [[ -n $($w -pa time) ]]
	then	exp="keyword
alias
$($w -pa time)"
	else
		exp=$'keyword\nalias'
	fi
	got=$(alias time=nottime
		$w -a time)
	[[ $exp == "$got" ]] || err_exit "'$w -a' output is incorrect (time command)" \
		"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
done

# ======
# 'cd ../.foo' should not exclude the '.' in '.foo'
# https://bugzilla.redhat.com/889748
expect=$tmp/.ssh
actual=$( HOME=$tmp
	mkdir ~/.ssh 2>&1 &&
	cd ~/.ssh 2>&1 &&
	cd ../.ssh 2>&1 &&
	print -r -- "$PWD" )
[[ $actual == "$expect" ]] || err_exit 'changing to a hidden directory using a path that contains the parent directory (..) fails' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
expect=$tmp/.java
actual=$( mkdir "$tmp/java" "$tmp/.java" 2>&1 &&
	cd "$tmp/.java" 2>&1 &&
	cd ../.java 2>&1 &&
	pwd )
[[ $actual == "$expect" ]] || err_exit 'the dot (.) part of the directory name is being stripped' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

# check that we cannot cd into a regular file and get misbehaviour
: > "$tmp/regular_file"
expect=": cd: $tmp/regular_file: [Not a directory]"
actual=$(LC_ALL=C cd "$tmp/regular_file" 2>&1)
e=$?
[[ e -eq 1 && $actual == *"$expect" ]] || err_exit 'can cd into a regular file' \
	"(expected status 1 and msg ending in $(printf %q "$expect"), got status $e and msg $(printf %q "$actual"))"

# https://bugzilla.redhat.com/1102627
if	[[ $(id -u) == '0' ]]
then	warning "running as root: skipping tests involving directory search (x) permission"
else
mkdir -m 600 "$tmp/no_x_dir"
expect=": cd: $tmp/no_x_dir: [Permission denied]"
actual=$(LC_ALL=C cd "$tmp/no_x_dir" 2>&1)
e=$?
[[ e -eq 1 && $actual == *"$expect" ]] || err_exit 'can cd into a directory without x permission bit (absolute path arg)' \
	"(expected status 1 and msg ending in $(printf %q "$expect"), got status $e and msg $(printf %q "$actual"))"
expect=": cd: no_x_dir: [Permission denied]"
actual=$(cd "$tmp" 2>&1 && LC_ALL=C cd "no_x_dir" 2>&1)
e=$?
[[ e -eq 1 && $actual == *"$expect" ]] || err_exit 'can cd into a directory without x permission bit (relative path arg)' \
	"(expected status 1 and msg ending in $(printf %q "$expect"), got status $e and msg $(printf %q "$actual"))"
rmdir "$tmp/no_x_dir"	# on HP-UX, 'rm -rf $tmp' won't work unless we rmdir this or fix the perms
fi

# https://bugzilla.redhat.com/1133582
expect=$HOME
actual=$({ a=`cd; pwd`; } >&-; print -r -- "$a")
[[ $actual == "$expect" ]] || err_exit "'cd' broke old-form command substitution with outer stdout closed" \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
actual=$({ a=$(cd; pwd); } >&-; print -r -- "$a")
[[ $actual == "$expect" ]] || err_exit "'cd' broke new-form command substitution with outer stdout closed" \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

# CDPATH was not ignored by 'cd ./dir': https://github.com/ksh93/ksh/issues/151
expect=': cd: ./dev: [No such file or directory]'
actual=$( (CDPATH=/ LC_ALL=C cd -P ./dev && pwd) 2>&1 )
let "(e=$?)==1" && [[ $actual == *"$expect" ]] || err_exit "CDPATH not ignored by cd ./dir" \
	"(expected *$(printf %q "$expect") with status 1, got $(printf %q "$actual") with status $e)"

# ======
# 'readonly' should set the correct scope when creating variables in functions
unset foo
(
	function test_func
	{
		readonly foo="bar"
		[[ $foo = "bar" ]]
	}
	test_func
) || err_exit "readonly variable is not assigned a value inside functions"

# ======
# Test the output of nonstandard date formats with 'printf %T'
[[ $(printf '%(%l)T') == $(printf '%(%_I)T') ]] || err_exit 'date format %l is not the same as %_I'
[[ $(printf '%(%k)T') == $(printf '%(%_H)T') ]] || err_exit 'date format %k is not the same as %_H'
[[ $(printf '%(%f)T') == $(printf '%(%Y.%m.%d-%H:%M:%S)T') ]] || err_exit 'date format %f is not the same as %Y.%m.%d-%H:%M:%S'
[[ $(printf '%(%q)T') == $(printf '%(%Qz)T') ]] && err_exit 'date format %q is the same as %Qz'
[[ $(printf '%(%Z)T') == $(date '+%Z') ]] || err_exit "date format %Z is incorrect (expected $(date '+%Z'), got $(printf '%(%Z)T'))"

# Test manually specified blank and zero padding with 'printf %T'
(
	IFS=$'\n\t' # Preserve spaces in output
	for i in d e H I j J k l m M N S U V W y; do
		for f in ' ' 0; do
			if [[ $f == ' ' ]]; then
				padding='blank'
				specify='_'
			else
				padding='zero'
				specify='0'
			fi
			actual="$(printf "%(%${specify}${i})T" 'January 1 6AM 2001')"
			expect="${f}${actual:1}"
			[[ $expect != $actual ]] && err_exit "Specifying $padding padding with format '%$i' doesn't work (expected '$expect', got '$actual')"
		done
	done
	exit $Errors
)
Errors=$?  # Ensure error count survives subshell

# ======
# Test various AST getopts usage/manual outputs

OPTIND=1
USAGE=$'
[-s8?
@(#)$Id: foo (ksh93) 2020-07-16 $
]
[+NAME?foo - bar]
[+DESC?Baz.]
[x:xylophone?Lorem.]
[y:ypsilon?Ipsum.]
[z:zeta?Sit.]

[ name=value ... ]
-y [ name ... ]

[+SEE ALSO?\bgetopts\b(1)]
'

function testusage {
	getopts "$USAGE" dummy 2>&1
}

actual=$(testusage -\?)
expect='Usage: testusage [-xyz] [ name=value ... ]
   Or: testusage -y [ name ... ]
 Help: testusage [ --help | --man ] 2>&1'
[[ $actual == "$expect" ]] || err_exit "getopts: '-?' output" \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

actual=$(testusage --\?x)
expect='Usage: testusage [ options ] [ name=value ... ]
   Or: testusage -y [ name ... ]
 Help: testusage [ --help | --man ] 2>&1
OPTIONS
  -x, --xylophone Lorem.'
[[ $actual == "$expect" ]] || err_exit "getopts: '--?x' output" \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

actual=$(testusage --help)
expect='Usage: testusage [ options ] [ name=value ... ]
   Or: testusage -y [ name ... ]
 Help: testusage [ --help | --man ] 2>&1
OPTIONS
  -x, --xylophone Lorem.
  -y, --ypsilon   Ipsum.
  -z, --zeta      Sit.'
[[ $actual == "$expect" ]] || err_exit "getopts: '--help' output" \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

actual=$(testusage --man)
expect='NAME
  foo - bar

SYNOPSIS
  foo [ options ] [ name=value ... ]
  foo -y [ name ... ]

DESC
  Baz.

OPTIONS
  -x, --xylophone Lorem.
  -y, --ypsilon   Ipsum.
  -z, --zeta      Sit.

SEE ALSO
  getopts(1)

IMPLEMENTATION
  version         foo (ksh93) 2020-07-16'
[[ $actual == "$expect" ]] || err_exit "getopts: '--man' output" \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

# ======
# 'sleep -s' should work in interactive shells when seconds > 30.
if((!SHOPT_SCRIPTONLY));then
sleepsig="$tmp/sleepsig.sh"
cat >| "$sleepsig" << 'EOF'
sleep -s 31 &
sleep .1
kill -CONT $!
sleep .1
if kill -0 $!; then
	kill -TERM $! # Don't leave a lingering background process
	exit 1
else
	exit 0
fi
EOF
"$SHELL" -i "$sleepsig" 2> /dev/null || err_exit "'sleep -s' doesn't work with intervals of more than 30 seconds"
fi # !SHOPT_SCRIPTONLY

# ======
# floating point
# The number of seconds to sleep. The got granularity depends on the
# underlying system, normally around 1 millisecond.
SECONDS=0
sleep 0.1
got=$SECONDS
exp=0.1
(( got >= exp )) ||
	err_exit "sleep 0.1 should sleep for at least 0.1 second (expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# PnYnMnDTnHnMnS
# An ISO 8601 duration where at least one of the duration parts must be
# specified.
SECONDS=0
sleep 'P0Y0M0DT0H0M0.1S'
got=$SECONDS
exp=0.1
(( got >= exp )) ||
    err_exit "sleep 'P0Y0M0DT0H0M1S' should sleep for at least 1 second (expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# PnW   An ISO 8601 duration specifying n weeks.
# Sleep for 0 weeks
sleep P0W || err_exit "sleep does not recocgnize PnW"

# ======
# pnYnMnDTnHnMnS
# A case insensitive ISO 8601 duration except that M specifies months,
# m before s or S specifies minutes and after specifies milliseconds, u
# or U specifies microseconds, and n specifies nanoseconds.
SECONDS=0
sleep 'p0Y0M0DT0H0M0.1S'
got=$SECONDS
exp=0.1
(( got >= exp )) ||
    err_exit "sleep 'p0Y0M0DT0H0M1S' should sleep for at least 1 second (expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# date/time
#       Sleep until the date(1) compatible date/time.
today=$(date +"%Y-%m-%d")
# This should return immediately
sleep "$today" || err_exit "sleep does not recognize date parameter"

# ======
# -s Sleep until a signal or a timeout is received. If duration is
#    omitted or 0 then no timeout will be used.
SECONDS=0
sleep -s 0.1
got=$SECONDS
exp=0.1
(( got >= exp )) ||
    err_exit "sleep -s 1 should sleep for at least 1 second (expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# Verify unexpected arguments result in an error.
exp="sleep: one operand expected"
got=$(sleep 0 .3 2>&1)
[[ $got == $exp ]] || err_exit "unexpected arguments isn't an error (expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# Verify an invalid interval results in an error.
exp="sleep: 1sx: bad number"
got=$(sleep 1sx 2>&1)
[[ $got == $exp ]] || err_exit "invalid interval isn't an error (expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# Builtins should handle unrecognized options correctly
function test_usage
{
	while IFS= read -r bltin <&3
	do	case $bltin in
		fc | hist )
			((SHOPT_SCRIPTONLY)) && continue ;;
		echo | test | true | false | \[ | : | expr | */expr | getconf | */getconf | uname | */uname | catclose | catgets | catopen | Dt* | _Dt* | X* | login | newgrp )
			continue ;;
		/*/*)	expect="Usage: ${bltin##*/} "
			actual=$({ PATH=${bltin%/*}; "${bltin##*/}" --this-option-does-not-exist; } 2>&1) ;;
		*/*)	err_exit "strange path name in 'builtin' output: $(printf %q "$bltin")"
			continue ;;
		autoload | compound | float | functions | integer | nameref)
			bltin=typeset ;&
		*)	expect="Usage: $bltin "
			actual=$({ "${bltin}" --this-option-does-not-exist; } 2>&1) ;;
		esac
		[[ $actual == *"$expect"* ]] || err_exit "$bltin should show usage info on unrecognized options" \
				"(expected string containing $(printf %q "$expect"), got $(printf %q "$actual"))"
	done 3< <(builtin)
}; test_usage

# ======
# The 'alarm' builtin could make 'read' crash due to IFS table corruption caused by unsafe asynchronous execution.
# https://bugzilla.redhat.com/1176670
if	(builtin alarm) 2>/dev/null
then	got=$( { "$SHELL" -c '
		builtin alarm
		alarm -r alarm_handler +.005
		i=0
		function alarm_handler.alarm
		{
			let "(++i) > 20" && exit
		}
		while :; do
			echo cargo,odds and ends,jetsam,junk,wreckage,castoffs,sea-drift
		done | while IFS="," read arg1 arg2 arg3 arg4 junk; do
			:
		done
	'; } 2>&1)
	((!(e = $?))) || err_exit 'crash with alarm and IFS' \
		"(got status $e$( ((e>128)) && print -n /SIG && kill -l "$e"), $(printf %q "$got"))"
fi

# ======
# Verify that the POSIX 'test' builtin exits with status 2 when given an invalid binary operator.
for operator in '===' ']]'
do
	actual=$(test foo "$operator" foo 2>&1)
	actual_status=$?
	actual=${actual#*: }
	expect="test: $operator: unknown operator"
	expect_status=2
	[[ "$actual" = "$expect" ]] || err_exit "test $operator failed" \
		"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
	[[ "$actual_status" = "$expect_status" ]] ||
		err_exit "'test foo $operator foo' failed with the wrong exit status (expected $expect_status, got $actual_status)"
done

# ======
# Regression test for https://github.com/att/ast/issues/1402
#
# We throw away stderr because we only want the value of '$t', not the error text from running
# 'command' with an invalid flag.
exp='good'
got=$($SHELL -c 't=good; t=bad command -@; print $t' 2>/dev/null)
[[ $exp == $got ]] || err_exit "temp var assignment with 'command'" \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

# ======
# In ksh93v- 2013-10-10 alpha cd doesn't fail on directories without execute permission.
# Additionally, ksh93v- added a regression test for attempting to use cd on a file.
if	[[ $(id -u) == 0 ]]
then	warning "running as root: skipping test if cd fails on directory without execute permission"
else	mkdir "$tmp/noexecute"
	chmod -x "$tmp/noexecute"
	"$SHELL" -c "cd $tmp/noexecute" 2>/dev/null && err_exit "'cd' on directories without an execute bit doesn't fail"
fi
touch "$tmp/notadir"
$SHELL -c "cd $tmp/notadir" 2> /dev/null && err_exit "'cd' on a normal file doesn't fail"

# ======
# 'kill %' should fail with exit status 1
{ $SHELL -c 'kill %' ;} 2> /dev/null
got=$?
exp=1
[[ $got == $exp ]] || err_exit "'kill %' has the wrong exit status (expected '$exp'; got '$got')"

# ======
# 'cd -' should recognize the value of an overridden $OLDPWD variable
# https://github.com/ksh93/ksh/pull/249
# https://github.com/att/ast/issues/8

mkdir "$tmp/oldpwd" "$tmp/otherpwd"
exp=$tmp/oldpwd
OLDPWD=$exp
cd - > /dev/null
got=$PWD
[[ $got == "$exp" ]] || err_exit "cd - doesn't recognize overridden OLDPWD variable" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

cd "$tmp"
OLDPWD=$tmp/otherpwd
got=$(OLDPWD=$tmp/oldpwd cd -)
[[ $got == "$exp" ]] ||
	err_exit "cd - doesn't recognize overridden OLDPWD variable if it is overridden in new scope" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

function fn
{
	typeset OLDPWD=/tmp
	cd -
}
exp='/tmp'
got=$(OLDPWD=/bin fn)
[[ $got == "$exp" ]] ||
	err_exit "cd - doesn't recognize overridden OLDPWD variable if it is overridden in function scope" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

function fn
{
	typeset PWD=bug
	cd /tmp
	echo "$PWD"
}
exp='/tmp'
got=$(fn)
[[ $got == "$exp" ]] ||
	err_exit "PWD isn't set after cd if already set in function scope" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# $PWD should be set correctly after cd
exp="$PWD
$PWD"
got=$(echo $PWD; PWD=/tmp cd /dev; echo $PWD)
[[ $got == "$exp" ]] ||
	err_exit "PWD is incorrect after cd" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Test for $OLDPWD and/or $PWD leaking out of subshell
exp='/tmp /dev'
got=$(
	PWD=/dev
	OLDPWD=/tmp
	(
		cd /etc; cd /bin
		cd - > /dev/null
	)
	echo $OLDPWD $PWD
)
[[ $got == "$exp" ]] ||
	err_exit "OLDPWD and/or PWD leak out of subshell" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# $OLDPWD and $PWD should survive after being set in a subshare
exp='/etc /bin'
got=$(
	PWD=/dev
	OLDPWD=/tmp
	foo=${
		cd /etc; cd /bin
	}
	echo $OLDPWD $PWD
)
[[ $got == "$exp" ]] ||
	err_exit "OLDPWD and/or PWD fail to survive subshare" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# Test for bugs related to 'uname -d'
# https://github.com/att/ast/pull/1187
if	(builtin uname) 2>/dev/null
then	builtin uname
	exp=$(uname -o)

	# Test for a possible crash (to avoid crashing the script, fork the subshell)
	(
		ulimit -t unlimited 2> /dev/null
		uname -d > /dev/null
	) || err_exit "'uname -d' crashes"

	# 'uname -d' shouldn't change the output of 'uname -o'
	got=$(ulimit -t unlimited 2> /dev/null; uname -d > /dev/null; uname -o)
	[[ $exp == $got ]] || err_exit "'uname -d' changes the output of 'uname -o'" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
fi

# ======
# part of https://github.com/ksh93/ksh/issues/153
mkdir "$tmp/deleted"
cd "$tmp/deleted"
tmp=$tmp "$SHELL" -c 'cd /; rmdir "$tmp/deleted"'
exp=$PWD
got=$("$SHELL" -c 'cd /; echo "$OLDPWD"' 2>&1)
[[ $got == "$exp" ]] || err_exit "OLDPWD not correct after cd'ing from a nonexistent PWD" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
cd "$tmp"

# ======
# BUG_CMDSPEXIT
exp='ok1ok2ok3ok4ok5ok6ok7ok8ok9ok10ok11ok12end'
got=$(	readonly v=foo
	exec 2>/dev/null
	# All the "special builtins" below should fail, and not exit, so 'print end' is reached.
	# Ref.: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/contents.html
	# Left out are 'command exec /dev/null/nonexistent', where no shell follows the standard,
	# as well as 'command exit' and 'command return', because, well, obviously.
	command : </dev/null/no		|| print -n ok1
	command . /dev/null/no		|| print -n ok2
	command set +o bad@option	|| print -n ok3
	command shift $(($# + 1))	|| print -n ok4
	(unalias times; PATH=/dev/null; eval 'command times foo bar >/dev/null || print -n ok5')
	command trap foo bar baz quux	|| print -n ok6
	command unset v			|| print -n ok7
	command eval "("		|| print -n ok8
	command export v=baz		|| print -n ok9
	command readonly v=bar		|| print -n ok10
	command break			&& print -n ok11  # 'break' and 'continue' are POSIXly allowed to quietly...
	command continue		&& print -n ok12  # ..."succeed" if they are used outside of a loop :-/
	print end)
[[ $got == "$exp" ]] || err_exit "prefixing special builtin with 'command' does not stop it from exiting the shell on error" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
if((!SHOPT_SCRIPTONLY));then

# https://github.com/att/ast/issues/872
hist_leak=$tmp/hist_leak.sh
print 'ulimit -n 15' > "$hist_leak"
for ((i=0; i!=11; i++)) do
	print 'true foo\nhist -s foo=bar 2> /dev/null' >> "$hist_leak"
done
print 'print OK' >> "$hist_leak"
exp="OK"
got="$($SHELL -i "$hist_leak" 2>&1)"
[[ $exp == "$got" ]] || err_exit "file descriptor leak in hist builtin" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# File descriptor leak after hist builtin substitution error
hist_error_leak=$tmp/hist_error_leak.sh
print 'ulimit -n 15' > "$hist_error_leak"
for ((i=0; i!=11; i++)) do
	print 'hist -s no=yes 2> /dev/null' >> "$hist_error_leak"
done
print 'print OK' >> "$hist_error_leak"
exp="OK"
got="$($SHELL -i "$hist_error_leak" 2>&1)"
[[ $exp == "$got" ]] || err_exit "file descriptor leak after substitution error in hist builtin" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

fi # !SHOPT_SCRIPTONLY

# ======
# printf -v works as of 2021-11-18
((.sh.version >= 20211118)) && {
integer ver=.sh.version
exp=ok$'\f'0000$ver$'\n'
printf -v got 'ok\f%012d\n' $ver 2>/dev/null
[[ $got == "$exp" ]] || err_exit "printf -v not working" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
unset got
printf -v 'got[1][two][3]' 'ok\f%012d\n' $ver 2>/dev/null
[[ ${got[1]["two"][3]} == "$exp" ]] || err_exit "printf -v not working with array subscripts" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
unset got ver
}

# ======
# These are regression tests for the cd command's -e and -P flags
if ((.sh.version >= 20211205))
then
	mkdir -p "$tmp/failpwd"
	cd "$tmp/failpwd"
	"$SHELL" -c 'cd /; exec rmdir "$1"' x "$tmp/failpwd"
	cd -eP . 2>/dev/null && err_exit "cd -eP doesn't fail if \$PWD doesn't exist"
fi

cd "$tmp"
cd -P "$tmp/notadir" >/dev/null 2>&1
got=$?; exp=1
(( got == exp )) || err_exit "cd -P without -e fails with wrong exit status on nonexistent dir (expected $exp, got $got)"
if ((.sh.version >= 20211205))
then
	cd -eP "$tmp/notadir" >/dev/null 2>&1
	got=$?; exp=2
	(( got == exp )) || err_exit "cd -eP fails with wrong exit status on nonexistent dir (expected $exp, got $got)"
fi

OLDPWD="$tmp/baddir"
cd -P - >/dev/null 2>&1
got=$?; exp=1
(( got == exp )) || err_exit "cd -P without -e fails with wrong exit status on \$OLDPWD (expected $exp, got $got)"
if ((.sh.version >= 20211205))
then
	cd -eP - >/dev/null 2>&1
	got=$?; exp=2
	(( got == exp )) || err_exit "cd -eP fails with wrong exit status on \$OLDPWD (expected $exp, got $got)"
fi
cd "$tmp" || err_exit "couldn't change directory from nonexistent dir"

(set -o restricted; cd -P /) >/dev/null 2>&1
got=$?; exp=1
(( got == exp )) || err_exit "cd -P in restricted shell has wrong exit status (expected $exp, got $got)"
if ((.sh.version >= 20211205))
then
	(set -o restricted; cd -eP /) >/dev/null 2>&1
	got=$?; exp=2
	(( got == exp )) || err_exit "cd -eP in restricted shell has wrong exit status (expected $exp, got $got)"
fi
(set -o restricted; cd -\?) >/dev/null 2>&1
got=$?; exp=1
(( got == exp )) || err_exit "cd -\\? shows usage info in restricted shell and has wrong exit status (expected $exp, got $got)"

(cd -P '') >/dev/null 2>&1
got=$?; exp=1
(( got == exp )) || err_exit "cd -P to empty string has wrong exit status (expected $exp, got $got)"
if ((.sh.version >= 20211205))
then
	(cd -eP '') >/dev/null 2>&1
	got=$?; exp=2
	(( got == exp )) || err_exit "cd -eP to empty string has wrong exit status (expected $exp, got $got)"
fi

# ======
# ksh93v- accidentally broke the sleep builtin's support for
# using microseconds in the form of <num>U.
got=$(sleep 1U 2>&1) || err_exit "sleep builtin cannot handle microseconds in the form of <num>U (got $(printf %q "$got"))"

# ======
# Tests to avoid backporting an inherently broken 'read -p' compat hack from ksh 93v- or ksh2020
# Discussion: https://github.com/ksh93/ksh/issues/463
# ...It hangs in an infinite option parsing loop on encountering a stacked 'p' option
(echo ok |& read -pt1 && [[ $REPLY == ok ]]) &
test_pid=$!
(sleep 1; kill -s KILL "$test_pid" 2>/dev/null) &	# another bg job to kill frozen test job
sleep_pid=$!
{ wait "$test_pid"; } 2>/dev/null			# get job's exit status, suppressing signal messages
((!(e = $?))) || err_exit "'read -pt1' hangs (got status $e$( ((e>128)) && print -n /SIG && kill -l "$e"))"
kill "$sleep_pid" 2>/dev/null
# ...It failed to implement co-process compatibility for 'read -p .dot.varname' or 'read -p varname?prompt'
(echo ok |& read -p .sh.foo"?dummy prompt: " && [[ ${.sh.foo} == ok ]]) </dev/null \
|| err_exit "'read -p' co-process usage is not fully backward compatible with ksh 93u+"

# Backported ksh93v- 2014-06-25 test for eval bug when called
# from . script in a startup file.
if((!SHOPT_SCRIPTONLY));then
print $'eval : foo\nprint ok' > "$tmp/evalbug"
print ". $tmp/evalbug" > "$tmp/envfile"
[[ $(ENV=$tmp/envfile "$SHELL" -i -c : 2> /dev/null) == ok ]] || err_exit 'eval inside dot script called from profile file not working'
fi # !SHOPT_SCRIPTONLY

# Backported ksh93v- 2013-03-18 test for 'read -A', where
# IFS sets the delimiter to a newline while -d specifies
# no delimiter (-d takes priority over IFS).
if ((SHOPT_BRACEPAT)); then
	got=$(printf %s\\n {a..f} | IFS=$'\n' read -rd '' -A a; typeset -p a)
	exp=$'typeset -a a=($\'a\\nb\\nc\\nd\\ne\\nf\\n\')'
	[[ $got == "$exp" ]] || err_exit "IFS overrides the delimiter specified by the read command's -d option" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
fi

# The read builtin's -a and -A flags should function identically
read_a_test=$tmp/read_a_test.sh
cat > "$read_a_test" << 'EOF'
. "${SHTESTS_COMMON}"
exp=foo
exp1=bar
exp2=baz
read -a foo_a <<< 'foo bar baz'
if [[ ${foo_a[0]} != ${exp} ]] || [[ ${foo_a[1]} != ${exp1} ]] || [[ ${foo_a[2]} != ${exp2} ]]
then
	err_exit "read -a fails to create array with first use" \
		"(foo_a[0] is $(printf %q "${foo_a[0]}"), foo_a[1] is $(printf %q "${foo_a[1]}"), foo_a[2] is $(printf %q "${foo_a[2]}"))"
fi
unset foo_a
read -a foo_a <<< 'foo bar baz'
if [[ ${foo_a[0]} != ${exp} ]] || [[ ${foo_a[1]} != ${exp1} ]] || [[ ${foo_a[2]} != ${exp2} ]]
then
	err_exit "read -a fails to create array with second use" \
		"(foo_a[0] is $(printf %q "${foo_a[0]}"), foo_a[1] is $(printf %q "${foo_a[1]}"), foo_a[2] is $(printf %q "${foo_a[2]}"))"
fi
read -A foo_A <<< 'foo bar baz'
if [[ ${foo_A[0]} != ${exp} ]] || [[ ${foo_A[1]} != ${exp1} ]] || [[ ${foo_A[2]} != ${exp2} ]]
then
	err_exit "read -A fails to create array with first use" \
		"(foo_A[0] is $(printf %q "${foo_A[0]}"), foo_A[1] is $(printf %q "${foo_A[1]}"), foo_A[2] is $(printf %q "${foo_A[2]}"))"
fi
unset foo_A
read -A foo_A <<< 'foo bar baz'
if [[ ${foo_A[0]} != ${exp} ]] || [[ ${foo_A[1]} != ${exp1} ]] || [[ ${foo_A[2]} != ${exp2} ]]
then
	err_exit "read -A fails to create array with second use" \
		"(foo_A[0] is $(printf %q "${foo_A[0]}"), foo_A[1] is $(printf %q "${foo_A[1]}"), foo_A[2] is $(printf %q "${foo_A[2]}"))"
fi
exit $Errors
EOF
"$SHELL" "$read_a_test"
let Errors+=$?

# ======
# Most built-ins should handle --version
while IFS= read -r bltin <&3
do	case $bltin in
	# TODO: remove 'alarm' below when it's properly self-documented
	alarm | echo | test | true | false | \[ | : | catclose | catgets | catopen | Dt* | _Dt* | X* | login | newgrp )
		continue ;;
	fc | hist )
		((SHOPT_SCRIPTONLY)) && continue ;;
	esac
	got=$({ "$bltin" --version; } 2>&1)  # the extra { } are needed for 'redirect'
	[[ $got == "  version  "* ]] || err_exit "$bltin does not support --version (got $(printf %q "$got"))"
done 3< <(builtin)

# ======
# https://github.com/ksh-community/ksh/issues/19
# https://github.com/ksh93/ksh/issues/602
cd /
cd
[[ $PWD == "$HOME" ]] || err_exit "'cd' does not chdir to \$HOME"

HOME=/dev cd
[[ $PWD == /dev ]] || err_exit "'cd' does not chdir to \$HOME (preceding assignment)"

function fn
{
	typeset HOME=/tmp
	cd
}
fn
unset -f fn
[[ $PWD == /tmp ]] || err_exit "'cd' does not chdir to \$HOME (local assignment)"

# ======
# Double evaluation of arithmetic expression passed to float conversion operators in printf
# https://github.com/ksh93/ksh/issues/610#issuecomment-1480530518
typeset -si i
unset c
for c in o x X u U d D i a e f g A E F G
do	i=0
	printf "%$c" ++i
	let i==1 || err_exit "printf %$c: bad arithmetic evaluation (expected 1, got $i)"
done >/dev/null
unset c i

# ======
# segfault with read -rd $'\200' in multibyte locales
# https://github.com/ksh93/ksh/issues/590
exp='first line'
got=$(set +x; { "$SHELL" -c 'read -rd $'\''\200'\'' && echo "$REPLY"';} 2>&1 <<<$'first line\200second line')
[[ e=$? -eq 0 && $got == "$exp" ]] || err_exit "read -rd \$'\\200'" \
	"(expected status 0, '$exp';" \
	"got status $e$( ((e>128)) && print -n /SIG && kill -l "$e"), $(printf %q "$got"))"

# ======
# printf -v varname %B overwrites/truncates data
# https://github.com/ksh93/ksh/issues/608
exp='YWJjZGVmZ2g= / abcdefgh'
got=$(
	typeset  -b foo
	read -N8 foo <<<abcdefghijklmnop &&
	printf -v got '%s / %B\n' "$foo" foo &&
	print -rn "$got"
)
[[ e=$? -eq 0 && $got == "$exp" ]] || err_exit "read -N8 && printf -v %B" \
	"(expected status 0, '$exp';" \
	"got status $e$( ((e>128)) && print -n /SIG && kill -l "$e"), $(printf %q "$got"))"
# one proposed fix used the stack, breaking %q
s='one two'
exp="'$s' / '$s' / '$s'"
printf -v got "%q / %q / %q" "$s" "$s" "$s"
[[ $got == "$exp" ]] || err_exit "printf repeated %q" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
exit $((Errors<125?Errors:125))

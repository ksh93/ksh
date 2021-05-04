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

unset HISTFILE

function fun
{
	while  redirect 3>&1
	do	break
	done 2>   /dev/null
	print -u3 good
}
print 'read -r a; print -r -u$1 -- "$a"' > $tmp/mycat
chmod 755 $tmp/mycat
for ((i=3; i < 10; i++))
do
	eval "a=\$(print foo | $tmp/mycat" $i $i'>&1 > /dev/null |cat)' 2> /dev/null
	[[ $a == foo ]] || err_exit "bad file descriptor $i in comsub script"
done
exec 3> /dev/null
[[ $(fun) == good ]] || err_exit 'file 3 closed before subshell completes'
exec 3>&-
print foo > file1
print bar >> file1
if	[[ $(<file1) != $'foo\nbar' ]]
then	err_exit 'append (>>) not working'
fi
set -o noclobber
exec 3<> file1
read -u3 line
exp=foo
if	[[ $line != $exp ]]
then	err_exit "read on <> fd failed -- expected '$exp', got '$line'"
fi
if	( 4> file1 ) 2> /dev/null
then	err_exit 'noclobber not causing exclusive open'
fi
set +o noclobber

FDFS=(
	( dir=/proc/self/fd	semantics='open'	)
	( dir=/proc/$$/fd	semantics='open'	)
	( dir=/dev/fd		semantics='open|dup'	)
	( dir=/dev/fd		semantics='dup'	)
)
for ((fdfs=0; fdfs<${#FDFS[@]}-1; fdfs++))
do	[[ -e ${FDFS[fdfs].dir} ]] && { command : > ${FDFS[fdfs].dir}/1; } 2>/dev/null >&2 && break
done

exec 3<> file1
if	redirect 4< ${FDFS[fdfs].dir}/3
then	read -u3 got
	read -u4 got
	exp='foo|bar'
	case $got in
	foo)	semantics='open' ;;
	bar)	semantics='dup' ;;
	*)	semantics='failed' ;;
	esac
	[[ $semantics == @(${FDFS[fdfs].semantics}) ]] || err_exit "'4< ${FDFS[fdfs].dir}/3' $semantics semantics instead of ${FDFS[fdfs].semantics} -- expected '$exp', got '$got'"
fi

# 2004-11-25 ancient /dev/fd/N redirection bug fix
got=$(
	set +x
	{
		print -n 1
		print -n 2 > ${FDFS[fdfs].dir}/2
		print -n 3
		print -n 4 > ${FDFS[fdfs].dir}/2
	}  2>&1
)
exp='1234|4'
case $got in
1234)	semantics='dup' ;;
4)	semantics='open' ;;
*)	semantics='failed' ;;
esac
[[ $semantics == @(${FDFS[fdfs].semantics}) ]] || err_exit "${FDFS[fdfs].dir}/N $semantics semantics instead of ${FDFS[fdfs].semantics} -- expected '$exp', got '$got'"

cat > close0 <<\!
exec 0<&-
echo $(./close1)
!
print "echo abc" > close1
chmod +x close0 close1
x=$(./close0)
if	[[ $x != "abc" ]]
then	err_exit "picked up file descriptor zero for opening script file"
fi
cat > close0 <<\!
	for ((i=0; i < 1100; i++))
	do	exec 4< /dev/null
		read -u4
	done
	exit 0
!
./close0 2> /dev/null || err_exit "multiple exec 4< /dev/null can fail"
$SHELL -c '
	trap "rm -f in out" EXIT
	for ((i = 0; i < 1000; i++))
	do	print -r -- "This is a test"
	done > in
	> out
	exec 1<> out
	builtin cat
	print -r -- "$(<in)"
	cmp -s in out'  2> /dev/null
[[ $? == 0 ]] || err_exit 'builtin cat truncates files'
cat >| script <<-\!
print hello
( exec 3<&- 4<&-)
exec 3<&- 4<&-
print world
!
chmod +x script
[[ $( $SHELL ./script) == $'hello\nworld' ]] || err_exit 'closing 3 & 4 causes script to fail'
( exec  > '' ) 2> /dev/null  && err_exit '> "" does not fail'
unset x
( exec > ${x} ) 2> /dev/null && err_exit '> $x, where x null does not fail'
exec <<!
foo
bar
!
( exec 0< /dev/null)
read line
if	[[ $line != foo ]]
then	err_exit 'file descriptor not restored after exec in subshell'
fi
exec 3>&- 4>&-
[[ $( {
	read -r line; print -r -- "$line"
	(
	        read -r line; print -r -- "$line"
	) & wait
	while	read -r line
        do	print -r -- "$line"
	done
 } << !
line 1
line 2
line 3
!) == $'line 1\nline 2\nline 3' ]] || err_exit 'read error with subshells'
# 2004-05-11 bug fix
cat > $tmp/1 <<- ++EOF++
	script=$tmp/2
	trap "rm -f \$script" EXIT
	exec 9> \$script
	for ((i=3; i<9; i++))
	do	eval "while read -u\$i; do : ; done \$i</dev/null"
		print -u9 "exec \$i< /dev/null"
	done
	for ((i=0; i < 60; i++))
	do	print -u9 -f "%.80c\n"  ' '
	done
	print -u9 'print ok'
	exec 9<&-
	chmod +x \$script
	\$script
++EOF++
chmod +x $tmp/1
[[ $($SHELL  $tmp/1) == ok ]]  || err_exit "parent i/o causes child script to fail"
# 2004-12-20 redirection loss bug fix
cat > $tmp/1 <<- \++EOF++
	function a
	{
		trap 'print ok' EXIT
		: > /dev/null
	}
	a
++EOF++
chmod +x $tmp/1
[[ $($tmp/1) == ok ]] || err_exit "trap on EXIT loses last command redirection"
print > /dev/null {n}> $tmp/1
[[ ! -s $tmp/1 ]] && newio=1
if	[[ $newio && $(print hello | while read -u$n; do print $REPLY; done {n}<&0) != hello ]]
then	err_exit "{n}<&0 not working with for loop"
fi
[[ $({ read -r; read -u3 3<&0; print -- "$REPLY" ;} <<!
hello
world
!) == world ]] || err_exit 'I/O not synchronized with <&'
x="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNSPQRSTUVWXYZ1234567890"
for ((i=0; i < 62; i++))
do	printf "%.39c\n"  ${x:i:1}
done >  $tmp/seek
if	redirect 3<> $tmp/seek
then	[[ $(3<#) -eq 0 ]] || err_exit "not at position 0"
	[[ $(3<# ((EOF))) -eq 40*62 ]] || err_exit "not at end-of-file"
	redirect 3<# ((40*8)) || err_exit "absolute seek fails"
	read -u3
	[[ $REPLY == +(i) ]] || err_exit "expected iiii..., got $REPLY"
	[[ $(3<#) == $(3<# ((CUR)) ) ]] || err_exit '$(3<#)!=$(3<#((CUR)))'
	redirect 3<# ((CUR+80))
	read -u3
	[[ $REPLY == {39}(l) ]] || err_exit "expected lll..., got $REPLY"
	redirect 3<# ((EOF-80))
	read -u3
	[[ $REPLY == +(9) ]] || err_exit "expected 999..., got $REPLY"
	redirect 3># ((80))
	print -u3 -f "%.39c\n"  @
	redirect 3># ((80))
	read -u3
	[[ $REPLY == +(@) ]] || err_exit "expected @@@..., got $REPLY"
	read -u3
	[[ $REPLY == +(d) ]] || err_exit "expected ddd..., got $REPLY"
	redirect 3># ((EOF))
	print -u3 -f "%.39c\n"  ^
	[[ $(3<# ((CUR-0))) -eq 40*63 ]] || err_exit "not at extended end-of-file"
	redirect 3<# ((40*62))
	read -u3
	[[ $REPLY == +(^) ]] || err_exit "expected ddd..., got $REPLY"
	redirect 3<# ((0))
	redirect 3<# *jjjj*
	read -u3
	[[  $REPLY == {39}(j) ]] || err_exit "<# pattern failed"
	[[ $(redirect 3<## *llll*) == {39}(k) ]] || err_exit "<## pattern not saving standard output"
	read -u3
	[[  $REPLY == {39}(l) ]] || err_exit "<## pattern failed to position"
	redirect 3<# *abc*
	read -u3 && err_exit "not found pattern not positioning at eof"
	cat $tmp/seek | read -r <# *WWW*
	[[ $REPLY == *WWWWW* ]] || err_exit '<# not working for pipes'
	{ < $tmp/seek <# ((2358336120)) ;} 2> /dev/null || err_exit 'long seek not working'
else	err_exit "$tmp/seek: cannot open for reading"
fi
redirect 3<&- || 'cannot close 3'
for ((i=0; i < 62; i++))
do	printf "%.39c\n"  ${x:i:1}
done >  $tmp/seek
if	redirect {n}<> $tmp/seek
then	{ redirect {n}<#((EOF)) ;} 2> /dev/null || err_exit '{n}<# not working'
	if	$SHELL -c '{n}</dev/null' 2> /dev/null
	then	[[ $({n}<#) -eq 40*62 ]]  || err_exit '$({n}<#) not working'
	else	err_exit 'not able to parse {n}</dev/null'
	fi
fi
$SHELL -ic '
{
    print -u2  || exit 2
    print -u3  || exit 3
    print -u4  || exit 4
    print -u5  || exit 5
    print -u6  || exit 6
    print -u7  || exit 7
    print -u8  || exit 8
    print -u9  || exit 9
}  3> /dev/null 4> /dev/null 5> /dev/null 6> /dev/null 7> /dev/null 8> /dev/null 9> /dev/null' > /dev/null 2>&1
exitval=$?
(( exitval ))  && err_exit  "print to unit $exitval failed"
$SHELL -c "{ > $tmp/1 ; date;} >&- 2> /dev/null" > $tmp/2
[[ -s $tmp/1 || -s $tmp/2 ]] && err_exit 'commands with standard output closed produce output'
$SHELL -c "$SHELL -c ': 3>&1' 1>&- 2>/dev/null" && err_exit 'closed standard output not passed to subshell'
[[ $(cat  <<- \EOF | $SHELL
	do_it_all()
	{
		dd 2>/dev/null  # not a ksh93 builtin
	 	return $?
	}
	do_it_all ; exit $?
	hello world
EOF) == 'hello world' ]] || err_exit 'invalid readahead on stdin'
$SHELL -c 'exec 3>; /dev/null'  2> /dev/null && err_exit '>; with exec should be an error'
$SHELL -c ': 3>; /dev/null'  2> /dev/null || err_exit '>; not working with at all'
print hello > $tmp/1
$SHELL -c "false >; $tmp/1"
status=$?
(( status == 1 )) || err_exit "unexpected exit status" \
	"(expected 1, got $status)"
exp='hello'
got=$(<$tmp/1)
[[ $got == "$exp" ]] || err_exit '>; not preserving file on failure' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
$SHELL -c "sed -e 's/hello/hello world/' $tmp/1" >; $tmp/1
status=$?
(( status == 0 )) || err_exit "unexpected exit status" \
	"(expected 0, got $status)"
exp='hello world'
got="$(<$tmp/1)"
[[ $got == "$exp" ]] || err_exit '>; not updating file on success' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

$SHELL -c 'exec 3<>; /dev/null'  2> /dev/null && err_exit '<>; with exec should be an error'
$SHELL -c ': 3<>; /dev/null'  2> /dev/null || err_exit '<>; not working with at all'
print $'hello\nworld' > $tmp/1
if      ! $SHELL -c "false <>; $tmp/1"  2> /dev/null
then    [[ $(<$tmp/1) == $'hello\nworld' ]] || err_exit '<>; not preserving file on failure'
fi
if	! $SHELL -c "head -1 $tmp/1" <>; $tmp/1  2> /dev/null
then	[[ $(<$tmp/1) == hello ]] || err_exit '<>; not truncating file on success of head'
fi
print $'hello\nworld' > $tmp/1
if	! $SHELL -c head  < $tmp/1 <#((6)) <>; $tmp/1  2> /dev/null
then	[[ $(<$tmp/1) == world ]] || err_exit '<>; not truncating file on success of behead'
fi

unset y
read -n1 y <<!
abc
!
if      [[ $y != a ]]
then    err_exit  'read -n1 not working'
fi
unset a
{ read -N3 a; read -N1 b;}  <<!
abcdefg
!
[[ $a == abc ]] || err_exit 'read -N3 here-document not working'
[[ $b == d ]] || err_exit 'read -N1 here-document not working'
read -n3 a <<!
abcdefg
!
[[ $a == abc ]] || err_exit 'read -n3 here-document not working'
(print -n a; sleep .1; print -n bcde) | { read -N3 a; read -N1 b;}
[[ $a == abc ]] || err_exit 'read -N3 from pipe not working'
[[ $b == d ]] || err_exit 'read -N1 from pipe not working'
(print -n a; sleep .1; print -n bcde) 2>/dev/null |read -n3 a
[[ $a == a ]] || err_exit 'read -n3 from pipe not working'
if	mkfifo $tmp/fifo 2> /dev/null
then	(print -n a; sleep .2; print -n bcde) > $tmp/fifo &
	{
	read -u5 -n3 -t1 a || err_exit 'read -n3 from fifo timed out'
	read -u5 -n1 -t1 b || err_exit 'read -n1 from fifo timed out'
	} 5< $tmp/fifo
	exp=a
	got=$a
	[[ $got == "$exp" ]] || err_exit "read -n3 from fifo failed -- expected '$exp', got '$got'"
	exp=b
	got=$b
	[[ $got == "$exp" ]] || err_exit "read -n1 from fifo failed -- expected '$exp', got '$got'"
	rm -f $tmp/fifo
	wait
	mkfifo $tmp/fifo 2> /dev/null
	(print -n a; sleep .2; print -n bcde) > $tmp/fifo &
	{
	read -u5 -N3 -t1 a || err_exit 'read -N3 from fifo timed out'
	read -u5 -N1 -t1 b || err_exit 'read -N1 from fifo timed out'
	} 5< $tmp/fifo
	exp=abc
	got=$a
	[[ $got == "$exp" ]] || err_exit "read -N3 from fifo failed -- expected '$exp', got '$got'"
	exp=d
	got=$b
	[[ $got == "$exp" ]] || err_exit "read -N1 from fifo failed -- expected '$exp', got '$got'"
	wait
fi
(
	print -n 'prompt1: '
	sleep .01
	print line2
	sleep .01
	print -n 'prompt2: '
	sleep .01
) | {
	read -t1 -n 1000 line1
	read -t1 -n 1000 line2
	read -t1 -n 1000 line3
	read -t1 -n 1000 line4
}
[[ $? == 0 ]]		 	&& err_exit 'should have timed out'
[[ $line1 == 'prompt1: ' ]] 	|| err_exit "line1 should be 'prompt1: '"
[[ $line2 == line2 ]]		|| err_exit "line2 should be line2"
[[ $line3 == 'prompt2: ' ]]	|| err_exit "line3 should be 'prompt2: '"
[[ ! $line4 ]]			|| err_exit "line4 should be empty"

if	$SHELL -c "export LC_ALL=C.UTF-8; c=$'\342\202\254'; [[ \${#c} == 1 ]]" 2>/dev/null
then	lc_utf8=C.UTF-8
else	lc_utf8=''
fi

typeset -a e o=(-n2 -N2)
integer i
set -- \
	'a'	'bcd'	'a bcd'	'ab cd' \
	'ab'	'cd'	'ab cd'	'ab cd' \
	'abc'	'd'	'ab cd'	'ab cd' \
	'abcd'	''	'ab cd'	'ab cd'
while	(( $# >= 3 ))
do	a=$1
	b=$2
	e[0]=$3
	e[1]=$4
	shift 4
	for ((i = 0; i < 2; i++))
	do	for lc_all in C $lc_utf8
		do	g=$(LC_ALL=$lc_all $SHELL -c "{ print -n '$a'; sleep .02; print -n '$b'; sleep .02; } | { read ${o[i]} a; print -n \$a; read a; print -n \ \$a; }")
			[[ $g == "${e[i]}" ]] || err_exit "LC_ALL=$lc_all read ${o[i]} from pipe '$a $b' failed -- expected '${e[i]}', got '$g'"
		done
	done
done

if	[[ $lc_utf8 ]]
then	export LC_ALL=$lc_utf8
	typeset -a c=( '' 'A' $'\303\274' $'\342\202\254' )
	integer i w
	typeset o
	if	(( ${#c[2]} == 1 && ${#c[3]} == 1 ))
	then	for i in 1 2 3
		do	for o in n N
			do	for w in 1 2 3
				do	print -nr "${c[w]}" | read -${o}${i} g
					if	[[ $o == N ]] && (( i > 1 ))
					then	e=''
					else	e=${c[w]}
					fi
					[[ $g == "$e" ]] || err_exit "read -${o}${i} failed for '${c[w]}' -- expected '$e', got '$g'"
				done
			done
		done
	fi
fi
LC_ALL=C command true  # restore correct shellquoting on old ksh: https://github.com/ksh93/ksh/issues/5

file=$tmp/file
(
: disabling xtrace so we can redirect stderr for this test
set +x
redirect 5>$file 2>&5
print -u5 -f 'This is a test\n'
print -u2 OK
)
exp=$'This is a test\nOK'
got=$(< $file)
[[ $got == $exp ]] || err_exit "output garbled when stderr is duped -- expected $(printf %q "$exp"), got $(printf %q "$got")"
print 'hello world' > $file
1<>; $file  1># ((5))
(( $(wc -c < $file) == 5 )) || err_exit "$file was not truncate to 5 bytes"

$SHELL -c "PS4=':2:'
	exec 1> $tmp/21.out 2> $tmp/22.out
	set -x
	printf ':1:A:'
	print \$(:)
	print :1:Z:" 1> $tmp/11.out 2> $tmp/12.out
[[ -s $tmp/11.out ]] && err_exit "standard output leaked past redirection"
[[ -s $tmp/12.out ]] && err_exit "standard error leaked past redirection"
exp=$':1:A:\n:1:Z:'
got=$(<$tmp/21.out)
[[ $exp == "$got" ]] || err_exit "standard output garbled -- expected $(printf %q "$exp"), got $(printf %q "$got")"
exp=$':2:printf :1:A:\n:2::\n:2:print\n:2:print :1:Z:'
got=$(<$tmp/22.out)
[[ $exp == "$got" ]] || err_exit "standard error garbled -- expected $(printf %q "$exp"), got $(printf %q "$got")"

tmp=$tmp $SHELL 2> /dev/null -c 'exec 3<&1 ; exec 1<&- ; exec > $tmp/outfile;print foobar' || err_exit 'exec 1<&- causes failure'
[[ $(<$tmp/outfile) == foobar ]] || err_exit 'outfile does not contain foobar'
[[ $(<$tmp/outfile) == foobar ]] <&- >&- 2>&- || err_exit '$(<file) does not work with stdin, stdout and/or stderr closed'

print hello there world > $tmp/foobar
sed  -e 's/there //' $tmp/foobar  >; $tmp/foobar
[[ $(<$tmp/foobar) == 'hello world' ]] || err_exit '>; redirection not working on simple command'
print hello there world > $tmp/foobar
{ sed  -e 's/there //' $tmp/foobar;print done;} >; $tmp/foobar 
[[ $(<$tmp/foobar) == $'hello world\ndone' ]] || err_exit '>; redirection not working for compound command'
print hello there world > $tmp/foobar
$SHELL -c "sed  -e 's/there //' $tmp/foobar  >; $tmp/foobar"
[[ $(<$tmp/foobar) == 'hello world' ]] || err_exit '>; redirection not working with -c on a simple command'

binfalse=$(whence -p false)
for	(( i=1; i < 50; i++ ))
do      out=$("$binfalse" 2>/dev/null)
	if	(( $? == 0 ))
	then    err_exit 'wrong error code with redirection'
		break
	fi
done

for i in 1
do	:
done	{n}< /dev/null
[[ -r /dev/fd/$n ]] &&  err_exit "file descriptor n=$n left open after {n}<"

n=$( exec {n}< /dev/null; print -r -- $n)
[[ -r /dev/fd/$n ]] && err_exit "file descriptor n=$n left open after subshell"

# ======
# Truncating a file using <> and >#num
# https://github.com/att/ast/issues/61
# The <>; redirection operator didn't work correctly in -c scripts
# https://github.com/att/ast/issues/9
# https://github.com/ksh93/ksh/issues/278

# the extra '3>&2 3>&-' is to verify it all keeps working with multiple redirections
typeset -A exp=(
	['3>&2 3>&- 1<>;nums >#5']=$'1\n2\n3\n4'
	[': 3>&2 3>&- 1<>;nums >#5']=$'1\n2\n3\n4'
	['{ :; } 3>&2 3>&- 1<>;nums >#5']=$'1\n2\n3\n4'
	['(:) 3>&2 3>&- 1<>;nums >#5']=$'1\n2\n3\n4'
	['while false; do :; done 3>&2 3>&- 1<>;nums >#5']=$'1\n2\n3\n4'
	['until true; do :; done 3>&2 3>&- 1<>;nums >#5']=$'1\n2\n3\n4'
	['if false; then :; fi 3>&2 3>&- 1<>;nums >#5']=$'1\n2\n3\n4'
	['case x in y) :;; esac 3>&2 3>&- 1<>;nums >#5']=$'1\n2\n3\n4'
	['fn() { :; } 3>&2 3>&- 1<>;nums >#5; fn']=$'1\n2\n3\n4'
	['echo x 3>&2 3>&- 1<>;nums']=x
	['{ echo x; } 3>&2 3>&- 1<>;nums']=x
	['(echo x) 3>&2 3>&- 1<>;nums']=x
	['while :; do echo x; break; done 3>&2 3>&- 1<>;nums']=x
	['until ! :; do echo x; break; done 3>&2 3>&- 1<>;nums']=x
	['if :; then echo x; fi 3>&2 3>&- 1<>;nums']=x
	['case x in x) echo x;; esac 3>&2 3>&- 1<>;nums']=x
	['fn() { echo x; } 3>&2 3>&- 1<>;nums; fn']=x
)
nums=$'1\n2\n3\n4\n5\n6\n7\n8\n9\n10'
for script in "${!exp[@]}"
do
	echo "$nums" >nums
	eval "$script"
	got=$(<nums)
	[[ $got == "${exp[$script]}" ]] || err_exit "IOREWRITE: '$script' failed in main shell" \
		"(expected $(printf %q "${exp[$script]}"), got $(printf %q "$got"))"
	echo "$nums" >nums
	eval "( $script )"
	got=$(<nums)
	[[ $got == "${exp[$script]}" ]] || err_exit "IOREWRITE: '$script' failed in subshell" \
		"(expected $(printf %q "${exp[$script]}"), got $(printf %q "$got"))"
	echo "$nums" >nums
	"$SHELL" -c "$script"
	got=$(<nums)
	[[ $got == "${exp[$script]}" ]] || err_exit "IOREWRITE: '$script' failed as -c script" \
		"(expected $(printf %q "${exp[$script]}"), got $(printf %q "$got"))"
	echo "$nums" >nums
	echo "$script" >test.sh
	"$SHELL" test.sh
	got=$(<nums)
	[[ $got == "${exp[$script]}" ]] || err_exit "IOREWRITE: '$script' failed as regular script" \
		"(expected $(printf %q "${exp[$script]}"), got $(printf %q "$got"))"
done
unset exp script

got=$("$SHELL" -c $'trap "echo bye" 0\n{ echo hi; } >/dev/null' 2>&1)
exp=bye
[[ $got == "$exp" ]] || err_exit "redirection in last -c script command persists for trap" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

echo $'trap "echo bye" 0\n{ echo hi; } >/dev/null' >test.sh
got=$("$SHELL" test.sh 2>&1)
exp=bye
[[ $got == "$exp" ]] || err_exit "redirection in last regular script command persists for trap" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# Exit behaviour of 'exec', 'command exec', 'redirect' on redirections

actual=$(exec 2>&- 3>&2; echo should not reach)
[[ -z $actual ]] || err_exit "redirection error in 'exec' does not cause exit"
actual=$(command exec 2>&- 3>&2; echo ok)
[[ $actual == ok ]] || err_exit "redirection error in 'command exec' causes exit"
actual=$(redirect 2>&- 3>&2; echo ok)
[[ $actual == ok ]] || err_exit "redirection error in 'redirect' causes exit"

# Test that 'redirect' does not accept non-redir args
expect=$'*: redirect: incorrect syntax: /dev/null/foo\n status = 2'
actual=$( (redirect /dev/null/foo) 2>&1; echo " status = $?" );
[[ $actual == $expect ]] || err_exit 'redirect command accepts non-redirection argument' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
actual=$( (redirect /dev/null/foo >$tmp/wrong_redirect) 2>&1; echo " status = $?" )
[[ $actual == $expect ]] || err_exit 'redirect command accepts non-redirection argument along with redirection' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
[[ -e $tmp/wrong_redirect ]] && err_exit "redirect command executes redirections before checking arguments"

# ======
# Process substitution

# An output process substitution should work when combined with a redirection.
result=$("$SHELL" -c 'echo ok > >(sed s/ok/good/); wait' 2>&1)
[[ $result == good ]] || err_exit 'process substitution does not work with redirections' \
				"(expected 'good', got $(printf %q "$result"))"

# Process substitution in an interactive shell shouldn't print the
# process ID of the asynchronous process.
result=$("$SHELL" -ic 'echo >(true) >/dev/null' 2>&1)
[[ -z $result ]] || err_exit 'interactive shells print a PID during process substitution' \
				"(expected '', got $(printf %q "$result"))"

# ======
# Out of range file descriptors shouldn't cause 'read -u' to segfault
"$SHELL" -c 'read -u2000000' 2> /dev/null
[[ $? == 1 ]] || err_exit "Large file descriptors cause 'read -u' to crash"

# Separately test numbers outside of the 32-bit limit as well
"$SHELL" -c 'read -u2000000000000' 2> /dev/null
[[ $? == 1 ]] || err_exit "File descriptors larger than the 32-bit limit cause 'read -u' to crash"

# Negative numbers shouldn't segfault either
"$SHELL" -c 'read -u-2000000' 2> /dev/null
[[ $? == 1 ]] || err_exit "Negative file descriptors cause 'read -u' to crash"

# An out of range fd shouldn't segfault with redirections
"$SHELL" -c 'true 9>&20000000000000000000' 2> /dev/null
[[ $? == 1 ]] || err_exit "Out of range file descriptors cause redirections to segfault"

# ======
# A file descriptor opened with 'exec' or 'redirect' leaked out of a subshell.
exec 3>&-  # close FD 3 just in case
(exec 3>"$tmp/fdleak.txt")
{ echo bug >&3; } 2>/dev/null
if	[[ -s "$tmp/fdleak.txt" ]]
then	exec 3>&-
	err_exit "Open file descriptor leaks out of subshell"
fi
# However, it should still survive a shared-state command substitution if it's not 1 (stdout).
redirect 3>&-  # close FD 3 just in case
: ${ redirect 3>"$tmp/fdshared.txt"; }
{ echo good >&3; } 2>/dev/null
if	[[ ! -s "$tmp/fdshared.txt" ]]
then	err_exit "Open file descriptor does not survive shared-state command substitution"
fi
redirect 3>&-

# ======
# On unpatched ksh on macOS, 'read' used to block when reading from a FIFO and there was no final newline.
if	mkfifo "$tmp/fifo_no_lf"
then	trap 'sleep_pid=0; kill "$ksh_pid"; err_exit "'\''read'\'' hangs on EOF without final linefeed when reading from FIFO"' TERM
	(sleep 1; kill "$$") &
	sleep_pid=$!
	"$SHELL" -c 'print -n foo >$0 & while read f; do :; done <$0' "$tmp/fifo_no_lf" &
	ksh_pid=$!
	wait "$ksh_pid"
	trap - TERM
	((sleep_pid)) && kill "$sleep_pid"
else	err_exit "mkfifo failed; cannot test reading from FIFO"
fi

# ======
# "&>file" redirection operator, shorthand for ">file 2>&1" (new as of 93u+m; inherited from old SHOPT_BASH)
if	[[ -o ?posix ]] && command set -o posix
then
	# This should print in a background job, then create an empty file, as '>aha1.txt' is a separate command.
	eval '	print -u1 bad1 &>aha1.txt
		print -u2 bad2 &>aha2.txt
	' >/dev/null 2>&1
	[[ -s aha1.txt ]] && err_exit "&> not deactivated in POSIX mode (stdout; got '$(cat aha1.txt)')"
	[[ -s aha2.txt ]] && err_exit "&> not deactivated in POSIX mode (stderr; got '$(cat aha2.txt)')"
	set +o posix
	# This should write the text to the file.
	eval '	print -u1 ok1 &>aha1.txt
		print -u2 ok2 &>aha2.txt
	' >/dev/null 2>&1
	[[ $(< aha1.txt) == ok1 ]] || err_exit '&> does not redirect stdout'
	[[ $(< aha2.txt) == ok2 ]] || err_exit '&> does not redirect stderr'
	# In POSIX mode, file descriptors > 2 should remain open when invoking another program
	(set -o posix; exec 7>ok.txt; "$SHELL" -c 'print ok >&7' 2>/dev/null)
	[[ $(<ok.txt) == ok ]] || err_exit 'File descriptors > 2 not inherited in POSIX mode'
fi
(exec 7>bad.txt; "$SHELL" -c 'print bad >&7' 2>/dev/null)
[[ $(<bad.txt) == '' ]] || err_exit 'File descriptors > 2 inherited without POSIX mode' "(got $actual)"

# ======
# File descriptor leak with process substitution
# https://github.com/ksh93/ksh/issues/67
err=$(
	ulimit -n 15 || exit 0
	fdUser() {
		:
	}
	set +x
	for ((i=1; i<10; i++))
	do	fdUser <(:) >(:) || exit
	done 2>&1
) || err_exit 'Process substitution leaks file descriptors when used as argument to function' \
	"(got $(printf %q "$err"))"

# File descriptor leak after 'command not found' with process substitution as argument
err=$(
	ulimit -n 25 || exit 0
	set +x
	PATH=/dev/null
	for ((i=1; i<10; i++))
	do	notfound <(:) >(:) 2> /dev/null
	done 2>&1
	exit 0
) || err_exit "Process substitution leaks file descriptors when used as argument to nonexistent command" \
	"(got $(printf %q "$err"))"

got=$(command -x cat <(command -x echo foo) 2>&1) || err_exit "process substitution doesn't work with 'command'" \
	"(got $(printf %q "$got"))"

# ======
# A redirection with a null command could crash under certain circumstances (rhbz#1200534)
"$SHELL" -i >/dev/null 2>&1 -c '
	function dlog
	{
		fc -ln -0
	}
	trap dlog DEBUG
	for((i=0;i<1000;i++))
	do	>"$1/rhbz1200534"
	done
	exit
' empty_redir_crash_test "$tmp"
((!(e = $?))) || err_exit 'crash on null-command redirection with DEBUG trap' \
	"(got status $e$( ((e>128)) && print -n / && kill -l "$e"), $(printf %q "$got"))"

# ======
# stdout was misdirected if an EXIT/ERR trap handler was defined in a -c script
# https://github.com/att/ast/issues/36
exp=$'exit to stdout\ntrap to stdout'
exp2=$'exit to file\ntrap to file'
got=$(export tmp; "$SHELL" -ec \
'	function log
	{
		echo "$* to stdout"
		echo "$* to file" >> $tmp/ast36_a.test.log
	}
	function test_exit
	{
		log trap
	}
	trap test_exit EXIT
	log exit
')
[[ $got == "$exp" ]] || err_exit 'stdout misdirected to file with EXIT/ERR trap defined (1)' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
[[ $(< $tmp/ast36_a.test.log) == "$exp2" ]] || err_exit 'stdout not correctly redirected to file with EXIT/ERR trap defined (1)' \
	"(expected $(printf %q "$exp2"), wrote $(printf %q "$(< $tmp/ast36_a.test.log)"))"

exp=$'trap to stdout\nexit to stdout'
exp2=$'trap to file\nexit to file'
got=$(export tmp; "$SHELL" -ec \
'	function log
	{
		echo "$* to stdout"
		echo "$* to file" >> $tmp/ast36_b.test.log
	}
	function test_exit
	{
		trap test_exittrap EXIT
		log trap
	}
	function test_exittrap
	{
		log exit
	}
	test_exit
')
[[ $got == "$exp" ]] || err_exit 'stdout misdirected to file with EXIT/ERR trap defined (2)' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
[[ $(< $tmp/ast36_b.test.log) == "$exp2" ]] || err_exit 'stdout not correctly redirected to file with EXIT/ERR trap defined (2)' \
	"(expected $(printf %q "$exp2"), wrote $(printf %q "$(< $tmp/ast36_b.test.log)"))"

# ======
# Redirections of the form {varname}>file stopped working if brace expansion was turned off
((SHOPT_BRACEPAT)) && set +B
{ redirect {v}>$tmp/v.out && echo ok >&$v; } 2>/dev/null && redirect {v}>&-
((SHOPT_BRACEPAT)) && set -B
[[ -r $tmp/v.out && $(<$tmp/v.out) == ok ]] || err_exit '{varname}>file not working with brace expansion turned off'

# ...and they didn't work in subshells: https://github.com/ksh93/ksh/issues/167
(redirect {v}>$tmp/v.out; echo ok2 >&$v) 2>/dev/null
[[ -r $tmp/v.out && $(<$tmp/v.out) == ok2 ]] || err_exit 'redirect {varname}>file not working in a subshell'

# ======
# Process substitution hang in ksh93v- 2013-10-10 alpha
{
	producer() {
		for ((i = 0; i < 20000; i++ )) do
			print xxxxx${i}xxxxx
		done
	}
	consumer() {
		while read var; do
			print ${var}
		done < ${1}
	}
	consumer <(producer) > /dev/null
} & pid=$!
(sleep 5; kill -HUP $pid) 2> /dev/null &
wait $pid 2> /dev/null || err_exit "process substitution hangs"

# ======
# Test for looping or lingering process substitution processes
# https://github.com/ksh93/ksh/issues/213
procsub_pid=$(
	ulimit -t unlimited 2>/dev/null  # fork the subshell
	true >(true) <(true) >(true) <(true)
	echo "$!"
)
sleep .1
if kill -0 "$procsub_pid" 2>/dev/null; then
	kill -TERM "$procsub_pid" # don't leave around what is effectively a zombie process
	err_exit "process substitutions loop or linger after parent shell finishes"
fi
(true <(true) >(true) <(true) >(true); wait) &
sleep .1
if kill -0 $! 2> /dev/null; then
	kill -TERM $!
	err_exit "process substitutions linger when unused"
fi

# process substitutions should work correctly with delays
procsub_delay()
{
	sleep .1  # a delay >50ms, the current fifo_check delay in xec.c
	cat "$@"
}
exp=$'hi\nthere\nworld'
got=$(procsub_delay <(echo hi) <(echo there) <(echo world))
[[ $got == "$exp" ]] || err_exit "process substitutions passed to function failed" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# process substitutions should work regardless of umask
got=$(umask 777; set +x; { cat <(echo ok); } 2>&1)
[[ $got == ok ]] || err_exit "process substitution failed with umask 777 (got $(printf %q "$got"))"

# ======
# https://github.com/att/ast/issues/1336
# Use the /proc pseudo filesystem on Linux as a convenient way to force a write I/O error.
if [[ -e /proc/self/mem && $(uname) == Linux ]]
then
	actual=$(LC_ALL=C "$SHELL" -c 'echo > /proc/self/mem; echo okay' 2>&1)
	expect='write.*failed.*okay'
	[[ "$actual" =~ $expect ]] || err_exit "I/O failure not handled" \
		"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
fi

# ======
# Test for BUG_CSUBSTDO: If stdout is closed before running a command substitution,
# redirecting any file descriptor in the command substitution would break stdout
# inside of the command substitution. This only occurred when redirecting any other
# file descriptor inside of the command substitution.
exp='Foo bar'
{ got=$(echo 'Foo bar' 2>/dev/null); } >&-
[[ $exp == $got ]] || err_exit "BUG_CSUBSTDO: Closing stdout outside of command substitution breaks stdout inside of command substitution" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# In shcomp, process substitution did not work when used as the file name to a redirection.
# https://github.com/ksh93/ksh/issues/165
# Note: avoid testing this in a command substitution, as those are always parsed at runtime,
# meaning shcomp will also include them as literal source text instead of compiling them.
echo ok > >(cat >out1)
wait "$!"  # the procsub is run asynchronously, so wait before reading from the file
cat >out2 < <(case x in x) cat out1;; esac)
[[ $(<out2) == ok ]] || err_exit "process substitution not working as file name to redirection" \
	"(expected 'ok', got $(printf %q "$(<out2)"))"

# ======
# Reading a file through a command substitution
# https://github.com/att/ast/issues/203
TMPF=$tmp/tmpf
echo foo >$TMPF
export TMPF
[[ -n "$($SHELL -c 'echo $(<$TMPF)' <&-)" ]] || err_exit "Closing stdin causes failure when reading file through \$(<)"
[[ -n "$($SHELL -c "$SHELL -c 'echo \$(<$TMPF) >&2' >&-" 2>&1)" ]] || err_exit "Closing stdout causes failure when reading file through \$(<)"
[[ -n "$($SHELL -c 'echo $(<$TMPF)' 2>&-)" ]]  || err_exit "Closing stderr causes failure when reading file through \$(<)"

# ======
# Verify that symlinks are correctly canonicalized as part of a conditional redirection.
# https://github.com/att/ast/issues/492
mkdir -p dir1/dir2
ln -s dir1 s1
cd dir1
ln -s dir2 s2
cd ..
exp=symlinks-resolved
print wrong-answer > dir1/dir2/x
print $exp >; s1/s2/x
got=$(< dir1/dir2/x)
[[ $got == "$exp" ]] || err_exit "symlink in conditional redirect wrong" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
exit $((Errors<125?Errors:125))

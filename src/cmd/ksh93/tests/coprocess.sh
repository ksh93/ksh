########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1982-2012 AT&T Intellectual Property          #
#          Copyright (c) 2020-2025 Contributors to ksh 93u+m           #
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
# test the behavior of co-processes

. "${SHTESTS_COMMON:-${0%/*}/_common}"

if	[[ -d /cygdrive ]]
then	warning 'Cygwin detected - coprocess tests disabled - enable at the risk of wedging your system'
	exit 0
fi

bintrue=$(whence -p true)

function ping # ID
{
	integer x=0
	while ((x++ < 5))
	do	read -r
		print -r "$1 $REPLY"
	done
}

bincat=$(whence -p cat)
builtin cat 2> /dev/null && builtincat=cat

for cat in $builtincat $bincat
do

	$cat |&
	print -p "hello"
	read -p line
	[[ $line == hello ]] || err_exit "$cat coprocessing fails"
	exec 5>&p 6<&p
	print -u5 'hello again' || err_exit "write on $cat coprocess u5 fails"
	read -u6 line
	[[ $line == 'hello again' ]] || err_exit "$cat coprocess after moving fds fails"
	exec 5<&- 6<&-
	wait $!

	ping three |&
	exec 3>&p
	ping four |&
	exec 4>&p
	ping pipe |&

	integer count
	for i in three four pipe four pipe four three pipe pipe three pipe
	do	case $i in
		three)	to=-u3;;
		four)	to=-u4;;
		pipe)	to=-p;;
		esac
		(( count++ ))
		print $to $i $count
	done

	while	((count > 0))
	do	(( count-- ))
		read -p
		set -- $REPLY
		if	[[ $1 != $2 ]]
		then	err_exit "$cat coprocess $1 does not match $2"
		fi
		case $1 in
		three)	;;
		four)	;;
		pipe)	;;
		*)	err_exit "$cat coprocess unknown message +|$REPLY|+" ;;
		esac
	done
	kill $(jobs -p) 2>/dev/null

	file=$tmp/regress
	$cat > $file  <<-!
	$cat |&
	!
	chmod +x $file
	wait
	sleep 1 |&
	$file 2> /dev/null || err_exit "parent $cat coprocess prevents script coprocess"
	exec 5<&p 6>&p
	exec 5<&- 6>&-
	kill $(jobs -p) 2>/dev/null

	${SHELL-ksh} |&
	cop=$!
	exp=Done
	print -p $'print hello | '$cat$'\nprint '$exp
	read -t 1 -p
	read -t 1 -p
	got=$REPLY
	if	[[ $got != $exp ]]
	then	err_exit "${SHELL-ksh} $cat coprocess io failed -- got '$got', expected '$exp'"
	fi
	exec 5<&p 6>&p
	exec 5<&- 6>&-
	{ sleep .4; kill $cop; } 2>/dev/null &
	spy=$!
	if	wait $cop 2>/dev/null
	then	kill $spy 2>/dev/null
	else	err_exit "$cat coprocess hung after 'exec 5<&p 6>&p; exec 5<&- 6>&-'"
	fi
	wait

	{
	echo line1 | grep 'line2'
	echo line2 | grep 'line1'
	} |&
	SECONDS=0 count=0
	while	read -p -t 1 line
	do	((count++))
	done
	if	(( SECONDS > .8 ))
	then	err_exit "$cat coprocess read -p hanging (SECONDS=$SECONDS count=$count)"
	fi
	wait $!

	( sleep 3 |& sleep .1 && kill $!; sleep .5; sleep 3 |& sleep .1 && kill $! ) ||
		err_exit "$cat coprocess cleanup not working correctly"
	{ : |& } 2>/dev/null ||
		err_exit "subshell $cat coprocess lingers in parent"
	wait $!

	unset N r e
	integer N=5
	e=12345
	(
		integer i
		for ((i = 1; i <= N; i++))
		do	print $i |&
			read -p r
			print -n $r
			wait $!
		done
		print
	) 2>/dev/null | read -t 1 r
	[[ $r == $e ]] || err_exit "$cat coprocess timing bug -- expected $e, got '$r'"

	r=
	(
		integer i
		for ((i = 1; i <= N; i++))
		do	print $i |&
			sleep 0.02
			r=$r$($cat <&p)
			wait $!
		done
		print $r
	) 2>/dev/null | read -t 1 r
	[[ $r == $e ]] || err_exit "$cat coprocess command substitution bug -- expected $e, got '$r'"

	(
		$cat |&
		sleep 0.01
		exec 6>&p
		print -u6 ok
		exec 6>&-
		sleep .2
		kill $! 2> /dev/null
	) && err_exit "$cat coprocess with subshell would hang"
	for sig in IOT ABRT
	do	if	( trap - $sig ) 2> /dev/null
		then	if	[[ $( { sig=$sig $SHELL  2> /dev/null <<- ++EOF++
					$cat |&
					pid=\$!
					trap "print TRAP" \$sig
					(
						sleep .2
						kill -\$sig \$\$
						sleep .2
						kill -\$sig \$\$
						kill \$pid
						sleep .2
						kill \$\$
					) &
					while	read -p || ((\$? > 256))
					do	:
					done
				++EOF++
				} ) != $'TRAP\nTRAP' ]] 2> /dev/null
			then	err_exit "traps when reading from $cat coprocess not working"
			fi
			break
		fi
	done

	trap 'sleep_pid=; kill $pid; err_exit "$cat coprocess 1 hung"' TERM
	{ sleep 5; kill $$; } &
	sleep_pid=$!
	$cat |&
	pid=$!
	exec 5<&p 6>&p
	exp=hi
	print -u6 $exp; read -u5
	[[ $REPLY == "$exp" ]] || err_exit "REPLY from $cat coprocess failed -- expected '$exp', got '$REPLY'"
	exec 6>&-
	wait $pid
	trap - TERM
	[[ $sleep_pid ]] && kill $sleep_pid

	trap 'sleep_pid=; kill $pid; err_exit "$cat coprocess 2 hung"' TERM
	{ sleep 5; kill $$; } &
	sleep_pid=$!
	$cat |&
	pid=$!
	print foo1 >&p 2> /dev/null || err_exit "first write to $cat coprocess failed"
	print foo2 >&p 2> /dev/null || err_exit "second write to $cat coprocess failed"
	# avoid race between '$cat |&' initialising and 'kill $pid': read from the coprocess
	# before 'kill' so that $cat is known to be fully initialised and ready to catch SIGTERM
	read <&p && [[ $REPLY == foo1 ]] || err_exit "first read from $cat coprocess failed"
	read <&p && [[ $REPLY == foo2 ]] || err_exit "second read from $cat coprocess failed"
	kill $pid
	wait $pid 2> /dev/null
	trap - TERM
	[[ $sleep_pid ]] && kill $sleep_pid

	trap 'sleep_pid=; kill $pid; err_exit "$cat coprocess 3 hung"' TERM
	{ sleep 5; kill $$; } &
	sleep_pid=$!
	$cat |&
	pid=$!
	print -p foo
	print -p bar
	read <&p || err_exit "first read from $cat coprocess failed"
	[[ $REPLY == foo ]] || err_exit "first REPLY from $cat coprocess is $REPLY not foo"
	read <&p || err_exit "second read from $cat coprocess failed"
	[[ $REPLY == bar ]] || err_exit "second REPLY from $cat coprocess is $REPLY not bar"
	kill $pid
	wait $pid 2> /dev/null
	trap - TERM
	[[ $sleep_pid ]] && kill $sleep_pid

done

function cop
{
	read
	print ok
}

exp=ok

cop |&
pid=$!
if	print -p yo 2>/dev/null
then	read -p got
else	got='no coprocess'
fi
[[ $got == $exp ]] || err_exit "main coprocess main query failed -- expected $exp, got '$got'"
kill $pid 2>/dev/null
wait

cop |&
pid=$!
(
if	print -p yo 2>/dev/null
then	read -p got
else	got='no coprocess'
fi
[[ $got == $exp ]] || err_exit "main coprocess subshell query failed -- expected $exp, got '$got'"
)
kill $pid 2>/dev/null
wait

exp='no coprocess'

(
cop |&
print $! > $tmp/pid
)
pid=$(<$tmp/pid)
if	print -p yo 2>/dev/null
then	read -p got
else	got=$exp
fi
[[ $got == $exp ]] || err_exit "subshell coprocess main query failed -- expected $exp, got '$got'"
kill $pid 2>/dev/null
wait

(
cop |&
print $! > $tmp/pid
)
pid=$(<$tmp/pid)
(
if	print -p yo 2>/dev/null
then	read -p got
else	got=$exp
fi
[[ $got == $exp ]] || err_exit "subshell coprocess subshell query failed -- expected $exp, got '$got'"
kill $pid 2>/dev/null
wait
)

function mypipe
{
	read; read
	print -r -- "$REPLY"
}

mypipe |&
print -p "hello"
z="$( $bintrue $($bintrue) )"
{ print -p "world";} 2> /dev/null
read -p
[[ $REPLY == world ]] ||  err_exit "expected 'world' got '$REPLY'"
kill $pid 2>/dev/null
wait


function cop
{
	read
	print ok
}
exp=ok
cop |&
pid=$!
(
if      print -p yo 2>/dev/null
then    read -p got
else    got='no coprocess'
fi
[[ $got == $exp ]] || err_exit "main coprocess subshell query failed -- expected $exp, got '$got'"
)
kill $pid 2>/dev/null
wait

tee=$(whence -p tee)
ls -l |&
pid=$!
$tee -a /dev/null <&p > /dev/null
wait $pid
x=$?
[[ $x == 0 ]] || err_exit "coprocess exitval should be 0, not $x"

exit $((Errors<125?Errors:125))

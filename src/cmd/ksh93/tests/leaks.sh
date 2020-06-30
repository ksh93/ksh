########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1982-2012 AT&T Intellectual Property          #
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

function err_exit
{
	print -u2 -n "\t"
	print -u2 -r ${Command}[$1]: "${@:2}"
	let Errors+=1
}
alias err_exit='err_exit $LINENO'

Command=${0##*/}
integer Errors=0

# Get the current amount of memory usage
function getmem
{
	echo $(ps -p $$ -o pid,vsz | awk '{ if (NR != 1){ print $2 } }')
}

# test for variable reset leak #

function test_reset
{
	integer i n=$1

	for ((i = 0; i < n; i++))
	do	u=$i
	done
}

n=1000

# one round to get to steady state -- sensitive to -x

test_reset $n
before="$(getmem)"
after="$(getmem)"

test_reset $n
before="$(getmem)"
test_reset $n
after="$(getmem)"

if	(( after > before ))
then	err_exit "variable value reset memory leak -- $((after - before)) bytes after $n iterations"
fi

# buffer boundary tests

for exp in 65535 65536
do	got=$($SHELL -c 'x=$(printf "%.*c" '$exp' x); print ${#x}' 2>&1)
	[[ $got == $exp ]] || err_exit "large command substitution failed -- expected $exp, got $got"
done

data="(v=;sid=;di=;hi=;ti='1328244300';lv='o';id='172.3.161.178';var=(k='conn_num._total';u=;fr=;l='Number of Connections';n='22';t='number';))"
read -C stat <<< "$data"
before="$(getmem)"
for ((i=0; i < 500; i++))
do	print -r -- "$data"
done |	while read -u$n -C stat
	do	:
	done	{n}<&0-
after="$(getmem)"
(( after > before )) && err_exit 'memory leak with read -C when deleting compound variable'

read -C stat <<< "$data"
before="$(getmem)"
for ((i=0; i < 500; i++))
do      read -C stat <<< "$data"
done
after="$(getmem)"
(( after > before )) && err_exit 'memory leak with read -C when using <<<'

exit $((Errors<125?Errors:125))

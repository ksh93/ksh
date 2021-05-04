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

# Determine method for running tests.
# The 'vmstate' builtin can be used if ksh was compiled with vmalloc.
if	builtin vmstate 2>/dev/null &&
	n=$(vmstate --format='%(busy_size)u') &&
	let "($n) == ($n) && n > 0"	# non-zero number?
then	N=512			# number of iterations for each test
	unit=bytes
	tolerance=$((4*N))	# tolerate 4 bytes per iteration to account for vmalloc artefacts
	function getmem
	{
		vmstate --format='%(busy_size)u'
	}
# On Linux, we can use /proc to get byte granularity for vsize (field 23).
elif	[[ -f /proc/$$/stat && $(uname) == Linux ]]
then	N=2048			# number of iterations for each test
	unit=bytes
	tolerance=$((4*N))	# tolerate 4 bytes per iteration to account for malloc artefacts
	function getmem
	{
		cut -f 23 -d ' ' </proc/$$/stat
	}
# On UnixWare, read the process virtual size with ps
elif	[[ $(uname) == UnixWare ]] &&
	n=$(ps -o vsz= -p "$$" 2>/dev/null) &&
	let "($n) == ($n) && n > 0"
then	N=16384
	unit=KiB
	tolerance=$((4*N/1024))	# tolerate 4 bytes per iteration to account for malloc artefacts
	function getmem
	{
		ps -o vsz= -p "$$"
	}
# Otherwise, make do with the nonstandard 'rss' (real resident size) keyword
# of the 'ps' command (the standard 'vsz', virtual size, is not usable).
elif	n=$(ps -o rss= -p "$$" 2>/dev/null) &&
	let "($n) == ($n) && n > 0"
then	N=16384
	unit=KiB
	tolerance=$((8*N/1024))	# tolerate 8 bytes per iteration to account for malloc/ps artefacts
	function getmem
	{
		ps -o rss= -p "$$"
	}
else	warning 'cannot find method to measure memory usage; skipping tests'
	exit 0
fi

# test for variable reset leak #

function test_reset
{
	integer i N=$1

	for ((i = 0; i < N; i++))
	do	u=$i
	done
}

# Initialise variables used below to avoid false leaks
before=0 after=0 i=0 u=0


# Check results.
# The function has 'err_exit' in the name so that shtests counts each call as at test.
function err_exit_if_leak
{
	if	((after > before + tolerance))
	then	err\_exit "$1" "$2 (leaked approx $((after - before)) $unit after $N iterations)"
	fi
}
alias err_exit_if_leak='err_exit_if_leak "$LINENO"'

# one round to get to steady state -- sensitive to -x

test_reset $N
test_reset $N
before=$(getmem)
test_reset $N
after=$(getmem)
err_exit_if_leak "variable value reset memory leak"

# buffer boundary tests

for exp in 65535 65536
do	got=$($SHELL -c 'x=$(printf "%.*c" '$exp' x); print ${#x}' 2>&1)
	[[ $got == $exp ]] || err_exit "large command substitution failed -- expected $exp, got $got"
done

data="(v=;sid=;di=;hi=;ti='1328244300';lv='o';id='172.3.161.178';var=(k='conn_num._total';u=;fr=;l='Number of Connections';n='22';t='number';))"
read -C stat <<< "$data"
for ((i=0; i < 8; i++))	# steady state first
do	print -r -- "$data" | while read -u$n -C stat; do :; done {n}<&0-
done
before=$(getmem)
for ((i=0; i < N; i++))
do	print -r -- "$data"
done |	while read -u$n -C stat
	do	:
	done	{n}<&0-
after=$(getmem)
err_exit_if_leak "memory leak with read -C when deleting compound variable"

# extra 'read's to get to steady state
for ((i=0; i < 10; i++))
do	read -C stat <<< "$data"
done
before=$(getmem)
for ((i=0; i < N; i++))
do      read -C stat <<< "$data"
done
after=$(getmem)
err_exit_if_leak "memory leak with read -C when using <<<"

# ======
# Unsetting an associative array shouldn't cause a memory leak
# See https://www.mail-archive.com/ast-users@lists.research.att.com/msg01016.html
typeset -A stuff
before=$(getmem)
for (( i=0; i < N; i++ ))
do
	unset stuff[xyz]
	typeset -A stuff[xyz]
	stuff[xyz][elem0]="data0"
	stuff[xyz][elem1]="data1"
	stuff[xyz][elem2]="data2"
	stuff[xyz][elem3]="data3"
	stuff[xyz][elem4]="data4"
done
unset stuff
after=$(getmem)
err_exit_if_leak 'unset of associative array causes memory leak'

# ======
# Memory leak when resetting PATH and clearing hash table
# ...steady memory state:
command -v ls >/dev/null	# add something to hash table
PATH=/dev/null true		# set/restore PATH & clear hash table
# ...test for leak:
before=$(getmem)
for	((i=0; i < N; i++))
do	PATH=/dev/null true	# set/restore PATH & clear hash table
	command -v ls		# do PATH search, add to hash table
done >/dev/null
after=$(getmem)
err_exit_if_leak 'memory leak on PATH reset before PATH search'
# ...test for another leak that only shows up when building with nmake:
before=$(getmem)
for	((i=0; i < N; i++))
do	PATH=/dev/null true	# set/restore PATH & clear hash table
done >/dev/null
after=$(getmem)
err_exit_if_leak 'memory leak on PATH reset'

# ======
# Defining a function in a virtual subshell
# https://github.com/ksh93/ksh/issues/114

unset -f foo
before=$(getmem)
for ((i=0; i < N; i++))
do	(function foo { :; }; foo)
done
after=$(getmem)
err_exit_if_leak 'ksh function defined in virtual subshell'
typeset -f foo >/dev/null && err_exit 'ksh function leaks out of subshell'

unset -f foo
before=$(getmem)
for ((i=0; i < N; i++))
do	(foo() { :; }; foo)
done
after=$(getmem)
err_exit_if_leak 'POSIX function defined in virtual subshell'
typeset -f foo >/dev/null && err_exit 'POSIX function leaks out of subshell'

# Unsetting a function in a virtual subshell

function foo { echo bar; }
before=$(getmem)
for ((i=0; i < N; i++))
do	(unset -f foo)
done
after=$(getmem)
err_exit_if_leak 'ksh function unset in virtual subshell'
typeset -f foo >/dev/null || err_exit 'ksh function unset in subshell was unset in main shell'

foo() { echo bar; }
before=$(getmem)
for ((i=0; i < N; i++))
do	(unset -f foo)
done
after=$(getmem)
err_exit_if_leak 'POSIX function unset in virtual subshell'
typeset -f foo >/dev/null || err_exit 'POSIX function unset in subshell was unset in main shell'

before=$(getmem)
for ((i=0; i < N; i++))
do	(function foo { echo baz; }; unset -f foo)
done
after=$(getmem)
err_exit_if_leak 'ksh function defined and unset in virtual subshell'

before=$(getmem)
for ((i=0; i < N; i++))
do	(foo() { echo baz; }; unset -f foo)
done
after=$(getmem)
err_exit_if_leak 'POSIX function defined and unset in virtual subshell'

# ======
# Sourcing a dot script in a virtual subshell

echo 'echo "$@"' > $tmp/dot.sh
before=$(getmem)
for ((i=0; i < N; i++))
do	(. "$tmp/dot.sh" dot one two three >/dev/null)
done
after=$(getmem)
err_exit_if_leak 'script dotted in virtual subshell'

echo 'echo "$@"' > $tmp/dot.sh
before=$(getmem)
for ((i=0; i < N; i++))
do	(source "$tmp/dot.sh" source four five six >/dev/null)
done
after=$(getmem)
err_exit_if_leak 'script sourced in virtual subshell'

# ======
# Multiple leaks when using arrays in functions (Red Hat #921455)
# Fix based on: https://src.fedoraproject.org/rpms/ksh/blob/642af4d6/f/ksh-20120801-memlik.patch

# TODO: both of these tests still leak (although much less after the patch) when run in a non-C locale.
saveLANG=$LANG; LANG=C	# comment out to test remaining leak (1/2)

function _hash
{
	typeset w=([abc]=1 [def]=31534 [xyz]=42)
	print -u2 $w 2>&-
	# accessing the var will leak
}
before=$(getmem)
for ((i=0; i < N; i++))
do	_hash
done
after=$(getmem)
err_exit_if_leak 'associative array in function'

function _array
{
	typeset w=(1 31534 42)
	print -u2 $w 2>&-
	# unset w will prevent leak
}
before=$(getmem)
for ((i=0; i < N; i++))
do	_array
done
after=$(getmem)
err_exit_if_leak 'indexed array in function'

LANG=$saveLANG		# comment out to test remaining leak (2/2)

# ======
# Memory leak in typeset (Red Hat #1036470)
# Fix based on: https://src.fedoraproject.org/rpms/ksh/blob/642af4d6/f/ksh-20120801-memlik3.patch
# The fix was backported from ksh 93v- beta.

function myFunction
{
	typeset toPrint="something"
	echo "${toPrint}"
}
state=$(myFunction)
before=$(getmem)
for ((i=0; i < N; i++))
do	state=$(myFunction)
done
after=$(getmem)
err_exit_if_leak 'typeset in function called by command substitution'

# ======
# Check that unsetting an alias frees both the node and its value

before=$(getmem)
for ((i=0; i < N; i++))
do	alias "test$i=command$i"
	unalias "test$i"
done
after=$(getmem)
err_exit_if_leak 'unalias'

# ======
# Red Hat bug rhbz#982142: command substitution leaks

# case1: Nested command substitutions
# (reportedly already fixed in 93u+, but let's keep the test)
before=$(getmem)
for ((i=0; i < N; i++))
do	a=`true 1 + \`true 1 + 1\``	# was: a=`expr 1 + \`expr 1 + 1\``
done
after=$(getmem)
err_exit_if_leak 'nested command substitutions'

# case2: Command alias
alias ls='true -ltr'			# was: alias ls='ls -ltr'
before=$(getmem)
for ((i=0; i < N; i++))
do	eval 'a=`ls`'
done
after=$(getmem)
unalias ls
err_exit_if_leak 'alias in command substitution'

# case3: Function call via autoload
cat >$tmp/func1 <<\EOF
function func1
{
    echo "func1 call";
}
EOF
FPATH=$tmp
autoload func1
a=`func1`  # steady memory state
before=$(getmem)
for ((i=0; i < N; i++))
do	a=`func1`
done
after=$(getmem)
unset -f func1
unset -v FPATH
err_exit_if_leak 'function call via autoload in command substitution'

# ======

# add some random utilities to the hash table to detect memory leak on hash table reset when changing PATH
random_utils=(chmod cp mv awk sed diff comm cut sort uniq date env find mkdir rmdir pr sleep)

save_PATH=$PATH
hash "${random_utils[@]}"
before=$(getmem)
for ((i=0; i < N; i++))
do	hash -r
	hash "${random_utils[@]}"
done
after=$(getmem)
err_exit_if_leak 'clear hash table (hash -r) in main shell'

before=$(getmem)
for ((i=0; i < N; i++))
do	PATH=/dev/null
	PATH=$save_PATH
	hash "${random_utils[@]}"
done
after=$(getmem)
err_exit_if_leak 'set PATH value in main shell'

before=$(getmem)
for ((i=0; i < N; i++))
do	PATH=/dev/null command true
done
after=$(getmem)
err_exit_if_leak 'run command with preceding PATH assignment in main shell'

: <<'disabled'	# TODO: known leak (approx 73552 bytes after 512 iterations)
before=$(getmem)
for ((i=0; i < N; i++))
do	typeset -A PATH
	unset PATH
	PATH=$save_PATH
	hash "${random_utils[@]}"
done
after=$(getmem)
err_exit_if_leak 'set PATH attribute in main shell'
disabled

: <<'disabled'	# TODO: known leak (approx 99568 bytes after 512 iterations)
before=$(getmem)
for ((i=0; i < N; i++))
do	unset PATH
	PATH=$save_PATH
	hash "${random_utils[@]}"
done
after=$(getmem)
err_exit_if_leak 'unset PATH in main shell'
disabled

hash "${random_utils[@]}"
before=$(getmem)
for ((i=0; i < N; i++))
do	(hash -r)
done
after=$(getmem)
err_exit_if_leak 'clear hash table (hash -r) in subshell'

: <<'disabled'	# TODO: known leak (approx 123520 bytes after 512 iterations)
before=$(getmem)
for ((i=0; i < N; i++))
do	(PATH=/dev/null)
done
after=$(getmem)
err_exit_if_leak 'set PATH value in subshell'
disabled

: <<'disabled'	# TODO: known leak (approx 24544 bytes after 512 iterations)
before=$(getmem)
for ((i=0; i < N; i++))
do	(PATH=/dev/null command true)
done
after=$(getmem)
err_exit_if_leak 'run command with preceding PATH assignment in subshell'
disabled

: <<'disabled'	# TODO: known leak (approx 131200 bytes after 512 iterations)
before=$(getmem)
for ((i=0; i < N; i++))
do	(readonly PATH)
done
after=$(getmem)
err_exit_if_leak 'set PATH attribute in subshell'
disabled

: <<'disabled'	# TODO: known leak (approx 229440 bytes after 512 iterations)
before=$(getmem)
for ((i=0; i < N; i++))
do	(unset PATH)
done
after=$(getmem)
err_exit_if_leak 'unset PATH in subshell'
disabled

# ======
# Test for a memory leak after 'cd' (in relation to $PWD and $OLDPWD)
original_pwd=$PWD
before=$(getmem)
for ((i=0; i < N; i++))
do	cd /tmp
	cd - > /dev/null
	PWD=/foo
	OLDPWD=/bar
	cd /bin
	cd /usr
	cd /dev
	cd /dev
	cd - > /dev/null
	unset OLDPWD PWD
	cd /bin
	cd /tmp
done
after=$(getmem)
err_exit_if_leak 'PWD and/or OLDPWD changed by cd'
cd $original_pwd

# ======
# https://github.com/ksh93/ksh/issues/253#issuecomment-815308466
: <<'disabled'	# TODO: known leak(s) (approx 1008 KiB after 16384 iterations)
before=$(getmem)
for ((i=0; i < N; i++))
do
	(SECONDS=1; LANG=C)
done
after=$(getmem)
err_exit_if_leak 'Variable with discipline function in subshell causes memory leak'
disabled

# ======
exit $((Errors<125?Errors:125))

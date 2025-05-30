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
#          atheik <14833674+atheik@users.noreply.github.com>           #
#                  Lev Kujawski <int21h@mailbox.org>                   #
#                                                                      #
########################################################################

. "${SHTESTS_COMMON:-${0%/*}/_common}"

# Determine method for running tests.
# On Linux, we can use /proc to get byte granularity for vsize (field 23).
if	[[ -f /proc/$$/stat && $(uname) == Linux ]]
then	getmem()
	{
		print $(( $(cut -f 23 -d ' ' </proc/$$/stat ) / 1024 ))
	}
# On UnixWare, read the process virtual size with ps
elif	[[ $(uname) == UnixWare ]] &&
	n=$(ps -o vsz= -p "$$" 2>/dev/null) &&
	let "($n) == ($n) && n > 0"
then	getmem()
	{
		ps -o vsz= -p "$$"
	}
# Otherwise, make do with the nonstandard 'rss' (real resident size) keyword
# of the 'ps' command (the standard 'vsz', virtual size, is not usable).
elif	n=$(ps -o rss= -p "$$" 2>/dev/null) &&
	let "($n) == ($n) && n > 0"
then	getmem()
	{
		ps -o rss= -p "$$"
	}
else	warning 'cannot find method to measure memory usage; skipping tests'
	exit 0
fi

# Parameters for test blocks.
# Intended to cope with all non-deterministic OS memory management artefacts.
#
# Theory: if we can get a sequence of $min_good_blocks blocks of $block_iter iterations without the memory state changing,
# then we can safely assume it's not a memory leak, break the loop, and consider the test succeeded. To allow for
# unpredictable OS memory management artefacts, that sequence is allowed to occur anywhere within $max_iter iterations.
# This speeds up the tests, as successful tests can bow out at $((min_good_blocks * block_iter)) iterations if they're
# lucky. If the OS decides to randomly grow the memory heap, it may take more tries, but hopefully not more than
# $max_iter iterations. If the loop counter reaches $max_iter, then we assume a memory leak and throw a test failure.

typeset -ir max_iter=65536 block_iter=128 min_good_blocks=16

# Set up test block construct.
# Known leaks can be marked known=y to turn them into non-fail warnings.
#
# Usage:
#	TEST	title='description' [ known=y [ url=<issue tracker URL> ] ]
#		<optional preparatory commands>
#	DO
#		<test payload commands>
#	DONE
#
# To avoid messing up $LINENO, aliases should not contain newline characters.
# To keep things readable, backslash line continuation is used instead.

typeset .lt	# create lt (leak test) namespace for internal variables
typeset -i .lt.before=0 .lt.after=0 .lt.total=0 .lt.good=0 .lt.i=0 .lt.j=0
alias TEST=\
'for .lt.i in 1; do '\
'	unset -v known url; '
	# optional preparatory commands go here
alias DO=\
'	if	[[ -v known && ! -v DEBUG ]]; '\
'	then	warning "skipping test for known leak \"$title\";" '\
'			"export DEBUG=y to test" ${url:+"and help us fix it at: $url"}; '\
'		break; '\
'	fi; '\
'	.lt.before=$(getmem) .lt.good=0 .lt.total=0; '\
'	for ((.lt.i = 0; .lt.i < max_iter; .lt.i += block_iter)); '\
'	do	for ((.lt.j = 0; .lt.j < block_iter; .lt.j++)); '\
'		do '
			# test payload commands go here
alias DONE=\
'		done; '\
'		.lt.after=$(getmem); '\
'		if	((.lt.after <= .lt.before)); '\
'		then	((.lt.good++ == min_good_blocks)) && break; '\
'		else 	((.lt.good = 0)); '\
'			((.lt.total += (.lt.after - .lt.before))); '\
'			.lt.before=$(getmem); '\
'		fi; '\
'	done; '\
'	if	((.lt.i >= max_iter)); '\
'	then	if	[[ -v known ]]; '\
'		then	err_exit "known leak: $title: leaked approx ${.lt.total} KiB after ${.lt.i} iterations" '\
'				${url:+"-- help us fix it at: $url"}; '\
'		else	err_exit "$title: leaked approx ${.lt.total} KiB after ${.lt.i} iterations"; '\
'		fi; '\
'	elif	[[ -v known ]]; '\
'	then	warning "did not detect known leak \"$title\": succeeded after ${.lt.i} iterations)" ${url:+"-- see: $url"}; '\
'	elif	[[ -v DEBUG ]]; '\
'	then	_message "$LINENO" "[DEBUG] test \"$title\" succeeded after ${.lt.i} iterations"; '\
'	fi; '\
'done'

# ____ Begin memory leak tests ____

TEST	title='variable value reset'
	integer i=0
	u=foo
DO
	u=$((++i))
DONE

# data for the next two tests...
data="(v=;sid=;di=;hi=;ti='1328244300';lv='o';id='172.3.161.178';var=(k='conn_num._total';u=;fr=;l='Number of Connections';n='22';t='number';))"
read -C stat <<< "$data"

while	:
do	print -r -- "$data"
done | \
TEST	title='read -C when deleting compound variable'
DO	read -u$n -C stat
DONE {n}<&0-

TEST	title='read -C when using <<<'
DO
	read -C stat <<< "$data"
DONE

unset data stat

# ======
# Unsetting an associative array shouldn't cause a memory leak
# See https://www.mail-archive.com/ast-users@lists.research.att.com/msg01016.html
TEST	title='unset of associative array'
	typeset -A stuff
DO
	unset stuff[xyz]
	typeset -A stuff[xyz]
	stuff[xyz][elem0]="data0"
	stuff[xyz][elem1]="data1"
	stuff[xyz][elem2]="data2"
	stuff[xyz][elem3]="data3"
	stuff[xyz][elem4]="data4"
DONE

# https://github.com/ksh93/ksh/issues/94
TEST	title='defining associative array in subshell'
DO
	(typeset -A foo=([a]=1 [b]=2 [c]=3))
	(typeset -Ai foo=([a]=1 [b]=2 [c]=3))
DONE

TEST	title='defining and unsetting associative array in subshell'
DO
	(typeset -A foo=([a]=1 [b]=2 [c]=3); unset foo)
	(typeset -Ai foo=([a]=1 [b]=2 [c]=3); unset foo)
DONE

TEST	title='unsetting associative array in subshell'
	typeset -A foo=([a]=1 [b]=2 [c]=3)
DO
	(unset foo)
DONE
unset foo

# ======
# Memory leak when resetting PATH and clearing hash table
# ...test for leak:
TEST	title='PATH reset before PATH search'
DO
	PATH=/dev/null true	# set/restore PATH & clear hash table
	command -v ls		# do PATH search, add to hash table
DONE >/dev/null
# ...test for another leak that only shows up when building with nmake:
TEST	title='PATH reset'
DO
	PATH=/dev/null true	# set/restore PATH & clear hash table
DONE >/dev/null

# ======
# Defining a function in a virtual subshell
# https://github.com/ksh93/ksh/issues/114

TEST	title='ksh function defined in virtual subshell'
	unset -f foo
DO
	(function foo { :; }; foo)
DONE

TEST	title='POSIX function defined in virtual subshell'
	unset -f foo
DO
	(foo() { :; }; foo)
DONE

# Unsetting a function in a virtual subshell

TEST	title='ksh function unset in virtual subshell'
	function foo { echo bar; }
DO
	(unset -f foo)
DONE

TEST	title='POSIX function unset in virtual subshell'
	foo() { echo bar; }
DO
	(unset -f foo)
DONE

TEST	title='ksh function defined and unset in virtual subshell'
DO
	(function foo { echo baz; }; unset -f foo)
DONE

TEST	title='POSIX function defined and unset in virtual subshell'
DO
	(foo() { echo baz; }; unset -f foo)
DONE

# ======
# Sourcing a dot script in a virtual subshell

TEST	title='script dotted in virtual subshell'
	echo 'echo "$@"' > $tmp/dot.sh
DO
	(. "$tmp/dot.sh" dot one two three >/dev/null)
DONE

TEST	title='script sourced in virtual subshell'
	echo 'echo "$@"' > $tmp/dot.sh
DO
	(source "$tmp/dot.sh" source four five six >/dev/null)
DONE

# ======
# Multiple leaks when using arrays in functions (Red Hat #921455)
# Fix based on: https://src.fedoraproject.org/rpms/ksh/blob/642af4d6/f/ksh-20120801-memlik.patch

TEST	title='associative array in function'
	function _hash
	{
		typeset w=([abc]=1 [def]=31534 [xyz]=42)
		print -u2 $w 2>&-
		# accessing the var will leak
	}
DO
	_hash
DONE

TEST	title='indexed array in function'
	function _array
	{
		typeset w=(1 31534 42)
		print -u2 $w 2>&-
		# unset w will prevent leak
	}
DO
	_array
DONE

# ======
# Memory leak in typeset (Red Hat #1036470)
# Fix based on: https://src.fedoraproject.org/rpms/ksh/blob/642af4d6/f/ksh-20120801-memlik3.patch
# The fix was backported from ksh 93v- beta.

TEST	title='typeset in function called by command substitution'
	function myFunction
	{
		typeset toPrint="something"
		echo "${toPrint}"
	}
DO
	state=$(myFunction)
DONE

# ======
# Check that unsetting an alias frees both the node and its value

TEST	title='unalias'
DO
	alias "test$i=command$i"
	unalias "test$i"
DONE

# ======
# Red Hat bug rhbz#982142: command substitution leaks

# case1: Nested command substitutions
# (reportedly already fixed in 93u+, but let's keep the test)
TEST	title='nested command substitutions'
DO
	a=`true 1 + \`true 1 + 1\``	# was: a=`expr 1 + \`expr 1 + 1\``
DONE

# case2: Command alias
TEST	title='alias in command substitution'
	alias ls='true -ltr'		# was: alias ls='ls -ltr'
DO
	eval 'a=`ls`'
DONE

# case3: Function call via autoload
TEST	title='function call via autoload in command substitution'
	cat >$tmp/func1 <<-\EOF
	function func1
	{
	    echo "func1 call";
	}
	EOF
	FPATH=$tmp
	autoload func1
DO
	a=`func1`
DONE

# ======

# add some random utilities to the hash table to detect memory leak on hash table reset when changing PATH
random_utils=(chmod cp mv awk sed diff comm cut sort uniq date env find mkdir rmdir pr sleep)
save_PATH=$PATH

TEST	title='clear hash table (hash -r) in main shell'
DO
	hash -r
	hash "${random_utils[@]}"
DONE

TEST	title='set PATH value in main shell'
DO
	PATH=/dev/null
	PATH=$save_PATH
	hash "${random_utils[@]}"
DONE

TEST	title='run command with preceding PATH assignment in main shell'
DO
	PATH=/dev/null command true
DONE

TEST	title='set PATH attribute in main shell' known=y url=https://github.com/ksh93/ksh/issues/405
DO
	typeset -A PATH
	unset PATH
	PATH=$save_PATH
	hash "${random_utils[@]}"
DONE

TEST	title='unset PATH in main shell' known=y url=https://github.com/ksh93/ksh/issues/405
DO
	unset PATH
	PATH=$save_PATH
	hash "${random_utils[@]}"
DONE

TEST	title='clear hash table (hash -r) in subshell'
	hash "${random_utils[@]}"
DO
	(hash -r)
DONE

TEST	title='set PATH value in subshell' known=y url=https://github.com/ksh93/ksh/issues/405
DO
	(PATH=/dev/null)
DONE

TEST	title='run command with preceding PATH assignment in subshell' known=y url=https://github.com/ksh93/ksh/issues/405
DO
	(PATH=/dev/null command true)
DONE

TEST	title='set PATH attribute in subshell' known=y url=https://github.com/ksh93/ksh/issues/405
DO
	(readonly PATH)
DONE

TEST	title='unset PATH in subshell' known=y url=https://github.com/ksh93/ksh/issues/405
DO
	(unset PATH)
DONE

# ======
# Test for a memory leak after 'cd' (in relation to $PWD and $OLDPWD)
TEST	title='PWD and/or OLDPWD changed by cd'
DO
	cd /tmp 2>/dev/null || cd /
	cd - > /dev/null
	PWD=/foo
	OLDPWD=/bar
	cd /bin
	cd /usr 2>/dev/null || cd /
	cd /dev
	cd /dev
	cd - > /dev/null
	unset OLDPWD PWD
	cd /bin
	cd "$tmp"
DONE

# ======
TEST	title='variable with discipline function in subshell' known=y url=https://github.com/ksh93/ksh/issues/404
DO
	(SECONDS=1; LANG=C)
DONE

# ======
TEST title='showing --man info'
DO
	set --man >/dev/null 2>&1
	ulimit --man >/dev/null 2>&1
DONE

# ======
# Possible memory leak when using the += operator on variables
# in an invocation-local scope.
bintrue=$(whence -p true)
testfunc() { true; }
alias testalias=testfunc
TEST title='+= operator used before command'
DO
	baz=bar
	baz+=baz :          # Special builtin
	baz+=foo true       # Regular builtin
	baz+=foo "$bintrue" # External command
	baz+=foo testfunc   # Function
	baz+=foo testalias  # Alias
	unset baz
DONE

# ======
TEST	title='assigning and comparing enum rvalue in arithmetic'
	enum Test1_t=(lorem ipsum dolor sit amet consectetur adipiscing elit curabitur scelerisque massa nec diam fermentum tempor)
	Test1_t foo
DO
	(((foo = amet) && foo == amet && foo != fermentum))
DONE

# ======
exit $((Errors<125?Errors:125))

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

# to avoid spurious test failures with 'whence -a' tests, we need
# to remove any duplicate paths to the same directory from $PATH.
function rm_path_dups
{
	typeset IFS=':' p seen s newpath
	set -o noglob
	p=$PATH:	# IFS field splitting discards a final empty field; add one to avoid that
	for p in $p
	do	if	[[ -z $p ]]
		then	# empty $PATH element == current dir
			p='.'
		fi
		for	s in $seen
		do	[[ $p -ef $s ]] && continue 2
		done
		newpath=${newpath:+$newpath:}$p
		seen=$seen$p:
	done
	PATH=$newpath
}
rm_path_dups
PATH_orig=$PATH

# output all paths to a command, skipping duplicates in $PATH
# (reimplementation of 'whence -a -p', useful for testing 'whence')
function all_paths
{
	typeset IFS=':' CDPATH='' p seen
	set -o noglob
	p=$PATH:	# IFS field splitting discards a final empty field; add one to avoid that
	for p in $p
	do	if	[[ -z $p ]]
		then	# empty $PATH element == current dir
			p='.'
		fi
		if	[[ $p != /* ]]
		then	# get absolute directory
			p=$(cd -L -- "$p" 2>/dev/null && print -r -- "${PWD}X") && p=${p%X} || continue
		fi
		if	[[ :$seen: == *:"$p":* ]]
		then	continue
		fi
		if	[[ -f $p/$1 && -x $p/$1 ]]
		then	print -r "$p/$1"
		fi
		seen=${seen:+$seen:}$p
	done
}

type /xxxxxx > out1 2> out2
[[ -s out1 ]] && err_exit 'type should not write on stdout for not found case'
[[ -s out2 ]] || err_exit 'type should write on stderr for not found case'
mkdir dir1 dir2
cat  > dir1/foobar << '+++'
foobar() { print foobar1 >foobar1.txt; cat <foobar1.txt;}
function dir1 { print dir1;}
+++
cat  > dir2/foobar << '+++'
foobar() { print foobar2;}
function dir2 { print dir2;}
+++
chmod +x dir[12]/foobar
p=$PATH
FPATH=$PWD/dir1
PATH=$FPATH:$p
[[ $( foobar) == foobar1 ]] || err_exit 'foobar should output foobar1'
FPATH=$PWD/dir2
PATH=$FPATH:$p
[[ $(foobar) == foobar2 ]] || err_exit 'foobar should output foobar2'
FPATH=$PWD/dir1
PATH=$FPATH:$p
[[ $(foobar) == foobar1 ]] || err_exit 'foobar should output foobar1 again'
FPATH=$PWD/dir2
PATH=$FPATH:$p
[[ ${ foobar;} == foobar2 ]] || err_exit 'foobar should output foobar2 with ${}'
[[ ${ dir2;} == dir2 ]] || err_exit 'should be dir2'
[[ ${ dir1;} == dir1 ]] 2> /dev/null &&  err_exit 'should not be dir1'
FPATH=$PWD/dir1
PATH=$FPATH:$p
[[ ${ foobar;} == foobar1 ]] || err_exit 'foobar should output foobar1 with ${}'
[[ ${ dir1;} == dir1 ]] || err_exit 'should be dir1'
[[ ${ dir2;} == dir2 ]] 2> /dev/null &&  err_exit 'should not be dir2'
FPATH=$PWD/dir2
PATH=$FPATH:$p
[[ ${ foobar;} == foobar2 ]] || err_exit 'foobar should output foobar2 with ${} again'
PATH=$p
(PATH="/bin")
[[ $($SHELL -c 'print -r -- "$PATH"') == "$PATH" ]] || err_exit 'export PATH lost in subshell'
cat > bug1 <<- EOF
	print print ok > $tmp/ok
	command -p chmod 755 $tmp/ok
	function a
	{
	        typeset -x PATH=$tmp
	        ok
	}
	path=\$PATH
	unset PATH
	a
	PATH=\$path
}
EOF
[[ $($SHELL ./bug1 2>/dev/null) == ok ]]  || err_exit "PATH in function not working"
cat > bug1 <<- \EOF
	function lock_unlock
	{
	typeset PATH=/usr/bin
	typeset -x PATH=''
	}

	PATH=/usr/bin
	: $(PATH=/usr/bin getconf PATH)
	typeset -ft lock_unlock
	lock_unlock
EOF
($SHELL ./bug1)  2> /dev/null || err_exit "path_delete bug"
mkdir tdir
if	$SHELL tdir > /dev/null 2>&1
then	err_exit 'not an error to run ksh on a directory'
fi

print 'print hi' > ls
if	[[ $($SHELL ls 2> /dev/null) != hi ]]
then	err_exit "$SHELL name not executing version in current directory"
fi
if	[[ $(ls -d . 2>/dev/null) == . && $(PATH=/bin:/usr/bin:$PATH ls -d . 2>/dev/null) != . ]]
then	err_exit 'PATH export in command substitution not working'
fi
pwd=$PWD
# get rid of leading and trailing : and trailing :.
PATH=${PATH%.}
PATH=${PATH%:}
PATH=${PATH#.}
PATH=${PATH#:}
path=$PATH
var=$(whence date)
dir=$(basename "$var")
for i in 1 2 3 4 5 6 7 8 9 0
do	if	! whence notfound$i 2> /dev/null
	then	cmd=notfound$i
		break
	fi
done
print 'print hello' > date
chmod +x date
print 'print notfound' >  $cmd
chmod +x "$cmd"
> foo
chmod 755 foo
for PATH in $path :$path $path: .:$path $path: $path:. $PWD::$path $PWD:.:$path $path:$PWD $path:.:$PWD
do
#	print path=$PATH $(whence date)
#	print path=$PATH $(whence "$cmd")
		date
		"$cmd"
done > /dev/null 2>&1
builtin -d date 2> /dev/null
if	[[ $(PATH=:/usr/bin; date) != 'hello' ]]
then	err_exit "leading : in path not working"
fi

builtin -d rm 2> /dev/null
chmod=$(whence chmod)
rm=$(whence rm)
d=$(dirname "$rm")

chmod=$(whence chmod)

for cmd in date foo
do	exp="$cmd found"
	print print $exp > $cmd
	$chmod +x $cmd
	got=$($SHELL -c "unset FPATH; PATH=/dev/null; $cmd" 2>&1)
	[[ $got == "$exp" ]] && err_exit "$cmd as last command should not find ./$cmd with PATH=/dev/null" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
	exp=$PWD/./$cmd
	got=$($SHELL -c "unset FPATH; PATH=/dev/null; $cmd" 2>&1)
	[[ $got == "$exp" ]] && err_exit "$cmd should not find ./$cmd with PATH=/dev/null" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
	exp=$PWD/$cmd
	got=$(unset FPATH; PATH=/dev/null; whence ./$cmd)
	[[ $got == "$exp" ]] || err_exit "whence $cmd should find ./$cmd with PATH=/dev/null" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
	got=$(unset FPATH; PATH=/dev/null; whence $PWD/$cmd)
	[[ $got == "$exp" ]] || err_exit "whence \$PWD/$cmd should find ./$cmd with PATH=/dev/null" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
done

exp=''
got=$($SHELL -c "unset FPATH; PATH=/dev/null; whence ./notfound" 2>&1)
[[ $got == "$exp" ]] || err_exit "whence ./$cmd failed -- expected '$exp', got '$got'" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$($SHELL -c "unset FPATH; PATH=/dev/null; whence $PWD/notfound" 2>&1)
[[ $got == "$exp" ]] || err_exit "whence \$PWD/$cmd failed -- expected '$exp', got '$got'" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

unset FPATH
PATH=/dev/null
for cmd in date foo
do	exp="$cmd found"
	print print $exp > $cmd
	$chmod +x $cmd
	got=$($cmd 2>&1)
	[[ $got == "$exp" ]] && err_exit "$cmd as last command should not find ./$cmd with PATH=/dev/null" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
	got=$($cmd 2>&1; :)
	[[ $got == "$exp" ]] && err_exit "$cmd should not find ./$cmd with PATH=/dev/null" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
	exp=$PWD/$cmd
	got=$(whence ./$cmd)
	[[ $got == "$exp" ]] || err_exit "whence ./$cmd should find ./$cmd with PATH=/dev/null" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
	got=$(whence $PWD/$cmd)
	[[ $got == "$exp" ]] || err_exit "whence \$PWD/$cmd should find ./$cmd with PATH=/dev/null" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
	got=$(cd / && whence "${OLDPWD#/}/$cmd")
	[[ $got == "$exp" ]] || err_exit "whence output should not start with '//' if PWD is '/'" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
done
exp=''
got=$(whence ./notfound)
[[ $got == "$exp" ]] || err_exit "whence ./$cmd failed -- expected '$exp', got '$got'" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(whence $PWD/notfound)
[[ $got == "$exp" ]] || err_exit "whence \$PWD/$cmd failed -- expected '$exp', got '$got'" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
PATH=$d:
command -p cp "$rm" kshrm
got=$(whence kshrm)
exp=$PWD/kshrm
[[ $got == "$exp" ]] || err_exit 'trailing : in pathname not working' \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
command -p cp "$rm" rm
PATH=:$d
got=$(whence rm)
exp=$PWD/rm
[[ $got == "$exp" ]] || err_exit 'leading : in pathname not working' \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
PATH=$d: whence rm > /dev/null
got=$(whence rm)
exp=$PWD/rm
[[ $got == "$exp" ]] || err_exit 'pathname not restored after scoping' \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
command -p mkdir bin
print 'print ok' > bin/tst
command -p chmod +x bin/tst
if	[[ $(PATH=$PWD/bin tst 2>/dev/null) != ok ]]
then	err_exit '(PATH=$PWD/bin foo) does not find $PWD/bin/foo'
fi
cd /
if	whence ls > /dev/null
then	PATH=
	if	[[ $(whence rm) ]]
	then	err_exit 'setting PATH to Null not working'
	fi
	unset PATH
	if	[[ $(whence rm) != /*rm ]]
	then	err_exit 'unsetting PATH not working'
	fi
fi
PATH=/dev:$tmp
export PATH
x=$(whence rm)
typeset foo=$(PATH=/xyz:/abc :)
y=$(whence rm)
[[ $x != "$y" ]] && err_exit 'PATH not restored after command substitution'
whence getconf > /dev/null  &&  err_exit 'getconf should not be found'
builtin /bin/getconf
PATH=/bin
PATH=$(getconf PATH)
x=$(whence ls)
PATH=.:$PWD:${x%/ls}
[[ $(whence ls) == "$x" ]] || err_exit 'PATH search bug when .:$PWD in path'
PATH=$PWD:.:${x%/ls}
[[ $(whence ls) == "$x" ]] || err_exit 'PATH search bug when :$PWD:. in path'
cd   "${x%/ls}"
[[ $(whence ls) == /* ]] || err_exit 'whence not generating absolute pathname'
status=$($SHELL -c $'trap \'print $?\' EXIT;/xxx/a/b/c/d/e 2> /dev/null')
[[ $status == 127 ]] || err_exit "not found command exit status $status -- expected 127"
status=$($SHELL -c $'trap \'print $?\' EXIT;/dev/null 2> /dev/null')
[[ $status == 126 ]] || err_exit "non executable command exit status $status -- expected 126"
status=$($SHELL -c $'trap \'print $?\' ERR;/xxx/a/b/c/d/e 2> /dev/null')
[[ $status == 127 ]] || err_exit "not found command with ERR trap exit status $status -- expected 127"
status=$($SHELL -c $'trap \'print $?\' ERR;/dev/null 2> /dev/null')
[[ $status == 126 ]] || err_exit "non executable command ERR trap exit status $status -- expected 126"

# universe via PATH

builtin getconf
getconf UNIVERSE - att # override sticky default 'UNIVERSE = foo'

[[ $(PATH=/usr/ucb/bin:/usr/bin echo -n ucb) == 'ucb' ]] || err_exit "ucb universe echo ignores -n option"
[[ $(PATH=/usr/xpg/bin:/usr/bin echo -n att) == '-n att' ]] || err_exit "att universe echo does not ignore -n option"

PATH=$path

scr=$tmp/script
exp=126

if [[ $(id -u) == '0' ]]; then
	warning "running as root: skipping tests involving unreadable scripts"
else

: > $scr
chmod a=x $scr
{ got=$($scr; print $?); } 2>/dev/null
[[ "$got" == "$exp" ]] || err_exit "unreadable empty script should fail -- expected $exp, got $got"
{ got=$(command $scr; print $?); } 2>/dev/null
[[ "$got" == "$exp" ]] || err_exit "command of unreadable empty script should fail -- expected $exp, got $got"
[[ "$(:; $scr; print $?)" == "$exp" ]] 2>/dev/null || err_exit "unreadable empty script in [[ ... ]] should fail -- expected $exp"
[[ "$(:; command $scr; print $?)" == "$exp" ]] 2>/dev/null || err_exit "command unreadable empty script in [[ ... ]] should fail -- expected $exp"
got=$($SHELL -c "$scr; print \$?" 2>/dev/null)
[[ "$got" == "$exp" ]] || err_exit "\$SHELL -c of unreadable empty script should fail -- expected $exp, got" $got
got=$($SHELL -c "command $scr; print \$?" 2>/dev/null)
[[ "$got" == "$exp" ]] || err_exit "\$SHELL -c of command of unreadable empty script should fail -- expected $exp, got" $got

rm -f $scr
print : > $scr
chmod a=x $scr
{ got=$($scr; print $?); } 2>/dev/null
[[ "$got" == "$exp" ]] || err_exit "unreadable non-empty script should fail -- expected $exp, got $got"
{ got=$(command $scr; print $?); } 2>/dev/null
[[ "$got" == "$exp" ]] || err_exit "command of unreadable non-empty script should fail -- expected $exp, got $got"
[[ "$(:; $scr; print $?)" == "$exp" ]] 2>/dev/null || err_exit "unreadable non-empty script in [[ ... ]] should fail -- expected $exp"
[[ "$(:; command $scr; print $?)" == "$exp" ]] 2>/dev/null || err_exit "command unreadable non-empty script in [[ ... ]] should fail -- expected $exp"
got=$($SHELL -c "$scr; print \$?" 2>/dev/null)
[[ "$got" == "$exp" ]] || err_exit "\$SHELL -c of unreadable non-empty script should fail -- expected $exp, got" $got
got=$($SHELL -c "command $scr; print \$?" 2>/dev/null)
[[ "$got" == "$exp" ]] || err_exit "\$SHELL -c of command of unreadable non-empty script should fail -- expected $exp, got" $got

fi  # if [[ $(id -u) == '0' ]]

# whence -a bug fix
cd "$tmp"
ifs=$IFS
IFS=$'\n'
PATH=$PATH:
> ls
chmod +x ls
ok=
for i in $(whence -a ls)
do	if	[[ $i == *"$PWD/ls" ]]
	then	ok=1
		break;
	fi
done
[[ $ok ]] || err_exit 'whence -a not finding all executables'
rm -f ls
PATH=${PATH%:}

#whence -p bug fix
function foo
{
	:
}
[[ $(whence -p foo) == foo ]] && err_exit 'whence -p foo should not find function foo'

# whence -q bug fix
$SHELL -c 'whence -q cat' & pid=$!
sleep .1
kill $! 2> /dev/null && err_exit 'whence -q appears to be hung'

FPATH=$PWD
print  'function foobar { :;}' > foobar
autoload foobar;
exec {m}< /dev/null
for ((i=0; i < 25; i++))
do	( foobar )
done
exec {m}<& -
exec {n}< /dev/null
(( n > m )) && err_exit 'autoload function in subshell leaves file open'

# whence -a bug fix
rmdir=rmdir
if	mkdir "$rmdir"
then	rm=${ whence rm;}
	cp "$rm" "$rmdir"
	{ PATH=:${rm%/rm} $SHELL -c "cd \"$rmdir\";whence -a rm";} > /dev/null 2>&1
	exitval=$?
	(( exitval==0 )) || err_exit "whence -a has exitval $exitval"
fi

[[ ! -d bin ]] && mkdir bin
[[ ! -d fun ]] && mkdir fun
print 'FPATH=../fun' > bin/.paths
cat <<- \EOF > fun/myfun
	function myfun
	{
		print myfun
	}
EOF
x=$(FPATH= PATH=$PWD/bin $SHELL -c  ': $(whence less);myfun') 2> /dev/null
[[ $x == myfun ]] || err_exit 'function myfun not found'

command -p cat >user_to_group_relationship.hdr.query <<EOF
#!$SHELL
print -r -- "\$@"
EOF
command -p chmod 755 user_to_group_relationship.hdr.query
FPATH=/foobar:
PATH=$FPATH:$PATH:.
[[ $(user_to_group_relationship.hdr.query foobar) == foobar ]] || err_exit 'Cannot execute command with . in name when PATH and FPATH end in :.'

mkdir -p $tmp/new/bin
mkdir $tmp/new/fun
print FPATH=../fun > $tmp/new/bin/.paths
print FPATH=../xxfun > $tmp/bin/.paths
cp "$(whence -p echo)" $tmp/new/bin
PATH=$tmp/bin:$tmp/new/bin:$PATH
x=$(whence -p echo 2> /dev/null)
[[ $x == "$tmp/new/bin/echo" ]] ||  err_exit 'nonexistent FPATH directory in .paths file causes path search to fail'

$SHELL 2> /dev/null <<- \EOF || err_exit 'path search problem with non-existent directories in PATH'
	builtin getconf
	PATH=$(getconf PATH)
	PATH=/dev/null/nogood1/bin:/dev/null/nogood2/bin:$PATH
	tail /dev/null && tail /dev/null
EOF

( PATH=/dev/null
command -p cat << END >/dev/null 2>&1
${.sh.version}
END
) || err_exit '${.sh.xxx} variables causes cat not be found'

PATH=$PATH_orig

# ======
# Check that 'command -p' searches the default OS utility PATH.
expect=/dev/null
actual=$(PATH=/dev/null "$SHELL" -c 'command -p ls /dev/null' 2>&1)
[[ $actual == "$expect" ]] || err_exit 'command -p fails to find standard utility' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

# ======
# ksh segfaults if $PATH contains a .paths directory
mkdir -p $tmp/paths-dir-crash/
cat > $tmp/paths-dir-crash/run.sh <<- EOF
mkdir -p $tmp/paths-dir-crash/.paths
export PATH=$tmp/paths-dir-crash:$PATH
print ok
EOF
ofile=$tmp/dotpaths.out
trap 'sleep_pid=; while kill -9 $pid; do :; done 2>/dev/null; err_exit "PATH containing .paths directory: shell hung"' TERM
trap 'kill $sleep_pid; while kill -9 $pid; do :; done 2>/dev/null; trap - INT; kill -s INT $$"' INT
{ sleep 5; kill $$; } &
sleep_pid=$!
"$SHELL" "$tmp/paths-dir-crash/run.sh" >$ofile 2>&1 &
pid=$!
{ wait $pid; } 2>>$ofile
e=$?
trap - TERM INT
[[ $sleep_pid ]] && kill $sleep_pid
((!e)) && [[ $(<$ofile) == ok ]] || err_exit "PATH containing .paths directory:" \
	"got status $e$( ((e>128)) && print -n / && kill -l "$e"), $(printf %q "$(<$ofile)")"

# ======
# Check that 'command -p' and 'command -p -v' do not use the hash table (a.k.a. tracked aliases).
print 'echo "wrong path used"' > $tmp/ls
chmod +x $tmp/ls
expect=/dev/null
actual=$(set +x; PATH=$tmp; redirect 2>&1; hash ls; command -p ls /dev/null)
[[ $actual == "$expect" ]] || err_exit "'command -p' fails to search default path if tracked alias exists" \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
actual=$(set +x; PATH=$tmp; redirect 2>&1; hash ls; command -p ls /dev/null; exit)  # the 'exit' disables subshell optimization
[[ $actual == "$expect" ]] || err_exit "'command -p' fails to search default path if tracked alias exists" \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
expect=$(builtin getconf; PATH=$(getconf PATH); whence -p ls)
actual=$(set +x; PATH=$tmp; redirect 2>&1; hash ls; command -p -v ls)
[[ $actual == "$expect" ]] || err_exit "'command -p -v' fails to search default path if tracked alias exists" \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

# Check that adding '-x' makes '-v'/'-V' look up external commands
if	((.sh.version >= 20210130))
then
	exp=$tmp/echo
	touch "$exp"
	chmod +x "$exp"
	got=$(PATH=$tmp; command -vx echo 2>&1)
	[[ $got == "$exp" ]] || err_exit "'command -v -x' failed to look up external command in \$PATH" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
	exp="echo is a tracked alias for $exp"
	got=$(PATH=$tmp; command -Vx echo 2>&1)
	[[ $got == "$exp" ]] || err_exit "'command -V -x' failed to look up external command in \$PATH" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
	exp=$(PATH=${ getconf PATH 2>/dev/null; }; whence -p echo)
	got=$(PATH=$tmp; command -pvx echo 2>&1)
	[[ $got == "$exp" ]] || err_exit "'command -p -v -x' failed to look up external command in default path" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
	exp="echo is $exp"
	got=$(PATH=$tmp; command -pVx echo 2>&1)
	[[ $got == "$exp" ]] || err_exit "'command -p -V -x' failed to look up external command in default path" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
fi

# ======
# 'command -x' used to hang in an endless E2BIG loop on Linux and macOS
ofile=$tmp/command_x_chunks.sh
trap 'sleep_pid=; while kill -9 $pid; do :; done 2>/dev/null; err_exit "'\''command -x'\'' hung"' TERM
trap 'kill $sleep_pid; while kill -9 $pid; do :; done 2>/dev/null; trap - INT; kill -s INT $$"' INT
{ sleep 60; kill $$; } &
sleep_pid=$!
(
	export LC_ALL=C
	unset IFS; set +f
	builtin getconf && arg_max=$(getconf ARG_MAX) && let arg_max || { err_exit "getconf ARG_MAX not working"; exit 1; }
	set +x	# trust me, you don't want to xtrace what follows
	# let's try to use a good variety of argument lengths
	set -- $(typeset -p) $(functions) /dev/* /tmp/* /* *
	args=$*
	while	let "${#args} < 3 * arg_max"
	do	set -- "$RANDOM" "$@" "$RANDOM" "$@" "$RANDOM"
		args=$*
	done
	print "chunks=0 expargs=$# args=0 expsize=$((${#args}+1)) size=0"
	unset args
	command -x "$SHELL" -c '
		integer i size=0 numargs=${#}-2
		for ((i=numargs; i; i--))
		do	let "size += ${#1} + 1"
			shift
		done
		print "let chunks++ args+=$numargs size+=$size"
		if	[[ $0 != static_argzero || $1 != final_static_arg_1 || $2 != final_static_arg_2 ]]
		then	print "err_exit \"'\''command -x'\'': static arguments for chunk \$chunks incorrect\""
		fi
	' static_argzero "$@" final_static_arg_1 final_static_arg_2
) >$ofile &
pid=$!
{ wait $pid; } 2>/dev/null	# wait and suppress signal messages
e=$?
trap - TERM INT
[[ $sleep_pid ]] && kill $sleep_pid
if	[[ ${ kill -l "$e"; } == KILL ]]
then	warning "'command -x' test killed, probably due to lack of memory; skipping test"
else	if	let "e > 0"
	then	err_exit "'command -x' test yielded exit status $e$( let "e>128" && print -n / && kill -l "$e")"
	fi
	if	[[ ! -s $ofile ]]
	then	err_exit "'command -x' test failed to produce output"
	else	save_Command=$Command
		Command+=": ${ofile##*/}"
		. "$ofile"
		Command=$save_Command
		let "args == expargs && size == expsize" || err_exit "'command -x' did not correctly divide arguments" \
			"(expected $expargs args of total size $expsize, got $args args of total size $size;" \
			"divided in $chunks chunks)"
	fi
fi

# ======
# whence -a/-v tests

# wrong path to tracked aliases after loading builtin: https://github.com/ksh93/ksh/pull/25
actual=$("$SHELL" -c '
	hash chmod
	builtin chmod
	whence -a chmod
')
expect=$'chmod is a shell builtin\n'$(all_paths chmod | sed '1 s/^/chmod is a tracked alias for /; 2,$ s/^/chmod is /')
[[ $actual == "$expect" ]] || err_exit "'whence -a' does not work correctly with tracked aliases" \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

# spurious 'undefined function' message: https://github.com/ksh93/ksh/issues/26
actual=$("$SHELL" -c 'whence -a printf')
expect=$'printf is a shell builtin\n'$(all_paths printf | sed 's/^/printf is /')
[[ $actual == "$expect" ]] || err_exit "'whence -a': incorrect output" \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

# 'whence -a'/'type -a' failed to list builtin if function exists: https://github.com/ksh93/ksh/issues/83
actual=$(printf() { :; }; whence -a printf)
expect="printf is a function
printf is a shell builtin
$(all_paths printf | sed 's/^/printf is /')"
[[ $actual == "$expect" ]] || err_exit "'whence -a': incorrect output for function+builtin" \
        "(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
actual=$(autoload printf; whence -a printf)
expect="printf is an undefined function
printf is a shell builtin
$(all_paths printf | sed 's/^/printf is /')"
[[ $actual == "$expect" ]] || err_exit "'whence -a': incorrect output for autoload+builtin" \
        "(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

# 'whence -v' canonicalized paths improperly: https://github.com/ksh93/ksh/issues/84
cmdpath=${ whence -p printf; }
actual=$(cd /; whence -v "${cmdpath#/}")
expect="${cmdpath#/} is $cmdpath"
[[ $actual == "$expect" ]] || err_exit "'whence -v': incorrect canonicalization of initial /" \
        "(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
dotdot=
num=$(set -f; IFS=/; set -- $PWD; echo $#)
for ((i=1; i<num; i++))
do	dotdot+='../'
done
actual=$(cd /; whence -v "$dotdot${cmdpath#/}")
expect="$dotdot${cmdpath#/} is $cmdpath"
[[ $actual == "$expect" ]] || err_exit "'whence -v': incorrect canonicalization of pathname containing '..'" \
        "(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

# even absolute paths should be canonicalized
if	[[ -x /usr/bin/env && -d /usr/lib ]]	# even NixOS has this...
then	expect='/usr/lib/../bin/./env is /usr/bin/env'
	actual=$(whence -v /usr/lib/../bin/./env)
	[[ $actual == "$expect" ]] || err_exit "'whence -v': incorrect canonicalization of absolute path" \
		"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
fi

# whence -v/-a should not autoload functions itself
echo 'ls() { echo "Oops, I'\''m a function!"; }' >$tmp/ls
expect=$'/dev/null\n/dev/null'
actual=$(FPATH=$tmp; ls /dev/null; whence -a ls >/dev/null; ls /dev/null)
[[ $actual == "$expect" ]] || err_exit "'whence -a': mistaken \$FPATH function autoload (non-executable file)" \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
chmod +x "$tmp/ls"
actual=$(FPATH=$tmp; ls /dev/null; whence -a ls >/dev/null; ls /dev/null)
[[ $actual == "$expect" ]] || err_exit "'whence -a': mistaken \$FPATH function autoload (executable file)" \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

# "tracked aliases" (known on other shells as hash table entries) are really just cached PATH search
# results; they should be reported independently from real aliases, as they're actually completely
# different things, and "tracked aliases" are actually used when bypassing an alias (with e.g. \ls).
expect=$'ls is an alias for \'echo ALL UR F1LEZ R G0N3\'\n'$(all_paths ls|sed '1 s/^/ls is a tracked alias for /;2,$ s/^/ls is /')
actual=$(hash -r; alias ls='echo ALL UR F1LEZ R G0N3'; hash ls; whence -a ls)
[[ $actual == "$expect" ]] || err_exit "'whence -a' does not report tracked alias if alias exists" \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"

# Unlike pdksh, ksh93 didn't report the path to autoloadable functions, which was an annoying omission.
if	((.sh.version >= 20200925))
then	fundir=$tmp/whencefun
	mkdir $fundir
	echo "whence_FPATH_test() { echo I\'m just on FPATH; }" >$fundir/whence_FPATH_test
	echo "whence_autoload_test() { echo I was explicitly autoloaded; }" >$fundir/whence_autoload_test
	echo "function chmod { echo Hi, I\'m your new chmod!; }" >$fundir/chmod
	echo "function ls { echo Hi, I\'m your new ls!; }" >$fundir/ls
	actual=$("$SHELL" -c 'FPATH=$1
			autoload chmod whence_autoload_test
			whence -a chmod whence_FPATH_test whence_autoload_test ls cp
			whence_FPATH_test
			whence_autoload_test
			cp --totally-invalid-option 2>/dev/null
			ls --totally-invalid-option /dev/null/foo 2>/dev/null
			chmod --totally-invalid-option' \
		whence_autoload_test "$fundir" 2>&1)
	expect="chmod is an undefined function (autoload from $fundir/chmod)"
	expect+=$'\n'$(all_paths chmod | sed 's/^/chmod is /')
	expect+=$'\n'"whence_FPATH_test is an undefined function (autoload from $fundir/whence_FPATH_test)"
	expect+=$'\n'"whence_autoload_test is an undefined function (autoload from $fundir/whence_autoload_test)"
	expect+=$'\n'$(all_paths ls | sed '1 s/^/ls is a tracked alias for /; 2,$ s/^/ls is /')
	expect+=$'\n'"ls is an undefined function (autoload from $fundir/ls)"
	expect+=$'\n'$(all_paths cp | sed '1 s/^/cp is a tracked alias for /; 2,$ s/^/cp is /')
	expect+=$'\n'"I'm just on FPATH"
	expect+=$'\n'"I was explicitly autoloaded"
	expect+=$'\n'"Hi, I'm your new chmod!"
	[[ $actual == "$expect" ]] || err_exit "failure in reporting or running autoloadable functions" \
		$'-- diff follows:\n'"$(diff -u <(print -r -- "$expect") <(print -r -- "$actual") | sed $'s/^/\t| /')"
fi

# ======
# Very long nonexistent command names used to crash
# https://github.com/ksh93/ksh/issues/144
ofile=$tmp/longname.err
trap 'sleep_pid=; while kill -9 $pid; do :; done 2>/dev/null; err_exit "Long nonexistent command name: shell hung"' TERM
trap 'kill $sleep_pid; while kill -9 $pid; do :; done 2>/dev/null; trap - INT; kill -s INT $$"' INT
{ sleep 5; kill $$; } &
sleep_pid=$!
PATH=/dev/null FPATH=/dev/null "$SHELL" -c "$(awk -v ORS= 'BEGIN { for(i=0;i<10000;i++) print "xxxxxxxxxx"; }')" 2>/dev/null &
pid=$!
{ wait $pid; } 2>$ofile
e=$?
trap - TERM INT
[[ $sleep_pid ]] && kill $sleep_pid
((e == 127)) || err_exit "Long nonexistent command name:" \
	"got status $e$( ((e>128)) && print -n / && kill -l "$e"), $(printf %q "$(<$ofile)")"

# ======
# A function autoload recursion loop used to crash
# https://github.com/ksh93/ksh/issues/136
mkdir "$tmp/fun.$$" && cd "$tmp/fun.$$" || exit
echo 'self2' >self
echo 'self3' >self2
echo 'self4' >self3
echo 'self' >self4
cd ~- || exit
exp="$SHELL: line 2: autoload loop: self in $tmp/fun.$$/self
$SHELL: function, built-in or type definition for self4 not found in $tmp/fun.$$/self4
$SHELL: function, built-in or type definition for self3 not found in $tmp/fun.$$/self3
$SHELL: function, built-in or type definition for self2 not found in $tmp/fun.$$/self2
$SHELL: function, built-in or type definition for self not found in $tmp/fun.$$/self"
got=$({ FPATH=$tmp/fun.$$ "$SHELL" -c self; } 2>&1)
(((e = $?) == 126)) || err_exit 'Function autoload recursion loop:' \
	"got status $e$( ((e>128)) && print -n / && kill -l "$e"), $(printf %q "$got")"

# ======
# If a shared-state ${ command substitution; } changed the value of $PATH, the variable
# would change but the internal pathlist would not, making path searches inconsistent.
savePATH=$PATH
got=${ PATH=/dev/null; }
got=$(whence ls)
PATH=$savePATH
[[ -z $got ]] || err_exit "PATH search inconsistent after changing PATH in subshare (got $(printf %q "$got"))"

# ======
# POSIX: If a command is found but isn't executable, the exit status should be 126.
# The tests are arranged as follows:
#   Test *A runs commands with the -c execve(2) optimization.
#   Test *B runs commands with spawnveg (i.e., with posix_spawn(3) or vfork(2)).
#   Test *C runs commands with fork(2) in an interactive shell.
#   Test *D runs commands with 'command -x'.
#   Test *E runs commands with 'exec'.
# https://github.com/att/ast/issues/485
rm -rf noexecute
print 'print cannot execute' > noexecute
mkdir emptydir cmddir
exp=126
PATH=$PWD $SHELL -c 'noexecute' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 1A: exit status of non-executable command wrong" \
	"(expected $exp, got $got)"
PATH=$PWD $SHELL -c 'noexecute; exit $?' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 1B: exit status of non-executable command wrong" \
	"(expected $exp, got $got)"
PATH=$PWD $SHELL -ic 'noexecute; exit $?' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 1C: exit status of non-executable command wrong" \
	"(expected $exp, got $got)"
PATH=$PWD $SHELL -c 'command -x noexecute; exit $?' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 1D: exit status of non-executable command wrong" \
	"(expected $exp, got $got)"
PATH=$PWD $SHELL -c 'exec noexecute' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 1E: exit status of exec'd non-executable command wrong" \
	"(expected $exp, got $got)"

# Add an empty directory where the command isn't found.
PATH=$PWD:$PWD/emptydir $SHELL -c 'noexecute' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 2A: exit status of non-executable command wrong" \
	"(expected $exp, got $got)"
PATH=$PWD:$PWD/emptydir $SHELL -c 'noexecute; exit $?' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 2B: exit status of non-executable command wrong" \
	"(expected $exp, got $got)"
PATH=$PWD:$PWD/emptydir $SHELL -ic 'noexecute; exit $?' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 2C: exit status of non-executable command wrong" \
	"(expected $exp, got $got)"
PATH=$PWD:$PWD/emptydir $SHELL -c 'command -x noexecute; exit $?' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 2D: exit status of non-executable command wrong" \
	"(expected $exp, got $got)"
PATH=$PWD:$PWD/emptydir $SHELL -c 'exec noexecute' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 2E: exit status of exec'd non-executable command wrong" \
	"(expected $exp, got $got)"

# If an executable command is found after a non-executable command, skip the non-executable one.
print 'true' > cmddir/noexecute
chmod +x cmddir/noexecute
exp=0
PATH=$PWD:$PWD/cmddir $SHELL -c 'noexecute'
got=$?
[[ $exp == $got ]] || err_exit "Test 3A: failed to run executable command after encountering non-executable command" \
	"(expected $exp, got $got)"
PATH=$PWD:$PWD/cmddir $SHELL -c 'noexecute; exit $?'
got=$?
[[ $exp == $got ]] || err_exit "Test 3B: failed to run executable command after encountering non-executable command" \
	"(expected $exp, got $got)"
PATH=$PWD:$PWD/cmddir $SHELL -ic 'noexecute; exit $?'
got=$?
[[ $exp == $got ]] || err_exit "Test 3C: failed to run executable command after encountering non-executable command" \
	"(expected $exp, got $got)"
PATH=$PWD:$PWD/cmddir $SHELL -c 'command -x noexecute; exit $?'
got=$?
[[ $exp == $got ]] || err_exit "Test 3D: failed to run executable command after encountering non-executable command" \
	"(expected $exp, got $got)"
PATH=$PWD:$PWD/cmddir $SHELL -c 'exec noexecute'
got=$?
[[ $exp == $got ]] || err_exit "Test 3E: failed to run exec'd executable command after encountering non-executable command" \
	"(expected $exp, got $got)"

# Same test as above, but with a directory of the same name in the PATH.
rm "$PWD/noexecute"
mkdir "$PWD/noexecute"
PATH=$PWD:$PWD/cmddir $SHELL -c 'noexecute' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 4A: failed to run executable command after encountering directory with same name in PATH" \
	"(expected $exp, got $got)"
PATH=$PWD:$PWD/cmddir $SHELL -c 'noexecute; exit $?' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 4B: failed to run executable command after encountering directory with same name in PATH" \
	"(expected $exp, got $got)"
PATH=$PWD:$PWD/cmddir $SHELL -ic 'noexecute; exit $?' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 4C: failed to run executable command after encountering directory with same name in PATH" \
	"(expected $exp, got $got)"
PATH=$PWD:$PWD/cmddir $SHELL -c 'command -x noexecute; exit $?' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 4D: failed to run executable command after encountering directory with same name in PATH" \
	"(expected $exp, got $got)"
PATH=$PWD:$PWD/cmddir $SHELL -c 'exec noexecute' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 4E: failed to run exec'd executable command after encountering directory with same name in PATH" \
	"(expected $exp, got $got)"
# Don't treat directories as commands.
# https://github.com/att/ast/issues/757
mkdir cat
PATH=".:$PATH" cat < /dev/null || err_exit "Test 4F: directories should not be treated as executables"

# Test attempts to run directories located in the PATH.
exp=126
PATH=$PWD $SHELL -c 'noexecute' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 5A: exit status of non-executable command wrong" \
	"(expected $exp, got $got)"
PATH=$PWD $SHELL -c 'noexecute; exit $?' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 5B: exit status of non-executable command wrong" \
	"(expected $exp, got $got)"
PATH=$PWD $SHELL -ic 'noexecute; exit $?' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 5C: exit status of non-executable command wrong" \
	"(expected $exp, got $got)"
PATH=$PWD $SHELL -c 'command -x noexecute; exit $?' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 5D: exit status of non-executable command wrong" \
	"(expected $exp, got $got)"
PATH=$PWD $SHELL -c 'exec noexecute' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 5E: exit status of exec'd non-executable command wrong" \
	"(expected $exp, got $got)"

# Tests for attempting to run a non-existent command.
exp=127
PATH=/dev/null $SHELL -c 'nonexist' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 6A: exit status of non-existent command wrong" \
	"(expected $exp, got $got)"
PATH=/dev/null $SHELL -c 'nonexist; exit $?' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 6B: exit status of non-existent command wrong" \
	"(expected $exp, got $got)"
PATH=/dev/null $SHELL -ic 'nonexist; exit $?' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 6C: exit status of non-existent command wrong" \
	"(expected $exp, got $got)"
PATH=/dev/null $SHELL -c 'command -x nonexist; exit $?' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 6D: exit status of non-existent command wrong" \
	"(expected $exp, got $got)"
PATH=/dev/null $SHELL -c 'exec nonexist' > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 6E: exit status of exec'd non-existent command wrong" \
	"(expected $exp, got $got)"

# Tests for attempting to use a command name that's too long.
long_cmd=$(awk -v ORS= 'BEGIN { for(i=0;i<500;i++) print "xxxxxxxxxx"; }')
exp=127
PATH=$PWD $SHELL -c "$long_cmd" > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 7A: exit status or error message for command with long name wrong" \
	"(expected $exp, got $got)"
PATH=$PWD $SHELL -c "$long_cmd; exit \$?" > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 7B: exit status or error message for command with long name wrong" \
	"(expected $exp, got $got)"
PATH=$PWD $SHELL -ic "$long_cmd; exit \$?" > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 7C: exit status or error message for command with long name wrong" \
	"(expected $exp, got $got)"
PATH=$PWD $SHELL -c "command -x $long_cmd; exit \$?" > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 7D: exit status or error message for command with long name wrong" \
	"(expected $exp, got $got)"
PATH=$PWD $SHELL -c "exec $long_cmd" > /dev/null 2>&1
got=$?
[[ $exp == $got ]] || err_exit "Test 7E: exit status or error message for exec'd command with long name wrong" \
	"(expected $exp, got $got)"

# ======
exit $((Errors<125?Errors:125))

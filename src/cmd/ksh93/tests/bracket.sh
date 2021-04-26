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

null=''
if	[[ ! -z $null ]]
then	err_exit "-z: null string should be of zero length"
fi
file=$tmp/original
newer_file=$tmp/newer
if	[[ -z $file ]]
then	err_exit "-z: $file string should not be of zero length"
fi
if	[[ -a $file ]]
then	err_exit "-a: $file shouldn't exist"
fi
if	[[ -e $file ]]
then	err_exit "-e: $file shouldn't exist"
fi
> $file
if	[[ ! -a $file ]]
then	err_exit "-a: $file should exist"
fi
if	[[ ! -e $file ]]
then	err_exit "-e: $file should exist"
fi
chmod 777 $file
if	[[ ! -r $file ]]
then	err_exit "-r: $file should be readable"
fi
if	[[ ! -w $file ]]
then	err_exit "-w: $file should be writable"
fi
if	[[ ! -x $file ]]
then	err_exit "-x: $file should be executable"
fi
if	[[ ! -w $file || ! -r $file ]]
then	err_exit "-rw: $file should be readable/writable"
fi
if	[[ -s $file ]]
then	err_exit "-s: $file should be of zero size"
fi
if	[[ ! -f $file ]]
then	err_exit "-f: $file should be an ordinary file"
fi
if	[[  -d $file ]]
then	err_exit "-d: $file should not be a directory file"
fi
if	[[  ! -d . ]]
then	err_exit "-d: . should be a directory file"
fi
if	[[  -f /dev/null ]]
then	err_exit "-f: /dev/null should not be an ordinary file"
fi
chmod 000 $file

if	[[ $(id -u) == '0' ]]
then	warning "running as root: skipping tests involving r/w permissions"
else	if	[[ -r $file ]]
	then	err_exit "-r: $file should not be readable"
	fi
	if	[[ -w $file ]]
	then	err_exit "-w: $file should not be writable"
	fi
	if	[[ -w $file || -r $file ]]
	then	err_exit "-rw: $file should not be readable/writable"
	fi
fi
if	[[ ! -O $file ]]
then	err_exit "-O: $file should be owned by me"
fi
if	[[ -x $file ]]
then	err_exit "-x: $file should not be executable"
fi

if	[[   -z x &&  -z x || ! -z x ]]
then	:
else	err_exit " wrong precedence"
fi
if	[[   -z x &&  (-z x || ! -z x) ]]
then	err_exit " () grouping not working"
fi
if	[[ foo < bar ]]
then	err_exit "foo comes before bar"
fi
[[ . -ef $(pwd) ]] || err_exit ". is not $PWD"
set -o allexport
[[ -o allexport ]] || err_exit '-o: did not set allexport option'
if	[[ -n  $null ]]
then	err_exit "'$null' has non-zero length"
fi
if	[[ ! -r /dev/fd/0 ]]
then	err_exit "/dev/fd/0 not open for reading"
fi
if	[[ ! -w /dev/fd/2 ]]
then	err_exit "/dev/fd/2 not open for writing"
fi
sleep 1  # there are still file systems with 1-second timestamp granularity...
> $newer_file
if	[[ ! $file -ot $newer_file ]]
then	err_exit "$file should be older than $newer_file"
fi
if	[[ $file -nt $newer_file ]]
then	err_exit "$newer_file should be newer than $file"
fi
if	[[ $file != $tmp/* ]]
then	err_exit "$file should match $tmp/*"
fi
if	[[ $file == $tmp'/*' ]]
then	err_exit "$file should not equal $tmp'/*'"
fi
[[ ! ( ! -z $null && ! -z x) ]]	|| err_exit "negation and grouping"
[[ -z '' || -z '' || -z '' ]]	|| err_exit "three ors not working"
[[ -z '' &&  -z '' && -z '' ]]	|| err_exit "three ors not working"
(exit 8)
if	[[ $? -ne 8 || $? -ne 8 ]]
then	err_exit 'value $? within [[ ... ]]'
fi
x='(x'
if	[[ '(x' != '('* ]]
then	err_exit " '(x' does not match '('* within [[ ... ]]"
fi
if	[[ '(x' != "("* ]]
then	err_exit ' "(x" does not match "("* within [[ ... ]]'
fi
if	[[ '(x' != \(* ]]
then	err_exit ' "(x" does not match \(* within [[ ... ]]'
fi
if	[[ 'x(' != *'(' ]]
then	err_exit " 'x(' does not match '('* within [[ ... ]]"
fi
if	[[ 'x&' != *'&' ]]
then	err_exit " 'x&' does not match '&'* within [[ ... ]]"
fi
if	[[ 'xy' == *'*' ]]
then	err_exit " 'xy' matches *'*' within [[ ... ]]"
fi
if	[[ 3 > 4 ]]
then	err_exit '3 < 4'
fi
if	[[ 4 < 3 ]]
then	err_exit '3 > 4'
fi
if	[[ 3x > 4x ]]
then	err_exit '3x < 4x'
fi
x='@(bin|dev|?)'
cd /
if	[[ $(print $x) != "$x" ]]
then	err_exit 'extended pattern matching on command arguments'
fi
if	[[ dev != $x ]]
then	err_exit 'extended pattern matching not working on variables'
fi
if	[[ -u $SHELL ]]
then	err_exit "setuid on $SHELL"
fi
if	[[ -g $SHELL ]]
then	err_exit "setgid on $SHELL"
fi
test -d .  -a '(' ! -f . ')' || err_exit 'test not working'
if	[[ '!' != ! ]]
then	err_exit 'quoting unary operator not working'
fi
test \( -n x \) -o \( -n y \) 2> /dev/null || err_exit 'test ( -n x ) -o ( -n y) not working'
test \( -n x \) -o -n y 2> /dev/null || err_exit 'test ( -n x ) -o -n y not working'
chmod 600 $file
exec 4> $file
print -u4 foobar
if	[[ ! -s $file ]]
then	err_exit "-s: $file should be non-zero"
fi
exec 4>&-
if	[[ 011 -ne 11 ]]
then	err_exit "leading zeros in arithmetic compares not ignored"
fi
(
	{
		set -x
		[[ foo > bar ]]
	} 2> /dev/null
) || err_exit "foo<bar with -x enabled"
(
	eval "[[ (a) ]]"
) 2> /dev/null || err_exit "[[ (a) ]] not working"
> $file
chmod 4755 "$file"
if	test -u $file && test ! -u $file
then	err_exit "test ! -u suidfile not working"
fi
for i in '(' ')' '[' ']'
do	[[ $i == $i ]] || err_exit "[[ $i != $i ]]"
done
(
	[[ aaaa == {4}(a) ]] || err_exit 'aaaa != {4}(a)'
	[[ aaaa == {2,5}(a) ]] || err_exit 'aaaa != {2,4}(a)'
	[[ abcdcdabcd == {3,6}(ab|cd) ]] || err_exit 'abcdcdabcd == {3,4}(ab|cd)'
	[[ abcdcdabcde == {5}(ab|cd)e ]] || err_exit 'abcdcdabcd == {5}(ab|cd)e'
) || err_exit 'errors with {..}(...) patterns'
[[ D290.2003.02.16.temp == D290.+(2003.02.16).temp* ]] || err_exit 'pattern match bug with +(...)'

# Tests for [[ -N ... ]] (test -N ...) are disabled because it is expected to break on systems where
# $TMPDIR (or even the entire root file system) is mounted with noatime for better performance.
# Ref.: https://opensource.com/article/20/6/linux-noatime
# (It also needs annoyingly long sleep times on older systems with a 1-second timestamp granularity.)
: <<\end_disabled
rm -rf $file
{
[[ -N $file ]] && err_exit 'test -N $tmp/*: st_mtime>st_atime after creat'
sleep .02
print 'hello world'
[[ -N $file ]] || err_exit 'test -N $tmp/*: st_mtime<=st_atime after write'
sleep .02
read
[[ -N $file ]] && err_exit 'test -N $tmp/*: st_mtime>st_atime after read'
} > $file < $file
end_disabled

if	rm -rf "$file" && ln -s / "$file"
then	[[ -L "$file" ]] || err_exit '-L not working'
	[[ -L "$file"/ ]] && err_exit '-L with file/ not working'
fi
$SHELL -c 't=1234567890; [[ $t == @({10}(\d)) ]]' 2> /dev/null || err_exit ' @({10}(\d)) pattern not working'
$SHELL -c '[[ att_ == ~(E)(att|cus)_.* ]]' 2> /dev/null || err_exit ' ~(E)(att|cus)_* pattern not working'
$SHELL -c '[[ att_ =~ (att|cus)_.* ]]' 2> /dev/null || err_exit ' =~ ere not working'
$SHELL -c '[[ abc =~ a(b)c ]]' 2> /dev/null || err_exit '[[ abc =~ a(b)c ]] fails'
$SHELL -xc '[[ abc =~  \babc\b ]]' 2> /dev/null || err_exit '[[ abc =~ \babc\b ]] fails'
[[ abc == ~(E)\babc\b ]] || err_exit '\b not preserved for ere when not in ()'
[[ abc == ~(iEi)\babc\b ]] || err_exit '\b not preserved for ~(iEi) when not in ()'

e=$($SHELL -c '[ -z "" -a -z "" ]' 2>&1)
[[ $e ]] && err_exit "[ ... ] compatibility check failed -- $e"
i=hell
[[ hell0 == $i[0] ]]  ||  err_exit 'pattern $i[0] interpreted as array ref'
test '(' = ')' && err_exit '"test ( = )" should not be true'
[[ $($SHELL -c 'case  F in ~(Eilr)[a-z0-9#]) print ok;;esac' 2> /dev/null) == ok ]] || err_exit '~(Eilr) not working in case command'
[[ $($SHELL -c "case  Q in ~(Fi)q |  \$'\E') print ok;;esac" 2> /dev/null) == ok ]] || err_exit '~(Fi)q | \E not working in case command'

locales=$'\n'$(command -p locale -a 2>/dev/null)$'\n'
for l in C C.UTF-8 en_US.ISO8859-1 en_US.ISO8859-15 en_US.UTF-8
do	if	[[ $l != C* && ( $locales != *$'\n'"${l}"$'\n'* || -n $("$SHELL" -c "LC_COLLATE=${l}" 2>&1) ) ]]
	then	continue
	fi
	export LC_COLLATE=$l
	set -- \
		'A'   0 1 1   0 1 1      1 0 0   1 0 0   \
		'Z'   0 1 1   0 1 1      1 0 0   1 0 0   \
		'/'   0 0 0   0 0 0      1 1 1   1 1 1   \
		'.'   0 0 0   0 0 0      1 1 1   1 1 1   \
		'_'   0 0 0   0 0 0      1 1 1   1 1 1   \
		'-'   1 1 1   1 1 1      0 0 0   0 0 0   \
		'%'   0 0 0   0 0 0      1 1 1   1 1 1   \
		'@'   0 0 0   0 0 0      1 1 1   1 1 1   \
		'!'   0 0 0   0 0 0      1 1 1   1 1 1   \
		'^'   0 0 0   0 0 0      1 1 1   1 1 1   \
		# retain this line #
	while	(( $# >= 13 ))
	do	c=$1
		shift
		for p in \
			'[![.-.]]' \
			'[![.-.][:upper:]]' \
			'[![.-.]A-Z]' \
			'[!-]' \
			'[!-[:upper:]]' \
			'[!-A-Z]' \
			'[[.-.]]' \
			'[[.-.][:upper:]]' \
			'[[.-.]A-Z]' \
			'[-]' \
			'[-[:upper:]]' \
			'[-A-Z]' \
			# retain this line #
		do	e=$1
			shift
			[[ $c == $p ]]
			g=$?
			[[ $g == $e ]] || err_exit "[[ '$c' == $p ]] for LC_COLLATE=$l failed -- expected $e, got $g"
		done
	done
done
foo=([1]=a [2]=b [3]=c)
[[ -v foo[1] ]] ||  err_exit 'foo[1] should be set'
[[ ${foo[1]+x} ]] ||  err_exit '${foo[1]+x} should be x'
[[ ${foo[@]+x} ]] ||  err_exit '${foo[@]+x} should be x'
unset foo[1]
[[ -v foo[1] ]] && err_exit 'foo[1] should not be set'
[[ ${foo[1]+x} ]] &&  err_exit '${foo[1]+x} should be empty'
bar=(a b c)
[[ -v bar[1] ]]  || err_exit 'bar[1] should be set'
[[ ${bar[1]+x} ]] ||  err_exit '${foo[1]+x} should be x'
unset bar[1]
[[ ${bar[1]+x} ]] &&  err_exit '${foo[1]+x} should be empty'
[[ -v bar ]] || err_exit 'bar should be set'
[[ -v bar[1] ]] && err_exit 'bar[1] should not be set'
integer z=( 1 2 4)
[[ -v z[1] ]] || err_exit 'z[1] should be set'
unset z[1]
[[ -v z[1] ]] && err_exit 'z[1] should not be set'
typeset -si y=( 1 2 4)
[[ -v y[6] ]] && err_exit 'y[6] should not be set'
[[ -v y[1] ]] ||  err_exit  'y[1] should be set'
unset y[1]
[[ -v y[1] ]] && err_exit 'y[1] should not be set'
x=abc
[[ -v x[0] ]] || err_exit  'x[0] should be set'
[[ ${x[0]+x} ]] || err_exit print  '${x[0]+x} should be x'
[[ -v x[3] ]] && err_exit 'x[3] should not be set'
[[ ${x[3]+x} ]] && err_exit  '${x[0]+x} should be Empty'
unset x
[[ ${x[@]+x} ]] && err_exit  '${x[@]+x} should be Empty'
unset x y z foo bar

{ x=$($SHELL -c '[[ (( $# -eq 0 )) ]] && print ok') 2> /dev/null;}
[[ $x == ok ]] || err_exit '((...)) inside [[ ... ]] not treated as nested ()'

[[ -e /dev/fd/ ]] || err_exit '/dev/fd/ does not exits'
[[ -e /dev/tcp/ ]] || err_exit '/dev/tcp/ does not exist'
[[ -e /dev/udp/ ]] || err_exit '/dev/udp/ does not exist'
[[ -e /dev/xxx/ ]] &&  err_exit '/dev/xxx/ exists'

$SHELL 2> /dev/null -c '[[(-n foo)]]' || err_exit '[[(-n foo)]] should not require space in front of ('

$SHELL 2> /dev/null -c '[[ "]" == ~(E)[]] ]]' || err_exit 'pattern "~(E)[]]" does not match "]"'

unset var
[[ -v var ]] &&  err_exit '[[ -v var ]] should be false after unset var'
float var
[[ -v var ]] &&  err_exit '[[ -v var ]] should be false after float var'
unset var; float var=
[[ -v var ]] ||  err_exit '[[ -v var ]] should be true after float var='
unset var
[[ -v var ]] &&  err_exit '[[ -v var ]] should be false after unset var again'

test ! ! ! 2> /dev/null || err_exit 'test ! ! ! should return 0'
test ! ! x 2> /dev/null || err_exit 'test ! ! x should return 0'
test ! ! '' 2> /dev/null && err_exit 'test ! ! "" should return non-zero'

# ======
# The POSIX mode should disable the ancient 'test -t' compatibility hack.
if	[[ -o ?posix ]]
then	# need 'eval' in first test, as it's a parser hack, and ksh normally parses ahead of execution
	(set -o posix; eval '[ -t ] && test -t') >/dev/null \
	|| err_exit "posix mode fails to disable 'test -t' compat hack"
	# the compound command variant of the hack is in the 'test' builtin itself; no 'eval' needed
	expect=$'*: argument expected\n*: argument expected'
	actual=$(set -o posix; [ -n X -a -t ] 2>&1; test -n X -a -t 2>&1)
	[[ $actual == $expect ]] || err_exit "posix mode fails to disable 'test -t' compat hack (compound expression)" \
		"(expected output matching $(printf %q "$expect"), got $(printf %q "$actual"))"
fi

# ======
# POSIX specifies that on error, test builtin should always return status > 1
expect=$': test: 123x: arithmetic syntax error\nExit status: 2'
actual=$(test 123 -eq 123x 2>&1; echo "Exit status: $?")
[[ $actual == *"$expect" ]] || err_exit 'test builtin does not error out with status 2' \
	"(expected *$(printf %q "$expect"), got $(printf %q "$actual"))"

# ======
# The '=~' operator should work with curly brackets
error=$(set +x; "$SHELL" -c '[[ AATAAT =~ (AAT){2} ]]' 2>&1) \
|| err_exit "[[ AATAAT =~ (AAT){2} ]] does not match${error:+ (got $(printf %q "$error"))}"
error=$(set +x; "$SHELL" -c '[[ AATAATCCCAATAAT =~ (AAT){2}CCC(AAT){2} ]]' 2>&1) \
|| err_exit "[[ AATAATCCCAATAAT =~ (AAT){2}CCC(AAT){2} ]] does not match${error:+ (got $(printf %q "$error"))}"

# ======
# The -v unary operator should work for names with all type attributes.
empty=
for flag in a b i l n s si u ui usi uli E F H L Mtolower Mtoupper R X lX S Z
do	unset var
	typeset "-$flag" var
	[[ -v var ]] && err_exit "[[ -v var ]] should be false for unset var with attribute -$flag"
	[[ -n ${var+s} ]] && err_exit "[[ -n \${var+s} ]] should be false for unset var with attribute -$flag"
	unset var
	case $flag in
	n)	typeset -n var=empty ;;
	*)	typeset "-$flag" var= ;;
	esac
	[[ -v var ]] || err_exit "[[ -v var ]] should be true for empty var with attribute -$flag"
	[[ -n ${var+s} ]] || err_exit "[[ -n \${var+s} ]] should be true for empty var with attribute -$flag"
done

# ======
# Tests from ksh93v- for the -eq operator
[[ 010 -eq 10 ]] || err_exit '010 is not 10 in [[ ... ]]'

unset foo
foo=10
([[ foo -eq 10 ]]) || err_exit 'foo -eq 10 fails in [[ ... ]] with foo=10'

# ======
exit $((Errors<125?Errors:125))

########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1982-2012 AT&T Intellectual Property          #
#          Copyright (c) 2020-2026 Contributors to ksh 93u+m           #
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

. "${SHTESTS_COMMON:-${0%/*}/_common}"

{
x=abc
x+=def ;} 2> /dev/null
if	[[ $x != abcdef ]]
then	err_exit 'abc+def != abcdef'
fi
integer i=3
{ i+=4;} 2> /dev/null
if	(( i != 7 ))
then	err_exit '3+4!=7'
fi
iarray=( one two three )
{ iarray+= (four five six) ;} 2> /dev/null
if	[[ ${iarray[@]} != 'one two three four five six' ]]
then	err_exit 'indexed array append fails'
fi
unset iarray
iarray=one
{ iarray+= (four five six) ;} 2> /dev/null
if	[[ ${iarray[@]} != 'one four five six' ]]
then	err_exit 'indexed array append to scalar fails'
fi
typeset -A aarray
aarray=( [1]=1 [3]=4 [xyz]=xyz )
aarray+=( [2]=2 [3]=3 [foo]=bar )
if	[[ ${aarray[3]} != 3 ]]
then	err_exit 'associative array append fails'
fi
if	[[ ${#aarray[@]} != 5 ]]
then	err_exit 'number of elements of associative array append fails'
fi
point=(x=1 y=2)
point+=( y=3 z=4)
if	[[ ${point.y} != 3 ]]
then	err_exit 'compound append fails'
fi
if	[[ ${point.x} != 1 ]]
then	err_exit 'compound append to compound variable unsets existing variables'
fi
unset foo
foo=one
foo+=(two)
if	[[ ${foo[@]} != 'one two' ]]
then	err_exit 'array append to non array variable fails'
fi
unset foo
foo[0]=(x=3)
foo+=(x=4)
[[ ${foo[1].x} == 4 ]] || err_exit 'compound append to indexed array not working'
[[ ${foo[0].x} == 3 ]] || err_exit 'compound append to indexed array unsets existing variables'

unset foo
foo=a
foo+=''
[[ $foo == 'a' ]] || err_exit 'appending an empty string not working'

unset x z arr
typeset -a x=(a b)
x+=(c d)
exp='typeset -a x=(a b c d)'
[[ $(typeset -p x) == "$exp" ]] || err_exit 'append (c d) to indexed array not working'

typeset -a arr=(a=b b=c)
arr+=(c=d d=e)
exp='typeset -a arr=(a\=b b\=c c\=d d\=e)'
[[ $(typeset -p arr) == "$exp" ]] || err_exit 'append (c=d d=e) to indexed array not working'

exp='typeset -a z=(a\=b b\=c d\=3 e f\=l)'
typeset -a z=(a=b b=c)
{ z+=(d=3 e f=l); } 2> /dev/null
[[ $(typeset -p z) == "$exp" ]] || err_exit 'append (d=3 e f=l) to indexed array not working'

unset arr2
exp='typeset -a arr2=(b\=c :)'
typeset -a arr2
arr2+=(b=c :)
[[ $(typeset -p arr2) == "$exp" ]] || err_exit 'append (b=c :) to indexed array not working'

unset arr2
exp='typeset -a arr2=(b\=c xxxxx)'
typeset -a arr2
{
	arr2+=(b=c xxxxx)
} 2> /dev/null
[[ $(typeset -p arr2) == "$exp" ]] || err_exit 'append (b=c xxxxx) to indexed array not working'

# ======
unset foo
exp='typeset -x foo=barbaz'
got=$(foo=bar; export foo+=baz 2>&1 && typeset -p foo)
[[ e=$? -eq 0 && $got == "$exp" ]] || err_exit 'declaration command does not support +=' \
	"(expected status 0, $(printf %q "$exp");" \
	"got status $e$( ((e>128)) && print -n /SIG && kill -l "$e"), $(printf %q "$got"))"
exp='typeset -x -a foo=([23]=barbaz)'
got=$(foo[23]=bar; export foo[23]+=baz 2>&1 && typeset -p foo)
[[ e=$? -eq 0 && $got == "$exp" ]] || err_exit 'declaration command does not support +=' \
	"(expected status 0, $(printf %q "$exp");" \
	"got status $e$( ((e>128)) && print -n /SIG && kill -l "$e"), $(printf %q "$got"))"

# ======
# Appending a compound value word (a value in parentheses) to a non-empty
# indexed array used to append an uninitialised pointer, producing garbage
# output. Compound elements were overwritten or discarded.
unset foo
foo[1]=(x=7)
foo+=( (x=9) )
got=$(typeset -p foo)
exp='typeset -a foo=([1]=(x=7) [2]=(x=9))'
[[ $got == "$exp" ]] || err_exit "compound value in += not appended" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=${foo[*]}
exp=$'(\n\tx=7\n) (\n\tx=9\n)'
[[ $got == "$exp" ]] || err_exit "compound value in += corrupts the array" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
[[ ${foo[2].x} == 9 ]] || err_exit 'compound value in += not appended as a compound value'
[[ ${#foo[@]} == 2 ]] || err_exit 'compound value in += changed the number of elements'

unset foo
foo=( one )
foo+=( (two three) )
got=$(typeset -p foo)
exp='typeset -a foo=(one (two three) )'
[[ $got == "$exp" ]] || err_exit "nested array value in += not appended" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
[[ ${foo[1][1]} == three ]] || err_exit 'nested array value in += not appended as a nested array'
[[ ${#foo[@]} == 2 ]] || err_exit 'nested array value in += changed the number of elements'

# Appending to an indexed array in stages must produce the same array as
# assigning the whole value at once, also if the appended value list
# contains a multidimensional array value. In an earlier iteration of this
# fix, the appended list was nested inside a new element instead of being
# appended to the array itself, adding a spurious array dimension.
unset foo allatonce
typeset -a foo=(1 2 (3 4 5) 6 7)
typeset -a allatonce=(1)
allatonce+=(2 (3 4 5) 6)
allatonce+=(7)
got=$(typeset -p allatonce)
exp='typeset -a allatonce=(1 2 (3 4 5) 6 7)'
[[ $got == "$exp" ]] || err_exit "appending a nested array value in stages failed" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=${allatonce[2][*]}
exp='3 4 5'
[[ $got == "$exp" ]] || err_exit "nested array value not appended to the array itself" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
[[ ${allatonce[*]} == "${foo[*]}" && ${!allatonce[*]} == "${!foo[*]}" ]] ||
	err_exit 'appending a nested array value in stages did not produce the same array as a single assignment'

# The same, with the appended value list starting with a multidimensional array value.
unset foo
typeset -a foo=(1)
foo+=( (3 4) 6 )
got=$(typeset -p foo)
exp='typeset -a foo=(1 (3 4) 6)'
[[ $got == "$exp" ]] || err_exit "appending a list starting with a nested array value" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=${foo[1][*]}
exp='3 4'
[[ $got == "$exp" ]] || err_exit 'nested array value in += not appended as a nested array' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=${foo[2]}
exp=6
[[ $got == "$exp" ]] || err_exit 'value after a nested array value in += not appended' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# An appended value list must be placed after the highest existing subscript.
unset foo
typeset -a foo=([5]=x)
foo+=( 2 (3 4) )
got=${!foo[*]}
exp='5 6 7'
[[ $got == "$exp" ]] || err_exit 'appended subscripts are wrong' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
[[ ${foo[6]} == 2 && ${foo[7][*]} == '3 4' ]] ||
	err_exit 'nested array value in += not appended after the last element'

# The parse tree must not be modified, so that appending the same
# values list more than once (e.g., in a loop or function) works.
unset foo
typeset -a foo=(1)
for i in 1 2
do	foo+=( 2 (3 4) )
done
got=$(typeset -p foo)
exp='typeset -a foo=(1 2 (3 4) 2 (3 4) )'
[[ $got == "$exp" ]] || err_exit "appending the same value list twice" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Multidimensional nested array values must be appended intact.
unset foo
typeset -a foo=(1)
foo+=( 2 (3 (4 5) 6) 7 )
got=$(typeset -p foo)
exp='typeset -a foo=(1 2 (3 (4 5) 6) 7)'
[[ $got == "$exp" ]] || err_exit "appending a multidimensional array value" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
[[ ${foo[2][1][*]} == '4 5' ]] || err_exit 'multidimensional array value in += not appended intact'

# ======
exit $((Errors<125?Errors:125))

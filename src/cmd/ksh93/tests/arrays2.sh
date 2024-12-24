########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1982-2012 AT&T Intellectual Property          #
#          Copyright (c) 2020-2024 Contributors to ksh 93u+m           #
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
#         hyenias <58673227+hyenias@users.noreply.github.com>          #
#                                                                      #
########################################################################

. "${SHTESTS_COMMON:-${0%/*}/_common}"

for	((i=0; i < 4; i++ ))
do	for	((j=0; j < 5; j++ ))
	do	a[i][j]=$i$j
	done
done
for	((i=0; i < 4; i++ ))
do	for	((j=0; j < 5; j++ ))
	do	[[ ${a[i][j]} == "$i$j" ]] || err_exit "\${a[$i][$j]} != $i$j"
	done
done
for	((i=0; i < 4; i++ ))
do	j=0;for k in ${a[i][@]}
	do	[[ $k == "$i$j" ]] || err_exit "\${a[i][@]} != $i$j"
		(( j++ ))
	done
done
unset a
a=(
	( 00 01 02 03 04 )
	( 10 11 12 13 14 15)
	( 20 21 22 23 24 )
	( 30 31 32 33 34 )
)

function check
{
	nameref a=$1
	nameref b=a[2]
	typeset c=$1
	integer i j
	for	((i=0; i < 4; i++ ))
	do	for	((j=0; j < 5; j++ ))
		do	[[ ${a[$i][$j]} == "$i$j" ]] || err_exit "\${$c[$i][$j]} != $i$j"
		done
	done
	(( ${#a[@]} == 4 )) || err_exit "\${#$c[@]} not 4"
	(( ${#a[0][@]} == 5 )) || err_exit "\${#$c[0][@]} not 5"
	(( ${#a[1][@]} == 6 )) || err_exit "\${#$c[1][@]} not 6"
	set -s -- ${!a[@]}
	[[ ${@} == '0 1 2 3' ]] || err_exit "\${!$c[@]} not 0 1 2 3"
	set -s -- ${!a[0][@]}
	[[ ${@} == '0 1 2 3 4' ]] || err_exit "\${!$c[0][@]} not 0 1 2 3 4"
	set -s -- ${!a[1][@]}
	[[ ${@} == '0 1 2 3 4 5' ]] || err_exit "\${!$c[1][@]} not 0 1 2 3 4 5"
	[[ $a == 00 ]] || err_exit  "\$$c is not 00"
	[[ ${a[0]} == 00 ]] || err_exit  "\${$a[0]} is not 00"
	[[ ${a[0][0]} == 00 ]] || err_exit  "${a[0][0]} is not 00"
	[[ ${a[0][0][0]} == 00 ]] || err_exit  "\${$c[0][0][0]} is not 00"
	[[ ${a[0][0][1]} == '' ]] || err_exit  "\${$c[0][0][1]} is not empty"
	[[ ${b[3]} == 23 ]] || err_exit "${!b}[3] not = 23"
}

check a

unset a
typeset -A a
for	((i=0; i < 4; i++ ))
do	for	((j=0; j < 5; j++ ))
	do	a[$i][j]=$i$j
	done
done
for	((i=0; i < 4; i++ ))
do	for	((j=0; j < 5; j++ ))
	do	[[ ${a[$i][j]} == "$i$j" ]] || err_exit "\${a[$i][$j]} == $i$j"
	done
done
a[1][5]=15
b=(
	[0]=( 00 01 02 03 04 )
	[1]=( 10 11 12 13 14 15)
	[2]=( 20 21 22 23 24 )
	[3]=( 30 31 32 33 34 )
)
check b
[[ ${a[1][@]} == "${b[1][@]}" ]] || err_exit "a[1] not equal to b[1]"
c=(
	[0]=( [0]=00 [1]=01 [2]=02 [3]=03 [4]=04 )
	[1]=( [0]=10 [1]=11 [2]=12 [3]=13 [4]=14 [5]=15)
	[2]=( [0]=20 [1]=21 [2]=22 [3]=23 [4]=24 )
	[3]=( [0]=30 [1]=31 [2]=32 [3]=33 [4]=34 )
)
check c
typeset -A d
d[0]=( [0]=00 [1]=01 [2]=02 [3]=03 [4]=04 )
d[1]=( [0]=10 [1]=11 [2]=12 [3]=13 [4]=14 [5]=15)
d[2]=( [0]=20 [1]=21 [2]=22 [3]=23 [4]=24 )
d[3]=( [0]=30 [1]=31 [2]=32 [3]=33 [4]=34 )
check d
unset a b c d
[[ ${a-set} ]] || err_exit "a is set after unset"
[[ ${b-set} ]] || err_exit "b is set after unset"
[[ ${c-set} ]] || err_exit "c is set after unset"
[[ ${d-set} ]] || err_exit "c is set after unset"

$SHELL 2> /dev/null <<\+++ ||  err_exit 'input of 3 dimensional array not working'
typeset x=(
	( (g G) (h H) (i I) )
	( (d D) (e E) (f F) )
	( (a A) (b B) (c C) )
)
[[ ${x[0][0][0]} == g ]] || exit 1
[[ ${x[1][1][0]} == e ]] || exit 1
[[ ${x[1][1][1]} == E ]] || exit 1
[[ ${x[0][2][1]} == I ]] || exit 1
+++

typeset -a -si x=( [0]=(1 2 3) [1]=(4 5 6) [2]=(7 8 9) )
[[ ${x[1][1]} == 5 ]] || err_exit 'changing two dimensional indexed array to short integer failed'
unset x
typeset -A -si x=( [0]=(1 2 3) [1]=(4 5 6) [2]=(7 8 9) )
[[ ${x[1][2]} == 6 ]] || err_exit 'changing two dimensional associative array to short integer failed'

unset ar x y
integer -a ar
integer i x y
for (( i=0 ; i < 100 ; i++ ))
do	(( ar[y][x++]=i ))
	(( x > 9 )) && (( y++ , x=0 ))
done
[[ ${#ar[0][*]} == 10 ]] || err_exit "\${#ar[0][*]} is '${#ar[0][*]}', should be 10"
[[ ${#ar[*]} == 10 ]] || err_exit  "\${#ar[*]} is '${#ar[*]}', should be 10"
[[ ${ar[5][5]} == 55 ]] || err_exit "ar[5][5] is '${ar[5][5]}', should be 55"

unset ar
integer -a ar
x=0 y=0
for (( i=0 ; i < 81 ; i++ ))
do	nameref ar_y=ar[$y]
	(( ar_y[x++]=i ))
	(( x > 8 )) && (( y++ , x=0 ))
	typeset +n ar_y
done
[[ ${#ar[0][*]} == 9 ]] || err_exit "\${#ar[0][*]} is '${#ar[0][*]}', should be 9"
[[ ${#ar[*]} == 9 ]] || err_exit  "\${#ar[*]} is '${#ar[*]}', should be 9"
[[ ${ar[4][4]} == 40 ]] || err_exit "ar[4][4] is '${ar[4][4]}', should be 40"

$SHELL 2> /dev/null -c 'compound c;float -a c.ar;(( c.ar[2][3][3] = 5))' || 'multidimensional arrays in arithmetic expressions not working'

expected='typeset -a -l -E c.ar=(typeset -a [2]=(typeset -a [3]=([3]=5) ) )'
unset c
float c.ar
c.ar[2][3][3]=5
[[ $(typeset -p c.ar) == "$expected" ]] || err_exit "c.ar[2][3][3]=5;typeset -c c.ar expands to $(typeset -p c.ar)"

unset values
float -a values=( [1][3]=90 [1][4]=89 )
function fx
{
	nameref arg=$1
	[[ ${arg[0..5]} == '90 89' ]] || err_exit '${arg[0..5]} not correct where arg is a nameref to values[1]'
}
fx values[1]

function test_short_integer
{
	compound out=( typeset stdout stderr ; integer res )
	compound -r -a tests=(
		( cmd='integer -s -r -a x=( 1 2 3 ) ; print "${x[2]}"' stdoutpattern='3' )
		( cmd='integer -s -r -A x=( [0]=1 [1]=2 [2]=3 ) ; print "${x[2]}"' stdoutpattern='3' )
		# 2D integer arrays: the following two tests crash for both "integer -s" and "integer"
		( cmd='integer    -r -a x=( [0]=( [0]=1 [1]=2 [2]=3 ) [1]=( [0]=4 [1]=5 [2]=6 ) [2]=( [0]=7 [1]=8 [2]=9 ) ) ; print "${x[1][1]}"' stdoutpattern='5' )
		( cmd='integer -s -r -a x=( [0]=( [0]=1 [1]=2 [2]=3 ) [1]=( [0]=4 [1]=5 [2]=6 ) [2]=( [0]=7 [1]=8 [2]=9 ) ) ; print "${x[1][1]}"' stdoutpattern='5' )
   	)
	typeset testname
	integer i

	for (( i=0 ; i < ${#tests[@]} ; i++ )) ; do
		nameref tst=tests[i]
		testname="${0}/${i}"

		out.stderr="${ { out.stdout="${ ${SHELL} -o nounset -o errexit -c "${tst.cmd}" ; (( out.res=$? )) ; }" ; } 2>&1 ; }"

	        [[ "${out.stdout}" == ${tst.stdoutpattern}      ]] || err_exit "${testname}: Expected stdout to match $(printf '%q\n' "${tst.stdoutpattern}"), got $(printf '%q\n' "${out.stdout}")"
       		[[ "${out.stderr}" == ''			]] || err_exit "${testname}: Expected empty stderr, got $(printf '%q\n' "${out.stderr}")"
		(( out.res == 0 )) || err_exit "${testname}: Unexpected exit code ${out.res}"
	done

	return 0
}
# run tests
test_short_integer

typeset -a arr=( ( 00 ) ( 01 ) ( 02 ) ( 03 ) ( 04 ) ( 05 ) ( 06 ) ( 07 ) ( 08 ) ( 09 ) ( 10 ) )
typeset -i i=10 j=0
{  y=$( echo ${arr[i][j]} ) ;} 2> /dev/null
[[ $y == 10 ]] || err_exit '${arr[10][0] should be 10 '

unset cx l
compound cx
typeset -a cx.ar[4][4]
print -v cx > /dev/null
print -v cx | read -C l 2> /dev/null || err_exit 'read -C fails from output of print -v'
((SHOPT_FIXEDARRAY)) && [[ ${cx%cx=} != "${l%l=}" ]] && err_exit 'print -v for compound variable with fixed 2d array not working'

unset foo
typeset -A foo
typeset -A foo[bar]
foo[bar][x]=2
(( foo[bar][x]++ ))
exp=3
[[ ${foo[bar][x]} == $exp ]] || err_exit "subscript gets added incorrectly to an associative array when ++ operator is called" \
	"(expected '$exp', got '${foo[bar][x]}')"

unset A
typeset -A A
typeset -A A[a]
A[a][z]=1
[[ ${!A[a][@]} == z ]] || err_exit 'A[a] should only have subscript z' \
	"(got $(printf %q "${!A[a][@]}"))"

# ======
# Multidimensional arrays with an unset method shouldn't cause a crash.
# The test itself must be run inside of a function.
multiarray_unset="$tmp/multiarray_unset.sh"
cat >| "$multiarray_unset" << EOF
function foo {
	typeset -a a
	a.unset() {
		print unset
	}
	a[3][6][11][20]=7
}
foo
EOF
{ $SHELL "$multiarray_unset" > /dev/null; } 2>/dev/null
let "(e=$?)==0" || err_exit 'Multidimensional arrays with an unset method crash ksh' \
		"(got status $e$( ((e>128)) && print -n /SIG && kill -l "$e"))"

# ======
# Multidimensional indexed array arithmetic assignment operation tests
ini() { unset arr; typeset -a arr=((10 20) 6 8); }
[[ $( ini; ((arr[0][0]+=1)); typeset -p arr) == 'typeset -a arr=((11 20) 6 8)' ]] || err_exit 'ASSIGNOP: arr[0][0]+=1 failed.'
[[ $( ini; ((arr[0][1]+=1)); typeset -p arr) == 'typeset -a arr=((10 21) 6 8)' ]] || err_exit 'ASSIGNOP: arr[0][1]+=1 failed.'
[[ $( ini; ((arr[0][2]+=1)); typeset -p arr) == 'typeset -a arr=((10 20 1) 6 8)' ]] || err_exit 'ASSIGNOP: arr[0][2]+=1 failed.'
[[ $( ini; ((arr[0][1]+=arr[1])); typeset -p arr) == 'typeset -a arr=((10 26) 6 8)' ]] || err_exit 'ASSIGNOP: arr[0][1]+=arr[1] failed.'
[[ $( ini; ((arr[0][2]+=arr[1])); typeset -p arr) == 'typeset -a arr=((10 20 6) 6 8)' ]] || err_exit 'ASSIGNOP: arr[0][2]+=arr[1] failed.'
[[ $( ini; ((arr[1]+=1)); typeset -p arr) == 'typeset -a arr=((10 20) 7 8)' ]] || err_exit 'ASSIGNOP: arr[1]+=1 failed.'
[[ $( ini; ((arr[2]+=1)); typeset -p arr) == 'typeset -a arr=((10 20) 6 9)' ]] || err_exit 'ASSIGNOP: arr[2]+=1 failed.'
[[ $( ini; ((arr[3]+=1)); typeset -p arr) == 'typeset -a arr=((10 20) 6 8 1)' ]] || err_exit 'ASSIGNOP: arr[3]+=1 failed.'
[[ $( ini; ((arr[1]+=arr[0][0])); typeset -p arr) == 'typeset -a arr=((10 20) 16 8)' ]] || err_exit 'ASSIGNOP: arr[1]+=arr[0][0] failed.'
[[ $( ini; ((arr[1]+=arr[0][1])); typeset -p arr) == 'typeset -a arr=((10 20) 26 8)' ]] || err_exit 'ASSIGNOP: arr[1]+=arr[0][1] failed.'
[[ $( ini; ((arr[2]+=arr[0][0])); typeset -p arr) == 'typeset -a arr=((10 20) 6 18)' ]] || err_exit 'ASSIGNOP: arr[2]+=arr[0][0] failed.'
[[ $( ini; ((arr[2]+=arr[0][1])); typeset -p arr) == 'typeset -a arr=((10 20) 6 28)' ]] || err_exit 'ASSIGNOP: arr[2]+=arr[0][1] failed.'
[[ $( ini; ((arr[3]+=arr[0][1])); typeset -p arr) == 'typeset -a arr=((10 20) 6 8 20)' ]] || err_exit 'ASSIGNOP: arr[2]+=arr[0][1] failed.'
[[ $( ini; ((arr[0][1]+=arr[1]+13, arr[2]=42)); typeset -p arr) == 'typeset -a arr=((10 39) 6 42)' ]] || err_exit 'ASSIGNOPs: (_+=,_=_) failed.'
[[ $( ini; (( (arr[0][1]*=arr[2]) ? arr[0][1]+=arr[1] : arr[1]=7)); typeset -p arr) == 'typeset -a arr=((10 166) 6 8)' ]] || err_exit 'ASSIGNOPs: (_*=_)?_:_ true failed.'
[[ $( ini; (( (arr[0][2]*=arr[2]) ? arr[0][1]+=arr[1] : arr[1]=7)); typeset -p arr) == 'typeset -a arr=((10 20 0) 7 8)' ]] || err_exit 'ASSIGNOPs: (_*=_)?_:_ false failed.'
unset arr
unset -f ini

# ======
# Array slicing
# https://github.com/ksh93/ksh/issues/605
unset arr
exp='1 2 3'
got=$(arr=(1 2 3); echo "${arr[*]:0:${#arr[@]}}")
[[ $got == "$exp" ]] || err_exit "array slicing test 1 (expected $(printf %q "$exp"), got $(printf %q "$got"))"
exp='1 2 3 4'
got=$(arr=(1 2 3 4 5); echo "${arr[*]:0:${arr[@]:3:1}}")
[[ $got == "$exp" ]] || err_exit "array slicing test 2 (expected $(printf %q "$exp"), got $(printf %q "$got"))"
exp='a b c'
got=$(typeset -a arr=(a b (c d e)); echo ${arr[@]:0:${#arr[@]}})
[[ $got == "$exp" ]] || err_exit "array slicing test 3 (expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(typeset -a arr=(a (b d e) c); num=${#arr[@]}; echo ${arr[@]:0:num})
[[ $got == "$exp" ]] || err_exit "array slicing test 4 (expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(typeset -a arr=(a (b d e) c); echo ${arr[@]})
[[ $got == "$exp" ]] || err_exit "array slicing test 5 (expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(typeset -a arr=(a (b d e) c); echo ${arr[@]:})
[[ $got == "$exp" ]] || err_exit "array slicing test 6 (expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(typeset -a arr=(a (b d e) c); echo ${arr[@]:0})
[[ $got == "$exp" ]] || err_exit "array slicing test 7 (expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(typeset -a arr=(a (b d e) c); echo ${arr[@]:0:${#arr[@]}})
[[ $got == "$exp" ]] || err_exit "array slicing test 8 (expected $(printf %q "$exp"), got $(printf %q "$got"))"
# below, we must have 'arr=( (a...' and not 'arr=((a...' due to https://github.com/ksh93/ksh/issues/269
got=$(typeset -a arr=( (a d e) b c); echo ${arr[@]:0:${#arr[@]}})
[[ $got == "$exp" ]] || err_exit "array slicing test 9 (expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(typeset -A arr=(a (b d e) c); num=${#arr[@]}; echo ${arr[@]:0:num})
[[ $got == "$exp" ]] || err_exit "array slicing test 10 (expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(typeset -A arr=(a (b d e) c); echo ${arr[@]:0:${#arr[@]}})
[[ $got == "$exp" ]] || err_exit "array slicing test 11 (expected $(printf %q "$exp"), got $(printf %q "$got"))"
exp='c d e'
got=$(typeset -a arr=(a (b d e) c d e); echo ${arr[@]:2:${#arr[@]}-2})
[[ $got == "$exp" ]] || err_exit "array slicing test 12 (expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(typeset -A arr=(a (b d e) c d e); echo ${arr[@]:2:${#arr[@]}-2})
[[ $got == "$exp" ]] || err_exit "array slicing test 13 (expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# Reserved words as values
# https://github.com/ksh93/ksh/issues/790
got=$(set +x; eval 'typeset -a a1=((demo) (select) (if) (case) (while))' 2>&1 && typeset -p a1)
exp='typeset -a a1=((demo) (select) (if) (case) (while) )'
[[ $got == "$exp" ]] || err_exit "reserved word assigned as multidimensional array value" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(set +x; eval 'typeset -a a1=((demo) (select) if case while)' 2>&1 && typeset -p a1)
exp='typeset -a a1=((demo) (select) if case while)'
[[ $got == "$exp" ]] || err_exit "reserved word assigned as top-level value in multidimensional array" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(set +x; eval 'a1=((demo) (select) (if) (case) (while))' 2>&1 && typeset -p a1)
exp='typeset -a a1=((demo) (select) (if) (case) (while) )'
[[ $got == "$exp" ]] || err_exit "reserved word assigned as implicit multidimensional array value" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(set +x; eval 'a1=((demo) (select) if case while)' 2>&1 && typeset -p a1)
exp='typeset -a a1=((demo) (select) if case while)'
[[ $got == "$exp" ]] || err_exit "reserved word assigned as top-level value in implicit multidimensional array" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
exit $((Errors<125?Errors:125))

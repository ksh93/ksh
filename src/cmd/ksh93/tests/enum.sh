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

enum Color_t=(red green blue orange yellow)
enum -i Bool_t=(False True)
for ((i=0; i < 1000; i++))
do
Color_t x
[[ $x == red ]] || err_exit 'Color_t does not default to red'
x=orange
[[ $x == orange ]] || err_exit '$x should be orange'
( x=violet) 2> /dev/null && err_exit 'x=violet should fail'
x[2]=green
[[ ${x[2]} == green ]] || err_exit '${x[2]} should be green'
(( x[2] == 1 )) || err_exit '((x[2]!=1))'
[[ $((x[2])) == 1 ]] || err_exit '$((x[2]))!=1'
[[ $x == orange ]] || err_exit '$x is no longer orange'
Color_t -A y
y[foo]=yellow
[[ ${y[foo]} == yellow ]] || err_exit '${y[foo]} != yellow'
(( y[foo] == 4 )) || err_exit '(( y[foo] != 4))'
unset y
typeset -a [Color_t] z
z[green]=xyz
[[ ${z[green]} == xyz ]] || err_exit '${z[green]} should be xyz'
[[ ${z[1]} == xyz ]] || err_exit '${z[1]} should be xyz'
z[orange]=bam
[[ ${!z[@]} == 'green orange' ]] || err_exit '${!z[@]} == "green orange"'
unset x
Bool_t x
[[ $x == False ]] || err_exit 'Bool_t not defaulting to False'
x=true
[[ $x == True ]] || err_exit 'Bool_t not case sensitive'
unset x y z
done
(
typeset -T X_t=( typeset name=aha )
typeset -a[X_t] arr
) 2> /dev/null
[[ $? == 1 ]] || err_exit 'typeset -a[X_t] should generate an error message when X-t is not an enumeration type'

typeset -a [Color_t] arr
arr[green]=foo
[[ ${arr[1]} == ${arr[green]}  ]] || err_exit 'arr[1] != arr[green]'
read -A arr <<<  'x y z xx yy'
[[ ${arr[1]} == ${arr[green]}  ]] || err_exit 'arr[1] != arr[green] after read'

# ======
# Various fixed bugs with associative and indexed arrays of a type created by 'enum'
# https://github.com/ksh93/ksh/issues/87

# values not specified by the enum type should be blocked
exp=': Color_t: clr[2]: invalid value WRONG'
got=$(set +x; redirect 2>&1; Color_t -a clr=(red blue WRONG yellow); printf '%s\n' "${clr[@]}")
(((e = $?) == 1)) && [[ $got == *"$exp" ]] || err_exit "indexed enum array, unspecified value:" \
	"expected status 1, *$(printf %q "$exp"); got status $e, $(printf %q "$got")"
exp=': clr: invalid value BAD'
got=$(set +x; redirect 2>&1; Color_t -A clr=([foo]=red [bar]=blue [bad]=BAD); printf '%s\n' "${clr[@]}")
(((e = $?) == 1)) && [[ $got == *"$exp" ]] || err_exit "associative enum array, unspecified value:" \
	"expected status 1, *$(printf %q "$exp"); got status $e, $(printf %q "$got")"

# associative enum array
Color_t -A Colors=([foo]=red [bar]=blue [bad]=green [zut]=orange [blauw]=blue [rood]=red [groen]=green [geel]=yellow)
exp='green blue blue red yellow green red orange'
got=${Colors[@]}
[[ $got == "$exp" ]] || err_exit "\${array[@]} doesn't yield all values for associative enum arrays" \
	"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
exp='Color_t -A Colors=([bad]=green [bar]=blue [blauw]=blue [foo]=red [geel]=yellow [groen]=green [rood]=red [zut]=orange)'
got=$(typeset -p Colors)
[[ $got == "$exp" ]] || err_exit "'typeset -p' doesn't yield all values for associative enum arrays" \
	"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
unset Colors
got=$(typeset -p Colors)
[[ -n $got ]] && err_exit "unsetting associative enum array does not work (got $(printf %q "$got"))"

# indexed enum array
Color_t -a iColors=(red blue green orange blue red green yellow)
exp='red blue green orange blue red green yellow'
got=${iColors[@]}
[[ $got == "$exp" ]] || err_exit "\${array[@]} doesn't yield all values for indexed enum arrays" \
	"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
exp='Color_t -a iColors=(red blue green orange blue red green yellow)'
got=$(typeset -p iColors)
[[ $got == "$exp" ]] || err_exit "'typeset -p' doesn't yield all values for indexed enum arrays" \
	"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
unset iColors
got=$(typeset -p iColors)
[[ -n $got ]] && err_exit "unsetting indexed enum array does not work (got $(printf %q "$got"))"

# assigning the first enum type element should work
Color_t -a testarray
testarray[3]=red
exp="red red"
got="${testarray[3]:-BUG} ${testarray[@]:-BUG}"
[[ $got == "$exp" ]] || err_exit "assigning first enum element to indexed array failed" \
	"(expected $(printf %q "$exp"); got $(printf %q "$got"))"

# ======
# https://github.com/ksh93/ksh/issues/256
cmd='enum Color_t=(red green blue); Color_t -A Colors=([rubie]=red [verde]=green [blau]=blue); typeset -p Colors'
exp='Color_t -A Colors=([blau]=blue [rubie]=red [verde]=green)'
got=$("$SHELL" -c "$cmd" 2>&1)
[[ $got == "$exp" ]] || err_exit "-c failed" \
	"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
got=$("$SHELL" -c "eval '$cmd'" 2>&1)
[[ $got == "$exp" ]] || err_exit "-c script with eval failed" \
	"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
print "$cmd" >cmd.sh
got=$("$SHELL" -c '. ./cmd.sh' 2>&1)
[[ $got == "$exp" ]] || err_exit "dotted script failed" \
	"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
got=$("$SHELL" -c 'source ./cmd.sh' 2>&1)
[[ $got == "$exp" ]] || err_exit "sourced script failed" \
	"(expected $(printf %q "$exp"); got $(printf %q "$got"))"

# ======
# https://github.com/ksh93/ksh/issues/335
unset a
Color_t a

let "a=5" 2>/dev/null && err_exit "arithmetic can assign out of range (positive)"
let "a=-1" 2>/dev/null && err_exit "arithmetic can assign out of range (negative)"
a=yellow; let "a++" 2>/dev/null && err_exit "arithmetic can assign out of range (increment)"
a=red; let "a--" 2>/dev/null && err_exit "arithmetic can assign out of range (decrement)"
a=orange; let "a+=2" 2>/dev/null && err_exit "arithmetic can assign out of range (add)"
a=green; let "a-=2" 2>/dev/null && err_exit "arithmetic can assign out of range (subtract)"
a=blue; let "a*=3" 2>/dev/null && err_exit "arithmetic can assign out of range (multiply)"

# ======
exit $((Errors<125?Errors:125))

########################################################################
#                                                                      #
#              This file is part of the ksh 93u+m package              #
#          Copyright (c) 1982-2012 AT&T Intellectual Property          #
#             Copyright (c) 2021 Contributors to ksh 93u+m             #
#                    <https://github.com/ksh93/ksh>                    #
#                      and is licensed under the                       #
#                 Eclipse Public License, Version 1.0                  #
#                                                                      #
#                A copy of the License is available at                 #
#          http://www.eclipse.org/org/documents/epl-v10.html           #
#         (with md5 checksum b35adb5213ca9657e911e9befb180842)         #
#                                                                      #
#                              hyenias                                 #
#                                                                      #
########################################################################

. "${SHTESTS_COMMON:-${0%/*}/_common}"

rtests=(
	(
		ini='typeset -ri x=900'
		chg='typeset -R1 x'
		res='typeset -p x'
		exp='typeset -r -i x=900'
	)
	(
		ini='typeset -ri x=900'
		chg='typeset -X x; typeset -L1 x; typeset -i x'
		res='typeset -p x'
		exp='typeset -r -i x=900'
	)
	(
		ini='typeset -L3 x=0123; readonly x'
		chg='typeset -Z5 x'
		res='typeset -p x'
		exp='typeset -r -L 3 x=012'
	)
	(
		ini='typeset -rL3 x=0123'
		chg='typeset -R5 x'
		res='typeset -p x'
		exp='typeset -r -L 3 x=012'
	)
	(
		ini='typeset -rL3 x=0123'
		chg='typeset -L5 x'
		res='typeset -p x'
		exp='typeset -r -L 3 x=012'
	)
	(
		ini='typeset -R3 x; typeset -r x'
		chg='x=0'
		res='typeset -p x'
		exp='typeset -r -R 3 x'
	)
	(
		ini='set -A arr[0] a b c; readonly arr'
		chg='arr[1]=1'
		res='typeset -p arr'
		exp='typeset -r -a arr=((a b c) )'
	)
	(
		ini='set -A arr[0] a b c; readonly arr'
		chg='arr[0][1]=d'
		res='typeset -p arr'
		exp='typeset -r -a arr=((a b c) )'
	)
	(
		ini='set -A arr[0] a b c; readonly arr'
		chg='set +A arr 1'
		res='typeset -p arr'
		exp='typeset -r -a arr=((a b c) )'
	)
	(
		ini='typeset -a arr=((a b c) 1); readonly arr'
		chg='arr[1]=d'
		res='typeset -p arr'
		exp='typeset -r -a arr=((a b c) 1)'
	)
	(
		ini='typeset -ra arr=((a b c) 1)'
		chg='arr[1]=d'
		res='typeset -p arr'
		exp='typeset -r -a arr=((a b c) 1)'
	)
	(
		ini='typeset -ra arr=((a b c) 1)'
		chg='arr[1]=()'
		res='typeset -p arr'
		exp='typeset -r -a arr=((a b c) 1)'
	)
	(
		ini='typeset -r -a arr=((a b c) 1)'
		chg='arr[0][1]=d'
		res='typeset -p arr'
		exp='typeset -r -a arr=((a b c) 1)'
	)
	(
		ini='typeset -r -a arr=((a b c) 1)'
		chg='arr[0][1]=()'
		res='typeset -p arr'
		exp='typeset -r -a arr=((a b c) 1)'
	)
	(
		ini='typeset -r -a arr=((a b c) 1)'
		chg='arr[0][1]=(d)'
		res='typeset -p arr'
		exp='typeset -r -a arr=((a b c) 1)'
	)
	(# For a move, the unset should be blocked but copy succeeds.
		ini='readonly old=RDONLY'
		chg='typeset -m new=old'
		res='typeset -p old new'
		exp=$'typeset -r old=RDONLY\nnew=RDONLY'
	)
	(
		ini='typeset -C arr=(typeset -r -a alphas=(a b c);name=x)'
		chg='arr.name=y; arr.alphas[1]=b'
		res='typeset -p arr'
		exp='typeset -C arr=(typeset -r -a alphas=(a b c);name=y)'
	)
	(
		ini='typeset -C arr=(typeset -r -a alphas=(a b c);name=x)'
		chg='arr.name=y; arr.alphas[1]=()'
		res='typeset -p arr'
		exp='typeset -C arr=(typeset -r -a alphas=(a b c);name=y)'
	)
	(
		ini='typeset -C arr=(typeset -r -a alphas=(a b c);name=x)'
		chg='arr.name=y; arr.alphas[1]=(b)'
		res='typeset -p arr'
		exp='typeset -C arr=(typeset -r -a alphas=(a b c);name=y)'
	)
	(
		ini='arr=(alphas=(a b c);name=x); readonly arr.alphas'
		chg='arr.alphas[1]=([b]=5)'
		res='typeset -p arr arr.alphas'
		exp=$'typeset -C arr=(typeset -r -a alphas=(a b c);name=x)\ntypeset -r -a arr.alphas=(a b c)'
	)
	(
		ini='arr=(alphas=(a b c);name=x); readonly arr.alphas'
		chg='arr.alphas[1]=(b)'
		res='typeset -p arr arr.alphas'
		exp=$'typeset -C arr=(typeset -r -a alphas=(a b c);name=x)\ntypeset -r -a arr.alphas=(a b c)'
	)
	(
		ini='typeset -ra -E arr=(0 1 2 3)'
		chg='arr[1]=()'
		res='typeset -p arr'
		exp='typeset -r -a -E arr=(0 1 2 3)'
	)
	(
		ini='typeset -T FB_t=(typeset x=foo y=bar; typeset -r z=${_.y}; create() { _.y=bam; }; )'
		chg='FB_t fb; fb.z=foo'
		res='typeset -p fb'
		exp='FB_t fb=(x=foo;y=bam;typeset -r z=bar)'
	)
	(
		ini='typeset -ra arr=((a b c) 1)'
		chg='arr+=(2)'
		res='typeset -p arr'
		exp='typeset -r -a arr=((a b c) 1)'
	)
	(
		ini='typeset -ra arr=((a b c) 1)'
		chg='arr[0]+=(d)'
		res='typeset -p arr'
		exp='typeset -r -a arr=((a b c) 1)'
	)
	(
		ini='typeset -ra -i arr=((10 11 12) 3 4)'
		chg='(( arr[1] += 2 ))'
		res='typeset -p arr'
		exp='typeset -r -a -i arr=((10 11 12) 3 4)'
	)
	(
		ini='typeset -ra -i arr=((10 11 12) 3 4)'
		chg='(( arr[1] -= 2 ))'
		res='typeset -p arr'
		exp='typeset -r -a -i arr=((10 11 12) 3 4)'
	)
	(
		ini='typeset -ra -i arr=((10 11 12) 3 4)'
		chg='(( arr[1] *= 2 ))'
		res='typeset -p arr'
		exp='typeset -r -a -i arr=((10 11 12) 3 4)'
	)
	(
		ini='typeset -ra -i arr=((10 11 12) 3 4)'
		chg='(( arr[1] /= 2 ))'
		res='typeset -p arr'
		exp='typeset -r -a -i arr=((10 11 12) 3 4)'
	)
	(
		ini='typeset -ra -i arr=((10 11 12) 3 4)'
		chg='(( arr[0][1] += 2 ))'
		res='typeset -p arr'
		exp='typeset -r -a -i arr=((10 11 12) 3 4)'
	)
	(
		ini='typeset -ra -i arr=((10 11 12) 3 4)'
		chg='(( arr[0][1] -= 2 ))'
		res='typeset -p arr'
		exp='typeset -r -a -i arr=((10 11 12) 3 4)'
	)
	(
		ini='typeset -ra -i arr=((10 11 12) 3 4)'
		chg='(( arr[0][1] *= 2 ))'
		res='typeset -p arr'
		exp='typeset -r -a -i arr=((10 11 12) 3 4)'
	)
	(
		ini='typeset -ra -i arr=((10 11 12) 3 4)'
		chg='(( arr[0][1] /= 2 ))'
		res='typeset -p arr'
		exp='typeset -r -a -i arr=((10 11 12) 3 4)'
	)
	(
		ini='typeset -r -A arr=([a]=10 [b]=20 [c]=30)'
		chg='arr[b]=40'
		res='typeset -p arr'
		exp='typeset -r -A arr=([a]=10 [b]=20 [c]=30)'
	)
	(
		ini='typeset -r -A arr=([a]=10 [b]=20 [c]=30)'
		chg='arr[b]=()'
		res='typeset -p arr'
		exp='typeset -r -A arr=([a]=10 [b]=20 [c]=30)'
	)
	(
		ini='typeset -r -A arr=([a]=10 [b]=20 [c]=30)'
		chg='arr[b]=(40)'
		res='typeset -p arr'
		exp='typeset -r -A arr=([a]=10 [b]=20 [c]=30)'
	)
	(
		ini='typeset -r -A arr=([a]=([10]=X [11]=XI [12]=XII) [b]=20)'
		chg='arr[c]=30'
		res='typeset -p arr'
		exp='typeset -r -A arr=([a]=([10]=X [11]=XI [12]=XII) [b]=20)'
	)
	(
		ini='typeset -r -A arr=([a]=([10]=X [11]=XI [12]=XII) [b]=20)'
		chg='arr[c]+=30'
		res='typeset -p arr'
		exp='typeset -r -A arr=([a]=([10]=X [11]=XI [12]=XII) [b]=20)'
	)
	(
		ini='typeset -r -A arr=([a]=([10]=X [11]=XI [12]=XII) [b]=20)'
		chg='arr[a][11]=0xb'
		res='typeset -p arr'
		exp='typeset -r -A arr=([a]=([10]=X [11]=XI [12]=XII) [b]=20)'
	)
	(
		ini='typeset -r -A arr=([a]=([10]=X [11]=XI [12]=XII) [b]=20)'
		chg='arr[a][11]+=0xb'
		res='typeset -p arr'
		exp='typeset -r -A arr=([a]=([10]=X [11]=XI [12]=XII) [b]=20)'
	)
	(
		ini='typeset -r -A arr=([a]=([10]=X [11]=XI [12]=XII) [b]=20)'
		chg='arr[a][13]=XIII'
		res='typeset -p arr'
		exp='typeset -r -A arr=([a]=([10]=X [11]=XI [12]=XII) [b]=20)'
	)
	(
		ini='typeset -r -A -i arr=([a]=10 [b]=20 [c]=30)'
		chg='(( arr[b] += 5 ))'
		res='typeset -p arr'
		exp='typeset -r -A -i arr=([a]=10 [b]=20 [c]=30)'
	)
	(
		ini='typeset -r -A -i arr=([a]=10 [b]=20 [c]=30)'
		chg='(( arr[b] -= 5 ))'
		res='typeset -p arr'
		exp='typeset -r -A -i arr=([a]=10 [b]=20 [c]=30)'
	)
	(
		ini='typeset -r -A -i arr=([a]=10 [b]=20 [c]=30)'
		chg='(( arr[b] *= 5 ))'
		res='typeset -p arr'
		exp='typeset -r -A -i arr=([a]=10 [b]=20 [c]=30)'
	)
	(
		ini='typeset -r -A -i arr=([a]=10 [b]=20 [c]=30)'
		chg='(( arr[b] /= 5 ))'
		res='typeset -p arr'
		exp='typeset -r -A -i arr=([a]=10 [b]=20 [c]=30)'
	)
	(
		ini='typeset -r -A -i arr=([a]=([X]=10 [XI]=11 [XII]=12) [b]=20)'
		chg='(( arr[a][XI] += 5 ))'
		res='typeset -p arr'
		exp='typeset -r -A -i arr=([a]=([X]=10 [XI]=11 [XII]=12) [b]=20)'
	)
	(
		ini='typeset -r -A -i arr=([a]=([X]=10 [XI]=11 [XII]=12) [b]=20)'
		chg='(( arr[a][XI] -= 5 ))'
		res='typeset -p arr'
		exp='typeset -r -A -i arr=([a]=([X]=10 [XI]=11 [XII]=12) [b]=20)'
	)
	(
		ini='typeset -r -A -i arr=([a]=([X]=10 [XI]=11 [XII]=12) [b]=20)'
		chg='(( arr[a][XI] *= 5 ))'
		res='typeset -p arr'
		exp='typeset -r -A -i arr=([a]=([X]=10 [XI]=11 [XII]=12) [b]=20)'
	)
	(
		ini='typeset -r -A -i arr=([a]=([X]=10 [XI]=11 [XII]=12) [b]=20)'
		chg='(( arr[a][XI] /= 5 ))'
		res='typeset -p arr'
		exp='typeset -r -A -i arr=([a]=([X]=10 [XI]=11 [XII]=12) [b]=20)'
	)
)

typeset -i i n
n=${#rtests[@]}
ulimit --cpu 3 2>/dev/null
for ((i=0; i<n; i++))
do
	got=$(
		trap "${rtests[$i].res}" EXIT
		eval "${rtests[$i].ini}"
		eval "${rtests[$i].chg}" 2>&1
	)
	[[ $got == *$': is read only\n'* ]] || err_exit "Readonly variable did not warn for rtests[$i]: "\
		"setup='${rtests[$i].ini}', change='${rtests[$i].chg}'"
	got=${got#*$': is read only\n'}
	[[ $got == *"${rtests[$i].exp}" ]] || err_exit "Readonly variable changed on rtests[$i]: "\
		"expected '${rtests[$i].exp}', got '$got'"
done
unset i n got rtests

# ======
# readonly variables should still accept setting the readonly or export attributes
# https://github.com/ksh93/ksh/issues/258
(readonly v=1; readonly v) 2>/dev/null || err_exit "readonly variable cannot be set readonly (1)"
(readonly v=1; typeset -r v) 2>/dev/null || err_exit "readonly variable cannot be set readonly (2)"
(readonly v=1; export v) 2>/dev/null || err_exit "readonly variable cannot be exported (1)"
(readonly v=1; typeset -x v) 2>/dev/null || err_exit "readonly variable cannot be exported (2)"
(readonly v=1; typeset -rx v) 2>/dev/null || err_exit "readonly variable cannot be set readonly and exported"

# ======
# the environment list was not checked for readonly for commands that are not fork()ed
(readonly v=1; v=2 true) 2>/dev/null && err_exit 'readonly not verified for command environment list'

# ======
exit $((Errors<125?Errors:125))

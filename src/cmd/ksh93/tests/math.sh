########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1982-2011 AT&T Intellectual Property          #
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

set -o nounset

function test_arithmetric_expression_accesss_array_element_through_nameref
{
        compound out=( typeset stdout stderr ; integer res )
	compound -r -a tests=(
		(
			cmd='@@TYPE@@ -a @@VAR@@ ;  @@VAR@@[1]=90 ;       function x { nameref nz=$1 ;              print " $(( round(nz) ))==$(( round($nz) ))" ; } ; x @@VAR@@[1]'		; stdoutpattern=' 90==90'
		)
		(
			cmd='@@TYPE@@ -a @@VAR@@=( [1]=90 ) ;             function x { nameref nz=$1 ;              print " $(( round(nz) ))==$(( round($nz) ))" ; } ; x @@VAR@@[1]'		; stdoutpattern=' 90==90'
		)
		(
			cmd='@@TYPE@@ -a @@VAR@@ ;  @@VAR@@[1][3]=90 ;    function x { nameref nz=$1 ;               print " $(( round(nz) ))==$(( round($nz) ))" ; } ; x @@VAR@@[1][3]'	; stdoutpattern=' 90==90'
		)
		(
			cmd='@@TYPE@@ -a @@VAR@@=( [1][3]=90 ) ;          function x { nameref nz=$1 ;               print " $(( round(nz) ))==$(( round($nz) ))" ; } ; x @@VAR@@[1][3]'	; stdoutpattern=' 90==90'
		)
		(
			cmd='@@TYPE@@ -a @@VAR@@ ;  @@VAR@@[1][3][5]=90 ; function x { nameref nz=$1 ;               print " $(( round(nz) ))==$(( round($nz) ))" ; } ; x @@VAR@@[1][3][5]'	; stdoutpattern=' 90==90'
		)
		(
			cmd='@@TYPE@@ -a @@VAR@@=( [1][3][5]=90 ) ;       function x { nameref nz=$1 ;               print " $(( round(nz) ))==$(( round($nz) ))" ; } ; x @@VAR@@[1][3][5]'	; stdoutpattern=' 90==90'
		)
		(
			cmd='@@TYPE@@ -a @@VAR@@ ;  @@VAR@@[1][3][5]=90 ; function x { nameref nz=${1}[$2][$3][$4] ; print " $(( round(nz) ))==$(( round($nz) ))" ; } ; x @@VAR@@ 1 3 5'	; stdoutpattern=' 90==90'
		)
		(
			cmd='@@TYPE@@ -A @@VAR@@ ;  @@VAR@@[1]=90 ;       function x { nameref nz=$1 ;               print " $(( round(nz) ))==$(( round($nz) ))" ; } ; x @@VAR@@[1]'		; stdoutpattern=' 90==90'
		)
		(
			cmd='@@TYPE@@ -A @@VAR@@=( [1]=90 ) ;             function x { nameref nz=$1 ;               print " $(( round(nz) ))==$(( round($nz) ))" ; } ; x @@VAR@@[1]'		; stdoutpattern=' 90==90'
		)
	)

	typeset testname
	integer i
	typeset mode
	typeset cmd

	for (( i=0 ; i < ${#tests[@]} ; i++ )) ; do
		# fixme: This list should include "typeset -lX" and "typeset -X" but ast-ksh.2010-03-09 fails like this:
		# 'typeset -X -a z ;  z[1][3]=90 ; function x { nameref nz=$1 ; print " $(( nz ))==$(( $nz ))" ; } ; x z[1][3]'
		# + typeset -X -a z
		# + z[1][3]=90
		# + x 'z[1][3]'
		# /home/test001/bin/ksh[1]: x: line 1: x1.68000000000000000000000000000000p: no parent
		for ty in \
			'typeset' \
			'integer' \
			'float' \
			'typeset -i' \
			'typeset -si' \
			'typeset -li' \
			'typeset -E' \
			'typeset -F' \
			'typeset -X' \
			'typeset -lE' \
			'typeset -lX' \
			'typeset -lF' ; do
			for mode in \
				'plain' \
				'in_compound' \
				'in_indexed_compound_array' \
				'in_2d_indexed_compound_array' \
				'in_4d_indexed_compound_array' \
				'in_associative_compound_array' \
				'in_compound_nameref' \
				'in_indexed_compound_array_nameref' \
				'in_2d_indexed_compound_array_nameref' \
				'in_4d_indexed_compound_array_nameref' \
				'in_associative_compound_array_nameref' \
				 ; do
				nameref tst=tests[i]
			
				cmd="${tst.cmd//@@TYPE@@/${ty}}"
				
				case "${mode}" in
					'plain')
						cmd="${cmd//@@VAR@@/z}"
						;;

					'in_compound')
						cmd="compound c ; ${cmd//@@VAR@@/c.z}"
						;;
					'in_indexed_compound_array')
						cmd="compound -a c ; ${cmd//@@VAR@@/c[11].z}"
						;;
					'in_2d_indexed_compound_array')
						cmd="compound -a c ; ${cmd//@@VAR@@/c[17][19].z}"
						;;
					'in_4d_indexed_compound_array')
						cmd="compound -a c ; ${cmd//@@VAR@@/c[17][19][23][27].z}"
						;;
					'in_associative_compound_array')
						cmd="compound -A c ; ${cmd//@@VAR@@/c[info].z}"
						;;

					'in_compound_nameref')
						cmd="compound c ; nameref ncr=c.z ; ${cmd//@@VAR@@/ncr}"
						;;
					'in_indexed_compound_array_nameref')
						cmd="compound -a c ; nameref ncr=c[11].z ; ${cmd//@@VAR@@/ncr}"
						;;
					'in_2d_indexed_compound_array_nameref')
						cmd="compound -a c ; nameref ncr=c[17][19].z ; ${cmd//@@VAR@@/ncr}"
						;;
					'in_4d_indexed_compound_array_nameref')
						cmd="compound -a c ; nameref ncr=c[17][19][23][27].z ; ${cmd//@@VAR@@/ncr}"
						;;
					'in_associative_compound_array_nameref')
						cmd="compound -A c ; nameref ncr=c[info].z ; ${cmd//@@VAR@@/ncr}"
						;;
					*)
						err_exit "Unexpected mode ${mode}"
						;;
				esac
								
				testname="${0}/${cmd}"
#set -x
				out.stderr="${ { out.stdout="${ ${SHELL} -o nounset -o errexit -c "${cmd}" ; (( out.res=$? )) ; }" ; } 2>&1 ; }"
#set +x

			        [[ "${out.stdout}" == ${tst.stdoutpattern}      ]] || err_exit "${testname}: Expected stdout to match $(printf '%q\n' "${tst.stdoutpattern}"), got $(printf '%q\n' "${out.stdout}")"
       				[[ "${out.stderr}" == ''			]] || err_exit "${testname}: Expected empty stderr, got $(printf '%q\n' "${out.stderr}")"
				(( out.res == 0 )) || err_exit "${testname}: Unexpected exit code ${out.res}"
			done
		done
	done
	
	return 0
}

function test_has_iszero
{
	typeset str
	integer i
	
	typeset -r -a tests=(
		'(( iszero(0)   )) && print "OK"'
		'(( iszero(0.)  )) && print "OK"'
		'(( iszero(-0)  )) && print "OK"'
		'(( iszero(-0.) )) && print "OK"'
		'float n=0.  ; (( iszero(n) )) && print "OK"'
		'float n=+0. ; (( iszero(n) )) && print "OK"'
		'float n=-0. ; (( iszero(n) )) && print "OK"'
		'float n=1.  ; (( iszero(n) )) || print "OK"'
		'float n=1.  ; (( iszero(n-1.) )) && print "OK"'
		'float n=-1. ; (( iszero(n+1.) )) && print "OK"'
	)
	
	for (( i=0 ; i < ${#tests[@]} ; i++ )) ; do
		str="$( ${SHELL} -o errexit -c "${tests[i]}" 2>&1 )" || err_exit "test $i: returned non-zero exit code $?"
		[[ "${str}" == 'OK' ]] || err_exit "test $i: expected 'OK', got '${str}'"
	done

	return 0
}

# run tests
test_arithmetric_expression_accesss_array_element_through_nameref
test_has_iszero

# ======
# Validate that typeset -E/F formatting matches that of their equivalent
# printf formatting options as well as checking for correct float scaling
# of the fractional parts.

if	((SHOPT_BRACEPAT))
then	set --  {'','-'}{0..1}.{0,9}{0,9}{0,1,9}{0,1,9}
else	set --	0.0000 0.0001 0.0009 0.0010 0.0011 0.0019 0.0090 0.0091 0.0099 \
		0.0900 0.0901 0.0909 0.0910 0.0911 0.0919 0.0990 0.0991 0.0999 \
		0.9000 0.9001 0.9009 0.9010 0.9011 0.9019 0.9090 0.9091 0.9099 \
		0.9900 0.9901 0.9909 0.9910 0.9911 0.9919 0.9990 0.9991 0.9999 \
		1.0000 1.0001 1.0009 1.0010 1.0011 1.0019 1.0090 1.0091 1.0099 \
		1.0900 1.0901 1.0909 1.0910 1.0911 1.0919 1.0990 1.0991 1.0999 \
		1.9000 1.9001 1.9009 1.9010 1.9011 1.9019 1.9090 1.9091 1.9099 \
		1.9900 1.9901 1.9909 1.9910 1.9911 1.9919 1.9990 1.9991 1.9999 \
		-0.0000 -0.0001 -0.0009 -0.0010 -0.0011 -0.0019 -0.0090 -0.0091 -0.0099 \
		-0.0900 -0.0901 -0.0909 -0.0910 -0.0911 -0.0919 -0.0990 -0.0991 -0.0999 \
		-0.9000 -0.9001 -0.9009 -0.9010 -0.9011 -0.9019 -0.9090 -0.9091 -0.9099 \
		-0.9900 -0.9901 -0.9909 -0.9910 -0.9911 -0.9919 -0.9990 -0.9991 -0.9999 \
		-1.0000 -1.0001 -1.0009 -1.0010 -1.0011 -1.0019 -1.0090 -1.0091 -1.0099 \
		-1.0900 -1.0901 -1.0909 -1.0910 -1.0911 -1.0919 -1.0990 -1.0991 -1.0999 \
		-1.9000 -1.9001 -1.9009 -1.9010 -1.9011 -1.9019 -1.9090 -1.9091 -1.9099 \
		-1.9900 -1.9901 -1.9909 -1.9910 -1.9911 -1.9919 -1.9990 -1.9991 -1.9999
fi
unset i tf pf; typeset -F 3 tf
for i
do	tf=$i
	pf=${ printf '%.3f' tf ;}
	if	[[ $tf != "$pf" ]]
	then	err_exit "typeset -F formatted data does not match its printf. typeset -F 3: $tf != $pf"
	fi
done
unset i tf pf; typeset -lF 3 tf
for i
do	tf=$i
	pf=${ printf '%.3Lf' tf ;}
	if	[[ $tf != "$pf" ]]
	then	err_exit "typeset -lF formatted data does not match its printf. typeset -lF 3: $tf != $pf"
	fi
done
unset i tf pf; typeset -E 3 tf
for i
do	tf=$i
	pf=${ printf '%.3g' tf ;}
	if	[[ $tf != "$pf" ]]
	then	err_exit "typeset -E formatted data does not match its printf. typeset -E 3: $tf != $pf"
	fi
done
unset i tf pf; typeset -lE 3 tf
for i
do	tf=$i
	pf=${ printf '%.3Lg' tf ;}
	if	[[ $tf != "$pf" ]]
	then	err_exit "typeset -lE formatted data does not match its printf. typeset -lE 3: $tf != $pf"
	fi
done
unset i tf pf

unset x
[[ $(typeset -F 0 x=5.67; typeset -p x) == 'typeset -F 0 x=6' ]] || err_exit 'typeset -F 0 with assignment failed to round.'
[[ $(typeset -E 0 x=5.67; typeset -p x) == 'typeset -E 0 x=6' ]] || err_exit 'typeset -E 0 with assignment failed to round.'
[[ $(typeset -X 0 x=5.67; typeset -p x) == 'typeset -X 0 x=0x1p+2' ]] || err_exit 'typeset -X 0 with assignment failed to round.'

[[ $(typeset -lF 0 x=5.67; typeset -p x) == 'typeset -l -F 0 x=6' ]] || err_exit 'typeset -lF 0 with assignment failed to round.'
[[ $(typeset -lE 0 x=5.67; typeset -p x) == 'typeset -l -E 0 x=6' ]] || err_exit 'typeset -lE 0 with assignment failed to round.'
[[ $(typeset -lX 0 x=5.67; typeset -p x) == 'typeset -l -X 0 x=0x1p+2' ]] || err_exit 'typeset -lX 0 with assignment failed to round.'

# ======
# typeset -s used without -i shouldn't set foo to garbage
exp=30000
got="$(typeset -s foo=30000; echo $foo)"
[[ $exp == $got ]] || err_exit "unexpected output from typeset -s without -i" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
exit $((Errors<125?Errors:125))

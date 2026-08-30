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

unset LANG LANGUAGE "${!LC_@}"

IFS=$'\n'; set -o noglob
typeset -a locales=( $(command -p locale -a 2>/dev/null) )
IFS=$' \t\n'

# test Shift JIS \x81\x40 ... \x81\x7E encodings
# (shift char followed by 7 bit ASCII)

typeset -i16 chr
((SHOPT_MULTIBYTE)) && for locale in "${locales[@]}"
do	[[ $locale == *[Jj][Ii][Ss] ]] || continue
	export LC_ALL=$locale
	for ((chr=0x40; chr<=0x7E; chr++))
	do	c=${chr#16#}
		for s in \\x81\\x$c \\x$c
		do	b="$(printf "$s")"
			eval n=\$\'$s\'
			[[ $b == "$n" ]] || err_exit "LC_ALL=$locale printf difference for \"$s\" -- expected '$n', got '$b'"
			u=$(print -- $b)
			q=$(print -- "$b")
			[[ $u == "$q" ]] || err_exit "LC_ALL=$locale quoted print difference for \"$s\" -- $b => '$u' vs \"$b\" => '$q'"
		done

		# https://www.mail-archive.com/ast-developers@lists.research.att.com/msg01848.html
		# In Shift_JIS (unlike in UTF-8), the trailing bytes of a multibyte character may
		# contain a byte without the high-order bit set. If the final byte happened to
		# correspond to an ASCII backslash (\x5C), 'read' would incorrectly treat this as a
		# dangling final backslash (which is invalid) and return a nonzero exit status.
		# Note that the byte sequence '\x95\x5c' represents a multibyte character U+8868,
		# whereas '\x5c' is a backslash when interpreted as a single-byte character.
		[[ $locale == *.[Ss][Jj][Ii][Ss] ]] || continue
		printf "\x95\x$c\n" | read x || err_exit "'read' doesn't skip multibyte input correctly ($LC_ALL, \x95\x$c)"
	done
done
unset LC_ALL chr

# Test the effect of setting a locale, followed by setting a different locale
# then setting the previous locale. The output from 'printf %T' should use
# the current locale.
# https://github.com/ksh93/ksh/issues/261
((SHOPT_MULTIBYTE)) && for locale in "${locales[@]}"
do	[[ $locale == *nl_NL*[Uu][Tt][Ff]* ]] && nl_NL=$locale
	[[ $locale == *ja_JP*[Uu][Tt][Ff]* ]] && ja_JP=$locale
done
if [[ -n $nl_NL ]] && [[ -n $ja_JP ]]; then
	LC_ALL=$nl_NL
	exp=$(printf '%(%A)T' now)
	LC_ALL=$ja_JP
	printf '%T' now > /dev/null
	LC_ALL=$nl_NL
	got=$(printf '%(%A)T' now)
	[[ $exp == $got ]] || err_exit "'printf %T' ignores changes to LC_ALL when locale is set, unset then set (expected $exp, got $got)"
fi
unset LC_ALL

# this locale is supported by AST on all platforms
# EU for { decimal_point="," thousands_sep="." }

if ((SHOPT_MULTIBYTE)); then
locale=C_EU.UTF-8
else
locale=C_EU
fi

export LC_ALL=C

# test multibyte value/trace format -- $'\303\274' is UTF-8 u-umlaut

c=$(LC_ALL=C $SHELL -c "printf $':%2s:\n' $'\303\274'")
u=$(LC_ALL=$locale $SHELL -c "printf $':%2s:\n' $'\303\274'" 2>/dev/null)
if	[[ "$c" != "$u" ]]
then	LC_ALL=$locale
	x=$'+2+ typeset item.text\
+3+ item.text=\303\274\
+4+ print -- \303\274\
\303\274\
+5+ eval $\'arr[0]=(\\n\\ttext=\\303\\274\\n)\'
+2+ arr[0].text=ü\
+6+ print -- \303\274\
ü\
+7+ eval txt=$\'(\\n\\ttext=\\303\\274\\n)\'
+2+ txt.text=\303\274\
+8+ print -- \'(\' text=$\'\\303\\274\' \')\'\
( text=\303\274 )'
	u=$(LC_ALL=$locale PS4='+$LINENO+ ' $SHELL -x -c "
		item=(typeset text)
		item.text=$'\303\274'
		print -- \"\${item.text}\"
		eval \"arr[0]=\$item\"
		print -- \"\${arr[0].text}\"
		eval \"txt=\${arr[0]}\"
		print -- \$txt
	" 2>&1)
	[[ "$u" == "$x" ]] || err_exit LC_ALL=$locale multibyte value/trace format failed

	x=$'00fc\n20ac'
	u=$(LC_ALL=$locale $SHELL -c $'printf "%04x\n" \$\'\"\303\274\"\' \$\'\"\xE2\x82\xAC\"\'')
	[[ $u == $x ]] || err_exit LC_ALL=$locale multibyte %04x printf format failed
fi

if	(( SHOPT_MULTIBYTE && $($SHELL -c $'export LC_ALL='$locale$'; print -r "\342\202\254\342\202\254\342\202\254\342\202\254w\342\202\254\342\202\254\342\202\254\342\202\254" | wc -m' 2>/dev/null) == 10 ))
then	LC_ALL=$locale $SHELL -c b1=$'"\342\202\254\342\202\254\342\202\254\342\202\254w\342\202\254\342\202\254\342\202\254\342\202\254"; [[ ${b1:4:1} == w ]]' || err_exit 'multibyte ${var:offset:len} not working correctly'
fi

#$SHELL -c 'export LANG='$locale'; printf "\u[20ac]\u[20ac]" > $tmp/two_euro_chars.txt'
printf $'\342\202\254\342\202\254' > $tmp/two_euro_chars.txt
if	(builtin wc) 2>/dev/null
then	if((SHOPT_MULTIBYTE)); then
	exp="6 2 6"
	else
	exp="6 6 6"
	fi # SHOPT_MULTIBYTE
	set -- $($SHELL -c "
		if	builtin wc 2>/dev/null || builtin -f cmd wc 2>/dev/null
		then	unset LC_CTYPE
			export LANG=$locale
			export LC_ALL=C
			wc -C < $tmp/two_euro_chars.txt
			unset LC_ALL
			wc -C < $tmp/two_euro_chars.txt
			export LC_ALL=C
			wc -C < $tmp/two_euro_chars.txt
		fi
	")
	got=$*
	[[ $got == $exp ]] || err_exit "builtin wc LC_ALL default failed -- expected '$exp', got '$got'"
fi

# multibyte char straddling buffer boundary

{
	unset i
	integer i
	for ((i = 0; i < 163; i++))
	do	print "#234567890123456789012345678901234567890123456789"
	done
	printf $'%-.*c\n' 15 '#'
	for ((i = 0; i < 2; i++))
	do	print $': "\xe5\xae\x9f\xe8\xa1\x8c\xe6\xa9\x9f\xe8\x83\xbd\xe3\x82\x92\xe8\xa1\xa8\xe7\xa4\xba\xe3\x81\x97\xe3\x81\xbe\xe3\x81\x99\xe3\x80\x82" :'
	done
} > ko.dat

LC_ALL=$locale $SHELL < ko.dat 2> /dev/null || err_exit "script with multibyte char straddling buffer boundary fails"

#	exp	LC_ALL		LC_NUMERIC	LANG
set -- \
	2,5	$locale		C		''		\
	2.5	C		$locale		''		\
	2,5	$locale		''		C		\
	2,5	''		$locale		C		\
	2.5	C		''		$locale		\
	2.5	''		C		$locale		\

unset a b c
unset LC_ALL LC_NUMERIC LANG
integer a b c
while	(( $# >= 4 ))
do	exp=$1
	unset H V
	typeset -A H
	typeset -a V
	[[ $2 ]] && V[0]="export LC_ALL=$2;"
	[[ $3 ]] && V[1]="export LC_NUMERIC=$3;"
	[[ $4 ]] && V[2]="export LANG=$4;"
	for ((a = 0; a < 3; a++))
	do	for ((b = 0; b < 3; b++))
		do	if	(( b != a ))
			then	for ((c = 0; c < 3; c++))
				do	if	(( c != a && c != b ))
					then	T=${V[$a]}${V[$b]}${V[$c]}
						if	[[ ! ${H[$T]} ]]
						then	H[$T]=1
							got=$($SHELL -c "${T}print \$(( $exp ))" 2>&1)
							[[ $got == $exp ]] || err_exit "${T} sequence failed -- expected '$exp', got '$got'"
						fi
					fi
				done
			fi
		done
	done
	shift 4
done

# setlocale(LC_ALL,"") after setlocale() initialization

if builtin cut 2> /dev/null; then
	printf 'f1\357\274\240f2\n' > input1
	printf 't2\357\274\240f1\n' > input2
	printf '\357\274\240\n' > delim
	print "export LC_ALL=$locale
	builtin cut
	cut -f 1 -d \$(cat delim) input1 input2 > out" > script
	$SHELL -c 'unset LANG ${!LC_*}; $SHELL ./script' || err_exit "'cut' builtin failed -- exit code $?"
	exp=$'f1\nt2'
	got="$(<out)"
	[[ $got == "$exp" ]] || err_exit "LC_ALL test script failed" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
fi

# multibyte identifiers

if((SHOPT_MULTIBYTE)); then
exp=OK
export LC_CTYPE=C.UTF-8
got=$("$SHELL" -c $'\u[5929]=OK; print ${\u[5929]}' 2>&1)
[[ $got == "$exp" ]] || err_exit "multibyte variable definition/expansion failed -- expected '$exp', got '$got'"
got=$("$SHELL" -c $'function \u[5929]\n{\nprint OK;\n}; \u[5929]' 2>&1)
[[ $got == "$exp" ]] || err_exit "multibyte ksh function definition/execution failed -- expected '$exp', got '$got'"
got=$("$SHELL" -c $'\u[5929]()\n{\nprint OK;\n}; \u[5929]' 2>&1)
[[ $got == "$exp" ]] || err_exit "multibyte POSIX function definition/execution failed -- expected '$exp', got '$got'"
unset LC_CTYPE
fi # SHOPT_MULTIBYTE

unset LANG

LC_ALL=C
x=$"hello"
[[ $x == hello ]] || err_exit 'assignment of message strings not working'

# this test backported from 93v- and fixed
if	((SHOPT_MULTIBYTE))
then	LC_ALL=C.UTF-8
	got=$(printf '\u[20ac]')
	exp=$'\342\202\254'
	[[ $got == "$exp" ]] || err_exit 'locales not handled correctly in command substitution' \
		"(expected $(printf %q "$exp"); got $(printf %q "$got"))"		
	unset LC_ALL
fi

# ======
# The locale should be restored along with locale variables when leaving a virtual subshell.
# https://github.com/ksh93/ksh/issues/253#issuecomment-815290154
if	((SHOPT_MULTIBYTE))
then
	unset LANG "${!LC_@}"
	LANG=C.UTF-8
	exp=$'5\n10\n5'
	got=$(eval 'var=äëïöü'; echo ${#var}; (LANG=C; echo ${#var}); echo ${#var})
	[[ $got == "$exp" ]] || err_exit "locale is not restored properly upon leaving virtual subshell" \
		"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
fi

# ======
unset LANG "${!LC_@}"
(LANG=C_EU && LC_NUMERIC=C && let .5) || err_exit "radix point not updated by LC_NUMERIC"
(LANG=C && LC_NUMERIC=C_EU && let ,5) || err_exit "radix point not updated by LC_NUMERIC"
(LC_NUMERIC=C_EU && LC_ALL=C && let .5) || err_exit "radix point not updated by LC_ALL"
(LC_NUMERIC=C  && LC_ALL=C_EU && let ,5) || err_exit "radix point not updated by LC_ALL"
(LC_ALL=C_EU && unset LC_ALL && LANG=C && let .5) || err_exit "radix point not updated by LANG"
(LC_ALL=C && unset LC_ALL && LANG=C_EU && let ,5) || err_exit "radix point not updated by LANG"

# ======
# Segmentation fault or quiet failure when using $"..."
# https://github.com/ksh93/ksh/issues/599
# Test based on reproducer by Koichi Nakashima (@ko1nksm), license: https://creativecommons.org/publicdomain/zero/1.0/
if	((SHOPT_MULTIBYTE))
then	unset LANG "${!LC_@}" i
	for locale in "${locales[@]}"
	do	[[ $locale =~ ^ja_JP.*[Uu][Tt][Ff]-?8$ ]] && export LANG=$locale && break
	done
	if	! [[ -v LANG ]] || ! whence -qp gencat
	then	warning "ja_JP locale or gencat not available - skipping issue 599 test"
	else	orig='Hello World!'
		exp='こんにちは世界！'
		catalogname=ksh93-i18n
		mkdir -p locale/C locale/$LANG
		printf '$quote "\n$set 3 This is the C locale message set\n1 "%s"\n' "$orig" > locale/C/$catalogname.msg
		printf '$quote "\n$set 3 This is the ja locale message set\n1 "%s"\n' "$exp" > locale/$LANG/$catalogname.msg
		for i in C "$LANG"
		do	gencat "locale/$i/$catalogname.cat" "locale/$i/$catalogname.msg"
		done
		# be crash-proof: fork the subshell with ulimit
		got=$(set +x; ulimit -t unlimited 2>/dev/null; {
			# %L matches full locale name, e.g. ja_JP.UTF-8 (whereas lowercase %l matches language name, e.g. ja)
			# (Also good to know: %N matches $(basename $0) -- note $0 changes when calling a ksh function)
			export NLSPATH=locale/%L/$catalogname.cat
			eval "echo \$\"$orig\""
		} 2>&1)
		[[ e=$? -eq 0 && $got == "$exp" ]] || err_exit '$"..." fails' \
			"(expected status 0, '$exp';" \
			"got status $e$( ((e>128)) && print -n /SIG && kill -l "$e"), $(printf %q "$got"))"
	fi
fi

# ======
# double-width characters should count for two for default justification width
# https://github.com/ksh93/ksh/issues/189
if	((SHOPT_MULTIBYTE))
then	unset s "${!LC_@}"
	LANG=C.UTF-8
	s='コーンシェル'
	typeset -L s
	got=$(typeset -p s; echo ${#s})
	exp=$'typeset -L 12 s=コーンシェル\n6'  # each double-width character counts for two terminal positions
	[[ $got == "$exp" ]] || err_exit "default terminal width for typeset -L incorrect" \
		"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
	unset s
fi

# ======
# https://github.com/ksh93/ksh/issues/813
if	((SHOPT_MULTIBYTE))
then	LANG=C.UTF-8
	s='あいう'
	got=${s//""/-}
	exp=$s
	[[ $got == "$exp" ]] || err_exit '${parameter//“”/string}' \
		"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
	got=${s//~(E)^/-}
	exp='-あ-い-う-'
	[[ $got == "$exp" ]] || err_exit '${parameter//~(E)^/string}' \
		"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
fi

# ======
# https://github.com/ksh93/ksh/issues/861#issuecomment-2917738184
if	((SHOPT_MULTIBYTE))
then	export LANG=C.UTF-8
	got=$(set +x; redirect 2>&1; readonly コーンシェル=OK; echo "$コーンシェル")
	exp=OK
	[[ $got == "$exp" ]] || err_exit 'declaration command assignment with multibyte variable name' \
		"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
	cat >linecontinuation.sh <<-'EOF'
		export a\
		bc\
		def=OK1
		export コ\
		ーン\
		シェル=OK2
		echo $abcdef
		echo $コーンシェル
	EOF
	got=$("$SHELL" linecontinuation.sh 2>&1)
	exp=$'OK1\nOK2'
	[[ $got == "$exp" ]] || err_exit 'declaration command assignment with multibyte variable name and line continuation' \
		"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
	unset LANG
fi

# ======
if	((SHOPT_MULTIBYTE))
then	export LANG=C.UTF-8
	# initial A makes it sort before LANG, LC_* -- so this checks locale is init'ed before importing environment
	got=$(export Aコーンシェル=OK; "$SHELL" -c 'echo "${Aコーンシェル}"' 2>&1)
	exp=OK
	[[ $got == "$exp" ]] || err_exit 'importing environment variable with multibyte variable name' \
		"(expected $(printf %q "$exp"); got $(printf %q "$got"))"

	# test that this works in multiple levels of execution of scripts without hashbang (#!) path
	echo >foo1 $'echo "foo1: A日本語の変数名==${A日本語の変数名}"\n./foo2\necho end1'
	echo >foo2 $'echo "foo2: A日本語の変数名==${A日本語の変数名}"\n./foo3\necho end2'
	echo >foo3 $'echo "foo3: A日本語の変数名==${A日本語の変数名}"\n./foo4\necho end3'
	echo >foo4 $'echo "foo4: A日本語の変数名==${A日本語の変数名}"\necho end4'
	chmod +x foo1 foo2 foo3 foo4
	exp=$'foo1: A日本語の変数名==OK\nfoo2: A日本語の変数名==OK\nfoo3: A日本語の変数名==OK\nfoo4: A日本語の変数名==OK\nend4\nend3\nend2\nend1'
	got=$(A日本語の変数名=OK ./foo1)
	[[ $got == "$exp" ]] || err_exit 'importing environment variable with multibyte variable name in script without #! path' \
		"(expected $(printf %q "$exp"); got $(printf %q "$got"))"
fi

# ======
# A strxfrm(3) bug on some macOS versions caused false positive matches in bracket patterns.
# This test is to ensure the workaround is functional.
# https://github.com/ksh93/ksh/issues/936
if	(LC_ALL=en_GB.UTF-8; eval 'c=$'\''\342\202\254'\'; [[ ${#c} == 1 ]]) 2>/dev/null
then	LC_ALL=en_GB.UTF-8 "$SHELL" -c "LINENO=$((LINENO+1))"'
	. "${SHTESTS_COMMON:-${0%/*}/_common}"
	for c in k $'\''\uE9'\'' $'\''\uFC'\'' $'\''\uA7'\'' $'\''\u39a'\'' \
		$'\''\u1D542'\'' $'\''\u306E'\'' $'\''\u2FA6'\'' '\''$\u4EBA'\''
	do	case $c in 
		[\|-])    err_exit "'\''$c'\'' wrongly matches [\\|-]" ;;
		esac
		case $c in 
		[x\|x-])  err_exit "'\''$c'\'' wrongly matches [x\\|x-]" ;;
		esac
	done
	exit $((Errors<125?Errors:125))
	' "$0"
	((Errors += $?))
	unset LC_ALL
fi

# ======
# macOS's iconv(3) may return multiple-character string approximations for Unicode code points in UTF-8
# locales, but those cannot be used in \u[...] expansions. Also, Solaris iconv fails to fail altogether,
# and returns '?' for unsupported code points. Make sure chrexp() performs correctly in these scenarios.
case " ${locales[*]} " in
*" en_GB.ISO8859-1 "*)
	LANG=en_GB.ISO8859-1
	for exp in '\u[2026]' '\u133' '\u2116' '\u210a' '\u2122' '\u2103' '\u2109'
	do	eval "got=\$'$exp'"
		[[ $got == "$exp" ]] || err_exit "unspported code point $exp fails to fail in iso-8858-1 (got '$got')"
	done
	unset LANG
esac

# ======
# Crash or freeze when combining an unterminated here-document with a pattern seeking redirection
for locale in C C.UTF-8
do	for reproducer in \
	$'cat <<EOF <#*termin*\nUnterminated heredoc' \
	$'cat <<EOF <#NO_MATCH\nUnterminated heredoc' \
	$'cat <<EOF\n\x[D3]'  # invalid UTF-8 at end
	do	LC_CTYPE=$locale "$SHELL" -c "$reproducer" > out 2>&1 &
		pid=$!
		(sleep .5; kill -9 $pid) &
		wait $pid 2> out2
		e=$?
		kill $! 2>/dev/null
		((e==0)) || err_exit "$(printf %q "$reproducer") with LC_CTYPE=$locale: expected status 0," \
			"got status $e$( ((e>128)) && print /SIG$(kill -l $e) ) with output $(printf %q "$(<out)$(<out2)")"
	done
done

# ======
exit $((Errors<125?Errors:125))

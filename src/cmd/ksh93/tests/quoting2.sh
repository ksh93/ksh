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

set -o noglob
if	[[ 'hi there' != "hi there" ]]
then	err_exit "single quotes not the same as double quotes"
fi
x='hi there'
if	[[ $x != 'hi there' ]]
then	err_exit "$x not the same as 'hi there'"
fi
if	[[ $x != "hi there" ]]
then	err_exit "$x not the same as \"hi there \""
fi
if	[[ \a\b\c\*\|\"\ \\ != 'abc*|" \' ]]
then	err_exit " \\ differs from '' "
fi
if	[[ "ab\'\"\$(" != 'ab\'\''"$(' ]]
then	err_exit " \"\" differs from '' "
fi
if	[[ $(print -r - 'abc*|" \') !=  'abc*|" \' ]]
then	err_exit "\$(print -r - '') differs from ''"
fi
if	[[ $(print -r - "abc*|\" \\") !=  'abc*|" \' ]]
then	err_exit "\$(print -r - '') differs from ''"
fi
if	[[ "$(print -r - 'abc*|" \')" !=  'abc*|" \' ]]
then	err_exit "\"\$(print -r - '')\" differs from ''"
fi
if	[[ "$(print -r - "abc*|\" \\")" !=  'abc*|" \' ]]
then	err_exit "\"\$(print -r - "")\" differs from ''"
fi
if	[[ $(print -r - "$(print -r - 'abc*|" \')") !=  'abc*|" \' ]]
then	err_exit "nested \$(print -r - '') differs from ''"
fi
if	[[ "$(print -r - $(print -r - 'abc*|" \'))" !=  'abc*|" \' ]]
then	err_exit "\"nested \$(print -r - '')\" differs from ''"
fi
if	[[ $(print -r - "$(print -r - 'abc*|" \')") !=  'abc*|" \' ]]
then	err_exit "nested \"\$(print -r - '')\" differs from ''"
fi
unset x
if	[[ ${x-$(print -r - "abc*|\" \\")} !=  'abc*|" \' ]]
then	err_exit "\${x-\$(print -r - '')} differs from ''"
fi
if	[[ ${x-$(print -r - "a}c*|\" \\")} !=  'a}c*|" \' ]]
then	err_exit "\${x-\$(print -r - '}')} differs from ''"
fi
x=$((echo foo)|(cat))
if	[[ $x != foo  ]]
then	err_exit "((cmd)|(cmd)) failed"
fi
x=$(print -r -- "\"$HOME\"")
if	[[ $x != '"'$HOME'"' ]]
then	err_exit "nested double quotes failed"
fi
unset z
: ${z="a{b}c"}
if	[[ $z != 'a{b}c' ]]
then	err_exit '${z="a{b}c"} not correct'
fi
unset z
: "${z="a{b}c"}"
if	[[ $z != 'a{b}c' ]]
then	err_exit '"${z="a{b}c"}" not correct'
fi
if	[[ $(print -r -- "a\*b") !=  'a\*b' ]]
then	err_exit '$(print -r -- "a\*b") differs from a\*b'
fi
unset x
if	[[ $(print -r -- "a\*b$x") !=  'a\*b' ]]
then	err_exit '$(print -r -- "a\*b$x") differs from a\*b'
fi
x=hello
set -- ${x+foo bar bam}
if	(( $# !=3 ))
then	err_exit '${x+foo bar bam} does not yield three arguments'
fi
set -- ${x+foo "bar bam"}
if	(( $# !=2 ))
then	err_exit '${x+foo "bar bam"} does not yield two arguments'
fi
set -- ${x+foo 'bar bam'}
if	(( $# !=2 ))
then	err_exit '${x+foo '\''bar bam'\''} does not yield two arguments'
fi
set -- ${x+foo $x bam}
if	(( $# !=3 ))
then	err_exit '${x+foo $x bam} does not yield three arguments'
fi
set -- ${x+foo "$x" bam}
if	(( $# !=3 ))
then	err_exit '${x+foo "$x" bam} does not yield three arguments'
fi
set -- ${x+"foo $x bam"}
if	(( $# !=1 ))
then	err_exit '${x+"foo $x bam"} does not yield one argument'
fi
set -- "${x+foo $x bam}"
if	(( $# !=1 ))
then	err_exit '"${x+foo $x bam}" does not yield one argument'
fi
set -- ${x+foo "$x "bam}
if	(( $# !=2 ))
then	err_exit '${x+foo "$x "bam} does not yield two arguments'
fi
x="ab$'cd"
if	[[ $x != 'ab$'"'cd" ]]
then	err_exit '$'"' inside double quotes not working"
fi
x=`print 'ab$'`
if	[[ $x != 'ab$' ]]
then	err_exit '$'"' inside `` quotes not working"
fi
unset a
x=$(print -r -- "'\
\
")
if	[[ $x != "'" ]]
then	err_exit 'line continuation in double strings not working'
fi
x=$(print -r -- "'\
$a\
")
if	[[ $x != "'" ]]
then	err_exit 'line continuation in expanded double strings not working'
fi
x='\*'
if	[[ $(print -r -- $x) != '\*' ]]
then	err_exit 'x="\\*";$x != \*'
fi
if	[[ $(print -r -- "\}" ) != '\}' ]]
then	err_exit '(print -r -- "\}"' not working
fi
if	[[ $(print -r -- "\{" ) != '\{' ]]
then	err_exit 'print -r -- "\{"' not working
fi
# The following caused a syntax error on earlier versions
foo=foo x=-
if	[[  `eval print \\${foo$x}` != foo* ]]
then	err_exit '`eval print \\${foo$x}`' not working
fi
if	[[  "`eval print \\${foo$x}`" != foo* ]]
then	err_exit '"`eval print \\${foo$x}`"' not working
fi
if	( [[ $() != '' ]] )
then	err_exit '$() not working'
fi
x=a:b:c
set -- $( IFS=:; print $x)
if	(( $# != 3))
then	err_exit 'IFS not working correctly with command substitution'
fi
$SHELL -n 2> /dev/null << \! || err_exit '$(...) bug with ( in comment'
y=$(
	# ( this line is a bug fix
	print hi
)
!
x=
for j in  glob noglob
do	for i in 'a\*b' 'a\ b' 'a\bc' 'a\*b' 'a\"b'
	do	eval [[ '$('print -r -- \'$i\'\$x')' != "'$i'" ]]  && err_exit "quoting of $i\$x with $j enabled failed"
		eval [[ '$('print -r -- \'$i\'\${x%*}')' != "'$i'" ]]  && err_exit "quoting of $i\${x%*} with $j enabled failed"
		if	[[ $j == noglob ]]
		then	eval [[ '$('print -r -- \'$i\'\${x:-*}')' != "'$i''*'" ]]  && err_exit "quoting of $i\${x:-*} with $j enabled failed"
		fi
	done
	set -f
done
foo=foo
[[ "$" == '$' ]] || err_exit '"$" != $'
[[ "${foo}$" == 'foo$' ]] || err_exit 'foo=foo;"${foo}$" != foo$'
[[ "${foo}${foo}$" == 'foofoo$' ]] || err_exit 'foo=foo;"${foo}${foo}$" != foofoo$'
foo='$ '
[[ "$foo" == ~(Elr)(\\\$|#)\  ]] || err_exit $'\'$ \' not matching RE \\\\\\$|#\''
[[ "$foo" == ~(Elr)('\$'|#)\  ]] || err_exit $'\'$ \' not matching RE \'\\$\'|#\''
foo='# '
[[ "$foo" == ~(Elr)(\\\$|#)\  ]] || err_exit $'\'# \' not matching RE \\'\$|#\''
[[ "$foo" == ~(Elr)('\$'|#)\  ]] || err_exit $'\'# \' not matching RE \'\\$\'|#\''
[[ '\$' == '\$'* ]] ||   err_exit $'\'\\$\' not matching \'\\$\'*'
[[ a+a == ~(E)a\+a ]] || err_exit '~(E)a\+a not matching a+a'
[[ a+a =~ a\+a ]] || err_exit 'RE a\+a not matching a+a'

exp='ac'
got=$'a\0b'c
[[ $got == "$exp" ]] || err_exit "\$'a\\0b'c expansion failed -- expected '$exp', got '$got'"

# ======
# generating shell-quoted strings using printf %q (same algorithm used for xtrace and output of 'set', 'trap', ...)

[[ $(printf '%q\n') == '' ]] || err_exit 'printf "%q" with missing arguments yields non-empty result'

# the following fails on 2012-08-01 in UTF-8 locales
expect="'shell-quoted string'"
actual=$(
	print -nr $'\303\274' | read -n1 foo  # interrupt processing of 2-byte UTF-8 char after reading 1 byte
	printf '%q\n' "shell-quoted string"
)
LC_CTYPE=POSIX true	    # on buggy ksh, a locale re-init via temp assignment restores correct shellquoting
[[ $actual == "$expect" ]] || err_exit 'shell-quoting corrupted after interrupted processing of UTF-8 char' \
				"(expected $expect; got $actual)"

# shell-quoting UTF-8 characters: check for unnecessary encoding
case ${LC_ALL:-${LC_CTYPE:-${LANG:-}}} in
( *[Uu][Tt][Ff]8* | *[Uu][Tt][Ff]-8* )
	# must wrap literal UTF-8 characters in 'eval' to avoid syntax error in ja_JP.SJIS
	eval 'expect=$'\''$\'\''عندما يريد العالم أن \\u[202a]يتكلّم \\u[202c] ، فهو يتحدّث بلغة يونيكود.\'\'''\'
	eval 'actual=$(printf %q '\''عندما يريد العالم أن ‪يتكلّم ‬ ، فهو يتحدّث بلغة يونيكود.'\'')'
	[[ $actual == "$expect" ]] || err_exit 'shell-quoting: Arabic UTF-8 characters' \
				"(expected $expect; got $actual)"
	eval 'expect="'\''正常終了 正常終了'\''"'
	eval 'actual=$(printf %q '\''正常終了 正常終了'\'')'
	[[ $actual == "$expect" ]] || err_exit 'shell-quoting: Japanese UTF-8 characters' \
				"(expected $expect; got $actual)"
	eval 'expect="'\''aeu aéu'\''"'
	eval 'actual=$(printf %q '\''aeu aéu'\'')'
	[[ $actual == "$expect" ]] || err_exit 'shell-quoting: Latin UTF-8 characters' \
				"(expected $expect; got $actual)"
	expect=$'$\'\\x86\\u[86]\\xf0\\x96v\\xa7\\xb5\''
	actual=$(printf %q $'\x86\u86\xF0\x96\x76\xA7\xB5')
	[[ $actual == "$expect" ]] || err_exit 'shell-quoting: invalid UTF-8 characters not encoded with \xNN' \
				"(expected $expect; got $actual)"
	;;
esac

# check that hex bytes are protected with square braces if needed
expect=$'$\'1\\x[11]1\''
actual=$(printf %q $'1\x[11]1')
[[ $actual == "$expect" ]] || err_exit 'shell-quoting: hex bytes not protected from subsequent hex-like chars' \
				"(expected $expect; got $actual)"

# ======
# https://github.com/ksh93/ksh/issues/290
var=dummy
exp='{}'
got=$(set +x; eval 'echo ${var:+'\''{}'\''}' 2>&1)
[[ $got == "$exp" ]] || err_exit "Single quotes misparsed in expansion operator string (1)" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
unset var
exp='}'
got=$(set +x; eval 'echo ${var:-'\''}'\''}' 2>&1)
[[ $got == "$exp" ]] || err_exit "Single quotes misparsed in expansion operator string (2)" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
exp='x'
got=$(var=x; set +x; eval 'echo ${var:-'\''{}'\''}' 2>&1)
[[ $got == "$exp" ]] || err_exit "Single quotes misparsed in expansion operator string (3)" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
exp=''
got=$(var=; set +x; eval 'echo ${var:+'\''{}'\''}' 2>&1)
[[ $got == "$exp" ]] || err_exit "Single quotes misparsed in expansion operator string (4)" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
exp='{}'
got=$(unset var; set +x; eval 'echo ${var-'\''{}'\''}' 2>&1)
[[ $got == "$exp" ]] || err_exit "Single quotes misparsed in expansion operator string (5)" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
exp=''
got=$(unset var; set +x; eval 'echo ${var+'\''{}'\''}' 2>&1)
[[ $got == "$exp" ]] || err_exit "Single quotes misparsed in expansion operator string (6)" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
exit $((Errors<125?Errors:125))

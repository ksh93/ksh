########################################################################
#                                                                      #
#              This file is part of the ksh 93u+m package              #
#          Copyright (c) 2022-2025 Contributors to ksh 93u+m           #
#                      and is licensed under the                       #
#                 Eclipse Public License, Version 2.0                  #
#                                                                      #
#                A copy of the License is available at                 #
#      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      #
#         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         #
#                                                                      #
#                      Phi <phi.debian@gmail.com>                      #
#                  Martijn Dekker <martijn@inlv.org>                   #
#               K. Eugene Carlson <kvngncrlsn@gmail.com>               #
#                                                                      #
########################################################################

. "${SHTESTS_COMMON:-${0%/*}/_common}"

if	let '.sh.version < 20211118'
then	err_exit 'ksh too old for "printf -v"; tests would fail massively'
	exit 1
fi

alias T='do_test "$LINENO"'

# ======
# Tests for mixing and matching % and %x$
# https://github.com/ksh93/ksh/issues/324

function do_test
{	printf -v got "$2" 1 2 3 4 5 6 7 8 9 0
	[[ $got == "$3" ]] || \err_exit "$1" "printf '$2': expected $(printf %q "$3"), got $(printf %q "$got")"
}

# This part was generated once, and can now be augmented with more test

x='1 2
3 4
5 6
7 8
9 0
'
f='%s %s\n'
T "$f" "$x"

x='1 1
2 2
3 3
4 4
5 5
6 6
7 7
8 8
9 9
0 0
'
f='%s %1$s\n'
T "$f" "$x"

x='1 1
2 2
3 3
4 4
5 5
6 6
7 7
8 8
9 9
0 0
'
f='%1$s %s\n'
T "$f" "$x"

x='1 1
2 2
3 3
4 4
5 5
6 6
7 7
8 8
9 9
0 0
'
f='%1$s %1$s\n'
T "$f" "$x"

x='1 1 2 2
3 3 4 4
5 5 6 6
7 7 8 8
9 9 0 0
'
f='%1$s %s %s %2$s\n'
T "$f" "$x"

x='1 1 2 3
4 4 5 6
7 7 8 9
0 0  
'
f='%s %1$s %s %s\n'
T "$f" "$x"

x='1 1 2 2
3 3 4 4
5 5 6 6
7 7 8 8
9 9 0 0
'
f='%s %1$s %2$s %s\n'
T "$f" "$x"

x='2 1
4 3
6 5
8 7
0 9
'
f='%2$s %s\n'
T "$f" "$x"

x='2 1 2 1
4 3 4 3
6 5 6 5
8 7 8 7
0 9 0 9
'
f='%2$s %1$s %2$s %1$s\n'
T "$f" "$x"

x='2 1 1 2 1 2
4 3 3 4 3 4
6 5 5 6 5 6
8 7 7 8 7 8
0 9 9 0 9 0
'
f='%2$s %1$s %s %2$s %1$s %s\n'
T "$f" "$x"

x=' 1 2
   3 4
     5 6
       7 8
9 0
'
f='%*2$s %s\n'
T "$f" "$x"

x=' 1 2
    4 5
       7 8
 
'
f='%*2$.*3$s %s\n'
T "$f" "$x"

x='  1 2
     4 5
        7 8
 
'
f='%*3$.*2$s %s\n'
T "$f" "$x"

x='2   5
     7        0
'
f='%*s %*.*s\n'
T "$f" "$x"

x='2 0005
     7 000000000
'
f='%*d %*.*d\n'
T "$f" "$x"

x='2.000000 5.0000
7.000000 0.000000000
'
f='%*f %*.*f\n'
T "$f" "$x"

x=' 3 2 1
    6 5 4
       9 8 7
  0
'
f='%3$*2$.*1$s %2$s %1$s\n'
T "$f" "$x"

x='1 5
6 0
'
f='%s %5$s\n'
T "$f" "$x"

unset x f

# ======
# Tests for printf %T with relative date spec and 'exact' keyword
# https://github.com/ksh93/ksh/issues/182

# Check printf against a string
function do_test # 1:LINENO 2:printf-STRING 3:match-string
{	printf -v got "%($format)T" "$2"
	[[ $got == "$3" ]] || \err_exit "$1" "$C: printf '%($format)T' '$2':" \
		"expected $(printf %q "$3"), got $(printf %q "$got")"
}

# The first tests require a time zone with one or more historical changes.
format='%Y-%m-%d %H:%M:%S'
export TZ=Europe/Riga

C='Historical changes (bad time)' # https://github.com/ksh93/ksh/issues/669
T '#236961303'				'1977-07-05 17:35:03'

export TZ=Europe/London
T '#0'					'1970-01-01 01:00:00'

format='%Y-%m-%d'
export TZ=UTC

C='Calendar dates'
T '2020-01-14'				'2020-01-14'
T '2020-01-14 -14 days'			'2019-12-31'
T '2020-01-14 -14 days ago'		'2020-01-28'
T '2020-01-14 -1 days'			'2020-01-13'
T '2020-01-14 -1 days ago'		'2020-01-15'
T '2020-01-14 -0 days'			'2020-01-14'
T '2020-01-14 -0 days ago'		'2020-01-14'
T '2020-01-14 +0 days'			'2020-01-14'
T '2020-01-14 +0 days ago'		'2020-01-14'
T '2020-01-14 +1 days'			'2020-01-15'
T '2020-01-14 +1 days ago'		'2020-01-13'
T '2020-01-14 +14 days'			'2020-01-28'
T '2020-01-14 +14 days ago'		'2019-12-31'
T '2020-01-14 0 days'			'2020-01-14'
T '2020-01-14 0 days ago'		'2020-01-14'
T '2020-01-14 1 days'			'2020-01-15'
T '2020-01-14 1 days ago'		'2020-01-13'
T '2020-01-14 14 days'			'2020-01-28'
T '2020-01-14 14 days ago'		'2019-12-31'

format='%Y-%m-%d %H:%M:%S'

C='Backward rollover'
T '#1684340457'				'2023-05-17 16:20:57'
T '#1684340457 exact 2400 min  ago'	'2023-05-16 00:20:57'
T '#1684340457 exact   40 hour ago'	'2023-05-16 00:20:57'

C='Ambigous sec'
T '#1579042800            +120 seconds'	'2020-01-14 23:02:00'
T '#1579042800        fri +120 seconds'	'2020-01-17 00:02:00'
T '#1579042800 next   fri +120 seconds'	'2020-01-24 00:02:00'
T '#1579042800 second fri +120 seconds'	'2020-01-10 00:02:00'
T '#1579042800 second'			'2020-01-14 23:00:01'
T '#1579042800 second second'		'2020-01-14 23:00:02'
T '#1579042800 second second second'	'2020-01-14 23:00:03'
T '#1579042800 second minute second'	'2020-01-14 23:01:01'
T '#1579042800 second minute +2 second'	'2020-01-14 23:01:02'

# ksh doesn't handle week dates with day number: 2020-W20-D20
C='Week dates'
T '2020W20'				'2020-05-11 00:00:00'
T '2020W20 exact'			'2020-05-11 00:00:00'
T '2020-W20'				'2020-05-11 00:00:00'
T '2020-W20 exact'			'2020-05-11 00:00:00'
T '2020W20  fri'			'2020-05-15 00:00:00'
T '2020-W20 fri'			'2020-05-15 00:00:00'
T '2020W20  next fri'			'2020-05-22 00:00:00'
T '2020-W20 next fri'			'2020-05-22 00:00:00'
T '2020W20  second fri'			'2020-05-08 00:00:00'
T '2020-W20 second fri'			'2020-05-08 00:00:00'
T '2020W20  third fri'			'2020-05-15 00:00:00'
T '2020-W20 third fri'			'2020-05-15 00:00:00'

C='Ordinal time'  # ksh only supports the extended form
T '2020-2-3T12:34:56'			'2020-02-03 12:34:56'
T '2020-2-3 12:34:56'			'2020-02-03 12:34:56'
T '2020-2-3 12:34:56 next fri'		'2020-02-14 12:34:56'
# Ordinal time with time zone (we run with TZ=UTC)
# (ksh doesn't handle UTC offset UTC+02:30)
T '2020-2-3T12:34:56Z'			'2020-02-03 12:34:56'
T '2020-2-3 12:34:56Z'			'2020-02-03 12:34:56'

C='Ordinal access'
format='%F'
T '1th tuesday in 2016-3'		'2016-03-01'
T '2th tuesday in 2016-3'		'2016-03-08'
T '3th tuesday in 2016-3'		'2016-03-15'
T '4th tuesday in 2016-3'		'2016-03-22'
T '5th tuesday in 2016-3'		'2016-03-29'
T 'first tuesday in march 2016'		'2016-03-01'
T 'second tuesday in march 2016'	'2016-03-08'
T 'third tuesday in march 2016'		'2016-03-15'
T 'fourth tuesday in march 2016'	'2016-03-22'
T 'fifth tuesday in march 2016'		'2016-03-29'
T '1st tuesday in march 2016'		'2016-03-01'
T '2nd tuesday in march 2016'		'2016-03-08'
T '3rd tuesday in march 2016'		'2016-03-15'
T '4th tuesday in march 2016'		'2016-03-22'
T '5th tuesday in march 2016'		'2016-03-29'

# The following tests for times relative to the current time require GNU 'date' to compare our results to.
if	! gd=$(	set -o noglob
		IFS=:
		search=$PATH:$userPATH:   # userPATH is the user's original path (saved in tests/shtests)
		for p in $search
		do	[[ -z $p ]] && p=.
			for c in gnudate gdate date
			do	if	[[ -x $p/$c && $(LC_ALL=C "$p/$c" --version 2>/dev/null) == 'date (GNU coreutils)'* ]]
				then	print -r -- "$p/$c"
					exit 0
				fi
			done
		done
		exit 1
	)
then
	warning "GNU 'date' not available -- printf %T 'ago' tests skipped"
else
	# Check printf %T with 'exact' keyword against GNU 'date'
	function do_test # 1:LINENO 2:date-STRING [3:printf-STRING]
	{	typeset -si i
		for ((i=0; i<2; i++))
		{	exp=${ "$gd" +"$format" --date="$2" 2>/dev/null; }
			if	[[ -z $exp ]]
			then	_message "$1" "warning: printf '%($format)T' 'exact ${3:-$2}':" \
					"GNU date(1) did not provide a reference date for '${3:-$2}'; test skipped"
				return
			fi
			printf -v got "%($format)T" "exact ${3:-$2}"
			[[ $got == "$exp" ]] && return
		}
		\err_exit "$1" "check against GNU 'date': printf '%($format)T' 'exact ${3:-$2}':" \
			"expected $(printf %q "$exp"), got $(printf %q "$got")"
	}

	# Trivial
	T 'now'

	# Abs time
	T '@1234567890.987654321' '#1234567890.987654321'

	# The ago case, inspired by the #182 reproducer. Here we do less testing
	# than that reproducer, but span on more rollover, to trigger a bug that
	# works on 1 rollover but not mode (modulo bugs)
	T '-40 years'
	T '-40 years ago'
	T '-40 months'
	T '-40 months ago'
	T '-40 weeks'
	T '-40 weeks ago'
	T '-40 days'
	T '-40 days ago'
	T '-40 hours'
	T '-40 hours ago'
	T '-40 minutes'
	T '-40 minutes ago'
	T '-40 seconds'
	T '-40 seconds ago'
	T '-20 years'
	T '-20 years ago'
	T '-20 months'
	T '-20 months ago'
	T '-20 weeks'
	T '-20 weeks ago'
	T '-20 days'
	T '-20 days ago'
	T '-20 hours'
	T '-20 hours ago'
	T '-20 minutes'
	T '-20 minutes ago'
	T '-20 seconds'
	T '-20 seconds ago'
	T '-10 years'
	T '-10 years ago'
	T '-10 months'
	T '-10 months ago'
	T '-10 weeks'
	T '-10 weeks ago'
	T '-10 days'
	T '-10 days ago'
	T '-10 hours'
	T '-10 hours ago'
	T '-10 minutes'
	T '-10 minutes ago'
	T '-10 seconds'
	T '-10 seconds ago'
	T '-5 years'
	T '-5 years ago'
	T '-5 months'
	T '-5 months ago'
	T '-5 weeks'
	T '-5 weeks ago'
	T '-5 days'
	T '-5 days ago'
	T '-5 hours'
	T '-5 hours ago'
	T '-5 minutes'
	T '-5 minutes ago'
	T '-5 seconds'
	T '-5 seconds ago'
	T '0 years'
	T '0 years ago'
	T '0 months'
	T '0 months ago'
	T '0 weeks'
	T '0 weeks ago'
	T '0 days'
	T '0 days ago'
	T '0 hours'
	T '0 hours ago'
	T '0 minutes'
	T '0 minutes ago'
	T '0 seconds'
	T '0 seconds ago'
	T '5 years'
	T '5 years ago'
	T '5 months'
	T '5 months ago'
	T '5 weeks'
	T '5 weeks ago'
	T '5 days'
	T '5 days ago'
	T '5 hours'
	T '5 hours ago'
	T '5 minutes'
	T '5 minutes ago'
	T '5 seconds'
	T '5 seconds ago'
	T '10 years'
	T '10 years ago'
	T '10 months'
	T '10 months ago'
	T '10 weeks'
	T '10 weeks ago'
	T '10 days'
	T '10 days ago'
	T '10 hours'
	T '10 hours ago'
	T '10 minutes'
	T '10 minutes ago'
	T '10 seconds'
	T '10 seconds ago'
	T '20 years'
	T '20 years ago'
	T '20 months'
	T '20 months ago'
	T '20 weeks'
	T '20 weeks ago'
	T '20 days'
	T '20 days ago'
	T '20 hours'
	T '20 hours ago'
	T '20 minutes'
	T '20 minutes ago'
	T '20 seconds'
	T '20 seconds ago'
	T '40 years'
	T '40 years ago'
	T '40 months'
	T '40 months ago'
	T '40 weeks'
	T '40 weeks ago'
	T '40 days'
	T '40 days ago'
	T '40 hours'
	T '40 hours ago'
	T '40 minutes'
	T '40 minutes ago'
	T '40 seconds'
	T '40 seconds ago'

	# Various keywords: last, this, next
	# Note that ksh doesn't support fortnight
	T 'last year'
	T 'last year ago'
	T 'last month'
	T 'last month ago'
	T 'last week'
	T 'last week ago'
	T 'last day'
	T 'last day ago'
	T 'last hour'
	T 'last hour ago'
	T 'last minute'
	T 'last minute ago'
	T 'last second'
	T 'last second ago'
	T 'this year'
	T 'this year ago'
	T 'this month'
	T 'this month ago'
	T 'this week'
	T 'this week ago'
	T 'this day'
	T 'this day ago'
	T 'this hour'
	T 'this hour ago'
	T 'this minute'
	T 'this minute ago'
	T 'this second'
	T 'this second ago'
	T 'next year'
	T 'next year ago'
	T 'next month'
	T 'next month ago'
	T 'next week'
	T 'next week ago'
	T 'next day'
	T 'next day ago'
	T 'next hour'
	T 'next hour ago'
	T 'next minute'
	T 'next minute ago'
	T 'next second'
	T 'next second ago'
fi

unset format gd

# ======
# negative non-base-10 numbers were mangled as they were incorrectly treated as unsigned
# https://github.com/ksh93/ksh/issues/696
integer 20 n=20#j12
printf -v got '%s %s %d %..20d %s' "$n" "$((n *= -1))" n n "$n"
exp='20#j12 -7622 -7622 -j12 -20#j12'
[[ $got == "$exp" ]] || err_exit "issue 696 reproducer (expected $(printf %q "$exp"), got $(printf %q "$got"))"
unset n

# ======
# '\0' in format string skips input
# https://github.com/ksh93/ksh/issues/725
exp=1N2N3N4N5N6N7N
got=$(printf '%s\0' 1 2 3 4 5 6 7 | tr '\0' N)
[[ $got == "$exp" ]] || err_exit "'\0' in format string skips input (expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(printf '%s%Z' 1 2 3 4 5 6 7 | tr '\0' N)
[[ $got == "$exp" ]] || err_exit "'%Z' in format string skips input (expected $(printf %q "$exp"), got $(printf %q "$got"))"

exp=1NN2NN3NN4NN5NN6NN7NN
got=$(printf '%s\0\0' 1 2 3 4 5 6 7 | tr '\0' N)
[[ $got == "$exp" ]] || err_exit "'\0\0' in format string skips input (expected $(printf %q "$exp"), got $(printf %q "$got"))"
got=$(printf '%s%Z%Z' 1 2 3 4 5 6 7 | tr '\0' N)
[[ $got == "$exp" ]] || err_exit "'%Z%Z' in format string skips input (expected $(printf %q "$exp"), got $(printf %q "$got"))"

exp=$'1N\n2N\n3N\n4N\nX'
got=$(printf '%s%c%c' 1 $'\0' $'\n' 2 $'\0' $'\n' 3 $'\0' $'\n' 4 $'\0' $'\n' | tr '\0' N; echo X)
[[ $got == "$exp" ]] || err_exit "regression involving %c as \$'\0' (expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# until 2024-07-31, TZ=UTC 'stuck' even if another TZ is set later
(
	ulimit -c 0	# if we have the bug, forking makes it local to the subshell
	TZ=UTC printf -v one '%(%H)T'
	TZ=Europe/Amsterdam printf -v two '%(%H)T'
	[[ $one != "$two" ]]
) || err_exit "printf %T: TZ=UTC sticks after changing TZ"

# ======
# If there was a gap in index conversion specifiers (for example, %5$s given
# without %4$s, %3%s or %2%s), then ksh incorrectly executed the string
# arguments corresponding to the gap as arithmetic expressions.
# https://github.com/ksh93/ksh/issues/824

function do_test
{	typeset lnno=$1 exp=$2 fmt=$3
	typeset -si e
	shift 3
	set -u
	printf -v got "$fmt" "$@" 2>|stderr
	e=$?
	set +u
	[[ $got == "$exp" ]] || \err_exit "$lnno" "printf '$fmt': expected $(printf %q "$exp"), got $(printf %q "$got")"
	[[ -s stderr ]] && \err_exit "$lnno" "printf '$fmt' wrote to standard error: $(printf %q "$(<stderr)")"
	((e)) && \err_exit "$lnno" "printf '$fmt' had nonzero exit status: $e"
}

T $'a e\nf j\n'			'%s %5$s\n'		a b c d e f g h i j
T $'a \n'			'%s %99$s\n'		a b c d e f g h i j
T $'first fifth\nsixth \n'	'%s %5$s\n'		first 2+ @ 2/0 fifth sixth

# ======
exit $((Errors<125?Errors:125))

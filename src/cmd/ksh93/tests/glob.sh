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
#                                                                      #
########################################################################

. "${SHTESTS_COMMON:-${0%/*}/_common}"

function test_glob
{
	typeset lineno expected drop arg got sep op val add del
	lineno=$1
	shift
	if	[[ $1 == --* ]]
	then	del=${1#--}
		shift
	fi
	if	[[ $1 == ++* ]]
	then	add=${1#++}
		shift
	fi
	expected=$1
	shift
	if	(( contrary ))
	then	if	[[ $expected == "<Beware> "* ]]
		then	expected=${expected#"<Beware> "}
			expected="$expected <Beware>"
		fi
		if	[[ $expected == *"<aXb> <abd>"* ]]
		then	expected=${expected/"<aXb> <abd>"/"<abd> <aXb>"}
		fi
	fi
	for arg
	do	got="$got$sep<$arg>"
		sep=" "
	done
	if	(( ignorant && aware ))
	then	if	[[ $del ]]
		then	got="<$del> $got"
		fi
		if	[[ $add ]]
		then	expected="<$add> $expected"
		fi
	fi
	if	[[ $got != "$expected" ]]
	then	'err_exit' $lineno "glob${ [[ -o globstar ]] && print star; } -- expected '$expected', got '$got'"
	fi
}
alias test_glob='test_glob $LINENO'

function test_case
{
	typeset lineno expected subject pattern got
	lineno=$1 expected=$2 subject=$3 pattern=$4
	eval "
		case $subject in
		$pattern)	got='<match>' ;;
		*)		got='<nomatch>' ;;
		esac
	"
	if	[[ $got != "$expected" ]]
	then	'err_exit' $lineno "case $subject in $pattern) -- expected '$expected', got '$got'"
	fi
}
alias test_case='test_case $LINENO'

unset undefined

export LC_COLLATE=C
touch B b
set -- *
case $* in
'b B')	contrary=1 ;;
b|B)	ignorant=1 ;;
esac
set -- $(LC_ALL=C sh -c 'echo [a-c]')
case $* in
B)	aware=1 ;;
esac
rm -rf *

touch a b c d abc abd abe bb bcd ca cb dd de Beware
mkdir bdir

test_glob '<a> <abc> <abd> <abe> <X*>' a* X*
test_glob '<a> <abc> <abd> <abe>' \a*

if	( set --nullglob ) 2>/dev/null
then
	set --nullglob

	test_glob '<a> <abc> <abd> <abe>' a* X*

	set --nonullglob
fi

if	( set --failglob ) 2>/dev/null
then
	set --failglob
	mkdir tmp
	touch tmp/l1 tmp/l2 tmp/l3

	test_glob '' tmp/l[12] tmp/*4 tmp/*3
	test_glob '' tmp/l[12] tmp/*4 tmp/*3

	rm -r tmp
	set --nofailglob
fi

test_glob '<bdir/>' b*/
test_glob '<*>' \*
test_glob '<a*>' 'a*'
test_glob '<a*>' a\*
test_glob '<c> <ca> <cb> <a*> <*q*>' c* a\* *q*
test_glob '<**>' "*"*
test_glob '<**>' \**
test_glob '<\.\./*/>' "\.\./*/"
test_glob '<s/\..*//>' 's/\..*//'
test_glob '</^root:/{s/^[!:]*:[!:]*:\([!:]*\).*$/\1/>' "/^root:/{s/^[!:]*:[!:]*:\([!:]*\).*"'$'"/\1/"
test_glob '<abc> <abd> <abe> <bb> <cb>' [a-c]b*
test_glob ++Beware '<abd> <abe> <bb> <bcd> <bdir> <ca> <cb> <dd> <de>' [a-y]*[!c]
test_glob '<abd> <abe>' a*[!c]

touch a-b aXb

test_glob '<a-b> <aXb>' a[X-]b

touch .x .y

test_glob --Beware '<Beware> <d> <dd> <de>' [!a-c]*

if	mkdir a\*b 2>/dev/null
then
	touch a\*b/ooo

	test_glob '<a*b/ooo>' a\*b/*
	test_glob '<a*b/ooo>' a\*?/*
	test_case '<match>' '!7' '*\!*'
	test_case '<match>' 'r.*' '*.\*'
	test_glob '<abc>' a[b]c
	test_glob '<abc>' a["b"]c
	test_glob '<abc>' a[\b]c
	test_glob '<abc>' a?c
	test_case '<match>' 'abc' 'a"b"c'
	test_case '<match>' 'abc' 'a*c'
	test_case '<nomatch>' 'abc' '"a?c"'
	test_case '<nomatch>' 'abc' 'a\*c'
	test_case '<nomatch>' 'abc' 'a\[b]c'
	test_case '<match>' '"$undefined"' '""'
	test_case '<match>' 'abc' 'a["\b"]c'

	rm -rf mkdir a\*b
fi

mkdir man
mkdir man/man1
touch man/man1/sh.1

test_glob '<man/man1/sh.1>' */man*/sh.*
test_glob '<man/man1/sh.1>' $(echo */man*/sh.*)
test_glob '<man/man1/sh.1>' "$(echo */man*/sh.*)"

test_case '<match>' 'abc' 'a***c'
test_case '<match>' 'abc' 'a*****?c'
test_case '<match>' 'abc' '?*****??'
test_case '<match>' 'abc' '*****??'
test_case '<match>' 'abc' '*****??c'
test_case '<match>' 'abc' '?*****?c'
test_case '<match>' 'abc' '?***?****c'
test_case '<match>' 'abc' '?***?****?'
test_case '<match>' 'abc' '?***?****'
test_case '<match>' 'abc' '*******c'
test_case '<match>' 'abc' '*******?'
test_case '<match>' 'abcdecdhjk' 'a*cd**?**??k'
test_case '<match>' 'abcdecdhjk' 'a**?**cd**?**??k'
test_case '<match>' 'abcdecdhjk' 'a**?**cd**?**??k***'
test_case '<match>' 'abcdecdhjk' 'a**?**cd**?**??***k'
test_case '<match>' 'abcdecdhjk' 'a**?**cd**?**??***k**'
test_case '<match>' 'abcdecdhjk' 'a****c**?**??*****'
test_case '<match>' "'-'" '[-abc]'
test_case '<match>' "'-'" '[abc-]'
test_case '<match>' "'\\'" '\\'
test_case '<match>' "'\\'" '[\\]'
test_case '<match>' "'\\'" "'\\'"
test_case '<match>' "'['" '[[]'
test_case '<match>' '[' '[[]'
test_case '<match>' "'['" '['
test_case '<match>' '[' '['
test_case '<match>' "'[abc'" "'['*"
test_case '<nomatch>' "'[abc'" '[*'
test_case '<match>' '[abc' "'['*"
test_case '<nomatch>' '[abc' '[*'
test_case '<match>' 'abd' "a[b/c]d"
test_case '<match>' 'a/d' "a[b/c]d"
test_case '<match>' 'acd' "a[b/c]d"
test_case '<match>' "']'" '[]]'
test_case '<match>' "'-'" '[]-]'
test_case '<match>' 'p' '[a-\z]'
test_case '<match>' '"/tmp"' '[/\\]*'
test_case '<nomatch>' 'abc' '??**********?****?'
test_case '<nomatch>' 'abc' '??**********?****c'
test_case '<nomatch>' 'abc' '?************c****?****'
test_case '<nomatch>' 'abc' '*c*?**'
test_case '<nomatch>' 'abc' 'a*****c*?**'
test_case '<nomatch>' 'abc' 'a********???*******'
test_case '<nomatch>' "'a'" '[]'
test_case '<nomatch>' 'a' '[]'
test_case '<nomatch>' "'['" '[abc'
test_case '<nomatch>' '[' '[abc'

test_glob ++Beware '<b> <bb> <bcd> <bdir>' b*
test_glob '<Beware> <b> <bb> <bcd> <bdir>' [bB]*

if	( set --nocaseglob ) 2>/dev/null
then
	set --nocaseglob

	test_glob '<Beware> <b> <bb> <bcd> <bdir>' b*
	test_glob '<Beware> <b> <bb> <bcd> <bdir>' [b]*
	test_glob '<Beware> <b> <bb> <bcd> <bdir>' [bB]*

	set --nonocaseglob
fi

if	( set -f ) 2>/dev/null
then
	set -f

	test_glob '<*>' *

	set +f
fi

if	( set --noglob ) 2>/dev/null
then
	set --noglob

	test_glob '<*>' *

	set --glob
fi

FIGNORE='@(.*|*)'
test_glob '<*>' *

FIGNORE='@(.*|*c|*e|?)'
test_glob '<a-b> <aXb> <abd> <bb> <bcd> <bdir> <ca> <cb> <dd> <man>' *

FIGNORE='@(.*|*b|*d|?)'
test_glob '<Beware> <abc> <abe> <bdir> <ca> <de> <man>' *

FIGNORE=
test_glob '<man/man1/sh.1>' */man*/sh.*
test_glob '<.x> <.y> <Beware> <a> <a-b> <aXb> <abc> <abd> <abe> <b> <bb> <bcd> <bdir> <c> <ca> <cb> <d> <dd> <de> <man>' *
test_glob '<.x> <.y>' .*

FIGNORE='@(*[abcd]*)'
test_glob '<.x> <.y>' *
test_glob '<.x> <.y>' .*

unset FIGNORE
test_glob '<bb> <ca> <cb> <dd> <de>' ??
test_glob '<man/man1/sh.1>' */man*/sh.*
test_glob '<Beware> <a> <a-b> <aXb> <abc> <abd> <abe> <b> <bb> <bcd> <bdir> <c> <ca> <cb> <d> <dd> <de> <man>' *
test_glob '<.x> <.y>' .*

GLOBIGNORE='.*:*'
set -- *
if	[[ $1 == '*' ]]
then
	GLOBIGNORE='.*:*c:*e:?'
	test_glob '<>' *

	GLOBIGNORE='.*:*b:*d:?'
	test_glob '<>' *

	unset GLOBIGNORE
	test_glob '<>' *
	test_glob '<man/man1/sh.1>' */man*/sh.*

	GLOBIGNORE=
	test_glob '<man/man1/sh.1>' */man*/sh.*
fi
unset GLOBIGNORE

function test_sub
{
	x='${subject'$2'}'
	eval g=$x
	if	[[ "$g" != "$3" ]]
	then	'err_exit' $1 subject="'$subject' $x failed, expected '$3', got '$g'"
	fi
}
alias test_sub='test_sub $LINENO'

set --noglob
((SHOPT_BRACEPAT)) && set --nobraceexpand

subject='A regular expressions test'

test_sub '/e/#'               'A r#gular expressions test'
test_sub '//e/#'              'A r#gular #xpr#ssions t#st'
test_sub '/[^e]/#'            '# regular expressions test'
test_sub '//[^e]/#'           '###e######e###e########e##'
test_sub '/+(e)/#'            'A r#gular expressions test'
test_sub '//+(e)/#'           'A r#gular #xpr#ssions t#st'
test_sub '/@-(e)/#'           'A r#gular expressions test'
test_sub '//@-(e)/#'          'A r#gular #xpr#ssions t#st'
test_sub '/?(e)/#'            '#A regular expressions test'
test_sub '//?(e)/#'           '#A# #r#g#u#l#a#r# #x#p#r#s#s#i#o#n#s# #t#s#t#'
test_sub '/*(e)/#'            '#A regular expressions test'
test_sub '//*(e)/#'           '#A# #r#g#u#l#a#r# #x#p#r#s#s#i#o#n#s# #t#s#t#'
test_sub '//@(e)/[\1]'        'A r[e]gular [e]xpr[e]ssions t[e]st'
test_sub '//@-(e)/[\1]'       'A r[e]gular [e]xpr[e]ssions t[e]st'
test_sub '//+(e)/[\1]'        'A r[e]gular [e]xpr[e]ssions t[e]st'
test_sub '//+-(e)/[\1]'       'A r[e]gular [e]xpr[e]ssions t[e]st'
test_sub '//@(+(e))/[\1]'     'A r[e]gular [e]xpr[e]ssions t[e]st'
test_sub '//@(+-(e))/[\1]'    'A r[e]gular [e]xpr[e]ssions t[e]st'
test_sub '//-(e)/#'           'A regular expressions test'
test_sub '//--(e)/#'          'A regular expressions test'
test_sub '//?(e)/[\1]'        '[]A[] []r[e]g[]u[]l[]a[]r[] [e]x[]p[]r[e]s[]s[]i[]o[]n[]s[] []t[e]s[]t[]'
test_sub '//{0,1}(e)/[\1]'    '[]A[] []r[e]g[]u[]l[]a[]r[] [e]x[]p[]r[e]s[]s[]i[]o[]n[]s[] []t[e]s[]t[]'
test_sub '//*(e)/[\1]'        '[]A[] []r[e]g[]u[]l[]a[]r[] [e]x[]p[]r[e]s[]s[]i[]o[]n[]s[] []t[e]s[]t[]'
test_sub '//{0,}(e)/[\1]'     '[]A[] []r[e]g[]u[]l[]a[]r[] [e]x[]p[]r[e]s[]s[]i[]o[]n[]s[] []t[e]s[]t[]'
test_sub '//@(?(e))/[\1]'     '[]A[] []r[e]g[]u[]l[]a[]r[] [e]x[]p[]r[e]s[]s[]i[]o[]n[]s[] []t[e]s[]t[]'
test_sub '//@({0,1}(e))/[\1]' '[]A[] []r[e]g[]u[]l[]a[]r[] [e]x[]p[]r[e]s[]s[]i[]o[]n[]s[] []t[e]s[]t[]'
test_sub '//@(*(e))/[\1]'     '[]A[] []r[e]g[]u[]l[]a[]r[] [e]x[]p[]r[e]s[]s[]i[]o[]n[]s[] []t[e]s[]t[]'
test_sub '//@({0,}(e))/[\1]'  '[]A[] []r[e]g[]u[]l[]a[]r[] [e]x[]p[]r[e]s[]s[]i[]o[]n[]s[] []t[e]s[]t[]'
test_sub '/?-(e)/#'           '#A regular expressions test'
test_sub '/@(?-(e))/[\1]'     '[]A regular expressions test'
test_sub '/!(e)/#'            '#'
test_sub '//!(e)/#'           '#'
test_sub '/@(!(e))/[\1]'      '[A regular expressions test]'
test_sub '//@(!(e))/[\1]'     '[A regular expressions test]'

subject='e'

test_sub '/!(e)/#'            '#e'
test_sub '//!(e)/#'           '#e#'
test_sub '/!(e)/[\1]'         '[]e'
test_sub '//!(e)/[\1]'        '[]e[]'
test_sub '/@(!(e))/[\1]'      '[]e'
test_sub '//@(!(e))/[\1]'     '[]e[]'

subject='a'

test_sub '/@(!(a))/[\1]'      '[]a'
test_sub '//@(!(a))/[\1]'     '[]a[]'

subject='aha'

test_sub '/@(!(a))/[\1]'      '[aha]'
test_sub '//@(!(a))/[\1]'     '[aha]'
test_sub '/@(!(aha))/[\1]'    '[ah]a'
test_sub '//@(!(aha))/[\1]'   '[ah][a]'

# ======
# Recursive double-star globbing (globstar) tests
set --glob --globstar
mkdir -p d_un/d_duo/d_tres/d_quatro d_un/d_duo/d_3/d_4
touch d_un/d_duo/.tres
ln -s d_duo d_un/d_sym

# As of commit 5312a59d, globstar failed to expand **/. or **/.. or **/./file or **/../file
# https://github.com/ksh93/ksh/issues/146#issuecomment-790845391
test_glob \
 '<d_un/.> <d_un/d_duo/.> <d_un/d_duo/d_3/.> <d_un/d_duo/d_3/d_4/.> <d_un/d_duo/d_tres/.> <d_un/d_duo/d_tres/d_quatro/.>' \
  d_un/**/.
test_glob \
 '<d_un/..> <d_un/d_duo/..> <d_un/d_duo/d_3/..> <d_un/d_duo/d_3/d_4/..> <d_un/d_duo/d_tres/..> <d_un/d_duo/d_tres/d_quatro/..>' \
  d_un/**/..
test_glob \
 '<d_un/./d_duo> <d_un/./d_sym> <d_un/d_duo/./d_3> <d_un/d_duo/./d_tres> <d_un/d_duo/d_3/./d_4> <d_un/d_duo/d_tres/./d_quatro>' \
  d_un/**/./d_*
test_glob \
 '<d_un/../d_un> <d_un/d_duo/../d_duo> <d_un/d_duo/../d_sym> <d_un/d_duo/d_3/../d_3> <d_un/d_duo/d_3/../d_tres>'\
' <d_un/d_duo/d_3/d_4/../d_4> <d_un/d_duo/d_tres/../d_3> <d_un/d_duo/d_tres/../d_tres> <d_un/d_duo/d_tres/d_quatro/../d_quatro>' \
  d_un/**/../d_*
test_glob '<d_un/d_duo/.tres>' d_un/**/.*
test_glob '<d_un/d_duo/d_3/../.tres> <d_un/d_duo/d_tres/../.tres>' d_un/*/**/../.*
test_glob \
	'<d_un/d_duo/d_3/../.tres> <d_un/d_duo/d_tres/../.tres> <d_un/d_sym/d_3/../.tres> <d_un/d_sym/d_tres/../.tres>' \
	d_un/**/*/../.*
test_glob '<d_un/./d_duo/./d_3/./.././.tres> <d_un/./d_duo/./d_tres/./.././.tres>' d_un/./**/./*/./.././.*

# New in 93u+m 2021-03-06: follow symlink to directory if specified literally or matched by a regular glob pattern component
# https://github.com/ksh93/ksh/issues/146#issuecomment-792142794
test_glob '<d_un/d_sym/d_3> <d_un/d_sym/d_3/d_4> <d_un/d_sym/d_tres> <d_un/d_sym/d_tres/d_quatro>' d_un/d_sym/**
test_glob '<d_un/d_sym> <d_un/d_sym/d_3> <d_un/d_sym/d_3/d_4> <d_un/d_sym/d_tres> <d_un/d_sym/d_tres/d_quatro>' d_un/d_sy[m]/**
test_glob '<d_un/d_sym/d_3/d_4>' d_un/d_sym/d_3/**
test_glob '<d_un/d_sym/d_3/d_4>' d_un/d_sy[m]/d_3/**
test_glob '<d_un/d_duo> <d_un/d_duo/d_3> <d_un/d_duo/d_3/d_4> <d_un/d_duo/d_tres> <d_un/d_duo/d_tres/d_quatro>' **/d_duo/**
test_glob '<d_un/d_sym> <d_un/d_sym/d_3> <d_un/d_sym/d_3/d_4> <d_un/d_sym/d_tres> <d_un/d_sym/d_tres/d_quatro>' **/d_sym/**
test_glob '<d_un/d_sym> <d_un/d_sym/d_3> <d_un/d_sym/d_3/d_4> <d_un/d_sym/d_tres> <d_un/d_sym/d_tres/d_quatro>' **/d_s[y]m/**
test_glob '<d_un/d_sym> <d_un/d_sym/d_3> <d_un/d_sym/d_3/d_4> <d_un/d_sym/d_tres> <d_un/d_sym/d_tres/d_quatro>' **/d_*ym/**
test_glob '<d_un/d_sym//d_3> <d_un/d_sym//d_3/d_4> <d_un/d_sym//d_tres> <d_un/d_sym//d_tres/d_quatro>' **/d_sym//**
test_glob '<d_un/d_sym//d_3> <d_un/d_sym//d_3/d_4> <d_un/d_sym//d_tres> <d_un/d_sym//d_tres/d_quatro>' **/d_[s]ym//**
test_glob '<d_un/d_sym//d_3> <d_un/d_sym//d_3/d_4> <d_un/d_sym//d_tres> <d_un/d_sym//d_tres/d_quatro>' **/d_*ym//**

set --noglobstar

# ======
# Shell quoting within bracket expressions in glob patterns had no effect
# https://github.com/ksh93/ksh/issues/488

mkdir BUG_BRACQUOT
cd BUG_BRACQUOT
: > b > \\
test_glob '<[a-c]>' [a'-'c]
test_glob '<[!N]>' ['!'N]
test_glob '<[^N]>' ['^'N]
test_glob '<[a-c]>' [a$'-'c]
test_glob '<[!N]>' [$'!'N]
test_glob '<[^N]>' [$'^'N]
test_glob '<[a-c]>' [a"-"c]
test_glob '<[!N]>' ["!"N]
test_glob '<[^N]>' ["^"N]
test_glob '<[a-c]>' [a\-c]
test_glob '<[!N]>' [\!N]
test_glob '<[^N]>' [\^N]
# same tests again with ~(S) prefixed (sh pattern)
# also check that S cancels out previous E
test_glob '<~(ES)[a-c]>' ~(ES)[a'-'c]
test_glob '<~(ES)[!N]>' ~(ES)['!'N]
test_glob '<~(ES)[^N]>' ~(ES)['^'N]
test_glob '<~(ES)[a-c]>' ~(ES)[a$'-'c]
test_glob '<~(ES)[!N]>' ~(ES)[$'!'N]
test_glob '<~(ES)[^N]>' ~(ES)[$'^'N]
test_glob '<~(ES)[a-c]>' ~(ES)[a"-"c]
test_glob '<~(ES)[!N]>' ~(ES)["!"N]
test_glob '<~(ES)[^N]>' ~(ES)["^"N]
test_glob '<~(ES)[a-c]>' ~(ES)[a\-c]
test_glob '<~(ES)[!N]>' ~(ES)[\!N]
test_glob '<~(ES)[^N]>' ~(ES)[\^N]
# same tests again with ~(K) prefixed (ksh pattern)
test_glob '<~(K)[a-c]>' ~(K)[a'-'c]
test_glob '<~(K)[!N]>' ~(K)['!'N]
test_glob '<~(K)[^N]>' ~(K)['^'N]
test_glob '<~(K)[a-c]>' ~(K)[a$'-'c]
test_glob '<~(K)[!N]>' ~(K)[$'!'N]
test_glob '<~(K)[^N]>' ~(K)[$'^'N]
test_glob '<~(K)[a-c]>' ~(K)[a"-"c]
test_glob '<~(K)[!N]>' ~(K)["!"N]
test_glob '<~(K)[^N]>' ~(K)["^"N]
test_glob '<~(K)[a-c]>' ~(K)[a\-c]
test_glob '<~(K)[!N]>' ~(K)[\!N]
test_glob '<~(K)[^N]>' ~(K)[\^N]
# quoting should also work for the end character ']'
test_glob '<[]-z]>' [\]\-z]
test_glob '<[]-z]>' [']-z']
test_glob '<[]-z]>' ["]-z"]
test_glob '<~(S)[]-z]>' ~(S)[\]\-z]
test_glob '<~(S)[]-z]>' ~(S)[']-z']
test_glob '<~(S)[]-z]>' ~(S)["]-z"]
test_glob '<~(K)[]-z]>' ~(K)[\]\-z]
test_glob '<~(K)[]-z]>' ~(K)[']-z']
test_glob '<~(K)[]-z]>' ~(K)["]-z"]
# check internal escaping of bracket expression in glob pattern resulting from field splitting
# https://github.com/ksh93/ksh/issues/549
unquoted_patvar='[\!N]'; test_glob '<[\!N]>' $unquoted_patvar
unquoted_patvar='[\^N]'; test_glob '<[\^N]>' $unquoted_patvar
unquoted_patvar='[a\-c]'; test_glob '<[a\-c]>' $unquoted_patvar
: > -; test_glob '<->' $unquoted_patvar
: > a > c; test_glob '<-> <a> <c>' $unquoted_patvar
: > !; unquoted_patvar='[\!N]'; test_glob '<!>' $unquoted_patvar
: > ^; unquoted_patvar='[\^N]'; test_glob '<^>' $unquoted_patvar
if	[[ -o ?globex ]]
then	set -o globex
	rm ./- a c ! ^
	unquoted_patvar='~(S)[\!N]'; test_glob '<~(S)[\!N]>' $unquoted_patvar
	unquoted_patvar='~(S)[\^N]'; test_glob '<~(S)[\^N]>' $unquoted_patvar
	unquoted_patvar='~(S)[a\-c]'; test_glob '<~(S)[a\-c]>' $unquoted_patvar
	: > -; test_glob '<->' $unquoted_patvar
	: > a > c; test_glob '<-> <a> <c>' $unquoted_patvar
	: > !; unquoted_patvar='~(S)[\!N]'; test_glob '<!>' $unquoted_patvar
	: > ^; unquoted_patvar='~(S)[\^N]'; test_glob '<^>' $unquoted_patvar
	rm ./- a c ! ^
	unquoted_patvar='~(K)[\!N]'; test_glob '<~(K)[\!N]>' $unquoted_patvar
	unquoted_patvar='~(K)[\^N]'; test_glob '<~(K)[\^N]>' $unquoted_patvar
	unquoted_patvar='~(K)[a\-c]'; test_glob '<~(K)[a\-c]>' $unquoted_patvar
	: > -; test_glob '<->' $unquoted_patvar
	: > a > c; test_glob '<-> <a> <c>' $unquoted_patvar
	: > !; unquoted_patvar='~(K)[\!N]'; test_glob '<!>' $unquoted_patvar
	: > ^; unquoted_patvar='~(K)[\^N]'; test_glob '<^>' $unquoted_patvar
	set +o globex
fi
cd ..

# ======
# https://www.reddit.com/r/ksh/comments/13k9af3/globbing_eres/jkjpk5a/
: > 'ef{' > 'o1_mf_1_13962_l5p4ts16_.arc'
unquoted_patvar='*f{'
((SHOPT_BRACEPAT)) && set +B
test_glob '<ef{>' *f{
test_glob '<ef{>' $unquoted_patvar
if	((SHOPT_BRACEPAT))
then	set -B
	test_glob '<ef{>' *f{
	test_glob '<ef{>' $unquoted_patvar	# basic shell patterns should be expanded from unquoted vars
fi
unquoted_patvar='~(K)*f{'
((SHOPT_BRACEPAT)) && set +B
test_glob '<ef{>' ~(K)*f{
test_glob '<~(K)*f{>' $unquoted_patvar  	# extended patterns should *not* be expanded from unquoted vars (Bourne/POSIX compat)
if	[[ -o ?globex ]]
then	set -o globex				# ...unless the globex option is on
	test_glob '<ef{>' $unquoted_patvar
	set +o globex
fi
if	((SHOPT_BRACEPAT))
then	set -B
	test_glob '<ef{>' ~(K)*f{
	test_glob '<~(K)*f{>' $unquoted_patvar
	if	[[ -o ?globex ]]
	then	set -o globex
		test_glob '<ef{>' $unquoted_patvar
		set +o globex
	fi
fi
unquoted_patvar='~(E)^o1_mf_1_\d{1,6}_[[:alnum:]]{8}_.arc$'
((SHOPT_BRACEPAT)) && set +B
test_glob '<o1_mf_1_13962_l5p4ts16_.arc>' ~(E)^o1_mf_1_\d{1,6}_[[:alnum:]]{8}_.arc$
test_glob '<~(E)^o1_mf_1_\d{1,6}_[[:alnum:]]{8}_.arc$>' $unquoted_patvar
if	[[ -o ?globex ]]
then	set -o globex
	test_glob '<o1_mf_1_13962_l5p4ts16_.arc>' $unquoted_patvar
	set +o globex
fi
if	((SHOPT_BRACEPAT))
then	set -B
	test_glob '<o1_mf_1_13962_l5p4ts16_.arc>' ~(E)^o1_mf_1_\d{1,6}_[[:alnum:]]{8}_.arc$
	test_glob '<o1_mf_1_13962_l5p4ts16_.arc>' o~(E)1_mf_1_\d{1,6}_[[:alnum:]]{8}_.arc$
	test_glob '<o1_mf_1_13962_l5p4ts16_.arc>' o1~(E)_mf_1_\d{1,6}_[[:alnum:]]{8}_.arc$
	test_glob '<o1_mf_1_13962_l5p4ts16_.arc>' o1_~(E)mf_1_\d{1,6}_[[:alnum:]]{8}_.arc$
	test_glob '<o1_mf_1_13962_l5p4ts16_.arc>' o1_m~(E)f_1_\d{1,6}_[[:alnum:]]{8}_.arc$
	test_glob '<o1_mf_1_13962_l5p4ts16_.arc>' o1_mf~(E)_1_\d{1,6}_[[:alnum:]]{8}_.arc$
	test_glob '<o1_mf_1_13962_l5p4ts16_.arc>' o1_mf_~(E)1_\d{1,6}_[[:alnum:]]{8}_.arc$
	test_glob '<o1_mf_1_13962_l5p4ts16_.arc>' o1_mf_1~(E)_\d{1,6}_[[:alnum:]]{8}_.arc$
	test_glob '<o1_mf_1_13962_l5p4ts16_.arc>' o1_mf_1_~(E)\d{1,6}_[[:alnum:]]{8}_.arc$
	# in non-POSIX mode, brace expansion *is* expanded from unquoted var, though extended patterns are not.
	# TODO: consider fixing this insanity as of ksh 93u+m/1.1 (incompatible change)
	test_glob '<~(E)^o1_mf_1_\d1_[[:alnum:]]{8}_.arc$> <~(E)^o1_mf_1_\d6_[[:alnum:]]{8}_.arc$>' $unquoted_patvar
	if	[[ -o ?globex ]]
	then	set -o globex
		test_glob '<o1_mf_1_13962_l5p4ts16_.arc>' $unquoted_patvar
		set +o globex
	fi
fi
# not all ~(...) should disallow brace expansion
if	((SHOPT_BRACEPAT))
then	set -B
	touch ANNOUNCE aSan_CCFLAGS.sav
	unquoted_patvar='~(i:A){N,S}*'
	test_glob '<ANNOUNCE> <aSan_CCFLAGS.sav>' ~(i:A){N,S}*
	test_glob '<~(i:A)N*> <~(i:A)S*>' $unquoted_patvar
	if	[[ -o ?globex ]]
	then	set -o globex
		test_glob '<ANNOUNCE> <aSan_CCFLAGS.sav>' $unquoted_patvar
		set +o globex
	fi
fi

# ======
: > 'a\\b.txt'
unquoted_patvar='a\\\\b.*'
test_glob '<a\\b.txt>' a\\\\b.*
test_glob '<a\\b.txt>' $unquoted_patvar

# ======
# 93u+m/1.0.5 regression - glob expansion with brace expansion and parameter expansion
v='./'
mkdir -p bin/BAD sbin/WRONG
((SHOPT_BRACEPAT)) && test_glob '<./bin/*> <./sbin/*>' "./"{bin,sbin}"/*"
test_glob '<./bin/*>' "$v"bin"/*"
((SHOPT_BRACEPAT)) && test_glob '<./bin/*> <./sbin/*>' "$v"{bin,sbin}"/*"
unset null
((SHOPT_BRACEPAT)) && test_glob '<b*> <s*>' {b,s}"*"
((SHOPT_BRACEPAT)) && test_glob '<b*> <s*>' $null{b,s}"*"
((SHOPT_BRACEPAT)) && test_glob '<*b> <*c>' $null"*"{b,c}
((SHOPT_BRACEPAT)) && test_glob '<*b> <*c>' "*"$null{b,c}
test_glob '<*>' $null"*"

# ======
exit $((Errors<125?Errors:125))

########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1982-2011 AT&T Intellectual Property          #
#          Copyright (c) 2020-2022 Contributors to ksh 93u+m           #
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

bar=foo2
bam=foo[3]
for i in foo1 foo2 foo3 foo4 foo5 foo6
do	foo=0
	case $i in
	foo1)	foo=1;;
	$bar)	foo=2;;
	$bam)	foo=3;;
	foo[4])	foo=4;;
	${bar%?}5)
		foo=5;;
	"${bar%?}6")
		foo=6;;
	esac
	if	[[ $i != foo$foo ]]
	then	err_exit "$i not matching correct pattern"
	fi
done
f="[ksh92]"
case $f in
\[*\])  ;;
*)      err_exit "$f does not match \[*\]";;
esac

if	[[ $($SHELL -c '
		x=$(case abc {
			abc)	{ print yes;};;
			*)	 print no;;
			}
		)
		print -r -- "$x"' 2> /dev/null) != yes ]]
then err_exit 'case abc {...} not working'
fi
[[ $($SHELL -c 'case a in
a)      print -n a > /dev/null ;&
b)      print b;;
esac') != b ]] && err_exit 'bug in ;& at end of script'
[[ $(VMDEBUG=1 $SHELL -c '
	tmp=foo
	for i in a b
	do	case $i in
		a)	:  tmp=$tmp tmp.h=$tmp.h;;
		b)	( tmp=bar )
			for j in a
			do	print -r -- $tmp.h
			done
			;;
		esac
	done
') == foo.h ]] || err_exit "optimizer bug"

x=$($SHELL -ec 'case a in a) echo 1; false; echo 2 ;& b) echo 3;; esac')
[[ $x == 1 ]] || err_exit 'set -e ignored on case fail through'

# ======
# An empty 'case' list was a syntax error: https://github.com/ksh93/ksh/issues/177
got=$(eval 'case x in esac' 2>&1) || err_exit "empty case list fails: got $(printf %q "$got")"
got=$(eval ': || for i in esac; do :; done' 2>&1) || err_exit "'for i in esac' fails: got $(printf %q "$got")"
got=$(eval ': || select i in esac; do :; done' 2>&1) || err_exit "'select i in esac' fails: got $(printf %q "$got")"
(eval 'case x in esac) foo;; esac') 2>/dev/null && err_exit "unquoted 'esac' keyword as first pattern fails to fail"
got=$(eval 'case x in (esac) foo;; esac' 2>&1) || err_exit "'(' + unquoted 'esac' keyword as first pattern fails: got $(printf %q "$got")"
(eval 'case x in y) bar;; esac) foo;; esac') 2>/dev/null && err_exit "unquoted 'esac' keyword as nth pattern fails to fail"
got=$(eval 'case x in (y) bar;; (esac) foo;; esac' 2>&1) || err_exit "'(' + unquoted 'esac' keyword as nth pattern fails: got $(printf %q "$got")"
got=$(eval 'case x in e\sac) foo;; esac' 2>&1) || err_exit "quoted 'esac' pattern (1st) fails: got $(printf %q "$got")"
got=$(eval 'case x in (e\sac) foo;; esac' 2>&1) || err_exit "'(' + quoted 'esac' pattern (1st) fails: got $(printf %q "$got")"
got=$(eval 'case x in y) bar;; es\ac) foo;; esac' 2>&1) || err_exit "quoted 'esac' pattern (nth) fails: got $(printf %q "$got")"
got=$(eval 'case x in (y) bar;; (es\ac) foo;; esac' 2>&1) || err_exit "'(' + quoted 'esac' pattern (nth) fails: got $(printf %q "$got")"
got=$(eval 'case x in if);; esac' 2>&1) || err_exit "'if' as first pattern fails: got $(printf %q "$got")"
got=$(eval 'case x in (if);; esac' 2>&1) || err_exit "'(' + 'if' as first pattern fails: got $(printf %q "$got")"
got=$(eval 'case x in foo);; if);; esac' 2>&1) || err_exit "'if' as nth pattern fails: got $(printf %q "$got")"
got=$(eval 'case x in (foo);; (if);; esac' 2>&1) || err_exit "'(' + 'if' as nth pattern fails: got $(printf %q "$got")"

# ======
# Verify an invalid character class name is handled without a SIGSEGV or similar failure
# https://github.com/att/ast/issues/1409
got=$(set +x; { "$SHELL" -c 'case x in [x[:bogus:]]) echo x ;; esac'; } 2>&1)
((!(e = $?))) && [[ -z $got ]] || err_exit 'use of invalid character class name' \
	"(got status $e$( ((e>128)) && print -n /SIG && kill -l "$e"), $(printf %q "$got"))"

# ======
# Shell quoting within bracket expressions in glob patterns had no effect
# https://github.com/ksh93/ksh/issues/488

case b in *[a'-'c]*) err_exit 'BUG_BRACQUOT: 1a';; esac
case b in *['!'N]*) err_exit 'BUG_BRACQUOT: 1b';; esac
case b in *['^'N]*) err_exit 'BUG_BRACQUOT: 1c';; esac
case b in *[a$'-'c]*) err_exit 'BUG_BRACQUOT: 2a';; esac
case b in *[$'!'N]*) err_exit 'BUG_BRACQUOT: 2b';; esac
case b in *[$'^'N]*) err_exit 'BUG_BRACQUOT: 2c';; esac
case b in *[a"-"c]*) err_exit 'BUG_BRACQUOT: 3a';; esac
case b in *["!"N]*) err_exit 'BUG_BRACQUOT: 3b';; esac
case b in *["^"N]*) err_exit 'BUG_BRACQUOT: 3c';; esac
case b in *[a\-c]*) err_exit 'BUG_BRACQUOT: 4a';; esac
case b in *[\!N]*) err_exit 'BUG_BRACQUOT: 4b';; esac
case b in *[\^N]*) err_exit 'BUG_BRACQUOT: 4c';; esac
p='*[a\-c]*'; case b in $p) err_exit 'BUG_BRACQUOT: 5a';; esac
p='*[\!N]*'; case b in $p) err_exit 'BUG_BRACQUOT: 5b';; esac
p='*[\^N]*'; case b in $p) err_exit 'BUG_BRACQUOT: 5c';; esac
p='*[a\-c]*'; case - in $p) ;; *) err_exit 'BUG_BRACQUOT: 6a';; esac
p='*[\!N]*'; case \! in $p) ;; *) err_exit 'BUG_BRACQUOT: 6b';; esac
p='*[\^N]*'; case ^ in $p) ;; *) err_exit 'BUG_BRACQUOT: 6c';; esac

# also test bracket expressions with ] as the first character, e.g. []abc]
case b in *[]a'-'c]*) err_exit 'BUG_BRACQUOT: A1';; esac
case b in *[]a$'-'c]*) err_exit 'BUG_BRACQUOT: A2';; esac
case b in *[]a"-"c]*) err_exit 'BUG_BRACQUOT: A3';; esac
case b in *[]a\-c]*) err_exit 'BUG_BRACQUOT: A4';; esac

# ======
exit $((Errors<125?Errors:125))

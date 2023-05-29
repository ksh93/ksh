########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1982-2011 AT&T Intellectual Property          #
#          Copyright (c) 2020-2023 Contributors to ksh 93u+m           #
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
# Handling of escapes in pattern matching contexts
# https://marc.info/?l=ast-users&m=136562952931688&w=2
p=\\x
[[ x == $p ]] && err_exit '[[ does not handle escapes correctly'
case x in
$p) err_exit 'case statements do not handle escapes correctly' ;;
esac
# https://github.com/ksh93/ksh/issues/488#issuecomment-1262641076
p=\\
case \\ in
$p)	;;
*)	err_exit 'case statements do not correctly handle a dangling backslash' ;;
esac

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

# quoting should also work for the end character ']'
case b in [\]\-z]) err_exit 'BUG_BRACQUOT: 7a' ;; esac
case b in [']-z']) err_exit 'BUG_BRACQUOT: 7b' ;; esac
case b in ["]-z"]) err_exit 'BUG_BRACQUOT: 7c' ;; esac

# also test bracket expressions with ] as the first character, e.g. []abc]
case b in *[]a'-'c]*) err_exit 'BUG_BRACQUOT: A1';; esac
case b in *[]a$'-'c]*) err_exit 'BUG_BRACQUOT: A2';; esac
case b in *[]a"-"c]*) err_exit 'BUG_BRACQUOT: A3';; esac
case b in *[]a\-c]*) err_exit 'BUG_BRACQUOT: A4';; esac

# ======
# Test various backslash behaviours in glob patterns. Thanks to Daniel Douglas for these.
# https://gist.github.com/ormaaj/6195070
# https://github.com/ksh93/ksh/pull/556#issuecomment-1278528579
# The cases that include unquoted ${p} in the pattern are unspecified by POSIX and shells vary widely,
# so it may be acceptable to change them if necessary to implement another fix in a reasonable manner.
p=\\
case \\x in "${p}""${p}"x)	err_exit "match: ormaaj case test 1";; esac
case \\x in "${p}"${p}x)	warning "match: ormaaj case test 2 [unspecified]";; esac
case \\x in "${p}""\\"x)	err_exit "match: ormaaj case test 3";; esac
case \\x in "${p}"\\x)		err_exit "match: ormaaj case test 4";; esac
case \\x in ${p}"${p}"x)	warning "match: ormaaj case test 5 [unspecified]";; esac
case \\x in ${p}${p}x) ;; *)	warning "non-match: ormaaj case test 6 [unspecified]";; esac
case \\x in ${p}"\\"x)		warning "match: ormaaj case test 7 [unspecified]";; esac
case \\x in ${p}\\x)		warning "match: ormaaj case test 8 [unspecified]";; esac
case \\x in "\\""${p}"x)	err_exit "match: ormaaj case test 9";; esac
case \\x in "\\"${p}x)		warning "match: ormaaj case test 10 [unspecified]";; esac
case \\x in "\\""\\"x)		err_exit "match: ormaaj case test 11";; esac
case \\x in "\\"\\x)		err_exit "match: ormaaj case test 12";; esac
case \\x in \\"${p}"x)		err_exit "match: ormaaj case test 13";; esac
case \\x in \\${p}x)		warning "match: ormaaj case test 14 [unspecified]";; esac
case \\x in \\"\\"x)		err_exit "match: ormaaj case test 15";; esac
case \\x in \\\\x)		err_exit "match: ormaaj case test 16";; esac

# ======
# negator quoting
case \\ in [!X]) ;; *) err_exit "negator ! not working";; esac
case \\ in [^X]) ;; *) err_exit "negator ^ not working";; esac
case \\ in [\!X]) err_exit "\\ mismatches \\!";; esac
case \\ in [\^X]) err_exit "\\ mismatches \\^";; esac
case \\ in ['!'X]) err_exit "\\ mismatches '!'";; esac
case \\ in ['^'X]) err_exit "\\ mismatches '^'";; esac
case \\ in ["!"X]) err_exit "\\ mismatches \"!\"";; esac
case \\ in ["^"X]) err_exit "\\ mismatches \"^\"";; esac
case \\ in [$'!'X]) err_exit "\\ mismatches \$'!'";; esac
case \\ in [$'^'X]) err_exit "\\ mismatches \$'^'";; esac

# ======
exit $((Errors<125?Errors:125))

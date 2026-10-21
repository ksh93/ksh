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
#                Govind Kamat <govind_kamat@yahoo.com>                 #
#               K. Eugene Carlson <kvngncrlsn@gmail.com>               #
#                      Phi <phi.debian@gmail.com>                      #
#                   Spenser Truex <truex@equwal.com>                   #
#                                                                      #
########################################################################

. "${SHTESTS_COMMON:-${0%/*}/_common}"

# These are tests for the interactive shell, run in a pseudoterminal utility
# called 'pty', which allows for scripting interactive sessions and which is
# installed in arch/*/bin while building. To understand these tests, first
# read the pty manual by running: arch/*/bin/pty --man
#
# Do not globally set the locale; these tests must pass for all locales.

# the trickiest part of the tests is avoiding typeahead
# in the pty dialogue

JOBMAX=32  # max number of parallel tests

((!SHOPT_SCRIPTONLY)) || { warning "interactive shell was compiled out -- tests skipped"; exit 0; }
whence -q pty || { warning "pty command not found -- tests skipped"; exit 0; }
case $(uname -s) in
Darwin | DragonFly | FreeBSD | Linux | NetBSD | MidnightBSD | OpenBSD )
	;;
* )	warning "pty not confirmed to work correctly on this system -- tests skipped"
	exit 0 ;;
esac

# On some systems, the stty command does not appear to work correctly on a pty pseudoterminal.
# To avoid false regressions, we have to set 'erase' and 'kill' on the real terminal.
if	test -t 0 2>/dev/null </dev/tty && stty_restore=$(stty -g </dev/tty)
then	trap 'stty "$stty_restore" </dev/tty' EXIT  # note: on ksh, the EXIT trap is also triggered for termination due to a signal
	stty erase ^H kill ^X </dev/tty >/dev/tty 2>&1
else	warning "cannot set tty state -- tests skipped"
	exit 0
fi

enum -i bool=(false true)
typeset -i bgtests=0 fgtests=0 i
bintrue=$(whence -p true)

x=$( "$SHELL" 2>&1 <<- \EOF
		trap 'exit 0' EXIT
		bintrue=$(whence -p true)
		set -o monitor
		{
			eval $'command set -o vi 2>/dev/null\npty $bintrue'
		} < /dev/null & pid=$!
		jobs
		kill $$
	EOF
)
[[ $x == *Stop* ]] && err_exit "monitor mode enabled incorrectly causes job to stop (got $(printf %q "$x"))"

bool xtrace
if	[[ -o xtrace ]]
then	debug=--debug=1
	xtrace=true
else	debug=
	xtrace=false
fi

function tst
{
	bool noparallel=false
	while	[[ $1 == -* ]]
	do	case $1 in
		--)	shift
			break
			;;
		--debug)
			typeset debug=--debug=1
			;;
		--debug=*)
			typeset debug=$1
			;;
		--noparallel)
			noparallel=true
			;;
		-*)
			print -r "$0: tst: $1: unknown option" >&2
			exit $(((++Errors) < 125 ? Errors : 125))
		esac
		shift
	done
	if	((noparallel || xtrace))
	then	((fgtests++))
		export HISTFILE=$tmp/pty_history_fg_$fgtests
		{
			print $1  # lineno
			pty $debug --dialogue --messages='/dev/fd/1' 2>/dev/tty "$SHELL"
		} | chk
	else	((bgtests++))
		export HISTFILE=$tmp/pty_history_bg_$bgtests
		{
			print $1  # lineno
			pty $debug --dialogue --messages='/dev/fd/1' 2>/dev/tty "$SHELL" &
		} >pty_$bgtests.out
	fi
}

function chk
{
	typeset -i lineno offset
	typeset text

	read lineno
	while	read -r text
	do	if	[[ $text == *debug* ]]
		then	print -u2 -r -- "$text"
		else	offset=${text/*: line +([[:digit:]]):*/\1}
			err\_exit "$lineno" "${text/: line $offset:/: line $(( lineno + offset)):}"
		fi
	done
}

# VISUAL, or if that is not set, EDITOR, automatically sets vi, gmacs or emacs mode if
# its value matches *[Vv][Ii]*, *gmacs* or *macs*, respectively. See put_ed() in init.c.
unset EDITOR
if	((SHOPT_VSH))
then	export VISUAL=vi
elif	((SHOPT_ESH))
then	export VISUAL=emacs
else	unset VISUAL
fi
export PS1=':test-!: ' PS2='> ' PS4=': ' ENV=/./dev/null EXINIT= TERM=dumb

if	! pty $bintrue < /dev/null
then	warning "pty command hangs on $bintrue -- tests skipped"
	exit 0
fi

# ksh invokes tput(1) to get terminal escape sequences necessary for multiline editing.
# If tput is not available, tests requiring multiline editing would fail.
typeset -si multiline
command -pv tput >/dev/null
if	! let "multiline = ! $?"
then	warning "tput(1) not available on default path; tests that require multiline editing are skipped"
fi

# The --noparallel tests cannot be run in the background because, for
# reasons unclear, doing so disables the effect of Ctrl+C. It is not
# because SIGINT is ignored or because 'stty intr' is set wrong.
#
# Define these in a shell function now to defer them, and run them
# at the end so other tests can at least run in parallel with them.

foreground_tests()
{

tst --noparallel $LINENO <<"!"
L POSIX sh 026(C)

# If the User Portability Utilities Option is supported:  When the
# POSIX locale is specified and a background job is suspended by a
# SIGTSTP signal then the <state> field in the output message is set to
# Stopped, Suspended, Stopped(SIGTSTP) or Suspended(SIGTSTP).

t 5000
d 40
I ^\r?\n$
p :test-1:
w sleep 60 &
u [[:digit:]]\r?\n$
p :test-2:
w kill -TSTP $!
s 100
u (Stopped|Suspended)
p :test-3:
w kill -KILL $!
w wait
s 100
u (Killed|Done)
!

tst --noparallel $LINENO <<"!"
L POSIX sh 028(C)

# If the User Portability Utilities Option is supported:  When the
# POSIX locale is specified and a background job is suspended by a
# SIGTTIN signal then the <state> field in the output message is set to
# Stopped(SIGTTIN) or Suspended(SIGTTIN).

t 5000
d 40
I ^\r?\n$
p :test-1:
w sleep 60 &
u [[:digit:]]\r?\n$
p :test-2:
w kill -TTIN $!
s 100
u (Stopped|Suspended) \(SIGTTIN\)
p :test-3:
w kill -KILL $!
w wait
s 100
u (Killed|Done)
!

tst --noparallel $LINENO <<"!"
L POSIX sh 029(C)

# If the User Portability Utilities Option is supported:  When the
# POSIX locale is specified and a background job is suspended by a
# SIGTTOU signal then the <state> field in the output message is set to
# Stopped(SIGTTOU) or Suspended(SIGTTOU).

t 5000
d 40
I ^\r?\n$
p :test-1:
w sleep 60 &
u [[:digit:]]\r?\n$
p :test-2:
w kill -TTOU $!
s 100
u (Stopped|Suspended) \(SIGTTOU\)
p :test-3:
w kill -KILL $!
w wait
s 100
u (Killed|Done)
!

tst --noparallel $LINENO <<"!"
L suspend a blocked write to a FIFO
# https://github.com/ksh93/ksh/issues/464

t 5000
d 40
p :test-1:
w mkfifo testfifo464; echo >testfifo464
r echo >testfifo464\r?\n$
# untrapped SIGTSTP (Ctrl+Z) should be ineffective here and just print ^Z
c \cZ
r \^Z
# Ctrl+C should interrupt it and trigger an error message
c \cC
s 50
u testfifo464: cannot create \[.*\]\r\n$
p :test-2:
w echo ok
r echo ok\r?\n$
r ^ok\r?\n$
!

tst --noparallel $LINENO <<"!"
L loss of here-doc in loaded function after interrupting a here-doc
# https://github.com/ksh93/ksh/issues/978

t 5000
d 40
p :test-1:
w function fn { cat <<EOF\nthis is a test\nEOF\necho 'HERE-DOC LOST'\n}
p :test-2:
w sleep 3
s 40
c \cC
p :test-3:
w fn
r fn
r ^this is a test\r?\n$
!

tst --noparallel $LINENO <<"!"
L notify job state changes

# 'set -b' should immediately notify the user about job state changes.

t 5000
d 40
p :test-1:
w set -b; sleep .2 &
r set -b; sleep .2 &\r?\n$
u ^\[1\][[:blank:]]+[[:digit:]]+\r?\n$
s 100
u Done
!

} # end of foreground_tests()



# ====== start of tests that can run in parallel ======

tst $LINENO <<"!"
L POSIX sh 091(C)

# If the User Portability Utilities Option is supported and shell
# command line editing is supported:  When in insert mode an entered
# character other than <newline>, erase, interrupt, kill, control-V,
# control-W, backslash \ (followed by erase or kill), end-of-file and
# <ESC> is inserted in the current command line.

d 50
p :test-1:
c echo h
c ell
w o
r echo hello\r?\n$
r ^hello\r?\n$
!

((SHOPT_VSH)) && tst $LINENO <<"!"
L POSIX sh 093(C)

# If the User Portability Utilities Option is supported and shell
# command line editing is supported:  After termination of a previous
# command, sh is entered in insert mode.

d 50
p :test-1:
w echo hello\E
r echo hell
u ^hello\r?\n$
p :test-2:
c echo goo
c dby
w e
r echo goodbye\r?\n$
r ^goodbye\r?\n$
!

((SHOPT_VSH)) && tst $LINENO <<"!"
L POSIX sh 094(C)

# If the User Portability Utilities Option is supported and shell
# command line editing is supported:  When in insert mode an <ESC>
# switches sh into command mode.

d 50
p :test-1:
c echo he
c \E
c a
w llo
r echo hello\r?\n$
r ^hello\r?\n$
!

tst $LINENO <<"!"
L POSIX sh 096(C)

# If the User Portability Utilities Option is supported and shell
# command line editing is supported:  When in command mode the
# interrupt character causes sh to terminate command line editing on
# the current command line, re-issue the prompt on the next line of the
# terminal and to reset the command history so that the command that
# was interrupted is not entered in the history.

d 50
I ^\r?\n$
p :test-1:
w echo first
p :test-2:
w stty intr ^C
p :test-3:
c echo bad\E
s 40
c \cC
w echo scrambled
p :test-4:
w history
u echo first
r stty intr \^C
r echo
r history
!

tst $LINENO <<"!"
L POSIX sh 097(C)

# If the User Portability Utilities Option is supported and shell
# command line editing is supported:  When in insert mode a <newline>
# causes the current command line to be executed.

d 50
c echo ok\n
u ^ok\r?\n$
!

tst $LINENO <<"!"
L POSIX sh 099(C)

# If the User Portability Utilities Option is supported and shell
# command line editing is supported:  When in insert mode the interrupt
# character causes sh to terminate command line editing on the current
# command line, re-issue the prompt on the next line of the terminal
# and to reset the command history so that the command that was
# interrupted is not entered in the history.

d 50
I ^\r?\n$
p :test-1:
w echo first
u ^first
p :test-2:
w stty intr ^C
r
p :test-3:
c echo bad\cC
w echo last
p :test-4:
w history
u echo first
r stty intr \^C
r echo last
r history
!

tst $LINENO <<"!"
L POSIX sh 100(C)

# If the User Portability Utilities Option is supported and shell
# command line editing is supported:  When in insert mode the kill
# character clears all the characters from the input line.

t 10000
d 50
p :test-1:
w stty kill ^X
p :test-2:
c echo bad\cX
w echo ok
u ^ok\r?\n$
!

((SHOPT_VSH || SHOPT_ESH)) && tst $LINENO <<"!"
L POSIX sh 101(C)

# If the User Portability Utilities Option is supported and shell
# command line editing is supported:  When in insert mode a control-V
# causes the next character to be inserted even in the case that the
# character is a special insert mode character.
# Testing Requirements: The assertion must be tested with at least the
# following set of characters: <newline>, erase, interrupt, kill,
# control-V, control-W, end-of-file, backslash \ (followed by erase or
# kill) and <ESC>.

t 10000
d 50
p :test-1:
w stty erase ^H intr ^C kill ^X
p :test-2:
w echo erase=:\cV\cH:
u ^erase=:\r?\n$
p :test-3:
w echo kill=:\cV\cX:
u ^kill=:\cX:\r?\n$
p :test-4:
w echo control-V=:\cV\cV:
u ^control-V=:\cV:\r?\n$
p :test-5:
w echo control-W:\cV\cW:
u ^control-W:\cW:\r?\n$
p :test-6:
w echo EOF=:\cV\cD:
u ^EOF=:\004:\r?\n$
p :test-7:
w echo backslash-erase=:\\\cH:
u ^backslash-erase=:\r?\n$
p :test-8:
w echo backslash-kill=:\\\cX:
u ^backslash-kill=:\cX:\r?\n$
p :test-9:
w echo ESC=:\cV\E:
u ^ESC=:\E:\r?\n$
p :test-10:
w echo interrupt=:\cV\cC:
u ^interrupt=:\cC:\r?\n$
!

tst $LINENO <<"!"
L POSIX sh 104(C)

# If the User Portability Utilities Option is supported and shell
# command line editing is supported:  When in insert mode an
# end-of-file at the beginning of an input line is interpreted as the
# end of input.

d 50
p :test-1:
w trap 'echo done >&2' EXIT
p :test-2:
s 100
c \cD
u ^done\r?\n$
!

((SHOPT_VSH)) && tst $LINENO <<"!"
L POSIX sh 111(C)

# If the User Portability Utilities Option is supported and shell
# command line editing is supported:  When in command mode, # inserts
# the character # at the beginning of the command line and causes the
# line to be treated as a comment and the line is entered in the
# command history.

d 50
p :test-1:
c echo save\E
s 40
c #
p :test-2:
w history
u #echo save
r history
!

((SHOPT_VSH)) && tst $LINENO <<"!"
L POSIX sh 251(C)

# If the User Portability Utilities Option is supported and shell
# command line editing is supported:  When in command mode, then the
# command N repeats the most recent / or ? command, reversing the
# direction of the search.

d 50
p :test-1:
w echo repeat-1
u ^repeat-1\r?\n$
p :test-2:
w echo repeat-2
u ^repeat-2\r?\n$
p :test-3:
s 100
c \E
s 40
w /rep
u echo repeat-2
c n
r echo repeat-1
c N
r echo repeat-2
w dd
p :test-3:
w echo repeat-3
u ^repeat-3\r?\n$
p :test-4:
s 100
c \E
s 40
w ?rep
r echo repeat-2
c N
r echo repeat-1
c n
r echo repeat-2
c n
r echo repeat-3
!

# This test freezes the 'less' pager on OpenBSD, which is not a ksh bug.
: <<\disabled
whence -q less &&
TERM=vt100 tst $LINENO <<"!"
L process/terminal group exercise

d 50
w m=yes; while true; do echo $m-$m; done | less
u :$|:\E|lines
c \cZ
r Stopped
w fg
u yes-yes
!
disabled

# Test file name completion in vi mode
if((SHOPT_VSH)) && mkdir "/tmp/fakehome_$$" 2>/dev/null; then
tst $LINENO <<!
L vi mode file name completion

# Completing a file name in vi mode that contains '~' and has a
# base name the same length as the home directory's parent directory
# shouldn't fail.

d 50
p :test-1:
w set -o vi; HOME=/tmp/fakehome_$$; touch ~/testfile_$$
r \r?\n$
p :test-2:
c echo ~/tes
w \t
u ^/tmp/fakehome_$$/testfile_$$\r?\n$
p :test-3:
w rm -rf /tmp/fakehome_$$
!
fi # SHOPT_VSH

VISUAL='' tst $LINENO <<"!"
L raw Bourne mode literal tab characters

# With wide characters (e.g. UTF-8) disabled, raw mode is handled by ed_read()
# in edit.c; it does not expand tab characters on the command line.
# With wide characters enabled, and if vi mode is compiled in, raw mode is
# handled by ed_viread() in vi.c (even though vi mode is off); it expands tab
# characters to spaces on the command line. See slowread() in io.c.

d 50
p :test-1:
w set +o emacs 2>/dev/null
p :test-2:
w true /de\tv/nu\tl\tl
r true (/de\tv/nu\tl\tl|/de       v/nu    l       l)\r?\n$
p :test-3:
!

VISUAL='' tst $LINENO <<"!"
L raw Bourne mode backslash handling

# The escaping backslash feature should be disabled in the raw Bourne mode.
# This is tested with both erase and kill characters.

d 50
p :test-1:
w set +o emacs 2>/dev/null
p :test-2:
w stty erase ^H kill ^X
p :test-3:
w true string\\\\\cH\cH
r true string\r?\n$
p :test-4:
w true incorrect\\\cXtrue correct
r true correct\r?\n$
!

set --
((SHOPT_VSH)) && set -- "$@" vi
((SHOPT_ESH)) && set -- "$@" emacs gmacs
for mode do
# NOTE: here-doc delimiter not quoted; expansions and backslash-escaping in effect
VISUAL=$mode tst $LINENO << !
L escaping backslashes in $mode mode

# Backslashes should only be escaped if the previous input was a backslash.
# Other backslashes stored in the input buffer should be erased normally.

d 50
p :test-1:
w stty erase ^H
r stty erase \^H\r?\n$
p :test-2:
c true string\\\\\\\\
c \\cH
c \\cH
w \\cH
r true string\\r?\\n\$
!
done

# Tests for 'test -t'. These were moved here from bracket.sh because they require a tty.
cat >test_t.sh <<"EOF"
integer n
redirect {n}< /dev/tty
[[ -t $n ]] && echo OK0 || echo "[[ -t n ]] fails when n > 9"
# _____ Verify that [ -t 1 ] behaves sensibly inside a command substitution.
#	This is the simple case that doesn't do any redirection of stdout within
#	the command substitution. Thus the [ -t 1 ] test should be false.
expect=$'begin\nend'
actual=$(echo begin; [ -t 1 ] || test -t 1 || [[ -t 1 ]] && echo -t 1 is true; echo end)
[[ $actual == "$expect" ]] && echo OK1 || echo 'test -t 1 in comsub fails' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
actual=$(echo begin; [ -n X -a -t 1 ] || test -n X -a -t 1 || [[ -n X && -t 1 ]] && echo -t 1 is true; echo end)
[[ $actual == "$expect" ]] && echo OK2 || echo 'test -t 1 in comsub fails (compound expression)' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
# Same for the ancient compatibility hack for 'test -t' with no arguments.
actual=$(echo begin; [ -t ] || test -t && echo -t is true; echo end)
[[ $actual == "$expect" ]] && echo OK3 || echo 'test -t in comsub fails' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
actual=$(echo begin; [ -n X -a -t ] || test -n X -a -t && echo -t is true; echo end)
[[ $actual == "$expect" ]] && echo OK4 || echo 'test -t in comsub fails (compound expression)' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
#	This is the more complex case that does redirect stdout within the command
#	substitution to the actual tty. Thus the [ -t 1 ] test should be true.
actual=$(echo begin; exec >/dev/tty; [ -t 1 ] && test -t 1 && [[ -t 1 ]]) \
&& echo OK5 || echo 'test -t 1 in comsub with exec >/dev/tty fails'
actual=$(echo begin; exec >/dev/tty; [ -n X -a -t 1 ] && test -n X -a -t 1 && [[ -n X && -t 1 ]]) \
&& echo OK6 || echo 'test -t 1 in comsub with exec >/dev/tty fails (compound expression)'
# Same for the ancient compatibility hack for 'test -t' with no arguments.
actual=$(echo begin; exec >/dev/tty; [ -t ] && test -t) \
&& echo OK7 || echo 'test -t in comsub with exec >/dev/tty fails'
actual=$(echo begin; exec >/dev/tty; [ -n X -a -t ] && test -n X -a -t) \
&& echo OK8 || echo 'test -t in comsub with exec >/dev/tty fails (compound expression)'
# The broken ksh2020 fix for [ -t 1 ] (https://github.com/att/ast/pull/1083) caused
# [ -t 1 ] to fail in non-comsub virtual subshells.
( test -t 1 ) && echo OK9 || echo 'test -t 1 in virtual subshell fails'
( test -t ) && echo OK10 || echo 'test -t in virtual subshell fails'
got=$(test -t 1 >/dev/tty && echo ok) && [[ $got == ok ]] && echo OK11 || echo 'test -t 1 in comsub fails'
EOF
tst $LINENO <<"!"
L test -t 1 inside command substitution
d 50
p :test-1:
w . ./test_t.sh
r \. \./test_t\.sh\r?\n$
u ^OK0\r?\n$
r ^OK1\r?\n$
r ^OK2\r?\n$
r ^OK3\r?\n$
r ^OK4\r?\n$
r ^OK5\r?\n$
r ^OK6\r?\n$
r ^OK7\r?\n$
r ^OK8\r?\n$
r ^OK9\r?\n$
r ^OK10\r?\n$
r ^OK11\r?\n$
p :test-2:
!

tst $LINENO <<"!"
L race condition while launching external commands

# Test for bug in ksh binaries that use posix_spawn() while job control is active.
# See discussion at: https://github.com/ksh93/ksh/issues/79
#
# The 'r /dev/null' tests are deliberately not anchored as Android/Termux ls(1) insists
# on inserting colouring escape sequences regardless of the TERM or LS_COLORS env vars.

d 50
p :test-1:
w printf '%s\\n' 1 2 3 4 5 | while read; do ls /dev/null; done
r printf '%s\\n' 1 2 3 4 5 | while read; do ls /dev/null; done\r?\n$
r /dev/null
r /dev/null
r /dev/null
r /dev/null
r /dev/null
p :test-2:
!

((SHOPT_ESH)) && [[ -o ?backslashctrl ]] && tst $LINENO <<"!"
L nobackslashctrl in emacs

d 50
p :test-1:
w set -o emacs --nobackslashctrl

# --nobackslashctrl shouldn't be ignored by reverse search
p :test-2:
w \cR\\\cH\cH
r ^:test-2: \r?\n$
!

((SHOPT_ESH)) && tst $LINENO <<"!"
L emacs backslash escaping

d 50
p :test-1:
w set -o emacs

# Test for too many backslash deletions in reverse-search mode
p :test-2:
w \cRset\\\\\\\\\cH\cH\cH\cH\cH
r set -o emacs$

# \ should escape the interrupt character (usually Ctrl+C)
w true \\\cC
r true \^C
!

((SHOPT_VSH)) && touch vi_completion_A_file vi_completion_B_file && tst $LINENO <<"!"
L vi filename completion menu

d 50
p :test-1:
c ls vi_co
c \t
c \t
r ls vi_completion\r?\n$
r ^1) vi_completion_A_file\r?\n$
r ^2) vi_completion_B_file\r?\n$
w 2\t
r ls vi_completion_B_file \r?\n$
r ^vi_completion_B_file\r?\n$

# 93v- bug: tab completion writes past input buffer
# https://github.com/ksh93/ksh/issues/195

# ...reproducer 1
p :test-2:
c ls vi_compl
c \t
c \t
r ls vi_completion\r?\n$
r ^1) vi_completion_A_file\r?\n$
r ^2) vi_completion_B_file\r?\n$
c a
w B_file
r ls vi_completion_B_file\r?\n$
r ^vi_completion_B_file\r?\n$

# ...reproducer 2
p :test-3:
c \r
c ls vi_comple
c \t
c \t
u ls vi_completion\r?\n$
r ^1) vi_completion_A_file\r?\n$
r ^2) vi_completion_B_file\r?\n$
c 0$a
w A_file
r ls vi_completion_A_file\r?\n$
r ^vi_completion_A_file\r?\n$
!

tst $LINENO <<"!"
L syntax error added to history file

# https://github.com/ksh93/ksh/issues/209

d 50
p :test-1:
w do something
u : syntax error: `do' unexpected\r?\n$
p :test-2:
w fc -lN1
s 10
r fc -lN1\r?\n$
s 10
r [[:blank:]]do something\r?\n$
!

tst $LINENO <<"!"
L value of $? after the shell uses a variable with a discipline function

d 50
p :test-1:
w PS1.get() { true; }; PS2.get() { true; }; false
r PS1\.get\(\) \{ true; \}; PS2\.get\(\) \{ true; \}; false\r?\n$
p :test-2:
w echo "Exit status is: $?"
r echo "Exit status is: \$\?"\r?\n$
r ^Exit status is: 1\r?\n$
p :test-3:
w LINES.set() { return 13; }
r LINES.set\(\) \{ return 13; \}\r?\n$
p :test-4:
w echo "Exit status is: $?"
r echo "Exit status is: \$\?"\r?\n$
r ^Exit status is: 0\r?\n$

# It's worth noting that the test below will always fail in ksh93u+ and ksh2020,
# even when $PS2 lacks a discipline function (see https://github.com/ksh93/ksh/issues/117).
# After that bug was fixed the test below could still fail if PS2.get() existed.
p :test-5:
w false
r false
p >
w (
r \(
p >
w exit
r exit
p >
w )
r )
p :test-6:
w echo "Exit status is: $?"
r echo "Exit status is: \$\?"\r?\n$
r ^Exit status is: 1\r?\n$
!

((SHOPT_ESH)) && ((SHOPT_VSH)) && tst $LINENO <<"!"
L crash after switching from emacs to vi mode

# In ksh93r using the vi 'r' command after switching from emacs mode could
# trigger a memory fault: https://bugzilla.opensuse.org/show_bug.cgi?id=179917

d 50
p :test-1:
w exec "$SHELL" -o emacs
r exec "\$SHELL" -o emacs\r?\n$
p :test-2:
w set -o vi
r set -o vi\r?\n$
p :test-3:
c \Erri
w echo Success
r echo Success\r?\n$
r ^Success\r?\n$
!

((SHOPT_VSH || SHOPT_ESH)) && tst $LINENO <<"!"
L value of $? after tilde expansion in tab completion

# Make sure that a .sh.tilde.set discipline function
# cannot influence the exit status.

d 50
p :test-1:
w .sh.tilde.set() { true; }
r \r?\n$
p :test-2:
w HOME=/tmp
r \r?\n$
p :test-3:
c false ~
w \t
u false /tmp
p :test-4:
w echo "Exit status is: $?"
r \r?\n$
u Exit status is: 1
p :test-5:
w (exit 42)
r \r?\n$
p :test-6:
c echo $? ~
w \t
r \r?\n$
u 42 /tmp
!

((SHOPT_MULTIBYTE && (SHOPT_VSH || SHOPT_ESH))) &&
[[ ${LC_ALL:-${LC_CTYPE:-${LANG:-}}} =~ [Uu][Tt][Ff]-?8 ]] &&
touch $'XXX\xc3\xa1' $'XXX\xc3\xab' &&
tst $LINENO <<"!"
L autocomplete should not fill partial multibyte characters
# https://github.com/ksh93/ksh/issues/223

d 50
p :test-1:
w : XX\t
r : XXX\r?\n$
!

((SHOPT_VSH)) && tst $LINENO <<"!"
L Using b, B, w and W commands in vi mode
# https://github.com/att/ast/issues/1467

d 50
p :test-1:
w set -o vi
r set -o vi\r?\n$
p :test-2:
w echo asdf\EbwBWa
r echo asdf\r?\n$
r ^asdf\r?\n$
!

((SHOPT_ESH)) && mkdir -p emacstest/123abc && VISUAL=emacs tst $LINENO <<"!"
L autocomplete stops numeric input
# https://github.com/ksh93/ksh/issues/198

d 50
p :test-1:
c cd emacste
c \t
w 123abc
u cd emacstest/123abc\r?\n$
!

echo '((' >$tmp/synerror
ENV=$tmp/synerror tst $LINENO <<"!"
L syntax error in profile causes exit on startup
# https://github.com/ksh93/ksh/issues/281

d 50
r /synerror: syntax error: `\(' unmatched\r?\n$
p :test-1:
w echo ok
r echo ok\r?\n$
r ^ok\r?\n$
!

((SHOPT_VSH)) && tst $LINENO <<"!"
L split on quoted whitespace when extracting words from command history
# https://github.com/ksh93/ksh/pull/291

d 50
p :test-1:
w true ls One\\ "Two Three"$'Four Five'.mp3
r true ls One\\ "Two Three"\$'Four Five'\.mp3\r?\n$
p :test-2:
w :\E_
u : One\\ "Two Three"\$'Four Five'\.mp3\r?\n$
!

# needs non-dumb terminal for multiline editing
((multiline && SHOPT_VSH)) && TERM=vt100 tst $LINENO <<"!"
L crash when entering comment into history file (vi mode)
# https://github.com/att/ast/issues/798

d 50
p :test-1:
c foo \E#
r #foo\r?\n$
p :test-2:
w hist -lnN 1
r hist -lnN 1\r?\n$
r [[:blank:]]#foo\r?\n$
r [[:blank:]]hist -lnN 1\r?\n$
!

((SHOPT_VSH || SHOPT_ESH)) && tst $LINENO <<"!"
L tab completion while expanding ${.sh.*} variables
# https://github.com/att/ast/issues/1461
# also tests $'...' string: https://github.com/ksh93/ksh/issues/462

d 50
p :test-1:
c test \$'foo\\'bar' \$\{.sh.level
s 50
w \t
s 50
r test \$'foo\\'bar' \$\{.sh.level\}\r?\n$
!

((SHOPT_VSH || SHOPT_ESH)) && tst $LINENO <<"!"
L tab completion executes command substitutions
# https://github.com/ksh93/ksh/issues/268
# https://github.com/ksh93/ksh/issues/462#issuecomment-1038482307

d 50
p :test-1:
c $(echo true)
w \t
r \$\(echo true\)\r?\n$
p :test-2:
c `echo true`
w \t
r `echo true`\r?\n$
p :test-3:
c '`/dev
w \t
r '`/dev[[:blank:]]*\r?\n$
# escape from PS2 prompt with Ctrl+C
r ^> $
c \cC
p :test-4:
c '$(/dev
w \t
r '\$\(/dev[[:blank:]]*\r?\n$
r ^> $
c \cC
p :test-5:
c $'`/dev
w \t
r \$'`/dev[[:blank:]]*\r?\n$
!

# needs non-dumb terminal for multiline editing
((multiline && SHOPT_ESH)) && VISUAL=emacs TERM=vt100 tst $LINENO <<"!"
L emacs: keys with repeat parameters repeat extra steps
# https://github.com/ksh93/ksh/issues/292

d 50
p :test-1:
c : foo bar delete add
w \1\6\6\E3\Ed
r :  add\r?\n$
p :test-2:
c : foo bar delete add
w \E3\Eh
r : foo \r?\n$
p :test-3:
c : test_string
w \1\6\6\E3\E[3~
r : t_string\r?\n$
p :test-4:
c : test_string
w \1\E6\E[C\4
r : teststring\r?\n$
!

tst $LINENO <<"!"
L crash with KEYBD trap after entering multi-line command substitution
# https://www.mail-archive.com/ast-users@lists.research.att.com/msg00313.html

d 50
p :test-1:
w trap : KEYBD
w : $(
w true); echo "Exit status is $?"
u Exit status is 0
!

tst $LINENO <<"!"
L assigning an array to .sh.edtext within a KEYBD trap
# https://github.com/ksh93/ksh/issues/1058

d 50
p :test-1:
w trap '.sh.edtext=(x)' KEYBD
r KEYBD\r?\n$
p :test-2:
w echo ok
r echo ok\r?\n$
r ^ok\r?\n$
!

tst $LINENO <<"!"
L interrupted PS2 discipline function
# https://github.com/ksh93/ksh/issues/347

d 50
p :test-1:
w PS2.get() { trap --bad-option 2>/dev/null; .sh.value="NOT REACHED"; }
p :test-2:
w echo \$\(
w echo one \\
w two three
w echo end
w \)
u ^one two three end\r?\n$
!

((SHOPT_VSH || SHOPT_ESH)) && tst $LINENO <<"!"
L tab completion of '.' and '..'
# https://github.com/ksh93/ksh/issues/372

d 50

# typing '.' followed by two tabs should show a menu that includes "number) ../"
p :test-1:
c : .
c \t
w \t
u ) \.\./\r?\n$

# typing '..' followed by a tab should complete to '../' (as it is
# known that there are no files starting with '..' in the test PWD)
p :test-2:
c : ..
w \t
u : \.\./\r?\n$
!

((SHOPT_VSH)) && tst $LINENO <<"!"
L Ctrl+C with SIGINT ignored
# https://github.com/ksh93/ksh/issues/343
# Fix improved on 2024-02-12.
# Note: without emacs, the Ctrl+C should be echoed as ^C.

d 50

# SIGINT ignored by child
p :test-1:
w PS1=':child-!: ' "$SHELL"
p :child-1:
w trap '' INT
p :child-2:
w : lorem\cCipsum
r : lorem\^Cipsum\r?\n$
w exit

# SIGINT ignored by parent
p :test-2:
w (trap '' INT; ENV=/./dev/null PS1=':child-!: ' "$SHELL")
p :child-1:
w : lorem\cCipsum
r : lorem\^Cipsum\r?\n$
w exit

# SIGINT ignored by parent, trapped in child
p :test-3:
w (trap '' INT; ENV=/./dev/null PS1=':child-!: ' "$SHELL")
p :child-1:
w trap 'echo test' INT
p :child-2:
w : lorem\cCipsum
r : lorem\^Cipsum\r?\n$
w exit
!

touch "$tmp/foo bar"
# needs non-dumb terminal for multiline editing
# NOTE: here-doc delimiter not quoted; expansions and backslash-escaping in effect
((multiline && (SHOPT_VSH || SHOPT_ESH))) && TERM=vt100 tst $LINENO <<!
L tab completion with space in string and -o noglob on
# https://github.com/ksh93/ksh/pull/413
# Amended to test that completion keeps working after -o noglob

d 50
p :test-1:
w set -o noglob
r \\r?\\n\$
p :test-2:
# NOTE: the following line must end in a space
c echo $tmp/foo\\\\ 
w \\t
r echo $tmp/foo\\\\ bar \\r?\\n$
r ^$tmp/foo bar\\r?\\n$
!

((SHOPT_HISTEXPAND)) && tst $LINENO <<"!"
L history expansion of an out-of-range event

d 50
p :test-1:
w set -H
r set -H\r?\n$
p :test-2:
w echo "!99"
r !99
r : !99: event not found\r?\n$
!

# TODO: fails too often on github runners; 'set -b' must still
# have a race condition when several jobs terminate all at once
: <<\DISABLED
tst $LINENO <<"!"
L --notify does not report all simultaneously terminated jobs

d 50
p :test-1:
w set -b; sleep .1 & sleep .1 & sleep .1 &
u Done
u Done
u Done
!
DISABLED

((SHOPT_HISTEXPAND)) && tst $LINENO <<"!"
L history expansion: history comment character stops line from being processed
# https://github.com/ksh93/ksh/issues/513

d 50
p :test-1:
w set -H
p :test-2:
w true ${#v} !non_existent
u : !non_existent: event not found
w histchars='!^@'
p :test-3:
w true \\@ !non_existent
u : !non_existent: event not found
p :test-4:
w echo @ !non_existent
u @ !non_existent\r?\n$
!

((SHOPT_VSH)) && tst $LINENO <<"!"
L reverse search isn't canceled after an interrupt in vi mode
# note: must unignore SIGINT with undocumented 'trap + INT'

d 50
p :test-1:
w trap + INT; echo WRONG
u ^WRONG\r?\n$
p :test-2:
w print CORREC
u ^CORREC\r?\n$
p :test-3:
w echo foo
u ^foo\r?\n$
p :test-4:
w sleep 0
r sleep 0\r?\n$
p :test-5:
c e\E[A\E[A\cC\E[A\E[A\E[A
w $aT
r :test-5:
r :test-5: print CORRECT
!

((SHOPT_ESH)) && VISUAL=emacs tst $LINENO <<"!"
L failure to start new reverse search in emacs mode
# https://github.com/ksh93/ksh/commit/b5e52703

d 50
p :test-1:
w echo WRON
p :test-2:
w print CORREC
p :test-3:
w e\E[AG
u WRONG
p :test-4:
w p\E[AT
u CORRECT
!

((SHOPT_VSH)) && tst $LINENO <<"!"
L backwards reverse search in vi mode

d 50
p :test-1:
w print bar
p :test-2:
w print foo
p :test-3:
w echo WRONG
p :test-4:
w print Correc
p :test-5:
c p\E[A\E[A\E[A\E[B\E[B
w $at
u Correct
!

((SHOPT_ESH)) && VISUAL=emacs tst $LINENO <<"!"
L backwards reverse search in emacs mode

d 50
p :test-1:
w print bar
p :test-2:
w print foo
p :test-3:
w echo WRONG
p :test-4:
w print Correc
p :test-5:
c p\E[A\E[A\E[A\E[B\E[B
w t
u Correct
!

# needs non-dumb terminal for multiline editing
((multiline && SHOPT_ESH)) && mkdir -p fullcomplete/foe && VISUAL=emacs TERM=vt100 tst $LINENO <<"!"
L full-word completion in emacs mode
# https://github.com/ksh93/ksh/pull/580

d 50
p :test-1:
c true fullcomplete/foi
w \cb\t
r true fullcomplete/foi\r?\n
p :test-2:
c true fullcomplete/foi
w \cb\E=
r true fullcomplete/foi\r?\n
p :test-3:
c true fullcomplete/foi
w \cb\E*
r true fullcomplete/foi\r?\n
!

# needs non-dumb terminal for multiline editing
((multiline && SHOPT_VSH)) && mkdir -p fullcomplete/fov && VISUAL=vi TERM=vt100 tst $LINENO <<"!"
L full-word completion in vi mode
# https://github.com/ksh93/ksh/pull/580

d 50
p :test-1:
c true fullcomplete/foi
w \Eh\\a
r true fullcomplete/foi\r?\n
p :test-2:
c true fullcomplete/foi
w \Eh=a
r true fullcomplete/foi\r?\n
p :test-3:
c true fullcomplete/foi
w \Eh*a
r true fullcomplete/foi\r?\n
!

((SHOPT_VSH && SHOPT_MULTIBYTE)) &&
[[ ${LC_ALL:-${LC_CTYPE:-${LANG:-}}} =~ [Uu][Tt][Ff]-?8 ]] &&
mkdir -p vitest/aあb && TERM=vt100 tst $LINENO <<"!"
L vi completion from wide produces corrupt characters
# https://github.com/ksh93/ksh/issues/571

d 50
p :test-1:
w cd vitest/aあ\t
r cd vitest/aあb/\r?\n$
!

((SHOPT_HISTEXPAND && (SHOPT_VSH || SHOPT_ESH))) &&
mkdir -p 'chrtest/aa#b' && tst $LINENO <<"!"
L tab-completing with first histchar

d 50
p :test-1:
w histchars='#^!'
r \r?\n$
p :test-2:
w set +o histexpand
r \r?\n$
p :test-3:
c ls chrtest/a
w \t
r ls chrtest/aa#b/\r?\n$
p :test-4:
w set -o histexpand
r \r?\n$
p :test-5:
c ls chrtest/a
w \t
r ls chrtest/aa\\#b/\r?\n$
p :test-6:
w unset histchars
!

((SHOPT_HISTEXPAND && (SHOPT_VSH || SHOPT_ESH))) &&
mkdir -p 'chrtest2/@a@b' && tst $LINENO <<"!"
L tab-completing with third histchar

d 50
p :test-1:
w histchars='!^@'
r \r?\n$
p :test-2:
w cd chrtest2
r \r?\n$
p :test-3:
w set +o histexpand
r \r?\n$
p :test-4:
# NOTE: the following line must end in a space
c ls 
w \t
r ls @a@b/\r?\n$
p :test-5:
w set -o histexpand
r \r?\n$
p :test-6:
# NOTE: the following line must end in a space
c ls 
w \t
r ls \\@a@b/\r?\n$
!

((SHOPT_VSH || SHOPT_ESH)) &&
mkdir -p 'chrtest3/~ab' && tst $LINENO <<"!"
L tab-completing with escaped ~

d 50
p :test-1:
w cd chrtest3; .sh.tilde.get() { print -n WRONG_TILDE_EXPANSION >&2; };
r \r?\n$
p :test-2:
c ls \\~
w \t
r ls \\~ab/\r?\n$
p :test-3:
c ls '~
w \t
r ls \\~ab/\r?\n$
p :test-4:
c ls "~
w \t
r ls \\~ab/\r?\n$
p :test-5:
c ls $'~
w \t
r ls \\~ab/\r?\n$
!

((SHOPT_VSH || SHOPT_ESH)) &&
chmod +x cmd_complete_me >cmd_complete_me &&
PATH=.:$PATH tst $LINENO <<"!"
L command completion after init and after TERM change
# https://github.com/ksh93/ksh/issues/642

d 50
p :test-1:
c cmd_complet
w \t
r cmd_complete_me \r?\n$
# also try after TERM change
p :test-2:
w TERM=ansi
r \r?\n$
p :test-3:
c cmd_complet
w \t
r cmd_complete_me \r?\n$
!

((SHOPT_VSH || SHOPT_ESH)) &&
echo "function _ksh_93u_m_cmdcomplete_test_ { echo RUN; }" > _ksh_93u_m_cmdcomplete_test_ &&
FPATH=$PWD tst $LINENO <<"!"
L function and builtin completion when a function is undefined
# https://github.com/ksh93/ksh/issues/650

d 50
p :test-1:
w _ksh_93u_m_cmdcomplete_test_; autoload _a_nonexistent_function_
u ^RUN\r?\n$
p :test-2:
c _ksh_93u_m_cmdcompl
w \t
r :test-2: _ksh_93u_m_cmdcomplete_test_ \r?\n$
!

tst $LINENO <<"!"
L terminate interactive shell using the kill built-in

d 50
p :test-1:
w PS1=':child-!: ' "$SHELL"
p :child-1:
w kill -s HUP \$\$
r kill -s HUP \$\$\r?\n$
r ^Hangup\r?\n$
!

((SHOPT_VSH)) && tst $LINENO <<"!"
L 'read -s' reads from history file on first go

d 50
p :test-1:
w "$SHELL" -o vi -c 'read -s "foo?:prompt: "'
p :prompt:
c \Ek
r ^:prompt: "\$SHELL" -o vi -c 'read -s "foo\?:prompt: "'$
!

tst $LINENO <<"!"
L crash when attempting to cancel a heredoc in an interactive shell
# https://github.com/ksh93/ksh/pull/721

d 50
p :test-1:
w "$SHELL"
p :test-2:
w cat << EOS
p :test-3:
w \cD
p :test-4:
w print Exit status $?
u ^Exit status 0\r?\n$
!

cat >issue346.sh <<"EOF"
PS1.get() {; printf '$ '; trap --invalid-flag 2>/dev/null; }
PS2.get() {; printf '> '; trap --invalid-flag 2>/dev/null; }
.sh.tilde.set() {
	case ${.sh.value} in
	'~ret') .sh.value='Exit status is' ;;
	esac
	trap --invalid-flag 2>/dev/null
}
EOF
tst $LINENO << "!"
L crash when discipline functions exit with an error
# https://github.com/ksh93/ksh/issues/346

d 50
p :test-1:
w "$SHELL"
r "\$SHELL"\r?\n$
p :test-2:
w . ./issue346.sh
r \. \./issue346\.sh\r?\n$
p :test-3:
w echo ~
r echo ~\r?\n$
# HOME should be set to the temp dir defined in shtests, which contains at least this component
r /ksh93\.shtests\.
p :test-4:
w echo ~ret
r echo ~ret\r?\n$
r ^Exit status is\r?\n$
p :test-5:
w echo ~
r echo ~\r?\n$
# HOME should be set to the temp dir defined in shtests, which contains at least this component
r /ksh93\.shtests\.
p :test-6:
w echo ~ret $?
r echo ~ret \$\?\r?\n$
r ^Exit status is 0\r?\n$
!

((multiline && (SHOPT_VSH || SHOPT_ESH))) && TERM=vt100 tst $LINENO <<"!"
L crash when TERM is undefined
# https://github.com/ksh93/ksh/issues/722

d 50
p :test-1:
w unset TERM
r unset TERM\r?\n$
p :test-2:
w "$SHELL"
r "\$SHELL"\r?\n$
p :test-3:
w print Exit status $?
r print Exit status \$\?\r?\n$
r ^Exit status 0\r?\n$
!

((SHOPT_VSH)) && tst $LINENO <<"!"
L 'k' skips over history entries starting with whitespace
# https://github.com/ksh93/ksh/issues/799

d 50
p :test-1:
w   true
p :test-2:
c \Ek
r :test-2:   true
!

((SHOPT_ESH)) && VISUAL=emacs tst $LINENO <<"!"
L emacs: repeat count sticks after ESC commands
# a bug introduced on 2020-09-17 and fixed on 2024-12-21

d 50
# The 'P' command sets an automatic 'p' before every 'w', delaying writing until a match is read.
P :test-.:
w false bad rigt wrong
# Insert 'print '; recall 3rd word 'rigt' (ESC 3 ESC _); cursor back one position (^B); insert 'h'.
# (With the bug, the repeat count of 3 sticks and ^B goes back 3 positions, resulting in 'rhigt'.)
w print \E3\E_\cBh
I print
r right
!

bintrue=$bintrue tst --noparallel $LINENO << "!"
L crash after E2BIG due to failed tcpgrp restoration

t 10000
d 50
p :test-1:
w integer savpid=$$
r \r?\n$
p :test-2:
w "$SHELL"
r \r?\n$
p :test-3:
w "$bintrue" "$(awk -v ORS= 'BEGIN { for(i=0;i<1000000;i++) print "xx"; }')"
r \r?\n$
p :test-4:
w ((savpid==$$)); print $?
u ^1\r?\n$
!

tst $LINENO <<"!"
L getopts aborts execution of command line
# https://github.com/ksh93/ksh/issues/978#issuecomment-4306610198

d 50
p :test-1:
w function fn { while getopts x o; do :; done; }; fn --help; echo OK/CONTINUED/1
u ^OK/CONTINUED/1\r?\n$
p :test-2:
w set -- --help; OPTIND=1; while getopts x o; do :; done; echo OK/CONTINUED/2
u ^OK/CONTINUED/2\r?\n$
!

tst $LINENO <<"!"
L suspend a pipeline whose last element reads from it
# https://github.com/ksh93/ksh/issues/750

d 50
p :test-1:
w sleep 10 | read v
r \r?\n$
c \cZ
s 100
u (Stopped|Suspended)
p :test-2:
w { sleep 10; } | read var
r \r?\n$
c \cZ
s 100
u (Stopped|Suspended)
p :test-3:
w echo OK
u ^OK
!

tst $LINENO <<"!"
L suspend two consecutive pipelines
# https://github.com/ksh93/ksh/issues/750

d 50
p :test-1:
w sleep 10 | sleep 20
r \r?\n$
c \cZ
s 500
u (Stopped|Suspended)
p :test-2:
w { sleep 10; } | sleep 20
r \r?\n$
c \cZ
s 500
u (Stopped|Suspended)
p :test-3:
w echo OK
u ^OK
!

((SHOPT_VSH)) && tst $LINENO <<"!"
L prompt width with CSI intermediate bytes

d 50
p :test-1:
w COLUMNS=20 PS1=$':child\\E[ q\\E[6 q: ' "$SHELL" +o multiline
p :child
c : abcdefg
w
r ^:child.*: : abcdefg\r\n$
!

((SHOPT_MULTIBYTE && SHOPT_ESH)) &&
[[ ${LC_ALL:-${LC_CTYPE:-${LANG:-}}} =~ [Uu][Tt][Ff]-?8 ]] &&
VISUAL=emacs tst $LINENO << "!"
L emacs mode not aware of multibyte spaces
# https://github.com/ksh93/ksh/pull/1037

d 50
p :test-1:
w echo foo　baz\Eb\Edbar
u ^foo　bar\r\n$
!

((SHOPT_MULTIBYTE && SHOPT_VSH)) &&
[[ ${LC_ALL:-${LC_CTYPE:-${LANG:-}}} =~ [Uu][Tt][Ff]-?8 ]] &&
VISUAL=vi tst $LINENO << "!"
L vi mode not aware of multibyte spaces
# https://github.com/ksh93/ksh/pull/1037

d 50
p :test-1:
w echo bar　uux\Ebiq
u ^bar　quux\r\n$
!

# ^^^^^^ ADD NEW TESTS ABOVE THIS LINE ^^^^^^

foreground_tests

# Check results of parallel tests
wait
for ((i=1; i<=bgtests; i++))
do	chk < pty_$i.out
done

# ======
exit $((Errors<125?Errors:125))

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

# These are tests for the interactive shell, run in a pseudoterminal utility
# called 'pty', which allows for scripting interactive sessions and which is
# installed in arch/*/bin while building. To understand these tests, first
# read the pty manual by running: arch/*/bin/pty --man
#
# Do not globally set the locale; these tests must pass for all locales.
#
# The # err_exit # comments are to enable shtests to count the tests.

# the trickiest part of the tests is avoiding typeahead
# in the pty dialogue

whence -q pty || { warning "pty command not found -- tests skipped"; exit 0; }
case $(uname -s) in
Darwin | FreeBSD | Linux )
	;;
* )	warning "pty not confirmed to work correctly on this system -- tests skipped"
	exit 0 ;;
esac

# On some systems, the stty command does not appear to work correctly on a pty pseudoterminal.
# To avoid false regressions, we have to set 'erase' and 'kill' on the real terminal.
if	test -t 0 2>/dev/null </dev/tty && stty_restore=$(stty -g </dev/tty)
then	trap 'stty "$stty_restore" </dev/tty' EXIT  # note: on ksh, the EXIT trap is also triggered for termination due to a signal
	stty erase ^H kill ^X
else	warning "cannot set tty state -- tests skipped"
	exit 0
fi

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

if	[[ -o xtrace ]]
then	debug=--debug=1
else	debug=
fi

function tst
{
	integer lineno=$1 offset
	typeset text

	pty $debug --dialogue --messages='/dev/fd/1' $SHELL |
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
export PS1=':test-!: ' PS2='> ' PS4=': ' ENV=/./dev/null EXINIT= HISTFILE= TERM=dumb

if	! pty $bintrue < /dev/null
then	warning "pty command hangs on $bintrue -- tests skipped"
	exit 0
fi

# err_exit #
tst $LINENO <<"!"
L POSIX sh 026(C)

# If the User Portability Utilities Option is supported:  When the
# POSIX locale is specified and a background job is suspended by a
# SIGTSTP signal then the <state> field in the output message is set to
# Stopped, Suspended, Stopped(SIGTSTP) or Suspended(SIGTSTP).

d 15
I ^\r?\n$
p :test-1:
w sleep 60 &
u [[:digit:]]\r?\n$
s 100
p :test-2:
w kill -TSTP $!
u (Stopped|Suspended)
p :test-3:
w kill -KILL $!
w wait
u (Killed|Done)
!

# err_exit #
tst $LINENO <<"!"
L POSIX sh 028(C)

# If the User Portability Utilities Option is supported:  When the
# POSIX locale is specified and a background job is suspended by a
# SIGTTIN signal then the <state> field in the output message is set to
# Stopped(SIGTTIN) or Suspended(SIGTTIN).

I ^\r?\n$
p :test-1:
w sleep 60 &
u [[:digit:]]\r?\n$
s 100
p :test-2:
w kill -TTIN $!
u (Stopped|Suspended) \(SIGTTIN\)
p :test-3:
w kill -KILL $!
w wait
u (Killed|Done)
!

# err_exit #
tst $LINENO <<"!"
L POSIX sh 029(C)

# If the User Portability Utilities Option is supported:  When the
# POSIX locale is specified and a background job is suspended by a
# SIGTTOU signal then the <state> field in the output message is set to
# Stopped(SIGTTOU) or Suspended(SIGTTOU).

I ^\r?\n$
p :test-1:
w sleep 60 &
u [[:digit:]]\r?\n$
s 100
p :test-2:
w kill -TTOU $!
u (Stopped|Suspended) \(SIGTTOU\)
p :test-3:
w kill -KILL $!
w wait
u (Killed|Done)
!

# err_exit #
tst $LINENO <<"!"
L POSIX sh 091(C)

# If the User Portability Utilities Option is supported and shell
# command line editing is supported:  When in insert mode an entered
# character other than <newline>, erase, interrupt, kill, control-V,
# control-W, backslash \ (followed by erase or kill), end-of-file and
# <ESC> is inserted in the current command line.

d 15
c echo h
c ell
w o
u ^hello\r?\n$
!

# err_exit #
((SHOPT_VSH)) && tst $LINENO <<"!"
L POSIX sh 093(C)

# If the User Portability Utilities Option is supported and shell
# command line editing is supported:  After termination of a previous
# command, sh is entered in insert mode.

w echo hello\E
u ^hello\r?\n$
c echo goo
c dby
w e
u ^goodbye\r?\n$
!

# err_exit #
((SHOPT_VSH)) && tst $LINENO <<"!"
L POSIX sh 094(C)

# If the User Portability Utilities Option is supported and shell
# command line editing is supported:  When in insert mode an <ESC>
# switches sh into command mode.

c echo he\E
s 400
w allo
u ^hello\r?\n$
!

if	[[ $(id -u) == 0 ]]
then	warning "running as root: skipping test POSIX sh 096(C)"
else
# err_exit #
tst $LINENO <<"!"
L POSIX sh 096(C)

# If the User Portability Utilities Option is supported and shell
# command line editing is supported:  When in command mode the
# interrupt character causes sh to terminate command line editing on
# the current command line, re-issue the prompt on the next line of the
# terminal and to reset the command history so that the command that
# was interrupted is not entered in the history.

I ^\r?\n$
p :test-1:
w echo first
p :test-2:
w stty intr ^C
p :test-3:
c echo bad\E
s 400
c \cC
w echo scrambled
p :test-4:
w history
u echo first
r stty intr \^C
r echo
r history
!
fi

# err_exit #
tst $LINENO <<"!"
L POSIX sh 097(C)

# If the User Portability Utilities Option is supported and shell
# command line editing is supported:  When in insert mode a <newline>
# causes the current command line to be executed.

d 15
c echo ok\n
u ^ok\r?\n$
!

if	[[ $(id -u) == 0 ]]
then	warning "running as root: skipping test POSIX sh 099(C)"
else
# err_exit #
tst $LINENO <<"!"
L POSIX sh 099(C)

# If the User Portability Utilities Option is supported and shell
# command line editing is supported:  When in insert mode the interrupt
# character causes sh to terminate command line editing on the current
# command line, re-issue the prompt on the next line of the terminal
# and to reset the command history so that the command that was
# interrupted is not entered in the history.

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
fi

# err_exit #
tst $LINENO <<"!"
L POSIX sh 100(C)

# If the User Portability Utilities Option is supported and shell
# command line editing is supported:  When in insert mode the kill
# character clears all the characters from the input line.

p :test-1:
w stty kill ^X
p :test-2:
c echo bad\cX
w echo ok
u ^ok\r?\n$
!

# err_exit #
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

d 15
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

# err_exit #
tst $LINENO <<"!"
L POSIX sh 104(C)

# If the User Portability Utilities Option is supported and shell
# command line editing is supported:  When in insert mode an
# end-of-file at the beginning of an input line is interpreted as the
# end of input.

p :test-1:
w trap 'echo done >&2' EXIT
p :test-2:
s 100
c \cD
u ^done\r?\n$
!

if	[[ $(id -u) == 0 ]]
then	warning "running as root: skipping test POSIX sh 111(C)"
else
# err_exit #
((SHOPT_VSH)) && tst $LINENO <<"!"
L POSIX sh 111(C)

# If the User Portability Utilities Option is supported and shell
# command line editing is supported:  When in command mode, # inserts
# the character # at the beginning of the command line and causes the
# line to be treated as a comment and the line is entered in the
# command history.

p :test-1:
c echo save\E
s 400
c #
p :test-2:
w history
u #echo save
r history
!
fi

if	[[ $(id -u) == 0 ]]
then	warning "running as root: skipping test POSIX sh 251(C)"
else
# err_exit #
((SHOPT_VSH)) && tst $LINENO <<"!"
L POSIX sh 251(C)

# If the User Portability Utilities Option is supported and shell
# command line editing is supported:  When in command mode, then the
# command N repeats the most recent / or ? command, reversing the
# direction of the search.

p :test-1:
w echo repeat-1
u ^repeat-1\r?\n$
p :test-2:
w echo repeat-2
u ^repeat-2\r?\n$
p :test-3:
s 100
c \E
s 400
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
s 400
w ?rep
r echo repeat-2
c N
r echo repeat-1
c n
r echo repeat-2
c n
r echo repeat-3
!
fi

# This test freezes the 'less' pager on OpenBSD, which is not a ksh bug.
: <<\disabled
whence -q less &&
TERM=vt100 tst $LINENO <<"!"
L process/terminal group exercise

w m=yes; while true; do echo $m-$m; done | less
u :$|:\E|lines
c \cZ
r Stopped
w fg
u yes-yes
!
disabled

# err_exit #
# Test file name completion in vi mode
if((SHOPT_VSH)); then
mkdir "/tmp/fakehome_$$" && tst $LINENO <<!
L vi mode file name completion

# Completing a file name in vi mode that contains '~' and has a
# base name the same length as the home directory's parent directory
# shouldn't fail.

w set -o vi; HOME=/tmp/fakehome_$$; touch ~/testfile_$$
w echo ~/tes\t
u ^/tmp/fakehome_$$/testfile_$$\r?\n$
!
rm -r "/tmp/fakehome_$$"
fi # SHOPT_VSH

# err_exit #
VISUAL='' tst $LINENO <<"!"
L raw Bourne mode literal tab characters

# With wide characters (e.g. UTF-8) disabled, raw mode is handled by ed_read()
# in edit.c; it does not expand tab characters on the command line.
# With wide characters enabled, and if vi mode is compiled in, raw mode is
# handled by ed_viread() in vi.c (even though vi mode is off); it expands tab
# characters to spaces on the command line. See slowread() in io.c.

p :test-1:
w true /de\tv/nu\tl\tl
r ^:test-1: true (/de\tv/nu\tl\tl|/de       v/nu    l       l)\r\n$
p :test-2:
!

# err_exit #
VISUAL='' tst $LINENO <<"!"
L raw Bourne mode backslash handling

# The escaping backslash feature should be disabled in the raw Bourne mode.
# This is tested with both erase and kill characters.

p :test-1:
w stty erase ^H kill ^X
p :test-2:
w true string\\\\\cH\cH
r ^:test-2: true string\r\n$
p :test-3:
w true incorrect\\\cXtrue correct
r ^:test-3: true correct\r\n$
!

# err_exit #
# err_exit #
set --
((SHOPT_VSH)) && set -- "$@" vi
((SHOPT_ESH)) && set -- "$@" emacs gmacs
for mode do
VISUAL=$mode tst $LINENO << !
L escaping backslashes in $mode mode

# Backslashes should only be escaped if the previous input was a backslash.
# Other backslashes stored in the input buffer should be erased normally.

d 15
p :test-1:
w stty erase ^H
p :test-2:
w true string\\\\\\\\\\cH\\cH\\cH
r ^:test-2: true string\\r\\n$
!
done

# err_exit #
tst $LINENO <<"!"
L notify job state changes

# 'set -b' should immediately notify the user about job state changes.

p :test-1:
w set -b; sleep .01 &
u Done
!

# err_exit #
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
EOF
tst $LINENO <<"!"
L test -t 1 inside command substitution
p :test-1:
d 15
w . ./test_t.sh
r ^:test-1: \. \./test_t\.sh\r\n$
r ^OK0\r\n$
r ^OK1\r\n$
r ^OK2\r\n$
r ^OK3\r\n$
r ^OK4\r\n$
r ^OK5\r\n$
r ^OK6\r\n$
r ^OK7\r\n$
r ^OK8\r\n$
r ^OK9\r\n$
r ^OK10\r\n$
r ^:test-2:
!

# err_exit #
tst $LINENO <<"!"
L race condition while launching external commands

# Test for bug in ksh binaries that use posix_spawn() while job control is active.
# See discussion at: https://github.com/ksh93/ksh/issues/79

p :test-1:
d 15
w printf '%s\\n' 1 2 3 4 5 | while read; do ls /dev/null; done
r ^:test-1: printf '%s\\n' 1 2 3 4 5 | while read; do ls /dev/null; done\r\n$
r ^/dev/null\r\n$
r ^/dev/null\r\n$
r ^/dev/null\r\n$
r ^/dev/null\r\n$
r ^/dev/null\r\n$
r ^:test-2:
!

# err_exit #
((SHOPT_ESH)) && [[ -o ?backslashctrl ]] && tst $LINENO <<"!"
L nobackslashctrl in emacs

d 15
w set -o emacs --nobackslashctrl

# --nobackslashctrl shouldn't be ignored by reverse search
p :test-2:
w \cR\\\cH\cH
r ^:test-2: \r\n$
!

# err_exit #
((SHOPT_ESH)) && tst $LINENO <<"!"
L emacs backslash escaping

d 15
w set -o emacs

# Test for too many backslash deletions in reverse-search mode
p :test-2:
w \cRset\\\\\\\\\cH\cH\cH\cH\cH
r ^:test-2: set -o emacs$

# \ should escape the interrupt character (usually Ctrl+C)
w true \\\cC
r true \^C
!

# err_exit #
((SHOPT_VSH)) && touch vi_completion_A_file vi_completion_B_file && tst $LINENO <<"!"
L vi filename completion menu

d 15
c ls vi_co\t\t
r ls vi_completion\r\n$
r ^1) vi_completion_A_file\r\n$
r ^2) vi_completion_B_file\r\n$
w 2\t
r ^:test-1: ls vi_completion_B_file \r\n$
r ^vi_completion_B_file\r\n$

# 93v- bug: tab completion writes past input buffer
# https://github.com/ksh93/ksh/issues/195

# ...reproducer 1
c ls vi_compl\t\t
r ls vi_completion\r\n$
r ^1) vi_completion_A_file\r\n$
r ^2) vi_completion_B_file\r\n$
w aB_file
r ^:test-2: ls vi_completion_B_file\r\n$
r ^vi_completion_B_file\r\n$

# ...reproducer 2
c \rls vi_comple\t\t
u ls vi_completion\r\n$
r ^1) vi_completion_A_file\r\n$
r ^2) vi_completion_B_file\r\n$
w 0$aA_file
r ^:test-3: ls vi_completion_A_file\r\n$
r ^vi_completion_A_file\r\n$
!

# err_exit #
tst $LINENO <<"!"
L syntax error added to history file

# https://github.com/ksh93/ksh/issues/209

d 15
p :test-1:
w do something
r ^:test-1: do something\r\n$
r : syntax error: `do' unexpected\r\n$
w fc -lN1
r ^:test-2: fc -lN1\r\n$
r \tdo something\r\n$
!

# err_exit #
tst $LINENO <<"!"
L value of $? after the shell uses a variable with a discipline function

w PS1.get() { true; }; PS2.get() { true; }; false
u PS1.get\(\) \{ true; \}; PS2.get\(\) \{ true; \}; false
w echo "Exit status is: $?"
u Exit status is: 1
w LINES.set() { return 13; }
u LINES.set\(\) \{ return 13; \}
w echo "Exit status is: $?"
u Exit status is: 0

# It's worth noting that the test below will always fail in ksh93u+ and ksh2020,
# even when $PS2 lacks a discipline function (see https://github.com/ksh93/ksh/issues/117).
# After that bug was fixed the test below could still fail if PS2.get() existed.
w false
w (
w exit
w )
w echo "Exit status is: $?"
u Exit status is: 1
!

# err_exit #
((SHOPT_ESH)) && ((SHOPT_VSH)) && tst $LINENO <<"!"
L crash after switching from emacs to vi mode

# In ksh93r using the vi 'r' command after switching from emacs mode could
# trigger a memory fault: https://bugzilla.opensuse.org/show_bug.cgi?id=179917

d 15
p :test-1:
w exec "$SHELL" -o emacs
r ^:test-1: exec "\$SHELL" -o emacs\r\n$
p :test-1:
w set -o vi
r ^:test-1: set -o vi\r\n$
p :test-2:
c \Erri
w echo Success
r ^:test-2: echo Success\r\n$
r ^Success\r\n$
!

# err_exit #
((SHOPT_VSH || SHOPT_ESH)) && tst $LINENO <<"!"
L value of $? after tilde expansion in tab completion

# Make sure that a .sh.tilde.set discipline function
# cannot influence the exit status.

w .sh.tilde.set() { true; }
w HOME=/tmp
w false ~\t
u false /tmp
w echo "Exit status is: $?"
u Exit status is: 1
w (exit 42)
w echo $? ~\t
u 42 /tmp
!

# err_exit #
((SHOPT_MULTIBYTE && (SHOPT_VSH || SHOPT_ESH))) &&
[[ ${LC_ALL:-${LC_CTYPE:-${LANG:-}}} =~ [Uu][Tt][Ff]-?8 ]] &&
touch $'XXX\xc3\xa1' $'XXX\xc3\xab' &&
tst $LINENO <<"!"
L autocomplete should not fill partial multibyte characters
# https://github.com/ksh93/ksh/issues/223

d 15
p :test-1:
w : XX\t
r ^:test-1: : XXX\r\n$
!

# err_exit #
((SHOPT_VSH)) && tst $LINENO <<"!"
L Using b, B, w and W commands in vi mode
# https://github.com/att/ast/issues/1467

d 15
p :test-1:
w set -o vi
r ^:test-1: set -o vi\r\n$
w echo asdf\EbwBWa
r ^:test-2: echo asdf\r\n$
r ^asdf\r\n$
!

# err_exit #
((SHOPT_ESH)) && mkdir -p emacstest/123abc && VISUAL=emacs tst $LINENO <<"!"
L autocomplete stops numeric input
# https://github.com/ksh93/ksh/issues/198

d 15
p :test-1:
w cd emacste\t123abc
r ^:test-1: cd emacstest/123abc\r\n$
!

# err_exit #
echo '((' >$tmp/synerror
ENV=$tmp/synerror tst $LINENO <<"!"
L syntax error in profile causes exit on startup
# https://github.com/ksh93/ksh/issues/281

d 15
r /synerror: syntax error: `\(' unmatched\r\n$
p :test-1:
w echo ok
r ^:test-1: echo ok\r\n$
r ^ok\r\n$
!

# err_exit #
((SHOPT_VSH)) && tst $LINENO <<"!"
L split on quoted whitespace when extracting words from command history
# https://github.com/ksh93/ksh/pull/291

d 15
p :test-1:
w true ls One\\ "Two Three"$'Four Five'.mp3
r ^:test-1: true ls One\\ "Two Three"\$'Four Five'\.mp3\r\n$
p :test-2:
w :\E_
r ^:test-2: : One\\ "Two Three"\$'Four Five'\.mp3\r\n$
!

# err_exit #
((SHOPT_VSH)) && tst $LINENO <<"!"
L crash when entering comment into history file (vi mode)
# https://github.com/att/ast/issues/798

d 15
p :test-1:
c foo \E#
r ^:test-1: #foo\r\n$
w hist -lnN 1
r ^:test-2: hist -lnN 1\r\n$
r \t#foo\r\n$
r \thist -lnN 1\r\n$
!

# err_exit #
((SHOPT_VSH || SHOPT_ESH)) && tst $LINENO <<"!"
L tab completion while expanding ${.sh.*} variables
# https://github.com/att/ast/issues/1461

d 15
p :test-1:
w test \$\{.sh.level\t
r ^:test-1: test \$\{.sh.level\}\r\n$
!

# err_exit #
((SHOPT_VSH || SHOPT_ESH)) && tst $LINENO <<"!"
L tab completion executes command substitutions
# https://github.com/ksh93/ksh/issues/268

d 15
p :test-1:
w $(echo true)\t
r ^:test-1: \$\(echo true\)\r\n$
p :test-2:
w `echo true`\t
r ^:test-2: `echo true`\r\n$
!

# err_exit #
((SHOPT_ESH)) && VISUAL=emacs tst $LINENO <<"!"
L emacs: keys with repeat parameters repeat extra steps
# https://github.com/ksh93/ksh/issues/292

d 15
p :test-1:
w : foo bar delete add\1\6\6\E3\Ed
r ^:test-1: :  add\r\n$
p :test-2:
w : foo bar delete add\E3\Eh
r ^:test-2: : foo \r\n$
p :test-3:
w : test_string\1\6\6\E3\E[3~
r ^:test-3: : t_string\r\n$
p :test-4:
w : test_string\1\E6\E[C\4
r ^:test-4: : teststring\r\n$
!

# ======
exit $((Errors<125?Errors:125))

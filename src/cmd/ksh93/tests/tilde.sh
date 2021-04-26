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
saveHOME=$HOME

if	$SHELL -c '[[ ~root == /* ]]'
then	x=$(print -r -- ~root)
	[[ $x == ~root ]] || err_exit '~user expanded in subshell prevent ~user from working'
fi

function home # id
{
	typeset IFS=: pwd=/etc/passwd
	set -o noglob
	if	[[ -f $pwd ]] && grep -c "^$1:" $pwd > /dev/null
	then	set -- $(grep "^$1:" $pwd)
		print -r -- "$6"
	else	print .
	fi
}

OLDPWD=/bin
if	[[ ~ != $HOME ]]
then	err_exit '~' not $HOME
fi
x=~
if	[[ $x != $HOME ]]
then	err_exit x=~ not $HOME
fi
x=x:~
if	[[ $x != x:$HOME ]]
then	err_exit x=x:~ not x:$HOME
fi
if	[[ ~+ != $PWD ]]
then	err_exit '~' not $PWD
fi
x=~+
if	[[ $x != $PWD ]]
then	err_exit x=~+ not $PWD
fi
if	[[ ~- != $OLDPWD ]]
then	err_exit '~' not $PWD
fi
x=~-
if	[[ $x != $OLDPWD ]]
then	err_exit x=~- not $OLDPWD
fi
for u in root Administrator
do	h=$(home $u)
	if	[[ $h != . ]]
	then	[[ ~$u -ef $h ]] || err_exit "~$u not $h"
		x=~$u
		[[ $x -ef $h ]] || x="~$u not $h"
		break
	fi
done
x=~g.r.emlin
if	[[ $x != '~g.r.emlin' ]]
then	err_exit "x=~g.r.emlin failed -- expected '~g.r.emlin', got '$x'"
fi
x=~:~
if	[[ $x != "$HOME:$HOME" ]]
then	err_exit "x=~:~ failed, expected '$HOME:$HOME', got '$x'"
fi
HOME=/
[[ ~ == / ]] || err_exit '~ should be /'
[[ ~/foo == /foo ]] || err_exit '~/foo should be /foo when ~==/'
print $'print ~+\n[[ $1 ]] && $0' > $tmp/tilde
chmod +x $tmp/tilde
nl=$'\n'
[[ $($tmp/tilde foo) == "$PWD$nl$PWD" ]] 2> /dev/null  || err_exit 'tilde fails inside a script run by name'

# ======
# Tilde expansion should not change the value of $HOME.

HOME=/
: ~/foo
[[ $HOME == / ]] || err_exit "tilde expansion changes \$HOME (value: $(printf %q "$HOME"))"

# ======
# After unsetting HOME, ~ should expand to the current user's OS-configured home directory.

unset HOME
exp=~${ id -un; }
got=~
[[ $got == "$exp" ]] || err_exit 'expansion of bare tilde breaks after unsetting HOME' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
HOME=$saveHOME

# ======
# Tilde expansion discipline function tests

# This nonfunctional mess was removed in ksh 93u+m ...
if builtin .sh.tilde 2>/dev/null
then	got=$(.sh.tilde & wait "$!" 2>&1)
	((!(e = $?))) || err_exit ".sh.tilde builtin crashes the shell" \
		"(got status $e$( ((e>128)) && print -n / && kill -l "$e"), $(printf %q "$got"))"
fi

# ... and replaced by a proper use of discipline functions that allows customising tilde expansion.
((.sh.version >= 20210316)) &&
for disc in get set
do	(
		ulimit -t unlimited 2>/dev/null  # fork subshell to cope with a possible crash

		eval ".sh.tilde.$disc()
		{
			case \${.sh.${ [[ $disc == get ]] && print tilde || print value; }} in
			'~tmp')	.sh.value=\$tmp ;;
			'~INC')	.sh.value=\$((++i)) ;;
			'~spc') .sh.value=$'one\ttwo  three\n\tfour' ;;
			'~')	.sh.value=~/addition ;;  # this should not recurse
			esac
		}"

		got=~/foo
		exp=$HOME/addition/foo
		[[ $got == "$exp" ]] || err_exit "$disc discipline: bare tilde expansion:" \
			"expected $(printf %q "$exp"), got $(printf %q "$got")"

		.sh.tilde=oldvalue
		got=$(print ~tmp/foo.$$; print "${.sh.tilde}")
		exp=$tmp/foo.$$$'\n'$tmp
		[[ $got == "$exp" ]] || err_exit "$disc discipline: result left in \${.sh.tilde}:" \
			"expected $(printf %q "$tmp"), got $(printf %q "${.sh.tilde}")"
		[[ ${.sh.tilde} == oldvalue ]] || err_exit "$disc discipline: \${.sh.tilde} subshell leak"

		i=0
		set -- ~INC ~INC ~INC ~INC ~INC
		got=$#,$1,$2,$3,$4,$5
		exp=5,1,2,3,4,5
		[[ $got == "$exp" ]] || err_exit "$disc discipline: counter:" \
			"expected $(printf %q "$exp"), got $(printf %q "$got")"
		((i==5)) || err_exit "$disc discipline: counter: $i != 5"

		set -- ~spc ~spc ~spc
		got=$#,$1,$2,$3
		exp=$'3,one\ttwo  three\n\tfour,one\ttwo  three\n\tfour,one\ttwo  three\n\tfour'
		[[ $got == "$exp" ]] || err_exit "$disc discipline: quoting of whitespace:" \
			"expected $(printf %q "$exp"), got $(printf %q "$got")"

		print "$Errors" >$tmp/Errors
	) &
	wait "$!" 2>crashmsg
	if	((!(e = $?)))
	then	read Errors <$tmp/Errors
	else	err_exit ".sh.tilde.$disc discipline function crashes the shell" \
			"(got status $e$( ((e>128)) && print -n / && kill -l "$e"), $(printf %q "$(<crashmsg)"))"
	fi
done

# ======
exit $((Errors<125?Errors:125))

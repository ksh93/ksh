########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1982-2012 AT&T Intellectual Property          #
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

.sh.tilde()
{
	case $1 in
	'~tmp')	print -r "$tmp" ;;
	'~INC')	print -r $((++i)) ;;
	'~spc') print -r $'one\ttwo  three\n\tfour' ;;
	esac
}

HOME=/
: ~/foo
[[ $HOME == / ]] || err_exit "tilde expansion changes \$HOME (value: $(printf %q "$HOME"))"

HOME=/dev/null
[[ ~/foo == "$HOME/foo" ]] || err_exit '.sh.tilde() prevents regular tilde expansion if uncaught'

touch "$tmp/foo.$$"
[[ -f ~tmp/foo.$$ ]] || err_exit '.sh.tilde() not working'
[[ ${.sh.tilde} == "$tmp" ]] || err_exit "result not left in \${.sh.tilde} (value: $(printf %q "${.sh.tilde-none}"))"

i=0
set -- ~INC ~INC ~INC ~INC ~INC
got=$#,$1,$2,$3,$4,$5
exp=5,1,2,3,4,5
[[ $got == "$exp" ]] || err_exit "tilde counter: expected $(printf %q "$exp"), got $(printf %q "$got")"
((i==5)) || err_exit "tilde counter: $i != 5"

set -- ~spc ~spc ~spc
got=$#,$1,$2,$3
exp=$'3,one\ttwo  three\n\tfour,one\ttwo  three\n\tfour,one\ttwo  three\n\tfour'
[[ $got == "$exp" ]] || err_exit "quoting of whitespace: expected $(printf %q "$exp"), got $(printf %q "$got")"

# ======
exit $((Errors<125?Errors:125))

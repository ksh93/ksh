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

function abspath
{
        base=$(basename $SHELL)
        cd ${SHELL%/$base}
        newdir=$(pwd)
        cd ~-
        print $newdir/$base
}
# test for proper exit of shell
if builtin getconf 2> /dev/null; then
	ABSHELL=$(abspath)
	print exit 0 >.profile
	${ABSHELL}  <<!
	HOME=$PWD \
	PATH=$PATH \
	SHELL=$ABSSHELL \
	$(
		v=$(getconf LIBPATH)
		for v in ${v//,/ }
		do	v=${v#*:}
			v=${v%%:*}
			eval [[ \$$v ]] && eval print -n \" \"\$v=\"\$$v\"
		done
	) \
	exec -c -a -ksh ${ABSHELL} -c "exit 1" 1>/dev/null 2>&1
!
	status=$(echo $?)
	if	[[ -o noprivileged && $status != 0 ]]
	then	err_exit 'exit in .profile is ignored'
	elif	[[ -o privileged && $status == 0 ]]
	then	err_exit 'privileged .profile not ignored'
	fi
fi
if	[[ $(trap 'code=$?; echo $code; trap 0; exit $code' 0; exit 123) != 123 ]]
then	err_exit 'exit not setting $?'
fi
cat > run.sh <<- "EOF"
	trap 'code=$?; echo $code; trap 0; exit $code' 0
	( trap 0; exit 123 )
EOF
if	[[ $($SHELL ./run.sh) != 123 ]]
then	err_exit 'subshell trap on exit overwrites parent trap'
fi
cd /
cd ~- || err_exit "cd back failed"
if builtin getconf 2> /dev/null; then
	$SHELL -c 'builtin -f cmd getconf; getconf --"?-version"; exit 0' >/dev/null 2>&1 || err_exit 'ksh plugin exit failed -- was ksh built with CCFLAGS+=$(CC.EXPORT.DYNAMIC)?'
fi

# ======
# Verify the 'exit' command behaves as expected

got=$($SHELL -c 'exit' 2>&1)
status=$?
exp=0
[[ -z $got ]] || err_exit 'bare exit' \
	"(got $(printf %q "$got"))"
[[ $exp == $status ]] || err_exit 'bare exit' \
	"(expected '$exp', got '$status')"

got=$($SHELL -c 'exit 0' 2>&1)
status=$?
exp=0
[[ -z $got ]] || err_exit 'exit 0' \
	"(got $(printf %q "$got"))"
[[ $exp == $status ]] || err_exit 'exit 0' \
	"(expected '$exp', got '$status')"

got=$($SHELL -c 'exit 1' 2>&1)
status=$?
exp=1
[[ -z $got ]] || err_exit 'exit 1' \
	"(got $(printf %q "$got"))"
[[ $exp == $status ]] || err_exit 'exit 1' \
	"(expected '$exp', got '$status')"

got=$($SHELL -c 'function e37 { return 37; } ; e37' 2>&1)
status=$?
exp=37
[[ -z $got ]] || err_exit 'exit 37' \
	"(got $(printf %q "$got"))"
[[ $exp == $status ]] || err_exit 'exit 37' \
	"(expected '$exp', got '$status')"

got=$($SHELL -c 'exit -1' 2>&1)
status=$?
exp=255
[[ -z $got ]] || err_exit 'exit -1' \
	"(got $(printf %q "$got"))"
[[ $exp == $status ]] || err_exit 'exit -1' \
	"(expected '$exp', got '$status')"

got=$($SHELL -c 'exit -2' 2>&1)
status=$?
exp=254
[[ -z $got ]] || err_exit 'exit -2' \
	"(got $(printf %q "$got"))"
[[ $exp == $status ]] || err_exit 'exit -2' \
	"(expected '$exp', got '$status')"

$SHELL +E -i 2>/dev/null <<- \!
	false
	exit
!
status=$?
exp=1
[[ $exp == $status ]] || err_exit 'bare exit after false' \
	"(expected '$exp', got '$status')"

# ======
# Exit behaviour for commands, expansions and assignments
# https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_08_01

builtin -s | while read cmd
do
	[[ $cmd == : ]] && continue  # ':' never errors out
	("$cmd" --badoption; true) 2>/dev/null \
	&& err_exit "Special built-in utility error does not cause shell to exit ($cmd)"
	(command "$cmd" --badoption; true) 2>/dev/null \
	|| err_exit "Error in special built-in utility prefixed by 'command' causes shell to exit ($cmd)"
done

(command shift 10; true) 2>/dev/null \
|| err_exit "Regular built-in utility error causes shell to exit"

(readonly foo=bar; foo=baz; true) 2>/dev/null \
&& err_exit "Variable assignment error does not cause shell to exit"

(command true ${foo@bar}; true) 2>/dev/null \
&& err_exit "Expansion error does not cause shell to exit"

(/dev/null/nonexistent; true) 2>/dev/null \
|| err_exit "Command not found causes shell to exit"

# Exit behaviour for redirections

(>/dev/null/nonexistent; true) 2>/dev/null \
|| err_exit 'Error in lone redirection causes shell to exit'

(: >/dev/null/nonexistent; true) 2>/dev/null \
&& err_exit 'Redirection error with special built-in utility does not cause shell to exit'

(false >/dev/null/nonexistent; true) 2>/dev/null \
|| err_exit 'Redirection error with regular built-in utility causes shell to exit'

("$(whence -p false)" >/dev/null/nonexistent; true) 2>/dev/null \
|| err_exit 'Redirection error with external command causes shell to exit'

(/dev/null/nonexistent >/dev/null/nonexistent; true) 2>/dev/null \
|| err_exit 'Redirection error with nonexistent command causes shell to exit'

(foo=bar >/dev/null/nonexistent; true) 2>/dev/null \
|| err_exit 'Redirection error with variable assignment causes shell to exit'

({ false; }; >/dev/null/nonexistent; true) 2>/dev/null \
|| err_exit 'Redirection error with compound command causes shell to exit'

# https://github.com/ksh93/ksh/issues/310
(fn() { false; }; fn >/dev/null/nonexistent; true) 2>/dev/null \
|| err_exit 'Redirection error with function execution causes shell to exit'

# ======
exit $((Errors<125?Errors:125))

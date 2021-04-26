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
builtin getconf
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
$SHELL -c 'builtin -f cmd getconf; getconf --"?-version"; exit 0' >/dev/null 2>&1 || err_exit 'ksh plugin exit failed -- was ksh built with CCFLAGS+=$(CC.EXPORT.DYNAMIC)?'

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
exit $((Errors<125?Errors:125))

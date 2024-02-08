########################################################################
#                                                                      #
#              This file is part of the ksh 93u+m package              #
#          Copyright (c) 2021-2024 Contributors to ksh 93u+m           #
#                      and is licensed under the                       #
#                 Eclipse Public License, Version 2.0                  #
#                                                                      #
#                A copy of the License is available at                 #
#      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      #
#         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         #
#                                                                      #
#                  Martijn Dekker <martijn@inlv.org>                   #
#                                                                      #
########################################################################

. "${SHTESTS_COMMON:-${0%/*}/_common}"

# All the tests here should run with job control on
set -o monitor

# ======
# Check job control job IDs: %%, %n. Before 2021-02-11 this did not work for 'fg' in scripts.
sleep 1 &
kill %% >out 2>&1
kill $! 2>/dev/null && err_exit "'kill %%' not working in script (got $(printf %q "$(<out)"))"
sleep 1 &
kill %2 >out 2>&1
kill $! 2>/dev/null && err_exit "'kill %2' not working in script (got $(printf %q "$(<out)"))"
sleep .05 &
wait >out 2>&1
kill $! 2>/dev/null && err_exit "'wait' not working in script (got $(printf %q "$(<out)"))"
sleep .05 &
wait %% >out 2>&1
kill $! 2>/dev/null && err_exit "'wait %%' not working in script (got $(printf %q "$(<out)"))"
sleep .05 &
wait %1 >out 2>&1
kill $! 2>/dev/null && err_exit "'wait %1' not working in script (got $(printf %q "$(<out)"))"
sleep .05 &
fg >out 2>&1
kill $! 2>/dev/null && err_exit "'fg' not working in script (got $(printf %q "$(<out)"))"
sleep .05 &
fg %% >out 2>&1
kill $! 2>/dev/null && err_exit "'fg %%' not working in script (got $(printf %q "$(<out)"))"
sleep .05 &
fg %1 >out 2>&1
kill $! 2>/dev/null && err_exit "'fg %1' not working in script (got $(printf %q "$(<out)"))"
sleep 1 &
sleep 1 &
bg >out 2>&1 || err_exit "'bg' not working in script (got $(printf %q "$(<out)"))"
bg %% >out 2>&1 || err_exit "'bg %%' not working in script (got $(printf %q "$(<out)"))"
bg %+ >out 2>&1 || err_exit "'bg %+' not working in script (got $(printf %q "$(<out)"))"
bg %- >out 2>&1 || err_exit "'bg %-' not working in script (got $(printf %q "$(<out)"))"
bg %1 >out 2>&1 || err_exit "'bg %1' not working in script (got $(printf %q "$(<out)"))"
bg %2 >out 2>&1 || err_exit "'bg %2' not working in script (got $(printf %q "$(<out)"))"
disown >out 2>&1 || err_exit "'disown' not working in script (got $(printf %q "$(<out)"))"
disown %% >out 2>&1 || err_exit "'disown %%' not working in script (got $(printf %q "$(<out)"))"
disown %+ >out 2>&1 || err_exit "'disown %+' not working in script (got $(printf %q "$(<out)"))"
disown %- >out 2>&1 || err_exit "'disown %-' not working in script (got $(printf %q "$(<out)"))"
disown %1 >out 2>&1 || err_exit "'disown %1' not working in script (got $(printf %q "$(<out)"))"
disown %2 >out 2>&1 || err_exit "'disown %2' not working in script (got $(printf %q "$(<out)"))"
kill %- >out 2>&1 || err_exit "'kill %-' not working in script (got $(printf %q "$(<out)"))"
kill %+ >out 2>&1 || err_exit "'kill %+' not working in script (got $(printf %q "$(<out)"))"

# fail gracefully: suppress "Terminated" noise on pre-93u+m ksh93
{ wait; } 2>/dev/null

# =====
# Before 2021-02-11, job control was deactivated in subshells
# https://www.mail-archive.com/austin-group-l@opengroup.org/msg06456.html
(sleep 1 & UNIX95=1 command -p ps -o pid= -o pgid= -p $! 2>/dev/null) | IFS=$' \t' read -r pid pgid
if	let "pid>0 && pgid>0" 2>/dev/null
then	kill $pid
	let "pgid == pid" || err_exit "background job run in subshell didn't get its own process group ($pgid != $pid)"
else	warning "skipping subshell job control test due to non-compliant 'ps'"
fi

# ======
# Before 2021-02-11, using a shared-state ${ command substitution; } twice caused ksh to lose track of all running jobs
jobs >/dev/null  # get 'Done' messages out of the way
sleep 1 & sleep 1 &
j1=${ jobs; }
[[ $j1 == $'[2] +  Running '*$'\n[1] -  Running '* ]] || err_exit "sleep jobs not registered (got $(printf %q "$j1"))"
: ${ :; } ${ :; }
j2=${ jobs; }
kill %- %+
wait 2>/dev/null
[[ $j2 == "$j1" ]] || err_exit "jobs lost after shared-state command substitution ($(printf %q "$j2") != $(printf %q "$j1"))"

# ======
# Before 2024-01-05, ksh wrongly printed job numbers for background jobs invoked from subshells in profile scripts.
if((!SHOPT_SCRIPTONLY));then
print '(true &); :' >$tmp/profile
got=$(ENV=$tmp/profile "$SHELL" -i </dev/null 2>&1)
[[ -n $got ]] && err_exit "subshell bg job in profile script prints job number (got $(printf %q "$got"))"
fi # !SHOPT_SCRIPTONLY

# ======
exit $((Errors<125?Errors:125))

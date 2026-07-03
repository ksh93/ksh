########################################################################
#                                                                      #
#              This file is part of the ksh 93u+m package              #
#          Copyright (c) 2021-2026 Contributors to ksh 93u+m           #
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

# ksh 93u+m has ${.sh.pid} which resolves to the current subshell's PID.
# For the tests to fail accurately on older ksh versions, provide a fallback.
[[ -v .sh.pid ]] || .sh.pid.get() { .sh.value=$("$SHELL" -c 'echo $PPID'); }

# Check for an adequately compliant ps(1).
# Note: FreeBSD ps(1) (as of 15.1-RELEASE) does not support multiple comma-separated
# arguments to the -o option, though POSIX requires it. Only multiple -o options work.
function ps { UNIX95=1 command ps "$@"; }
enum bool=(false true)
bool compliant_ps=false
(ps -o pid= -o pgid= -p ${.sh.pid} | read pid pgid) && compliant_ps=true

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
got=$(set +x; ENV=$tmp/profile "$SHELL" -i </dev/null 2>&1)
[[ -n $got ]] && err_exit "subshell bg job in profile script prints job number (got $(printf %q "$got"))"
fi # !SHOPT_SCRIPTONLY

# =====
if	((compliant_ps == false))
then	warning "skipping process group tests due to non-compliant 'ps'"
else
	# With job control on, the top-level subshell should start its own process group
	# and any sub-subshells and external commands should be part of that process group
	(ps -o pid= -o pgid= -p ${.sh.pid}) >out
	IFS=$' \t' read -r pid pgid <out
	let "pid != $$" || err_exit "subshell did not fork to start its own process group ($pid == $$)"
	let "pgid == pid" || err_exit "subshell did not start its own process group ($pgid != $pid)"

	(sleep 1 & ps -o pid= -o pgid= -p $!) >out
	IFS=$' \t' read -r pid pgid <out
	kill $pid
	let "pgid != pid" || err_exit "background job run in subshell not part of subshell's process group ($pgid == $pid)"

	(set -m; sleep 1 & ps -o pid= -o pgid= -p $!) >out
	IFS=$' \t' read -r pid pgid <out
	kill $pid
	let "pgid == pid" || err_exit "background job run in subshell with explicit 'set -m'" \
		"did not start its own process group ($pgid != $pid)"

	# An external command launched with -m on, unless launched from a subshell, should start its own process group
	"$SHELL" -mc '"$SHELL" -c '\''UNIX95=1 ps -o pid= -o pgid= -p $$'\''; true' |
		IFS=$' \t' read -r pid pgid
	let "pgid == pid" || err_exit "foreground job with -m on didn't start its own process group ($pgid != $pid)"

	# Just in case, also test this with -m off
	"$SHELL" +m -c '"$SHELL" -c '\''UNIX95=1 ps -o pid= -o pgid= -p $$'\''; true' |
		IFS=$' \t' read -r pid pgid
	let "pgid == pid" && err_exit "foreground command without -m on incorrectly started its own process group ($pgid == $pid)"

	# All components of a pipeline (except the last, if built-in command(s)) must be in the same process group
	# This failed intermittently between 2021-02-11 and 2026-07-01
	builtin cat
	ps -o pgid= -p ${.sh.pid} |			# 0
		{ cat; ps -o pgid= -p ${.sh.pid}; } |	# 1
		{ cat; ps -o pgid= -p ${.sh.pid}; } |	# 2
		{ cat; ps -o pgid= -p ${.sh.pid}; } |	# 3
		{ cat; ps -o pgid= -p ${.sh.pid}; } |	# 4
		{ cat; ps -o pgid= -p ${.sh.pid}; } |	# 5
		{ cat; ps -o pgid= -p ${.sh.pid}; } |	# 6
		{ cat; ps -o pgid= -p ${.sh.pid}; } |	# 7
		{ cat; ps -o pgid= -p ${.sh.pid}; } |	# 8
		{ cat; ps -o pgid= -p ${.sh.pid}; } |	# 9
		{ read p0 && read p1 && read p2 && read p3 && read p4 && read p5 && read p6 && read p7 && read p8 && read p9; }
	let "p1==p0 && p2==p0 && p3==p0 && p4==p0 && p5==p0 && p6==p0 && p7==p0 && p8==p0 && p9==p0" ||
		err_exit "not all components of pipeline in same process group" \
			"(got $p0, $p1, $p2, $p3, $p4, $p5, $p6, $p7, $p8, $p9)"
	unset p0 p1 p2 p3 p4 p5 p6 p7 p8 p9
fi

# ======
exit $((Errors<125?Errors:125))

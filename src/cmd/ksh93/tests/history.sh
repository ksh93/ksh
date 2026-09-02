########################################################################
#                                                                      #
#              This file is part of the ksh 93u+m package              #
#          Copyright (c) 2020-2026 Contributors to ksh 93u+m           #
#                      and is licensed under the                       #
#                 Eclipse Public License, Version 2.0                  #
#                                                                      #
#                A copy of the License is available at                 #
#      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      #
#         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         #
#                                                                      #
#                          Claude Opus 4.6                             #
#              Siteshwar Vashisht <svashisht@redhat.com>               #
#                  Martijn Dekker <martijn@inlv.org>                   #
#                                                                      #
########################################################################

# Tests for history file locking and concurrent access (issue #997)

. "${SHTESTS_COMMON:-${0%/*}/_common}"

((!SHOPT_SCRIPTONLY)) || { warning "interactive shell was compiled out -- tests skipped"; exit 0; }

# ======
# Basic sanity: a single interactive session writes and retains history

histfile=$tmp/hist_basic
got=$( set +x; HISTFILE=$histfile HISTSIZE=512 $SHELL +E -ic '
	print -s "echo alpha"
	print -s "echo bravo"
	print -s "echo charlie"
	hist -l -N 3 2>/dev/null
	exit
' 2>/dev/null )
[[ $got == *alpha* && $got == *bravo* && $got == *charlie* ]] \
	|| err_exit "single session: print -s entries not retained in history" \
		"(got $(printf %q "$got"))"
[[ -f $histfile ]] || err_exit "HISTFILE not created"

# ======
# History file survives re-open: entries written by one session are
# visible to a subsequent session using the same HISTFILE.

histfile=$tmp/hist_reopen
HISTFILE=$histfile HISTSIZE=512 $SHELL +E -ic '
	print -s "first_session_cmd"
	exit
' </dev/null 2>/dev/null
got=$( set +x; HISTFILE=$histfile HISTSIZE=512 $SHELL +E -ic '
	hist -l -N 5 2>/dev/null
	exit
' 2>/dev/null )
[[ $got == *first_session_cmd* ]] \
	|| err_exit "history entry from prior session not visible after reopen" \
		"(got $(printf %q "$got"))"

# ======
# Concurrent writes: N sessions writing to the same HISTFILE should not
# lose entries.  This is the scenario described in issue #997.
#
# We spawn several background shells that each write a unique set of
# lines, wait for all of them, then verify all entries exist.

histfile=$tmp/hist_concurrent
n_sessions=5
n_lines=20

for ((s=0; s<n_sessions; s++))
do
	HISTFILE=$histfile HISTSIZE=$((n_sessions * n_lines + 100)) \
	$SHELL +E -ic "
		for ((i=0; i<$n_lines; i++))
		do	print -s \"session_${s}_cmd_\${i}\"
		done
		exit
	" </dev/null 2>/dev/null &
done
wait

# Read back: all entries should be present
got=$( set +x; HISTFILE=$histfile HISTSIZE=$((n_sessions * n_lines + 100)) \
	$SHELL +E -ic 'hist -l -N '$((n_sessions * n_lines + 50))' 2>/dev/null; exit' 2>/dev/null )
missing=0
for ((s=0; s<n_sessions; s++))
do
	for ((i=0; i<n_lines; i++))
	do	[[ $got == *"session_${s}_cmd_${i}"* ]] || ((missing++))
	done
done
# Allow a small margin for inherent raciness on systems without working
# fcntl locking, but the majority should survive.
total=$((n_sessions * n_lines))
threshold=$(( total * 80 / 100 ))
present=$(( total - missing ))
(( present >= threshold )) \
	|| err_exit "concurrent history writes: only $present/$total entries survived" \
		"(expected at least $threshold)"

# ======
# History trimming: writing more than HISTSIZE entries should trigger
# a trim, and the most recent entries should be retained.

histfile=$tmp/hist_trim
HISTFILE=$histfile HISTSIZE=50 $SHELL +E -ic '
	for ((i=0; i<100; i++))
	do	print -s "trimtest_cmd_${i}"
	done
	exit
' </dev/null 2>/dev/null
# The last 50 entries (trimtest_cmd_50 .. trimtest_cmd_99) should survive;
# the first entries should have been trimmed away.
got=$( set +x; HISTFILE=$histfile HISTSIZE=50 $SHELL +E -ic '
	hist -l -N 60 2>/dev/null
	exit
' 2>/dev/null )
[[ $got == *trimtest_cmd_99* ]] \
	|| err_exit "most recent entry lost after history trim" \
		"(got $(printf %q "$got"))"
[[ $got == *trimtest_cmd_0* ]] \
	&& err_exit "oldest entry not trimmed (HISTSIZE limit not enforced)"

# ======
# Concurrent writes during trim: multiple sessions write enough to
# trigger trimming while other sessions are also writing.  Verify no
# crash and that the file remains a valid history file.

histfile=$tmp/hist_trim_race
n_sessions=4
n_lines=80
histsize=60

for ((s=0; s<n_sessions; s++))
do
	HISTFILE=$histfile HISTSIZE=$histsize $SHELL +E -ic "
		for ((i=0; i<$n_lines; i++))
		do	print -s \"trim_race_${s}_cmd_\${i}\"
		done
		exit
	" </dev/null 2>/dev/null &
done
wait

# Verify the file is still usable (ksh can open it and read history)
got=$( set +x; HISTFILE=$histfile HISTSIZE=$histsize $SHELL +E -ic '
	hist -l -N 10 2>/dev/null
	exit
' 2>/dev/null )
# We just need it to not crash and to produce some output
[[ -n $got ]] \
	|| err_exit "history file not usable after concurrent trim" \
		"(empty hist -l output)"

# ======
# History file with non-writable parent directory: verify that trim
# still works (the new code uses ftruncate+copyback via TMPDIR rather
# than unlink+recreate).

if	(( $(id -u) != 0 ))
then	histdir=$tmp/hist_noperm_dir
	mkdir "$histdir"
	histfile=$histdir/sh_history
	# Pre-create the history file while directory is writable
	HISTFILE=$histfile HISTSIZE=50 $SHELL +E -ic '
		for ((i=0; i<80; i++))
		do	print -s "noperm_cmd_${i}"
		done
		exit
	' </dev/null 2>/dev/null
	# Now make parent non-writable and try again -- trim should still work
	chmod a-w "$histdir"
	got=$( set +x; HISTFILE=$histfile HISTSIZE=50 $SHELL +E -ic '
		print -s "after_chmod_cmd"
		hist -l -N 5 2>/dev/null
		exit
	' 2>/dev/null )
	chmod u+w "$histdir"  # restore for cleanup
	[[ $got == *after_chmod_cmd* ]] \
		|| err_exit "history not usable when parent directory is read-only" \
			"(got $(printf %q "$got"))"
fi

# ======
# Rapidly alternating sessions: open, write one entry, close; repeat
# with the same HISTFILE.  Tests lock acquire/release cycling.

histfile=$tmp/hist_cycle
for ((s=0; s<10; s++))
do
	HISTFILE=$histfile HISTSIZE=512 $SHELL +E -ic "
		print -s \"cycle_cmd_$s\"
		exit
	" </dev/null 2>/dev/null
done
got=$( set +x; HISTFILE=$histfile HISTSIZE=512 $SHELL +E -ic '
	hist -l -N 15 2>/dev/null
	exit
' 2>/dev/null )
missing=0
for ((s=0; s<10; s++))
do
	[[ $got == *"cycle_cmd_$s"* ]] || ((missing++))
done
(( missing == 0 )) \
	|| err_exit "sequential session cycling: $missing/10 entries lost" \
		"(got $(printf %q "$got"))"

# ======
# Stress test: many concurrent sessions each writing one line, verifying
# no corruption or crashes.

histfile=$tmp/hist_stress
n_sessions=20
for ((s=0; s<n_sessions; s++))
do
	HISTFILE=$histfile HISTSIZE=512 $SHELL +E -ic "
		print -s \"stress_cmd_$s\"
		exit
	" </dev/null 2>/dev/null &
done
wait
# Verify no crash and file is parseable
got=$( set +x; HISTFILE=$histfile HISTSIZE=512 $SHELL +E -ic '
	hist -l -N 1 2>/dev/null
	exit
' 2>/dev/null )
[[ -n $got ]] || err_exit "ksh produced no history output after $n_sessions concurrent sessions"

# Verify the history file starts with the expected magic byte (HIST_UNDO = 0201 = 0x81)
got=$(dd if="$histfile" bs=1 count=1 2>/dev/null | od -An -tx1 | tr -d ' ')
[[ $got == 81 ]] \
	|| err_exit "history file magic byte corrupted after stress test" \
		"(expected 81, got $(printf %q "$got"))"

# ======
exit $((Errors<125?Errors:125))

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
#                                                                      #
########################################################################

. "${SHTESTS_COMMON:-${0%/*}/_common}"

# flag Android/Termux execve(3) breakage killing 'exec -a'
typeset -i execve_ignores_argv0
[[ $( (exec -a foo true) 2>&1 ) == *'not supported'* ]]
if	((execve_ignores_argv0 = ! $?))
then	warning "execve(3) broken on this system; tests involving 'exec -a' are skipped"
fi

unset HISTFILE
export LC_ALL=C ENV=/./dev/null

ulimit -c 0

bincat=$(whence -p cat)

if	[[ $( ${SHELL-ksh} -s hello<<-\!
		print $1
		!
	 ) != hello ]]
then	err_exit "${SHELL-ksh} -s not working"
fi
x=$(
	set -e
	false && print bad
	print good
)
if	[[ $x != good ]]
then	err_exit 'sh -e not working'
fi
[[ $($SHELL -D -c 'print hi; print $"hello"') == '"hello"' ]] || err_exit 'ksh -D not working'

rc=$tmp/.kshrc
print $'PS1=""\nfunction env_hit\n{\n\tprint OK\n}' > $rc

export ENV=/.$rc
if	[[ -o privileged ]]
then
	[[ $(print env_hit | $SHELL 2>&1) == "OK" ]] &&
		err_exit 'privileged noninteractive shell reads $ENV file'
	[[ $(print env_hit | $SHELL -E 2>&1) == "OK" ]] &&
		err_exit 'privileged -E reads $ENV file'
	[[ $(print env_hit | $SHELL +E 2>&1) == "OK" ]] &&
		err_exit 'privileged +E reads $ENV file'
	[[ $(print env_hit | $SHELL --rc 2>&1) == "OK" ]] &&
		err_exit 'privileged --rc reads $ENV file'
	[[ $(print env_hit | $SHELL --norc 2>&1) == "OK" ]] &&
		err_exit 'privileged --norc reads $ENV file'
else
	[[ $(print env_hit | $SHELL 2>&1) == "OK" ]] &&
		err_exit 'noninteractive shell reads $ENV file'
	[[ $(print env_hit | $SHELL -E 2>&1) == "OK" ]] ||
		err_exit '-E ignores $ENV file'
	[[ $(print env_hit | $SHELL +E 2>&1) == "OK" ]] &&
		err_exit '+E reads $ENV file'
	[[ $(print env_hit | $SHELL --rc 2>&1) == "OK" ]] ||
		err_exit '--rc ignores $ENV file'
	[[ $(print env_hit | $SHELL --norc 2>&1) == "OK" ]] &&
		err_exit '--norc reads $ENV file'
	if((!SHOPT_SCRIPTONLY));then
	[[ $(print env_hit | $SHELL -i 2>&1) == "OK" ]] ||
		err_exit '-i ignores $ENV file'
	fi # !SHOPT_SCRIPTONLY
fi

export ENV=/./dev/null
if	[[ -o privileged ]]
then
	[[ $(print env_hit | HOME=$tmp $SHELL 2>&1) == "OK" ]] &&
		err_exit 'privileged noninteractive shell reads $HOME/.kshrc file'
	[[ $(print env_hit | HOME=$tmp $SHELL -E 2>&1) == "OK" ]] &&
		err_exit 'privileged -E ignores empty $ENV'
	[[ $(print env_hit | HOME=$tmp $SHELL +E 2>&1) == "OK" ]] &&
		err_exit 'privileged +E reads $HOME/.kshrc file'
	[[ $(print env_hit | HOME=$tmp $SHELL --rc 2>&1) == "OK" ]] &&
		err_exit 'privileged --rc ignores empty $ENV'
	[[ $(print env_hit | HOME=$tmp $SHELL --norc 2>&1) == "OK" ]] &&
		err_exit 'privileged --norc reads $HOME/.kshrc file'
else
	[[ $(print env_hit | HOME=$tmp $SHELL 2>&1) == "OK" ]] &&
		err_exit 'noninteractive shell reads $HOME/.kshrc file'
	[[ $(print env_hit | HOME=$tmp $SHELL -E 2>&1) == "OK" ]] &&
		err_exit '-E ignores empty $ENV'
	[[ $(print env_hit | HOME=$tmp $SHELL +E 2>&1) == "OK" ]] &&
		err_exit '+E reads $HOME/.kshrc file'
	[[ $(print env_hit | HOME=$tmp $SHELL --rc 2>&1) == "OK" ]] &&
		err_exit '--rc ignores empty $ENV'
	[[ $(print env_hit | HOME=$tmp $SHELL --norc 2>&1) == "OK" ]] &&
		err_exit '--norc reads $HOME/.kshrc file'
fi

unset ENV
if	[[ -o privileged ]]
then
	[[ $(print env_hit | HOME=$tmp $SHELL 2>&1) == "OK" ]] &&
		err_exit 'privileged noninteractive shell reads $HOME/.kshrc file'
	[[ $(print env_hit | HOME=$tmp $SHELL -E 2>&1) == "OK" ]] &&
		err_exit 'privileged -E reads $HOME/.kshrc file'
	[[ $(print env_hit | HOME=$tmp $SHELL +E 2>&1) == "OK" ]] &&
		err_exit 'privileged +E reads $HOME/.kshrc file'
	[[ $(print env_hit | HOME=$tmp $SHELL --rc 2>&1) == "OK" ]] &&
		err_exit 'privileged --rc reads $HOME/.kshrc file'
	[[ $(print env_hit | HOME=$tmp $SHELL --norc 2>&1) == "OK" ]] &&
		err_exit 'privileged --norc reads $HOME/.kshrc file'
else
	[[ $(print env_hit | HOME=$tmp $SHELL 2>&1) == "OK" ]] &&
		err_exit 'noninteractive shell reads $HOME/.kshrc file'
	[[ $(set +x; print env_hit | HOME=$tmp $SHELL -E 2>&1) == "OK" ]] ||
		err_exit '-E ignores $HOME/.kshrc file'
	[[ $(print env_hit | HOME=$tmp $SHELL +E 2>&1) == "OK" ]] &&
		err_exit '+E reads $HOME/.kshrc file'
	[[ $(set +x; print env_hit | HOME=$tmp $SHELL --rc 2>&1) == "OK" ]] ||
		err_exit '--rc ignores $HOME/.kshrc file'
	[[ $(print env_hit | HOME=$tmp $SHELL --norc 2>&1) == "OK" ]] &&
		err_exit '--norc reads $HOME/.kshrc file'
fi

rm -rf $tmp/.kshrc

if	command set -G 2> /dev/null
then	mkdir bar foo
	> bar.c > bam.c
	> bar/foo.c > bar/bam.c
	> foo/bam.c
	set -- **.c
	expected='bam.c bar.c'
	[[ $* == $expected ]] ||
		err_exit "-G **.c failed -- expected '$expected', got '$*'"
	set -- **
	expected='bam.c bar bar.c bar/bam.c bar/foo.c foo foo/bam.c'
	[[ $* == $expected ]] ||
		err_exit "-G ** failed -- expected '$expected', got '$*'"
	set -- **/*.c
	expected='bam.c bar.c bar/bam.c bar/foo.c foo/bam.c'
	[[ $* == $expected ]] ||
		err_exit "-G **/*.c failed -- expected '$expected', got '$*'"
	set -- **/bam.c
	expected='bam.c bar/bam.c foo/bam.c'
	[[ $* == $expected ]] ||
		err_exit "-G **/bam.c failed -- expected '$expected', got '$*'"
fi

t="<$$>.profile.<$$>"
echo "echo '$t'" > .profile
cp $SHELL ./-ksh
if	[[ -o privileged ]]
then
	[[ $(HOME=$PWD $SHELL -l </dev/null 2>&1) == *$t* ]] &&
		err_exit 'privileged -l reads .profile'
	[[ $(HOME=$PWD $SHELL --login </dev/null 2>&1) == *$t* ]] &&
		err_exit 'privileged --login reads .profile'
	[[ $(HOME=$PWD $SHELL --login-shell </dev/null 2>&1) == *$t* ]] &&
		err_exit 'privileged --login-shell reads .profile'
	[[ $(HOME=$PWD $SHELL --login_shell </dev/null 2>&1) == *$t* ]] &&
		err_exit 'privileged --login_shell reads .profile'
	if((!execve_ignores_argv0));then
	[[ $(HOME=$PWD exec -a -ksh $SHELL </dev/null 2>&1) == *$t* ]] &&
		err_exit 'privileged exec -a -ksh ksh reads .profile'
	fi # !execve_ignores_argv0
	[[ $(HOME=$PWD ./-ksh -i </dev/null 2>&1) == *$t* ]] &&
		err_exit 'privileged ./-ksh reads .profile'
	[[ $(HOME=$PWD ./-ksh -ip </dev/null 2>&1) == *$t* ]] &&
		err_exit 'privileged ./-ksh -p reads .profile'
else
	[[ $(HOME=$PWD $SHELL -l </dev/null 2>&1) == *$t* ]] ||
		err_exit '-l ignores .profile'
	[[ $(HOME=$PWD $SHELL --login </dev/null 2>&1) == *$t* ]] ||
		err_exit '--login ignores .profile'
	[[ $(HOME=$PWD $SHELL --login-shell </dev/null 2>&1) == *$t* ]] ||
		err_exit '--login-shell ignores .profile'
	[[ $(HOME=$PWD $SHELL --login_shell </dev/null 2>&1) == *$t* ]] ||
		err_exit '--login_shell ignores .profile'
	if((!execve_ignores_argv0));then
	[[ $(HOME=$PWD exec -a -ksh $SHELL </dev/null 2>/dev/null) == *$t* ]] ||
		err_exit 'exec -a -ksh ksh 2>/dev/null ignores .profile'
	[[ $(HOME=$PWD exec -a -ksh $SHELL </dev/null 2>&1) == *$t* ]] ||
		err_exit 'exec -a -ksh ksh 2>&1 ignores .profile'
	fi # !execve_ignores_argv0
	if((!SHOPT_SCRIPTONLY));then
	[[ $(HOME=$PWD ./-ksh -i </dev/null 2>&1) == *$t* ]] ||
		err_exit './-ksh ignores .profile'
	fi # !SHOPT_SCRIPTONLY
	[[ $(HOME=$PWD ./-ksh -ip </dev/null 2>&1) == *$t* ]] &&
		err_exit './-ksh -p does not ignore .profile'
fi
rm .profile

# { exec interactive login_shell restricted xtrace } in the following test

set -- \
	allexport all-export all_export \
	bgnice bg-nice bg_nice \
	clobber \
	errexit err-exit err_exit \
	glob \
	globstar glob-star glob_star \
	ignoreeof ignore-eof ignore_eof \
	keyword log markdirs monitor notify \
	pipefail pipe-fail pipe_fail \
	trackall track-all track_all \
	unset verbose
((SHOPT_ESH && !SHOPT_SCRIPTONLY)) && set -- "$@" emacs gmacs
((SHOPT_VSH && !SHOPT_SCRIPTONLY)) && set -- "$@" vi
for opt
do	old=$opt
	if [[ ! -o $opt ]]
	then	old=no$opt
	fi

	set --$opt || err_exit "set --$opt failed"
	[[ -o $opt ]] || err_exit "[[ -o $opt ]] failed"
	[[ -o no$opt ]] && err_exit "[[ -o no$opt ]] failed"
	[[ -o no-$opt ]] && err_exit "[[ -o no-$opt ]] failed"
	[[ -o no_$opt ]] && err_exit "[[ -o no_$opt ]] failed"
	[[ -o ?$opt ]] || err_exit "[[ -o ?$opt ]] failed"
	[[ -o ?no$opt ]] || err_exit "[[ -o ?no$opt ]] failed"
	[[ -o ?no-$opt ]] || err_exit "[[ -o ?no-$opt ]] failed"
	[[ -o ?no_$opt ]] || err_exit "[[ -o ?no_$opt ]] failed"

	set --no$opt || err_exit "set --no$opt failed"
	[[ -o no$opt ]] || err_exit "[[ -o no$opt ]] failed"
	[[ -o $opt ]] && err_exit "[[ -o $opt ]] failed"

	set --no-$opt || err_exit "set --no-$opt failed"
	[[ -o no$opt ]] || err_exit "[[ -o no$opt ]] failed"
	[[ -o $opt ]] && err_exit "[[ -o $opt ]] failed"

	set --no_$opt || err_exit "set --no_$opt failed"
	[[ -o no$opt ]] || err_exit "[[ -o no$opt ]] failed"
	[[ -o $opt ]] && err_exit "[[ -o $opt ]] failed"

	set -o $opt || err_exit "set -o $opt failed"
	[[ -o $opt ]] || err_exit "[[ -o $opt ]] failed"
	set -o $opt=1 || err_exit "set -o $opt=1 failed"
	[[ -o $opt ]] || err_exit "[[ -o $opt ]] failed"
	set -o no$opt=0 || err_exit "set -o no$opt=0 failed"
	[[ -o $opt ]] || err_exit "[[ -o $opt ]] failed"
	set --$opt=1 || err_exit "set --$opt=1 failed"
	[[ -o $opt ]] || err_exit "[[ -o $opt ]] failed"
	set --no$opt=0 || err_exit "set --no$opt=0 failed"
	[[ -o $opt ]] || err_exit "[[ -o $opt ]] failed"

	set -o no$opt || err_exit "set -o no$opt failed"
	[[ -o no$opt ]] || err_exit "[[ -o no$opt ]] failed"
	set -o $opt=0 || err_exit "set -o $opt=0 failed"
	[[ -o no$opt ]] || err_exit "[[ -o no$opt ]] failed"
	set -o no$opt=1 || err_exit "set -o no$opt=1 failed"
	[[ -o no$opt ]] || err_exit "[[ -o no$opt ]] failed"
	set --$opt=0 || err_exit "set --$opt=0 failed"
	[[ -o no$opt ]] || err_exit "[[ -o no$opt ]] failed"
	set --no$opt=1 || err_exit "set --no$opt=1 failed"
	[[ -o no$opt ]] || err_exit "[[ -o no$opt ]] failed"

	set -o no-$opt || err_exit "set -o no-$opt failed"
	[[ -o no-$opt ]] || err_exit "[[ -o no-$opt ]] failed"

	set -o no_$opt || err_exit "set -o no_$opt failed"
	[[ -o no_$opt ]] || err_exit "[[ -o no_$opt ]] failed"

	set +o $opt || err_exit "set +o $opt failed"
	[[ -o no$opt ]] || err_exit "[[ -o no$opt ]] failed"

	set +o no$opt || err_exit "set +o no$opt failed"
	[[ -o $opt ]] || err_exit "[[ -o $opt ]] failed"

	set +o no-$opt || err_exit "set +o no-$opt failed"
	[[ -o $opt ]] || err_exit "[[ -o $opt ]] failed"

	set +o no_$opt || err_exit "set +o no_$opt failed"
	[[ -o $opt ]] || err_exit "[[ -o $opt ]] failed"

	set --$old
done

for opt in \
	exec interactive login_shell login-shell logi privileged \
	rc restricted xtrace
do	[[ -o $opt ]]
	y=$?
	[[ -o no$opt ]]
	n=$?
	case $y$n in
	10|01)	;;
	*)	err_exit "[[ -o $opt ]] == [[ -o no$opt ]]" ;;
	esac
done

for opt in \
	foo foo-bar foo_bar
do	if	[[ -o ?$opt ]]
	then	err_exit "[[ -o ?$opt ]] should fail"
	fi
	if	[[ -o ?no$opt ]]
	then	err_exit "[[ -o ?no$opt ]] should fail"
	fi
done

[[ $(set +o) == $(set --state) ]] || err_exit "set --state different from set +o"
set -- $(set --state)
[[ $1 == set && $2 == --default ]] || err_exit "set --state failed -- expected 'set --default *', got '$1 $2 *'"
shift
restore=$*
shift
off=
for opt
do	case $opt in
	--not*)	opt=${opt/--/--no} ;;
	--no*)	opt=${opt/--no/--} ;;
	--*)	opt=${opt/--/--no} ;;
	esac
	off="$off $opt"
done
set $off
state=$(set --state)
default=$(set --default --state)
[[ $state == $default ]] || err_exit "set --state for default options failed: expected '$default', got '$state'"
set $restore
state=$(set --state)
[[ $state == "set $restore" ]] || err_exit "set --state after restore failed: expected 'set $restore', got '$state'"

typeset -a pipeline
pipeline=(
	( nopipefail=0 pipefail=1 command='false|true|true' )
	( nopipefail=0 pipefail=1 command='true|false|true' )
	( nopipefail=1 pipefail=1 command='true|true|false' )
	( nopipefail=1 pipefail=1 command='false|false|false' )
	( nopipefail=0 pipefail=0 command='true|true|true' )
	( nopipefail=0 pipefail=0 command='print hi|(sleep .1;"$bincat")>/dev/null' )
)
set --nopipefail
for ((i = 0; i < ${#pipeline[@]}; i++ ))
do	eval ${pipeline[i].command}
	status=$?
	expected=${pipeline[i].nopipefail}
	[[ $status == $expected ]] ||
	err_exit "--nopipefail '${pipeline[i].command}' exit status $status -- expected $expected"
done
ftt=0
set --pipefail
for ((i = 0; i < ${#pipeline[@]}; i++ ))
do	eval ${pipeline[i].command}
	status=$?
	expected=${pipeline[i].pipefail}
	if	[[ $status != $expected ]]
	then	err_exit "--pipefail '${pipeline[i].command}' exit status $status -- expected $expected"
		(( i == 0 )) && ftt=1
	fi
done
if	(( ! ftt ))
then	exp=10
	got=$(for((n=1;n<exp;n++))do $SHELL --pipefail -c '(sleep 0.1;false)|true|true' && break; done; print $n)
	[[ $got == $exp ]] || err_exit "--pipefail -c '(sleep 0.1;false)|true|true' fails with exit status 0 (after $got/$exp iterations)"
fi

echo=$(whence -p echo)
for ((i=0; i < 20; i++))
do	if	! x=$(true | $echo 123)
	then	err_exit 'command substitution with wrong exit status with pipefai'
		break
	fi
done
(
	set -o pipefail
	false | true
	(( $? )) || err_exit 'pipe not failing in subshell with pipefail'
) | wc >/dev/null
$SHELL -c 'set -o pipefail; false | $(whence -p true);' && err_exit 'pipefail not returning failure with sh -c'
exp='1212 or 1221'
got=$(
	set --pipefail
	pipe() { date | cat > /dev/null ;}
	print $'1\n2' |
	while	read i
	do 	if	pipe $tmp
		then	{ print -n $i; sleep 2; print -n $i; } &
		fi
	done
	wait
)
[[ $got == @((12|21)(12|21)) ]] || err_exit "& job delayed by --pipefail, expected '$exp', got '$got'"
$SHELL -c '[[ $- == *c* ]]' || err_exit 'option c not in $-'
> $tmp/.profile
for i in $(let !SHOPT_SCRIPTONLY && echo i) l r s D E a b e f h k n t u v x \
	$(let SHOPT_BRACEPAT && echo B) C G $(let "SHOPT_HISTEXPAND && !SHOPT_SCRIPTONLY" && echo H)
do	HOME=$tmp ENV=/./dev/null $SHELL -$i >/dev/null 2>&1 <<- ++EOF++ || err_exit "option $i not in \$-"
	[[ \$- == *$i* ]] || exit 1
	++EOF++
done
letters=$(let !SHOPT_SCRIPTONLY && echo i)lrabefhknuvx$(let SHOPT_BRACEPAT && echo B)CGE
integer j=0
for i in $(let !SHOPT_SCRIPTONLY && echo interactive) \
	login restricted allexport notify errexit \
	noglob trackall keyword noexec nounset verbose xtrace \
	$(let SHOPT_BRACEPAT && echo braceexpand) \
	noclobber globstar rc
do	HOME=$tmp ENV=/./dev/null $SHELL -o $i >/dev/null 2>&1 <<- ++EOF++ || err_exit "option $i not equivalent to ${letters:j:1}"
	[[ \$- == *${letters:j:1}* ]] || exit 1
	++EOF++
	((j++))
done

if((!SHOPT_SCRIPTONLY));then
export ENV=/./dev/null PS1="(:$$:)"
histfile=$tmp/history
exp=$(HISTFILE=$histfile $SHELL -c $'function foo\n{\ncat\n}\ntype foo')
for var in HISTSIZE HISTFILE
do	got=$( set +x; ( HISTFILE=$histfile $SHELL +E -ic $'unset '$var$'\nfunction foo\n{\ncat\n}\ntype foo\nexit' ) 2>&1 )
	got=${got##*"$PS1"}
	[[ $got == "$exp" ]] || err_exit "function definition inside (...) with $var unset fails -- got '$got', expected '$exp'"
	got=$( set +x; { HISTFILE=$histfile $SHELL +E -ic $'unset '$var$'\nfunction foo\n{\ncat\n}\ntype foo\nexit' ;} 2>&1 )
	got=${got##*"$PS1"}
	[[ $got == "$exp" ]] || err_exit "function definition inside {...;} with $var unset fails -- got '$got', expected '$exp'"
done
( unset HISTFILE; $SHELL -ic "HISTFILE=$histfile" 2>/dev/null ) || err_exit "setting HISTFILE when not in environment fails"
fi # !SHOPT_SCRIPTONLY

# the next tests loop on all combinations of
#	{ SUB PAR CMD ADD }

SUB=(
	( BEG='$( '	END=' )'	)
	( BEG='${ '	END='; }'	)
)
PAR=(
	( BEG='( '	END=' )'	)
	( BEG='{ '	END='; }'	)
)
CMD=(	command-kill	script-kill	)
ADD=(	''		'; :'		)

print $'#!'$SHELL$'\nkill -KILL $$' > command-kill
print $'kill -KILL $$' > script-kill
chmod +x command-kill script-kill
export PATH=.:$PATH
exp='Killed'
exp2=$exp
expmsg="'$exp'"
unset exp2
if [[ $(uname) == Haiku ]]; then
	# On older versions of Haiku signal numbers 9 and 21
	# (SIGKILL and SIGKILLTHR) may both issue a 'Kill Thread'
	# message.
	exp2='Kill Thread'
	expmsg+=" or '$exp2'"
fi
for ((S=0; S<${#SUB[@]}; S++))
do	for ((P=0; P<${#PAR[@]}; P++))
	do	for ((C=0; C<${#CMD[@]}; C++))
		do	for ((A=0; A<${#ADD[@]}; A++))
			do	cmd="${SUB[S].BEG}${PAR[P].BEG}${CMD[C]}${PAR[P].END} 2>&1${ADD[A]}${SUB[S].END}"
				eval got="$cmd"
				got=${got##*': '}
				got=${got%%'('*}
				if [[ $got != "$exp" ]] && [[ $got != "$exp2" ]]
				then	err_exit "$cmd failed -- got '$got', expected $expmsg"
				fi
			done
		done
	done
done

$SHELL 2> /dev/null -c '{; true ;}' || err_exit 'leading ; causes syntax error in brace group'
$SHELL 2> /dev/null -c '(; true ;)' || err_exit 'leading ; causes syntax error in parenthesis group'

print 'for ((i = 0; i < ${1:-10000}; i++ )); do printf "%.*c\n" 15 x; done' > pipefail
chmod +x pipefail
$SHELL --pipefail -c './pipefail 10000 | sed 1q' >/dev/null 2>&1 &
tst=$!
{ sleep 4; kill $tst; } 2>/dev/null &
spy=$!
wait $tst 2>/dev/null
status=$?
if	[[ $status == 0 || $(kill -l $status) == PIPE ]]
then    kill $spy 2>/dev/null
else    err_exit "pipefail pipeline bypasses SIGPIPE and hangs"
fi
wait

[[ $($SHELL -uc '[[ "${d1.u[z asd].revents}" ]]' 2>&1) == *'d1.u[z asd].revents'* ]] || err_exit 'name of unset parameter not in error message'

[[ $($SHELL 2> /dev/null -xc $'set --showme\nprint 1\n; print 2') == 1 ]] || err_exit  'showme option with xtrace not working correctly'

$SHELL -uc 'var=foo;unset var;: ${var%foo}' >/dev/null 2>&1 && err_exit '${var%foo} should fail with set -u'
$SHELL -uc 'var=foo;unset var;: ${!var}' >/dev/null 2>&1 && err_exit '${!var} should fail with set -u'
$SHELL -uc 'var=foo;unset var;: ${#var}' >/dev/null 2>&1 && err_exit '${#var} should fail with set -u'
$SHELL -uc 'var=foo;unset var;: ${var-OK}' >/dev/null 2>&1 || err_exit '${var-OK} should not fail with set -u'
$SHELL -uc 'var=foo;unset var;: ${var:-OK}' >/dev/null 2>&1 || err_exit '${var:-OK} should not fail with set -u'
(set -u -- one two; : $2) 2>/dev/null || err_exit "an unset PP failed with set -u"
(set -u -- one two; : $3) 2>/dev/null && err_exit "a set PP failed to fail with set -u"
(set -u -- one two; : ${3%foo}) 2>/dev/null && err_exit '${3%foo} failed to fail with set -u'
(set -u -- one two; : ${3-OK}) 2>/dev/null || err_exit '${3-OK} wrongly failed with set -u'
(set -u -- one two; : ${3:-OK}) 2>/dev/null || err_exit '${3:-OK} wrongly failed with set -u'
(set -u -- one two; : ${#3}) 2>/dev/null && err_exit '${#3} failed to fail with set -u'
(set -u --; : $@ $*) 2>/dev/null || err_exit '$@ and/or $* fail to be exempt from set -u'
$SHELL -uc ': $!' 2>/dev/null && err_exit '$! failed to fail with set -u'
$SHELL -uc ': ${!%foo}' >/dev/null 2>&1 && err_exit '${!%foo} should fail with set -u'
$SHELL -uc ': ${#!}' >/dev/null 2>&1 && err_exit '${#!} should fail with set -u'
$SHELL -uc ': ${!-OK}' >/dev/null 2>&1 || err_exit '${!-OK} should not fail with set -u'
$SHELL -uc ': ${!:-OK}' >/dev/null 2>&1 || err_exit '${!:-OK} should not fail with set -u'

z=$($SHELL 2>&1 -uc 'print ${X23456789012345}')
[[ $z == *X23456789012345:* ]] || err_exit "error message garbled with set -u got $z"

# pipe hang bug fixed 2011-03-15
float start=SECONDS toolong=3
( export toolong
  $SHELL <<-EOF
	set -o pipefail
	(sleep $toolong;kill \$\$> /dev/null) &
	cat $SHELL | for ((i=0; i < 5; i++))
	do
		date | wc > /dev/null
		$SHELL -c 'read -N1'
	done
EOF
) 2> /dev/null
(( (SECONDS-start) > (toolong-0.5) )) && err_exit "pipefail causes script to hang"

# showme with arithmetic for loops
$SHELL -n -c $'for((;1;))\ndo ; nothing\ndone'  2>/dev/null  || err_exit 'showme commands give syntax error inside arithmetic for loops'

#set -x
float t1=SECONDS
set -o pipefail
print  | while read
do		if	{ date | true;} ; true
		then	sleep 2 &
		fi
done
(( (SECONDS-t1) > .5 )) && err_exit 'pipefail should not wait for background processes'

# ======
if ((!SHOPT_SCRIPTONLY)); then
print $'v=$(. ./dotfile)\n(. ./dotfile)\ncat <(. ./dotfile)\n. ./dotfile' > envfile
# dot scripts sourced from profile files are parsed line by line, so that aliases take effect on the next line in the same file
print $'alias print=:\nprint fail:subshell==${.sh.subshell} >&2' > dotfile
got=$(set +x; ENV=/.$PWD/envfile "$SHELL" -i -c : 2>&1)
exp=''
[[ $got == "$exp" ]] || err_exit 'dot script sourced from profile does not process aliases correctly' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
# $0 does not change in ksh functions within profile scripts
print 'function BAD { echo $0; }; BAD >&2' > dotfile
got=$(set +x; ENV=/.$PWD/envfile "$SHELL" -i -c : 2>&1)
exp=$SHELL$'\n'$SHELL$'\n'$SHELL$'\n'$SHELL
[[ $got == "$exp" ]] || err_exit '$0 in ksh function in profile script not correct' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
fi # !SHOPT_SCRIPTONLY

# ======
if [[ -o ?posix ]]; then
	(set +o posix; o1=${-/B/}; set -o posix; o2=${-/B/}; [[ $o1 == "$o2" ]]) || err_exit 'set -o posix affects $- expansion'
	(set +o posix; set --posix >/dev/null; [[ -o posix ]]) || err_exit "set --posix != set -o posix"
	(set -o posix; set --noposix; [[ -o posix ]]) && err_exit "set --noposix != set +o posix"
	(set -o posix +o letoctal; [[ -o letoctal ]]) && err_exit "failed to stop posix option from turning on letoctal"
if((SHOPT_BRACEPAT)); then
	(set +B; set -o posix -B; [[ -o braceexpand ]]) || err_exit "failed to stop posix option from turning off bracceexpand"
	(set --posix; [[ -o braceexpand ]]) && err_exit "set --posix fails to disable braceexpand"
	(set -o posix; [[ -o braceexpand ]]) && err_exit "set -o posix fails to disable braceexpand"
fi # SHOPT_BRACEPAT
	(set --default -o posix; [[ -o letoctal ]]) && err_exit "set --default failed to stop posix option from changing others"
	(set --posix; [[ -o letoctal ]]) || err_exit "set --posix fails to enable letoctal"
	(set -o posix; [[ -o letoctal ]]) || err_exit "set -o posix fails to enable letoctal"
	$SHELL --posix < <(echo 'exit 0') || err_exit "ksh fails to handle --posix during startup"
	$SHELL -o posix < <(echo 'exit 0') || err_exit "ksh fails to handle -o posix during startup"
fi

# ======
# ksh 93u+ did not honor 'monitor' option on command line (rhbz#960034)
"$SHELL" -m -c '[[ -o monitor ]]' || err_exit 'option -m on command line does not work'
"$SHELL" -o monitor -c '[[ -o monitor ]]' || err_exit 'option -o monitor on command line does not work'

# ======
# Brace expansion could not be turned off in command substitutions (rhbz#1078698)
if((SHOPT_BRACEPAT)); then
set -B
expect='test{1,2}'
actual=$(set +B; echo `echo test{1,2}`)
[[ $actual == "$expect" ]] || err_exit 'Brace expansion not turned off in `comsub`' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
actual=$(set +B; echo $(echo test{1,2}))
[[ $actual == "$expect" ]] || err_exit 'Brace expansion not turned off in $(comsub)' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
actual=$(set +B; echo ${ echo test{1,2}; })
[[ $actual == "$expect" ]] || err_exit 'Brace expansion not turned off in ${ comsub; }' \
	"(expected $(printf %q "$expect"), got $(printf %q "$actual"))"
fi # SHOPT_BRACEPAT

# ======
# ksh 93u+ did not correctly handle the combination of pipefail and (errexit or the ERR trap).
# https://github.com/ksh93/ksh/issues/121
got=$("$SHELL" -o errexit -o pipefail -c '(exit 3) | true; echo "still here despite $? status"' 2>&1)
let "(e=$?)==3" && [[ -z $got ]] || err_exit 'errexit + pipefail failed' \
	"(expected status 3, ''; got status $e, $(printf %q "$got"))"
got=$("$SHELL" -o pipefail -c 'trap "print ERR\ trap" ERR; true | exit 3 | false | true | true' 2>&1)
let "(e=$?)==1" && [[ $got == 'ERR trap' ]] || err_exit 'ERR trap + pipefail failed' \
	"(expected status 1, 'ERR trap'; got status $e, $(printf %q "$got"))"

# ======
# Basic test for 'ksh -v' backported from ksh93v- 2013-09-13
exp=:
got=$("$SHELL" -vc : 2>&1)
[[ $exp == $got ]] || err_exit 'incorrect output with ksh -v' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# Tests that set -m puts background jobs in a separate process group.
LINENO=$LINENO "$SHELL" -m <<- \EOF
	. "${SHTESTS_COMMON:-${0%/*}/_common}"
	[[ $- == *m* ]] || err_exit '$- does not contain m when monitor mode specified'
	float t=SECONDS
	sleep 2 & pid=$!
	kill -KILL -$pid 2> /dev/null || err_exit 'kill to background group failed'
	wait 2> /dev/null
	(( (SECONDS-t) > 1 )) && err_exit 'kill did not kill background sleep'
	exit $Errors
EOF
((Errors+=$?))

# ======
# Test for 'set -u' from ksh93v- 2013-04-09
"$SHELL" 2> /dev/null <<- \EOF && err_exit 'unset variable with set -u on does not terminate script'
	set -e -u -o pipefail
	ls | while read file
	do
		files[${#files[*]}]=$fil
	done
	exit
EOF

# ======
# https://github.com/ksh93/ksh/issues/449
set +o monitor +o pipefail
for opt in monitor pipefail
do
	outfile=out$opt
	exp="ok $opt"
	(
		set +x -o "$opt"
		(
			sleep .01
			print "$exp" >&2
		) 2>$outfile | "${ whence -p true; }"  # the external 'true' should not be exec-optimized
	) &
	wait "$!"
	got=$(<$outfile)
	[[ $got == "$exp" ]] || err_exit "shell did not wait for entire pipeline with -o $opt" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
done

# ======
# showme only printed the first redirection in a list
# https://github.com/ksh93/ksh/issues/753
got=$(set +x --showme; PS4='+ '; eval ';true >/dev/null 2>&1 3>&1 4>&3' 2>&1)
exp=$'+ true\n+ 1> /dev/null 2>& 1 3>& 1 4>& 3'
[[ $got == "$exp" ]] || err_exit "showme doesn't print redirects properly" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# pipefail did not set 9th bit in exit status if a process got signalled
# https://github.com/ksh93/ksh/discussions/755#discussioncomment-9925394
got=$(set --pipefail; "$SHELL" -c 'kill -s PIPE $$' | true; echo $?)
exp=$(( ${ kill -l PIPE; } + 256 ))
[[ $got == "$exp" ]] || err_exit "status of signalled process in pipe with pipefail (expected $exp, got $got)"
unset exp got

# ======
# ksh -n should flag all obsolete set(1) and alias(1) arguments
if((SHOPT_VSH)); then
script=$tmp/setscr.$RANDOM.ksh
echo 'set -k -n +t	# Obsolete
set --keyword		# Obsolete
set --log		# Obsolete
set -tn			# Obsolete
set --nobackslashctrl	# Current
set --trackall		# Obsolete
set -be			# Current
set --errexit --nolog	# Obsolete
set -o keyword		# Obsolete
set -o backslashctrl -v # Current
set -o backslashctrl -h # Obsolete
set -o viraw -m		# Obsolete
set +o nokeyword +h	# Obsolete
alias -x foo		# Obsolete
alias -pt -tx		# Obsolete
alias -p		# Current
alias -t -pxt -xp	# Obsolete
set -eku		# Obsolete
set -ebku		# Obsolete
set -o posix -emu	# Current
set -			# Obsolete
set +			# Obsolete
set --			# Current' > "$script"
exp=20
got=($(wc -l <(set +x; "$SHELL" -n "$script" 2>&1)))
[[ ${got[0]} -eq $exp ]] || err_exit "ksh -n is not flagging all instances of obsolete options to set and alias" \
	"(expected $exp warnings, got ${got[0]} warnings)"
unset script exp got
fi

# ======
exit $((Errors<125?Errors:125))

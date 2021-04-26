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
	[[ $(print env_hit | $SHELL -i 2>&1) == "OK" ]] ||
		err_exit '-i ignores $ENV file'
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
	[[ $(HOME=$PWD exec -a -ksh $SHELL </dev/null 2>&1) == *$t* ]] &&
		err_exit 'privileged exec -a -ksh ksh reads .profile'
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
	[[ $(HOME=$PWD exec -a -ksh $SHELL </dev/null 2>/dev/null) == *$t* ]] ||
		err_exit 'exec -a -ksh ksh 2>/dev/null ignores .profile'
	[[ $(HOME=$PWD exec -a -ksh $SHELL </dev/null 2>&1) == *$t* ]] ||
		err_exit 'exec -a -ksh ksh 2>&1 ignores .profile'
	[[ $(HOME=$PWD ./-ksh -i </dev/null 2>&1) == *$t* ]] ||
		err_exit './-ksh ignores .profile'
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
((SHOPT_ESH)) && set -- "$@" emacs gmacs
((SHOPT_VSH)) && set -- "$@" vi viraw vi-raw vi_raw
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
for i in i l r s D E a b e f h k n t u v x $(let SHOPT_BRACEPAT && echo B) C G $(let SHOPT_HISTEXPAND && echo H)
do	HOME=$tmp ENV=/./dev/null $SHELL -$i >/dev/null 2>&1 <<- ++EOF++ || err_exit "option $i not in \$-"
	[[ \$- == *$i* ]] || exit 1
	++EOF++
done
letters=ilrabefhknuvx$(let SHOPT_BRACEPAT && echo B)CGE
integer j=0
for i in interactive login restricted allexport notify errexit \
	noglob trackall keyword noexec nounset verbose xtrace \
	$(let SHOPT_BRACEPAT && echo braceexpand) \
	noclobber globstar rc
do	HOME=$tmp ENV=/./dev/null $SHELL -o $i >/dev/null 2>&1 <<- ++EOF++ || err_exit "option $i not equivalent to ${letters:j:1}"
	[[ \$- == *${letters:j:1}* ]] || exit 1
	++EOF++
	((j++))
done

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
for ((S=0; S<${#SUB[@]}; S++))
do	for ((P=0; P<${#PAR[@]}; P++))
	do	for ((C=0; C<${#CMD[@]}; C++))
		do	for ((A=0; A<${#ADD[@]}; A++))
			do	cmd="${SUB[S].BEG}${PAR[P].BEG}${CMD[C]}${PAR[P].END} 2>&1${ADD[A]}${SUB[S].END}"
				eval got="$cmd"
				got=${got##*': '}
				got=${got%%'('*}
				[[ $got == "$exp" ]] || err_exit "$cmd failed -- got '$got', expected '$exp'"
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

# process source files from profiles as profile files
print '. ./dotfile' > envfile
print $'alias print=:\nprint foobar' > dotfile
[[ $(ENV=/.$PWD/envfile $SHELL -i -c : 2>/dev/null) == foobar ]] && err_exit 'files source from profile does not process aliases correctly'

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
exit $((Errors<125?Errors:125))

########################################################################
#                                                                      #
#              This file is part of the ksh 93u+m package              #
#          Copyright (c) 2020-2021 Contributors to ksh 93u+m           #
#                    <https://github.com/ksh93/ksh>                    #
#                      and is licensed under the                       #
#                 Eclipse Public License, Version 1.0                  #
#                                                                      #
#                A copy of the License is available at                 #
#          http://www.eclipse.org/org/documents/epl-v10.html           #
#         (with md5 checksum b35adb5213ca9657e911e9befb180842)         #
#                                                                      #
#             Johnothan King <johnothanking@protonmail.com>            #
#                                                                      #
########################################################################

# These are regression tests for the local and declare builtins.
# In the cases when local could return an error, it's run using
# 'command' because it's a special builtin in ksh93v- and ksh2020.

function err_exit
{
	print -u2 -n "\t"
	print -u2 -r ${Command}[$1]: "${@:2}"
	(( Errors+=1 ))
}
alias err_exit='err_exit $LINENO'

Command=${0##*/}
integer Errors=0

# ======
# This test must be run first due to the next test.
command local 2> /dev/null && err_exit "'local' works outside of functions"

# local shouldn't suddenly work outside of functions after a function runs local.
posix_dummy() { command local > /dev/null; }
function ksh_dummy { command local > /dev/null; }
posix_dummy && command local 2> /dev/null && err_exit 'the local builtin works outside of functions after a POSIX function runs local'
ksh_dummy && command local 2> /dev/null && err_exit 'the local builtin works outside of functions after a KornShell function runs local'

# ======
for i in declare local; do
	# local should work inside both kinds of functions, without reliance on environment variables.
	function ksh_function_nounset {
		command $i foo=bar 2>&1
	}
	function ksh_function_unset {
		unset .sh.fun
		command $i foo=bar 2>&1
	}
	posix_function_nounset() {
		command $i foo=bar 2>&1
	}
	posix_function_unset() {
		unset .sh.fun
		command $i foo=bar 2>&1
	}
	[[ $(ksh_function_nounset) ]] && err_exit "'$i' fails inside of KornShell functions"
	[[ $(ksh_function_unset) ]] && err_exit "'$i' fails inside of KornShell functions when \${.sh.fun} is unset"
	[[ $(posix_function_nounset) ]] && err_exit "'$i' fails inside of POSIX functions"
	[[ $(posix_function_unset) ]] && err_exit "'$i' fails inside of POSIX functions when \${.sh.fun} is unset"

	# The local and declare builtins should have a dynamic scope
	# Tests for the scope of POSIX functions
	foo=globalscope
	subfunc() {
		[[ $foo == dynscope ]]
	}
	mainfunc() {
		$i foo=dynscope
		subfunc
	}
	mainfunc || err_exit "'$i' is not using a dynamic scope in POSIX functions"
	# TODO: Implement scoping in POSIX functions
	#[[ $foo == globalscope ]] || err_exit "'$i' changes variables outside of a POSIX function's scope"

	# Tests for the scope of KornShell functions
	bar=globalscope
	function subfunc_b {
		[[ $bar == dynscope ]]
	}
	function mainfunc_b {
		$i bar=dynscope
		subfunc_b
	}
	mainfunc_b || err_exit "'$i' is not using a dynamic scope in KornShell functions"
	[[ $bar == globalscope ]] || err_exit "'$i' changes variables outside of a KornShell function's scope"
done

# The declare builtin should work outside of functions
unset foo
declare foo=bar
[[ $foo == bar ]] || err_exit "'declare' doesn't work outside of functions"

# ======
exit $((Errors<125?Errors:125))

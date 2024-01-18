########################################################################
#                                                                      #
#              This file is part of the ksh 93u+m package              #
#          Copyright (c) 2020-2024 Contributors to ksh 93u+m           #
#                    <https://github.com/ksh93/ksh>                    #
#                      and is licensed under the                       #
#                 Eclipse Public License, Version 2.0                  #
#                                                                      #
#                A copy of the License is available at                 #
#      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      #
#         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         #
#                                                                      #
#            Johnothan King <johnothanking@protonmail.com>             #
#                                                                      #
########################################################################

# These are regression tests for the local and declare builtins.
# In the cases when local could return an error, it's run using
# 'command' because it's a special builtin in ksh93v- and ksh2020.

. "${SHTESTS_COMMON:-${0%/*}/_common}"

# ======
# These tests must run first for the subsequent tests to properly detect aberrant functionality
# in the buggy ksh93v-/ksh2020 local builtin.
command local 2> /dev/null && err_exit "'local' works outside of functions"
local 2> /dev/null && err_exit "'local' works outside of functions"

# local shouldn't suddenly work outside of functions after a function runs local.
posix_dummy() { command local > /dev/null; }
posix_dummy_two() { local > /dev/null; }
function ksh_dummy { command local > /dev/null; }
function ksh_dummy_two { local > /dev/null; }
posix_dummy && command local 2> /dev/null && err_exit 'the local builtin works outside of functions after a POSIX function runs local'
posix_dummy_two && local 2> /dev/null && err_exit 'the local builtin works outside of functions after a POSIX function runs local'
ksh_dummy && command local 2> /dev/null && err_exit 'the local builtin works outside of functions after a KornShell function runs local'
ksh_dummy_two && local 2> /dev/null && err_exit 'the local builtin works outside of functions after a KornShell function runs local'

# ======
for i in declare local; do
	unset foo bar
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
	got=$(ksh_function_nounset)
	[[ -n $got ]] && err_exit "'$i' fails inside of KornShell functions" \
		"(got ${ printf %q "$got" })"
	got=$(ksh_function_unset)
	[[ -n $got ]] && err_exit "'$i' fails inside of KornShell functions when \${.sh.fun} is unset" \
		"(got ${ printf %q "$got" })"
	got=$(posix_function_nounset)
	[[ -n $got ]] && err_exit "'$i' fails inside of POSIX functions" \
		"(got ${ printf %q "$got" })"
	got=$(posix_function_unset)
	[[ -n $got ]] && err_exit "'$i' fails inside of POSIX functions when \${.sh.fun} is unset" \
		"(got ${ printf %q "$got" })"

	# The local and declare builtins should have a dynamic scope
	# Tests for the scope of POSIX functions
	foo=globalscope
	exp=dynscope
	subfunc() {
		print $foo
	}
	mainfunc() {
		$i foo=$exp
		subfunc
	}
	got=${ mainfunc }
	[[ $exp == $got ]] || err_exit "'$i' is not using a dynamic scope in POSIX functions" \
			"(expected $exp, got $(printf %q "$got"))"
	[[ $foo == globalscope ]] || err_exit "'$i' changes variables outside of a POSIX function's scope" \
			"(expected globalscope, got $(printf %q "$foo"))"

	# Tests for the scope of KornShell functions
	bar=globalscope
	exp=dynscope
	function subfunc_b {
		print $bar
	}
	function mainfunc_b {
		$i bar=dynscope
		subfunc_b
	}
	got=${ mainfunc_b }
	[[ $exp == $got ]] || err_exit "'$i' is not using a dynamic scope in KornShell functions" \
			"(expected $exp, got $(printf %q "$got"))"
	[[ $bar == globalscope ]] || err_exit "'$i' changes variables outside of a KornShell function's scope" \
			"(expected globalscope, got $(printf %q "$bar"))"
done

got=$(command declare -cDg invalid=cannotset 2>&1)
status=$?
((status == 2)) || err_exit "attempting to combine -c, -D and -g doesn't fail correctly" \
			"(returned exit status $status and output $(printf %q "$got"))"

# The declare builtin should work outside of functions
unset foo
declare foo=bar
[[ $foo == bar ]] || err_exit "'declare' doesn't work outside of functions"

# Run all of the tests with 'command ' prefixes, as
# the underlying code treats 'typeset' and 'command typeset'
# differently (see sh_exec()).

for command in "" "command"; do
	unset prefix
	[[ $command == command ]] && prefix="(using command(1)) "

	# Test 1: Make dynamic $bar static with typeset(1) in ksh functions
	tst=$tmp/tst.sh
	cat > "$tst" <<-EOF
	function nxt {
		echo \$bar
	}
	function foo {
		$command local bar=1
		nxt
		$command local bar=BAD1
		function infun {
			echo \$bar
		}
		# The current implementation does not make a seperate version of var for the static scope.
		# Rather, it changes foo()'s $bar variable back to a static scope, which prevents it from being
		# accessed by called functions as $bar is no longer in a dynamic scope. Consequently, both infun
		# and nxt are expected to print only a newline.
		$command typeset bar=BAD2
		infun
		nxt
	}
	foo
	echo \$bar
	EOF
	exp=1
	got=$("$SHELL" "$tst")
	[[ $exp == "$got" ]] || err_exit "${prefix}Cannot switch from dynamic scope to static scope in ksh functions" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# Test 2: Make static $bar dynamic with local(1) in ksh functions
	cat > "$tst" <<-EOF
	function nxt {
		echo \$bar
	}
	function foo {
		$command typeset bar=BAD
		nxt
		$command local bar=1
		function infun {
			echo \$bar
		}
		infun
	}
	foo
	echo \${bar}2
	EOF
	exp=$'\n1\n2'
	got=$("$SHELL" "$tst")
	[[ $exp == "$got" ]] || err_exit "${prefix}Cannot switch from static scope to dynamic scope in ksh functions" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# Test 3: Make dynamic $bar global with typeset(1) in POSIX functions
	tst=$tmp/tst.sh
	cat > "$tst" <<-EOF
	nxt() {
		echo \$bar
	}
	foo() {
		$command local bar=1
		nxt
		$command typeset bar=3
		$command declare bar=2
		function infun {
			echo \$bar
		}
		infun
	}
	foo
	echo \$bar
	EOF
	exp=$'1\n2\n3'
	got=$("$SHELL" "$tst")
	[[ $exp == "$got" ]] || err_exit "${prefix}Cannot switch from dynamic scope to global scope in POSIX functions" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# Test 4: Make dynamic local bar separate from global bar created with typeset in POSIX functions
	cat > "$tst" <<-EOF
	nxt() {
		echo \$bar
	}
	foo() {
		$command typeset bar=1
		nxt
		$command typeset bar=3
		$command local bar=2
		function infun {
			echo \$bar
		}
		infun
	}
	foo
	echo \$bar
	EOF
	exp=$'1\n2\n3'
	got=$("$SHELL" "$tst")
	[[ $exp == "$got" ]] || err_exit "${prefix}Cannot create local version of \$bar in POSIX functions" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# Test 5: Ensure local actually works in POSIX functions
	cat > "$tst" <<-EOF
	nxt() {
		echo \$bar
	}
	foo() {
		$command local bar=1
		nxt
		$command local bar=2
		function infun {
			echo \$bar
		}
		infun
		$command local bar=BAD
	}
	foo
	echo \${bar}3
	EOF
	exp=$'1\n2\n3'
	got=$("$SHELL" "$tst")
	[[ $exp == "$got" ]] || err_exit "${prefix}Cannot create local variables in POSIX functions" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# Test 6: Ensure typeset doesn't default to making static variables in POSIX functions
	cat > "$tst" <<-EOF
	nxt() {
		echo \$bar
	}
	foo() {
		$command typeset bar=1
		nxt
		$command typeset bar=2
		function infun {
			echo \$bar
		}
		infun
		$command typeset bar=3
	}
	foo
	echo \$bar
	EOF
	exp=$'1\n2\n3'
	got=$("$SHELL" "$tst")
	[[ $exp == "$got" ]] || err_exit "${prefix}Cannot create global variables with plain typeset invocation in POSIX functions" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# Test 7: A plain typeset invocation should use the global scope for maximum backward compatibility
	cat > "$tst" <<-EOF
	nxt() {
		echo \$bar
	}
	foo() {
		$command typeset bar=1
		nxt
		$command typeset bar=2
		infun() {
			echo \$bar
		}
		infun
		$command typeset bar=3
	}
	foo
	echo \$bar
	EOF
	exp=$'1\n2\n3'
	got=$("$SHELL" "$tst")
	[[ $exp == "$got" ]] || err_exit "${prefix}Cannot create global variables with plain typeset invocation in POSIX functions with nested POSIX functions" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# Test 8: A plain invocation of local or declare should create a dynamic local variable
	cat > "$tst" <<-EOF
	nxt() {
		echo \$bar
	}
	foo() {
		$command local bar=1
		nxt
		$command declare bar=2
		infun() {
			echo \$bar
		}
		infun
		$command local bar=BAD
	}
	foo
	echo \${bar}3
	EOF
	exp=$'1\n2\n3'
	got=$("$SHELL" "$tst")
	[[ $exp == "$got" ]] || err_exit "${prefix}Cannot create local variables with local builtin in POSIX functions" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# Test 9: Create global variables without directly invoking typeset
	cat > "$tst" <<-EOF
	nxt() {
		echo \$bar
	}
	foo() {
		bar=1
		nxt
		bar=2
		function infun {
			echo \$bar
		}
		infun
		bar=3
	}
	foo
	echo \$bar
	EOF
	exp=$'1\n2\n3'
	got=$("$SHELL" "$tst")
	[[ $exp == "$got" ]] || err_exit "${prefix}Cannot create global variables in POSIX functions without direct typeset invocation" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# Test 10: Make static variables global in KornShell functions
	tst=$tmp/tst.sh
	cat > "$tst" <<-EOF
	function nxt {
		echo \${bar}1
	}
	function foo {
		$command typeset bar=BAD
		nxt
		$command typeset -g bar=2
		function infun {
			echo \$bar
		}
		infun
		$command typeset -g bar=3
	}
	foo
	echo \$bar
	EOF
	exp=$'1\n2\n3'
	got=$("$SHELL" "$tst")
	[[ $exp == "$got" ]] || err_exit "${prefix}Cannot switch from static scope to global scope in KornShell functions" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# Test 11: Make dynamic variables global in KornShell functions
	tst=$tmp/tst.sh
	cat > "$tst" <<-EOF
	function nxt {
		echo \${bar}
	}
	function foo {
		$command local bar=1
		nxt
		$command local -g bar=3
		$command local bar=2
		function infun {
			# The dynamic scope still applies, so the $bar value
			# from 'function foo' is inherited and used instead
			# of the global value.
			echo \$bar
		}
		infun
	}
	foo
	# This will be '3' because of the earlier 'local -g'
	echo \$bar
	EOF
	exp=$'1\n2\n3'
	got=$("$SHELL" "$tst")
	[[ $exp == "$got" ]] || err_exit "${prefix}Cannot switch from dynamic scope to global scope in KornShell functions" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# Test 12: Dynamic variables shouldn't leak out of nested POSIX functions
	tst=$tmp/tst.sh
	cat > "$tst" <<-EOF
	foo() {
		$command local foo=foo
		bar() {
			$command local foo=bar
			baz() {
				$command local foo=baz
				echo \$foo
			}
			baz
			echo \$foo
		}
		bar
		echo \$foo
	}
	foo
	EOF
	exp=$'baz\nbar\nfoo'
	got=$("$SHELL" "$tst")
	[[ $exp == "$got" ]] || err_exit "${prefix}Local variables from nested POSIX functions leak out into the parent functions" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# Test 13: Variables shouldn't leak out of nested KornShell functions
	tst=$tmp/tst.sh
	cat > "$tst" <<-EOF
	function foo {
		$command local foo=foo
		function bar {
			$command local foo=bar
			function baz {
				$command local foo=baz
				echo \$foo
			}
			baz
			echo \$foo
		}
		bar
		echo \$foo
	}
	foo
	EOF
	exp=$'baz\nbar\nfoo'
	got=$("$SHELL" "$tst")
	[[ $exp == "$got" ]] || err_exit "${prefix}Local variables from nested KornShell functions leak out into the parent functions" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# Test 14: Dynamic variables shouldn't leak out into other POSIX functions
	tst=$tmp/tst.sh
	cat > "$tst" <<-EOF
	baz() {
		$command local foo=baz
		echo \$foo
	}
	bar() {
		$command local foo=bar
		baz
		echo \$foo
	}
	foo() {
		$command local foo=foo
		bar
		echo \$foo
	}
	foo
	EOF
	exp=$'baz\nbar\nfoo'
	got=$("$SHELL" "$tst")
	[[ $exp == "$got" ]] || err_exit "${prefix}Local variables from POSIX functions leak out into other functions" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# Test 15: Variables shouldn't leak out into other KornShell functions
	tst=$tmp/tst.sh
	cat > "$tst" <<-EOF
	function baz {
		$command local foo=baz
		echo \$foo
	}
	function bar {
		$command local foo=bar
		baz
		echo \$foo
	}
	function foo {
		$command local foo=foo
		bar
		echo \$foo
	}
	foo
	EOF
	exp=$'baz\nbar\nfoo'
	got=$("$SHELL" "$tst")
	[[ $exp == "$got" ]] || err_exit "${prefix}Local variables from KornShell functions leak out into other functions" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# Test 16: 'typeset -c' should use static scoping in POSIX functions
	tst=$tmp/tst.sh
	cat > "$tst" <<-EOF
	foo=global
	bar() {
		echo \$foo
		$command typeset -c foo=static2
		echo \$foo
	}
	foo() {
		$command typeset -c foo=static
		echo \$foo
		bar
		echo \$foo
	}
	echo \$foo
	foo
	echo \$foo
	EOF
	exp=$'global\nstatic\nglobal\nstatic2\nstatic\nglobal'
	got=$("$SHELL" "$tst")
	[[ $exp == "$got" ]] || err_exit "${prefix}Cannot use static scoping in POSIX functions with 'local -c'" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# Test 17: 'local -c' should use static scoping in KornShell functions
	tst=$tmp/tst.sh
	cat > "$tst" <<-EOF
	foo=global
	function bar {
		echo \$foo
		$command local -c foo=static2
		echo \$foo
	}
	function foo {
		$command local -c foo=static
		echo \$foo
		bar
		echo \$foo
	}
	echo \$foo
	foo
	echo \$foo
	EOF
	exp=$'global\nstatic\nglobal\nstatic2\nstatic\nglobal'
	got=$("$SHELL" "$tst")
	[[ $exp == "$got" ]] || err_exit "${prefix}Cannot use static scoping in KornShell functions with 'local -c'" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# Test 18: Make static $bar dynamic in POSIX functions
	cat > "$tst" <<-EOF
	nxt() {
		echo \$bar
	}
	foo() {
		$command typeset -c bar=BAD
		nxt
		$command local bar=1
		function infun {
			echo \$bar
		}
		infun
	}
	foo
	echo \${bar}2
	EOF
	exp=$'\n1\n2'
	got=$("$SHELL" "$tst")
	[[ $exp == "$got" ]] || err_exit "${prefix}Cannot switch from static scope to dynamic scope in POSIX functions" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# Test 19: Make static $bar separate from global $bar in POSIX functions
	cat > "$tst" <<-EOF
	typeset bar=global
	nxt() {
		echo \$bar
	}
	foo() {
		$command local -c bar=static
		nxt
		echo \$bar
	}
	echo \${bar}
	foo
	echo \${bar}
	EOF
	exp=$'global\nglobal\nstatic\nglobal'
	got=$("$SHELL" "$tst")
	[[ $exp == "$got" ]] || err_exit "${prefix}Cannot separate static and global scopes in POSIX functions" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# Test 20: Ksh functions run by '.' should not have any form of scoping
	cat > "$tst" <<-EOF
	function nxt {
		echo \$bar
	}
	function foo {
		$command typeset bar=1
		nxt
		$command local foo=2
		function infun {
			echo \$foo
		}
		infun
		$command declare -c baz=3
		$command typeset -g bear=4
	}
	. foo
	print \$bar \$foo \$baz \$bear
	EOF
	exp=$'1\n2\n1 2 3 4'
	got=$("$SHELL" "$tst")
	[[ $exp == "$got" ]] || err_exit "${prefix}KornShell functions executed directly by '.' shouldn't have scoping" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# Test 21: typeset -p should print the -D flag when a variable has dynamic scoping applied to it
	cat > "$tst" <<-EOF
	function foo {
		$command declare foo=foo
		$command local -l -i bar=2
		$command typeset -p foo bar
	}
	foo
	EOF
	exp=$'typeset -D foo=foo\ntypeset -D -l -i bar=2'
	got=$("$SHELL" "$tst")
	[[ $exp == "$got" ]] || err_exit "${prefix}'typeset -p' cannot add the -D flag to output for variables given a dynamic scope" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
done

# ======
exit $((Errors<125?Errors:125))

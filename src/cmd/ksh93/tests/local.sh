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
# This test must be run first due to the next test.
command local 2> /dev/null && err_exit "'local' works outside of functions"

# local shouldn't suddenly work outside of functions after a function runs local.
posix_dummy() { command local > /dev/null; }
function ksh_dummy { command local > /dev/null; }
posix_dummy && command local 2> /dev/null && err_exit 'the local builtin works outside of functions after a POSIX function runs local'
ksh_dummy && command local 2> /dev/null && err_exit 'the local builtin works outside of functions after a KornShell function runs local'

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
	[[ $(ksh_function_nounset) ]] && err_exit "'$i' fails inside of KornShell functions"
	[[ $(ksh_function_unset) ]] && err_exit "'$i' fails inside of KornShell functions when \${.sh.fun} is unset"
	[[ $(posix_function_nounset) ]] && err_exit "'$i' fails inside of POSIX functions"
	[[ $(posix_function_unset) ]] && err_exit "'$i' fails inside of POSIX functions when \${.sh.fun} is unset"

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

# The declare builtin should work outside of functions
unset foo
declare foo=bar
[[ $foo == bar ]] || err_exit "'declare' doesn't work outside of functions"

# TODO: Implement optional static scoping via a flag, for use in POSIX functions.

# Test 1: make dynamic $bar static with typeset(1) in ksh function
tst=$tmp/tst.sh
cat > "$tst" << 'EOF'
function nxt {
	echo $bar
}
function foo {
	local bar=1
	nxt
	local bar=BAD1
	function infun {
		echo $bar
	}
	# The current implementation does not make a seperate version of var for the static scope.
	# Rather, it changes foo()'s $bar variable back to a static scope, which prevents it from being
	# accessed by called functions as $bar is no longer in a dynamic scope. Consequently, both infun
	# and nxt are expected to print only a newline.
	typeset bar=BAD2
	infun
	nxt
}
foo
echo $bar
EOF
exp=1
got=$("$SHELL" "$tst")
[[ $exp == "$got" ]] || err_exit "Cannot switch from dynamic scope to static scope in ksh functions" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Test 2: make static $bar dynamic with local(1) in ksh function
cat > "$tst" << 'EOF'
function nxt {
	echo $bar
}
function foo {
	typeset bar=BAD
	nxt
	local bar=1
	function infun {
		echo $bar
	}
	infun
}
foo
echo ${bar}2
EOF
exp=$'\n1\n2'
got=$("$SHELL" "$tst")
[[ $exp == "$got" ]] || err_exit "Cannot switch from static scope to dynamic scope in ksh functions" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Test 3: make dynamic $bar global with typeset(1) in POSIX function
tst=$tmp/tst.sh
cat > "$tst" << 'EOF'
nxt() {
	echo $bar
}
foo() {
	local bar=1
	nxt
	typeset bar=3
	declare bar=2
	function infun {
		echo $bar
	}
	infun
}
foo
echo $bar
EOF
exp=$'1\n2\n3'
got=$("$SHELL" "$tst")
[[ $exp == "$got" ]] || err_exit "Cannot switch from dynamic scope to global scope in POSIX functions" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Test 4: make local duplicate of global bar created with typeset in POSIX function
cat > "$tst" << 'EOF'
nxt() {
	echo $bar
}
foo() {
	typeset bar=1
	nxt
	typeset bar=3
	local bar=2
	function infun {
		echo $bar
	}
	infun
}
foo
echo $bar
EOF
exp=$'1\n2\n3'
got=$("$SHELL" "$tst")
[[ $exp == "$got" ]] || err_exit "Cannot create local version of \$bar in POSIX functions" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Test 5: ensure local really works in POSIX functions
cat > "$tst" << 'EOF'
nxt() {
	echo $bar
}
foo() {
	local bar=1
	nxt
	local bar=2
	function infun {
		echo $bar
	}
	infun
	local bar=BAD
}
foo
echo ${bar}3
EOF
exp=$'1\n2\n3'
got=$("$SHELL" "$tst")
[[ $exp == "$got" ]] || err_exit "Cannot create local variables in POSIX functions" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Test 6: ensure typeset doesn't default to making static variables in POSIX functions
cat > "$tst" << 'EOF'
nxt() {
	echo $bar
}
foo() {
	typeset bar=1
	nxt
	typeset bar=2
	function infun {
		echo $bar
	}
	infun
	typeset bar=3
}
foo
echo $bar
EOF
exp=$'1\n2\n3'
got=$("$SHELL" "$tst")
[[ $exp == "$got" ]] || err_exit "Cannot create global variables with plain typeset invocation in POSIX functions" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Test 7
cat > "$tst" << 'EOF'
nxt() {
	echo $bar
}
foo() {
	typeset bar=1
	nxt
	typeset bar=2
	infun() {
		echo $bar
	}
	infun
	typeset bar=3
}
foo
echo $bar
EOF
exp=$'1\n2\n3'
got=$("$SHELL" "$tst")
[[ $exp == "$got" ]] || err_exit "Cannot create global variables with plain typeset invocation in POSIX functions with nested POSIX functions" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Test 8
cat > "$tst" << 'EOF'
nxt() {
	echo $bar
}
foo() {
	local bar=1
	nxt
	local bar=2
	infun() {
		echo $bar
	}
	infun
	local bar=BAD
}
foo
echo ${bar}3
EOF
exp=$'1\n2\n3'
got=$("$SHELL" "$tst")
[[ $exp == "$got" ]] || err_exit "Cannot create local variables with local builtin in POSIX functions" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Test 9
cat > "$tst" << 'EOF'
nxt() {
	echo $bar
}
foo() {
	bar=1
	nxt
	bar=2
	function infun {
		echo $bar
	}
	infun
	bar=3
}
foo
echo $bar
EOF
exp=$'1\n2\n3'
got=$("$SHELL" "$tst")
[[ $exp == "$got" ]] || err_exit "Cannot create global variables in POSIX functions without direct typeset invocation" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Test 10: Make static variable global in KornShell function
tst=$tmp/tst.sh
cat > "$tst" << 'EOF'
function nxt {
	echo ${bar}1
}
function foo {
	typeset bar=BAD
	nxt
	typeset -g bar=2
	function infun {
		echo $bar
	}
	infun
	typeset -g bar=3
}
foo
echo $bar
EOF
exp=$'1\n2\n3'
got=$("$SHELL" "$tst")
[[ $exp == "$got" ]] || err_exit "Cannot switch from static scope to global scope in KornShell functions" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Test 11: Make dynamic variables global in KornShell functions
tst=$tmp/tst.sh
cat > "$tst" << 'EOF'
function nxt {
	echo ${bar}
}
function foo {
	local bar=1
	nxt
	local -g bar=3
	local bar=2
	function infun {
		# The dynamic scope still applies, so the $bar value
		# from 'function foo' is inherited and used instead
		# of the global value.
		echo $bar
	}
	infun
}
foo
# This will be '3' because of the earlier 'local -g'
echo $bar
EOF
exp=$'1\n2\n3'
got=$("$SHELL" "$tst")
[[ $exp == "$got" ]] || err_exit "Cannot switch from dynamic scope to global scope in KornShell functions" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Test 12: Variables shouldn't leak out of nested POSIX functions
tst=$tmp/tst.sh
cat > "$tst" << 'EOF'
foo() {
	local foo=foo
	bar() {
		local foo=bar
		baz() {
			local foo=baz
			echo $foo
		}
		baz
		echo $foo
	}
	bar
	echo $foo
}
foo
EOF
exp=$'baz\nbar\nfoo'
got=$("$SHELL" "$tst")
[[ $exp == "$got" ]] || err_exit "Local variables from nested POSIX functions leak out into the parent functions" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Test 13: Variables shouldn't leak out of nested KornShell functions
tst=$tmp/tst.sh
cat > "$tst" << 'EOF'
function foo {
	local foo=foo
	function bar {
		local foo=bar
		function baz {
			local foo=baz
			echo $foo
		}
		baz
		echo $foo
	}
	bar
	echo $foo
}
foo
EOF
exp=$'baz\nbar\nfoo'
got=$("$SHELL" "$tst")
[[ $exp == "$got" ]] || err_exit "Local variables from nested KornShell functions leak out into the parent functions" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Test 14: Variables shouldn't leak out into other POSIX functions
tst=$tmp/tst.sh
cat > "$tst" << 'EOF'
baz() {
	local foo=baz
	echo $foo
}
bar() {
	local foo=bar
	baz
	echo $foo
}
foo() {
	local foo=foo
	bar
	echo $foo
}
foo
EOF
exp=$'baz\nbar\nfoo'
got=$("$SHELL" "$tst")
[[ $exp == "$got" ]] || err_exit "Local variables from POSIX functions leak out into other functions" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Test 15: Variables shouldn't leak out into other KornShell functions
tst=$tmp/tst.sh
cat > "$tst" << 'EOF'
function baz {
	local foo=baz
	echo $foo
}
function bar {
	local foo=bar
	baz
	echo $foo
}
function foo {
	local foo=foo
	bar
	echo $foo
}
foo
EOF
exp=$'baz\nbar\nfoo'
got=$("$SHELL" "$tst")
[[ $exp == "$got" ]] || err_exit "Local variables from KornShell functions leak out into other functions" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
exit $((Errors<125?Errors:125))

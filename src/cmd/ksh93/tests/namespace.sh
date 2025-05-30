########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1982-2012 AT&T Intellectual Property          #
#          Copyright (c) 2020-2025 Contributors to ksh 93u+m           #
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

if((!SHOPT_NAMESPACE))
then	warning 'shell compiled without SHOPT_NAMESPACE; skipping tests'
	exit 0
fi

foo=abc
typeset -C bar=(x=3 y=4 t=7)
typeset -A z=([abc]=qqq)
integer r=9
function fn
{
	print global fn $foo
}
function fun
{
	print global fun $foo
}
mkdir -p $tmp/global/bin $tmp/local/bin
cat > $tmp/global/xfun <<- \EOF
	function xfun
	{
		print xfun global $foo
	}
EOF
cat > $tmp/local/xfun <<- \EOF
	function xfun
	{
		print xfun local $foo
	}
EOF
chmod +x "$tmp/global/xfun" "$tmp/local/xfun"
print 'print local prog $1' >  $tmp/local/bin/run
print 'print global prog $1' >  $tmp/global/bin/run
chmod +x "$tmp/local/bin/run" "$tmp/global/bin/run"
PATH=$tmp/global/bin:$PATH
FPATH=$tmp/global

namespace x
{
	foo=bar
	typeset -C bar=(x=1 y=2 z=3)
	typeset -A z=([qqq]=abc)
	function fn
	{
		print local fn $foo
	}
	[[ $(fn) == 'local fn bar' ]] || err_exit 'fn inside namespace should run local function'
	[[ $(fun) == 'global fun abc' ]] || err_exit 'global fun run from namespace not working'
	(( r == 9 )) || err_exit 'global variable r not set in namespace'
false
	[[ ${z[qqq]} == abc ]] || err_exit 'local array element not correct'
	[[ ${z[abc]} == '' ]] || err_exit 'global array element should not be visible when local element exists'
	[[ ${bar.y} == 2 ]] || err_exit 'local variable bar.y not found'
	[[ ${bar.t} == '' ]] || err_exit 'global bar.t should not be visible'
	function runxrun
	{
		xfun
	}
	function runrun
	{
		run $1
	}
	PATH=$tmp/local/bin:/bin
	FPATH=$tmp/local
	[[ $(runxrun) ==  'xfun local bar' ]] || err_exit 'local function on FPATH failed'
	[[ $(runrun $foo) ==  'local prog bar' ]] || err_exit 'local binary on PATH failed'
}
[[ $(fn) == 'global fn abc' ]] || err_exit 'fn outside namespace should run global function'
[[ $(.x.fn) == 'local fn bar' ]] || err_exit 'namespace function called from global failed'
[[  ${z[abc]} == qqq ]] || err_exit 'global associative array should not be affected by definition in namespace'
[[  ${bar.y} == 4 ]] || err_exit 'global compound variable should not be affected by definition in namespace'
[[  ${bar.z} == ''  ]] || err_exit 'global compound variable should not see elements in namespace'
[[ $(xfun) ==  'xfun global abc' ]] || err_exit 'global function on FPATH failed'
[[ $(run $foo) ==  'global prog abc' ]] || err_exit 'global binary on PATH failed'
false
[[ $(.x.runxrun) ==  'xfun local bar' ]] || err_exit 'namespace function on FPATH failed'

# ======
# Namespace variables should retain their exoprt attribute, even
# though they are not actually exported outside the namespace block.
set -o allexport
namespace foo_nam
{
	typeset bar
	typeset foo
	typeset baz=baz
	integer three
}
: ${.foo_nam.bar:=BAZ}
exp='typeset -x .foo_nam.bar=BAZ'
got=$(typeset -p .foo_nam.bar)
[[ $got == "$exp" ]] || err_exit 'Variable ${.foo_nam.bar} did not retain -x attribute' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
.foo_nam.foo=FOO
exp='typeset -x .foo_nam.foo=FOO'
got=$(typeset -p .foo_nam.foo)
[[ $got == "$exp" ]] || err_exit 'Variable ${.foo_nam.foo} did not retain -x attribute' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
exp='typeset -x .foo_nam.baz=baz'
got=$(typeset -p .foo_nam.baz)
[[ $got == "$exp" ]] || err_exit 'Variable ${.foo_nam.baz} did not retain -x attribute' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
((.foo_nam.three=3))
exp='typeset -x -l -i .foo_nam.three=3'
got=$(typeset -p .foo_nam.three)
[[ $got == "$exp" ]] || err_exit 'Variable ${.foo_nam.three} did not retain -x attribute' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
set +o allexport

# ======
# A namespace's parent scope should be restored after an error occurs.
# https://github.com/ksh93/ksh/issues/479#issuecomment-1140514159
exp=$'123 456\n789 456'
got=$(
	trap 'echo $x ${.test.x}; x=789; echo $x ${.test.x}' EXIT
	x=123
	namespace test
	{
		x=456
		set --bad_option 2>/dev/null
	}
)
[[ $got == "$exp" ]] || err_exit 'Parent scope not restored after special builtin throws error in namespace' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# 'typeset -g' should cause the active name space to be ignored.
# https://github.com/ksh93/ksh/issues/479#issuecomment-1140523291
if	! command typeset -g x 2>/dev/null
then
	warning "shell does not have 'typeset -g'; skipping those tests"
else
	unset x arr a b c
	# must specify the length for -X as the default is -X 24 on 32-bit systems and -X 32 on 64-bit systems
	typeset -A exp=(
		['x=123; namespace test { typeset -g -i x; }; typeset -p x']='typeset -i x=123'
		['x=123; namespace test { typeset -g -F x; }; typeset -p x']='typeset -F x=123.0000000000'
		['x=123; namespace test { typeset -g -E x; }; typeset -p x']='typeset -E x=123'
		['x=123; namespace test { typeset -g -X24 x; }; typeset -p x']='typeset -X 24 x=0x1.ec0000000000000000000000p+6'
		['x=aBc; namespace test { typeset -g -u x; }; typeset -p x']='typeset -u x=ABC'
		['x=aBc; namespace test { typeset -g -l x; }; typeset -p x']='typeset -l x=abc'
		['x=aBc; namespace test { typeset -g -L x; }; typeset -p x']='typeset -L 3 x=aBc'
		['x=aBc; namespace test { typeset -g -R x; }; typeset -p x']='typeset -R 3 x=aBc'
		['x=aBc; namespace test { typeset -g -Z x; }; typeset -p x']='typeset -Z 3 -R 3 x=aBc'
		['x=aBc; namespace test { typeset -g -L2 x; }; typeset -p x']='typeset -L 2 x=aB'
		['x=aBc; namespace test { typeset -g -R2 x; }; typeset -p x']='typeset -R 2 x=Bc'
		['x=aBc; namespace test { typeset -g -Z2 x; }; typeset -p x']='typeset -Z 2 -R 2 x=Bc'
		['x=8; namespace test { typeset -g -i2 x; }; typeset -p x']='typeset -i 2 x=2#1000'
		['x=8; namespace test { typeset -g -i8 x; }; typeset -p x']='typeset -i 8 x=8#10'
		['arr=(a b c); namespace test { typeset -g -i arr[1]=(1 2 3); }; typeset -p arr']='typeset -a -i arr=(0 (1 2 3) 0)'
		['arr=(a b c); namespace test { typeset -g -F arr[1]=(1 2 3); }; typeset -p arr']='typeset -a -F arr=(0.0000000000 (1.0000000000 2.0000000000 3.0000000000) 0.0000000000)'
		['arr=(a b c); namespace test { typeset -g -E arr[1]=(1 2 3); }; typeset -p arr']='typeset -a -E arr=(0 (1 2 3) 0)'
		['arr=(a b c); namespace test { typeset -g -X24 arr[1]=(1 2 3); }; typeset -p arr']='typeset -a -X 24 arr=(0x0.000000000000000000000000p+0 (0x1.000000000000000000000000p+0 0x1.000000000000000000000000p+1 0x1.800000000000000000000000p+1) 0x0.000000000000000000000000p+0)'
		['arr=(a b c); namespace test { typeset -A -g -i arr=([a]=1 [b]=2 [c]=3); }; typeset -p arr']='typeset -A -i arr=([a]=1 [b]=2 [c]=3)'
		['typeset -A arr=([a]=a [b]=b [c]=c); namespace test { typeset -A -g -i arr=([a]=1 [b]=2 [c]=3); }; typeset -p arr']='typeset -A -i arr=([a]=1 [b]=2 [c]=3)'
	)
	for cmd in "${!exp[@]}"
	do
		got=$(set +x; eval "$cmd" 2>&1)
		[[ $got == "${exp[$cmd]}" ]] || err_exit "typeset -g in $(printf %q "$cmd") failed to activate global" \
			"scope from name space (expected $(printf %q "${exp[$cmd]}"), got $(printf %q "$got"))"
	done
	unset exp
fi

# ======
# Test 'unset -f' in subshell in namespace
# https://github.com/ksh93/ksh/issues/648
# NOTE: for ast commands, '--version' is expected to exit with status 2 on 93u+m/1.0, 0 on 93u+m/1.1 and up
case ${.sh.version} in
*93u+m/1.[!0]* | 93u+m/[2-9].* | 93u+m/?[!.]*)
	s=0 ;;
*)	s=2 ;;
esac
exp='^  version         [[:alpha:]]{2,} (.*) ....-..-..$'
for b in cd disown fg getopts printf pwd read ulimit umask whence
do
	got=$(
		namespace ns
		{
			eval "$b() { echo BAD; }"
			(unset -f "$b"; PATH=/dev/null; "$b" --version 2>&1)
			exit	# avoid optimizing out the subshell
		}
	)
	[[ e=$? -eq s && $got =~ $exp ]] || err_exit "'unset -f $b' fails in subshell (1a)" \
		"(expected status $s and ERE match of $(printf %q "$exp"), got status $e and $(printf %q "$got"))"

	namespace ns
	{
		got=$(
			eval "$b() { echo BAD; }"
			(unset -f "$b"; PATH=/dev/null; "$b" --version 2>&1)
			exit	# avoid optimizing out the subshell
		)
	}
	[[ e=$? -eq s && $got =~ $exp ]] || err_exit "'unset -f $b' fails in subshell (1b)" \
		"(expected status $s and ERE match of $(printf %q "$exp"), got status $e and $(printf %q "$got"))"

	# bug introduced on 2023-06-02
	got=$(
		eval "$b() { echo BAD; }"
		namespace ns
		{
			(unset -f "$b"; PATH=/dev/null; "$b" --version 2>&1)
			exit	# avoid optimizing out the subshell
		}
	)
	# for ast commands, '--version' is expected to exit with status 2 on 93u+m/1.0, 0 on 93u+m/1.1 and up
	case ${.sh.version} in
	*93u+m/1.[!0]* | 93u+m/[2-9].* | 93u+m/?[!.]*)
		s=0 ;;
	*)	s=2 ;;
	esac
	[[ e=$? -eq s && $got =~ $exp ]] || err_exit "'unset -f $b' fails in subshell (1c)" \
		"(expected status $s and ERE match of $(printf %q "$exp"), got status $e and $(printf %q "$got"))"

	got=$(
		namespace ns
		{
			eval "$b() { echo BAD; }"
			(unset -f .ns."$b"; PATH=/dev/null; .ns."$b" 2>/dev/null || { let "$?==127" && "$b" --version 2>&1; })
			exit	# avoid optimizing out the subshell
		}
	)
	[[ e=$? -eq s && $got =~ $exp ]] || err_exit "'unset -f .ns.$b' fails in subshell (2a)" \
		"(expected status $s and ERE match of $(printf %q "$exp"), got status $e and $(printf %q "$got"))"

	namespace ns
	{
		got=$(
			eval "$b() { echo BAD; }"
			(unset -f .ns."$b"; PATH=/dev/null; .ns."$b" 2>/dev/null || { let "$?==127" && "$b" --version 2>&1; })
			exit	# avoid optimizing out the subshell
		)
	}
	[[ e=$? -eq s && $got =~ $exp ]] || err_exit "'unset -f .ns.$b' fails in subshell (2b)" \
		"(expected status $s and ERE match of $(printf %q "$exp"), got status $e and $(printf %q "$got"))"
done

# ======
# https://github.com/ksh93/ksh/issues/727
exp=foo
got=$(unset _AST_FEATURES; "$SHELL" -c 'namespace foo { echo foo; }' 2>&1)
[[ $got == "$exp" ]] || err_exit "'echo' botched in namespace" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
case $'\n'${ builtin;}$'\n' in
*$'\n'/opt/ast/bin/getconf$'\n'*)
	got=$(unset _AST_FEATURES; "$SHELL" -c 'namespace ucb { /opt/ast/bin/getconf UNIVERSE = ucb; echo foo; }' 2>&1)
	[[ $got == "$exp" ]] || err_exit "'getconf' and/or 'echo' botched in namespace" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
	;;
esac

# ======
# https://github.com/ksh93/ksh/issues/728
got=$(PATH=/opt/ast/bin; PATH=$(getconf PATH); "$SHELL" -c 'builtin basename; namespace ns { basename --version; }' 2>&1)
exp='  version         basename (*) ????-??-??'
[[ $got == $exp ]] || err_exit "optional builtin not found when run from namespace" \
	"(expected match of $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
got=$("$SHELL" -c 'namespace foo { echo OK | read _DeFiNiTeLy_nOnExIsTeNt_vAr_; }' 2>&1) ||
	err_exit "read fails in namespace (got $(printf %q "$got"))"
got=$("$SHELL" -c 'namespace foo { export "_dEfInItElY_NoNeXiStEnT_VaR_=1"; }' 2>&1) ||
	err_exit "export with quoted assignment-argument fails in namespace (got $(printf %q "$got"))"

# ======
exit $((Errors<125?Errors:125))

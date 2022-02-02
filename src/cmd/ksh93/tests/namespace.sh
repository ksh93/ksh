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
namespace foo_nam {
	typeset bar
	typeset foo
	typeset baz=baz
	integer three
}
: ${.foo_nam.bar:=BAZ}
exp='typeset -x .foo_nam.bar=BAZ'
got=$(typeset -p .foo_nam.bar)
[[ $got == "$exp" ]] || err_exit 'Variable ${.foo_nam.bar} dit not retain -x attribute' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
.foo_nam.foo=FOO
exp='typeset -x .foo_nam.foo=FOO'
got=$(typeset -p .foo_nam.foo)
[[ $got == "$exp" ]] || err_exit 'Variable ${.foo_nam.foo} dit not retain -x attribute' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
exp='typeset -x .foo_nam.baz=baz'
got=$(typeset -p .foo_nam.baz)
[[ $got == "$exp" ]] || err_exit 'Variable ${.foo_nam.baz} dit not retain -x attribute' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
((.foo_nam.three=3))
exp='typeset -x -l -i .foo_nam.three=3'
got=$(typeset -p .foo_nam.three)
[[ $got == "$exp" ]] || err_exit 'Variable ${.foo_nam.three} dit not retain -x attribute' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
set +o allexport

# ======
exit $((Errors<125?Errors:125))

########################################################################
#                                                                      #
#              This file is part of the ksh 93u+m package              #
#          Copyright (c) 1984-2012 AT&T Intellectual Property          #
#          Copyright (c) 2020-2026 Contributors to ksh 93u+m           #
#                      and is licensed under the                       #
#                 Eclipse Public License, Version 2.0                  #
#                                                                      #
#                A copy of the License is available at                 #
#      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      #
#         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         #
#                                                                      #
#                 Glenn Fowler <gsf@research.att.com>                  #
#                  Martijn Dekker <martijn@inlv.org>                   #
#                                                                      #
########################################################################

# Generator for *.req library dependency files.
# This script is designed to be called from Mamfiles only.
# It is a heavily refactored version of the code generated
# by the nmake LIBS_req rule, and replaces the latter.

# Set options.
case ${ZSH_VERSION+z} in
z)	emulate ksh ;;
*)	(command set -o posix) 2>/dev/null && set -o posix ;;
esac
set -o noglob -o nounset
: $INSTALLROOT	# error out early

# Get colon-separated compiler and linker invocations from arguments.
# Like this whole build system, we assume arguments do not contain
# spaces, or if they do, that they are meant to be field-split.
allcc=
while	test "$#" -gt 0 && test "$1" != ":"
do	allcc=$allcc${allcc:+ }$1
	shift
done
test "${1-}" = ":" || { echo "$0: bad arguments" >&2; exit 1; }
shift
ldflags=
while	test "$#" -gt 0 && test "$1" != ":"
do	ldflags=$ldflags${ldflags:+ }$1
	shift
done
test "${1-}" = ":" || { echo "$0: bad arguments" >&2; exit 1; }
shift

# Setup.
trap 'set +o noglob; rm -rf mkreq.$$.*' 0
echo 'int main(void) { return 0; }' > mkreq.$$.c
$allcc -c mkreq.$$.c || exit

# Function: try to link the test program with possible extra linker flags.
try_to_link()
{
	_lib=$1
	shift
	# Delete any previous test binary (including any OS-specific extra files).
	test -e mkreq.$$.x && (set +o noglob; rm -rf mkreq.$$.x*)
	# Try to link.
	$allcc ${1+"$@"} $ldflags -o mkreq.$$.x mkreq.$$.o -l${_lib} 2>/dev/null || return
	# To work around possible linker breakage, we have to
	# actually run the test program, not merely link it.
	./mkreq.$$.x
	e=$?
	if	test "$e" -gt 0
	then	if	test "$e" -gt 128 &&
			test "$(kill -l $e)" = "ABRT" &&
			test "$(uname -s)" = "Darwin"
		then	echo "$0: THE ABOVE CRASH IS APPLE'S BUG, NOT OURS." >&2
			echo "$0: We have successfully worked around the bug." >&2
			echo "$0: https://github.com/ksh93/ksh/issues/596" >&2
		fi
		return 1
	fi
	return 0
}

######### Main ##########
#  Generate .req file.  #
#########################
self=$1
shift
exec >$self.req
echo " -l$self"
for name
do	if	test -f $INSTALLROOT/lib/lib/$name
	then	grep '^ -l.' $INSTALLROOT/lib/lib/$name
		continue
	elif	test ! -f $INSTALLROOT/lib/lib$name.a
	then	try_to_link $name -L$INSTALLROOT/lib || try_to_link $name || continue
	fi
	echo " -l$name"
done | sort -u

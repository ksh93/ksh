########################################################################
#                                                                      #
#              This file is part of the ksh 93u+m package              #
#          Copyright (c) 2021-2024 Contributors to ksh 93u+m           #
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

# Dynamic library linking tool for ksh 93u+m and supporting libraries.
# Called from **/Mamfile.

case ${ZSH_VERSION+z} in
z)	emulate ksh ;;
*)	(command set -o posix) 2>/dev/null && set -o posix ;;
esac
set -o noglob	# avoid interference from pathname expansion

note()
{
	printf "$0: %s\\n" "$@" >&2
}

err_out()
{
	note "$@"
	exit 3
}

do_link()
{
	target=$dest_dir/$1
	shift
	test -e "$target" && rm -f "$target"
	(
		PS4="[dylink] $PS4"
		set -o xtrace
		# why pass CCFLAGS here? because it *might* have a flag like -m64 that the linker also needs
		${CC:-cc} $CCFLAGS $LDFLAGS -o "$target" "$@"
	) || exit
}

# Basic sanity check.
case ${AST_DYLIB_VERSION:+A}${HOSTTYPE:+H}${AST_NO_DYLIB+n} in
AH)
	;;
*)
	echo "$0: building dynamic libraries was not enabled; skipping" >&2
	exit 0  # continue build
	;;
esac

# Parse options.
exec_file= module_name= l_flags= suffix=
while getopts 'e:m:l:s:' opt
do	case $opt in
	e)	exec_file=$OPTARG ;;
	m)	module_name=$OPTARG ;;
	l)	l_flags="$l_flags -l$OPTARG" ;;
	s)	suffix=$OPTARG ;;   # this should be like .6.0.dylib or .so.6.0
	'?')	exit 2 ;;
	*)	err_out "Internal error (getopts)" ;;
	esac
done
shift $((OPTIND - 1))

# Validate options.
case ${exec_file:+e}${module_name:+m} in
e | m)	;;
*)	err_out "Either -e or -m should be specified" ;;
esac
case ${module_name:+m}${suffix:+s} in
'')	;;
ms)	case $suffix in
	*$AST_DYLIB_VERSION*) ;;
	*)	err_out "suffix does not contain \$AST_DYLIB_VERSION ($AST_DYLIB_VERSION)" ;;
	esac ;;
*)	err_out "-m requires -s and vice versa" ;;
esac

case $HOSTTYPE in
android.* | darwin.* | freebsd* | linux.* | openbsd.* | sol* )
	# supported
	;;
cygwin.*)
	note "Dynamic libraries are not supported on Cygwin."
	exit 0 ;;  # continue build
*)
	note "The system $HOSTTYPE is currently untested for dynamic libraries" \
		"so dynamic libraries are disabled by default. To test them," \
		"export AST_DYLIB_TEST to try to build a dynamically linked ksh."
	case ${AST_DYLIB_TEST:+y} in
	y)	;;
	* )	exit 0 ;; # continue build
	esac ;;
esac

# Set destination directory.
dest_dir=$INSTALLROOT/dyn
mkdir -p "$dest_dir/bin" "$dest_dir/lib" || err_out "could not mkdir"

# Do the dynamic linking.
case ${exec_file} in
'')	# ... figure out library file name(s) and internal name for linking purposes
	lib_file=lib$module_name$suffix
	lib_linkname=$(echo "$lib_file" | sed "s/\.$AST_DYLIB_VERSION/.${AST_DYLIB_VERSION%%.*}/")
	sym_links="$lib_linkname $(echo "$lib_file" | sed "s/\.$AST_DYLIB_VERSION//")"
	# ... execute linker command
	case $HOSTTYPE in
	darwin*)
		do_link "lib/$lib_file" -dynamiclib \
			-Wl,-dylib_install_name -Wl,"$lib_linkname" \
			"$@" -L"$dest_dir/lib" $l_flags
		;;
	*)
		do_link "lib/$lib_file" -shared -Wl,-soname -Wl,"$lib_linkname" \
			"$@" -L"$dest_dir/lib" $l_flags
		;;
	esac
	for f in $sym_links
	do	ln -sf "$lib_file" "$dest_dir/lib/$f"
	done
	;;
*)	# Link an executable.
	do_link "bin/$exec_file" "$@" -L"$dest_dir/lib" $l_flags
	;;
esac

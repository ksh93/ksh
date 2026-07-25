########################################################################
#                                                                      #
#              This file is part of the ksh 93u+m package              #
#          Copyright (c) 2021-2026 Contributors to ksh 93u+m           #
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
set -o noglob	# avoid pathname expansion interfering with field splitting
CCn='
'  # newline

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
case ${HOSTTYPE:+H}${INSTALLROOT:+I} in
HI)	;;
*)	err_out "Required environment missing"
	;;
esac

# Parse options.
unset opt_query exec_file module_name l_flags version prefix suffix
deproot=$INSTALLROOT/lib/lib
while getopts 'Qe:m:l:v:p:s:' opt
do	case $opt in
	Q)	opt_query=y         # query support for dynamic libraries on this system
		;;
	e)	exec_file=$OPTARG
		;;
	m)	test -n "${module_name+m}" && err_out "-m cannot be used more than once"
		test -n "${l_flags+l}" && err_out "-m cannot be used after -l"
		test -f "$OPTARG.req" || err_out "$OPTARG.req not found -- generate it with mkreq(1)"
		l_flags=$(sed 1d "$OPTARG.req") || exit
		module_name=$OPTARG
		;;
	l)	if	test -f "$OPTARG.req"
		then    l_flags=${l_flags:+$l_flags$CCn}$(cat "$OPTARG.req") || exit
		elif	test -f "$deproot/$OPTARG"
		then	l_flags=${l_flags:+$l_flags$CCn}$(cat "$deproot/$OPTARG") || exit
		else	l_flags=${l_flags:+$l_flags$CCn}\ -l$OPTARG
		fi
		;;
	v)	version=$OPTARG     # this should be like 6.0
		;;
	p)	prefix=$OPTARG      # this should be 'lib' or empty
		;;
	s)	suffix=$OPTARG      # this should be like .dylib or .so
		;;
	'?')	exit 2
		;;
	*)	err_out "Internal error (getopts)"
		;;
	esac
done
shift $((OPTIND - 1))

# Validate options.
case ${opt_query+Q}${exec_file:+e}${module_name:+m}${l_flags+l}${version+v}${prefix+p}${suffix+s} in
Q?*)	err_out "-Q cannot be used with other options" ;;
Q)	test "$#" -gt 0 && err_out "-Q cannot be used with arguments" ;;
e | e[!m]* | m*)
	;;
*)	err_out "Either -e or -m should be specified" ;;
esac
case ${module_name:+m}${prefix+p}${suffix+s}${version:+v} in
'' | mpsv )
	;;
msv)	note "warning: -p not given; assuming -p lib for backward compat"
	prefix=lib ;;
*)	err_out "-m requires -v/-p/-s and vice versa" ;;
esac

# Check if building dynamic libraries was disabled.
case ${AST_NO_DYLIB+N}${opt_query+Q} in
NQ)	exit 1
	;;
N)	note "Building dynamic libraries was disabled; skipping"
	exit 0  # continue build
	;;
esac

# Check for supported system.
case $HOSTTYPE in
android.* | darwin.* | dragonflybsd.* | freebsd* | haiku.* | linux.* | netbsd.* | openbsd.* | qnx.* | sol??.* )
	# supported
	test -n "$opt_query" && exit 0
	;;
cygwin.*)
	test -n "$opt_query" && exit 1
	note "Dynamic libraries are not supported on Cygwin."
	exit 0  # continue build
	;;
*)	case ${AST_DYLIB_TEST:+y} in
	y)	test -n "$opt_query" && exit 0
		;;
	*)	test -n "$opt_query" && exit 1
		note "The system $HOSTTYPE is currently untested for dynamic libraries" \
			"so dynamic libraries are disabled by default. To test them," \
			"export AST_DYLIB_TEST to try to build a dynamically linked ksh."
		exit 0  # continue build
		;;
	esac
	;;
esac

# Deduplicate -l flags.
# Keep the last-mentioned of each item that occurs multiple times (this is
# required for passing libraries to the linker in the correct order; that
# is, each dependency must come after all the libraries that depend on it).
dupes=$l_flags
l_flags=
while	test -n "$dupes"
do	# Grab first item from dupes
	item=${dupes%%"$CCn"*}
	# Remove that item from dupes
	dupes=${dupes#"$item"}
	dupes=${dupes#"$CCn"} # remove $CCn separately as last item won't end in it
	# Skip the item if another identical item is still in dupes
	case "$CCn$dupes$CCn" in
	*"$CCn$item$CCn"*)
		continue ;;
	esac
	l_flags="$l_flags$item"
done

# Set destination directory.
dest_dir=$INSTALLROOT/dyn
mkdir -p "$dest_dir/bin" "$dest_dir/lib" || err_out "could not mkdir"

# Do the dynamic linking.
case ${exec_file} in
'')	# ... figure out library file name(s) and internal name for linking purposes
	#     on macOS we have version before extension (libast.6.0.dylib), on other systems, after (libast.so.6.0)
	case $suffix in
	.dylib)	lib_file=$prefix$module_name.$version$suffix
		lib_linkname=$prefix$module_name.${version%%.*}$suffix
		;;
	*)	lib_file=$prefix$module_name$suffix.$version
		lib_linkname=$prefix$module_name$suffix.${version%%.*}
		;;
	esac
	sym_links="$lib_linkname $prefix$module_name$suffix"
	# ... remove possible old versions
	(set +o noglob; exec rm -f "$dest_dir/lib/$prefix$module_name".*)
	# ... execute linker command
	case $HOSTTYPE in
	darwin.*)
		do_link "lib/$lib_file" -dynamiclib \
			-Wl,-dylib_install_name -Wl,"@rpath/$lib_linkname" \
			"$@" -L"$dest_dir/lib" $l_flags
		;;
	*)	do_link "lib/$lib_file" -shared -Wl,-soname -Wl,"$lib_linkname" \
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

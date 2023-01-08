########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1994-2012 AT&T Intellectual Property          #
#          Copyright (c) 2020-2022 Contributors to ksh 93u+m           #
#                      and is licensed under the                       #
#                 Eclipse Public License, Version 2.0                  #
#                                                                      #
#                A copy of the License is available at                 #
#      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      #
#         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         #
#                                                                      #
#                 Glenn Fowler <gsf@research.att.com>                  #
#                  Martijn Dekker <martijn@inlv.org>                   #
#                      Trey Valenta <t@trey.net>                       #
#            Johnothan King <johnothanking@protonmail.com>             #
#  Sterling Jensen <5555776+sterlingjensen@users.noreply.github.com>   #
#                   Chase <nicetrynsa@protonmail.ch>                   #
#               Anuradha Weeraman <anuradha@debian.org>                #
#                                                                      #
########################################################################
### this sh script is POSIX compliant and compatible with shell bugs ###
#          KornShell 93u+m build system, main control script           #
#    based on AST 'package' by Glenn Fowler <gsf@research.att.com>     #
#    simplified and rewritten by Martijn Dekker <martijn@inlv.org>     #
########################################################################

# Escape from a non-POSIX shell
# ('test X -ef Y' is technically non-POSIX, but practically universal)
min_posix='test / -ef / && path=Bad && case $PATH in (Bad) exit 1;; esac && '\
'PWD=Bad && cd -P -- / && case $PWD in (/) ;; (*) exit 1;; esac && '\
'! { ! case x in ( x ) : ${0##*/} || : $( : ) ;; esac; } && '\
'trap "exit 0" 0 && exit 1'
if	(eval "$min_posix") 2>/dev/null
then	: good shell
else	"$SHELL" -c "$min_posix" 2>/dev/null && exec "$SHELL" -- "$0" ${1+"$@"}
	sh -c "$min_posix" 2>/dev/null && exec sh -- "$0" ${1+"$@"}
	DEFPATH=`getconf PATH` 2>/dev/null || DEFPATH=/usr/xpg4/bin:/bin:/usr/bin:/sbin:/usr/sbin
	PATH=$DEFPATH:$PATH
	export PATH
	sh -c "$min_posix" 2>/dev/null && exec sh -- "$0" ${1+"$@"}
	echo "$0: Can't escape from obsolete or broken shell. Run me with a POSIX shell." >&2
	exit 128
fi
readonly min_posix	# use for checksh()

# Set standards compliance mode
(command set -o posix) 2>/dev/null && set -o posix

# Sanitize 'cd'
unset CDPATH

# Make the package root the current working directory
# This makes it possible to run '/my/path/package make' without cd'ing first
# (for all its featuritis, the AT&T version never could manage this)
case $0 in
[0123456789+-]*)
	echo "dodgy \$0: $0" >&2
	exit 128 ;;
*/*)
	me=$0 ;;
*)
	me=$(command -v "$0") || exit 128 ;;
esac
me=$(dirname "$me")
cd "$me" || exit
unset -v me
case $PWD in
*/arch/*/*/bin)
	cd .. ;;
*/arch/*/bin)
	cd ../../.. ;;
*/bin)
	cd .. ;;
*)
	echo "this script must live in bin/" >&2
	exit 1 ;;
esac || exit

# shell checks
checksh()
{
	"$1" -c "$min_posix" 2>/dev/null || return 1
}

LC_ALL=C
export LC_ALL

TMPDIR=${TMPDIR:-/tmp}
export TMPDIR

src="cmd contrib etc lib"
use="/usr/common /exp /usr/local /usr/add-on /usr/addon /usr/tools /usr /opt"
usr="/home"
lib="" # need /usr/local/lib /usr/local/shlib
ccs="/usr/kvm /usr/ccs/bin"
org="gnu GNU"
makefiles="Mamfile"  # ksh 93u+m no longer uses these: Nmakefile nmakefile Makefile makefile
env="HOSTTYPE PACKAGEROOT INSTALLROOT PATH"

package_use='=$HOSTTYPE=$PACKAGEROOT=$INSTALLROOT=$EXECROOT=$CC='

CROSS=0

MAKESKIP=${MAKESKIP:-"*[-.]*"}

all_types='*.*|sun4'		# all but sun4 match *.*

command=${0##*/}
case $(getopts '[-][123:xyz]' opt --xyz 2>/dev/null; echo 0$opt) in
0123)	USAGE=$'
[-?
@(#)$Id: '$command$' (ksh 93u+m) 2023-01-08 $
]
[-author?Glenn Fowler <gsf@research.att.com>]
[-author?Contributors to https://github.com/ksh93/ksh]
[-copyright?(c) 1994-2012 AT&T Intellectual Property]
[-copyright?(c) 2020-2022 Contributors to https://github.com/ksh93/ksh]
[-license?https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html]
[+NAME?'$command$' - build, test and install ksh 93u+m]
[+DESCRIPTION?The \b'$command$'\b command is the main control script
    for building and installing KornShell 93u+m.
    It is a POSIX \bsh\b(1) script coded for maximal portability.
    A POSIX shell and C compiler installation are the only requirements.
    All package files are in the \b$PACKAGEROOT\b directory tree.
    Binary package files are in the \b$INSTALLROOT\b
    (\b$PACKAGEROOT/arch/\b\ahosttype\a) tree, where
    \ahosttype\a=$(\bbin/package host type\b).
    See \bDETAILS\b for more information.]
[+?Note that no environment variables need be set by the user;
    \b'$command$'\b determines the environment based on the current working
    directory. The \buse\b action starts a \bsh\b(1) with the environment
    initialized. \bCC\b, \bCCFLAGS\b, \bHOSTTYPE\b and \bSHELL\b may be set
    by explicit command argument assignments to override the defaults.]
[+?The command arguments are composed of a sequence of words: zero or
    more \aqualifiers\a, one \aaction\a, and zero or more action-specific
    \aarguments\a, and zero or more \aname=value\a definitions.
    \boptget\b(3) documentation options such as \b--man\b, \b--html\b
    and \b--nroff\b are also supported.
    The default with no arguments is \bhost type\b.]
[+?The qualifiers are:]
    {
        [+debug|environment?Show environment and actions but do not
            execute.]
	[+flat?With the \bmake\b action, create a flat view by linking all
	    files from \b$INSTALLROOT\b, minus \b*.old\b files,
	    onto their corresponding path under \b$PACKAGEROOT\b.
	    Subsequent \bmake\b actions will update an existing flat view
	    whether or not \bflat\b is specified.
	    Only one architecture can have a flat view.
	    If \bflat\b is specified with the \bclean\b action, then
	    only clean up this flat view and do not delete \b$INSTALLROOT\b.]
        [+force?Force the action to override saved state.]
        [+never?Run make -N and show other actions.]
        [+only?Only operate on the specified packages.]
        [+quiet?Do not list captured action output.]
        [+show?Run make -n and show other actions.]
        [+verbose?Provide detailed action output.]
        [+DEBUG?Trace the package script actions in detail.]
    }
[+?The actions are:]
    {
	[+clean | clobber?Clean up the flat view, if any.
	    Then, unless \bflat\b was given,
	    delete the \barch/\b\aHOSTTYPE\a hierarchy; this
	    deletes all generated files and directories for \aHOSTTYPE\a.
	    The hierarchy can be rebuilt by \b'$command$' make\b.]
        [+export\b [ \avariable\a ...]]?List \aname\a=\avalue\a for
            \avariable\a, one per line. If the \bonly\b attribute is
            specified then only the variable values are listed. If no
	    variables are specified then \b'$env$'\b are assumed.]
        [+help\b [ \aaction\a ]]?Display help text on the standard
            error (standard output for \aaction\a).]
        [+host\b [ \aattribute\a ... ]]?List
            architecture/implementation dependent host information on the
            standard output. \btype\b is listed if no attributes are
            specified. Information is listed on a single line in
            \aattribute\a order. The attributes are:]
            {
                [+canon \aname\a?An external host type name to be
                    converted to \b'$command$'\b syntax.]
                [+cpu?The number of CPUs; 1 if the host is not a
                    multiprocessor.]
                [+name?The host name.]
                [+type?The host type, usually in the form
                    \avendor\a.\aarchitecture\a, with an optional trailing
                    -\aversion\a. The main theme is that type names within
                    a family of architectures are named in a similar,
                    predictable style. OS point release information is
                    avoided as much as possible, but vendor resistance to
                    release incompatibilities has for the most part been
                    futile.]
            }
        [+install\b [ \adest_dir\a ]] [ \acommand\a ... ]]?Install commands
            from the \b$INSTALLROOT\b tree
            into appropriate subdirectories of \adest_dir\a.
            If \adest_dir\a does not exist,
            then it and any necessary subdirectories are created.
            \adest_dir\a can be a directory like \a/usr/local\a
            to install the \acommand\as directly,
            or a temporary directory like \a/tmp/pkgtree/usr\a
            to prepare for packaging with operating system-specific tools.
            If no \acommand\a is specified,
            then \aksh\a and \ashcomp\a are assumed.]
        [+make\b [ \apackage\a ]] [ \aoption\a ... ]] [ \atarget\a ... ]]?Build
	    and install. The default \atarget\a is \binstall\b, which makes
	    and installs \apackage\a. If the standard output is a terminal
	    then the output is also captured in
            \b$INSTALLROOT/lib/package/gen/make.out\b. The build is done in
            the \b$INSTALLROOT\b directory tree viewpathed on top of the
            \b$PACKAGEROOT\b directory tree. Leaf directory names matching the
            \b|\b-separated shell pattern \b$MAKESKIP\b are ignored. The
            \bview\b action is done before making. \aoption\a operands are
	    passed to the underlying make command.]
        [+results\b [ \bfailed\b ]] [ \bpath\b ]] [ \bold\b ]] [\bmake\b | \btest\b | \bwrite\b ]]?List
            results and interesting messages captured by the most recent
            \bmake\b (default), \btest\b or \bwrite\b action. \bold\b
            specifies the previous results, if any (current and previous
            results are retained). \b$HOME/.pkgresults\b, if it exists,
            must contain an \begrep\b(1) expression of result lines to be
            ignored. \bfailed\b lists failures only and \bpath\b lists the
            results file path name only.]
        [+test\b [ \b\adir\a\b ]]\b?Run all available default regression tests.
            If the optional \adir\a argument (such as \bsrc/cmd/ksh93\b) is given,
            only the tests in that directory are run.
            If the standard output is a terminal then the
            output is also captured in \b$INSTALLROOT/lib/package/gen/test.out\b.
            Programs must be made before they can be tested.
            For \bksh\b, a separate \bshtests\b command is available that allows
            passing arguments to select and tune the regression tests.
            See \bbin/shtests --man\b for more information.]
        [+use\b [ \auid\a | \apackage\a | . | - [ command ... ]] ]]?Run
            \acommand\a, or an interactive shell if \acommand\a is omitted,
            with the environment initialized for using the package. If
            \auid\a or \apackage\a or \a.\a is specified then it is used
	    to determine a \b$PACKAGEROOT\b, possibly different from
	    the current directory. For example, to try out bozo'\'$'s package:
            \bpackage use bozo\b. The \buse\b action may be run from any
            directory. If the file \b$INSTALLROOT/lib/package/profile\b is
            readable then it is sourced to initialize the environment.]
        [+view\b?Initialize the architecture specific viewpath
            hierarchy. The \bmake\b action implicitly calls this action.]
    }
[+DETAILS?The package directory hierarchy is rooted at
    \b$PACKAGEROOT\b. All source and binaries reside under this tree. A two
    level viewpath is used to separate source and binaries. The top view is
    architecture specific, the bottom view is shared source. All building
    is done in the architecture specific view; no source view files are
    intentionally changed. This means that many different binary
    architectures can be made from a single copy of the source.]
[+?Independent \b$PACKAGEROOT\b hierarchies can be combined by
    appending \b$INSTALLROOT:$PACKAGEROOT\b pairs to \bVPATH\b. The
    \bVPATH\b viewing order is from left to right.]
[+?\b$HOSTTYPE\b names the current binary architecture and is determined
    by the output of \b'$command$'\b (no arguments). The \b$HOSTTYPE\b naming
    scheme is used to separate incompatible executable and object formats.
    All architecture specific binaries are placed under \b$INSTALLROOT\b
    (\b$PACKAGEROOT/arch/$HOSTTYPE\b). There are a few places that match
    against \b$HOSTTYPE\b when making binaries; these are limited to
    makefile compiler workarounds, e.g., if \b$HOSTTYPE\b matches \bhp.*\b
    then turn off the optimizer for these objects. All other architecture
    dependent logic is handled either by the \bAST\b \biffe\b(1) command or
    by component specific configure scripts. Explicit \b$HOSTTYPE\b
    values matching *,*cc*[,-*,...]] optionally set the default \bCC\b and
    \bCCFLAGS\b. This is handy for build farms that support different
    compilers on the same architecture.]
[+?Each component contains a \bMAM\b (make abstract machine)
    file (\bMamfile\b). A Mamfile contains a portable makefile description
    written in a simple dependency tree language using indented
    \bmake\b...\bdone\b blocks.]
[+?All scripts and commands under \b$PACKAGEROOT\b use \b$PATH\b
    relative pathnames (via the \bAST\b \bpathpath\b(3) function); there
    are no embedded absolute pathnames. This means that binaries generated
    under \b$PACKAGEROOT\b may be copied to a different root; users need
    only change their \b$PATH\b variable to reference the new installation
    root \bbin\b directory. \b'$command$' install\b installs binary packages in
    a new \b$INSTALLROOT\b.]

[ qualifier ... ] [ action ] [ arg ... ] [ n=v ... ]

[+SEE ALSO?\bautoconfig\b(1), \bcksum\b(1), \bexpmake\b(1),
	\bgzip\b(1), \bmake\b(1), \bmamake\b(1), \bpax\b(1),
	\bpkgadd\b(1), \bpkgmk\b(1), \brpm\b(1),
	\bsh\b(1), \btar\b(1), \boptget\b(3)]
'
	case $* in
	help)	set -- --man ;;
	esac
	while	getopts -a "$command" "$USAGE" OPT
	do	:
	done
	shift $((OPTIND-1))
	;;
esac

# check the args

case $AR in
'')	AR=ar ;;
esac
case $CC in
'')	CC=cc ;;
esac
case $LD in
'')	LD=ld ;;
esac
case $NM in
'')	NM=nm ;;
esac

action=
bit=
exec=
flat=0
force=0
global=
hi=
ifs=${IFS-'
	 '}
lo=
make=
makeflags='-K'
nl="
"
noexec=
only=0
output=
quiet=0
show=:
tab="        "
verbose=0
AUTHORIZE=
DEBUG=
SHELLMAGIC=-

unset FIGNORE BINDIR DLLDIR ETCDIR FUNDIR INCLUDEDIR LIBDIR LOCALEDIR MANDIR SHAREDIR 2>/dev/null || true

while	:
do	case $# in
	0)	set host type ;;
	esac
	case $1 in
	clean|clobber|export|host|install|make|remove|results|test|use|view)
		action=$1
		shift
		break
		;;
	debug|environment)
		exec=echo make=echo show=echo
		;;
	flat)	flat=1
		;;
	force)	force=1
		;;
	never)	exec=echo noexec=-N
		;;
	only)	only=1
		;;
	quiet)	quiet=1
		;;
	show)	exec=echo noexec=-n
		;;
	verbose)verbose=1
		;;
	DEBUG)	DEBUG=1
		PS4='+$LINENO:$SECONDS+ '
		set -x
		;;
	help|HELP|html|man|--[?m]*)
                case $1 in
		help)	code=0
			case $2 in
			'')	exec 1>&2 ;;
			esac
			;;
                html)	code=0 html=1
			echo "<html><title>$command help</title><body><h1><code>$command</code> help</h1><pre>"
			;;
		*)	code=2
			exec 1>&2
			;;
		esac
		# Plain-text fallback. Regenerate with:
		# ksh -c 'COLUMNS=80 bin/package --man' 2>&1 | sed "s/'/'\\\\''/g; 1s/^/echo '/; \$s/\$/'/"
		echo 'NAME
  package - build, test and install ksh 93u+m

SYNOPSIS
  package [ options ] [ qualifier ... ] [ action ] [ arg ... ] [ n=v ... ]

DESCRIPTION
  The package command is the main control script for building and installing
  KornShell 93u+m. It is a POSIX sh(1) script coded for maximal portability. A
  POSIX shell and C compiler installation are the only requirements. All
  package files are in the $PACKAGEROOT directory tree. Binary package files
  are in the $INSTALLROOT ($PACKAGEROOT/arch/hosttype) tree, where
  hosttype=$(bin/package host type). See DETAILS for more information.

  Note that no environment variables need be set by the user; package
  determines the environment based on the current working directory. The use
  action starts a sh(1) with the environment initialized. CC, CCFLAGS, HOSTTYPE
  and SHELL may be set by explicit command argument assignments to override the
  defaults.

  The command arguments are composed of a sequence of words: zero or more
  qualifiers, one action, and zero or more action-specific arguments, and zero
  or more name=value definitions. optget(3) documentation options such as
  --man, --html and --nroff are also supported. The default with no arguments
  is host type.

  The qualifiers are:
    debug|environment
          Show environment and actions but do not execute.
    flat  With the make action, create a flat view by linking all files from
          $INSTALLROOT, minus *.old files, onto their corresponding path under
          $PACKAGEROOT. Subsequent make actions will update an existing flat
          view whether or not flat is specified. Only one architecture can have
          a flat view. If flat is specified with the clean action, then only
          clean up this flat view and do not delete $INSTALLROOT.
    force Force the action to override saved state.
    never Run make -N and show other actions.
    only  Only operate on the specified packages.
    quiet Do not list captured action output.
    show  Run make -n and show other actions.
    verbose
          Provide detailed action output.
    DEBUG Trace the package script actions in detail.

  The actions are:
    clean | clobber
          Clean up the flat view, if any. Then, unless flat was given, delete
          the arch/HOSTTYPE hierarchy; this deletes all generated files and
          directories for HOSTTYPE. The hierarchy can be rebuilt by package
          make.
    export [ variable ...]
          List name=value for variable, one per line. If the only attribute is
          specified then only the variable values are listed. If no variables
          are specified then HOSTTYPE PACKAGEROOT INSTALLROOT PATH are assumed.
    help [ action ]
          Display help text on the standard error (standard output for action).
    host [ attribute ... ]
          List architecture/implementation dependent host information on the
          standard output. type is listed if no attributes are specified.
          Information is listed on a single line in attribute order. The
          attributes are:
            canon name
                  An external host type name to be converted to package syntax.
            cpu   The number of CPUs; 1 if the host is not a multiprocessor.
            name  The host name.
            type  The host type, usually in the form vendor.architecture, with
                  an optional trailing -version. The main theme is that type
                  names within a family of architectures are named in a
                  similar, predictable style. OS point release information is
                  avoided as much as possible, but vendor resistance to release
                  incompatibilities has for the most part been futile.
    install [ dest_dir ] [ command ... ]
          Install commands from the $INSTALLROOT tree into appropriate
          subdirectories of dest_dir. If dest_dir does not exist, then it and
          any necessary subdirectories are created. dest_dir can be a directory
          like /usr/local to install the commands directly, or a temporary
          directory like /tmp/pkgtree/usr to prepare for packaging with
          operating system-specific tools. If no command is specified, then ksh
          and shcomp are assumed.
    make [ package ] [ option ... ] [ target ... ]
          Build and install. The default target is install, which makes and
          installs package. If the standard output is a terminal then the
          output is also captured in $INSTALLROOT/lib/package/gen/make.out. The
          build is done in the $INSTALLROOT directory tree viewpathed on top of
          the $PACKAGEROOT directory tree. Leaf directory names matching the
          |-separated shell pattern $MAKESKIP are ignored. The view action is
          done before making. option operands are passed to the underlying make
          command.
    results [ failed ] [ path ] [ old ] [make | test | write ]
          List results and interesting messages captured by the most recent
          make (default), test or write action. old specifies the previous
          results, if any (current and previous results are retained).
          $HOME/.pkgresults, if it exists, must contain an egrep(1) expression
          of result lines to be ignored. failed lists failures only and path
          lists the results file path name only.
    test [ dir ]
          Run all available default regression tests. If the optional dir
          argument (such as src/cmd/ksh93) is given, only the tests in that
          directory are run. If the standard output is a terminal then the
          output is also captured in $INSTALLROOT/lib/package/gen/test.out.
          Programs must be made before they can be tested. For ksh, a separate
          shtests command is available that allows passing arguments to select
          and tune the regression tests. See bin/shtests --man for more
          information.
    use [ uid | package | . | - [ command ... ] ]
          Run command, or an interactive shell if command is omitted, with the
          environment initialized for using the package. If uid or package or .
          is specified then it is used to determine a $PACKAGEROOT, possibly
          different from the current directory. For example, to try out bozo'\''s
          package: package use bozo. The use action may be run from any
          directory. If the file $INSTALLROOT/lib/package/profile is readable
          then it is sourced to initialize the environment.
    view  Initialize the architecture specific viewpath hierarchy. The make
          action implicitly calls this action.

DETAILS
  The package directory hierarchy is rooted at $PACKAGEROOT. All source and
  binaries reside under this tree. A two level viewpath is used to separate
  source and binaries. The top view is architecture specific, the bottom view
  is shared source. All building is done in the architecture specific view; no
  source view files are intentionally changed. This means that many different
  binary architectures can be made from a single copy of the source.

  Independent $PACKAGEROOT hierarchies can be combined by appending
  $INSTALLROOT:$PACKAGEROOT pairs to VPATH. The VPATH viewing order is from
  left to right.

  $HOSTTYPE names the current binary architecture and is determined by the
  output of package (no arguments). The $HOSTTYPE naming scheme is used to
  separate incompatible executable and object formats. All architecture
  specific binaries are placed under $INSTALLROOT
  ($PACKAGEROOT/arch/$HOSTTYPE). There are a few places that match against
  $HOSTTYPE when making binaries; these are limited to makefile compiler
  workarounds, e.g., if $HOSTTYPE matches hp.* then turn off the optimizer for
  these objects. All other architecture dependent logic is handled either by
  the AST iffe(1) command or by component specific configure scripts. Explicit
  $HOSTTYPE values matching *,*cc*[,-*,...] optionally set the default CC and
  CCFLAGS. This is handy for build farms that support different compilers on
  the same architecture.

  Each component contains a MAM (make abstract machine) file (Mamfile). A
  Mamfile contains a portable makefile description written in a simple
  dependency tree language using indented make...done blocks.

  All scripts and commands under $PACKAGEROOT use $PATH relative pathnames (via
  the AST pathpath(3) function); there are no embedded absolute pathnames. This
  means that binaries generated under $PACKAGEROOT may be copied to a different
  root; users need only change their $PATH variable to reference the new
  installation root bin directory. package install installs binary packages in
  a new $INSTALLROOT.

SEE ALSO
  autoconfig(1), cksum(1), expmake(1), gzip(1), make(1), mamake(1), pax(1),
  pkgadd(1), pkgmk(1), rpm(1), sh(1), tar(1), optget(3)

IMPLEMENTATION
  version         package (ksh 93u+m) 2023-01-08
  author          Glenn Fowler <gsf@research.att.com>
  author          Contributors to https://github.com/ksh93/ksh
  copyright       (c) 1994-2012 AT&T Intellectual Property
  copyright       (c) 2020-2022 Contributors to https://github.com/ksh93/ksh
  license         https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html'
		case $1 in
		html)	echo "</pre></body></html>" ;;
		esac
		exit $code
		;;
	*=*)	set DEFAULT host type "$@"
		;;
	*)	# simulate AST getopt(3) usage output
		echo "Usage: $command [ options ] [ qualifier ... ] [ action ] [ arg ... ] [ n=v ... ]" >&2
		echo " Help: $command [ --help | --man ] 2>&1" >&2
		exit 2
		;;
	esac
	global="$global $1"
	shift
done

# gather HOSTTYPE *,* options
# 	,*cc*,-*,...	set CC and CCFLAGS

hostopts()
{
	_ifs_=$IFS
	IFS=,
	set '' $HOSTTYPE
	IFS=$_ifs_
	shift
	while	:
	do	case $# in
		0|1)	break ;;
		esac
		shift
		case $1 in
		*cc*)	CC=$1
			while	:
			do	case $# in
				0|1)	break ;;
				esac
				case $2 in
				-*)	case $assign_CCFLAGS in
					?*)	assign_CCFLAGS="$assign_CCFLAGS " ;;
					esac
					assign_CCFLAGS="$assign_CCFLAGS$2"
					shift
					;;
				*)	break
					;;
				esac
			done
			;;
		esac
	done
}

# collect command line targets and definitions

case $_PACKAGE_HOSTTYPE_ in
?*)	HOSTTYPE=$_PACKAGE_HOSTTYPE_
	KEEP_HOSTTYPE=1
	;;
*)	KEEP_HOSTTYPE=0
	;;
esac
KEEP_PACKAGEROOT=0
KEEP_SHELL=0	# set to 1 if SHELL is a known-good system shell, 2 if SHELL supplied by user
USER_VPATH=
args=
assign=
assign_CCFLAGS=
for i
do	case $i in
	*:*=*)	args="$args $i"
		continue
		;;
	*=*)	n=${i%%=*} v=${i#*=}
		;;
	esac
	case $i in
	AR=*|LD=*|NM=*)
		assign="$assign $n='$v'"
		eval $n='$'v
		;;
	CC=*)	eval $n='$'v
		;;
	CCFLAGS=*)
		eval $n='$'v
		assign_CCFLAGS="CCFLAGS=\"\$CCFLAGS\""
		;;
	HOSTTYPE=*)
		eval $n='$'v
		case $HOSTTYPE in
		?*)	KEEP_HOSTTYPE=1 ;;
		esac
		;;
	PACKAGEROOT=*)
		eval $n='$'v
		case $PACKAGEROOT in
		?*)	KEEP_PACKAGEROOT=1 ;;
		esac
		;;
	SHELL=*)eval $n='$'v
		case $SHELL in
		?*)	KEEP_SHELL=2 ;;
		esac
		;;
	VPATH=*)eval USER_$n='$'v
		;;
	'debug=1')
		makeflags="$makeflags --debug-symbols"
		;;
	'strip=1')
		makeflags="$makeflags --strip-symbols"
		;;
	*=*)	assign="$assign $n='$v'"
		;;
	*)	args="$args $i"
		;;
	esac
done
case $HOSTTYPE in
*,*)	hostopts $HOSTTYPE ;;
esac
case $assign_CCFLAGS in
?*)	assign="$assign $assign_CCFLAGS"
esac
case $CC in
''|cc)	;;
*)	export CC ;;
esac

# grab action specific args

case $action in
use)	case $1 in
	.)	shift
		PACKAGEROOT=$PWD
		$show export PACKAGEROOT
	esac
	;;
esac

# true if arg is a valid PACKAGEROOT

packageroot() # dir
{
	test -d "$1/lib/$command" || test -x "$1/bin/$command"
}

# true if arg is executable

executable() # [!] command
{
	case $1 in
	'!')	test ! -x "$2" && test ! -x "$2.exe" ;;
	*)	test -x "$1" || test -x "$1.exe" ;;
	esac
}

# initialize SHELLMAGIC
# tangible proof of Cygwin's disdain for Unix

shellmagic()
{
	case $SHELLMAGIC in
	'')	;;
	-)	if	test -f /emx/bin/sh.exe
		then	SHELLMAGIC='#!/emx/bin/sh.exe'$nl
		elif	test -f /bin/env.exe
		then	SHELLMAGIC='#!/bin/env sh'$nl
		else	SHELLMAGIC=
		fi
		;;
	esac
}

# true if arg is executable command on $PATH

onpath() # command
{
	_onpath_b=$1
	case $_onpath_b in
	/*)	if	executable $_onpath_b
		then	_onpath_=$_onpath_b
			return 0
		fi
		return 1
		;;
	esac
	IFS=':'
	set '' $PATH
	IFS=$ifs
	shift
	for _onpath_d
	do	case $_onpath_d in
		'')	_onpath_d=. ;;
		esac
		if	executable "$_onpath_d/$_onpath_b"
		then	_onpath_=$_onpath_d/$_onpath_b
			return 0
		fi
	done
	return 1
}

# determine local host attributes

hostinfo() # attribute ...
{
	case $DEBUG in
	1)	set -x ;;
	esac
	map=
	something=
	path=$PATH
	for i in $ccs
	do	PATH=$PATH:$i
	done
	for i in $use
	do	for j in $org
		do	PATH=$PATH:$i/$j/bin
		done
		PATH=$PATH:$i/bin
	done
	case $PACKAGE_PATH in
	?*)	for i in $(echo "$PACKAGE_PATH" | sed 's,:, ,g')
		do	PATH=$PATH:$i/bin
		done
		;;
	esac

	# validate the args

	canon=
	cc=$CC
	for info
	do	case $canon in
		-)	canon=$info
			;;
		*)	case $info in
			*/*|*[cC][cC])
				cc=$info
				;;
			canon)	canon=-
				something=1
				;;
			cpu|name|type)
				something=1
				;;
			*)	err_out "$action: $info: unknown attribute"
				;;
			esac
			;;
		esac
	done
	case $canon in
	-)	err_out "$action: canon: host type name expected"
		;;
	esac
	case $something in
	"")	set "$@" type ;;
	esac
	case $DEBUG in
	'')	exec 9>&2
		exec 2>/dev/null
		;;
	esac

	# compute the info

	_hostinfo_=
	for info
	do
	case $info in
	cpu)	cpu=$(sysctl -n hw.ncpu)
		case $cpu in
		[123456789]*)
			_hostinfo_="$_hostinfo_ $cpu"
			continue
			;;
		esac
		cpu=$(grep -ic '^processor[ 	][ 	]*:[ 	]*[0123456789]' /proc/cpuinfo)
		case $cpu in
		[123456789]*)
			_hostinfo_="$_hostinfo_ $cpu"
			continue
			;;
		esac
		cpu=1
		# exact match
		set							\
			hinv			'^Processor [0123456789]'	\
			psrinfo			'on-line'		\
			'cat /reg/LOCAL_MACHINE/Hardware/Description/System/CentralProcessor'					'.'			\
			'cat /proc/registry/HKEY_LOCAL_MACHINE/Hardware/Description/System/CentralProcessor'			'.'			\

		while	:
		do	case $# in
			0)	break ;;
			esac
			i=$($1 2>/dev/null | grep -c "$2")
			case $i in
			[123456789]*)
				cpu=$i
				break
				;;
			esac
			shift;shift
		done
		case $cpu in
		0|1)	set						\
			/bin/mpstat

			while	:
			do	case $# in
				0)	break ;;
				esac
				if	executable $1
				then	case $($1 | grep -ic '^cpu ') in
					1)	cpu=$($1 | grep -ic '^ *[0123456789][0123456789]* ')
						break
						;;
					esac
				fi
				shift
			done
			;;
		esac
		case $cpu in
		0|1)	# token match
			set						\
			/usr/kvm/mpstat			'cpu[0123456789]'	\
			/usr/etc/cpustatus		'enable'	\
			/usr/alliant/showsched		'CE'		\
			'ls /config/hw/system/cpu'	'cpu'		\
			prtconf				'cpu-unit'	\

			while	:
			do	case $# in
				0)	break ;;
				esac
				i=$($1 2>/dev/null | tr ' 	' '

' | grep -c "^$2")
				case $i in
				[123456789]*)
					cpu=$i
					break
					;;
				esac
				shift;shift
			done
			;;
		esac
		case $cpu in
		0|1)	# special match
			set						\
									\
			hinv						\
			'/^[0123456789][0123456789]* .* Processors*$/'		\
			'/[ 	].*//'					\
									\
			/usr/bin/hostinfo				\
			'/^[0123456789][0123456789]* .* physically available\.*$/'	\
			'/[ 	].*//'					\

			while	:
			do	case $# in
				0)	break ;;
				esac
				i=$($1 2>/dev/null | sed -e "${2}!d" -e "s${3}")
				case $i in
				[123456789]*)
					cpu=$i
					break
					;;
				esac
				shift;shift;shift
			done
			;;
		esac
		case $cpu in
		0|1)	cpu=$(
			cd "$TMPDIR"
			tmp=hi$$
			trap 'rm -f $tmp.*' 0 1 2
			cat > $tmp.c <<!
#include <stdio.h>
#include <pthread.h>
int main()
{
	printf("%d\n", pthread_num_processors_np());
	return 0;
}
!
			for o in -lpthread ''
			do	if	$CC $o -O -o $tmp.exe $tmp.c $o >/dev/null 2>&1 ||
					gcc $o -O -o $tmp.exe $tmp.c $o >/dev/null 2>&1
				then	./$tmp.exe
					break
				fi
			done
			)
			case $cpu in
			[0123456789]*)	;;
			*)	cpu=1 ;;
			esac
			;;
		esac
		_hostinfo_="$_hostinfo_ $cpu"
		;;
	name)	_name_=$(hostname || uname -n || cat /etc/whoami || echo local)
		_hostinfo_="$_hostinfo_ $_name_"
		;;
	type|canon)
		case $CROSS:$canon in
		0:)	case $cc in
			cc)	case $KEEP_HOSTTYPE:$HOSTTYPE in
				0:?*)	if	test -d ${PACKAGEROOT:-.}/arch/$HOSTTYPE
					then	KEEP_HOSTTYPE=1
					fi
					;;
				esac
				;;
			esac
			case $KEEP_HOSTTYPE in
			1)	_hostinfo_="$_hostinfo_ $HOSTTYPE"
				continue
				;;
			esac
			;;
		esac
		a=$($cc -dumpmachine $CCFLAGS 2>/dev/null)
		case $a in
		'')	case $CCFLAGS in
			?*)	a=$($cc -dumpmachine 2>/dev/null) ;;
			esac
			;;
		esac
		case $a in
		''|*' '*|*/*:*)
			;;
		*.*-*)	_hostinfo_="$_hostinfo_ $a"
			continue
			;;
		*-*-*)	case $canon in
			'')	canon=$a ;;
			esac
			;;
		*-*)	case $canon in
			'')	canon=${a%-*}-unknown-${a#*-} ;;
			esac
			;;
		*)	_hostinfo_="$_hostinfo_ $a"
			continue
			;;
		esac
		IFS=:
		set /$IFS$PATH
		IFS=$ifs
		shift
		f=../lib/hostinfo/typemap
		for i
		do	case $i in
			"")	i=. ;;
			esac
			case $canon in
			'')	case $cc in
				/*|cc)	;;
				*)	if	executable $i/$cc
					then	a=$($i/$cc -dumpmachine $CCFLAGS 2>/dev/null)
						case $a in
						'')	case $CCFLAGS in
							?*)	a=$($cc -dumpmachine 2>/dev/null) ;;
							esac
							;;
						esac
						case $a in
						''|*' '*|*/*:*)
							;;
						*-*)	canon=$a
							;;
						*)	_hostinfo_="$_hostinfo_ $a"
							continue 2
							;;
						esac
					fi
					;;
				esac
				;;
			esac
			if	test -f "$i/$f"
			then	map="$(grep -v '^#' $i/$f) $map"
			fi
		done

		# inconsistent -dumpmachine filtered here

		case $canon in
		*-*-linux-gnu*)
			;;
		*-linux-gnu*)
			# fix missing machine field, e.g. aarch64-linux-gnu => aarch64-unknown-linux-gnu
			canon=${canon%%-*}-unknown-${canon#*-} ;;
		esac
		case -${canon}- in
		--|*-powerpc-*)
			h=$(hostname || uname -n || cat /etc/whoami)
			case $h in
			'')	h=local ;;
			esac
			a=$(arch || uname -m || att uname -m || uname -s || att uname -s)
			case $a in
			*[\ \	]*)	a=$(echo $a | sed "s/[ 	]/-/g") ;;
			esac
			case $a in
			'')	a=unknown ;;
			esac
			m=$(mach || machine || uname -p || att uname -p)
			case $m in
			*[\ \	]*)	m=$(echo $m | sed "s/[ 	]/-/g") ;;
			esac
			case $m in
			'')	m=unknown ;;
			esac
			x=$(uname -a || att uname -a)
			case $x in
			'')	x="unknown $host unknown unknown unknown unknown unknown" ;;
			esac
			set "" $h $a $m $x
			expected=$1 host=$2 arch=$3 mach=$4 os=$5 sys=$6 rel=$7 ver=$8
			;;
		*)	case $canon in
			*-*)	IFS=-
				set "" $canon
				shift
				IFS=$ifs
				case $# in
				2)	host= mach= arch=$1 os=$2 sys= rel= ;;
				*)	host= mach=$2 arch=$1 os=$3 sys= rel= ;;
				esac
				case $os in
				[abcdefghijklmnopqrstuvwxyz]*[0123456789])
					eval $(echo $os | sed -e 's/^\([^0123456789.]*\)\.*\(.*\)/os=\1 rel=\2/')
					;;
				esac
				;;
			*)	arch=$canon mach= os= sys= rel=
				;;
			esac
			;;
		esac
		type=unknown
		case $host in
		*.*)	host=$(echo $host | sed -e 's/\..*//') ;;
		esac
		case $arch in
		aarch64)
			# some call it aarch64, some arm64 -- let's stick to one
			arch=arm64
			;;
		esac
		case $mach in
		unknown)
			mach=
			;;
		[Rr][0123][0123456789][0123456789][0123456789])
			mach=mips1
			;;
		[Rr][4][0123456789][0123456789][0123456789])
			mach=mips2
			;;
		[Rr][56789][0123456789][0123456789][0123456789]|[Rr][123456789][0123456789][0123456789][0123456789][0123456789])
			mach=mips4
			;;
		pc)	arch=i386
			mach=
			;;
		[Pp][Oo][Ww][Ee][Rr][Pp][Cc])
			arch=ppc
			mach=
			;;
		*)	case $arch in
			34[0123456789][0123456789])
				os=ncr
				arch=i386
				;;
			esac
			;;
		esac
		case $canon in
		'')	set						\
									\
			/NextDeveloper		-d	next	-	\
			/config/hw/system/cpu	-d	tandem	mach	\

			while	:
			do	case $# in
				0)	break ;;
				esac
				if	test $2 $1
				then	os=$3
					case $4 in
					arch)	mach=$arch ;;
					mach)	arch=$mach ;;
					esac
					break
				fi
				shift;shift;shift;shift
			done
			;;
		esac
		case $os in
		AIX*|aix*)
			type=ibm.risc
			;;
		HP-UX)	case $arch in
			9000/[78]*)
				type=hp.pa
				;;
			*/*)	type=hp.$(echo $arch | sed 's,/,_,g')
				;;
			*)	type=hp.$arch
				;;
			esac
			;;
		[Ii][Rr][Ii][Xx]*)
			set xx $(hinv | sed -e '/^CPU:/!d' -e 's/CPU:[ 	]*\([^ 	]*\)[ 	]*\([^ 	]*\).*/\1 \2/' -e q | tr ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz)
			shift
			type=$1
			n=
			case $2 in
			r[0123][0123456789][0123456789][0123456789])
				n=1
				;;
			r[4][0123][0123456789][0123456789])
				n=2
				;;
			r[4][456789][0123456789][0123456789]|r[5][0123456789][0123456789][0123456789])
				n=3
				;;
			r[6789][0123456789][0123456789][0123456789]|r[123456789][0123456789][0123456789][0123456789][0123456789])
				n=4
				;;
			esac
			case $rel in
			[01234].*|5.[012]|5.[012].*)
				case $n in
				1)	;;
				*)	n=2 ;;
				esac
				;;
			5.*)	case $n in
				2)	n=3 ;;
				esac
				;;
			esac
			if	executable $cc
			then	a=$cc
			else	IFS=:
				set /$IFS$PATH
				IFS=$ifs
				shift
				for i
				do	a=$i/$cc
					if	executable $a
					then	break
					fi
				done
			fi
			split='
'
			a=$(strings $a < /dev/null | sed -e 's/[^abcdefghijklmnopqrstuvwxyz0123456789]/ /g' -e 's/[ 	][ 	]*/\'"$split"'/g' | sed -e "/^${type}[0123456789]$/!d" -e "s/^${type}//" -e q)
			case $a in
			[0123456789])	n=$a ;;
			esac
			case $n in
			4)	a=$($cc -${type}3 2>&1)
				case $a in
				*unknown*|*install*|*conflict*)
					;;
				*)	n=3
					;;
				esac
				;;
			esac
			a=$($cc -show F0oB@r.c 2>&1)
			case $n:$a in
			[!2]:*mips2*)	n=2 ;;
			[!23]:*mips3*)	n=3 ;;
			[!234]:*mips4*)	n=4 ;;
			esac
			case $n:$a in
			[!2]:*[Oo]32*)	abi=-o32 ;;
			[!3]:*[Nn]32*)	abi=-n32 ;;
			esac
			mach=${type}$n
			type=sgi.$mach
			;;
		OSx*|SMP*|pyramid)
			type=pyr
			;;
		OS/390)	type=mvs.390
			;;
		[Ss][Cc][Oo]*)
			type=sco
			;;
		[Ss]ol*)
			v=$(echo $rel | sed -e 's/^[25]\.//' -e 's/\.[^.]*$//')
			case $v in
			[6789]|[1-9][0-9])
				;;
			*)	v=
				;;
			esac
			case $arch in
			'')	case $mach in
				'')	arch=sun4 ;;
				*)	arch=$mach ;;
				esac
				;;
			esac
			case $arch in
			sparc)	arch=sun4 ;;
			esac
			type=sol$v.$arch
			;;
		[Ss]un*)type=$(echo $arch | sed -e 's/\(sun.\).*/\1/')
			case $type in
			sparc)	type=sun4 ;;
			esac
			case $rel in
			[01234]*)
				;;
			'')	case $os in
				*[Oo][Ss])
					;;
				*)	type=sol.$type
					;;
				esac
				;;
			*)	case $type in
				'')	case $mach in
					sparc*)	type=sun4 ;;
					*)	type=$mach ;;
					esac
					;;
				esac
				v=$(echo $rel | sed -e 's/^[25]\.//' -e 's/\.[^.]*$//')
				case $v in
				[6789]|[1-9][0-9])
					;;
				*)	v=
					;;
				esac
				type=sol$v.$type
				;;
			esac
			case $type in
			sun*|*.*)
				;;
			*)	type=sun.$type
				;;
			esac
			;;
		[Uu][Nn][Ii][Xx]_[Ss][Vv])
			type=unixware
			;;
		UTS*|uts*)
			if	test -x /bin/u370 || test -x /bin/u390
			then	type=uts.390
			else	case $arch in
				'')	arch=$mach ;;
				esac
				type=uts.$arch
			fi
			;;
		$host)	type=$arch
			case $type in
			*.*|*[0123456789]*86|*68*)
				;;
			*)	case $mach in
				*[0123456789]*86|*68*|mips)
					type=$type.$mach
					;;
				esac
				;;
			esac
			;;
		unknown)
			case $arch in
			?*)	case $arch in
				sun*)	mach= ;;
				esac
				type=$arch
				case $mach in
				?*)	type=$type.$mach ;;
				esac
				;;
			esac
			;;
		*)	case $ver in
			FTX*|ftx*)
				case $mach in
				*[0123456789][abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ]*)
					mach=$(echo $mach | sed -e 's/[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ]*$//')
					;;
				esac
				type=stratus.$mach
				;;
			*)	case $arch in
				[Oo][Ss][-/.]2)
					type=os2
					arch=$rel
					;;
				*)	type=$(echo $os | sed -e 's/[0123456789].*//' -e 's/[^ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789.].*//')
					;;
				esac
				case $type in
				[Cc][Yy][Gg][Ww][Ii][Nn]_*)
					type=cygwin
					;;
				[Uu][Ww][Ii][Nn]*|[Ww]indows_[0123456789][0123456789]|[Ww]indows_[Nn][Tt])
					type=win32
					arch=$(echo $arch | sed -e 's/_[^_]*$//')
					;;
				esac
				case $arch in
				'')	case $mach in
					?*)	type=$type.$mach ;;
					esac
					;;
				*)	type=$type.$arch ;;
				esac
				;;
			esac
		esac
		case $type in
		[0123456789]*)
			case $mach in
			?*)	type=$mach ;;
			esac
			case $type in
			*/MC)	type=ncr.$type ;;
			esac
			;;
		*.*)	;;
		*[0123456789]*86|*68*)
			case $rel in
			[34].[0123456789]*)
				type=att.$type
				;;
			esac
			;;
		[abcdefghijklmnopqrstuvwxyz]*[0123456789])
			;;
		[abcdefghijklmnopqrstuvwxyz]*)	case $mach in
			$type)	case $ver in
				Fault*|fault*|FAULT*)
					type=ft.$type
					;;
				esac
				;;
			?*)	case $arch in
				'')	type=$type.$mach ;;
				*)	type=$type.$arch ;;
				esac
				;;
			esac
			;;
		esac
		case $type in
		*[-_]32|*[-_]64|*[-_]128)
			bits=$(echo $type | sed 's,.*[-_],,')
			type=$(echo $type | sed 's,[-_][0-9]*$,,')
			;;
		*)	bits=
			;;
		esac
		type=$(echo $type | sed -e 's%[-+/].*%%' | tr ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz)
		case $type in
		*.*)	lhs=$(echo $type | sed -e 's/\..*//')
			rhs=$(echo $type | sed -e 's/.*\.//')
			case $rhs in
			[x0123456789]*86)	rhs=i$rhs ;;
			68*)			rhs=m$rhs ;;
			esac
			case $rhs in
			i[x23456789]86|i?[x23456789]86|*86pc)
						rhs=i386 ;;
			powerpc)		rhs=ppc ;;
			s[0123456789]*[0123456789]x)
						rhs=$(echo $rhs | sed -e 's/x$/-64/') ;;
			esac
			case $rhs in
			arm[abcdefghijklmnopqrstuvwxyz_][0123456789]*)
						rhs=arm ;;
			hppa)			rhs=pa ;;
			esac
			case $lhs in
			?*coff|?*dwarf|?*elf)
				case $lhs in
				?*coff)	x=coff ;;
				?*dwarf)x=coff ;;
				?*elf)	x=elf ;;
				esac
				lhs=$(echo ${lhs}XXX | sed -e "s/${x}XXX//")
				;;
			esac
			case $lhs in
			bsdi)			lhs=bsd ;;
			darwin)			case $(/usr/bin/cc --version) in
						*'(GCC)'*)	case $rel in
								'' | [0-9].* | 10.*)
										lhs=darwin07 ;;
								*)		lhs=darwin11 ;;
								esac ;;
						esac
						;;
			freebsd)		case $rel in
						[01234].*)	lhs=${lhs}4 ;;
						[123456789]*.*)	lhs=${lhs}$(echo $rel | sed -e 's/\..*//') ;;
						esac
						;;
			hpux)			lhs=hp ;;
			mvs)			rhs=390 ;;
			esac
			case $lhs in
			'')			type=$rhs ;;
			$rhs)			type=$lhs ;;
			*)			type=$lhs.$rhs ;;
			esac
			;;
		esac
		case $type in
		sgi.mips*)
			case $mach in
			mips2)	type=sgi.$mach
				abi=-o32
				;;
			mips3)	type=sgi.$mach
				abi=-n32
				;;
			mips[456789])
				type=sgi.$mach
				case $abi in
				*-n32) ;;
				*) abi=-64 ;;
				esac
				;;
			*)	pwd=$PWD
				cd "$TMPDIR"
				tmp=hi$$
				trap 'rm -f $tmp.*' 0 1 2
				cat > $tmp.a.c <<!
extern int b();
int main() { return b(); }
!
				cat > $tmp.b.c <<!
int b() { return 0; }
!
				abi=
				if	$cc -c $tmp.a.c
				then	for i in -n32 -o32 -64
					do	if	$cc $i -c $tmp.b.c &&
							$cc -o $tmp.exe $tmp.a.o $tmp.b.o
						then	abi=$i
							for i in 2 3 4 5 6 7 8 9
							do	case $i:$abi in
								2:-n32|2:-64|3:-64)
									continue
									;;
								esac
								if	$cc $abi -mips$i -c $tmp.b.c &&
									$cc -o $tmp.exe $tmp.a.o $tmp.b.o
								then	type=$(echo $type | sed -e 's/.$//')$i
									break
								fi
							done
							break
						fi
					done
				fi </dev/null >/dev/null 2>&1
				rm -f $tmp.*
				trap - 0 1 2
				cd $pwd
				;;
			esac
			case $type$abi in
			sgi.mips2-o32)
				;;
			sgi.mips3)
				type=$type-o32
				;;
			sgi.mips3-n32)
				;;
			sgi.mips4)
				type=$type-o32
				;;
			sgi.mips[456789]-64)
				;;
			*)	type=$type$abi
				;;
			esac
			;;
		*)	case $bits in
			'')	bits=$(	set -e
					cd "$TMPDIR"
					tmp=hi$$
					trap 'rm -rf "$tmp".*' 0 1 2
					echo 'int main() { return 0; }' > $tmp.a.c
					checkcc
					$cc $CCFLAGS -o $tmp.a.exe $tmp.a.c </dev/null >/dev/null 2>&1
					file $tmp.a.exe 2>/dev/null | sed "s/$tmp\.a\.exe//g"  )
				case $bits in
				*\ 64-bit* | *\ 64\ bit* | *\ 64bit*)
					bits=64 ;;
				*)	bits= ;;
				esac
				;;
			esac
			;;
		esac
		case $bits in
		32)	case $type in
			*.i386)	bits= ;;
			esac
			;;
		esac
		case $bits in
		?*)	type=$type-$bits ;;
		esac

		# last chance mapping

		set "" "" $map
		while	:
		do	case $# in
			[012])	break ;;
			esac
			shift;shift
			eval "	case \$type in
				$1)	type=\$2; break ;;
				esac"
		done
		_hostinfo_="$_hostinfo_ $type"
		;;
	esac
	done
	set '' $_hostinfo_
	shift
	_hostinfo_=$*

	# restore the global state

	PATH=$path
	case $DEBUG in
	'')	exec 2>&9
		exec 9>&-
		;;
	esac
}

# info message

note() # message ...
{
	printf "$command: %s\\n" "$@" >&2
}

err_out()
{
	note "$@"
	exit 1
}

trace()
(
	PS4="${action}: executing: "
	exec 2>&1  # trace to standard output
	set -o xtrace
	"$@"
)

# cc checks
#
#	CC: compiler base name name
#	cc: full path, empty if not found

checkcc()
{
	cc=
	if	onpath $CC
	then	cc=$_onpath_
	else	case $CC in
		cc)	if	onpath clang
			then	CC=clang
				cc=$_onpath_
			elif	onpath gcc
			then	CC=gcc
				cc=$_onpath_
			fi
			;;
		esac
	fi
	case $cc in
	'')	case $action in
		make|test)	err_out "$CC: not found" ;;
		*)		note "warning: $CC: not found" ;;
		esac
		;;
	esac
}

# some actions have their own PACKAGEROOT or kick out early

case $action in
host)	eval u=$package_use
	case $u in
	$PACKAGE_USE)
		;;
	*)	if	onpath $0
		then	case $_onpath_ in
			*/arch/$HOSTTYPE/bin/package)
				KEEP_HOSTTYPE=1
				;;
			*)	KEEP_HOSTTYPE=0
				;;
			esac
		else	KEEP_HOSTTYPE=0
		fi
		;;
	esac
	hostinfo $args
	echo $_hostinfo_
	exit 0
	;;
export|setup|use)
	x=
	;;
*)	x=
	eval u=$package_use
	case $u in
	$PACKAGE_USE)
		case :$PATH: in
		*:$INSTALLROOT/bin:*)
			case $LIBPATH: in
			$INSTALLROOT/bin:$INSTALLROOT/lib:*)
				case $SHLIB_PATH: in
				$INSTALLROOT/lib:*)
					x=1
					;;
				esac
				;;
			esac
			;;
		esac
		;;
	esac
	;;
esac
run=-
case $x in
1)	: accept the current package use environment
	INITROOT=$PACKAGEROOT/src/cmd/INIT
	checkcc
	;;
*)	hosttype=
	case $KEEP_PACKAGEROOT in
	0)	case $action in
		use)	PACKAGEROOT=
			case $show in
			echo)	exec=echo make=echo show=echo ;;
			esac
			set '' $args
			shift
			case $# in
			0)	;;
			*)	case $1 in
				-|.)	;;
				/*)	PACKAGEROOT=$1
					;;
				*)	i=$(echo ~$1)
					if	packageroot $i
					then	PACKAGEROOT=$i
					else	for i in $(echo $HOME | sed -e 's,/[^/]*$,,') $usr $use
						do	if	packageroot $i/$1
							then	PACKAGEROOT=$i/$1
								break
							fi
						done
						case $PACKAGEROOT in
						'')	hosttype=$1 ;;
						esac
					fi
					;;
				esac
				shift
				;;
			esac
			run="$@"
			;;
		esac
		case $PACKAGEROOT in
		'')	PACKAGEROOT=$PWD ;;
		esac

		# . must be within the PACKAGEROOT tree

		i=X$PACKAGEROOT
		IFS=/
		set $i
		IFS=$ifs
		while	:
		do	i=$1
			shift
			case $i in
			X)	break ;;
			esac
		done
		case $PACKAGEROOT in
		//*)	d=/ ;;
		*)	d= ;;
		esac
		case $1 in
		home)	k=1 ;;
		*)	k=0 ;;
		esac
		for i
		do	case $i in
			'')	continue ;;
			esac
			d=$d/$i
			case $k in
			2)	k=1
				;;
			1)	k=0
				;;
			0)	case $i in
				arch)	k=2
					;;
				*)	if	packageroot $d
					then	PACKAGEROOT=$d
					fi
					;;
				esac
				;;
			esac
		done
		;;
	esac
	INITROOT=$PACKAGEROOT/src/cmd/INIT
	$show PACKAGEROOT=$PACKAGEROOT
	$show export PACKAGEROOT
	export PACKAGEROOT

	# initialize the architecture environment

	case $KEEP_HOSTTYPE in
	0)	hostinfo type
		HOSTTYPE=$_hostinfo_
		;;
	1)	_PACKAGE_HOSTTYPE_=$HOSTTYPE
		export _PACKAGE_HOSTTYPE_
		;;
	esac
	$show HOSTTYPE=$HOSTTYPE
	$show export HOSTTYPE
	export HOSTTYPE
	INSTALLROOT=$PACKAGEROOT/arch/$HOSTTYPE
	case $action in
	install|make|remove|test|view)
		;;
	*)	if	test ! -d $INSTALLROOT
		then	INSTALLROOT=$PACKAGEROOT
		fi
		;;
	esac
	$show INSTALLROOT=$INSTALLROOT
	$show export INSTALLROOT
	export INSTALLROOT

	# check the basic package hierarchy

	case $action in
	export|use)
		packageroot $PACKAGEROOT || err_out "$PACKAGEROOT: invalid package root directory"
		case $KEEP_HOSTTYPE:$hosttype in
		0:?*)	if	test -d ${PACKAGEROOT:-.}/arch/$hosttype
			then	KEEP_HOSTTYPE=1
				HOSTTYPE=$hosttype
			else	err_out "$hosttype: package root not found"
			fi
			;;
		esac
		;;
	*)	packageroot $PACKAGEROOT || {
			case $KEEP_PACKAGEROOT in
			1)	;;
			*)	err_out "$PACKAGEROOT: must be in the package root directory tree"
				;;
			esac
		}

		for i in arch arch/$HOSTTYPE
		do	test -d $PACKAGEROOT/$i || $exec mkdir $PACKAGEROOT/$i || exit
		done
		for i in lib
		do	test -d $INSTALLROOT/$i || $exec mkdir $INSTALLROOT/$i || exit
		done

		# no $INITROOT means INIT already installed elsewhere

		if	test -d $INITROOT
		then
			# update the basic package commands

			for i in mamprobe
			do	test -h $PACKAGEROOT/bin/$i 2>/dev/null ||
				case $(ls -t $INITROOT/$i.sh $PACKAGEROOT/bin/$i 2>/dev/null) in
				"$INITROOT/$i.sh"*)
					note "update $PACKAGEROOT/bin/$i"
					shellmagic
					case $SHELLMAGIC in
					'')	$exec cp $INITROOT/$i.sh $PACKAGEROOT/bin/$i || exit
						;;
					*)	case $exec in
						'')	{
							echo "$SHELLMAGIC"
							cat $INITROOT/$i.sh
							} > $PACKAGEROOT/bin/$i || exit
							;;
						*)	echo "{
echo \"$SHELLMAGIC\"
cat $INITROOT/$i.sh
} > $PACKAGEROOT/bin/$i"
							;;
						esac
						;;
					esac
					$exec chmod +x $PACKAGEROOT/bin/$i || exit
					;;
				esac
			done
		fi
		;;
	esac
	path=$PATH
	PATH=$INSTALLROOT/bin:$PACKAGEROOT/bin:$PATH
	checkcc
	PATH=$path
	case $cc in
	?*)	if	test -f $INITROOT/hello.c
		then
			# check if $CC (full path $cc) is a cross compiler

			(
				cd "$TMPDIR" || exit 3
				cp $INITROOT/hello.c pkg$$.c || exit 3
				$cc -o pkg$$.exe pkg$$.c > pkg$$.e 2>&1 || {
					if $cc -Dnew=old -o pkg$$.exe pkg$$.c > /dev/null 2>&1
					then	err_out "${warn}$CC: must be a C compiler (not C++)"
					else	cat pkg$$.e
						err_out "${warn}$CC: failed to compile and link $INITROOT/hello.c -- is it a C compiler?"
					fi
				}
				if ./pkg$$.exe >/dev/null 2>&1
				then	code=0
				else	code=1
				fi
				rm -f pkg$$.*
				exit $code
			)
			code=$?
			case $code in
			1)	CROSS=1 ;;
			esac
		fi
		;;
	esac
	EXECTYPE=$HOSTTYPE
	EXECROOT=$INSTALLROOT
	case $CROSS in
	0) 	# dll hackery -- why is this so complicated?

		abi=
		case $HOSTTYPE in
		sgi.mips[0123456789]*)
			x=rld
			if	executable /lib32/$x || executable /lib64/$x
			then	case $INSTALLROOT in
				*/sgi.mips[0123456789]*)
					u=$(echo $INSTALLROOT | sed -e 's,-[^-/]*$,,' -e 's,.$,,')
					;;
				*)	u=
					;;
				esac
				for a in "n=2 v= l=" "n=3 v=N32 l=lib32" "n=4-n32 v=N32 l=lib32" "n=4 v=64 l=lib64"
				do	eval $a
					case $v in
					N32)	case $n:$HOSTTYPE in
						*-n32:*-n32)	;;
						*-n32:*)	continue ;;
						*:*-n32)	continue ;;
						esac
						;;
					esac
					case $l in
					?*)	if	executable ! /$l/$x
						then	continue
						fi
						;;
					esac
					case $u in
					'')	case $HOSTTYPE in
						sgi.mips$n|sgi.mips$n-*)
							abi="$abi 'd=$INSTALLROOT v=$v'"
							;;
						*)	continue
							;;
						esac
						;;
					*)	if	test -d $u$n
						then	abi="$abi 'd=$u$n v=$v'"
						fi
						;;
					esac
				done
			fi
			;;
		esac
		case $abi in
		'')	abi="'d=$INSTALLROOT v='" ;;
		esac
		p=0
		eval "
			for a in $abi
			do	eval \$a
				eval \"
					case \\\$LD_LIBRARY\${v}_PATH: in
					\\\$d/lib:*)
						;;
					*)	x=\\\$LD_LIBRARY\${v}_PATH
						case \\\$x in
						''|:*)	;;
						*)	x=:\\\$x ;;
						esac
						LD_LIBRARY\${v}_PATH=\$d/lib\\\$x
						export LD_LIBRARY\${v}_PATH
						p=1
						;;
					esac
				\"
			done
		"
		case $LD_LIBRARY_PATH in
		'')	;;
		*)	for d in $lib
			do	case $HOSTTYPE in
				*64)	if	test -d ${d}64
					then	d=${d}64
					fi
					;;
				esac
				case :$LD_LIBRARY_PATH: in
				*:$d:*)	;;
				*)	if	test -d $d
					then	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$d
						p=1
					fi
					;;
				esac
			done
			;;
		esac
		case $p in
		1)	$show LD_LIBRARY_PATH=$LD_LIBRARY_PATH
			$show export LD_LIBRARY_PATH
			export LD_LIBRARY_PATH
			;;
		esac
		case $LIBPATH: in
		$INSTALLROOT/bin:$INSTALLROOT/lib:*)
			;;
		*)	case $LIBPATH in
			'')	LIBPATH=/usr/lib:/lib ;;
			esac
			LIBPATH=$INSTALLROOT/bin:$INSTALLROOT/lib:$LIBPATH
			$show LIBPATH=$LIBPATH
			$show export LIBPATH
			export LIBPATH
			;;
		esac
		case $SHLIB_PATH: in
		$INSTALLROOT/lib:*)
			;;
		*)	SHLIB_PATH=$INSTALLROOT/lib${SHLIB_PATH:+:$SHLIB_PATH}
			$show SHLIB_PATH=$SHLIB_PATH
			$show export SHLIB_PATH
			export SHLIB_PATH
			;;
		esac
		case $DYLD_LIBRARY_PATH: in
		$INSTALLROOT/lib:*)
			;;
		*)	DYLD_LIBRARY_PATH=$INSTALLROOT/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}
			$show DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH
			$show export DYLD_LIBRARY_PATH
			export DYLD_LIBRARY_PATH
			;;
		esac
		case $_RLD_ROOT in
		$INSTALLROOT/arch*)	;;
		':')	_RLD_ROOT=$INSTALLROOT/arch:/ ;;
		/|*:/)	_RLD_ROOT=$INSTALLROOT/arch:$_RLD_ROOT ;;
		*)	_RLD_ROOT=$INSTALLROOT/arch:$_RLD_ROOT:/ ;;
		esac
		$show _RLD_ROOT=$_RLD_ROOT
		$show export _RLD_ROOT
		export _RLD_ROOT

		# now set up PATH
		#
		# NOTE: PACKAGEROOT==INSTALLROOT is possible for binary installations

		case $PATH: in
		$PACKAGEROOT/bin:*)
			;;
		*)	PATH=$PACKAGEROOT/bin:$PATH
			;;
		esac
		case $PATH: in
		$INSTALLROOT/bin:*)
			;;
		*)	PATH=$INSTALLROOT/bin:$PATH
			;;
		esac
		$show PATH=$PATH
		$show export PATH
		export PATH
		;;
	*)	for i in package
		do	if	onpath $i
			then	EXECROOT=$(echo $_onpath_ | sed -e 's,//*[^/]*//*[^/]*$,,')
				EXECTYPE=$(echo $EXECROOT | sed -e 's,.*/,,')
				break
			fi
		done
		case $HOSTTYPE in
		$EXECTYPE)
			OCC=$CC
			CC=cc
			hostinfo type
			EXECTYPE=$_hostinfo_
			case $HOSTTYPE in
			$EXECTYPE)
				err_out "$CC seems to be a cross-compiler." \
					"Set HOSTTYPE to something other than the native $EXECTYPE." \
					"If not, your $TMPDIR directory may be mounted without execute permission." \
					"Try exporting TMPDIR as a directory where you can execute binary files."
				;;
			esac
			;;
		esac
		$show EXECTYPE=$EXECTYPE
		$show export EXECTYPE
		export EXECTYPE
		;;
	esac
	$show EXECROOT=$EXECROOT
	$show export EXECROOT
	export EXECROOT

	# grab a decent default shell

	case $KEEP_SHELL in
	0)	save_PATH=$PATH
		if	PATH=$(getconf PATH 2>/dev/null)
		then	PATH=$PATH:$path
		else	PATH=/bin:/usr/bin:/sbin:/usr/sbin:$path
		fi
		for i in ksh ksh93 mksh yash bash sh
		do	if onpath "$i" && checksh "$_onpath_"
			then	SHELL=$_onpath_
				KEEP_SHELL=1
				break
			fi
		done
		PATH=$save_PATH
		unset save_PATH
		case $KEEP_SHELL in
		0)	err_out "Cannot find a good default shell; please supply SHELL=/path/to/shell"
			;;
		esac
		;;
	esac
	export SHELL
	$show SHELL=$SHELL
	$show export SHELL

	# tame the environment

	case $action in
	use)	;;
	*)	ENV=
		ERROR_OPTIONS=
		export ENV ERROR_OPTIONS
		;;
	esac

	# finalize the views

	case $USER_VPATH in
	'')	case $VPATH in
		?*)	IFS=':'
			set '' $VPATH
			shift
			IFS=$ifs
			USER_VPATH=
			for i
			do	case $i in
				*/arch/$HOSTTYPE)	;;
				*/arch/*/*)		;;
				*/arch/*)		continue ;;
				esac
				if	packageroot $i
				then	case $USER_VPATH in
					'')	USER_VPATH=$i ;;
					?*)	USER_VPATH=$USER_VPATH:$i ;;
					esac
				fi
			done
		esac
		;;
	esac
	case $USER_VPATH in
	?*)	IFS=':'
		set '' $USER_VPATH
		shift
		IFS=$ifs
		USER_VPATH=
		USER_VPATH_CHAIN=
		p=$PACKAGEROOT
		for i
		do	case $i in
			''|$PACKAGEROOT|$INSTALLROOT)
				;;
			?*)	USER_VPATH=$USER_VPATH:$i
				USER_VPATH_CHAIN="$USER_VPATH_CHAIN $p $i"
				p=$i
				;;
			esac
		done
		;;
	esac
	;;
esac

PACKAGEBIN=$INSTALLROOT/lib/package
case $action:$run in
use:-)	set '' $args
	shift
	case $# in
	0)	;;
	*)	shift ;;
	esac
	run="$@"
	;;
esac

# HOSTTYPE specific package profile

if	test -r $INSTALLROOT/lib/package/profile
then	. $INSTALLROOT/lib/package/profile
fi

# more Cygwin hassles

case $HOSTTYPE in
cygwin.*)
	lose=
	case $CYGWIN in
	*nontsec*)
		lose=ntsec
		;;
	*ntsec*);;
	*)	exe=$TMPDIR/pkg$$.exe
		rm -f "$exe"
		: > "$exe"
		if	test -x "$exe"
		then	lose=ntsec
		fi
		;;
	esac
	case $CYGWIN in
	*nobinmode*)
		case $lose in
		'')	lose=binmode ;;
		*)	lose="$lose binmode" ;;
		esac
		;;
	esac
	case $lose in
	?*)	err_out "$HOSTTYPE: export '$lose' in CYGWIN or languish in Windows"
		;;
	esac
	;;
esac

# set up the view state

VIEW_bin=$INSTALLROOT VIEW_src=$PACKAGEROOT VIEW_all="$INSTALLROOT $PACKAGEROOT"
VPATH=$INSTALLROOT:$PACKAGEROOT$USER_VPATH
$show VPATH=$VPATH
$show export VPATH
export VPATH
IFS=':'
set '' $VPATH
shift
IFS=$ifs
for i
do	case $i in
	*/arch/*/*)
		VIEW_src="$VIEW_src $i"
		;;
	*/arch/*)
		VIEW_bin="$VIEW_bin $i"
		;;
	*)
		VIEW_src="$VIEW_src $i"
		;;
	esac
	VIEW_all="$VIEW_all $i"
done

# return 0 if arg in src|bin|all view

view() # [test] [-|type] [src|bin|all] file
{
	case $1 in
	-[dfsx])_view_T_=$1; shift ;;
	*)	_view_T_=-f ;;
	esac
	case $1 in
	-)	_view_t_= ;;
	*)	_view_t_=$1 ;;
	esac
	shift
	case $1 in
	all)	shift; _view_v_=$VIEW_all ;;
	bin)	shift; _view_v_=$VIEW_bin ;;
	src)	shift; _view_v_=$VIEW_src ;;
	*)	_view_v_=$VIEW_all ;;
	esac
	case $1 in
	/*)	if	test $_view_T_ $1
		then	_view_=$1
			return 0
		fi
		;;
	*)	for _view_d_ in $_view_v_
		do	if	test $_view_T_ $_view_d_/$1
			then	_view_=$_view_d_/$1
				return 0
			fi
		done
		;;
	esac
	_view_=
	case $_view_t_ in
	?*)	note "$1: $_view_t_ not found" ;;
	esac
	return 1
}

# determine the package and targets

case $action in
*)	package=
	target=
	set '' $args
	while	:
	do	shift
		case $# in
		0)	break ;;
		esac
		case $1 in
		''|-)	target="$target $package"
			package=
			;;
		*)	if	view - src "lib/package/$1.pkg"
			then	package="$package $1"
			else	target="$target $package $1"
				package=
			fi
			;;
		esac
	done
	;;
esac

# check that cmd args are up to date a.out's

checkaout()	# cmd ...
{
	case $cc in
	'')	_PACKAGE_cc=0
		;;
	*)	_PACKAGE_cc=1
		test -f "$INITROOT/hello.c" && test -f "$INITROOT/p.c" || {
			note "$INITROOT: INIT package source not found"
			return 1
		}

		for i in arch arch/$HOSTTYPE arch/$HOSTTYPE/bin
		do	test -d $PACKAGEROOT/$i || $exec mkdir $PACKAGEROOT/$i || return
		done
		;;
	esac
	case $_PACKAGE_cc in
	'')	case $cc in
		'')	_PACKAGE_cc=0 ;;
		*)	_PACKAGE_cc=1 ;;
		esac
		;;
	esac
	for i
	do	eval j=\$_PACKAGE_AOUT_$i
		case $j in
		'')	eval _PACKAGE_AOUT_$i=1 ;;
		*)	continue ;;
		esac
		k=$_PACKAGE_cc
		if	test -f $INITROOT/$i.c
		then	k=${k}1
		else	k=${k}0
		fi
		if	executable $EXECROOT/bin/$i
		then	k=${k}1
		else	k=${k}0
		fi
		: $k : compiler : source : binary :
		case $k in
		*00)	view - bin/$i && continue ;;
		esac
		case $k in
		000)	note "$i: not found: download the INIT package $HOSTTYPE binary to continue"
			return 1
			;;
		010)	note "$i: not found: set CC=C-compiler or download the INIT package $HOSTTYPE binary to continue"
			return 1
			;;
		100)	note "$i: not found: download the INIT package source or $HOSTTYPE binary to continue"
			return 1
			;;
		110)	case $CROSS in
			1)	note "$i: not found: make the local $EXECTYPE binary package before $HOSTTYPE"
				return 1
				;;
			esac
			;;
		?01)	: accept binary
			continue
			;;
		011)	: accept binary
			continue
			;;
		??1)	case $CROSS in
			1)	continue ;;
			esac
			;;
		esac
		case $(ls -t $INITROOT/$i.c $INSTALLROOT/bin/$i 2>/dev/null) in
		"$INITROOT/$i.c"*)
			note "update $INSTALLROOT/bin/$i"
			if	test ! -d $INSTALLROOT/bin
			then	for j in arch arch/$HOSTTYPE arch/$HOSTTYPE/bin
				do	test -d $PACKAGEROOT/$j || $exec mkdir $PACKAGEROOT/$j || return
				done
			fi
			$exec $CC $CCFLAGS -o $INSTALLROOT/bin/$i $INITROOT/$i.c || return
			test -f $i.o && $exec rm -f $i.o
			i=$PATH
			PATH=/bin
			PATH=$i
			;;
		esac
	done
	return 0
}

# list main environment values

showenv()
{
	case $1 in
	''|make)for __i__ in CC SHELL $env
		do	eval echo $__i__='$'$__i__
		done
		;;
	esac
}

# capture command output

capture() # file command ...
{
	tee_pid=
	case $make:$noexec in
	:)	case $action in
		install|make|view)
			o=$action
			;;
		*)	case $package in
			''|*' '*)
				o=$action
				;;
			*)	o=$package
				;;
			esac
			;;
		esac
		d=$PACKAGEBIN/gen
		test -d $d || $exec mkdir $d
		o=$d/$o
		case $o in
		$output)o=$o.out
			s=
			;;
		*)	output=$o
			if	test -f $o.old
			then	mv $o.old $o.out.1
				if	test -f $o.out
				then	mv $o.out $o.out.2
				fi
			elif	test -f $o.out
			then	for i in $(ls -t $o.out.? 2>/dev/null)
				do	break
				done
				case $i in
				*.1)	i=2 ;;
				*.2)	i=3 ;;
				*.3)	i=4 ;;
				*.4)	i=5 ;;
				*.5)	i=6 ;;
				*.6)	i=7 ;;
				*.7)	i=8 ;;
				*.8)	i=9 ;;
				*)	i=1 ;;
				esac
				mv $o.out $o.out.$i
			fi
			o=$o.out
			: > $o
			note "$action output captured in $o"
			s="$command: $action start at $(date) in $INSTALLROOT"
			cmd='case $error_status in 0) r=done;; *) r=failed;; esac;'
			cmd=$cmd' echo "$command: $action $r at $(date) in $INSTALLROOT"'
			case $quiet in
			0)	cmd="$cmd 2>&1 | tee -a $o" ;;
			*)	cmd="$cmd >> $o" ;;
			esac
			trap "$cmd" 0
			trap "error_status=1; $cmd; trap 1 0; kill -1 $$" 1
			trap "error_status=1; $cmd; trap 2 0; kill -2 $$" 2
			;;
		esac
		case $quiet in
		0)	# Connect 'tee' to a FIFO instead of a pipe, so that we can obtain
			# the build's exit status and use it for $error_status
			rm -f $o.fifo
			mkfifo -m 600 $o.fifo || exit
			(
				sleep 1
				# unlink early
				exec rm $o.fifo
			) &
			tee -a $o < $o.fifo &
			tee_pid=$!
			o=$o.fifo
			;;
		esac
		{
			case $s in
			?*)	echo "$s"  ;;
			esac
			showenv $action
			"$@"
		} < /dev/null > $o 2>&1
		;;
	*)	$make "$@"
		;;
	esac
	exit_status=$?
	if	test "$exit_status" -gt "$error_status"
	then	error_status=$exit_status
	fi
	case $tee_pid in
	?*)	# allow 'tee' to catch up before returning to prompt
		wait "$tee_pid"
		;;
	esac
}

make_recurse() # dir
{
	for _make_recurse_j in $makefiles
	do	if	view - $1/$_make_recurse_j
		then	return
		fi
	done
}

do_install() # dir [ command ... ]
{
	cd "$INSTALLROOT"
	printf 'install: installing from %s\n' "$PWD"
	set -o errexit
	dd=$1
	shift
	case $dd in
	'' | [!/]*)
		err_out "ERROR: destination directory '$dd' must begin with a /" ;;
	esac
	# commands to install by default
	test "$#" -eq 0 && set -- ksh shcomp  # pty suid_exec
	for f
	do	test -f "bin/$f" || err_out "Not found: $f" "Build first? Run $0 make"
	done
	# set install directories
	bindir=$dd/bin
	mandir=$dd/share/man
	man1dir=$mandir/man1
	# and off we go
	trace mkdir -p "$bindir" "$man1dir"
	for f
	do	# install executable
		trace cp "bin/$f" "$bindir/"
		# install manual
		case $f in
		ksh)	trace cp "$PACKAGEROOT/src/cmd/ksh93/sh.1" "$man1dir/ksh.1"
			;;
		*)	# AT&T --man, etc. is a glorified error message: writes to stderr and exits with status 2 :-/
			manfile=$man1dir/${f##*/}.1
			bin/ksh -c '"$@" 2>&1; exit 0' _ "bin/$f" --nroff >$manfile
			# ...so we cannot check for success; instead, check the result.
			if	grep -q '^.TH .* 1' "$manfile"
			then	printf "install: wrote '%s --nroff' output into %s\n" "bin/$f" "$manfile"
			else	rm "$manfile"
			fi
			;;
		esac
	done
}

# check for native ASCII 0:yes 1:no

__isascii__=

isascii()
{
	case $__isascii__ in
	'')	case $(echo A | od -o | sed -e 's/[ 	]*$//' -e '/[ 	]/!d' -e 's/.*[ 	]//') in
		005101|040412)	__isascii__=0 ;;
		*)		__isascii__=1 ;;
		esac
	esac
	return $__isascii__
}

error_status=0

case $action in

clean|clobber)
	cd "$PACKAGEROOT" || exit
	note "cleaning up flat view"
	# clean up all links with arch dir except bin/package
	$exec find "arch/$HOSTTYPE" -path "arch/$HOSTTYPE/bin/package" -o -type f -exec "$SHELL" -c '
		first=y
		for h					# loop through the PPs
		do	case $first in
			y)	set --			# clear PPs ("for" uses a copy)
				first=n ;;
			esac
			p=${h#"arch/$HOSTTYPE/"}	# get flat view path
			if	test "$p" -ef "$h"	# is it part of the flat view?
			then	set -- "$@" "$p"	# add to new PPs
			fi
		done
		exec rm -f -- "$@"			# rm all at once: fast
	' "$0" {} +
	case $flat in
	0)	note "deleting arch/$HOSTTYPE"
		$exec rm -rf arch/$HOSTTYPE
		;;
	esac
	note "removing empty directories"
	$exec find . -depth -type d -exec rmdir {} + 2>/dev/null
	;;

export)	case $INSTALLROOT in
	$PACKAGEROOT)
		INSTALLROOT=$INSTALLROOT/arch/$HOSTTYPE
		;;
	esac
	case $only in
	0)	v='$i=' ;;
	*)	v= ;;
	esac
	set '' $target $package
	case $# in
	1)	set '' $env ;;
	esac
	while	:
	do	case $# in
		1)	break ;;
		esac
		shift
		i=$1
		eval echo ${v}'$'${i}
	done
	;;

install)cd $PACKAGEROOT
	# TODO: breaks on directories with spaces or glob characters; make arguments handling robust
	test -n "$args" || err_out "Usage: $0 install ROOTDIR [ COMMANDNAME ... ]"
	capture do_install $args
	;;

make|view)
	cd $PACKAGEROOT

	# check for some required commands

	must="$AR"
	warn="$NM"
	test="$must $warn"
	have=
	IFS=:
	set /$IFS$PATH
	IFS=$ifs
	shift
	for t in $test
	do	if	executable $t
		then	have="$have $t"
		fi
	done
	for d
	do	for t in $test
		do	case " $have " in
			*" $t "*)
				;;
			*)	if	executable $d/$t
				then	have="$have $t"
				fi
				;;
			esac
		done
	done
	for t in $test
	do	case " $have " in
		*" $t "*)
			;;
		*)	case " $must " in
			*" $t "*)
				err_out "$t: not found -- must be on PATH to $action"
				;;
			*)	note "warning: $t: not found -- some $action actions may fail"
				;;
			esac
			;;
		esac
	done

	# verify the top view

	if	test ! -d $PACKAGEROOT/src
	then	note "no source packages to make"
		exit 0
	elif	test ! -d $INSTALLROOT/src
	then	note "initialize the $INSTALLROOT view"
	fi
	for i in arch arch/$HOSTTYPE
	do	test -d $PACKAGEROOT/$i || $exec mkdir $PACKAGEROOT/$i || exit
	done
	for i in bin bin/ok bin/ok/lib fun include lib lib/package lib/package/gen src man man/man1 man/man3 man/man8
	do	test -d $INSTALLROOT/$i || $exec mkdir $INSTALLROOT/$i || exit
	done
	make_recurse src
	o= k=
	for i in $makefiles
	do	case $o in
		?*)	o="$o -o" k="$k|" ;;
		esac
		o="$o -name $i"
		k="$k$i"
	done
	o="( $o ) -print"
	for d in $src
	do	i=src/$d
		if	test -d $i
		then	test -d $INSTALLROOT/$i || $exec mkdir $INSTALLROOT/$i || exit
			make_recurse $i
			for j in $(cd $i; find . $o 2>/dev/null | sed -e 's,^\./,,' -e '/\//!d' -e 's,/[^/]*$,,' | sort -u)
			do	case $j in
				$k|$MAKESKIP) continue ;;
				esac
				test -d $INSTALLROOT/$i/$j ||
				$exec mkdir -p $INSTALLROOT/$i/$j || exit
			done
		fi
	done

	# check $CC and { ar cc ld ldd } intercepts

	h="${HOSTTYPE} ${HOSTTYPE}.*"
	case $HOSTTYPE in
	*.*)	t=$(echo $HOSTTYPE | sed 's/[.][^.]*//')
		h="$h $t"
		;;
	*)	t=$HOSTTYPE
		;;
	esac
	case $t in
	*[0123456789])
		t=$(echo $t | sed 's/[0123456789]*$//')
		h="$h $t"
		;;
	esac
	case $CC in
	cc)	c=cc
		b=$INSTALLROOT/bin/$c
		t=$INSTALLROOT/lib/package/gen/$c.tim
		intercept=0
		for k in $h
		do	for s in $INITROOT/$c.$k
			do	test -x "$s" || continue
				if	cmp -s "$s" "$b" >/dev/null 2>&1
				then	intercept=1
					break 2
				fi
				case $(ls -t "$t" "$b" "$s" 2>/dev/null) in
				$t*)	;;
				$b*)	cc=$b
					;;
				$s*)	$exec cd $INSTALLROOT/lib/package/gen
					tmp=pkg$$
					$exec eval "echo 'int main(){return 0;}' > $tmp.c"
					if	$exec $s -o $tmp.exe $tmp.c >/dev/null 2>&1 &&
						test -x $tmp.exe
					then	case $HOSTTYPE in
						*.mips*)$s -version >/dev/null 2>&1 || s= ;;
						esac
						case $s in
						?*)	$exec sed "s/^HOSTTYPE=.*/HOSTTYPE=$HOSTTYPE/" < "$s" > "$b" || exit
							$exec chmod +x "$b" || exit
							cc=$b
							intercept=1
							note "update $b"
							;;
						esac
					fi
					$exec rm -f $tmp.*
					$exec touch "$t"
					cd $PACKAGEROOT
					;;
				esac
				break 2
			done
		done
		case $intercept in
		1)	c=ld
			b=$INSTALLROOT/bin/$c
			for k in $h
			do	for s in $INITROOT/$c.$k
				do	test -x "$s" || continue
					case $(ls -t "$b" "$s" 2>/dev/null) in
					$b*)	;;
					$s*)	$exec cp "$s" "$b"
						note "update $b"
						;;
					esac
				done
			done
			;;
		esac
		;;
	esac
	c=ldd
	b=$INSTALLROOT/bin/$c
	for t in $h
	do	s=$INITROOT/$c.$t
		test -x "$s" || continue
		onpath $c ||
		case $(ls -t "$b" "$s" 2>/dev/null) in
		$b*)	;;
		$s*)	$exec cp "$s" "$b"
			note "update $b"
			;;
		esac
	done
	c=ar
	b=$INSTALLROOT/bin/$c
	for t in $h
	do	s=$INITROOT/$c.$t
		test -x "$s" || continue
		case $(ls -t "$b" "$s" 2>/dev/null) in
		$b*)	;;
		$s*)	$exec cp "$s" "$b"
			note "update $b"
			;;
		esac
	done
	case $cc in
	/*)	;;
	*)	err_out "$CC: not found -- set CC=C-compiler"
		;;
	esac
	case $exec in
	'')	cd $INSTALLROOT/lib/package/gen
		tmp=pkg$$
		echo 'int main(){return 0;}' > $tmp.c
		if	$CC -o $tmp.exe $tmp.c > /dev/null 2> $tmp.err &&
			test -x $tmp.exe
		then	: ok
		else	note "$CC: failed to compile this program:"
			cat $tmp.c >&2
			if	test -s $tmp.err
			then	cat $tmp.err >&2
			else	note "$CC: not a C compiler"
			fi
			rm -f $tmp.*
			exit 1
		fi
		rm -f $tmp.*
		cd $PACKAGEROOT
		;;
	esac

	# remember the default $CC

	case $CC in
	cc)	;;
	*)	if	test -x $INSTALLROOT/bin/cc
		then	case $(sed 1q $INSTALLROOT/bin/cc) in
			": $CC :")
				CC=cc
				export CC
				;;
			*)	assign="$assign CC=\"\$CC\""
				;;
			esac
		else	case $CROSS in
			1)	assign="$assign CC=\"\$CC\""
				;;
			*)	case $exec in
				'')	{
					echo ": $CC :"
					echo "$CC \"\$@\""
					} > $INSTALLROOT/bin/cc
					chmod +x $INSTALLROOT/bin/cc
					;;
				*)	note "generate a $INSTALLROOT/bin/cc wrapper for $CC"
					;;
				esac
				CC=cc
				export CC
				;;
			esac
		fi
		;;
	esac

	# no $INITROOT means INIT already installed elsewhere

	if	test -d $INITROOT
	then
		# update probe scripts

		for i in lib/probe lib/probe/C lib/probe/C/make
		do	test -d $INSTALLROOT/$i || $exec mkdir $INSTALLROOT/$i || exit
		done
		i=$INSTALLROOT/lib/probe/C/make/probe
		j=$INITROOT/C+probe
		k=$INITROOT/make.probe
		case $(ls -t $i $j $k 2>/dev/null) in
		$i*)	;;
		*)	if	test -f "$j" && test -f "$k"
			then	note "update $i"
				shellmagic
				case $exec in
				'')	{
					case $SHELLMAGIC in
					?*)	echo "$SHELLMAGIC" ;;
					esac
					cat $j $k
					} > $i || exit
					;;
				*)	echo "{
echo $SHELLMAGIC
cat $j $k
} > $i"
					;;
				esac
				$exec chmod +x $i || exit
			fi
			;;
		esac
	fi

	# initialize mamake

	checkaout mamake || exit

	case $action in
	view)	exit 0 ;;
	esac

	# check against previous compiler and flags

	err=
	for	var in CC CCFLAGS CCLDFLAGS LDFLAGS
	do	store=$INSTALLROOT/lib/package/gen/$var
		eval "new=\$$var"
		if	test -f $store
		then	old=$(cat $store)
			case $old in
			"$new")	;;
			*)	case $old in
				'')	old="(none)" ;;
				*)	old="'$old'" ;;
				esac
				case $new in
				'')	new="(none)" ;;
				*)	new="'$new'" ;;
				esac
				note "$var changed from $old to $new"
				err=y ;;
			esac
		else	test -d $INSTALLROOT/lib/package/gen && case $new in
			'')	;;
			*)	echo "$new" ;;
			esac > $store
		fi
	done
	case $err,${FORCE_FLAGS+f} in
	y,)	err_out "This would likely break the build. Restore the flag(s)," \
			"or delete the build directory and rebuild from scratch."
		;;
	esac
	unset err var store old new

	# all work under $INSTALLROOT/src

	$make cd $INSTALLROOT/src

	# record the build host name

	case $exec in
	'')	hostinfo name
		echo "$_hostinfo_" | sed 's,\..*,,' > $PACKAGEBIN/gen/host
		;;
	esac

	# separate flags from target list

	case $target in
	*-*)	a=
		for t in $target
		do	case $t in
			-*)	makeflags="$makeflags $t"
				;;
			*)	a="$a $t"
				;;
			esac
		done
		target=$a
		;;
	esac

	# mamprobe data should have been generated by this point

	case $exec in
	'')	if	test ! -f "$INSTALLROOT/bin/.paths" || test -w "$INSTALLROOT/bin/.paths"
		then	N='
'
			b= f= h= n= p= u= B= L=
			if	test -f $INSTALLROOT/bin/.paths
			then	exec < $INSTALLROOT/bin/.paths
				while	read x
				do	case $x in
					'#'?*)		case $h in
							'')	h=$x ;;
							esac
							;;
					*BUILTIN_LIB=*)	b=$x
							;;
					*FPATH=*)	f=$x
							;;
					*PLUGIN_LIB=*)	p=$x
							;;
					*)		case $u in
							?*)	u=$u$N ;;
							esac
							u=$u$x
							;;
					esac
				done
			fi
			ifs=$IFS
			m=
			case $p in
			?*)	b=
				;;
			esac
			case $b in
			?*)	IFS='='
				set $b
				IFS=$ifs
				shift
				p="PLUGIN_LIB=$*"
				case $b in
				[Nn][Oo]*)	p=no$p ;;
				esac
				m=1
				;;
			esac
			case $f in
			'')	f="FPATH=../fun"
				m=1
				;;
			esac
			case $h in
			'')	h='# use { no NO } prefix to permanently disable #' ;;
			esac
			case $p in
			'')	p="PLUGIN_LIB=cmd"
				if	grep '^setv mam_cc_DIALECT .* EXPORT=[AD]LL' $INSTALLROOT/lib/probe/C/mam/* >/dev/null 2>&1
				then	p=no$p
				fi
				m=1
				;;
			esac
			case $m in
			1)	case $u in
				?*)	u=$N$u ;;
				esac
				echo "$h$N$p$N$f$N$u" > $INSTALLROOT/bin/.paths
				;;
			esac
		fi
		;;
	esac

	# run from separate copies since ksh may be rebuilt

	case $EXECROOT in
	$INSTALLROOT)
		$make cd $INSTALLROOT/bin
		if	executable /bin/cp
		then	cp=/bin/cp
		else	cp=cp
		fi
		if	executable /bin/mv
		then	mv=/bin/mv
		else	mv=mv
		fi
		if	executable /bin/rm
		then	rm=/bin/rm
		else	rm=rm
		fi
		for i in \
			ksh tee cp ln mv rm \
			*ast*.dll *cmd*.dll *dll*.dll *shell*.dll
		do	executable $i && {
				cmp -s $i ok/$i 2>/dev/null || {
					test -f ok/$i &&
					$exec $rm ok/$i </dev/null
					test -f ok/$i &&
					$exec $mv ok/$i ok/$i.old </dev/null
					test -f ok/$i &&
					case $exec:$i in
					:ksh)
						err_out "ok/$i: cannot update [may be in use by a running process] remove manually and try again"
						;;
					esac
					$exec $cp $i ok/$i
				}
			}
		done
		if	test "$KEEP_SHELL" -eq 0 && executable ok/ksh
		then	SHELL=$INSTALLROOT/bin/ok/ksh
			export SHELL
		fi
		case :$PATH: in
		*:$INSTALLROOT/bin/ok:*)
			;;
		*)	PATH=$INSTALLROOT/bin/ok:$PATH
			export PATH
			;;
		esac
		$make cd $INSTALLROOT/src
		;;
	esac

	# build with mamake

	note "make with mamake"
	case $target in
	'')	target="install" ;;
	esac
	eval capture mamake \$makeflags \$noexec \$target $assign

	case $HOSTTYPE in
	darwin.*)
		# clean up macOS .dSYM bundles belonging to deleted temps
		cd "$PACKAGEROOT" || exit
		$exec find "arch/$HOSTTYPE" -type d -name '*.dSYM' -exec "$SHELL" -c '
			first=y
			for d					# loop through the PPs
			do	case $first in
				y)	set --			# clear PPs ("for" uses a copy)
					first=n ;;
				esac
				e=${d%.dSYM}			# get exe name
				if	! test -f "$e"		# nonexistent?
				then	set -- "$@" "$d"	# add to new PPs
				fi
			done
			exec rm -rf -- "$@"			# rm all at once: fast
		' "$0" {} +
		;;
	esac

	if	test -d "$PACKAGEROOT/lib/package/gen"
	then	a='updating'
		flat=1
	else	a='creating'
	fi
	case $flat in
	1)	note "$a flat view"
		cd "$PACKAGEROOT" || exit
		$exec find "arch/$HOSTTYPE" -type f ! -name '*.old' -exec "$SHELL" -c '
			for h
			do	p=${h#"arch/$HOSTTYPE/"}
				test "$h" -ef "$p" && continue	# already created
				d=${p%/*}
				test -d "$d" || mkdir -p "$d" || exit
				ln -f "$h" "$p" 2>/dev/null || ln -sf "$INSTALLROOT/$p" "$p" || exit
			done
		' "$0" {} +
		;;
	esac
	;;

results)set '' $target
	shift
	def=make
	dir=$PACKAGEBIN/gen
	case $verbose in
	0)	filter=yes ;;
	*)	filter=cat ;;
	esac
	path=0
	suf=out
	on=
	while	:
	do	case $# in
		0)	break ;;
		esac
		case $1 in
		--)	shift
			break
			;;
		error*|fail*)
			filter=errors
			;;
		make|test|view)
			def=$1
			;;
		old)	suf=old
			;;
		on)	case $# in
			1)	err_out "$action: $1: host pattern argument expected"
				;;
			esac
			shift
			case $on in
			?*)	on="$on|" ;;
			esac
			on="$on$1"
			;;
		path)	path=1
			;;
		test)	def=test
			;;
		*)	break
			;;
		esac
		shift
	done
	case $dir in
	*/admin)case $on in
		'')	on="*" ;;
		*)	on="@($on)" ;;
		esac
		def=$def.log/$on
		;;
	esac
	case $# in
	0)	set "$def" ;;
	esac
	m=
	t=
	for i
	do	k=0
		eval set '""' $i - $i.$suf - $dir/$i - $dir/$i.$suf -
		shift
		for j
		do	case $j in
			-)	case $k in
				1)	continue 2 ;;
				esac
				;;
			*)	if	test -f $j
				then	k=1
					case /$j in
					*/test.*)	t="$t $j" ;;
					*)		m="$m $j" ;;
					esac
				fi
				;;
			esac
		done
		err_out "$i action output not found"
	done
	sep=
	case $t in
	?*)	case $path in
		0)	for j in $t
			do	echo "$sep==> $j <=="
				sep=$nl
				case $filter in
				cat)	$exec cat $j
					;;
				errors)	$exec grep -E -i '\*\*\*|FAIL[ES]|^TEST.* [123456789][0123456789]* error|core.*dump' $j | sed -e '/^TEST.\//s,/[^ ]*/,,'
					;;
				*)	$exec grep -E -i '^TEST|FAIL' $j
					;;
				esac
			done
			;;
		1)	echo $t
			;;
		esac
		;;
	esac
	case $m in
	?*)	case $path in
		0)	case $filter in
			cat)	cat $m
				;;
			*)	if	test -f $HOME/.pkgresults
				then	i=$(cat $HOME/.pkgresults)
					case $i in
					'|'*)	;;
					*)	i="|$i" ;;
					esac
				else	i=
				fi
				for j in $m
				do	echo "$sep==> $j <=="
					sep=$nl
					case $filter in
					errors)	$exeg grep -E '^pax:|\*\*\*' $j
						;;
					*)	$exec grep -E -iv '^($||[\+\[]|cc[^-:]|kill |make.*(file system time|has been replaced)|so|[0123456789]+ error|uncrate |[0123456789]+ block|ar: creat|iffe: test: |conf: (check|generate|test)|[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_][abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789]*=|gsf@research|ar:.*warning|cpio:|ld:.*(duplicate symbol|to obtain more information)|[0123456789]*$|(checking|creating|touch) [/abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789])| obsolete predefined symbol | is (almost always misused|dangerous|deprecated|not implemented)| trigraph| assigned to | cast .* different size| integer overflow .*<<| optimization may be attained | passed as |::__builtin|^creating.*\.a$|warning.*not optimized|exceeds size thresh|ld:.*preempts|is unchanged|with value >=|(-l|lib)\*|/(ast|sys)/(dir|limits|param|stropts)\.h.*redefined|usage|base registers|`\.\.\.` obsolete'"$i" $j |
						$exec grep :
						;;
					esac
				done
				;;
			esac
			;;
		1)	echo $m
			;;
		esac
	esac
	;;

test)	# run all available default regression tests, using our newly compiled shell unless overridden

	cd "$INSTALLROOT" || err_out "run '$0 make' first"
	if	test "$KEEP_SHELL" -lt 2
	then	executable bin/ksh || err_out "build ksh first, or supply a SHELL=/path/to/ksh argument"
		SHELL=$INSTALLROOT/bin/ksh
	fi
	export SHELL
	set -f
	set -- ${args:-src}
	cd "$1" || exit
	capture mamake test
	;;

use)	# finalize the environment

	if	test "$KEEP_SHELL" -lt 2 && executable "$INSTALLROOT/bin/ksh"
	then	SHELL=$INSTALLROOT/bin/ksh
	fi
	x=:..
	for d in $( cd $PACKAGEROOT; ls src/*/Mamfile 2>/dev/null | sed 's,/[^/]*$,,' | sort -u )
	do	x=$x:$INSTALLROOT/$d
	done
	x=$x:$INSTALLROOT
	case $CDPATH: in
	$x:*)	;;
	*)	CDPATH=$x:$CDPATH
		$show CDPATH=$CDPATH
		$show export CDPATH
		export CDPATH
		;;
	esac
	eval PACKAGE_USE=$package_use
	export PACKAGE_USE
	unset LC_ALL  # respect the user's locale again; avoids multibyte corruption

	# run the command

	case $run in
	'')	note	"You are now entering a new environment set up to use the package in:" \
			"    $INSTALLROOT" \
			"You're using the shell:" \
			"    $SHELL" \
			"Type 'exit' to leave and go back to normal."
		case $show in
		':')	$exec exec $SHELL ;;
		esac
		;;
	*)	$exec exec $SHELL -c "$run"
		;;
	esac
	;;

*)	err_out "$action: internal error"
	;;

esac

exit "$error_status"

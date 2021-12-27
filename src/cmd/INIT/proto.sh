########################################################################
#                                                                      #
#              This file is part of the ksh 93u+m package              #
#             Copyright (c) 2021 Contributors to ksh 93u+m             #
#                    <https://github.com/ksh93/ksh>                    #
#                      and is licensed under the                       #
#                 Eclipse Public License, Version 1.0                  #
#                                                                      #
#                A copy of the License is available at                 #
#          http://www.eclipse.org/org/documents/epl-v10.html           #
#         (with md5 checksum b35adb5213ca9657e911e9befb180842)         #
#                                                                      #
#                  Martijn Dekker <martijn@inlv.org>                   #
#                                                                      #
########################################################################

# proto(1) has been removed. This is a backwards compatibility stub that allows
# compiling older AST code (with Mamfiles containing proto commands) using the
# current INIT system. This stub ignores all options, then invokes 'cat'.

while	getopts ':dfhinprstvzPSC:e:l:o:L:' opt
do	case $opt in
	\?)	case $OPTARG in
		+)	;;
		*)	echo "proto: $OPTARG: unknown option"
			echo 'Usage: proto [-dfhinprstvzP+S] [-C directory] [-e package] [-l file]'
			echo '             [-o "name='\''value'\'' ..."] [-L file] file ...'
			exit 2
			;;
		esac >&2
		;;
	esac
done
shift $((OPTIND - 1))
exec cat -- "$@"

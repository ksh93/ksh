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

# proto(1) has been removed. This is a backward compatibility stub that allows
# compiling older AST code (with Mamfiles containing proto commands) using the
# current INIT system. This stub ignores all options, then invokes 'cat'.

usage()
{
	echo 'Usage: proto [-dfhinprstvzP+S] [-C directory] [-e package] [-l file]'
	echo '             [-o "name='\''value'\'' ..."] [-L file] file ...'
	exit 2
} >&2

while	getopts ':dfhinprstvzPSC:e:l:o:L:' opt
do	case $opt in
	:)	usage
		;;
	\?)	case $OPTARG in
		+)	;;
		*)	echo "proto: $OPTARG: unknown option" >&2
			usage
			;;
		esac
		;;
	esac
done
shift $((OPTIND - 1))
exec cat -- "$@"

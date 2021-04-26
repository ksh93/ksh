########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1994-2011 AT&T Intellectual Property          #
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
#                 Glenn Fowler <gsf@research.att.com>                  #
#                                                                      #
########################################################################
# non-ksh stub for the nmake silent prefix
# @(#)silent (AT&T Research) 1992-08-11

(command set -o posix) 2>/dev/null && set -o posix
modern_export=`v=; export v=ok 2>/dev/null; echo "$v"`

while	:
do	case $# in
	0)	exit 0 ;;
	esac
	case $1 in
	*=*)	case $modern_export in
		ok)	export "$1" ;;
		*)	`echo $1 | sed "s/\\([^=]*\\)=\\(.*\\)/eval \\1='\\2'; export \\1/"` ;;
		esac
		shift
		;;
	*)	break
		;;
	esac
done
"$@"

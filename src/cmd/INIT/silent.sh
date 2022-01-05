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
# @(#)silent (ksh 93u+m) 2021-12-31

while	:
do	case $# in
	0)	exit 0 ;;
	esac
	case $1 in
	*=*)	export "$1"
		shift
		;;
	*)	break
		;;
	esac
done
"$@"

########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1985-2011 AT&T Intellectual Property          #
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
#                  David Korn <dgk@research.att.com>                   #
#                   Phong Vo <kpv@research.att.com>                    #
#                                                                      #
########################################################################
: generate "<sys/param.h> + <sys/types.h> + <sys/stat.h>" include sequence

case $ZSH_VERSION in
?*)	emulate ksh ;;
*)	(command set -o posix) 2>/dev/null && set -o posix ;;
esac

case $# in
0)	;;
*)	eval $1
	shift
	;;
esac
for i in "#include <sys/param.h>" "#include <sys/param.h>
#ifndef S_IFDIR
#include <sys/stat.h>
#endif" "#include <sys/param.h>
#ifndef S_IFDIR
#include <sys/types.h>
#include <sys/stat.h>
#endif" "#ifndef S_IFDIR
#include <sys/types.h>
#include <sys/stat.h>
#endif"
do	echo "$i
struct stat V_stat_V;
F_stat_F() { V_stat_V.st_mode = 0; }" > $tmp.c
	if	$cc -c $tmp.c >/dev/null
	then	echo "$i"
		break
	fi
done

########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1985-2011 AT&T Intellectual Property          #
#          Copyright (c) 2020-2024 Contributors to ksh 93u+m           #
#                      and is licensed under the                       #
#                 Eclipse Public License, Version 2.0                  #
#                                                                      #
#                A copy of the License is available at                 #
#      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      #
#         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         #
#                                                                      #
#                 Glenn Fowler <gsf@research.att.com>                  #
#                  David Korn <dgk@research.att.com>                   #
#                   Phong Vo <kpv@research.att.com>                    #
#                  Martijn Dekker <martijn@inlv.org>                   #
#            Johnothan King <johnothanking@protonmail.com>             #
#                                                                      #
########################################################################
: generate sig features

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
echo "#include <signal.h>
int xxx;" > $tmp.c
$cc -c $tmp.c >/dev/null 2>$tmp.e
echo "#define sig_info	_sig_info_

#define kill	______kill
#define killpg	______killpg
#include <signal.h>
#undef	kill
#undef	killpg
#ifndef sigmask
#define sigmask(s)	(1<<((s)-1))
#endif"
echo "#include <signal.h>
#ifdef TYPE
typedef TYPE (*Sig_handler_t)(ARG);
#endif
Sig_handler_t f(void)
{
	Sig_handler_t	handler;
	handler = signal(1, SIG_IGN);
	return handler;
}" > $tmp.c
if	$cc -c $tmp.c >/dev/null
then	:
else	e=`wc -l $tmp.e`
	i1= j1=
	for i in void int
	do	for j in int,... ... int
		do	$cc -c -DTYPE=$i -DARG=$j $tmp.c >/dev/null 2>$tmp.e || continue
			case `wc -l $tmp.e` in
			$e)	i1= j1=; break 2 ;;
			esac
			case $i1 in
			"")	i1=$i j1=$j ;;
			esac
		done
	done
	case $i1 in
	?*)	i=$i1 j=$j1 ;;
	esac
	echo "typedef $i (*Sig_handler_t)($j);"
fi
echo '

#define Handler_t		Sig_handler_t

#define SIG_REG_PENDING		(-1)
#define SIG_REG_POP		0
#define SIG_REG_EXEC		00001
#define SIG_REG_PROC		00002
#define SIG_REG_TERM		00004
#define SIG_REG_ALL		00777
#define SIG_REG_SET		01000

typedef struct
{
	char**		name;
	char**		text;
	int		sigmax;
} Sig_info_t;

extern int		kill(pid_t, int);
extern int		killpg(pid_t, int);

extern Sig_info_t	sig_info;

#if _lib_sigflag && _npt_sigflag
extern int		sigflag(int, int, int);
#endif

#if !_lib_sigflag
extern int		sigflag(int, int, int);
#endif
extern int		sigcritical(int);
extern int		sigunblock(int);
extern Sig_handler_t	_ast_signal(int, Sig_handler_t);'

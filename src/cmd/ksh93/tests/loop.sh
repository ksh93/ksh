########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1982-2011 AT&T Intellectual Property          #
#          Copyright (c) 2020-2024 Contributors to ksh 93u+m           #
#                      and is licensed under the                       #
#                 Eclipse Public License, Version 2.0                  #
#                                                                      #
#                A copy of the License is available at                 #
#      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      #
#         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         #
#                                                                      #
#                  David Korn <dgk@research.att.com>                   #
#                  Martijn Dekker <martijn@inlv.org>                   #
#                                                                      #
########################################################################

. "${SHTESTS_COMMON:-${0%/*}/_common}"

PS3='ABC '

cat > $tmp/1 <<\!
1) foo
2) bar
3) bam
!

select i in foo bar bam
do	case $i in
	foo)	break;;
	*)	err_exit "select 1 not working"
		break;;
	esac
done 2> /dev/null <<!
1
!

unset i
select i in foo bar bam
do	case $i in
	foo)	err_exit "select foo not working" 2>&3
		break;;
	*)	if	[[ $REPLY != foo ]]
		then	err_exit "select REPLY not correct" 2>&3
		fi
		( set -u; : $i ) || err_exit "select: i not set to null" 2>&3
		break;;
	esac
done  3>&2 2> $tmp/2 <<!
foo
!

# ======
# break, continue

got=$(for i in a b c; do print -n $i; for j in 1 2 3; do print -n $j; break 2; done; done)
exp=a1
[[ $got == "$exp" ]] || err_exit "'break 2' broken (expected '$exp', got '$got')"

got=$(for i in a b c; do print -n $i; for j in 1 2 3; do print -n $j; continue 2; done; done)
exp=a1b1c1
[[ $got == "$exp" ]] || err_exit "'continue 2' broken (expected '$exp', got '$got')"

got=$(for i in a b c; do print -n $i; for j in 1 2 3; do print -n $j; for k in x y z; do print -n $k; break 3; done; done; done)
exp=a1x
[[ $got == "$exp" ]] || err_exit "'break 3' broken (expected '$exp', got '$got')"

got=$(for i in a b c; do print -n $i; for j in 1 2 3; do print -n $j; for k in x y z; do print -n $k; continue 3; done; done; done)
exp=a1xb1xc1x
[[ $got == "$exp" ]] || err_exit "'continue 3' broken (expected '$exp', got '$got')"

# ======
exit $((Errors<125?Errors:125))

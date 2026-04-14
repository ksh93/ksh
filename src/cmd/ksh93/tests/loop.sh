########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1982-2011 AT&T Intellectual Property          #
#          Copyright (c) 2020-2026 Contributors to ksh 93u+m           #
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

# ======
# select

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

# '! break' is not portable (as of 2026, it fails on mksh, dash, yash),
# but on ksh93 we should be able to rely on it.
# Also test 'break' with an argument, because on 93u+m/1.1, that's a
# different code path ('break' without arguments is optimised).
for i in 0 1
do	! break
done && err_exit "'! break' fails to set nonzero exit status"
for i in 0 1
do	! break 1
done && err_exit "'! break 1' fails to set nonzero exit status"
while :
do	! break
done && err_exit "'! break' fails to set nonzero exit status"
while :
do	! break 1
done && err_exit "'! break 1' fails to set nonzero exit status"

# Same with '! continue'.
for i in 0 1
do	case $i,$? in
	1,0)	err_exit "'! continue' fails to set nonzero exit status" ;;
	esac
	! continue
done
for i in 0 1
do	case $i,$? in
	1,0)	err_exit "'! continue 1' fails to set nonzero exit status" ;;
	esac
	! continue 1
done

# ======
# arithmetic for

exp=': `))'\'' unexpected'
for t in 'for((i=0,i<10,i++))' 'for(())' 'for((;))' 'for((0;))'
do	got=$(set +x; (ulimit -c 0; eval "$t; do :; done") 2>&1)
	[[ $got == *"$exp" ]] || err_exit "$t (expected match of *$(printf %q "$exp"), got $(printf %q "$got"))"
done

# ======
exit $((Errors<125?Errors:125))

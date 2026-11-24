########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1982-2011 AT&T Intellectual Property          #
#          Copyright (c) 2020-2022 Contributors to ksh 93u+m           #
#                      and is licensed under the                       #
#                 Eclipse Public License, Version 2.0                  #
#                                                                      #
#                A copy of the License is available at                 #
#      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      #
#         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         #
#                                                                      #
#                  David Korn <dgk@research.att.com>                   #
#                  Martijn Dekker <martijn@inlv.org>                   #
#            Johnothan King <johnothanking@protonmail.com>             #
#                                                                      #
########################################################################

. "${SHTESTS_COMMON:-${0%/*}/_common}"

binecho=$(PATH=${DEFPATH:-$(getconf PATH)}; whence -p echo)
bintrue=$(PATH=${DEFPATH:-$(getconf PATH)}; whence -p true)

# test restricted shell
pwd=$PWD
case $SHELL in
/*)	;;
*/*)	SHELL=$pwd/$SHELL;;
*)	SHELL=$(whence "$SHELL");;
esac
function check_restricted
{
	rm -f out
	LC_MESSAGES=C rksh -c "$@" 2> out > /dev/null
	grep restricted out  > /dev/null 2>&1
}

ln -s $SHELL rksh
PATH=$PWD:$PATH
rksh -c  '[[ -o restricted ]]' || err_exit 'restricted option not set'
[[ $(rksh -c 'print hello') == hello ]] || err_exit 'unable to run print'
check_restricted "$binecho" || err_exit "$binecho not restricted"
check_restricted ./echo || err_exit './echo not restricted'
check_restricted 'SHELL=ksh' || err_exit 'SHELL assignment not restricted'
check_restricted 'PATH=/bin' || err_exit 'PATH assignment not restricted'
check_restricted 'FPATH=/bin' || err_exit 'FPATH assignment not restricted'
check_restricted 'ENV=/bin' || err_exit 'ENV assignment not restricted'
check_restricted 'print > file' || err_exit '> file not restricted'
> empty
check_restricted 'print <> empty' || err_exit '<> file not restricted'
print 'echo hello' > script
chmod +x ./script
! check_restricted script ||  err_exit 'script without builtins should run in restricted mode'
check_restricted ./script ||  err_exit 'script with / in name should not run in restricted mode'
printf '%q hello\n' "$binecho" > script
! check_restricted script ||  err_exit 'script with pathnames should run in restricted mode'
print 'echo hello> file' > script
! check_restricted script ||  err_exit 'script with output redirection should run in restricted mode'
print 'PATH=/bin' > script
! check_restricted script ||  err_exit 'script with PATH assignment should run in restricted mode'
cat > script <<!
#! $SHELL
print hello
!
! check_restricted 'script;:' ||  err_exit 'script with #! pathname should run in restricted mode'
! check_restricted 'script' ||  err_exit 'script with #! pathname should run in restricted mode even if last command in script'
for i in PATH ENV FPATH
do	check_restricted  "function foo { typeset $i=foobar;};foo" || err_exit "$i can be changed in function by using typeset"
done

# ======
# The restricted option cannot be unset
check_restricted 'set +r' || err_exit "'set +r' unsets the restricted option"
check_restricted 'set +o restricted' || err_exit "'set +o restricted' unsets the restricted option"
check_restricted "set --default; $bintrue" || err_exit "'set --default' unsets the restricted option"

# ======
exit $((Errors<125?Errors:125))

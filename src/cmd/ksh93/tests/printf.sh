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

# PHI: Initial version.
#      The intent is to check Issue #182, but other printf issues can
#      be hosted here.


# ======
# printf %T date parsing: GNU-style "ago" date spec completely broken #182 
# https://github.com/ksh93/ksh/issues/182
#
# This test is a variation of the one provided by @martijn in #182.
#
# This test require a descent gnudate, on system with no gnudate, we skip
# the test with a warning (don't count as an error).
#

# ksh93 printf %T don't (can't?) handle TZ correctly when crossing DST during
# time traveling i.e "-xyz month"
export TZ=UTC

# We need gnudate
gd=$(command -v gnudate || command -v gdate || command -v date)
$gd --version | grep -q GNU ||
{ warning "GNU date(1) required -- tests skipped"; exit 0
}
$gd + # fault-in

# Stress 'ago' in printf %T
integer s0 s1
for u in years months weeks days hours minutes seconds
do
  for((i=24; i>=-24; i--))
  { for((;;))
    { printf -v t0  "%(%S)T" now
      printf -v ksh  "%(%Y-%m-%d %H:%M:%S)T"      "$i $u ago exact"
      gnu=$($gd       +"%Y-%m-%d %H:%M:%S" --date="$i $u ago" )
      printf -v t1  "%(%S)T" now
      ((t0==t1)) && break
    }
    [ ! "${ksh% @*}" = "${gnu% @*}" ] &&  err_exit "printf %T i=$i u=$u" \
	 "(expected $(printf %q "$gnu"), got $(printf %q "$ksh"))"
  }
done 

((t0!=t1)) &&
{ warning "Takes too long, run on faster HW, less loaded OS -- tests skipped"
  exit 0
}

# ======
exit $((Errors<125?Errors:125))

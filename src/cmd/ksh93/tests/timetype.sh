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

typeset -T Time_t=(
	integer .=-1
	_='%F+%H:%M'
	get()
	{
		if      (( _ < 0 ))
		then	.sh.value=${ printf "%(${_._})T" now ;}
		else	.sh.value=${ printf "%(${_._})T" "#$((_))" ;}
		fi
	}
	set()
	{
		.sh.value=${ printf "%(%#)T" "${.sh.value}";}
	}
)

d=$(printf "%(%F+%H:%M)T" now)
integer s=$(printf "%(%#)T" "$d")
Time_t t=$d
[[ $t == "$d" ]] || err_exit "printf %T != Time_t -- expected '$d', got '$t'"
(( t == s )) || err_exit "numeric Time_t failed -- expected '$s', got '$t'"
t._='%#'
[[ $t == $s ]] || err_exit "t._='%#' failed -- expected '$s', got '$t'"
unset t
Time_t tt=(yesterday today tomorrow)
tt[3]=2pm
[[ ${!tt[@]} == '0 1 2 3' ]] || err_exit "indexed array subscript names failed -- expected '0 1 2 3', got '${!tt[@]}'"
[[ ${tt[0]} == *+00:00 ]] || err_exit "tt[0] failed -- expected 00:00, got '${tt[0]##*+}'"
[[ ${tt[1]} == *+00:00 ]] || err_exit "tt[1] failed -- expected 00:00, got '${tt[1]##*+}'"
[[ ${tt[2]} == *+00:00 ]] || err_exit "tt[2] failed -- expected 00:00, got '${tt[2]##*+}'"
[[ ${tt[3]} == *+14:00 ]] || err_exit "tt[3] failed -- expected 14:00, got '${tt[3]##*+}'"
unset tt
Time_t tt=('2008-08-11+00:00:00,yesterday' '2008-08-11+00:00:00,today' '2008-08-11+00:00:00,tomorrow')
tt[3]=9am
tt[4]=5pm
(( (tt[1] - tt[0]) == 24*3600 )) || err_exit "today-yesterday='$((tt[1] - tt[0]))' != 1 day"
(( (tt[2] - tt[1]) == 24*3600 )) || err_exit "tomorrow-today='$((tt[2] - tt[1]))' != 1 day"
(( (tt[4] - tt[3]) ==  8*3600 )) || err_exit "9am..5pm='$((tt[4] - tt[3]))' != 8 hours"
unset tt
Time_t tt=([yesterday]='2008-08-11+00:00:00,yesterday' [today]='2008-08-11+00:00:00,today' [tomorrow]='2008-08-11+00:00:00,tomorrow')
tt[2pm]='2008-08-11+00:00:00,2pm'
[[ ${tt[yesterday]} == *+00:00 ]] || err_exit "tt[yesterday] failed -- expected 00:00, got '${tt[yesterday]##*+}'"
[[ ${tt[today]} == *+00:00 ]] || err_exit "tt[today] failed -- expected 00:00, got '${tt[today]##*+}'"
[[ ${tt[tomorrow]} == *+00:00 ]] || err_exit "tt[tomorrow] failed -- expected 00:00, got '${tt[tomorrow]##*+}'"
[[ ${tt[2pm]} == *+14:00 ]] || err_exit "tt[2pm] failed -- expected 14:00, got '${tt[2pm]##*+}'"
(( (tt[today] - tt[yesterday] ) == 24*3600 )) || err_exit "tt[today]-tt[yesterday] failed -- expected 24*3600, got $(((tt[today]-tt[yesterday])/3600.0))*3600"
(( (tt[tomorrow] - tt[today] ) == 24*3600 )) || err_exit "tt[tomorrow]-tt[today] failed -- expected 24*3600, got $(((tt[tomorrow]-tt[today])/3600.0))*3600"
(( (tt[2pm] - tt[today] ) == 14*3600 )) || err_exit "tt[2pm]-tt[today] failed -- expected 14*3600, got $(((tt[2pm]-tt[today])/3600.0))*3600"
unset tt

# ======
# Prior to 2026-03-01, the following script triggered a use after free in three places, crashing under ASan
# Source of reproducer: https://stackoverflow.com/a/78246894
got=$( { set +x; "$SHELL" -c '
# This will do a show and tell using the typeset -T feature
# of ksh
# Sat Mar 30 01:01:35 AM EDT 2024
#
typeset -T TheTime_T=(
    typeset -S skew=0
    function get {
        now=$( date +%s )
        (( .sh.value=now+skew ))
        (( skew+=1 ))
    }
)
typeset -T Upper_T=(
    TheTime_T now
    typeset one=11
    typeset two=2U
    typeset countU=0
    typeset start="Upper"
    function initialize {
        typeset -S countS=0  # static
        typeset    countI=0  # instance
        (( _.countU+=1 ))
        (( countS+=1 ))
        countI=$(( countI+1 ))
        echo "init of Upper: ${!_}  S=${countS} I=${countI} U=${_.countU}"
    }
    function setStart {
        echo "Upper:setStart ${_.now} $@"
    }
    function endStart {
        echo "Upper:endStart ${_.now} $@"
    }
)
typeset -T Middle_T=(
    Upper_T _
    typeset middleVal="middle value"
    typeset start=middle
    typeset two="middle"
    function initialize {
        echo "init of Middle: ${!_}"
        .sh.type.Upper_T.initialize ${!_}
    }
    function endStart {
        echo "Middle:endStart $@"
        _.two="midEnd"
    }
)
typeset -T Lower_T=(
    Middle_T _
    typeset one=1L
    typeset start="lower"	# CRASH
    function initialize {
        echo "init of Lower: ${!_}"
        .sh.type.Upper_T.initialize ${!_}
    }
    function endStart {
        echo "Lower:endStart $@"
        echo "Ending the start process in mv=${_.middleVal} t=${_.two} ${_.one}"
    }
)

Upper_T uu
uu.initialize toStart
uu.setStart hownow
uu.endStart then

Middle_T mm
mm.initialize inMiddle
mm.setStart middleStart
mm.endStart middleStartThen	# CRASH

Lower_T ll=(
    middleVal="lower val"
)

ll.initialize TowardsEnd
ll.setStart startingLower
ll.endStart endingLower		# CRASH
'; } 2>&1 )

exp='^init of Upper: uu  S=1 I=1 U=1
Upper:setStart [[:digit:]]+ hownow
Upper:endStart [[:digit:]]+ then
init of Middle: mm
init of Upper: Upper_T  S=2 I=1 U=1
Upper:setStart [[:digit:]]+ middleStart
Middle:endStart middleStartThen
init of Lower: ll
init of Upper: Upper_T  S=3 I=1 U=2
Upper:setStart [[:digit:]]+ startingLower
Lower:endStart endingLower
Ending the start process in mv=lower val t=middle 1L$'

[[ $got =~ $exp ]] || err_exit "TheTime_T test script (expected match of $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
exit $((Errors<125?Errors:125))

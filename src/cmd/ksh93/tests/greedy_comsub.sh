########################################################################
#                                                                      #
#              This file is part of the ksh 93u+m package              #
#          Copyright (c) 2022-2025 Contributors to ksh 93u+m           #
#                      and is licensed under the                       #
#                 Eclipse Public License, Version 2.0                  #
#                                                                      #
#                A copy of the License is available at                 #
#      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      #
#         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         #
#                                                                      #
#                      Phi <phi.debian@gmail.com>                      #
#                  Martijn Dekker <martijn@inlv.org>                   #
#                                                                      #
########################################################################


. "${SHTESTS_COMMON:-${0%/*}/_common}"


alias T='do_test "$LINENO"'

# ======
# Tests for greedy comsub, i.e ${ list } [;\n]}
#                                      |      |
#                                      |      +->   Reserved word (keyword)
#                                      +--------> ! Reserved word (keyword)
# https://github.com/ksh93/ksh/issues/691

function do_test # $LINENO "${ list ;}" "expect"
{ [ "$2" = "$3" ] ||
  \err_exit "$1" "expected '$3', got '$2'"
}


# ======

set -o | grep -q comsub_brace_greedy ||
{ warning \
  "This SHELL doesn't support 'set -o comsub_brace_greedy ';  skipping tests"
  alias T=:
}  
set -o comsub_brace_greedy

T "${ echo A ; }"                   "A"
T "${ echo A ;}"                    "A"
T "${ echo A;}"                     "A"
# ======
 
T "${ echo A ${ echo B ;} ;}"       "A B" # err_exit 
T "${ echo A ${ echo B ;};}"        "A B"
T "${ echo A ${ echo B;};}"         "A B"
T "${ echo A { ${ echo B ;} C } ;}" "A { B C }"
T "${ echo A { ${ echo B;} C } ;}"  "A { B C }"
T "${ echo A { ${ echo B;} C};}"    "A { B C}"
T "${ echo A ;} ${ echo B ;}"       "A B"
T "${ echo A ;} ${ echo B;}"        "A B"
T "${ echo A ;}${ echo B;}"         "AB"
T "${ echo A;}${ echo B;}"          "AB"

# @stephane-chazelas test cases
# https://github.com/ksh93/ksh/issues/691
T "${ echo {a.b}; }"                            "{a.b}"
T "${ echo '{a,b}c' ;}"                         "{a,b}c"
T "${ echo ${ echo {fd[0]}< /dev/null; } ;}"    ""

# Variation on @stephane-chazelas
a=1 a.c=1
T "${ echo ${a.c}; }"                           "1"
T "${ echo {a..c}; }"                           "a b c"
a=x b=z
T "${ echo {$a..$b}; }"                         "x y z"
T "${ echo {${a}..${b}}; }"                     "x y z"
a='x;' b='z;'
T "${ echo {${a//;}..${b//;}}; }"               "x y z"

 
# ======
exit $((Errors<125?Errors:125))

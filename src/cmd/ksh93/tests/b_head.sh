########################################################################
#                                                                      #
#               This software is part of the ast package               #
#           Copyright (c) 2019-2020 Contributors to ksh2020            #
#             Copyright (c) 2022 Contributors to ksh 93u+m             #
#                      and is licensed under the                       #
#                 Eclipse Public License, Version 2.0                  #
#                                                                      #
#                A copy of the License is available at                 #
#      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      #
#         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         #
#                                                                      #
#              Siteshwar Vashisht <svashisht@redhat.com>               #
#                 Kurtis Rader <krader@skepticism.us>                  #
#            Johnothan King <johnothanking@protonmail.com>             #
#                                                                      #
########################################################################

# Tests for `head` builtin

. "${SHTESTS_COMMON:-${0%/*}/_common}"
if ! builtin head 2> /dev/null; then
	warning 'Could not detect head builtin; skipping tests'
	exit 0
fi

cat > "$tmp/file1" <<EOF
This is line 1 in file1
This is line 2 in file1
This is line 3 in file1
This is line 4 in file1
This is line 5 in file1
This is line 6 in file1
This is line 7 in file1
This is line 8 in file1
This is line 9 in file1
This is line 10 in file1
This is line 11 in file1
This is line 12 in file1
EOF

cat > "$tmp/file2" <<EOF2
This is line 1 in file2
This is line 2 in file2
This is line 3 in file2
This is line 4 in file2
This is line 5 in file2
EOF2

# ======
# -*n*; i.e., an integer presented as a flag.
#
# The `awk` invocation is to strip whitespace around the output of `wc` since it might pad the
# value.
exp=11
got=$(head -11 < "$tmp/file1" | wc -l | awk '{ print $1 }')
[[ "$got" = "$exp" ]] || err_exit "'head -n' failed" "(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
#   -n, --lines=lines
#                   Copy lines lines from each file. The default value is 10.
got=$(head -n 3 "$tmp/file1")
exp=$'This is line 1 in file1\nThis is line 2 in file1\nThis is line 3 in file1'
[[ "$got" = "$exp" ]] || err_exit "'head -n' failed" "(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
#   -c, --bytes=chars
#                   Copy chars bytes from each file.
got=$(head -c 14 "$tmp/file1")
exp=$'This is line 1'
[[ "$got" = "$exp" ]] || err_exit "'head -c' failed" "(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
#   -q, --quiet|silent
#                   Never output filename headers.
got=$(head -q -n 3 "$tmp/file1" "$tmp/file2")
exp=$'This is line 1 in file1\nThis is line 2 in file1\nThis is line 3 in file1\nThis is line 1 in file2\nThis is line 2 in file2\nThis is line 3 in file2'
[[ "$got" = "$exp" ]] || err_exit "'head -q' failed" "(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
#   -s, --skip=char Skip char characters or lines from each file before copying.
got=$(head -s 5 -c 18 "$tmp/file1")
exp=$'is line 1 in file1'
[[ "$got" = "$exp" ]] || err_exit "'head -s' failed" "(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
#   -v, --verbose   Always output filename headers.
got=$(head -v -n 3 "$tmp/file1")
exp=$'file1 <==\nThis is line 1 in file1\nThis is line 2 in file1\nThis is line 3 in file1'
[[ "$got" =~ "$exp" ]] || err_exit "'head -v' failed" "(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
exit $((Errors<125?Errors:125))

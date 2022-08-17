# Tests for `wc` builtin
#   wc - print the number of bytes, words, and lines in files

. "${SHTESTS_COMMON:-${0%/*}/_common}"
if ! builtin wc 2> /dev/null; then
	warning 'Could not detect wc builtin; skipping tests'
	exit 0
fi


cat > "$tmp/file1" <<EOF
This is line 1 in file1
This is line 2 in file1
This is line 3 in file1
This is line 4 in file1
This is line 5 in file1
EOF

cat > "$tmp/file2" <<EOF
This is line 1 in file2
This is line 2 in file2
This is line 3 in file2
This is line 4 in file2
This is line 5 in file2
This is the longest line in file2
神
EOF
cp "$tmp/file2" "/tmp"

# ==========
#   -l, --lines     List the line counts.
got=$(wc -l "$tmp/file1")
exp=$"       5 $tmp/file1"
[[ "$got" = "$exp" ]] || err_exit "'wc -l' failed" "(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ==========
#   -w, --words     List the word counts.
got=$(wc -w "$tmp/file1")
exp=$"      30 $tmp/file1"
[[ "$got" = "$exp" ]] || err_exit "'wc -w' failed" "(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ==========
#   -c, --bytes|chars
#                   List the byte counts.
got=$(wc -c "$tmp/file1")
exp=$"     120 $tmp/file1"
[[ "$got" = "$exp" ]] || err_exit "'wc -c' failed" "(expected $(printf %q "$exp"), got $(printf %q "$got"))"

if ((SHOPT_MULTIBYTE)) && [[ ${LC_ALL:-${LC_CTYPE:-${LANG:-}}} =~ [Uu][Tt][Ff]-?8 ]]; then
	# ==========
	#   -m|C, --multibyte-chars
	#                   List the character counts.
	got=$(wc -m "$tmp/file2")
	exp=$"     156 $tmp/file2"
	[[ "$got" = "$exp" ]] || err_exit "'wc -m' failed" "(expected $(printf %q "$exp"), got $(printf %q "$got"))"
	got=$(wc -C "$tmp/file2")
	exp=$"     156 $tmp/file2"
	[[ "$got" = "$exp" ]] || err_exit "'wc -C' failed" "(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# ==========
	#   -q, --quiet     Suppress invalid multibyte character warnings.
	got=$(wc -q -m "$tmp/file2")
	exp=$"     156 $tmp/file2"
	[[ "$got" = "$exp" ]] || err_exit "'wc -q -m' failed" "(expected $(printf %q "$exp"), got $(printf %q "$got"))"
	got=$(wc -q -C "$tmp/file2")
	exp=$"     156 $tmp/file2"
	[[ "$got" = "$exp" ]] || err_exit "'wc -q -C' failed" "(expected $(printf %q "$exp"), got $(printf %q "$got"))"
fi

# ==========
#   -L, --longest-line|max-line-length
#                   List the longest line length; the newline,if any, is not
#                   counted in the length.
got=$(wc -L "$tmp/file2")
exp=$"      33 $tmp/file2"
[[ "$got" = "$exp" ]] || err_exit "'wc -l' failed" "(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ==========
#   -N, --utf8      For UTF-8 locales --noutf8 disables UTF-8 optimzations and
#                   relies on the native mbtowc(3). On by default; -N means
#                   --noutf8.
got=$(wc -N "$tmp/file2")
exp="       7      38     158 $tmp/file2"
[[ "$got" = "$exp" ]] || err_exit "'wc -N' failed" "(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
exit $((Errors<125?Errors:125))

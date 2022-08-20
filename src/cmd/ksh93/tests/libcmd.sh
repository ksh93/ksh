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
#                  Martijn Dekker <martijn@inlv.org>                   #
#                                                                      #
########################################################################

# Tests for path-bound built-ins from src/lib/libcmd

. "${SHTESTS_COMMON:-${0%/*}/_common}"

# ======
# Tests for the cp builtin
if builtin cp 2> /dev/null; then
	# The cp builtin's -r/-R flag should not interfere with the -L, -P and -H flags
	echo 'test file' > "$tmp/cp_testfile"
	ln -s "$tmp/cp_testfile" "$tmp/symlink1"
	cp -r "$tmp/symlink1" "$tmp/symlink2"
	{ test -f "$tmp/symlink2" && test -L "$tmp/symlink2"; } || err_exit "default behavior of 'cp -r' follows symlinks"
	rm "$tmp/symlink2"
	cp -R "$tmp/symlink1" "$tmp/symlink2"
	{ test -f "$tmp/symlink2" && test -L "$tmp/symlink2"; } || err_exit "default behavior of 'cp -R' follows symlinks"
	rm "$tmp/symlink2"
	cp -Pr "$tmp/symlink1" "$tmp/symlink2"
	{ test -f "$tmp/symlink2" && test -L "$tmp/symlink2"; } || err_exit "'cp -Pr' follows symlinks"
	rm "$tmp/symlink2"
	cp -PR "$tmp/symlink1" "$tmp/symlink2"
	{ test -f "$tmp/symlink2" && test -L "$tmp/symlink2"; } || err_exit "'cp -PR' follows symlinks"
	rm "$tmp/symlink2"
	cp -rP "$tmp/symlink1" "$tmp/symlink2"
	{ test -f "$tmp/symlink2" && test -L "$tmp/symlink2"; } || err_exit "'cp -rP' follows symlinks"
	rm "$tmp/symlink2"
	cp -RP "$tmp/symlink1" "$tmp/symlink2"
	{ test -f "$tmp/symlink2" && test -L "$tmp/symlink2"; } || err_exit "'cp -RP' follows symlinks"
	rm "$tmp/symlink2"
	cp -Lr "$tmp/symlink1" "$tmp/testfile2"
	{ test -f "$tmp/testfile2" && ! test -L "$tmp/testfile2"; } || err_exit "'cp -Lr' doesn't follow symlinks"
	rm "$tmp/testfile2"
	cp -LR "$tmp/symlink1" "$tmp/testfile2"
	{ test -f "$tmp/testfile2" && ! test -L "$tmp/testfile2"; } || err_exit "'cp -LR' doesn't follow symlinks"
	rm "$tmp/testfile2"
	cp -rL "$tmp/symlink1" "$tmp/testfile2"
	{ test -f "$tmp/testfile2" && ! test -L "$tmp/testfile2"; } || err_exit "'cp -rL' doesn't follow symlinks"
	rm "$tmp/testfile2"
	cp -RL "$tmp/symlink1" "$tmp/testfile2"
	{ test -f "$tmp/testfile2" && ! test -L "$tmp/testfile2"; } || err_exit "'cp -RL' doesn't follow symlinks"
	mkdir -p "$tmp/cp_testdir/dir1"
	ln -s "$tmp/cp_testdir" "$tmp/testdir_symlink"
	ln -s "$tmp/testfile2" "$tmp/cp_testdir/testfile2_sym"
	cp -RH "$tmp/testdir_symlink" "$tmp/result"
	{ test -d "$tmp/result" && ! test -L "$tmp/result"; } || err_exit "'cp -RH' didn't follow the given symlink"
	{ test -f "$tmp/result/testfile2_sym" && test -L "$tmp/result/testfile2_sym"; } || err_exit "'cp -RH' follows symlinks not given on the command line"
	rm -r "$tmp/result"
	cp -rH "$tmp/testdir_symlink" "$tmp/result"
	{ test -d "$tmp/result" && ! test -L "$tmp/result"; } || err_exit "'cp -rH' didn't follow the given symlink"
	{ test -f "$tmp/result/testfile2_sym" && test -L "$tmp/result/testfile2_sym"; } || err_exit "'cp -rH' follows symlinks not given on the command line"
	rm -r "$tmp/result"
	cp -Hr "$tmp/testdir_symlink" "$tmp/result"
	{ test -d "$tmp/result" && ! test -L "$tmp/result"; } || err_exit "'cp -Hr' didn't follow the given symlink"
	{ test -f "$tmp/result/testfile2_sym" && test -L "$tmp/result/testfile2_sym"; } || err_exit "'cp -Hr' follows symlinks not given on the command line"
	rm -r "$tmp/result"
	cp -HR "$tmp/testdir_symlink" "$tmp/result"
	{ test -d "$tmp/result" && ! test -L "$tmp/result"; } || err_exit "'cp -HR' didn't follow the given symlink"
	{ test -f "$tmp/result/testfile2_sym" && test -L "$tmp/result/testfile2_sym"; } || err_exit "'cp -HR' follows symlinks not given on the command line"
fi

# ======
# Tests for the head builtin
if builtin head 2> /dev/null; then
	cat > "$tmp/file1" <<-EOF
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

	cat > "$tmp/file2" <<-EOF2
	This is line 1 in file2
	This is line 2 in file2
	This is line 3 in file2
	This is line 4 in file2
	This is line 5 in file2
	EOF2

	# -*n*; i.e., an integer presented as a flag.
	#
	# The `awk` invocation is to strip whitespace around the output of `wc` since it might pad the
	# value.
	exp=11
	got=$(head -11 < "$tmp/file1" | wc -l | awk '{ print $1 }')
	[[ $got == "$exp" ]] || err_exit "'head -n' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#   -n, --lines=lines
	#                   Copy lines lines from each file. The default value is 10.
	got=$(head -n 3 "$tmp/file1")
	exp=$'This is line 1 in file1\nThis is line 2 in file1\nThis is line 3 in file1'
	[[ $got == "$exp" ]] || err_exit "'head -n' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#   -c, --bytes=chars
	#                   Copy chars bytes from each file.
	got=$(head -c 14 "$tmp/file1")
	exp=$'This is line 1'
	[[ $got == "$exp" ]] || err_exit "'head -c' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#   -q, --quiet|silent
	#                   Never output filename headers.
	got=$(head -q -n 3 "$tmp/file1" "$tmp/file2")
	exp=$'This is line 1 in file1\nThis is line 2 in file1\nThis is line 3 in file1\nThis is line 1 in file2\nThis is line 2 in file2\nThis is line 3 in file2'
	[[ $got == "$exp" ]] || err_exit "'head -q' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#   -s, --skip=char Skip char characters or lines from each file before copying.
	got=$(head -s 5 -c 18 "$tmp/file1")
	exp=$'is line 1 in file1'
	[[ $got == "$exp" ]] || err_exit "'head -s' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#   -v, --verbose   Always output filename headers.
	got=$(head -v -n 3 "$tmp/file1")
	exp=$'file1 <==\nThis is line 1 in file1\nThis is line 2 in file1\nThis is line 3 in file1'
	[[ $got =~ "$exp" ]] || err_exit "'head -v' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"
fi

# ======
# Tests for the wc builtin
#   wc - print the number of bytes, words, and lines in files
if builtin wc 2> /dev/null; then
	cat > "$tmp/file1" <<-EOF
	This is line 1 in file1
	This is line 2 in file1
	This is line 3 in file1
	This is line 4 in file1
	This is line 5 in file1
	EOF

	cat > "$tmp/file2" <<-EOF
	This is line 1 in file2
	This is line 2 in file2
	This is line 3 in file2
	This is line 4 in file2
	This is line 5 in file2
	This is the longest line in file2
	神
	EOF

	#   -l, --lines     List the line counts.
	got=$(wc -l "$tmp/file1")
	exp=$"       5 $tmp/file1"
	[[ $got == "$exp" ]] || err_exit "'wc -l' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#   -w, --words     List the word counts.
	got=$(wc -w "$tmp/file1")
	exp=$"      30 $tmp/file1"
	[[ $got == "$exp" ]] || err_exit "'wc -w' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#   -c, --bytes|chars
	#                   List the byte counts.
	got=$(wc -c "$tmp/file1")
	exp=$"     120 $tmp/file1"
	[[ $got == "$exp" ]] || err_exit "'wc -c' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	if ((SHOPT_MULTIBYTE)) && [[ ${LC_ALL:-${LC_CTYPE:-${LANG:-}}} =~ [Uu][Tt][Ff]-?8 ]]; then
		#   -m|C, --multibyte-chars
		#                   List the character counts.
		got=$(wc -m "$tmp/file2")
		exp=$"     156 $tmp/file2"
		[[ $got == "$exp" ]] || err_exit "'wc -m' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"
		got=$(wc -C "$tmp/file2")
		exp=$"     156 $tmp/file2"
		[[ $got == "$exp" ]] || err_exit "'wc -C' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

		#   -q, --quiet     Suppress invalid multibyte character warnings.
		got=$(wc -q -m "$tmp/file2")
		exp=$"     156 $tmp/file2"
		[[ $got == "$exp" ]] || err_exit "'wc -q -m' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"
		got=$(wc -q -C "$tmp/file2")
		exp=$"     156 $tmp/file2"
		[[ $got == "$exp" ]] || err_exit "'wc -q -C' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"
	fi

	#   -L, --longest-line|max-line-length
	#                   List the longest line length; the newline,if any, is not
	#                   counted in the length.
	got=$(wc -L "$tmp/file2")
	exp=$"      33 $tmp/file2"
	[[ $got == "$exp" ]] || err_exit "'wc -l' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#   -N, --utf8      For UTF-8 locales --noutf8 disables UTF-8 optimzations and
	#                   relies on the native mbtowc(3). On by default; -N means
	#                   --noutf8.
	got=$(wc -N "$tmp/file2")
	exp="       7      38     158 $tmp/file2"
	[[ $got == "$exp" ]] || err_exit "'wc -N' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"
fi

# ======
# The rm builtin's -d option should remove files and empty directories without
# removing non-empty directories (unless the -r option is also passed).
# https://www.austingroupbugs.net/view.php?id=802
if builtin rm 2> /dev/null; then
	echo foo > "$tmp/bar"
	mkdir "$tmp/emptydir"
	mkdir -p "$tmp/nonemptydir1/subfolder"
	mkdir "$tmp/nonemptydir2"
	echo dummyfile > "$tmp/nonemptydir2/shouldexist"

	# Tests for lone -d option
	got=$(rm -d "$tmp/emptydir" 2>&1) || err_exit 'rm builtin fails to remove empty directory with -d option' \
		"(got $(printf %q "$got"))"
	[[ -d $tmp/emptydir ]] && err_exit 'rm builtin fails to remove empty directory with -d option'
	got=$(rm -d $tmp/bar 2>&1) || err_exit 'rm builtin fails to remove files with -d option' \
		"(got $(printf %q "$got"))"
	[[ -f $tmp/bar ]] && err_exit 'rm builtin fails to remove files with -d option'
	rm -d "$tmp/nonemptydir1" 2> /dev/null
	[[ ! -d $tmp/nonemptydir1/subfolder ]] && err_exit 'rm builtin has unwanted recursion with -d option on folder containing folder'
	rm -d "$tmp/nonemptydir2" 2> /dev/null
	[[ ! -f $tmp/nonemptydir2/shouldexist ]] && err_exit 'rm builtin has unwanted recursion with -d option on folder containing file'

	# Recreate non-empty directories in case the above tests failed
	mkdir -p "$tmp/nonemptydir1/subfolder"
	mkdir -p "$tmp/nonemptydir2"
	echo dummyfile > "$tmp/nonemptydir2/shouldexist"

	# Tests combining -d with -r
	got=$(rm -rd "$tmp/nonemptydir1" 2>&1) \
		|| err_exit 'rm builtin fails to remove non-empty directory and subdirectory with -rd options' \
			"(got $(printf %q "$got"))"
	[[ -d $tmp/nonemptydir1/subfolder || -d $tmp/nonemptydir1 ]] \
		&& err_exit 'rm builtin fails to remove all folders with -rd options'
	got=$(rm -rd "$tmp/nonemptydir2" 2>&1) \
		|| err_exit 'rm builtin fails to remove non-empty directory and file with -rd options' \
			"(got $(printf %q "$got"))"
	[[ -f $tmp/nonemptydir2/shouldexist || -d $tmp/nonemptydir2 ]] \
		&& err_exit 'rm builtin fails to remove all folders and files with -rd options'

	# Repeat the above tests with -R instead of -r (because of possible optget bugs)
	mkdir -p "$tmp/nonemptydir1/subfolder"
	mkdir -p "$tmp/nonemptydir2"
	echo dummyfile > "$tmp/nonemptydir2/shouldexist"
	got=$(rm -Rd "$tmp/nonemptydir1" 2>&1) \
		|| err_exit 'rm builtin fails to remove non-empty directory and subdirectory with -Rd options' \
			"(got $(printf %q "$got"))"
	[[ -d $tmp/nonemptydir1/subfolder || -d $tmp/nonemptydir1 ]] \
		&& err_exit 'rm builtin fails to remove all folders with -Rd options'
	got=$(rm -Rd "$tmp/nonemptydir2" 2>&1) \
		|| err_exit 'rm builtin fails to remove non-empty directory and file with -Rd options' \
			"(got $(printf %q "$got"))"
	[[ -f $tmp/nonemptydir2/shouldexist || -d $tmp/nonemptydir2 ]] \
		&& err_exit 'rm builtin fails to remove all folders and files with -Rd options'

	# Additional test: 'rm -f' without additional arguments should act
	# as a no-op command. This bug was fixed in ksh93u+ 2012-02-14.
	got=$(rm -f 2>&1)
	if (($? != 0)) || [[ ! -z $got ]]
	then	err_exit 'rm -f without additional arguments does not work correctly' \
		"(got $(printf %q "$got"))"
	fi
fi

# ======
# Regression test for https://github.com/att/ast/issues/949
if builtin chmod 2>/dev/null; then
	foo_script='exit 0'
	echo "$foo_script" > "$tmp/foo1.sh"
	echo "$foo_script" > "$tmp/foo2.sh"
	chmod +x "$tmp/foo1.sh" "$tmp/foo2.sh"
	"$tmp/foo1.sh" || err_exit "builtin 'chmod +x' doesn't work on first script"
	"$tmp/foo2.sh" || err_exit "builtin 'chmod +x' doesn't work on second script"
fi

# ======
# https://github.com/ksh93/ksh/issues/138
builtin -d cat
if	[[ $'\n'${ builtin; }$'\n' == *$'\n/opt/ast/bin/cat\n'* ]]
then	exp='  version         cat (*) ????-??-??'
	got=$(/opt/ast/bin/cat --version 2>&1)
	[[ $got == $exp ]] || err_exit "path-bound builtin not executable by literal canonical path" \
		"(expected match of $(printf %q "$exp"), got $(printf %q "$got"))"
	got=$(PATH=/opt/ast/bin:$PATH; "${ whence -p cat; }" --version 2>&1)
	[[ $got == $exp ]] || err_exit "path-bound builtin not executable by canonical path resulting from expansion" \
		"(expected match of $(printf %q "$exp"), got $(printf %q "$got"))"
	got=$(PATH=/opt/ast/bin:$PATH; "$SHELL" -o restricted -c 'cat --version' 2>&1)
	[[ $got == $exp ]] || err_exit "restricted shells do not recognize path-bound builtins" \
		"(expected match of $(printf %q "$exp"), got $(printf %q "$got"))"
	got=$(set +x; PATH=/opt/ast/bin cat --version 2>&1)
	[[ $got == $exp ]] || err_exit "path-bound builtin not found on PATH in preceding assignment" \
		"(expected match of $(printf %q "$exp"), got $(printf %q "$got"))"
else	warning 'skipping path-bound builtin tests: builtin /opt/ast/bin/cat not found'
fi

# ======
# The head and tail builtins should work on files without newlines
if builtin head 2> /dev/null; then
	print -n nonewline > "$tmp/nonewline"
	exp=nonewline
	got=$(head -1 "$tmp/nonewline")
	[[ $got == $exp ]] || err_exit "head builtin fails to correctly handle files without an ending newline" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
fi
if builtin tail 2> /dev/null; then
	print -n 'newline\nnonewline' > "$tmp/nonewline"
	exp=nonewline
	got=$(tail -1 "$tmp/nonewline")
	[[ $got == $exp ]] || err_exit "tail builtin fails to correctly handle files without an ending newline" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
fi

# ======
exit $((Errors<125?Errors:125))

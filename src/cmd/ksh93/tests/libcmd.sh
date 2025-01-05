########################################################################
#                                                                      #
#               This software is part of the ast package               #
#           Copyright (c) 2019-2020 Contributors to ksh2020            #
#          Copyright (c) 2022-2024 Contributors to ksh 93u+m           #
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
#             dnewhall <dnewhall@users.noreply.github.com>             #
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

	# -<integer>, legacy and non-standard form of -n <integer>.
	# (The arithmetic expansion renders whitespace around the output of 'wc' irrelevant.)
	exp=11
	got=$(( $(head -11 < "$tmp/file1" | wc -l) ))
	((got==exp)) || err_exit "'head -<integer>' failed (expected $exp, got $got)"

	#   -n, --lines=l
	#                   Copy l lines from each file. The default value is 10.
	got=$(head -n 3 "$tmp/file1")
	exp=$'This is line 1 in file1\nThis is line 2 in file1\nThis is line 3 in file1'
	[[ $got == "$exp" ]] || err_exit "'head -n' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#   -c, --bytes=chars
	#                   Copy chars bytes from each file.
	got=$(head -c 14 "$tmp/file1")
	exp='This is line 1'
	[[ $got == "$exp" ]] || err_exit "'head -c' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#   -q, --quiet|silent
	#                   Never output filename headers.
	got=$(head -q -n 3 "$tmp/file1" "$tmp/file2")
	exp=$'This is line 1 in file1\nThis is line 2 in file1\nThis is line 3 in file1\nThis is line 1 in file2\nThis is line 2 in file2\nThis is line 3 in file2'
	[[ $got == "$exp" ]] || err_exit "'head -q' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#   -s, --skip=char Skip char characters or lines from each file before copying.
	got=$(head -s 5 -c 18 "$tmp/file1")
	exp='is line 1 in file1'
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
	#                   List the longest line length; the newline (if any) is not
	#                   counted in the length.
	got=$(wc -L "$tmp/file2")
	exp=$"      33 $tmp/file2"
	[[ $got == "$exp" ]] || err_exit "'wc -l' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#   -N, --utf8      For UTF-8 locales --noutf8 disables UTF-8 optimizations and
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
# Tests for the chmod builtin
if builtin chmod 2>/dev/null; then
	# Regression test for https://github.com/att/ast/issues/949
	foo_script='exit 0'
	echo "$foo_script" > "$tmp/foo1.sh"
	echo "$foo_script" > "$tmp/foo2.sh"
	chmod +x "$tmp/foo1.sh" "$tmp/foo2.sh"
	"$tmp/foo1.sh" || err_exit "builtin 'chmod +x' doesn't work on first script"
	"$tmp/foo2.sh" || err_exit "builtin 'chmod +x' doesn't work on second script"

	# chmod changes the permission of each file according to mode, which can be
	# either a symbolic representation of changes to make, or an octal number
	# representing the bit pattern for the new permissions.
	umask 077  # start by disabling setting all group/other permission bits by default
	mkdir "$tmp/foo"
	ln -s "$tmp/foo" "$tmp/symlink_to_foo"
	touch "$tmp/foo/bar"
	ln -s "$tmp/bar" "$tmp/symlink_to_bar"

	# Create some files to use in this test.
	umask 077  # start by disabling setting all group/other permission bits by default
	mkdir d
	touch a b d/e d/f

	# Create a function to report the pathname permissions in a fashion that works on BSD, Linux, and
	# other platforms. This could be simplified if ksh supported a 'stat' builtin.
	if [[ $(stat -f '%Sp' a 2>/dev/null) == '-rw-------' ]]
	then
		# This appears to be a BSD 'stat' command that supports the '-f' flag.
		function stat_perms {
			stat -f '%Sp' "$@"
		}
	elif [[ $(stat --format '%A' a 2>/dev/null) == '-rw-------' ]]
	then
		# This appears to be a GNU 'stat' command that supports the '-A' flag.
		function stat_perms {
			stat -c '%A' "$@"
		}
	fi
	if ! PATH=/dev/null whence -q stat_perms; then
		# We have no idea how to report pathname permissions in a human readable form.
		warning "no compatible 'stat' command found; skipping chmod tests"
	else
		# -L, --logical|follow
		# Follow symbolic links when traversing directories.
		exp=$(stat_perms "$tmp/foo/bar")
		chmod -RL 0777 "$tmp/symlink_to_foo"
		got=$(stat_perms "$tmp/foo/bar")
		[[ $got != "$exp" ]] || err_exit "chmod -L should follow symbolic links (expected $(printf %q "$exp"), got $(printf %q "$got"))"

		# -P, --physical|nofollow
		# Don't follow symbolic links when traversing directories.
		exp=$(stat_perms "$tmp/foo/bar")
		chmod -RP 0747 "$tmp/symlink_to_foo"
		got=$(stat_perms "$tmp/foo/bar")
		[[ $got == "$exp" ]] || err_exit "chmod -P should not follow symbolic links (expected $(printf %q "$exp"), got $(printf %q "$got"))"

		#  -R, --recursive Change the mode for files in subdirectories recursively.
		chmod -R 0777 "$tmp/foo"
		got=$(stat_perms "$tmp/foo/bar")
		exp="-rwxrwxrwx"
		[[ $got == "$exp" ]] || err_exit "chmod -R failed to change permissions (expected $(printf %q "$exp"), got $(printf %q "$got"))"

		#  -c, --changes   Describe only files whose permissions actually change.
		got=$(chmod -c 0755 "$tmp/foo/bar")
		exp="mode changed to 0755 (rwxr-xr-x)"
		[[ $got =~ "$exp" ]] || err_exit "chmod -c failed to change permissions (expected $(printf %q "$exp"), got $(printf %q "$got"))"

		#  -f, --quiet|silent
		#                  Do not report files whose permissions fail to change.
		got=$(chmod -f 0777 "$tmp/this_file_does_not_exit")
		exp=""
		[[ $got == "$exp" ]] || err_exit "chmod -f should give empty output (expected $(printf %q "$exp"), got $(printf %q "$got"))"

		#  -i, --ignore-umask
		#                  Ignore the umask(2) value in symbolic mode expressions. This
		#                  is probably how you expect chmod to work.
		touch "$tmp/this_file_ignores_umask"
		chmod -i 777 "$tmp/this_file_ignores_umask"
		got=$(stat_perms "$tmp/this_file_ignores_umask")
		exp="-rwxrwxrwx"
		[[ $got == "$exp" ]] || err_exit "chmod -i fails to ignore umask (expected $(printf %q "$exp"), got $(printf %q "$got"))"

		#  -n, --show      Show actions but do not change any file modes.
		exp=$(stat_perms "$tmp/foo/bar")
		chmod -n 000 "$tmp/foo/bar"
		got=$(stat_perms "$tmp/foo/bar")
		[[ $got == "$exp" ]] ||
			err_exit "chmod -n should not change permissions (expected $(printf %q "$exp"), got $(printf %q "$got"))"

		# -F, --reference=file
		# Omit the mode operand and use the mode of file instead.
		exp="chmod: /argle/bargle: cannot stat"
		got=$(chmod -F /argle/bargle 2>&1)
		[[ $got == "$exp" ]] || err_exit "chmod -F of a nonexistent file (expected $(printf %q "$exp"), got $(printf %q "$got"))"

		touch "$tmp/foo/baz"
		exp=$(stat_perms "$tmp/foo/bar")
		chmod -F "$tmp/foo/bar" "$tmp/foo/baz"
		got=$(stat_perms "$tmp/foo/baz")
		[[ $got == "$exp" ]] ||
			err_exit "chmod -F should use permission bits from reference file (expected $(printf %q "$exp"), got $(printf %q "$got"))"

		#  -v, --verbose   Describe changed permissions of all files.
		touch "$tmp/check_verbose_mode"
		got=$(chmod -v 000 "$tmp/check_verbose_mode")
		exp='check_verbose_mode: mode changed to 0000 (---------)'
		[[ $got =~ "$exp" ]] || err_exit "chmod -v does not give verbose output (expected $(printf %q "$exp"), got $(printf %q "$got"))"

		# Quick sanity check that the perms are as expected before being modified below.
		exp='-rw-------'
		got=$(stat_perms a)
		[[ $got == "$exp" ]] || err_exit "file 'a' perms wrong at start of test (expected $(printf %q "$exp"), got $(printf %q "$got"))"
		got=$(stat_perms d/f)
		[[ $got == "$exp" ]] || err_exit "file 'd/f' perms wrong at start of test (expected $(printf %q "$exp"), got $(printf %q "$got"))"

		# Can we change the perms on files and dirs without affecting the files in the dir.
		# Test by adding "group" read permission without recursion into dirs.
		chmod g+r a d b
		exp='-rw-r-----'
		got=$(stat_perms a)
		[[ $got == "$exp" ]] || err_exit "file 'a' perms wrong (expected $(printf %q "$exp"), got $(printf %q "$got"))"
		got=$(stat_perms b)
		[[ $got == "$exp" ]] || err_exit "file 'b' perms wrong (expected $(printf %q "$exp"), got $(printf %q "$got"))"

		exp='drwxr-----'
		got=$(stat_perms d)
		[[ $got == "$exp" ]] || err_exit "dir 'd' perms wrong (expected $(printf %q "$exp"), got $(printf %q "$got"))"

		# These two files should not have been modified.
		exp='-rw-------'
		got=$(stat_perms d/e)
		[[ $got == "$exp" ]] || err_exit "file 'd/e' perms wrong (expected $(printf %q "$exp"), got $(printf %q "$got"))"
		got=$(stat_perms d/f)
		[[ $got == "$exp" ]] || err_exit "file 'd/f' perms wrong (expected $(printf %q "$exp"), got $(printf %q "$got"))"

		# Can we change the perms on files and dirs including the files in the dir.
		# Test by adding "other" write permission without recursion into dirs.
		chmod -R o+w b a d
		exp='-rw-r---w-'
		got=$(stat_perms a)
		[[ $got == "$exp" ]] || err_exit "file 'a' perms wrong (expected $(printf %q "$exp"), got $(printf %q "$got"))"
		got=$(stat_perms b)
		[[ $got == "$exp" ]] || err_exit "file 'b' perms wrong (expected $(printf %q "$exp"), got $(printf %q "$got"))"

		exp='drwxr---w-'
		got=$(stat_perms d)
		[[ $got == "$exp" ]] || err_exit "dir 'd' perms wrong (expected $(printf %q "$exp"), got $(printf %q "$got"))"

		# These two subdir files should have been modified.
		exp='-rw-----w-'
		got=$(stat_perms d/e)
		[[ $got == "$exp" ]] || err_exit "file 'd/e' perms wrong (expected $(printf %q "$exp"), got $(printf %q "$got"))"
		got=$(stat_perms d/f)
		[[ $got == "$exp" ]] || err_exit "file 'd/f' perms wrong (expected $(printf %q "$exp"), got $(printf %q "$got"))"

		# Setting multiple bits using human friendly form.
		chmod o=rw,g=x,u-w d/e
		exp='-r----xrw-'
		got=$(stat_perms d/e)
		[[ $got == "$exp" ]] || err_exit "file 'd/e' perms wrong (expected $(printf %q "$exp"), got $(printf %q "$got"))"

		# Setting perms using numeric form.
		chmod 755 a d
		exp='-rwxr-xr-x'
		got=$(stat_perms a)
		[[ $got == "$exp" ]] || err_exit "file 'a' perms wrong (expected $(printf %q "$exp"), got $(printf %q "$got"))"
		exp='drwxr-xr-x'
		got=$(stat_perms d)
		[[ $got == "$exp" ]] || err_exit "dir 'd' perms wrong (expected $(printf %q "$exp"), got $(printf %q "$got"))"

		# Invalid symbolic perms produce an error.
		exp="chmod: -xyz: invalid mode"
		got=$(chmod -xyz a 2>&1)
		[[ $got == "$exp" ]] || err_exit "Invalid numeric perms not handled (expected $(printf %q "$exp"), got $(printf %q "$got"))"

		# Invalid numeric perms produce an error.
		# Note: the use of 'z' rather than 'r', 'w', or other symbolic mode is deliberate.
		# See https://github.com/att/ast/issues/1358.
		exp="chmod: 123z: invalid mode"
		got=$(chmod 123z a 2>&1)
		[[ $got == "$exp" ]] || err_exit "Invalid numeric perms not handled (expected $(printf %q "$exp"), got $(printf %q "$got"))"
	fi
fi

# ======
# Tests for the cat builtin
if builtin cat 2> /dev/null; then
	cat > "$tmp/sample_file" <<-EOF
	foo
	bar


	baz
	EOF
	print \\033x > "$tmp/file_with_control_character"

	# -b, --number-nonblank
	# Number lines as with -n but omit line numbers from blank
	# lines.
	got=$(cat -b "$tmp/sample_file")
	exp=$'     1\tfoo\n     2\tbar\n\n\n     3\tbaz'
	[[ $got == "$exp" ]] || err_exit "cat -b failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# -e is equivalent to -vE.
	got=$(cat -e "$tmp/sample_file")
	exp=$(cat -vE "$tmp/sample_file")
	[[ $got == "$exp" ]] || err_exit "cat -vE failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# -n, --number
	#  Causes a line number to be inserted at the beginning of each
	#  line.
	got=$(cat -n "$tmp/sample_file")
	exp=$'     1\tfoo\n     2\tbar\n     3\t\n     4\t\n     5\tbaz'
	[[ $got == "$exp" ]] || err_exit "cat -n failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# -s Equivalent to -S for att universe and -B otherwise.
	if [[ $(whence -t /opt/ast/bin/getconf 2>&1) == 'builtin' ]]; then
		got=$(cat -s "$tmp/sample_file")
		case $(/opt/ast/bin/getconf UNIVERSE) in
		att)	exp=$(cat -S "$tmp/sample_file") ;;
		*)	exp=$(cat -B "$tmp/sample_file") ;;
		esac
		[[ $got == "$exp" ]] || err_exit "cat -s failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"
	fi

	# -t Equivalent to -vT.
	got=$(cat -t "$tmp/sample_file")
	exp=$(cat -vT "$tmp/sample_file")
	[[ $got == "$exp" ]] || err_exit "cat -t failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# -u, --unbuffer  The output is not delayed by buffering.
	got=$(cat -u "$tmp/sample_file")
	exp=$'foo\nbar\n\n\nbaz'
	[[ $got = "$exp" ]] || err_exit "cat -u failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# -v, --show-nonprinting|print-chars
	#  Print characters as follows: space and printable characters
	#  as themselves; control characters as ^ followed by a letter
	#  of the alphabet; and characters with the high bit set as the
	#  lower 7 bit character prefixed by M^ for 7 bit non-printable
	#  characters and M- for all other characters. If the 7 bit
	#  character encoding is not ASCII then the characters are
	#  converted to ASCII to determine high bit set, and if set it
	#  is cleared and converted back to the native encoding.
	#  Multibyte characters in the current locale are treated as
	#  printable characters.
	got=$(cat -v "$tmp/file_with_control_character")
	exp="^[x"
	[[ $got == "$exp" ]] || err_exit "cat -v failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# -A, --show-all  Equivalent to -vET.
	got=$(cat -A "$tmp/sample_file")
	exp=$(cat -vET "$tmp/sample_file")
	[[ $got == "$exp" ]] || err_exit "cat -A failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# -B, --squeeze-blank
	#  Multiple adjacent new-line characters are replaced by one
	#  new-line.
	got=$(cat -B "$tmp/sample_file")
	exp=$'foo\nbar\n\nbaz'
	[[ $got == "$exp" ]] || err_exit "cat -B failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# -E, --show-ends Causes a $ to be inserted before each new-line.
	got=$(cat -E "$tmp/sample_file")
	exp=$'foo$\nbar$\n$\n$\nbaz$'
	[[ $got == "$exp" ]] || err_exit "cat -E failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# -R, --regress   Regression test defaults: -v buffer size 4.
	got=$(cat -R "$tmp/sample_file")
	exp=$'foo\nbar\n\n\nbaz'
	[[ $got == "$exp" ]] || err_exit "cat -R failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# -S, --silent    cat is silent about non-existent files.
	got=$(cat -S this_file_does_not_exist 2>&1)
	exp=""
	[[ $got == "$exp" ]] || err_exit "cat -S should give no error on non-existent files (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# -T, --show-blank|show-tabs
	#  Causes tabs to be copied as ^I.
	print "a\tb" > "$tmp/file_with_tabs"
	got=$(cat -T "$tmp/file_with_tabs")
	exp="a^Ib"
	[[ $got == "$exp" ]] || err_exit "cat -T failed to convert tabs to ^I. (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	got=$(cat this_file_does_not_exist 2>&1)
	exp="this_file_does_not_exist: cannot open"
	[[ $got =~ $exp ]] || err_exit "cat should give an error on non-existent files (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# https://github.com/ksh93/ksh/issues/138
	builtin -d cat
	if	[[ $'\n'${ builtin; }$'\n' == *$'\n/opt/ast/bin/cat\n'* ]]
	then	exp='  version         cat (*) ????-??-??'
		got=$(/opt/ast/bin/cat --version 2>&1)
		[[ $got == $exp && $got != *$'\n'* ]] || err_exit "path-bound builtin not executable by literal canonical path" \
			"(expected match of $(printf %q "$exp"), got $(printf %q "$got"))"
		got=$(PATH=/opt/ast/bin:$PATH; "${ whence -p cat; }" --version 2>&1)
		[[ $got == $exp && $got != *$'\n'* ]] || err_exit "path-bound builtin not executable by canonical path resulting from expansion" \
			"(expected match of $(printf %q "$exp"), got $(printf %q "$got"))"
		got=$(PATH=/opt/ast/bin:$PATH; "$SHELL" -o restricted -c 'cat --version' 2>&1)
		[[ $got == $exp && $got != *$'\n'* ]] || err_exit "restricted shells do not recognize path-bound builtins" \
			"(expected match of $(printf %q "$exp"), got $(printf %q "$got"))"
		got=$(set +x; PATH=/dev/null; PATH=/opt/ast/bin cat --version 2>&1)
		[[ $got == $exp && $got != *$'\n'* ]] || err_exit "path-bound builtin on PATH in preceding assignment" \
			"(expected match of $(printf %q "$exp"), got $(printf %q "$got"))"
	else	warning 'skipping path-bound builtin tests: builtin /opt/ast/bin/cat not found'
	fi
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
# Tests for the basename builtin
if builtin basename 2> /dev/null; then
	got=$(basename "$tmp/foo.bar")
	exp="foo.bar"
	[[ $got == "$exp" ]] || err_exit "basename failed to print filename (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# Last argument is treated as suffix and removed from output
	got=$(basename "$tmp/foo.bar" .bar)
	exp="foo"
	[[ $got == "$exp" ]] || err_exit "basename failed to strip suffix (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# -a, --all|multiple
	#  All operands are treated as string and each modified pathname
	#  is printed on a separate line on the standard output.
	got=$(basename -a "$tmp/foo.bar" "$tmp/bar.bar" "$tmp/baz.bar")
	exp=$'foo.bar\nbar.bar\nbaz.bar'
	[[ $got == "$exp" ]] || err_exit "basename -a failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# -s, --suffix=suffix
	#  All operands are treated as string and each modified
	#  pathname, with suffix removed if it exists, is printed on a
	#  separate line on the standard output.
	got=$(basename -s bar "$tmp/foo.bar" "$tmp/bar.bar" "$tmp/baz.bar")
	exp=$'foo.\nbar.\nbaz.'
	[[ $got == "$exp" ]] || err_exit "basename -s failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	got=$(basename "$tmp/.bar")
	exp=".bar"
	[[ $got == "$exp" ]] || err_exit "basename failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# PATH_LEADING_SLASHES handling
	case $(builtin getconf 2>/dev/null && getconf PATH_LEADING_SLASHES 2>/dev/null) in
	1)	exp=$'/\n//\n//\n//' ;;
	*)	exp=$'/\n/\n/\n/' ;;
	esac
	got=$(basename -a / // /// ////)
	[[ $got == "$exp" ]] || err_exit "PATH_LEADING_SLASHES handling (expected $(printf %q "$exp"), got $(printf %q "$got"))"
fi

# ======
# Tests for the cmp builtin
if builtin cmp 2> /dev/null; then
	cat  > "$tmp/file1" <<-EOF
	This is file 1
	EOF

	cat  > "$tmp/file2" <<-EOF
	This is file 2
	EOF

	cat  > "$tmp/file23" <<-EOF
	This is file 23
	EOF

	# EXIT STATUS
	#     0     The files or portions compared are identical.
	cmp "$tmp/file1" "$tmp/file1"
	got=$?
	exp=0
	((got == exp)) || err_exit "cmp should return 0 for identical files (expected $exp, got $got)"

	#     1     The files are different.
	cmp "$tmp/file1" "$tmp/file2" > /dev/null
	got=$?
	exp=1
	((got == exp)) || err_exit "cmp should return 1 for not identical files (expected $exp, got $got)"

	#     >1    An error occurred.
	cmp "$tmp/file1" "$tmp/this_file_does_not_exist" 2> /dev/null
	got=$?
	exp=2
	((got == exp)) || err_exit "cmp should return 2 on error (expected $exp, got $got)"

	#  -b, --print-bytes
	#                  Print differing bytes as 3 digit octal values.
	got=$(cmp -b "$tmp/file1" "$tmp/file2")
	exp="$tmp/file1 $tmp/file2 differ: char 14, line 1, 061 062"
	[[ $got == "$exp" ]] || err_exit "'cmp -b' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#  -c, --print-chars
	#                  Print differing bytes as follows: non-space printable
	#                  characters as themselves; space and control characters as ^
	#                  followed by a letter of the alphabet; and characters with the
	#                  high bit set as the lower 7 bit character prefixed by M^ for
	#                  7 bit space and non-printable characters and M- for all other
	#                  characters. If the 7 bit character encoding is not ASCII then
	#                  the characters are converted to ASCII to determine high bit
	#                  set, and if set it is cleared and converted back to the
	#                  native encoding. Multibyte characters in the current locale
	#                  are treated as printable characters.
	got=$(cmp -c "$tmp/file1" "$tmp/file2")
	exp="$tmp/file1 $tmp/file2 differ: char 14, line 1,   1   2"
	[[ $got == "$exp" ]] || err_exit "'cmp -c' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#  -d, --differences=differences
	#                  Print at most differences differences using --verbose output
	#                  format. --differences=0 is equivalent to --silent.
	got=$(cmp -d 1 "$tmp/file1" "$tmp/file23")
	exp="    14  061 062"
	[[ $got == "$exp" ]] || err_exit "'cmp -d' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#  -i, --ignore-initial|skip=skip1[:skip2]
	#                  Skip the first skip1 bytes in file1 and the first skip2
	#                  bytes in file2. If skip2 is omitted then skip1 is used. The
	#                  default value is 0:0.
	got=$(cmp -c -i 8:8 "$tmp/file1" "$tmp/file2")
	exp="$tmp/file1 $tmp/file2 differ: char 6, line 1,   1   2"
	[[ $got == "$exp" ]] || err_exit "'cmp -i 8' should ignore first 8 bytes (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#  -l, --verbose   Write the decimal byte number and the differing bytes (in
	#                  octal) for each difference.
	got=$(cmp -l "$tmp/file1" "$tmp/file2")
	exp="    14  061 062"
	[[ $got == "$exp" ]] || err_exit "'cmp -l' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#  -n, --count|bytes=count
	#                  Compare at most count bytes.
	cmp -n 13 "$tmp/file1" "$tmp/file2"
	got=$?
	exp=0
	((got == exp)) || err_exit "'cmp -n 13' should compare at most 13 bytes (expected $exp, got $got)"

	#  -s, --quiet|silent
	#                  Write nothing for differing files; return non-zero exit
	#                  status only.
	#
	got=$(cmp -s "$tmp/file1" "$tmp/file2")
	exp=""
	[[ $got == "$exp" ]] || err_exit "'cmp -s' should give empty output (expected $(printf %q "$exp"), got $(printf %q "$got"))"
fi

# ======
# Test for the cut builtin
if builtin cut 2> /dev/null; then
	rm -rf "$tmp/foo"
	cat > "$tmp/foo" <<-EOF
	foo:bar:baz
	bar:baz:foo
	baz:foo:bar
	foobarbaz
	EOF

	# https://github.com/att/ast/issues/1157
	#   -b, --bytes=list
	#                   cut based on a list of byte counts.
	got=$(cut -b1 "$tmp/foo")
	exp=$'f\nb\nb\nf'
	[[ $got == "$exp" ]] || err_exit "'cut -b' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#   -c, --characters=list
	#                   cut based on a list of character counts.
	got=$(cut -c1 "$tmp/foo")
	exp=$'f\nb\nb\nf'
	[[ $got == "$exp" ]] || err_exit "'cut -c' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"
	got=$(cut -c1,3 "$tmp/foo")
	exp=$'fo\nbr\nbz\nfo'
	[[ $got == "$exp" ]] || err_exit "'cut -c1,3' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#   -d, --delimiter=delim
	#                   The field character for the -f option is set to delim. The
	#                   default is the tab character.
	got=$(cut -d: -f1 "$tmp/foo")
	exp=$'foo\nbar\nbaz\nfoobarbaz'
	[[ $got == "$exp" ]] || err_exit "'cut -d' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#   -f, --fields=list
	#                   cut based on fields separated by the delimiter character
	#                   specified with the -d option.
	# This test also tests for default delimiter i.e. tab character
	got=$(cat "$tmp/foo" | tr ':' '\t' | cut -f1 )
	exp=$'foo\nbar\nbaz\nfoobarbaz'
	[[ $got == "$exp" ]] || err_exit "'cut -f' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#   -n, --split     Split multibyte characters selected by the -b option. On by
	#                   default; -n means --nosplit.
	got=$(cut -b1 -n "$tmp/foo")
	exp=$'f\nb\nb\nf'
	[[ $got == "$exp" ]] || err_exit "'cut -n' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#   -R|r, --reclen=reclen
	#                   If reclen > 0, the input will be read as fixed length records
	#                   of length reclen when used with the -b or -c option.
	got=$(cut -b1 -r4 "$tmp/foo")
	exp=$'f\nb\nb\nb\nb\nf\nb\nf\nb\nf\na'
	[[ $got == "$exp" ]] || err_exit "'cut -r' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#   -s, --suppress|only-delimited
	#                   Suppress lines with no delimiter characters, when used with
	#                   the -f option. By default, lines with no delimiters will be
	#                   passed in untouched.
	got=$(cut -d: -f1 -s "$tmp/foo")
	exp=$'foo\nbar\nbaz'
	[[ $got == "$exp" ]] || err_exit "'cut -s' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#   -D, --line-delimiter|output-delimiter=ldelim
	#                   The line delimiter character for the -f option is set to
	#                   ldelim. The default is the newline character.
	got=$(cut -D: -f1 "$tmp/foo")
	exp=$'foo:bar:baz\nbar:baz:foo\nbaz:foo:bar\nfoobarbaz'
	[[ $got == "$exp" ]] || err_exit "'cut -D' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#   -N, --newline   Output new-lines at end of each record when used with the -b
	#                   or -c option. On by default; -N means --nonewline.
	got=$(cut -N -d: -f1 "$tmp/foo")
	exp=$'foo\nbar\nbaz\nfoobarbaz'
	[[ $got == "$exp" ]] || err_exit "'cut -d' failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	got=$(cut -c1 -f1 "$tmp/foo" 2>&1)
	exp='cut: c option already specified'
	[[ $got =~ "$exp" ]] || err_exit "'cut -b1 f1' should show an error (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	got=$(cut -f1 -c1 "$tmp/foo" 2>&1)
	exp='cut: f option already specified'
	[[ $got =~ "$exp" ]] || err_exit "'cut -f1 c1' should show an error (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	got=$(cut "$tmp/foo" 2>&1)
	exp='cut: b, c or f option must be specified'
	[[ $got =~ "$exp" ]] || err_exit "'cut' without b, c or f options should show an error (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	got=$(cut -c -xyz "$tmp/foo" 2>&1)
	exp='cut: bad list for c/f option'
	[[ $got =~ "$exp" ]] || err_exit "'cut -b1 f1' should show an error (expected $(printf %q "$exp"), got $(printf %q "$got"))"
fi

# ======
# Tests for the dirname builtin
if builtin dirname 2> /dev/null; then
	# dirname treats string as a file name and returns the name of the directory
	# containing the file name by deleting the last component from string.
	mkdir "$tmp/dir"
	got=$(dirname "$tmp/dir")
	exp=$tmp
	[[ $got == "$exp" ]] || err_exit "dirname failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#  -f, --file      Print the $PATH relative regular file path for string.
	got=$(dirname -f cat)
	exp=$(whence -p cat)
	[[ $got == "$exp" ]] || err_exit "dirname -f failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#  -r, --relative  Print the $PATH relative readable file path for string.
	got=$(dirname -r cat)
	[[ $got == "$exp" ]] || err_exit "dirname -r failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	#  -x, --executable
	#  Print the $PATH relative executable file path for string.
	got=$(dirname -x cat)
	[[ $got == "$exp" ]] || err_exit "dirname -x failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# PATH_LEADING_SLASHES handling; multiple operands
	exp=$'/\n//\n//\n//\n//\n//server\n//server'
	got=$(export _AST_FEATURES='PATH_LEADING_SLASHES - 1'
	      "$SHELL" -c 'builtin dirname && dirname / // // //server ///server //server/name ///server//name' 2>&1)
	[[ $got == "$exp" ]] || err_exit "PATH_LEADING_SLASHES handling (expected $(printf %q "$exp"), got $(printf %q "$got"))"
fi

# ======
# Tests for the mkdir builtin
if builtin mkdir 2> /dev/null; then
	# mkdir creates one or more directories. By default, the mode of created
	# directories is a=rwx minus the bits set in the umask(1).
	umask 000

	rm -rf "$tmp/foo" "$tmp/bar"
	mkdir "$tmp/foo"
	[[ -e "$tmp/foo" ]] || err_exit "mkdir failed"

	#  -m, --mode=mode Set the mode of created directories to mode. mode is symbolic
	#                  or octal mode as in chmod(1). Relative modes assume an
	#                  initial mode of a=rwx.
	mkdir -m 700 "$tmp/bar"
	[[ -r "$tmp/bar" ]] || err_exit "$tmp/bar should be readable"
	[[ -w "$tmp/bar" ]] || err_exit "$tmp/bar should be writable"
	[[ -x "$tmp/bar" ]] || err_exit "$tmp/bar should be executable"

	#  -p, --parents   Create any missing intermediate pathname components. For each
	#                  dir operand that does not name an existing directory, effects
	#                  equivalent to those caused by the following command shall
	#                  occur: mkdir -p -m $(umask -S),u+wx $(dirname dir) && mkdir
	#                  [-m mode] dir where the -m mode option represents that option
	#                  supplied to the original invocation of mkdir, if any. Each
	#                  dir operand that names an existing directory shall be ignored
	#                  without error.
	mkdir -p "$tmp/foo/bar/baz"
	[[ -d "$tmp/foo/bar/baz" ]] || err_exit "mkdir -p failed"

	#  -v, --verbose   Print a message on the standard error for each created
	#                  directory.
	got=$(mkdir -v "$tmp/foo.bar" 2>&1)
	exp="foo.bar: directory created"
	[[ $got =~ "$exp" ]] || err_exit "mkdir -v failed (expected $(printf %q "$exp"), got $(printf %q "$got"))"
fi

# ======
# Tests for the getconf builtin
if builtin getconf 2> /dev/null; then
	hosttype=$(getconf HOSTTYPE)
	bad_result=$(getconf --version 2>&1)

	# The -l option should convert all variable names to lowercase.
	# https://github.com/att/ast/issues/1171
	got=$(getconf -lq | LC_ALL=C sed -n -e 's/=.*//' -e '/[A-Z]/p')
	[[ -n $got ]] && err_exit "'getconf -l' doesn't convert all variable names to lowercase" \
		"(got $(printf %q "$got"))"

	# The -q option should quote all string values.
	# https://github.com/att/ast/issues/1173
	exp="HOSTTYPE=\"$hosttype\""
	got=$(getconf -q | grep 'HOSTTYPE=')
	[[ $exp == "$got" ]] || err_exit "'getconf -q' fails to quote string values" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

	# The -n option should only return matching names.
	# https://github.com/ksh93/ksh/issues/279
	exp="HOSTTYPE=$hosttype"
	got=$(getconf -n HOSTTYPE)
	[[ $exp == "$got" ]] || err_exit "'getconf -n' doesn't match names correctly" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
fi

# ======
# https://github.com/ksh93/ksh/issues/794

if builtin cp 2>/dev/null; then
	got=$(set +x; { (ulimit -c 0; cp --preserve=foo); } 2>&1)
	let "(e=$?) == 2" || err_exit "crash on unexpected option value" \
		"(got status $e$( ((e>128)) && print -n /SIG && kill -l "$e"), $(printf %q "$got"))"
fi

# ======
exit $((Errors<125?Errors:125))

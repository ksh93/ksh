########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1982-2011 AT&T Intellectual Property          #
#          Copyright (c) 2020-2021 Contributors to ksh 93u+m           #
#                      and is licensed under the                       #
#                 Eclipse Public License, Version 1.0                  #
#                    by AT&T Intellectual Property                     #
#                                                                      #
#                A copy of the License is available at                 #
#          http://www.eclipse.org/org/documents/epl-v10.html           #
#         (with md5 checksum b35adb5213ca9657e911e9befb180842)         #
#                                                                      #
#              Information and Software Systems Research               #
#                            AT&T Research                             #
#                           Florham Park NJ                            #
#                                                                      #
#                  David Korn <dgk@research.att.com>                   #
#                                                                      #
########################################################################

. "${SHTESTS_COMMON:-${0%/*}/_common}"

alias foo='print hello'
if	[[ $(foo) != hello ]]
then	err_exit 'foo, where foo is alias for "print hello" failed'
fi
if	[[ $(foo world) != 'hello world' ]]
then	err_exit 'foo world, where foo is alias for "print hello" failed'
fi
alias foo='print hello '
alias bar=world
if	[[ $(foo bar) != 'hello world' ]]
then	err_exit 'foo bar, where foo is alias for "print hello " failed'
fi
if	[[ $(foo \bar) != 'hello bar' ]]
then	err_exit 'foo \bar, where foo is alias for "print hello " failed'
fi
alias bar='foo world'
if	[[ $(bar) != 'hello world' ]]
then	err_exit 'bar, where bar is alias for "foo world" failed'
fi
if	[[ $(alias bar) != "bar='foo world'" ]]
then	err_exit 'alias bar, where bar is alias for "foo world" failed'
fi
unalias foo  || err_exit  "unalias foo failed"
alias foo 2> /dev/null  && err_exit "alias for non-existent alias foo returns true"
unset bar
alias bar="print foo$bar"
bar=bar
if	[[ $(bar) != foo ]]
then	err_exit 'alias bar, where bar is alias for "print foo$bar" failed'
fi
unset bar
alias bar='print hello'
if	[[ $bar != '' ]]
then	err_exit 'alias bar cause variable bar to be set'
fi
alias !!=print
if	[[ $(!! hello 2>/dev/null) != hello ]]
then	err_exit 'alias for !!=print not working'
fi
alias foo=echo
if	[[ $(print  "$(foo bar)" ) != bar  ]]
then	err_exit 'alias in command substitution not working'
fi
( unalias foo)
if	[[ $(foo bar 2> /dev/null)  != bar  ]]
then	err_exit 'alias not working after unalias in subshell'
fi
builtin -d rm 2> /dev/null
if	whence rm > /dev/null
then	[[ ! $(alias -t | grep rm= ) ]] && err_exit 'tracked alias not set'
	PATH=$PATH
	[[ $(alias -t | grep rm= ) ]] && err_exit 'tracked alias not cleared'
fi
if	hash -r 2>/dev/null && [[ ! $(hash) ]]
then	PATH=$tmp:$PATH
	for i in foo -foo --
	do	print ':' > $tmp/$i
		chmod +x $tmp/$i
		hash -r -- $i 2>/dev/null || err_exit "hash -r -- $i failed"
		[[ $(hash) == $i=$tmp/$i ]] || err_exit "hash -r -- $i failed, expected $i=$tmp/$i, got $(hash)"
	done
else	err_exit 'hash -r failed'
fi
( alias :pr=print) 2> /dev/null || err_exit 'alias beginning with : fails'
( alias p:r=print) 2> /dev/null || err_exit 'alias with : in name fails'

unalias no_such_alias && err_exit 'unalias should return non-zero for unknown alias'

# ======
# Adding a utility after resetting the hash table should work
hash -r chmod
[[ $(hash) == "chmod=$(whence -p chmod)" ]] || err_exit $'"hash -r \'utility\'" doesn\'t reset the hash table correctly'

# ======
# Attempting to unalias a previously set alias twice should be an error
alias foo=bar
unalias foo
unalias foo && err_exit 'unalias should return non-zero when a previously set alias is unaliased twice'

# Removing the history and r aliases should work without an error from free(3)
err=$(set +x; { "$SHELL" -i -c 'unalias history; unalias r'; } 2>&1) && [[ -z $err ]] \
|| err_exit "the 'history' and 'r' aliases can't be removed (got $(printf %q "$err"))"

# ======
# Listing aliases in a script shouldn't break shcomp bytecode
exp="alias foo='bar('
alias four=4
alias three=3
alias two=2
foo='bar('
foo='bar('
four=4
three=3
two=2
foo='bar('
four=4
three=3
two=2
noalias: alias not found
ls=$(whence -p ls)
alias -t ls"
alias_script=$tmp/alias_script.sh
cat > "$alias_script" << EOF
unalias -a
alias foo='bar('
alias two=2
alias three=3
alias four=4
alias -p
alias foo
alias -x
alias
alias noalias
alias -t ls
alias -t
alias -pt
EOF
got=$(redirect 2>&1; $SHELL <($SHCOMP "$alias_script"))
[[ $exp == $got ]] || err_exit "Listing aliases corrupts shcomp bytecode" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Specifying an alias with 'alias -p' should print that alias in a reusable form
exp='alias foo=BAR'
got=$(
	alias foo=BAR bar=FOO
	alias -p foo
)
[[ $exp == $got ]] || err_exit "Specifying an alias with 'alias -p' doesn't work" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

exp="cat=$(whence -p cat)
alias -t cat"
got=$(
	hash -r cat
	alias -tx
	alias -ptx
)
[[ $exp == $got ]] || err_exit "The alias command doesn't ignore the obsolete -x option when -t is passed" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Listing members of the hash table with 'alias -pt' should work
exp='alias -t cat
vi: tracked alias not found
alias -t cat
alias -t chmod'
got=$(
	redirect 2>&1
	hash -r cat chmod
	alias -pt cat vi  # vi shouldn't be added to the hash table
	alias -pt
)
[[ $exp == $got ]] || err_exit "Listing members of the hash table with 'alias -pt' doesn't work" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Attempting to list a non-existent alias or tracked alias should fail
# with an error message. Note that the exit status should match the
# number of aliases passed that don't exist.
exp='foo: alias not found
bar: alias not found
nosuchalias: alias not found'
got=$(
	redirect 2>&1
	unalias -a
	alias foo bar nosuchalias
)
ret=$?
[[ $exp == $got ]] || err_exit "Listing non-existent aliases with 'alias' should fail with an error message" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
((ret == 3)) || err_exit "Listing non-existent aliases with 'alias' has wrong exit status" \
	"(expected 3, got $ret)"

# Same as above, but tests alias -p with a different number
# of non-existent aliases.
exp='foo: alias not found
bar: alias not found
nosuchalias: alias not found
stillnoalias: alias not found'
got=$(
	redirect 2>&1
	unalias -a
	alias -p foo bar nosuchalias stillnoalias
)
ret=$?
[[ $exp == $got ]] || err_exit "Listing non-existent aliases with 'alias -p' should fail with an error message" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
((ret == 4)) || err_exit "Listing non-existent aliases with 'alias -p' has wrong exit status" \
	"(expected 4, got $ret)"

# Tracked aliases next.
exp='rm: tracked alias not found
ls: tracked alias not found'
got=$(
	redirect 2>&1
	hash -r
	alias -pt rm ls
)
ret=$?
[[ $exp == $got ]] || err_exit "Listing non-existent tracked aliases with 'alias -pt' should fail with an error message" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
((ret == 2)) || err_exit "Listing non-existent tracked aliases with 'alias -pt' has wrong exit status" \
	"(expected 2, got $ret)"

# Detect zombie aliases with 'alias'.
exp='vi: alias not found
chmod: alias not found'
got=$($SHELL -c '
	hash vi chmod
	hash -r
	alias vi chmod
' 2>&1)
ret=$?
[[ $exp == $got ]] || err_exit "Listing removed tracked aliases with 'alias' should fail with an error message" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
((ret == 2)) || err_exit "Listing removed tracked aliases with 'alias' has wrong exit status" \
	"(expected 2, got $ret)"

# Detect zombie aliases with 'alias -p'.
exp='vi: alias not found
chmod: alias not found'
got=$($SHELL -c '
	hash vi chmod
	hash -r
	alias -p vi chmod
' 2>&1)
ret=$?
[[ $exp == $got ]] || err_exit "Listing removed tracked aliases with 'alias -p' should fail with an error message" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
((ret == 2)) || err_exit "Listing removed tracked aliases with 'alias -p' has wrong exit status" \
	"(expected 2, got $ret)"

# Detect zombie tracked aliases.
exp='vi: tracked alias not found
chmod: tracked alias not found'
got=$($SHELL -c '
	hash vi chmod
	hash -r
	alias -pt vi chmod
' 2>&1)
ret=$?
[[ $exp == $got ]] || err_exit "Listing removed tracked aliases with 'alias -pt' should fail with an error message" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
((ret == 2)) || err_exit "Listing removed tracked aliases with 'alias -pt' has wrong exit status" \
	"(expected 2, got $ret)"

# The exit status on error must be >0, including when handling
# 256 non-existent aliases.
(unalias -a; alias $(awk -v ORS= 'BEGIN { for(i=0;i<256;i++) print "x "; }') 2> /dev/null)
got=$?
((got > 0)) || err_exit "Exit status is zero when alias is passed 256 non-existent aliases"

# ======
exit $((Errors<125?Errors:125))

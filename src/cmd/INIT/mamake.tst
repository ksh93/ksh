# : : generated from mamake.rt by mktest : : #

# regression tests for the mamake command

UNIT mamake

TEST 01 expansions

	EXEC	-n -f ._tmp_Mamfile_
		INPUT -n -
		INPUT ._tmp_Mamfile_ $'info mam static 00000 1994-07-17 make (AT&T Research) 5.3 2009-05-05
setv DEFINED defined
setv EMPTY
make all virtual

exec - echo DEFINED ${DEFINED}
exec - echo DEFINED:VALUE ${DEFINED:VALUE}
exec - echo DEFINED:-VALUE ${DEFINED:-VALUE}
exec - echo DEFINED=VALUE ${DEFINED=VALUE}
exec - echo DEFINED[VALUE] ${DEFINED[VALUE]}
exec - echo DEFINED.COMPONENT ${DEFINED.COMPONENT}
exec - echo DEFINED.COMPONENT[VALUE] ${DEFINED.COMPONENT[VALUE]}

exec - echo EMPTY ${EMPTY}
exec - echo EMPTY:VALUE ${EMPTY:VALUE}
exec - echo EMPTY:-VALUE ${EMPTY:-VALUE}
exec - echo EMPTY=VALUE ${EMPTY=VALUE}
exec - echo EMPTY[VALUE] ${EMPTY[VALUE]}
exec - echo EMPTY.COMPONENT ${EMPTY.COMPONENT}
exec - echo EMPTY.COMPONENT[VALUE] ${EMPTY.COMPONENT[VALUE]}

exec - echo __NoT_DeFiNeD__ ${__NoT_DeFiNeD__}
exec - echo __NoT_DeFiNeD__:VALUE ${__NoT_DeFiNeD__:VALUE}
exec - echo __NoT_DeFiNeD__:-VALUE ${__NoT_DeFiNeD__:-VALUE}
exec - echo __NoT_DeFiNeD__=VALUE ${__NoT_DeFiNeD__=VALUE}
exec - echo __NoT_DeFiNeD__[VALUE] ${__NoT_DeFiNeD__[VALUE]}
exec - echo __NoT_DeFiNeD__.COMPONENT ${__NoT_DeFiNeD__.COMPONENT}
exec - echo __NoT_DeFiNeD__.COMPONENT[VALUE] ${__NoT_DeFiNeD__.COMPONENT[VAL'\
$'UE]}

done all'
		OUTPUT - $'echo DEFINED defined
echo DEFINED:VALUE 
echo DEFINED:-VALUE 
echo DEFINED=VALUE defined
echo DEFINED[VALUE] ${DEFINED[VALUE]}
echo DEFINED.COMPONENT 
echo DEFINED.COMPONENT[VALUE] ${DEFINED.COMPONENT[VALUE]}
echo EMPTY 
echo EMPTY:VALUE ${EMPTY:VALUE}
echo EMPTY:-VALUE ${EMPTY:-VALUE}
echo EMPTY=VALUE 
echo EMPTY[VALUE] ${EMPTY[VALUE]}
echo EMPTY.COMPONENT 
echo EMPTY.COMPONENT[VALUE] ${EMPTY.COMPONENT[VALUE]}
echo __NoT_DeFiNeD__ ${__NoT_DeFiNeD__}
echo __NoT_DeFiNeD__:VALUE ${__NoT_DeFiNeD__:VALUE}
echo __NoT_DeFiNeD__:-VALUE ${__NoT_DeFiNeD__:-VALUE}
echo __NoT_DeFiNeD__=VALUE ${__NoT_DeFiNeD__=VALUE}
echo __NoT_DeFiNeD__[VALUE] ${__NoT_DeFiNeD__[VALUE]}
echo __NoT_DeFiNeD__.COMPONENT 
echo __NoT_DeFiNeD__.COMPONENT[VALUE] ${__NoT_DeFiNeD__.COMPONENT[VALUE]}'
		ERROR - $'\n# ._tmp_Mamfile_: 4-30: make all'

TEST 02 'expansions at strict level 1'

	EXEC	-n -f ._tmp_Mamfile_
		INPUT -n -
		INPUT ._tmp_Mamfile_ $'setv MAMAKE_STRICT 1
setv DEFINED defined
setv EMPTY
make all virtual

exec - echo DEFINED ${DEFINED}
exec - echo DEFINED:VALUE ${DEFINED:VALUE}
exec - echo DEFINED:-VALUE ${DEFINED:-VALUE}
exec - echo DEFINED=VALUE ${DEFINED=VALUE}
exec - echo DEFINED[VALUE] ${DEFINED[VALUE]}
exec - echo DEFINED.COMPONENT ${DEFINED.COMPONENT}
exec - echo DEFINED.COMPONENT[VALUE] ${DEFINED.COMPONENT[VALUE]}

exec - echo EMPTY ${EMPTY}
exec - echo EMPTY:VALUE ${EMPTY:VALUE}
exec - echo EMPTY:-VALUE ${EMPTY:-VALUE}
exec - echo EMPTY=VALUE ${EMPTY=VALUE}
exec - echo EMPTY[VALUE] ${EMPTY[VALUE]}
exec - echo EMPTY.COMPONENT ${EMPTY.COMPONENT}
exec - echo EMPTY.COMPONENT[VALUE] ${EMPTY.COMPONENT[VALUE]}

exec - echo __NoT_DeFiNeD__ ${__NoT_DeFiNeD__}
exec - echo __NoT_DeFiNeD__:VALUE ${__NoT_DeFiNeD__:VALUE}
exec - echo __NoT_DeFiNeD__:-VALUE ${__NoT_DeFiNeD__:-VALUE}
exec - echo __NoT_DeFiNeD__=VALUE ${__NoT_DeFiNeD__=VALUE}
exec - echo __NoT_DeFiNeD__[VALUE] ${__NoT_DeFiNeD__[VALUE]}
exec - echo __NoT_DeFiNeD__.COMPONENT ${__NoT_DeFiNeD__.COMPONENT}
exec - echo __NoT_DeFiNeD__.COMPONENT[VALUE] ${__NoT_DeFiNeD__.COMPONENT[VAL'\
$'UE]}

done all'
		OUTPUT - $'echo DEFINED defined
echo DEFINED:VALUE 
echo DEFINED:-VALUE 
echo DEFINED=VALUE defined
echo DEFINED[VALUE] ${DEFINED[VALUE]}
echo DEFINED.COMPONENT 
echo DEFINED.COMPONENT[VALUE] ${DEFINED.COMPONENT[VALUE]}
echo EMPTY 
echo EMPTY:VALUE ${EMPTY:VALUE}
echo EMPTY:-VALUE ${EMPTY:-VALUE}
echo EMPTY=VALUE 
echo EMPTY[VALUE] ${EMPTY[VALUE]}
echo EMPTY.COMPONENT 
echo EMPTY.COMPONENT[VALUE] ${EMPTY.COMPONENT[VALUE]}
echo __NoT_DeFiNeD__ ${__NoT_DeFiNeD__}
echo __NoT_DeFiNeD__:VALUE ${__NoT_DeFiNeD__:VALUE}
echo __NoT_DeFiNeD__:-VALUE ${__NoT_DeFiNeD__:-VALUE}
echo __NoT_DeFiNeD__=VALUE ${__NoT_DeFiNeD__=VALUE}
echo __NoT_DeFiNeD__[VALUE] ${__NoT_DeFiNeD__[VALUE]}
echo __NoT_DeFiNeD__.COMPONENT 
echo __NoT_DeFiNeD__.COMPONENT[VALUE] ${__NoT_DeFiNeD__.COMPONENT[VALUE]}'
		ERROR - $'\n# ._tmp_Mamfile_: 4-30: make all'

TEST 03 'expansions at strict level 2'

	EXEC	-n -f ._tmp_Mamfile_
		INPUT -n -
		INPUT ._tmp_Mamfile_ $'setv MAMAKE_STRICT 2
setv DEFINED defined
setv EMPTY
make all virtual

exec - echo DEFINED ${DEFINED}
exec - echo DEFINED:VALUE ${DEFINED:VALUE}
exec - echo DEFINED:-VALUE ${DEFINED:-VALUE}
exec - echo DEFINED=VALUE ${DEFINED=VALUE}
exec - echo DEFINED[VALUE] ${DEFINED[VALUE]}
exec - echo DEFINED.COMPONENT ${DEFINED.COMPONENT}
exec - echo DEFINED.COMPONENT[VALUE] ${DEFINED.COMPONENT[VALUE]}

exec - echo EMPTY ${EMPTY}
exec - echo EMPTY:VALUE ${EMPTY:VALUE}
exec - echo EMPTY:-VALUE ${EMPTY:-VALUE}
exec - echo EMPTY=VALUE ${EMPTY=VALUE}
exec - echo EMPTY[VALUE] ${EMPTY[VALUE]}
exec - echo EMPTY.COMPONENT ${EMPTY.COMPONENT}
exec - echo EMPTY.COMPONENT[VALUE] ${EMPTY.COMPONENT[VALUE]}

exec - echo __NoT_DeFiNeD__ ${__NoT_DeFiNeD__}
exec - echo __NoT_DeFiNeD__:VALUE ${__NoT_DeFiNeD__:VALUE}
exec - echo __NoT_DeFiNeD__:-VALUE ${__NoT_DeFiNeD__:-VALUE}
exec - echo __NoT_DeFiNeD__=VALUE ${__NoT_DeFiNeD__=VALUE}
exec - echo __NoT_DeFiNeD__[VALUE] ${__NoT_DeFiNeD__[VALUE]}
exec - echo __NoT_DeFiNeD__.COMPONENT ${__NoT_DeFiNeD__.COMPONENT}
exec - echo __NoT_DeFiNeD__.COMPONENT[VALUE] ${__NoT_DeFiNeD__.COMPONENT[VAL'\
$'UE]}

done all'
		OUTPUT - $'echo DEFINED defined
echo DEFINED:VALUE ${DEFINED:VALUE}
echo DEFINED:-VALUE ${DEFINED:-VALUE}
echo DEFINED=VALUE ${DEFINED=VALUE}
echo DEFINED[VALUE] ${DEFINED[VALUE]}
echo DEFINED.COMPONENT ${DEFINED.COMPONENT}
echo DEFINED.COMPONENT[VALUE] ${DEFINED.COMPONENT[VALUE]}
echo EMPTY 
echo EMPTY:VALUE ${EMPTY:VALUE}
echo EMPTY:-VALUE ${EMPTY:-VALUE}
echo EMPTY=VALUE ${EMPTY=VALUE}
echo EMPTY[VALUE] ${EMPTY[VALUE]}
echo EMPTY.COMPONENT ${EMPTY.COMPONENT}
echo EMPTY.COMPONENT[VALUE] ${EMPTY.COMPONENT[VALUE]}
echo __NoT_DeFiNeD__ ${__NoT_DeFiNeD__}
echo __NoT_DeFiNeD__:VALUE ${__NoT_DeFiNeD__:VALUE}
echo __NoT_DeFiNeD__:-VALUE ${__NoT_DeFiNeD__:-VALUE}
echo __NoT_DeFiNeD__=VALUE ${__NoT_DeFiNeD__=VALUE}
echo __NoT_DeFiNeD__[VALUE] ${__NoT_DeFiNeD__[VALUE]}
echo __NoT_DeFiNeD__.COMPONENT ${__NoT_DeFiNeD__.COMPONENT}
echo __NoT_DeFiNeD__.COMPONENT[VALUE] ${__NoT_DeFiNeD__.COMPONENT[VALUE]}'
		ERROR - $'\n# ._tmp_Mamfile_: 4-30: make all'

TEST 04 'nested loop with notrace'

	EXEC	-f ._tmp_Mamfile_
		INPUT -n -
		INPUT ._tmp_Mamfile_ $'setv MAMAKE_STRICT
make looptest virtual
\tloop v1 one two three
\t\tloop v2 four five six
\t\t\tmake ${v1}_${v2} virtual notrace
\t\t\t\texec - echo \'${v1}: ${v2}\'
\t\t\tdone
\t\tdone
\tdone
\texec - : the current target is ${@}
\texec - : the last target was ${<}
\texec - : the targets were ${^}
\texec - : the updated targets were ${?}
done'
		OUTPUT - $'one: four
one: five
one: six
two: four
two: five
two: six
three: four
three: five
three: six'
		ERROR - $'
# ._tmp_Mamfile_: 5-7: make one_four

# ._tmp_Mamfile_: 5-7: make one_five

# ._tmp_Mamfile_: 5-7: make one_six

# ._tmp_Mamfile_: 5-7: make two_four

# ._tmp_Mamfile_: 5-7: make two_five

# ._tmp_Mamfile_: 5-7: make two_six

# ._tmp_Mamfile_: 5-7: make three_four

# ._tmp_Mamfile_: 5-7: make three_five

# ._tmp_Mamfile_: 5-7: make three_six

# ._tmp_Mamfile_: 2-14: make looptest
+ : the current target is looptest
+ : the last target was three_six
+ : the targets were one_four one_five one_six two_four two_five two_six thr'\
$'ee_four three_five three_six
+ : the updated targets were one_four one_five one_six two_four two_five two'\
'_six three_four three_five three_six'

TEST 05 $'shim, \'exec\' in loop'

	EXEC	-f ._tmp_Mamfile_
		INPUT -n -
		INPUT ._tmp_Mamfile_ $'setv MAMAKE_STRICT 2
setv _iter_ vertier
shim - echo KIJK EENS HIER >&2
make shimtest virtual notrace
\tmake looptest virtual notrace
\t\tloop _iter_ een twee drie vier
\t\t\texec - echo ${_iter_} hoedje${_iter_?een??s} van papier >&2
\t\tdone
\tdone
\texec - echo "nu is \'t gedaan met \'t ${_iter_}" >&2
done'
		OUTPUT -
		ERROR - $'
# ._tmp_Mamfile_: 5-9: make looptest
KIJK EENS HIER
een hoedje van papier
twee hoedjes van papier
drie hoedjes van papier
vier hoedjes van papier

# ._tmp_Mamfile_: 4-11: make shimtest
KIJK EENS HIER
nu is \'t gedaan met \'t vertier'

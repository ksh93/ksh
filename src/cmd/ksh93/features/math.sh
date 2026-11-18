########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1982-2013 AT&T Intellectual Property          #
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
#         hyenias <58673227+hyenias@users.noreply.github.com>          #
#                  Lev Kujawski <int21h@mailbox.org>                   #
#            Johnothan King <johnothanking@protonmail.com>             #
#                                                                      #
########################################################################
# generate the ksh math builtin table
# include math.tab

# @(#)math.sh (ksh 93u+m) 2026-11-18

case $ZSH_VERSION in
?*)	emulate ksh ;;
*)	(command set -o posix) 2>/dev/null && set -o posix ;;
esac
set -o noglob

command=$0
iffeflags="$IFFEFLAGS -n -v"
iffehdrs="ast_float.h"
iffelibs="-lm"
table=/dev/null

eval $1
shift
table=$1

# iffe(1) detects long double only if sizeof(long double) > sizeof(double).
eval $(iffe $iffeflags -c "$cc" - typ long.double 2>&$stderr)

eval $(iffe $iffeflags -c "$cc" - tst use_ieeefp -lm 'note{' 'ieeefp.h plays nice' '}end' \
	'link{' \
		'#include <math.h>' \
		'#include <ieeefp.h>' \
		'int main(void) { return 0; }' \
	'}end')
case $_use_ieeefp in
1)	iffehdrs="$iffehdrs ieeefp.h" ;;
esac

# C90 standard math functions need no probing; they are guaranteed to exist and
# to be declared in math.h as functions. Their long double variants are C99 and
# are probed as usual; we do not assume a C99-compliant system library yet.

c90='acos asin atan atan2 ceil cos cosh exp fabs floor fmod ldexp log log10 pow sin sinh sqrt tan tanh'
for name in $c90
do	eval _lib_$name=1 _mac_$name= _npt_$name=
done

# read the table

exec < $table
ifs=$IFS
libs=
names=
nums=
while	read type args name aka
do	case $type in
	[fix])	names="$names $name"
		case " $c90 " in
		*" $name "*)
			;;  # C90, see above
		*)	libs="$libs,$name" ;;
		esac
		case $_typ_long_double in
		1)	libs="$libs,${name}l" ;;
		esac
		for a in $aka
		do	case $a in
			'{'*)	break
				;;
			*=*)	IFS='=|'
				set $a
				IFS=$ifs
				case ",$libs" in
				*,$1,*)	;;
				*)	names="$names $1"
					case " $c90 " in
					*" $1 "*)	;;
					*)		libs="$libs,$1"
					esac
					case $_typ_long_double in
					1)	libs="$libs,${1}l" ;;
					esac
					;;
				esac
				shift
				while	:
				do	case $# in
					0)	break ;;
					esac
					case ",$nums" in
					*,$1,*)	;;
					*)	nums="$nums,$1" ;;
					esac
					shift
				done
				;;
			esac
		done
		eval TYPE_$name='$type' ARGS_$name='$args' AKA_$name='$aka'
		;;
	esac
done

# Check the math library.

# Testing each name separately means compiling and linking a test program for
# each one. So test the existence of all ordinary floating-point functions in
# one link test instead: if that succeeds they all exist, and the results can
# simply be set. If it fails, split the list in half and test each half, and
# so on recursively, until the failures are pinned down to a small group; the
# names in that group are then probed one by one below. A system that has them
# all thus needs one test, and one that lacks a few needs a few tests more,
# instead of one per function.

batch=
librest=
for name in $names
do	eval t='$'TYPE_$name
	case $t in
	f|x)	case $name in
		float|int)	continue ;;	# ksh pseudo-functions, not library symbols
		esac
		case ",$libs," in
		*,$name,*)	batch="$batch $name" ;;
		*,$name"l",*)	batch="$batch ${name}l" ;;
		esac
		;;
	*)	case ",$libs," in
		*,$name,*)	librest="$librest,$name" ;;
		*,$name"l",*)	librest="$librest,${name}l" ;;
		esac
		;;
	esac
done

# Each name is declared as an old-style function returning int so that it can
# simply be named in the test program and resolved by the linker. A declaration
# without a parameter list is valid C and is all that is needed here, as the
# functions are never called; but compilers that know these names as builtins
# warn about the mismatched declaration, which is pure noise, so silence that.

pragmas='#if	defined(__clang__)
#pragma clang diagnostic ignored "-Wincompatible-library-redeclaration"
#elif	defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"
#endif'

# Each function is probed through a volatile pointer so that the compiler
# cannot optimise away entries it thinks are unused: every name must actually
# be referenced at link time, or a missing function would go unnoticed.

linkmath()	# [ name ... ]
{
	test $# -gt 0 || return 1
	_decl= _init=
	for _name
	do	_decl="$_decl extern int $_name();"
		_init="$_init,$_name"
	done
	_mathlib=
	eval $(iffe $iffeflags -c "$cc" - tst mathlib 'note{' 'bisecting math functions grouped link probe' '}end' \
		'link{' \
			"${pragmas}" \
			"typedef int (*_iffe_math_fun)();" \
			"$_decl" \
			"static _iffe_math_fun _f[] = { 0$_init };" \
			"int main(void) { return *(volatile _iffe_math_fun *)_f == 0; }" \
		'}end' $iffelibs 2>&$stderr)
	if	test "$_mathlib" = 1
	then	for _name
		do	eval _lib_$_name=1
		done
		return 0
	fi
	test $# -gt 8 || return 1	# too few to be worth splitting; probed individually below
	_half=$(($# / 2))
	_a= _b=
	for _name
	do	if	test $_half -gt 0
		then	_a="$_a $_name"
			_half=$((_half - 1))
		else	_b="$_b $_name"
		fi
	done
	linkmath $_a
	linkmath $_b
}

linkmath $batch
libs=$librest
for name in $batch
do	eval t='$'_lib_$name
	test "$t" = 1 || libs="$libs,$name"
done

eval $(iffe $iffeflags -c "$cc" - lib $libs $iffehdrs $iffelibs 2>&$stderr)
lib=
for name in $names
do	eval x='$'_lib_${name}l y='$'_lib_${name}
	case $x in
	1)	lib="$lib,${name}l" ;;
	esac
	case $y in
	1)	case $x in
		'')	case " $c90 " in
			*" $name "*)	;;	# C90: declared in math.h, not a macro
			*)		lib="$lib,$name" ;;
			esac
			;;
		esac
		;;
	esac
done
eval $(iffe $iffeflags -c "$cc" - npt,mac $lib $iffehdrs $iffelibs 2>&$stderr)
eval $(iffe $iffeflags -c "$cc" - num $nums $iffehdrs $iffelibs 2>&$stderr)

# for privacy and brevity, strip leading $PACKAGEROOT/ from header comment
cat <<!
#if __clang__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

/* : : generated by $command from ${table#"${PACKAGEROOT:-/dev/null}"[\\/]} : : */

typedef Sfdouble_t (*Math_f)(Sfdouble_t,...);

!
case $_use_ast_standards in
1)	echo "#include <ast_standards.h>" ;;
esac
echo "#include <math.h>"
case $_hdr_ieeefp in
1)	echo "#include <ieeefp.h>" ;;
esac
cat <<!
#include <ast_float.h>
#if defined(__ia64__) && defined(signbit)
# if defined __GNUC__ && __GNUC__ >= 4
#  define __signbitl(f)		__builtin_signbitl(f)
# else
#  if _lib_copysignl
#   define __signbitl(f)	(int)(copysignl(1.0,(f))<0.0)
#  endif
# endif
#endif
!
echo

# generate the intercept functions and table entries

nl='
'
ht='	'
tab=
for name in $names
do	eval x='$'_lib_${name}l y='$'_lib_${name} r='$'TYPE_${name} a='$'ARGS_${name} aka='$'AKA_${name}
	case $r in
	i)	L=int R=1 ;;
	x)	L=Sfdouble_t R=4 ;;
	*)	L=Sfdouble_t R=0 ;;
	esac
	# some identifiers can't be functions in C but can in ksh #
	case $name in
	float|int)	x=0 y=1 ;;
	esac
	F=local_$name
	case $x:$y in
	1:*)	f=${name}l
		t=Sfdouble_t
		local=
		;;
	*:1)	f=${name}
		t=double
		case $name in
		float|int)	local=1 ;;
		*)		local=$_typ_long_double ;;
		esac
		;;
	*)	body=
		for k in $aka
		do	case $body in
			?*)	body="$body $k"
				continue
				;;
			esac
			case $k in
			'{'*)	body=$k
				;;
			*=*)	IFS='=|'
				set $k
				IFS=$ifs
				f=$1
				shift
				v=$*
				eval x='$'_lib_${f}l y='$'_lib_${f}
				case $x:$y in
				1:*)	f=${f}l
					;;
				*:1)	;;
				*)	continue
					;;
				esac
				y=
				while	:
				do	case $# in
					0)	break ;;
					esac
					eval x='$'_num_$1
					case $x in
					1)	case $y in
						?*)	y="$y || " ;;
						esac
						y="${y}q == $1"
						;;
					esac
					shift
				done
				case $y in
				'')	;;
				*)	r=int R=1
					echo "static $r $F(Sfdouble_t a1) { $r q = $f(a1); return $y; }"
					tab="$tab$nl$ht\"\\0${R}${a}${name}\",$ht(Math_f)${F},"
					break
					;;
				esac
				;;
			esac
		done
		case $body in
		?*)	code="static $L $F("
			sep=
			ta=
			tc=
			td=
			for p in 1 2 3 4 5 6 7 8 9
			do	case $R:$p in
				4:2)	T=int ;;
				*)	T=Sfdouble_t ;;
				esac
				code="$code${sep}$T a$p"
				ta="$ta${sep}a$p"
				tc="$tc${sep}0"
				td="${td}$T a$p;"
				case $a in
				$p)	break ;;
				esac
				sep=","
			done
			_it_links_=0
			eval $(iffe $iffeflags -c "$cc" - tst it_links_ 'note{' "$F function links" '}end' \
				'link{' \
					"static $L $F($ta)$td${body}int main(void){return $F($tc)!=0;}" \
				'}end' sfio.h $iffehdrs $iffelibs 2>&$stderr)
			case $_it_links_ in
			1)	code="$code)$body"
				echo "$code"
				tab="$tab$nl$ht\"\\0${R}${a}${name}\",$ht(Math_f)${F},"
				;;
			esac
			;;
		esac
		continue
		;;
	esac
	case $r in
	i)	r=int ;;
	*)	r=$t ;;
	esac
	eval n='$'_npt_$f m='$'_mac_$f d='$'_dat_$f
	case $d:$m:$n in
	1:*:*|*:1:*)
		;;
	*:*:1)	code="extern $r $f("
		sep=
		for p in 1 2 3 4 5 6 7
		do	case $p:$f in
			2:ldexp*)	code="$code${sep}int" ;;
			*)		code="$code${sep}$t" ;;
			esac
			case $a in
			$p)	break ;;
			esac
			sep=","
		done
		code="$code);"
		echo "$code"
		;;
	esac
	case $local:$m:$n:$d in
	1:*:*:*|*:1:*:*|*:*:1:)
		args=
		code="static $L local_$f("
		sep=
		for p in 1 2 3 4 5 6 7 8 9
		do	args="$args${sep}a$p"
			case $R:$p in
			4:2)	T=int ;;
			*)	T=Sfdouble_t ;;
			esac
			code="$code${sep}$T a$p"
			case $a in
			$p)	break ;;
			esac
			sep=","
		done
		code="$code)"

		# catch functions not in the math lib here #

		case $f in
		float)	code="$code{return $args;}" ;;
		int)	code="$code{return ($args < LDBL_LLONG_MIN || $args > LDBL_ULLONG_MAX) ? (Sfdouble_t)0 : "
			code="$code($args < 0) ? (Sfdouble_t)((Sflong_t)$args) : (Sfdouble_t)((Sfulong_t)$args);}" ;;
		*)	code="$code{return $f($args);}" ;;
		esac
		echo "$code"
		f=local_$f
		;;
	esac
	for x in $name $aka
	do	case $x in
		'{'*)	break
			;;
		*=*)	continue
			;;
		esac
		tab="$tab$nl$ht\"\\0${R}${a}${x}\",$ht(Math_f)$f,"
	done
done
tab="$tab$nl$ht\"\",$ht${ht}NULL"

cat <<!

/*
 * first byte is two-digit octal number.  Last digit is number of args
 * first digit is 0 if return value is double, 1 for integer
 */
const struct mathtab shtab_math[] =
{$tab
};
!

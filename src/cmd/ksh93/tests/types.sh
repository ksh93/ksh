########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1982-2012 AT&T Intellectual Property          #
#          Copyright (c) 2020-2024 Contributors to ksh 93u+m           #
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

integer n=2

typeset -T Type_t=(
	typeset name=foobar
	typeset x=(hi=ok bar=yes)
	typeset y=(xa=xx xq=89)
	typeset -A aa=([one]=abc [two]=def)
	typeset -a ia=(abc def)
	typeset -i z=5
)
for ((i=0; i < 10; i++))
do
	Type_t r s
	[[ $r == "$s" ]] || err_exit 'r is not equal to s'
	typeset -C x=r.x
	y=(xa=bb xq=cc)
	y2=xyz
	z2=xyz
	typeset -C z=y
	[[ $y == "$z" ]] || err_exit 'y is not equal to z'
	typeset -C s.y=z
	[[ $y == "${s.y}" ]] || err_exit 'y is not equal to s.y'
	.sh.q=$y
	typeset -C www=.sh.q
	[[ $www == "$z" ]] || err_exit 'www is not equal to z'
	typeset -C s.x=r.x
	[[ ${s.x} == "${r.x}" ]] || err_exit 's.x is not equal to r.x'

	function foo
	{
		nameref x=$1 y=$2
		typeset z=$x
		y=$x
		[[ $x == "$y" ]] || err_exit "x is not equal to y with ${!x}"
	}
	foo r.y y
	[[ $y == "${r.y}" ]] || err_exit 'y is not equal to r.y'
	typeset -C y=z
	foo y r.y
	[[ $y == "${r.y}" ]] || err_exit 'y is not equal to r.y again'
	typeset -C y=z
	(
		q=${z}
		[[ $q == "$z" ]] || err_exit 'q is not equal to z'
		z=abc
	)
	[[ $z == "$y" ]] || err_exit 'value of z not preserved after subshell'
	unset z y r s x z2 y2 www .sh.q
done
typeset -T Frame_t=( typeset file lineno )
Frame_t frame
[[ $(typeset -p frame) == 'Frame_t frame=(typeset file;typeset lineno)' ]] || err_exit 'empty fields in type not displayed'
x=( typeset -a arr=([2]=abc [4]=(x=1 y=def));zz=abc)
typeset -C y=x
[[ "$x" == "$y" ]] || err_exit "y is not equal to x (y == $(printf %q "$y"); x == $(printf %q "$x"))"
Type_t z=(y=(xa=bb xq=cc))
typeset -A arr=([foo]=one [bar]=2)
typeset -A brr=([foo]=one [bar]=2)
[[ "${arr[@]}" == "${brr[@]}" ]] || err_exit 'arr is not brr'
for ((i=0; i < 1; i++))
do	typeset -m zzz=x
	[[ $zzz == "$y" ]] || err_exit 'zzz is not equal to y'
	typeset -m x=zzz
	[[ $x == "$y" ]] || err_exit 'x is not equal to y'
	Type_t t=(y=(xa=bb xq=cc))
	typeset -m r=t
	[[ $r == "$z" ]] || err_exit 'r is not equal to z'
	typeset -m t=r
	[[ $t == "$z" ]] || err_exit 't is not equal to z'
	typeset -m crr=arr
	[[ "${crr[@]}" == "${brr[@]}" ]] || err_exit 'crr is not brr'
	typeset -m arr=crr
	[[ "${arr[@]}" == "${brr[@]}" ]] || err_exit 'brr is not arr'
done
typeset -m brr[foo]=brr[bar]
[[ ${brr[foo]} == 2 ]] || err_exit 'move an associative array element fails'
[[ ${brr[bar]} ]] && err_exit 'brr[bar] should be unset after move'
unset x y zzz
x=(a b c)
typeset -m x[1]=x[2]
[[ ${x[1]} == c ]] || err_exit 'move an indexed array element fails'
[[ ${x[2]} ]] && err_exit 'x[2] should be unset after move'
cat > $tmp/types <<- \+++
	typeset -T Pt_t=(float x=1. y=0.)
	Pt_t p=(y=2)
	print -r -- ${p.y}
+++
expected=2
got=$(. $tmp/types) 2>/dev/null
[[ "$got" == "$expected" ]] || err_exit "typedefs in dot script failed -- expected '$expected', got '$got'"
typeset -T X_t=(
	typeset x=foo y=bar
	typeset s=${_.x}
	create()
	{
		_.y=bam
	}
)
X_t x
[[ ${x.x} == foo ]] || err_exit 'x.x should be foo'
[[ ${x.y} == bam ]] || err_exit 'x.y should be bam'
[[ ${x.s} == ${x.x} ]] || err_exit 'x.s should be x.x'
typeset -T Y_t=( X_t r )
Y_t z
[[ ${z.r.x} == foo ]] || err_exit "z.r.x should be foo (got $(printf %q "${z.r.x}"))"
[[ ${z.r.y} == bam ]] || err_exit "z.r.y should be bam (got $(printf %q "${z.r.y}"))"
[[ ${z.r.s} == ${z.r.x} ]] || err_exit "z.r.s should be z.r.x (expected $(printf %q "${z.r.x}"), got $(printf %q "${z.r.s}"))"

unset xx yy
typeset -T xx=(typeset yy=zz)
xx=yy
{ typeset -T xx=(typeset yy=zz) ;} 2>/dev/null && err_exit 'type redefinition should fail'
$SHELL 2> /dev/null <<- +++ || err_exit 'typedef with only f(){} fails'
	typeset -T X_t=(
		f()
		{
			print ok
		}
	)
+++
$SHELL 2> /dev/null <<- +++ || err_exit 'unable to redefine f discipline function'
	typeset -T X_t=(
		x=1
		f()
		{
			print ok
		}
	)
	X_t z=(
		function f
		{
			print override f
		}
	)
+++
$SHELL 2> /dev/null <<- +++ && err_exit 'invalid discipline name should be an error'
	typeset -T X_t=(
		x=1
		f()
		{
			print ok
		}
	)
	X_t z=(
		function g
		{
			print override f
		}
	)
+++
# compound variables containing type variables
Type_t r
var=(
	typeset x=foobar
	Type_t	r
	integer z=5
)
[[ ${var.r} == "$r" ]] || err_exit 'var.r != r'
(( var.z == 5)) || err_exit 'var.z !=5'
[[ "$var" == *x=foobar* ]] || err_exit '$var does not contain x=foobar'

typeset -T A_t=(
	typeset x=aha
	typeset b=${_.x}
)
unset x
A_t x
expected=aha
got=${x.b}
[[ "$got" == "$expected" ]] || err_exit "type '_' reference failed -- expected '$expected', got '$got'"

typeset -T Tst_t=(
	 function f
	 {
	 	A_t a
	 	print ${ _.g ${a.x}; }
	 }
	 function g
	 {
	 	print foo
	 }
)
Tst_t tst
expected=foo
got=${ tst.f;}
[[ "$got" == "$expected" ]] || err_exit "_.g where g is a function in type discipline method failed -- expected '$expected', got '$got'"

typeset -T B_t=(
	integer -a arr
	function f
	{
		(( _.arr[0] = 0 ))
		(( _.arr[1] = 1 ))
		print ${_.arr[*]}
	}
)
unset x
B_t x
expected='0 1'
got=${ x.f;}
[[ "$got" == "$expected" ]] || err_exit "array assignment of subscripts in type discipline arithmetic failed -- expected '$expected', got '$got'"

typeset -T Fileinfo_t=(
	size=-1
	typeset -a text=()
	integer mtime=-1
)
Fileinfo_t -A _Dbg_filenames
Fileinfo_t finfo
function bar
{
	finfo.text=(line1 line2 line3)
	finfo.size=${#finfo.text[@]}
	_Dbg_filenames[foo]=finfo
}
bar

expected='Fileinfo_t -A _Dbg_filenames=([foo]=(size=3;typeset -a text=(line1 line2 line3);typeset -l -i mtime=-1))'
got=$(typeset -p _Dbg_filenames)
[[ "$got" == "$expected" ]] || {
	got=$(printf %q "$got")
	err_exit "copy to associative array of types in function failed -- expected '$expected', got $got"
}

$SHELL > /dev/null  <<- '+++++' || err_exit 'passing _ as nameref arg not working'
	function f1
	{
	 	typeset -n v=$1
	 	print -r -- "$v"
	}
	typeset -T A_t=(
 		typeset blah=xxx
	 	function f { f1 _ ;}
	)
	A_t a
	[[ ${ a.f ./t1;} == "$a" ]]
+++++
expected='A_t b.a=(name=one)'
[[ $( $SHELL << \+++
	typeset -T A_t=(
	     typeset name=aha
	)
	typeset -T B_t=(
	 	typeset     arr
	 	A_t         a
	 	f()
	 	{
	 		_.a=(name=one)
	 		typeset -p _.a
	 	}
	)
	B_t b
	b.f
+++
) ==  "$expected" ]] 2> /dev/null || err_exit  '_.a=(name=one) not expanding correctly'
expected='A_t x=(name=xxx)'
[[ $( $SHELL << \+++
	typeset -T A_t=(
		typeset name
	)
	A_t x=(name="xxx")
	typeset -p x
+++
) ==  "$expected" ]] || err_exit  'empty field in definition does not expand correctly'

typeset -T Foo_t=(
	integer x=3
	integer y=4
	len() { print -r -- $(( sqrt(_.x**2 + _.y**2))) ;}
)
Foo_t foo
[[ ${foo.len} == 5 ]] || err_exit "discipline function len not working"

typeset -T benchmark_t=(
	integer num_iterations
)
function do_benchmarks
{
	nameref tst=b
	integer num_iterations
	(( num_iterations= int(tst.num_iterations * 1.0) ))
	printf "%d\n" num_iterations
}
benchmark_t b=(num_iterations=5)
[[  $(do_benchmarks) == 5 ]] || err_exit 'scoping of nameref of type variables in arithmetic expressions not working'

function cat_content
{
	cat <<- EOF
	(
		foo_t -a foolist=(
			( val=3 )
			( val=4 )
			( val=5 )
		)
	)
	EOF
	return 0
}
typeset -T foo_t=(
	integer val=-1
	function print
	{
		print -- ${_.val}
	}
)
function do_something
{
	nameref li=$1 # "li" may be an index or associative array
	li[2].print
}
cat_content | read -C x
[[ $(do_something x.foolist) == 5  ]] || err_exit 'subscripts not honored for arrays of type with disciplines'

typeset -T benchcmd_t=(
	float x=1
	float y=2
)
unset x
compound x=(
	float o
	benchcmd_t -a m
	integer h
)
expected=$'(\n\ttypeset -l -i h=0\n\tbenchcmd_t -a m\n\ttypeset -l -E o=0\n)'
[[ $x == "$expected" ]] || err_exit 'compound variable with array of types with no elements not working'

expected=$'Std_file_t db.file[/etc/profile]=(action=preserve;typeset -A sum=([8242e663d6f7bb4c5427a0e58e2925f3]=1);)'
{
  got=$($SHELL <<- \EOF
	MAGIC='stdinstall (AT&T Research) 2009-08-25'
	typeset -T Std_file_t=(
		typeset action
		typeset -A sum
	)
	typeset -T Std_t=(
		typeset magic=$MAGIC
		Std_file_t -A file
	)
	Std_t db=(magic='stdinstall (AT&T Research) 2009-08-25';Std_file_t -A file=( [/./home/gsf/.env.sh]=(action=preserve;typeset -A sum=([9b67ab407d01a52b3e73e3945b9a3ee0]=1);)[/etc/profile]=(action=preserve;typeset -A sum=([8242e663d6f7bb4c5427a0e58e2925f3]=1);)[/home/gsf/.profile]=(action=preserve;typeset -A sum=([3ce23137335219672bf2865d003a098e]=1);));)
	typeset -p db.file[/etc/profile]
	EOF)
} 2> /dev/null
[[ $got == "$expected" ]] ||  err_exit 'types with arrays of types as members fails'

typeset -T x_t=(
	integer dummy
	function set
	{
		[[ ${.sh.name} == v ]] || err_exit  "name=${.sh.name} should be v"
		[[ ${.sh.subscript} == 4 ]] || err_exit "subscript=${.sh.subscript} should be 4"
		[[ ${.sh.value} == hello ]] || err_exit  "value=${.sh.value} should be hello"
	}
)
x_t -a v
v[4]="hello"

typeset -T oset=(
    typeset -A s
)
oset foo bar
: ${foo.s[a]:=foobar}
: ${bar.s[d]:=foobar}
[[ ${bar.s[a]} == foobar ]] && err_exit '${var:=val} for types assigns to type instead of type instance'

typeset -T olist=(
    typeset -a l
)
olist foo
foo.l[1]=x
[[  ${!foo.l[*]} == *0* ]] && '0-th element of foo.l should not be set'

typeset -T oset2=( typeset -A foo )
oset2 bar
: ${bar.foo[a]}
bar.foo[a]=b
[[ ${#bar.foo[*]} == 1 ]] || err_exit "bar.foo should have 1 element not  ${#bar.foo[*]}"
[[ ${bar.foo[*]} == b ]] || err_exit "bar.foo[*] should be 'b'  not  ${bar.foo[*]}"
[[ ${bar.foo[a]} == b ]] || err_exit "bar.foo[a] should be 'b'  not  ${bar.foo[*]}"

{ x=$( $SHELL 2> /dev/null << \++EOF++
    typeset -T ab_t=(
	integer a=1 b=2
	function increment
	{
		(( _.a++, _.b++ ))
	}
    )
    function ar_n
    {
	nameref sn=$2
	sn.increment
	$1 && printf "a=%d, b=%d\n" sn.a sn.b
    }
    function ar
    {
	ab_t -S -a s
	[[ -v s[5] ]] || s[5]=( )
	ar_n $1 s[5]
    }
    x=$(ar false ; ar false ; ar true ; printf ";")
    y=$(ar false ; ar false ; ar true ; printf ";")
    print -r -- "\"$x\"" ==  "\"$y\""
++EOF++
) ;} 2> /dev/null
[[ $x == *a=4*b=5* ]] || err_exit 'static types in a function not working'
{ eval "[[ $x ]]";} 2> /dev/null || err_exit 'arrays of types leaving side effects in subshells'

typeset -T y_t=(
	typeset dummy
	function print_b
	{
		print "B"
	}
)
y_t a b=(
	function print_b
	{
		print "1"
	}
)
[[ $(a.print_b) == B ]] || err_exit 'default discipline not working'
[[ $(b.print_b) == 1 ]] || err_exit 'discipline override not working'

$SHELL 2> /dev/null -c 'true || { typeset -T Type_t=(typeset name=foo);
	Type_t z=(name=bar) ;}' || err_exit 'unable to parse type command until typeset -T executes'

cd "$tmp"
FPATH=$PWD
PATH=$PWD:$PATH
cat > A_t <<-  \EOF
	if	false
	then	typeset -T Parser_shenanigans=(typeset -i foo)
	fi
	typeset -T A_t=(
		B_t b
	)
EOF
cat > B_t <<-  \EOF
	PATH=/dev/null command -v Parser_shenanigans
	typeset -T B_t=(
		integer n=5
	)
EOF

unset n
if	n=$(set +x; FPATH=$PWD PATH=$PWD:$PATH "$SHELL" -c 'A_t a; print ${a.b.n}' 2>&1)
then	[[ $n == '5' ]] || err_exit "dynamic loading of types gives wrong result (got $(printf %q "$n"))"
else	err_exit "unable to load types dynamically (got $(printf %q "$n"))"
fi

# check that typeset -T reproduces a type.
if	$SHELL  > /dev/null 2>&1  -c 'typeset -T'
then	$SHELL > junk1 <<- \+++EOF
		typeset -T foo_t=(
			integer x=3 y=4
			float z=1.2
			len()
			{
				((.sh.value=sqrt(_.x**2 + _.y**2) ))
			}
			function count
			{
				print z=$z
			}
		)
		typeset -T
		print 'typeset -T'
	+++EOF
	$SHELL -c '. ./junk1;print "typeset -T"' > junk2
	diff junk[12] > /dev/null || err_exit 'typeset -T not idempotent'
	$SHELL -c '. ./junk1;print "typeset +f"' > junk2
	[[ -s junk2 ]] || err_exit 'non-discipline-method functions found'
else
	err_exit 'typeset -T not supported'
fi

[[ $($SHELL -c 'typeset -T x=( typeset -a h ) ; x j; print -v j.h') ]] && err_exit 'type with indexed array without elements inserts element 0'

[[ $($SHELL  -c 'typeset -T x=( integer -a s ) ; compound c ; x c.i ; c.i.s[4]=666 ; print -v c') == *'[0]'* ]] &&  err_exit 'type with indexed array with non-zero element inserts element 0'


{ $SHELL -c '(sleep 3;kill $$)& typeset -T x=( typeset -a s );compound c;x c.i;c.i.s[7][5][3]=hello;x c.j=c.i;[[ ${c.i} == "${c.j}" ]]';} 2> /dev/null
exitval=$?
if	[[ $(kill -l $exitval) == TERM ]]
then	err_exit 'clone of multidimensional array timed out'
elif	((exitval))
then	err_exit "c.i and c.j are not the same multidimensional array"
fi

typeset -T foobar_t=(
	float x=1 y=0
	slen()
	{
		print -r -- $((sqrt(_.x**2 + _.y**2)))
	}
	typeset -fS slen
	len()
	{
		print -r -- $((sqrt(_.x**2 + _.y**2)))
	}
)
unset z
foobar_t z=(x=3 y=4)
(( z.len == 5 )) || err_exit 'z.len should be 5'
(( z.slen == 1 )) || err_exit 'z.slen should be 1'
(( .sh.type.foobar_t.slen == 1 )) || err_exit '.sh.type.foobar_t.slen should be 1'
(( .sh.type.foobar_t.len == 1 )) || err_exit '.sh.type.foobar_t.len should be 1'

typeset -T z_t=( typeset -a ce )
z_t x1
x1.ce[3][4]=45
compound c
z_t -a c.x2
c.x2[9]=x1
got=$(typeset +p "c.x2[9].ce")
exp='typeset -a c.x2[9].ce'
[[ $got == "$exp" ]] || err_exit "typeset +p 'c.x2[9].ce' failed -- expected '$exp', got '$got'"

unset b
typeset -T a_t=(
	typeset a="hello"
)
typeset -T b_t=(
	a_t b
)
compound b
compound -a b.ca
b_t b.ca[4].b
exp='typeset -C b=(typeset -C -a ca=( [4]=(b_t b=(a_t b=(a=hello))));)'
got=$(typeset -p b)
[[ $got == "$exp" ]] || err_exit 'typeset -p of nested type not correct' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

typeset -T u_t=(
	integer dummy
	unset()
	{
		print unset
	}
)
unset z
u_t -a x | read z
[[ $z == unset ]]  && err_exit 'unset discipline called on type creation'

got=$("$SHELL" -c 'typeset -T foo; typeset -T' 2>&1)
[[ -z $got ]] || err_exit '"typeset -T foo; typeset -T" exposed incomplete type builtin' \
	"(got $(printf %q "$got"))"

{ z=$($SHELL 2> /dev/null 'typeset -T foo=bar; typeset -T') ;} 2> /dev/null
[[ $z ]] && err_exit '"typeset -T foo=bar" should not creates type foo'

{
$SHELL << \EOF
	typeset -T board_t=(
		compound -a board_y
		function binsert
		{
			nameref figure=$1
			integer y=$2 x=$3
			typeset -m "_.board_y[y].board_x[x].field=figure"
		}
	)
	function main
	{
		compound c=(
			 board_t b
		)
		for ((i=0 ; i < 2 ; i++ )) ; do
			 compound p=( hello=world )
			 c.b.binsert p 1 $i
		done
		exp='typeset -C c=(board_t b=(typeset -C -a board_y=( [1]=(typeset -a board_x=( [0]=(field=(hello=world;))[1]=(field=(hello=world)));));))'
		[[ $(typeset -p c) == "$exp" ]] || exit 1
	}
	main
EOF
}
if	(( exitval=$?))
then	if	((exitval>128))
	then	err_exit "typeset -m in type discipline crashed with SIG$(kill -l $exitval)"
	else	err_exit 'typeset -m in type discipline gives wrong value'
	fi
fi

typeset -T pawn_t=(
	print_debug()
	{
		print 'PAWN'
	}
)
function main
{
	compound c=(
		compound -a board
	)

	for ((i=2 ; i < 8 ; i++ )) ; do
		pawn_t c.board[1][$i]
	done
}
main 2> /dev/null && err_exit 'type assignment to compound array instance should generate an error'

{	$SHELL -c 'typeset -T Foo_t=(integer -a data=([0]=0) );Foo_t x=(data[0]=2);((x.data[0]==2))'
} 2> /dev/null || err_exit 'type definition with integer array variable not working'

typeset -T Bar_t=(
	typeset -a foo
)
Bar_t bar
bar.foo+=(bam)
[[ ${bar.foo[0]} == bam ]] || err_exit 'appending to empty array variable in type does not create element 0'

# ======
# Compound arrays listed with 'typeset -p' should have their -C attribute
# preserved in the output.
typeset -T Z_t=(compound -a x)
Z_t z
[[ $(typeset -p z.x) ==  *'-C -a'* ]] || err_exit 'typeset -p for compound array element does not display all attributes'

# ======
# Type names that have 'a' as the first letter should be functional
"$SHELL" -c 'typeset -T al=(typeset bar); al foo=(bar=testset)' || err_exit "type names that start with 'a' don't work"

# ======
# 'typeset -T' shouldn't list types created by enum
got=$($SHELL -c 'enum Foo_t=(foo bar); typeset -T')
[[ -z $got ]] || err_exit "Types created by enum are listed with 'typeset -T'" \
	"(got $(printf %q "$got"))"

# ======
# Parser shenanigans.
if	false
then	typeset -T PARSER_t=(typeset name=foobar)
fi
PATH=/dev/null command -v PARSER_t >/dev/null && err_exit "PARSER_t incompletely defined though definition was never executed"
(PATH=/dev/null eval 'PARSER_t x=(name=y)') 2>/dev/null
(($?==3)) || err_exit "PARSER_t assignment not a syntax error though definition was never executed"

unset v
got=$( set +x; redirect 2>&1; typeset -T Subsh_t=(typeset -i x); Subsh_t -a v=( (x=1) (x=2) (x=3) ); typeset -p v )
exp='Subsh_t -a v=((typeset -i x=1) (typeset -i x=2) (typeset -i x=3))'
[[ $got == "$exp" ]] || err_exit "bad typeset output for Subsh_t" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
PATH=/dev/null command -v Subsh_t >/dev/null && err_exit "Subsh_t leaked out of subshell"

# ======
# https://github.com/ksh93/ksh/issues/350#issuecomment-982168684
got=$("$SHELL" -c 'typeset -T trap=( typeset -i i )' 2>&1)
exp=': trap: is a special shell builtin'
[[ $got == *"$exp" ]] || err_exit "typeset -T overrides special builtin" \
	"(expected match of *$(printf %q "$exp"); got $(printf %q "$got"))"

# ======
# Bugs involving scripts without a #! path
# Hashbangless scripts are executed in a reinitialised fork of ksh.
# https://github.com/ksh93/ksh/issues/350
# Some of these fixed bugs don't involve types at all, but the tests need to go somewhere.
# Plus, invoking these from an environment with a bunch of types defined is an additional test.
: >|foo1
: >|foo2
chmod +x foo1 foo2

enum _foo1_t=(VERY BAD TYPE)
_foo1_t foo=BAD
normalvar=BAD2
cat >|foo1 <<-'EOF'
	# no hashbang path here
	echo "normalvar=$normalvar"
	echo "foo=$foo"
	PATH=/dev/null
	typeset -p .sh.type
	_foo1_t --version
EOF
./foo1 >|foo1.out 2>&1
got=$(<foo1.out)
exp=$'normalvar=\nfoo=\nnamespace sh.type\n{\n\t:\n}\n*: _foo1_t: not found'
[[ $got == $exp ]] || err_exit "types survive exec of hashbangless script" \
	"(expected match of $(printf %q "$exp"), got $(printf %q "$got"))"

if	builtin basename 2>/dev/null
then	cat >|foo1 <<-'EOF'
		PATH=/dev/null
		basename --version
	EOF
	./foo1 >|foo1.out 2>&1
	got=$(<foo1.out)
	exp=': basename: not found'
	[[ $got == *"$exp" ]] || err_exit "builtins survive exec of hashbangless script" \
		"(expected match of *$(printf %q "$exp"), got $(printf %q "$got"))"
fi

echo $'echo start1\ntypeset -p a\n. ./foo2\necho end1' >| foo1
echo $'echo start2\ntypeset -p a\necho end2' >| foo2
unset a
typeset -a a=(one two three)
export a
got=$(./foo1)
exp=$'start1\ntypeset -x a=one\nstart2\ntypeset -x a=one\nend2\nend1'
[[ $got == "$exp" ]] || err_exit 'exporting variable to #!-less script' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# Backported regression test from ksh93v- 2013-08-07 for
# short integer arrays in types.
got=$(set +x; "$SHELL" 2>&1 <<- \EOF
       typeset -T X_t=(typeset -si -a arr=(7 8) )
       X_t x
       print -r -- $((x.arr[1]))
EOF) || err_exit "short integer arrays in types fails (got exit status $?)"
exp=8
[[ $got == $exp ]] || err_exit "short integer arrays in types isn't working correctly" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# Printing .sh.type should not crash or give variables a spurious -x attribute.
# https://github.com/ksh93/ksh/issues/456

got=$("$SHELL" -c 'typeset -p .sh.type
typeset -Ttyp1 typ1=(
	function get {
	        .sh.value="'\''Sample'\''";
	}
)
typeset -p .sh.type
typ1
command typ1 var11
typ1
print $var11
print -v var11
typeset -p .sh.type' 2>&1)

exp='namespace sh.type
{
	:
}
namespace sh.type
{
	typeset -r typ1='\''Sample'\''
}
typ1 var11='\''Sample'\''
'\''Sample'\''
'\''Sample'\''
namespace sh.type
{
	typeset -r typ1='\''Sample'\''
}'

[[ $got == "$exp" ]] || err_exit "'tyeset -p .sh.type' failed" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# Check for correct error message when attempting to set a discipline function for a nonexistent type member
exp=": foo.get: cannot set discipline for undeclared type member"
got=$(set +x; redirect 2>&1; typeset -T _bad_disc_t=(typeset dummy; function foo.get { :; }); echo end_reached)
let "(e=$?)==1" && [[ $got == *"$exp" ]] || err_exit "attempt to set disc for nonexistent type member not handled correctly" \
	"(expected status 1, match of *$(printf %q "$exp"); got status $e, $(printf %q "$got"))"

# ======
# Crash due to incorrect alignment calculation
# https://github.com/ksh93/ksh/issues/537
got=$({ "$SHELL" -c '
	typeset -T Coord_t=(
		typeset -i x
	)
	typeset -T Job_t=(
		typeset -si BUGTRIGGER
		Coord_t pos
		set_pos() { _.pos.x=0 ;}
	)
	Job_t job
	job.set_pos
	job.set_pos
'; } 2>&1)
e=$?
let "e==0" || err_exit 'crash involving short int as first type member' \
	"(got status $e$( ((e>128)) && print -n /SIG && kill -l "$e"), $(printf %q "$got"))"

# ======
# loop invariants optimizer bug
# reproducer by Daniel Douglas
# https://github.com/ksh93/ksh/issues/704
typeset -T Thing=(
	integer x
	typeset y
)
function f
{
	Thing -a t=(
		[0]=(x=1; y=hi)
		[1]=(x=2; y=yo)
		[2]=(x=3; y=moo)
		[5]=(x=6; y=boo)
		[9]=(x=10; y=boom)
	)
	typeset -p t
	g t
}
function g
{
	typeset -n ref=$1 d e
	set -- "${!ref[@]}"
	set -- "${@/*/ref[\0]}"
	for e do
		for d in e.x e.y
		do	printf '%s %s %-14s %s\n' "${@e}" "${!d}" "${@d}" "${d}"
		done
	done
	echo
}
exp='Thing -a t=([0]=(typeset -l -i x=1;y=hi) [1]=(typeset -l -i x=2;y=yo) [2]=(typeset -l -i x=3;y=moo) [5]=(typeset -l -i x=6;y=boo) [9]=(typeset -l -i x=10;y=boom))
Thing t[0].x typeset -l -i  1
Thing t[0].y                hi
Thing t[1].x typeset -l -i  2
Thing t[1].y                yo
Thing t[2].x typeset -l -i  3
Thing t[2].y                moo
Thing t[5].x typeset -l -i  6
Thing t[5].y                boo
Thing t[9].x typeset -l -i  10
Thing t[9].y                boom'
got=$(f 2>&1)
[[ $got == "$exp" ]] ||
{
	err_exit "issue 704: expected '-' lines, got '+' lines:"
	diff -U1 <(print "$exp") <(print "$got") | sed $'1,3 d; s,^,\t,' >&2
}

# ======
got=$("$SHELL" -c '(typeset -T echo=(typeset TYPEVAR); echo V; print $V); echo OK' 2>&1)
exp=$'( typeset TYPEVAR )\nOK'
[[ $got == "$exp" ]] || err_exit 'type definition overriding regular built-in' \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# ======
# As of 93u+m/1.1, _ in types always refers to the type variable, even within a member discipline function.
# Change backported from ksh 93v- 2013-07-27 and 2013-08-29.
case ${.sh.version} in
Version*93u+m/1.0* | Version*93??\ * | Version*93?\ *)
	;;
*)
	typeset -T argnod_discfunc_test_t=(
		typeset x
		integer y=5
		function x.getn
		{
			((.sh.value = ++_.y))
			typeset -p _
		}
	)
	argnod_discfunc_test_t argnod_discfunc_obj
	exp=$'typeset -n _=argnod_discfunc_obj\n6'
	got=$(set +x; redirect 2>&1; print -r -- "${argnod_discfunc_obj.x}")
	[[ $got == "$exp" ]] || err_exit "_ does not refer to the type variable in a member discipline function" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
	got=$(set +x; redirect 2>&1; print -r -- "$((argnod_discfunc_obj.x))")
	[[ $got == "$exp" ]] || err_exit "_ does not refer to the type variable in a member discipline function" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
	;;
esac

# ======
exit $((Errors<125?Errors:125))

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
#                  Martijn Dekker <martijn@inlv.org>                   #
#            Johnothan King <johnothanking@protonmail.com>             #
#                                                                      #
########################################################################

# A nice default .kshrc for 'bin/package use'
# Feel free to adapt and use as your own ~/.kshrc

# Enable ** patterns
set --globstar

# Disable \ escaping annoyance if supported
# (e.g. \ breaks a subsequent arrow key).
# This will be disabled by default in 93u+m/1.1
# You can always use the 'stty lnext' character (^V).
[[ -o ?backslashctrl ]] && set --nobackslashctrl

# create a namespace for global kshrc stuff
typeset .rc

.rc.me=${0##*/}           # a $0 without dir that won't change in functions
.rc.uid=${ id -u; }
.rc.host=${ uname -n; }
.rc.host=${.rc.host%%.*}  # remove domain
.rc.tty=${ tty; }
.rc.tty=${.rc.tty#/dev/}

if	[[ -v INSTALLROOT ]]
then	# do not interfere with the regular ksh history
	[[ -d $INSTALLROOT/var ]] || mkdir -p "$INSTALLROOT/var"
	HISTFILE=$INSTALLROOT/var/sh_history_${.rc.tty//\//_}
fi

if	((.sh.version < 20230403))
then	print -r "${.rc.me}: ${.sh.version} is too old for this kshrc; reducing" >&2
	if	((.sh.version < 20220201)) # later ksh does this itself
	then	[[ ! -o vi && ! -o emacs && ! -o gmacs && -o ?emacs ]] && set --emacs
	fi
	PS1='<$?>!:${PWD#${PWD%/*/*/*}/} $ '
	return
fi

# Formatting and colour codes
.rc.fmt=(
	[reset]=$'\E[0m'
	[underline]=$'\E[4m'
	[bold]=$'\E[1m'

	[magenta]=$'\E[35m'
	[blue]=$'\E[34m'
	[cyan]=$'\E[36m'
	[red]=$'\E[31m'

	[status0]=$'\E[32m'	# zero exit status: green
	[status1]=$'\E[31m'	# nonzero exit status: red
)

# extend tilde expansion functionality:
# define some nice default shortcuts and add some warnings
function .sh.tilde.get
{
	case ${.sh.tilde} in
	'~tmp') .sh.value=${XDG_RUNTIME_DIR:-${TMPDIR:-/tmp}} ;;
	'~doc') .sh.value=~/Documents ;;
	'~ast')	[[ -v INSTALLROOT ]] && .sh.value=$INSTALLROOT ;;
	'~today')
		printf -v .sh.value "%s/Documents/Projects/%(%Y-%m-%d)T" "$HOME" "now"
		if [[ ! -d ${.sh.value} && ! -L ${.sh.value} ]]; then
			printf '%s: creating directory %q\n' "${.rc.me}" "${.sh.value}" >&2
			mkdir -p "${.sh.value}"
		fi ;;
	\~*[![:alnum:]._-]*)
		printf '%s: warning: %s: bad username\n' "${.rc.me}" "${.sh.tilde#\~}" >&2 ;;
	\~*)	eval ".sh.value=${.sh.tilde}"  # perform default expansion
		if [[ ${.sh.value} == "${.sh.tilde}" ]]; then
			printf '%s: warning: %s: unknown username\n' "${.rc.me}" "${.sh.tilde#\~}" >&2
		fi ;;
	esac
}

# RELPWD expands to relative current working directory with ~ support
# and configurable number of pathname components kept
function RELPWD.get
{
	typeset del ellip=$'\u2026' v=$PWD keep=${RELPWD.keep}
	((${#ellip}==1)) || ellip='...'
	[[ ($v == "$HOME" || $v == "$HOME"/*) && $HOME != / ]] && v=\~${v#"$HOME"}
	if	[[ -n $keep ]]
	then	del=${v%/$keep}/
		[[ $v == /*/$keep ]] && v=$ellip/${v#"$del"}
		[[ $v == \~/*/$keep ]] && v=\~/$ellip/${v#"$del"}
	fi
	.sh.value=$v
}
# convert <=1 to *, 2 to */*, 3 to */*/*, etc.
function RELPWD.keep.set
{
	typeset -si i n
	let "(n=${.sh.value}) ? 1 : 1" 2>/dev/null || n=3
	((n>64)) && n=64   # sanity check
	((n<1)) && .sh.value='' && return
	.sh.value='*'
	for ((i=2; i <= n; i++))
	do	.sh.value+='/*'
	done
}
# change this anytime to set number of pathname components kept
# set to 0 to keep all (but still change your home directory to ~)
RELPWD.keep=3

# get the current git branch
function GITBRANCH.get
{
	command -v git >/dev/null || return
	.sh.value=${ git branch --show-current 2>/dev/null; } && return
	# fallback for old git versions without --show-current:
	.sh.value=${ git branch 2>/dev/null; }
	case $'\n'${.sh.value} in
	*$'\n* '*)
		.sh.value=$'\n'${.sh.value}
		.sh.value=${.sh.value#*$'\n* '}
		.sh.value=${.sh.value%%$'\n'*}
		;;
	*)	.sh.value=''
		;;
	esac
}

# ${.rc.status} is a nicely formatted/coloured $?
function .rc.status.get
{
	typeset e=$?
	typeset clr=${.rc.fmt[status$((e != 0))]}
	typeset q1=$'\u00AB' q2=$'\u00BB'
	((${#q1}==1 && ${#q2}==1)) || q1='<' q2='>'
	if	((e>256))  # add signal name
	then	typeset s=${ kill -l "$e"; }
		[[ $s == *[A-Z]* ]] && e+="/SIG$s"
	fi
	.sh.value=${.rc.fmt[reset]}$q1$clr$e${.rc.fmt[reset]}$q2
}

# Regular (PS1) prompt.
typeset -sui .rc.ps2_lineno=0
case ${.rc.uid} in
0)	.rc.clr1=${.rc.fmt[magenta]} .rc.clr2=${.rc.fmt[red]} .rc.char='#' ;;
*)	.rc.clr1=${.rc.fmt[blue]} .rc.clr2=${.rc.fmt[cyan]} .rc.char='$' ;;
esac
# use single quotes to avoid expanding variables now, because PS1 is special:
# it expands variables in its value even without a discipline function.
# Also, '!' expands to the history file number (see 'man fc').
PS1='${.rc.status}${.rc.clr1}!${.rc.fmt[reset]}:${.rc.clr2}${RELPWD}'
PS1+='${GITBRANCH:+${.rc.clr1}[${GITBRANCH}]}${.rc.fmt[reset]} ${.rc.char} '

function PS1.get
{
	# reset PS2 counter whenever PS1 is shown
	.rc.ps2_lineno=0
}

# PS2 line continuation prompt. It does not expand anything by itself,
# but we can use a discipline function to make it act dynamically.
function PS2.get
{
	typeset clr1 clr2 char
	case ${.rc.uid} in
	0)	clr1=${.rc.fmt[magenta]} clr2=${.rc.fmt[red]} ;;
	*)	clr1=${.rc.fmt[blue]} clr2=${.rc.fmt[cyan]} ;;
	esac
	char=$'\u22d7'  # greater-than with dot (UTF-8)
	((${#char}==1)) || char='>'
	((.rc.ps2_lineno++))
	printf -v .sh.value ' %s%s%02d%s%s %c%s ' \
		"$clr1" \
		"${.rc.fmt[underline]}" \
		.rc.ps2_lineno \
		"${.rc.fmt[reset]}" \
		"$clr2" \
		"$char" \
		"${.rc.fmt[reset]}"
}

# Stuff for when this is run from 'bin/package use'.
if	[[ -v INSTALLROOT ]]
then
	# keeping ENV pointed here can cause breakage when other shells run,
	# particularly as some shells load ENV even for scripts (contra POSIX)
	unset ENV

	# make shipped functions available
	case $FPATH: in
	"$INSTALLROOT"/fun:*)
		;;
	*)	FPATH=$INSTALLROOT/fun${FPATH:+:$FPATH} ;;
	esac
	export FPATH
	autoload autocd cd dirs man mcd popd pushd

	# Workaround for "System Integrity Protection" on macOS which filters
	# out DYLD_* env vars whenever anything in /bin or /usr/bin is run,
	# which kills env "$SHELL" for a preinstalled dynamically linked $SHELL
	case $HOSTTYPE,${DYLD_LIBRARY_PATH+s} in
	darwin.*,s)
		function env
		{
			typeset -a opts
			typeset -si i=0
			while	[[ $1 == -* && $1 != -- ]]
			do	opts[i++]=$1
				case $1 in
				-u | -S | -P)
					(($# > 1)) && opts[i++]=$2 && shift ;;
				esac
				shift
			done
			command -p env "${opts[@]}" "DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH" "$@"
		}
		;;
	esac

	# Print welcome message
	set -- ${.sh.version}
	print -r "${.rc.fmt[bold]}Welcome to ksh ${.rc.fmt[red]}$3 ${.rc.fmt[blue]}$4${.rc.fmt[reset]} on ${.rc.fmt[bold]}${.rc.host} (${.rc.tty})${.rc.fmt[reset]}"
	set --
	print -r -- "- Have a look at ${.rc.fmt[bold]}~ast/etc/kshrc${.rc.fmt[reset]} to see all the cool stuff activated here"
	print -r -- "- Type '${.rc.fmt[bold]}builtin${.rc.fmt[reset]}' for a list of built-in commands (with and without path)"
	print -r -- "- Autoloadable functions have been activated. Check them out in ${.rc.fmt[bold]}~ast/fun${.rc.fmt[reset]}"
	print -r -- "- Get help: use '${.rc.fmt[bold]}man${.rc.fmt[reset]} ${.rc.fmt[underline]}commandname${.rc.fmt[reset]}' for everything, even built-in commands"
fi

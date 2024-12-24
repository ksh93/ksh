#
# Compile-time SHOPT_* options for ksh93.
# 1 to enable, 0 to disable, empty value to probe.
#
# For a more complete description of the options, see src/cmd/ksh93/README.
#

SHOPT ACCT=0				# accounting
SHOPT ACCTFILE=0			# per-user accounting info
SHOPT ALL_LIBCMD=0			# enable all available builtins in src/lib/libcmd
SHOPT AUDIT=1				# enable auditing per SHOPT_AUDITFILE
SHOPT AUDITFILE='"/etc/ksh_audit"'	# auditing file
SHOPT BGX=1				# one SIGCHLD trap per completed job
SHOPT BRACEPAT=1			# C-shell {...,...} expansions (, required)
SHOPT CMDLIB_HDR=  # '<cmdlist.h>'	# custom -lcmd list for path-bound builtins
SHOPT CMDLIB_DIR=  # '"/opt/ast/bin"'	# virtual directory prefix for path-bound builtins
SHOPT CRNL=				# accept MS Windows newlines (<cr><nl>) for <nl>
SHOPT DEVFD=				# use /dev/fd instead of FIFOs for process substitutions
SHOPT DYNAMIC=1				# dynamic loading for builtins
SHOPT ECHOPRINT=			# make echo equivalent to print
SHOPT ESH=1				# emacs/gmacs edit mode
SHOPT FILESCAN=1			# fast file scan: while <file; do [use $REPLY or positional parameters]; done
SHOPT FIXEDARRAY=1			# fixed dimension indexed array
SHOPT GLOBCASEDET=			# -o globcasedetect: adapt globbing/completion to case-insensitive file systems
SHOPT HISTEXPAND=1			# csh-style history file expansions
SHOPT KIA=				# ksh -R <outfile> <script> generates cross-ref database from script
SHOPT MKSERVICE=0			# enable the mkservice and eloop builtins
SHOPT MULTIBYTE=			# multibyte character handling
SHOPT NAMESPACE=1			# allow namespaces
SHOPT NOECHOE=0				# turn off 'echo -e' when SHOPT_ECHOPRINT is disabled
SHOPT OPTIMIZE=1			# optimize loop invariants
SHOPT P_SUID=0				# real UIDs >= this value require -p for set[ug]id (to turn off, use empty, not 0)
SHOPT PRINTF_LEGACY=			# allow noncompliant printf(1) syntax (format arg starting with '-' without prior '--')
SHOPT REMOTE=				# enable --rc if running as a remote shell
SHOPT SCRIPTONLY=0			# build ksh for running scripts only; compile out the interactive shell
SHOPT SPAWN=				# use spawnveg for fork/exec
SHOPT STATS=1				# add .sh.stats variable
SHOPT SYSRC=1				# attempt . /etc/ksh.kshrc if interactive
SHOPT TEST_L=				# add 'test -l' as an alias for 'test -L'
SHOPT TIMEOUT=				# number of seconds for shell timeout
SHOPT VSH=1				# vi edit mode

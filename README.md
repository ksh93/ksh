![](https://github.com/ksh93/ksh/workflows/CI/badge.svg)

# KornShell 93u+m

Welcome to the repository where the KornShell is under active development.
This is where we develop bugfixes and new features for the shell, and where
users can download the latest releases or the current development version in
source code form.
The project started off from last stable release (93u+ 2012-08-01) of
[ksh93](http://www.kornshell.com/),
formerly developed by AT&T Software Technology (AST).
The sources in this repository were forked from the
GitHub [AST repository](https://github.com/att/ast)
which is no longer under active development.

For user-visible fixes, see [NEWS](https://github.com/ksh93/ksh/blame/dev/NEWS)
and click on commit messages for full details.
For all fixes, see [the commit log](https://github.com/ksh93/ksh/commits/).
To see what's left to fix, see [the issue tracker](https://github.com/ksh93/ksh/issues).

## Table of contents ##

* [Policy](#user-content-policy)
* [Why?](#user-content-why)
* [Installing from source](#user-content-installing-from-source)
    * [Supported systems](#user-content-supported-systems)
    * [Prepare](#user-content-prepare)
    * [Build](#user-content-build)
    * [Test](#user-content-test)
    * [Install](#user-content-install)
* [What is ksh93?](#user-content-what-is-ksh93)

## Policy

1. Feature development for future releases happens on the dev branch.
   The numbered release branch(es) are feature-frozen and get bugfixes
   and maintenance only, usually cherry-picked from the dev branch.
2. No major rewrites. No refactoring code that is not fully understood.
   Even gradual and careful development may culminate in profound changes.
   Bit rot is prevented by cleaning up unused and obsolete code.
3. Maintain documented behaviour. Changes required for compliance with the
   [POSIX shell language standard](https://pubs.opengroup.org/onlinepubs/9699919799/utilities/contents.html)
   are implemented for the `posix` mode only to avoid breaking legacy scripts.
4. No 100% bug compatibility. Broken and undocumented behaviour gets fixed.
5. No bureaucracy, no formalities. Just fix it, or report it: create issues,
   send pull requests. Every interested party is invited to contribute.
6. To help increase everyone's understanding of this code base, fixes and
   significant changes should be fully documented in commit messages.
   Each commit should be a complete, self-contained and self-documenting
   change, including updates to documentation and regression tests where
   applicable. Pull requests are therefore squashed into a single commit.
7. Code style varies somewhat in this historic code base.
   Your changes should match the style of the code surrounding them.
   Indent with tabs, assuming an 8-space tab width.
   Opening braces are on a line of their own, at the same indentation level
   as their corresponding closing brace.
   Comments always use `/*`...`*/`.
8. Good judgment may override this policy.

## Why?

Between 2017 and 2020 there was an ultimately unsuccessful
[attempt](https://github.com/att/ast/tree/2020.0.1)
to breathe new life into the KornShell by extensively refactoring the last
unstable AST beta version (93v-).
While that ksh2020 effort is now abandoned and still has many critical bugs,
it also had a lot of bugs fixed. More importantly, the AST issue tracker
now contains a lot of documentation on how to fix those bugs, which made
it possible to backport many of them to the last stable release instead.
This ksh 93u+m reboot now incorporates many of these bugfixes,
plus patches from
[OpenSUSE](https://github.com/ksh93/ksh/wiki/Patch-Upstream-Report:-OpenSUSE),
[Red Hat](https://github.com/ksh93/ksh/wiki/Patch-Upstream-Report:-Red-Hat),
and
[Solaris](https://github.com/ksh93/ksh/wiki/Patch-Upstream-Report:-Solaris),
as well as many new fixes from the community
([1](https://github.com/ksh93/ksh/pulls?q=is%3Apr+is%3Amerged),
[2](https://github.com/ksh93/ksh/issues?q=is%3Aissue+is%3Aclosed+label%3Abug)).
Though there are many
[bugs left to fix](https://github.com/ksh93/ksh/issues),
we are confident at this point that 93u+m is already the least buggy version
of ksh93 ever released.
As of late 2021, distributions such as Debian and Slackware have begun
to package it as their default version of ksh93.

## Installing from source

You can download a [release](releases) tarball,
or clone the current code from your preferred branch.
New features for the future release series are developed on the `dev` branch.
Stable releases are currently based on the `1.0` branch.

### Supported systems

KornShell 93u+m is currently known to build and run on:
* Android/Termux
* Cygwin
* DragonFly BSD
* FreeBSD
* Haiku
* illumos distributions (e.g., OmniOS)
* Linux: all distributions with glibc or musl libc
* macOS
* NetBSD
* OpenBSD
* QNX Neutrino (6.5.0)
* Solaris

Systems that may work, but that we have not been able to test lately, include:
* AIX
* HP-UX
* UnixWare

KornShell 93u+m supports systems that use the ASCII character set as the
lowest common denominator. This includes Linux on IBM zSeries, but not z/OS.
Support for the EBCDIC character set has been removed, as we do not have
access to a mainframe with z/OS to test and maintain it.

### Prepare

The build system requires only a basic POSIX-compatible shell, utilities and
compiler environment. The `cc`, `ar` and `getconf` commands are needed at
build time. The `tput` and `getconf` commands are used at runtime if
available (for multiline editing and to complete the `getconf` built-in,
respectively). Not all systems come with all of these preinstalled. Here are
system-specific instructions for making them available:

* **Android/[Termux](https://termux.dev/):**
  install dependencies using `pkg install`.
    * Build dependencies: `clang`, `binutils`, `getconf`
    * Runtime dependencies (optional): `ncurses-utils`, `getconf`
* **macOS:**
  install the Xcode Command Line Tools:    
  `xcode-select --install`
* (to be completed)

### Build

To build ksh with a custom configuration of features, edit
[`src/cmd/ksh93/SHOPT.sh`](https://github.com/ksh93/ksh/blob/dev/src/cmd/ksh93/SHOPT.sh).

On systems such as NetBSD and OpenBSD, where `/bin/ksh` is not ksh93 and the
preinstalled `/etc/ksh.kshrc` profile script is incompatible with ksh93, you'll
want to disable `SHOPT_SYSRC` to avoid loading it on startup -- unless you can
edit it to make it compatible with ksh93. This generally involves differences
in the declaration and usage of local variables in functions.

Then `cd` to the top directory and run:

```
bin/package make
```

To suppress compiler output, use `quiet make` instead of `make`.

In some non-POSIX shells you might need to prepend `sh` to all calls to `bin/package`.

Parallel building is supported by appending `-j` followed by the
desired maximum number of concurrent jobs, e.g., `bin/package make -j4`.
This speeds up building on systems with more than one CPU core.
(Type `bin/package host cpu` to find out how many CPU cores your system has.)

The compiled binaries are stored in the `arch` directory, in a subdirectory
that corresponds to your architecture. The command `bin/package host type`
outputs the name of this subdirectory.

Dynamically linked binaries, if supported for your system, are stored in
`dyn/bin` and `dyn/lib` subdirectories of your architecture directory.
If built, they are built in addition to the statically linked versions.
Export `AST_NO_DYLIB` to deactivate building dynamically linked versions.

If you have trouble or want to tune the binaries, you may pass additional
compiler and linker flags. It is usually best to export these as environment
variables *before* running `bin/package` as they could change the name of
the build subdirectory of the `arch` directory, so exporting them is a
convenient way to keep them consistent between build and test commands.
**Note that this system uses `CCFLAGS` instead of the usual `CFLAGS`.**
An example that makes Solaris Studio cc produce a 64-bit binary:

```
export CCFLAGS="-m64 -O" LDFLAGS="-m64"
bin/package make
```

Alternatively you can append these to the command, and they will only be
used for that command. You can also specify an alternative shell in which
to run the build scripts this way. For example:

```
bin/package make SHELL=/bin/bash CCFLAGS="-O2 -I/opt/local/include" LDFLAGS="-L/opt/local/lib"
```

**Note:** Do not add compiler flags that cause the compiler to emit terminal
escape codes, such as `-fdiagnostics-color=always`; this will cause the
build to fail as the probing code greps compiler diagnostics. Additionally,
do not add the `-ffast-math` compiler flag; arithmetic bugs will occur when
using that flag.

For more information run

```
bin/package help
```

Many other commands in this repo self-document via the `--help`, `--man` and
`--html` options; those that do have no separate manual page.

### Test

After compiling, you can run the regression tests.
To run the default test sets for ksh and the build system, use:

```
bin/package test
```

For ksh, use the `shtests` command directly to control the regression test runs.
Start by reading the information printed by:

```
bin/shtests --man
```

To hand-test ksh (as well as the utilities and the autoloadable functions
that come with it) without installing, run:

```
bin/package use
```

### Install

Usage: `bin/package install` *install_root_directory* [ *command* ... ]

Any command from the `arch` directory can be installed. If no *command* is
specified, `ksh` and `shcomp` are assumed.

The *install_root_directory* is the directory from which the command(s) will
actually be run. It will be created if it does not exist. Commands are
installed into its `bin` subdirectory, any shared libraries into `lib`, C
development header files into `include/ast`, and each command's manual page,
if available, is installed into `share/man`.

If a dynamically linked version of ksh and associated commands has been
built, then the `install` subcommand will prefer that: commands, dynamic
libraries and associated header files will be installed then. To install the
statically linked version instead (and skip the header files), either delete
the `dyn` subdirectory, or export `AST_NO_DYLIB=y` before building to prevent
it from being created in the first place.

An additional install prefix directory path can be passed in `DESTDIR`, which
can be either passed as an environment variable or specified on the comannd
line as an extra assignment-like argument. The value of `DESTDIR` will be
prefixed to the path of every destination file when installing it, but not
when configuring the install root directory in the installed files (as may be
required by individual systems, e.g., to find dynamic libraries). This feature
is designed for packagers who need to install ksh into a directory other than
the one from which it will be run in order to package it.

## What is ksh93?

The following is the official AT&T description from 1993 that came with the
ast-open distribution. The text is original, but hyperlinks were added here.

----

KSH-93 is the most recent version of the KornShell Language described in
"The KornShell Command and Programming Language," by Morris Bolsky and David
Korn of AT&T Bell Laboratories, ISBN 0-13-182700-6. The KornShell is a shell
programming language, which is upward compatible with "sh" (the Bourne
Shell), and is intended to conform to the IEEE P1003.2/ISO 9945.2
[Shell and Utilities standard](https://pubs.opengroup.org/onlinepubs/9699919799/utilities/contents.html).
KSH-93 provides an enhanced programming environment in addition to the major
command-entry features of the BSD shell "csh". With KSH-93, medium-sized
programming tasks can be performed at shell-level without a significant loss
in performance. In addition, "sh" scripts can be run on KSH-93 without
modification.

The code should conform to the
[IEEE POSIX 1003.1 standard](https://www.opengroup.org/austin/papers/posix_faq.html)
and to the proposed ANSI C standard so that it should be portable to all
such systems. Like the previous version, KSH-88, it is designed to accept
eight bit character sets transparently, thereby making it internationally
compatible. It can support multi-byte characters sets with some
characteristics of the character set given at run time.

KSH-93 provides the following features, many of which were also inherent in
KSH-88:

* Enhanced Command Re-entry Capability: The KSH-93 history function records
  commands entered at any shell level and stores them, up to a
  user-specified limit, even after you log off. This allows you to re-enter
  long commands with a few keystrokes - even those commands you entered
  yesterday. The history file allows for eight bit characters in commands
  and supports essentially unlimited size histories.
* In-line Editing: In "sh", the only way to fix mistyped commands is to
  backspace or retype the line. KSH-93 allows you to edit a command line
  using a choice of EMACS-TC or "vi" functions. You can use the in-line
  editors to complete filenames as you type them. You may also use this
  editing feature when entering command lines from your history file. A user
  can capture keystrokes and rebind keys to customize the editing interface.
* Extended I/O Capabilities: KSH-93 provides several I/O capabilities not
  available in "sh", including the ability to:
    * specify a file descriptor for input and output
    * start up and run co-processes
    * produce a prompt at the terminal before a read
    * easily format and interpret responses to a menu
    * echo lines exactly as output without escape processing
    * format output using printf formats.
    * read and echo lines ending in "\\". 
* Improved performance: KSH-93 executes many scripts faster than the System
  V Bourne shell. A major reason for this is that many of the standard
  utilities are built-in. To reduce the time to initiate a command, KSH-93
  allows commands to be added as built-ins at run time on systems that
  support dynamic loading such as System V Release 4.
* Arithmetic: KSH-93 allows you to do integer arithmetic in any base from
  two to sixty-four. You can also do double precision floating point
  arithmetic. Almost the complete set of C language operators are available
  with the same syntax and precedence. Arithmetic expressions can be used to
  as an argument expansion or as a separate command. In addition, there is an
  arithmetic for command that works like the for statement in C.
* Arrays: KSH-93 supports both indexed and associative arrays. The subscript
  for an indexed array is an arithmetic expression, whereas, the subscript
  for an associative array is a string.
* Shell Functions and Aliases: Two mechanisms - functions and aliases - can
  be used to assign a user-selected identifier to an existing command or
  shell script. Functions allow local variables and provide scoping for
  exception handling. Functions can be searched for and loaded on first
  reference the way scripts are.
* Substring Capabilities: KSH-93 allows you to create a substring of any
  given string either by specifying the starting offset and length, or by
  stripping off leading or trailing substrings during parameter
  substitution. You can also specify attributes, such as upper and lower
  case, field width, and justification to shell variables.
* More pattern matching capabilities: KSH-93 allows you to specify extended
  regular expressions for file and string matches.
* KSH-93 uses a hierarchical name space for variables. Compound variables can
  be defined and variables can be passed by reference. In addition, each
  variable can have one or more disciplines associated with it to intercept
  assignments and references.
* Improved debugging: KSH-93 can generate line numbers on execution traces.
  Also, I/O redirections are now traced. There is a DEBUG trap that gets
  evaluated before each command so that errors can be localized.
* Job Control: On systems that support job control, including System V
  Release 4, KSH-93 provides a job-control mechanism almost identical to
  that of the BSD "csh", version 4.1. This feature allows you to stop and
  restart programs, and to move programs between the foreground and the
  background.
* Added security: KSH-93 can execute scripts which do not have read
  permission and scripts which have the setuid and/or setgid set when
  invoked by name, rather than as an argument to the shell. It is possible
  to log or control the execution of setuid and/or setgid scripts. The
  noclobber option prevents you from accidentally erasing a file by
  redirecting to an existing file.
* KSH-93 can be extended by adding built-in commands at run time. In
  addition, KSH-93 can be used as a library that can be embedded into an
  application to allow scripting.

Documentation for KSH-93 consists of an "Introduction to KSH-93",
"Compatibility with the Bourne Shell" and a manual page and a README file.
In addition, the "New KornShell Command and Programming Language" book is
available from Prentice Hall.

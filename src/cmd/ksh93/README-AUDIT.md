# Korn Shell 93 Auditing and Accounting #

This documentation was adapted from a 2008 blog post by Finnbarr P. Murphy
and was added to the ksh 93u+m distribution under the same license as ksh
by permission from the author, given on 24th Jan 2021.
[Original](https://web.archive.org/web/20240303045802/https://blog.fpmurphy.com/2008/12/ksh93-auditing-and-accounting.html).
The responsibility for any errors in this file lies with the current
ksh 93u+m maintainers and not with the original author.

## Introduction ##

Korn Shell 93 (ksh93) is the only UNIX or GNU/Linux shell that I am aware
of that, with proper setup, supports a modicum of per-user
auditing. This post attempts to explain these facilities and show you how
to access and manipulate the resulting auditing records.

An auditing facility was added in July 2008. It is configurable and
writes out a fairly detailed record either locally or to a remote system for
each user command that is executed. This facility can be used to monitor,
track, record, and audit the activities of one or more users on a system,
including system administrators. As of ksh 93u+ 2012-06-12, this is compiled
in to the shell by default.

The facility only works for interactive users.

To enable or disable it, you need to modify the compile time options in
`src/cmd/ksh93/SHOPT.sh` as follows. Use `0` to disable and `1` to enable
`SHOPT_AUDIT` and edit the auditing file path if you like. Then recompile
the sources; see `README.md` in the top directory for building instructions.

    SHOPT AUDIT=1                         # enable auditing per SHOPT_AUDITFILE
    SHOPT AUDITFILE='\"/etc/ksh_audit\"'  # auditing file

After you have recompiled the sources, the new ksh executable is located in the
`arch/`...`/bin` subdirectory. To verify that `SHOPT_AUDIT` has been
compiled in to the executable, look for the `A` in the shell version string.

    $ arch/darwin.arm64-64/bin/ksh -c 'echo ${.sh.version}'
    Version AJM 93u+m/1.1.0-alpha 2026-09-12
    $ arch/darwin.arm64-64/bin/ksh -c 'echo KSH_VERSION'
    Version AJM 93u+m/1.1.0-alpha 2026-09-12

The option string AJM means that
(A) auditing is supported (`SHOPT_AUDIT`),
(J) one SIGCHLD trap per completed job is supported (`SHOPT_BGX`),
(M) multibyte characters are supported (`SHOPT_MULTIBYTE`).

## Auditing ##

Now we turn our attention to the auditing facility itself. Assuming ksh has been
compiled with the `SHOPT_AUDIT` option (the default), you must create an audit
configuration file on each system to tell ksh93 where to store the audit
records and to specify which users are to be audited. The configuration file
must be readable by the users whose activities are audited. Its default
location is `/etc/ksh_audit` but that can be changed in the `SHOPT.sh` file.
The configuration file should contain a line that defines the file to write
the audit records to, followed by the UID of each user whose commands are to
generate audit records. Here is the configuration file used to generate the
audit records for this part of this post.

    $ cat /etc/ksh_audit
    /tmp/ksh_auditfile;500

This configuration file specifies that audit records are to be written to
`/tmp/ksh_auditfile` for the user whose UID is 500. Note that the field
delimiter is a semicolon.

Here are the audit records stored in `/tmp/ksh_auditfile` which match the
accounting records shown previously in this post. The field separator is a
semicolon. The first field is the UID of the user executing the command.
The second field is the time in seconds since the Epoch. The third field is
the terminal device on which the command was executed, and the final field
is the actual command executed by the user.
The command is shown as originally typed; variables, etc., are not expanded.

    500;1230606552;/dev/pts/2; echo ${.sh.version}
    500;1230606554;/dev/pts/2; pwd
    500;1230606557;/dev/pts/2; id
    500;1230606563;/dev/pts/2; date
    500;1230606565;/dev/pts/2; exit

What is not visible is that each entry in this file is terminated by a 0
(zero) byte following the final newline character of each command. Because
commands may span multiple lines, this is the only way to reliably separate
audit records from each other. But it makes parsing the file in the shell a
bit challenging, as variable values cannot contain the 0 byte.

Here is a simple ksh93 script that reliably parses this audit file. It reads
0-terminated records by specifying an empty record separator (`-d ""`) to
the `read` command and splits the fields by semicolon. It shell-quotes every
command string, including the final newline, to ensure each output record
spans one single line, even for history entries that span multiple lines.
It also replaces the UID with the actual user's name (with caching to avoid
an expensive id(1) invocation for every line), and seconds since the Epoch
with the actual date and time. It outputs the enhanced records in a comma
separated value (CSV) format.

    AUDITFILE="/tmp/ksh_auditfile"
    while IFS=";" read -d "" uid sec tty cmdstr
    do
       printf -v cmdstr '%q' "${cmdstr:1}"  # trim 1 leading space
       unam=${cache[$uid]:=$(id -un "$uid" 2>/dev/null || echo '(unknown)')}
       printf '%(%Y-%m-%d %H:%M:%S)T, %s, %d, %s, %s\n' \
          "#$sec" "$unam" "$uid" "$tty" "$cmdstr"
    done < $AUDITFILE

Here is the output for the above audit records.

    2008-12-30 03:09:12, fpm, 500, /dev/pts/2, $'echo ${.sh.version}\n'
    2008-12-30 03:09:14, fpm, 500, /dev/pts/2, $'pwd\n'
    2008-12-30 03:09:17, fpm, 500, /dev/pts/2, $'id\n'
    2008-12-30 03:09:23, fpm, 500, /dev/pts/2, $'date\n'
    2008-12-30 03:09:25, fpm, 500, /dev/pts/2, $'exit\n'

The audit file must be writable by all users whose activities are audited,
presenting an obvious security problem. However, the Korn shell supports
networking using the `/dev/udp/`*host*`/`*port* or `/dev/tcp/`*host*`/`*port*
syntax, so audit records can be sent across a network to another system.
This mechanism could be used to store audit records on a secured centralized
system to which only specific personnel have access. As an example, the
following audit configuration file line designates that audit records for
the user whose UID is 500 should be sent using UDP to the syslog network
port (514) on a remote system whose IP is 192.168.0.99.

    /dev/udp/192.168.0.99/514;500

Here are the same audit records stored by the syslog daemon on the remote system.

    2008-12-29 22:09:12 192,169.0.115 500;1230606552;/dev/pts/2; echo ${.sh.version}
    2008-12-29 22:09:14 192.169.0.115 500;1230606554;/dev/pts/2; pwd
    2008-12-29 22:09:17 192.169.0.115 500;1230606557;/dev/pts/2; id
    2008-12-29 22:09:23 192.169.0.115 500;1230606563;/dev/pts/2; date
    2008-12-29 22:09:25 192.169.0.115 500;1230606565;/dev/pts/2; exit

Depending on the configuration of the syslog daemon on your particular
system, the first part of the record may contain more or less information or
be formatted differently but the final part of the record, i.e. the audit
record sent by ksh93 should be in the standard audit record format.

## Afterword ##

Note that while the auditing facility within ksh93 can provide you with much
useful information regarding the actions of one or more users on a system or
systems, it should not be regarded as providing enhanced security akin to
the Trusted Computing Base (TCB). There are many ways of circumventing it.
For example, a knowledgeable user could switch to a different shell such as
bash where their actions will not be recorded. There are a number of other
ways, but I will not discuss them here.

Most of the information provided in this post is not documented in a single
place anywhere that I can find by searching the Internet. The ksh93 man page
does not mention either the accounting or auditing facilities. Even the
ksh93 source code is somewhat vague. I gleaned most of this information by
studying the code in
[`src/cmd/ksh93/edit/history.c`](https://github.com/ksh93/ksh/blob/dev/src/cmd/ksh93/edit/history.c).

*Martijn Dekker adds:* I would like to thank the author Finnbarr P. Murphy
for his permission to use his ksh93-related blog posts in the ksh 93u+m
distribution. As of 2026, this is still the only documentation available for
the facility described. If you find any errors or omissions, please
[file an issue](https://github.com/ksh93/ksh).

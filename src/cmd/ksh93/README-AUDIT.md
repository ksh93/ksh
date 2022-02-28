# Korn Shell 93 Auditing and Accounting #

This documentation was adapted from a 2008 blog post by Finnbarr P. Murphy
and was added to the ksh 93u+m distribution under the same license as ksh
by permission from the author, given on 24th Jan 2021.
[Original](https://blog.fpmurphy.com/2008/12/ksh93-auditing-and-accounting.html).
The responsibility for any errors in this file lies with the current
ksh 93u+m maintainers and not with the original author.

## Introduction ##

Korn Shell 93 (ksh93) is the only UNIX or GNU/Linux shell that I am aware
of that, with proper setup, supports a modicum of per-user accounting and
auditing. This post attempts to explain these facilities and show you how
to access and manipulate the resulting accounting and auditing records.

Per-user accounting has been a feature of ksh93 since the earliest days of this
shell. It is a fairly primitive facility which writes out a record for each
user command that is executed. By default, it is not compiled in to the shell.

An auditing facility was added in July 2008. This is somewhat more
sophisticated than the accounting facility in that it is configurable and
writes out a more detailed record either locally or to a remote system for
each user command that is executed. This facility can be used to monitor,
track, record, and audit the activities of one or more users on an system,
including system administrators. As of ksh 93u+ 2012-06-12, this is compiled
in to the shell by default.

Both facilities only work for interactive users.

To enable or disable one or both facilities, you need to modify the compile
time options in `src/cmd/ksh93/SHOPT.sh` as follows. Use `0` to disable and
`1` to enable (except for `SHOPT_AUDITFILE`). Then recompile the sources;
see `README` for building instructions.

    SHOPT ACCT=1                          # accounting
    SHOPT ACCTFILE=1                      # per-user accounting info
    SHOPT AUDIT=1                         # enable auditing per SHOPT_AUDITFILE
    SHOPT AUDITFILE='\"/etc/ksh_audit\"'  # auditing file

After you have recompiled the sources, the new ksh executable is located in the
`arch/`...`/bin` subdirectory. To see what options have actually been compiled
into a particular executable, just print out the shell version string.

    $ arch/darwin.i386-64/bin/ksh -c 'echo ${.sh.version}'
    Version AJLM 93u+m/1.1.0-alpha+dev 2022-01-20
    $ arch/darwin.i386-64/bin/ksh -c 'echo $KSH_VERSION'
    Version AJLM 93u+m/1.1.0-alpha+dev 2022-01-20

The option string AJLM means that
(A) auditing is supported (`SHOPT_AUDIT`),
(J) one SIGCHLD trap per completed job is supported (`SHOPT_BGX`),
(L) accounting is supported (`SHOPT_ACCT`), and
(M) multibyte characters are supported (`SHOPT_MULTIBYTE`).

## Accounting ##

After recompiling the shell to enable this facility, per-user accounting is
enabled using the `SHACCT` environment variable. To turn on per-user
accounting, simply set `SHACCT` to the name of the file where you wish to
store the accounting records.

    export SHACCT="/tmp/ksh_acctfile"

Here is part of the resulting file. Note that the time is stored as
hexadecimal seconds since the Unix epoch (00:00:00 UTC on 1 January 1970).

    $ cat /tmp/ksh_acctfile
    echo ${.sh.version}	fpm	495990d8
    pwd	fpm	495990da
    id	fpm	495990dd
    date	fpm	495990e3
    exit	fpm	495990e5

The following shell script can be used to access the records in this file
and output them in a more useful format.

    #!/bin/ksh93
    ACCTFILE="/tmp/ksh_acctfile"
    printf "DATE       TIME     LOGIN  COMMAMD\n\n"
    # set IFS to TAB only
    while IFS=$'\t' read cmdstr name hexseconds
    do
        printf -v longsecs "%ld" "0x${hexseconds}"
        printf "%(%Y-%m-%d %H:%M:%S)T, %s, %s\n" "#${longsecs}" "$name" "$cmdstr"
    done < $ACCTFILE

Invoking this script gives the following output for the above accounting
records.

    $ ./parse_acctfile
    DATE       TIME     LOGIN  COMMAMD

    2008-12-29 22:09:12, fpm, echo ${.sh.version}
    2008-12-29 22:09:14, fpm, pwd
    2008-12-29 22:09:17, fpm, id
    2008-12-29 22:09:23, fpm, date
    2008-12-29 22:09:25, fpm, exit

## Auditing ##

Next we turn our attention to the auditing facility. Assuming ksh has been
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

    500;1230606552;/dev/pts/2; echo ${.sh.version}
    500;1230606554;/dev/pts/2; pwd
    500;1230606557;/dev/pts/2; id
    500;1230606563;/dev/pts/2; date
    500;1230606565;/dev/pts/2; exit

As before, here is a simple ksh93 script which parses this audit file,
replaces the UID with the actual user's name and seconds since the Epoch
with the actual date and time, and outputs the enhanced records in a comma
separated value (CSV) format.

    #!/bin/ksh93
    AUDITFILE="/tmp/ksh_auditfile"
    while IFS=";" read uid sec tty cmdstr
    do
       printf '%(%Y-%m-%d %H:%M:%S)T, %s, %d, %s, %s\n' \
          "#$sec" "$(id -un $uid)" "$uid" "$tty" "$cmdstr"
    done < $AUDITFILE

Here is the output for the above audit records.

    2008-12-29 22:09:12, fpm, 500, /dev/pts/2,  echo ${.sh.version}
    2008-12-29 22:09:14, fpm, 500, /dev/pts/2,  pwd
    2008-12-29 22:09:17, fpm, 500, /dev/pts/2,  id
    2008-12-29 22:09:23, fpm, 500, /dev/pts/2,  date
    2008-12-29 22:09:25, fpm, 500, /dev/pts/2,  exit

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

Note that while the auditing and accounting facilities within ksh93 can
provide you with much useful information regarding the actions of one or
more users on a system or systems, these facilities should not be regarded
as providing enhanced security akin to the Trusted Computing Base (TCB).
There are many ways of circumventing these facilities. For example, a
knowledgeable user could switch to a different shell such as bash where
their actions will not be recorded. There are a number of other ways but I
will not discuss them here.

Most of the information provided in this post is not documented in a single
place anywhere that I can find by searching the Internet. The ksh93 man page
does not mention either the accounting or auditing facilities. Even the
ksh93 source code is somewhat vague. I gleaned most of this information by
studying the code in
[`src/cmd/ksh93/edit/history.c`](https://github.com/ksh93/ksh/blob/dev/src/cmd/ksh93/edit/history.c).

*Martijn Dekker adds:* I would like to thank the author Finnbarr P. Murphy
for his permission to use his ksh93-related blog posts in the ksh 93u+m
distribution. As of 2022, this is still the only documentation available for
the facilities described. If you find any errors or omissions, please
[file an issue](https://github.com/ksh93/ksh).

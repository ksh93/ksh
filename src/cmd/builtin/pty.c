/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1992-2013 AT&T Intellectual Property          *
*          Copyright (c) 2020-2026 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  David Korn <dgk@research.att.com>                   *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/

static const char usage[] =
"[-?\n@(#)pty (ksh 93u+m) 2026-11-13\n]"
"[-author?Glenn Fowler <gsf@research.att.com>]"
"[-author?David Korn <dgk@research.att.com>]"
"[-author?Martijn Dekker <martijn@inlv.org>]"
"[-copyright?(c) 2001-2013 AT&T Intellectual Property]"
"[-copyright?(c) 2020-2026 Contributors to ksh 93u+m]"
"[-license?https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html]"
"[--catalog?builtin]"
"[+NAME?pty - create pseudoterminal and run command]"
"[+DESCRIPTION?\bpty\b creates a pseudo-tty and then runs \bcommand\b "
    "with arguments given by \aarg\a and the standard input, standard "
    "output, and standard error connected to the pseudoterminal.]"
"[+?If \bcommand\b does not contain a \b/\b, the \bPATH\b variable will "
    "be used to locate the \bcommand\b. "
    "The \bcommand\b with its \barg\bs is invoked directly via \bexecve\b(2) "
    "or an equivalent, so any shell constructs must be invoked via a shell, "
    "as in \b\"$SHELL\" -c '\b\ashell commands here\a\b'\b.]"
"[+?Input to \bpty\b will be written to the standard input of this "
    "command. The standard output and standard error from the command will "
    "be written to the standard output of \bpty\b.]"
"[+?The \bpty\b command terminates when the command completes.]"
"[+?\bpty\b does not need a terminal of its own; the pseudoterminal it "
    "creates becomes the terminal of the command. At init time, \bpty\b "
    "assigns it default attributes corresponding to a login session: "
    "line editing in canonical mode, echoing of typed input, and signals "
    "from the keyboard.]"
"[+?If the \b-d\b option is not given and the standard input of \bpty\b "
    "itself is a terminal, then the terminal is put in raw mode while the "
    "\bcommand\b runs, so that any typed input to the command is passed on as "
    "it is typed and echoed only by the command's pseudoterminal, which is in "
    "canonical mode. The real terminal's attributes are restored on exit.]"
"[d:dialogue?Execute the dialogue on the standard input. A dialogue is a "
    "sequence of commands, one command per line. All \are\a patterns are "
    "extended regular expressions. The \are\a \b?1\b will print the subject "
    "string on the standard error and match the string; the \are\a \b?0\b "
    "will print the subject string on the standard error and not match the "
    "string. The \are\a \b?.\b matches EOF. The commands are:]"
    "{"
        "[\b#\b \acomment\a?comment line]"
        "[c \atext\a?write \atext\a to the master; C style escapes "
            "in text are converted, including \\E for ESC and \\cX for "
            "control-X]"
        "[d \amilliseconds\a?set the delay before each master write to "
            "\amilliseconds\a; the default is no delay]"
        "[i \are\a?read a line from the master; if it matches \are\a "
            "then execute lines until matching \be\b or \bf\b]"
        "[e [\are\a]]?else [if match \are\a]] then execute lines until "
            "matching \be\b or \bf\b]"
        "[f?end of \bi\b/\be\b block]"
        "[m \atext\a?write \atext\a to the standard error]"
        "[p \atext\a?peek ahead and verify the next line begins "
            " with \atext\a; input is not consumed]"
        "[r [\are\a]]?read a line from the master [and it should match "
            "\are\a]]]"
        "[s \amilliseconds\a?sleep for \amilliseconds\a]"
        "[t \amilliseconds\a?set the master read timeout to "
            "\amilliseconds\a; the default is \b1000\b]"
        "[u \are\a?read lines from the master until one matches \are\a]"
        "[v \alevel\a?set the verbose trace \alevel\a, more output for "
            "higher levels, disabled for level 0]"
        "[w \atext\a?write \atext\a\\r\\n to the master; C style escapes "
            "in text are converted, including \\E for ESC and \\cX for "
            "control-X]"
        "[x [\acode\a]]"
            "?exit \bpty\b with exit code \b0\b [\acode\a]]]"
        "[I \are\a?ignore master lines matching \are\a]"
        "[L \alabel\a?prefix all diagnostics with \alabel\a:]"
        "[P \atext\a?delay each master write until the beginning of "
            "an unread input line exactly matches \atext\a]"
    "}"
"[D:debug?Set the debug trace \alevel\a, higher levels produce more "
    "output, disabled for level 0.]#[level]"
"[l:log?Log the master stdout and stderr to \afile\a.]:[file]"
"[m:messages?Redirect diagnostic message output to \afile\a.]:[file]"
"[s!:session?Create a separate session for the process started by "
    "\bpty\b.]"
"[t:timeout?Set the master read timeout to "
    "\amilliseconds\a.]#[milliseconds:=1000]"
"[T:tty?Pass \astty\a to the \bstty\b(1) command to initialize the "
    "pty.]:[stty]"
"[w:delay?Set the delay before each master write to "
    "\amilliseconds\a.]#[milliseconds:=0]"

"\n"
"\ncommand [arg ...]\n"
"\n"

"[+EXIT STATUS?If the command determined by \bcommand\b is run the exit "
    "status of \bpty\b is that of this command. Otherwise, the exit status "
    "is one of the following:]"
    "{"
        "[+127?The command is found but cannot be executed.]"
        "[+128?The command could not be found.]"
    "}"
"[+SEE ALSO?\bcommand\b(1), \bexec\b(1), \bopenpty\b(3), \bposix_openpt\b(3)]"
;

#include <cmd.h>
#include <error.h>
#include <fcntl.h>
#include <sig.h>
#include <termios.h>
#include <proc.h>
#include <ctype.h>
#include <regex.h>
#include <vmalloc.h>
#include <ast_time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "FEATURE/pty"

#define MODE_666	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
#define MAXNAME		64

#ifndef CMIN
#define CMIN		1
#endif

static COLD noreturn void outofmemory(size_t size)
{
	error(ERROR_SYSTEM|ERROR_PANIC, "out of memory (failed to allocate %zu bytes)", size);
	UNREACHABLE();
}

#if !_lib_openpty && !_lib__getpty && !defined(_pty_clone)
# if !_lib_grantpt || !_lib_unlock
#   if !_lib_ptsname
	static char *minionname(const char *name)
	{
		static char sname[MAXNAME];
		char *last;
		strlcpy(sname,name,sizeof(sname));
		last = strrchr(sname,'/');
		last[1] = 't';
		return sname;
	}
#   endif

    static char *master_name(char *name)
    {
	static char sname[MAXNAME];
	int n;
	if(!name)
	{
		strcpy(sname,_pty_first);
		return sname;
	}
	n = strlen(_pty_first);
	if(name[n-1]=='9')
		name[n-1]='a';
	else if(name[n-1]=='f')
	{
		if(_pty_first[n-2]=='0' && name[n-2]=='9')
		{
			name[n-2]='0';
			if(name[n-3]=='9' || name[n-3]=='z')
				return NULL;
			name[n-3]++;
		}
		if(_pty_first[n-2]=='p' && (name[n-2]=='z' || name[n-2]=='Z'))
		{
			if(name[n-2]=='z')
				name[n-2]='P';
			else
				return NULL;
		}
		else
			name[n-2]++;
		name[n-1]='0';
	}
	else
		name[n-1]++;
	return name;
    }
#endif

#if !_lib_openpty
	static char *ptymopen(int *master)
	{
		char *minion=0;
#   if _lib__getpty
		return _getpty(master,O_RDWR,MODE_666,0);
#   else
#	if defined(_pty_clone)
		*master = open(_pty_clone,O_RDWR|O_CREAT,MODE_666);
		if(*master>=0)
			minion = ptsname(*master);
#	else
		int fdm;
		char *name=0;
		while(name=master_name(name))
		{
			fdm = open(name,O_RDWR|O_CREAT,MODE_666);
			if(fdm >= 0)
			{
				*master = fdm;
#	   if _lib_ptsname
				minion = ptsname(fdm);
#	   else
				minion = minionname(name);
#	   endif
				break;
			}
		}
# 	endif
#   endif
		return minion;
	}
# endif
#endif

static int
mkpty(int* master, int* minion)
{
	struct termios	tty;
#ifdef TIOCGWINSZ
	struct winsize	win;
	struct winsize*	winp;
#endif
#if !_lib_openpty
	char*		sname;
#endif
#ifdef SIGTTOU
	sigset_t blckttou, oldset;
	(void)sigemptyset(&blckttou);
	(void)sigaddset(&blckttou, SIGTTOU);
	sigprocmask(SIG_BLOCK, &blckttou, &oldset);
#endif
	alarm(6);
#ifdef TIOCGWINSZ
	if (ioctl(sffileno(sfstderr), TIOCGWINSZ, &win) < 0)
	{
		if (errno != ENOTTY)
			error(-1, "unable to get standard error window size");
		win.ws_row = 0;
		win.ws_col = 0;
		winp = NULL;
	}
	if (win.ws_row < 24)
		win.ws_row = 24;
	if (win.ws_col < 80)
		win.ws_col = 80;
	winp = &win;
#endif
#ifdef __linux__
# if !_lib_openpty
#  undef  _lib_openpty
#  define _lib_openpty 1
# endif
#endif
#if _lib_openpty
	if (openpty(master, minion, NULL, NULL, winp) < 0)
		return -1;
#else
#if _lib_grantpt && _lib_unlockpt
#if !_lib_posix_openpt
#ifndef _pty_clone
#define _pty_clone	"/dev/ptmx"
#endif
#define posix_openpt(m)	open(_pty_clone,m)
#endif
	if ((*master = posix_openpt(O_RDWR)) < 0)
		return -1;
	if (grantpt(*master) || unlockpt(*master) || !(sname = ptsname(*master)) || (*minion = open(sname, O_RDWR|O_cloexec|O_NOCTTY)) < 0)
	{
		ast_close(*master);
		return -1;
	}
#else
	if (!(sname = ptymopen(master)) || (*minion = open(sname, O_RDWR|O_cloexec|O_NOCTTY)) < 0)
		return -1;
#endif
#ifdef I_PUSH
	{
		struct termios	tst;
		if (tcgetattr(*minion, &tst) < 0 && (ioctl(*minion, I_PUSH, "ptem") < 0 || ioctl(*minion, I_PUSH, "ldterm") < 0))
		{
			ast_close_batch(*minion,AST_QUEUE);
			ast_close_batch(*master,AST_ENACT);
			return -1;
		}
	}
#endif
#endif
	if (tcgetattr(*minion, &tty) < 0)
		error(ERROR_SYSTEM|1, "unable to get pty terminal attributes");
	/*
	 * The pseudoterminal must be suitable for interactive shell use: line editing
	 * in canonical mode, echoing of typed input, and signals from the keyboard.
	 */
	tty.c_lflag |= ICANON | IEXTEN | ISIG | ECHO | ECHOE | ECHOK;
#ifdef ECHOKE
	tty.c_lflag |= ECHOKE;
#endif
	tty.c_oflag |= (ONLCR | OPOST);
#ifdef OCRNL
	tty.c_oflag &= (tcflag_t)~OCRNL;
#endif
#ifdef ONLRET
	tty.c_oflag &= (tcflag_t)~ONLRET;
#endif
	tty.c_iflag |= BRKINT;
	tty.c_iflag &= (tcflag_t)~IGNBRK;
	tty.c_cc[VTIME] = 0;
	tty.c_cc[VMIN] = CMIN;
#ifdef B115200
	cfsetispeed(&tty, B115200);
	cfsetospeed(&tty, B115200);
#elif defined(B57600)
	cfsetispeed(&tty, B57600);
	cfsetospeed(&tty, B57600);
#elif defined(B38400)
	cfsetispeed(&tty, B38400);
	cfsetospeed(&tty, B38400);
#endif
	if (tcsetattr(*minion, TCSANOW, &tty) < 0)
		error(ERROR_warn(0), "unable to set pty terminal attributes");
#ifdef TIOCSWINSZ
	if (winp && ioctl(*minion, TIOCSWINSZ, winp) < 0)
		error(ERROR_warn(0), "unable to set pty window size");
#endif
	fcntl(*master, F_SETFD, FD_CLOEXEC);
#if !O_cloexec
	fcntl(*minion, F_SETFD, FD_CLOEXEC);
#endif
#ifdef SIGTTOU
	sigprocmask(SIG_SETMASK, &oldset, NULL);
#endif
	alarm(0);
	return 0;
}

static Proc_t*
runcmd(char** argv, int minion, int session)
{
	int64_t	ops[4];

	if (session)
	{
		ops[0] = PROC_FD_CTTY(minion);
		ops[1] = 0;
	}
	else
	{
		ops[0] = PROC_FD_DUP(minion, 0, PROC_FD_CHILD);
		ops[1] = PROC_FD_DUP(minion, 1, PROC_FD_CHILD);
		ops[2] = PROC_FD_DUP(minion, 2, PROC_FD_CHILD);
		ops[3] = 0;
	}
	return procopen(argv[0], argv, NULL, ops, 0);
}

static int		ttyfd = -1;	/* file descriptor of pty's own terminal	*/
static struct termios	ttyattrs;	/* saved attributes of pty's own terminal	*/

/*
 * Restore the attributes of pty's own terminal. Nothing is done if they have
 * not been changed. This is called when process() returns and, via atexit(3),
 * when pty exits for another reason, so that a terminal is not left in raw
 * mode.
 */

static void
rawoff(void)
{
	if (ttyfd >= 0)
	{
		tcsetattr(ttyfd, TCSADRAIN, &ttyattrs);
		ttyfd = -1;
	}
}

/*
 * Put pty's own terminal in raw mode, saving its attributes for rawoff().
 * Returns nonzero on success. The VMIN and VTIME values make a read return as
 * soon as any input is available, so that input is passed on as it is typed.
 */

static int
rawon(Sfio_t* ip)
{
	struct termios	tty;
	int		fd = sffileno(ip);

	if (!isatty(fd) || tcgetattr(fd, &tty) < 0)
		return 0;
	ttyattrs = tty;
	tty.c_lflag &= (tcflag_t)~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | IEXTEN);
#ifdef ECHOCTL
	tty.c_lflag &= (tcflag_t)~ECHOCTL;
#endif
#ifdef ECHOKE
	tty.c_lflag &= (tcflag_t)~ECHOKE;
#endif
#ifdef ISIG
	tty.c_lflag &= (tcflag_t)~ISIG;
#endif
	tty.c_iflag &= (tcflag_t)~(ICRNL | INLCR | IGNCR | IXON | IXOFF | ISTRIP | BRKINT | INPCK | PARMRK);
	tty.c_oflag &= (tcflag_t)~OPOST;
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 0;
	if (tcsetattr(fd, TCSADRAIN, &tty) < 0)
		return 0;
	ttyfd = fd;
	atexit(rawoff);
	return 1;
}

/*
 * Turn off echoing on the command's pseudoterminal. This is for running a
 * non-dialogue command when pty has no terminal of its own; the input is then
 * written to the pseudoterminal from a pipe or a file, and has already been
 * echoed by the invoking terminal, so echoing it again at the pseudoterminal
 * would show it twice. The pseudoterminal stays in canonical mode, so commands
 * that read a line, such as 'read', continue to work normally.
 */

static void
noecho(int fd)
{
	struct termios	tty;

	if (tcgetattr(fd, &tty) < 0)
		error(ERROR_warn(0), "unable to get pty terminal attributes");
	tty.c_lflag &= (tcflag_t)~(ECHO | ECHONL);
	if (tcsetattr(fd, TCSANOW, &tty) < 0)
		error(ERROR_warn(0), "unable to set pty terminal attributes");
}

/*
 * Copy a chunk of the command's output to the standard output, dropping the
 * carriage returns of CR LF line endings if requested. A carriage return at
 * the very end of the chunk is held back instead of being written, as its
 * newline may be in the next chunk; *crpending_p flags this up.
 */

static int
output(char* s, ssize_t n, int stripcr, int* crpending_p)
{
	char*	e = s + n;
	char*	p;
	char*	q;

	if (*crpending_p)
	{
		*crpending_p = 0;
		if (*s != '\n' && sfputc(sfstdout, '\r') != '\r')
			return -1;
	}
	if (!stripcr)
		return sfwrite(sfstdout, s, (size_t)n) == n ? 0 : -1;
	if (e > s && e[-1] == '\r')
	{
		e--;
		*crpending_p = 1;
	}
	for (p = q = s; p < e; p++)
	{
		if (*p == '\r' && p[1] == '\n')
		{
			if (p > q && sfwrite(sfstdout, q, (size_t)(p - q)) != p - q)
				return -1;
			q = ++p;
		}
	}
	if (e > q && sfwrite(sfstdout, q, (size_t)(e - q)) != e - q)
		return -1;
	return 0;
}

/*
 * Pass input on to the command's terminal and return 1, or return 0 when the
 * input has ended.
 *
 * The input is read from the file descriptor rather than with sfgetr(), which
 * buffers input past the line it returns and leaves sfpoll() unable to see
 * that there is more of it: the rest of the input would then only be passed on
 * when output from the command happened to wake the loop in process().
 * sfreserve() is no good either, as it can return a one-byte NUL buffer at the
 * end of the input, which the command's terminal would echo.
 */

static int
forward(Sfio_t* mp, Sfio_t* ip, int raw, char* buf)
{
	ssize_t		r;
	char*		s;

	if ((r = read(sffileno(ip), buf, SFIO_BUFSIZE)) <= 0)
		return 0;
	if (!raw)
	{
		/*
		 * The command's terminal is in canonical mode and its line endings
		 * come from the keyboard as '\r', so give it a '\r' for each newline
		 * of the input.
		 */
		for (s = buf; s < buf + r; s++)
			if (*s == '\n')
				*s = '\r';
	}
	if (sfwrite(mp, buf, (size_t)r) != r || sfsync(mp))
		error(ERROR_SYSTEM|2, "write failed");
	return 1;
}

/*
 * default master dance
 */

static int
process(Sfio_t* mp, Sfio_t* lp, useconds_t delay, int timeout)
{
	int		i;
	int		n;
	int		raw = 0;	/* set if pty's own terminal is in raw mode */
	int		crpending = 0;	/* set if a carriage return is held back for the next chunk */
	int		ready = 0;	/* set if input is a regular file (always ready to read) */
	ssize_t		r;
	char*		s;
	char		buf[SFIO_BUFSIZE];
	Sfio_t*		ip;
	Sfio_t*		sps[2];
	struct stat	dst;
	struct stat	fst;

	NOT_USED(lp);
	NOT_USED(delay);
	NOT_USED(timeout);
	ip = sfstdin;
	if (!fstat(sffileno(ip), &dst) && !stat("/dev/null", &fst) && dst.st_dev == fst.st_dev && dst.st_ino == fst.st_ino)
		ip = NULL;
	else
	{
		/*
		 * Avoid both parent terminal and pseudoterminal echoing typed input;
		 * set the parent terminal (if any) to raw mode.
		 */
		raw = rawon(ip);
		/*
		 * A regular file is always ready to read. Though sfpoll() marks a regular
		 * file's stream ready, it still waits for the others to become ready. So the
		 * regular file's data would not be passed on until the command produced
		 * output, and any command that waits for that data would then wait forever.
		 * Therefore, read it without polling until it terminates. This avoids a hang
		 * in something like 'pty cat <file'.
		 */
		ready = !fstat(sffileno(ip), &dst) && S_ISREG(dst.st_mode);
	}
	for (;;)
	{
		if (mp && ip && ready && !forward(mp, ip, raw, buf))
		{
			ip = NULL;
			ready = 0;
		}
		i = 0;
		if (mp)
			sps[i++] = mp;
		if (ip && !ready)
			sps[i++] = ip;
		if (!i)
			break;
		/*
		 * Wait for the command to produce output or for input to pass on.
		 * While a file is being read, do not wait: more of it is passed on regardless.
		 */
		if ((n = sfpoll(sps, i, (ip && ready) ? 0 : -1)) <= 0)
		{
			if (n < 0)
				error(ERROR_SYSTEM|2, "poll failed");
			if (!(ip && ready))
				break;
		}
		for (i = 0; i < n; i++)
		{
			if (!(sfvalue(sps[i]) & SFIO_READ))
				/*skip*/;
			else if (sps[i] == mp)
			{
				if (!(s = (char*)sfreserve(mp, SFIO_UNBOUND, -1)))
				{
					sfclose(mp);
					mp = NULL;
				}
				else if ((r = sfvalue(mp)) > 0 && (output(s, r, !raw, &crpending) < 0 || sfsync(sfstdout)))
				{
					error(ERROR_SYSTEM|2, "output write failed");
					goto done;
				}
			}
			else if (!forward(mp, ip, raw, buf))
			{
				ip = NULL;
				ready = 0;
			}
		}
		if (!mp)
			break;	/* the command has terminated; nothing can consume input */
	}
 done:
	if (crpending)
		sfputc(sfstdout, '\r');
	rawoff();
	if (mp)
		sfclose(mp);
	return error_info.errors != 0;
}

/*
 * return 1 if extended RE <pattern> matches <text>
 */

static int
match(char* pattern, char* text, int must)
{
	regex_t*	re;
	int		code;
	char		buf[64];

	if (!pattern[0])
		return 1;
	if (pattern[0] == '?' && pattern[1] && !pattern[2])
	{
		switch (pattern[1])
		{
		case '0':
		case '1':
			if (text)
				error(2, "got \"%s\"", fmtesq(text, "\""));
			else
				error(2, "got EOF");
			return pattern[1] == '1';
		case '.':
			if (!text)
				return 1;
			if (must)
				error(2, "expected EOF, got \"%s\"", fmtesq(text, "\""));
			return 0;
		}
	}
	if (!text)
	{
		if (must)
			error(2, "expected \"%s\", got EOF", pattern);
		return 0;
	}
	if (!(re = regcache(pattern, REG_EXTENDED, &code)))
	{
		regerror(code, re, buf, sizeof(buf));
		error(2, "%s: %s", pattern, buf);
		return 0;
	}
	if (regexec(re, text, 0, NULL, 0))
	{
		if (must)
			error(2, "expected \"%s\", got \"%s\"", pattern, fmtesq(text, "\""));
		return 0;
	}
	return 1;
}

/*
 * Time, in milliseconds, to wait for the rest of a line that is otherwise
 * ready to be returned (see masterline()). A newline that ends the line, or
 * the rest of a redraw, is nearly always already on its way when the text
 * matches, so a short wait keeps the line intact without stalling the test.
 */

#define GRACE	50

/*
 * Return true if regex <pattern> does *not* end in a newline.
 */

static int
unterminated(const char* pattern)
{
	size_t		n = strlen(pattern);

	if (n > 0 && pattern[n - 1] == '$')
		n--;
	if (n >= 2 && pattern[n - 2] == '\\' && pattern[n - 1] == 'n')
		return 0;
	if (n >= 1 && pattern[n - 1] == '\n')
		return 0;
	return 1;
}

/*
 * Time, in milliseconds, to wait for the shell's line editor to take over the
 * terminal after it has displayed a prompt (see waitraw()).
 */

#define RAWWAIT	500

/*
 * Wait until the terminal is in raw mode.
 *
 * In the following idiomatic sequence in a pty dialogue:
 *	p :test-1:
 *	w some command
 * the p command peeks for the shell prompt (PS1), then the w command writes
 * immediately after it has been displayed. But ksh only switches the terminal
 * away from canonical mode after printing the prompt. So there was a race
 * condition: typing could start before the terminal is in raw mode, causing
 * typeahead to be echoed, which interferes with the tests reading back input.
 * To fix the race, this function is used to wait until the terminal is in
 * raw mode before returning the prompt to the caller.
 *
 * A prompt that does not come from the editor (e.g., the read builtin's)
 * stays in canonical mode; for those, we give up after the timeout.
 */

static void
waitraw(Sfio_t* mp)
{
	struct termios	term;
	int		fd = sffileno(mp);
	int		n;

	for (n = 0; n < RAWWAIT; n += 10)
	{
		if (tcgetattr(fd, &term) < 0 || !(term.c_lflag & ICANON))
			return;
		usleep(10 * 1000);
	}
}

typedef struct Master_s
{
	Vmalloc_t*	vm;		/* allocation region			*/
	char*		ignore;		/* ignore master lines matching this re	*/
	char*		peek;		/* peek buffer pointer			*/
	char*		cur;		/* current line				*/
	char*		nxt;		/* next line				*/
	char*		end;		/* end of lines				*/
	char*		max;		/* end of buf				*/
	char*		buf;		/* current buffer			*/
	char*		prompt;		/* peek prompt				*/
	char*		partial;	/* text of an unfinished line		*/
	size_t		partiallen;	/* length of the text in partial	*/
	size_t		partialmax;	/* allocated size of partial		*/
	int		line;		/* prompt line number			*/
	char		save;		/* previous line's saved char		*/
	char		saving;		/* set if saved char is to be restored	*/
} Master_t;

/*
 * Make room for size bytes in the buffer that holds the text of an unfinished
 * line (see below).
 */

static void
reservepartial(Master_t* bp, size_t size)
{
	if (size > bp->partialmax)
	{
		bp->partialmax = roundof(size, (size_t)SFIO_BUFSIZE);
		if (bp->partial)
			bp->partial = vmresize(bp->vm, bp->partial, bp->partialmax);
		else
			bp->partial = vmnewof(bp->vm, NULL, char, bp->partialmax, 0);
	}
}

/*
 * Remember a line that was left unfinished, so that the next output from the
 * master can be joined to it (a redraw can be split over several reads).
 */

static void
savepartial(Master_t* bp, char* r)
{
	size_t	len = strlen(r);

	if (r != bp->partial)
	{
		reservepartial(bp, len + 1);
		memcpy(bp->partial, r, len + 1);
	}
	bp->partiallen = len;
}

/*
 * Return a line that continues one returned earlier: the unfinished line's
 * text is in bp->partial, and the new line is appended to it, so that the
 * caller sees the line as a terminal would. The unfinished line cannot simply
 * be pointed at in the buffer: it was returned at a read timeout, which resets
 * the buffer, so in the buffer the new line does not follow it, and pointing
 * before the buffer underflowed it.
 */

static char*
continueline(Master_t* bp, char* r)
{
	size_t	len = strlen(r);

	reservepartial(bp, bp->partiallen + len + 1);
	memcpy(bp->partial + bp->partiallen, r, len + 1);
	return bp->partial;
}

/*
 * Collapse the redraw sequences in a line of master output to the text that a
 * terminal would display: a carriage return moves the cursor to the start of
 * the line, a backspace moves it back one character, a bell has no visible
 * effect, and every other character overwrites what was at the cursor. So the
 * line 'abc\rde' displays as 'de', and what a redraw does not overwrite is
 * dropped once the line is complete: the shell rewrites the whole line when it
 * redraws. Returns nonzero if the line is complete, i.e., terminated by a
 * newline, and zero if the remaining output continues it. The redrawsback flag
 * is set if the line ends in the middle of a redraw, i.e., the cursor moved
 * back over text that a later redraw is going to overwrite.
 */

static int
collapse(char* r, int* redrawsback)
{
	char*	s = r;		/* scan the raw output			*/
	char*	c = r;		/* cursor in the collapsed line		*/
	char*	w = r;		/* end of the collapsed line		*/
	int	ch;
	int	back = 0;	/* a redraw moved the cursor back	*/

	while ((ch = (unsigned char)*s++))
	{
		if (ch == '\n')
		{
			w = c;
			*w++ = '\n';
			*w = 0;
			return 1;
		}
		if (ch == '\r' && *s != '\n')
		{
			c = r;
			back = 1;
		}
		else if (ch == '\b')
		{
			if (c > r)
			{
				c--;
				back = 1;
			}
		}
		else if (ch == '\a')
			/*skip*/;
		else
		{
			if (c < w)
				*c = (char)ch;
			else
				*w++ = (char)ch;
			c++;
		}
	}
	*w = 0;
	*redrawsback = back;
	return 0;
}

/*
 * read one line from the master
 */

#define MASTER_EOF	(-1)
#define MASTER_TIMEOUT	(-2)

/*
 * expand the prompt for the next line
 */

static char*
masterprompt(Master_t* bp)
{
	static char	buf[64];

	if (!bp->prompt)
		return NULL;
	sfsprintf(buf, sizeof(buf), bp->prompt, ++bp->line);
	return buf;
}

static char*
masterline(Sfio_t* mp, Sfio_t* lp, char* prompt, char* want, int must, int timeout, Master_t* bp)
{
	char*		r;
	char*		s;
	ssize_t		n;
	size_t		promptlen = 0;
	int		redraw;

	if (prompt)
		promptlen = strlen(prompt);
 again:
	if (prompt)
	{
		if (bp->cur < bp->end && bp->saving)
			*bp->cur = bp->save;
		/* avoid matching against consumed data: only match if the buffer hasn't been reset yet */
		if (bp->cur < bp->end && strneq(bp->cur, prompt, promptlen))
			r = bp->cur;
		else
			r = NULL;
		if (r)
		{
			/* leave the matched line intact for the caller */
			if (bp->cur < bp->end && bp->saving)
				bp->saving = 0;
			waitraw(mp);
			error(-1, "p \"%s\"", fmtnesq(prompt, "\"", promptlen));
			return r;
		}
		if (bp->cur < bp->end && bp->saving)
			*bp->cur = 0;
		if (r = bp->nxt)
		{
			if (strneq(r, prompt, promptlen))
			{
				waitraw(mp);
				error(-1, "p \"%s\"", fmtnesq(prompt, "\"", promptlen));
				return r;
			}
			while (r = memchr(r, '\n', (size_t)(bp->end - r)))
			{
				if (strneq(++r, prompt, promptlen))
				{
					waitraw(mp);
					error(-1, "p \"%s\"", fmtnesq(prompt, "\"", promptlen));
					return r;
				}
			}
		}
		/*
		 * Do not truncate unconsumed input here: the prompt may be split
		 * over several reads, in which case it only matches once all of
		 * its bytes have arrived and been preserved.
		 */
		if (bp->cur >= bp->end)
			*bp->cur = 0;
	}
	else if (bp->nxt)
	{
		if (bp->saving)
			*bp->cur = bp->save;
		r = bp->cur;
		bp->save = *bp->nxt;
		bp->saving = 1;
		*bp->nxt = 0;
		if (bp->nxt >= bp->end)
		{
			bp->cur = bp->end = bp->buf;
			bp->nxt = NULL;
		}
		else
		{
			bp->cur = bp->nxt;
			if (bp->nxt = memchr(bp->nxt + 1, '\n', (size_t)(bp->end - bp->nxt - 1)))
				bp->nxt++;
		}
		goto done;
	}
	else if (want && bp->cur < bp->end && unterminated(want))
	{
		/*
		 * The shell has written something that is not terminated by a newline: a
		 * command line that has been echoed but not yet run, for instance. Such text
		 * is normally only returned when the poll below times out, so a test that
		 * reads what the shell has already written would sit there for the whole
		 * timeout. If no further output is available and the text already matches
		 * what the caller is waiting for, then return it now-ish instead of waiting.
		 */
		if (bp->saving)
			*bp->cur = bp->save;
		r = bp->cur;
		n = sfpoll(&mp, 1, GRACE);
		if (!memchr(r, '\n', (size_t)(bp->end - r)) && (n <= 0 || !(sfvalue(mp) & SFIO_READ)) && match(want, r, 0))
		{
			bp->saving = 0;
			*bp->end = 0;
			bp->nxt = NULL;
			bp->cur = bp->end = bp->buf;
			goto done;
		}
		if (bp->saving)
			*bp->cur = 0; /* restore the hidden line terminator */
	}
	/*
	 * A master opened for reading and writing is reported as SFIO_WRITE
	 * ready even when the poll times out, so a timeout has to be detected
	 * as an absence of SFIO_READ rather than as a zero return value.
	 */
	if ((n = sfpoll(&mp, 1, timeout)) <= 0 || !(sfvalue(mp) & SFIO_READ))
	{
		if (n < 0)
		{
			if (must)
				error(ERROR_SYSTEM|2, "poll failed");
			else
				error(-1, "r poll failed");
		}
		else if (bp->cur < bp->end)
		{
			if (bp->saving)
			{
				*bp->cur = bp->save;
				bp->saving = 0;
			}
			r = bp->cur;
			*bp->end = 0;
			bp->nxt = NULL;
			if (prompt && strneq(r, prompt, promptlen))
			{
				error(-1, "p \"%s\"", fmtnesq(prompt, "\"", promptlen));
				return r;
			}
			bp->cur = bp->end = bp->buf;
			goto done;
		}
		else if (must >= 0)
			error(2, "read timeout");
		else
		{
			errno = 0;
			error(-1, "r EOF");
		}
		return NULL;
	}
	if (!(s = sfreserve(mp, SFIO_UNBOUND, -1)))
	{
		if (!prompt)
		{
			if (bp->cur < bp->end)
			{
				if (bp->saving)
				{
					*bp->cur = bp->save;
					bp->saving = 0;
				}
				r = bp->cur;
				*bp->end = 0;
				bp->cur = bp->end = bp->buf;
				bp->nxt = NULL;
				goto done;
			}
			else
			{
				errno = 0;
				error(-1, "r EOF");
			}
		}
		return NULL;
	}
	n = sfvalue(mp);
	error(-2, "b \"%s\"", fmtnesq(s, "\"", (size_t)n));
	if ((bp->max - bp->end) < n)
	{
		size_t	new_buf_size;
		r = bp->buf;
		new_buf_size = (size_t)roundof(bp->max - bp->buf + 1 + n, SFIO_BUFSIZE);
		bp->buf = vmresize(bp->vm, bp->buf, new_buf_size);
		bp->max = bp->buf + new_buf_size - 1;
		if (bp->buf != r)
		{
			ptrdiff_t d = bp->buf - r;
			bp->cur += d;
			bp->end += d;
		}
	}
	memcpy(bp->end, s, (size_t)n);
	bp->end += n;
	if ((r = bp->cur) > bp->buf && bp->saving)
		*r = bp->save;
	if (bp->cur = memchr(bp->cur, '\n', (size_t)(bp->end - bp->cur)))
	{
		bp->save = *++bp->cur;
		bp->saving = 1;
		*bp->cur = 0;
		if (bp->cur >= bp->end)
		{
			bp->cur = bp->end = bp->buf;
			bp->nxt = NULL;
		}
		else if (bp->nxt = memchr(bp->cur + 1, '\n', (size_t)(bp->end - bp->cur - 1)))
			bp->nxt++;
		if (prompt)
			goto again;
	}
	else
	{
		bp->saving = 0;
		bp->cur = r;
		bp->nxt = NULL;
		must = 0;
		goto again;
	}
 done:
	error(-3, "Q \"%s\"", fmtesq(r, "\""));
	/*
	 * If the last line returned was left unfinished by a redraw, then this
	 * one continues it: put the two together so that the caller sees the
	 * whole line. A complete line is finished here, and one that is still
	 * unfinished is remembered for the next output to continue.
	 */
	if (bp->partiallen)
		r = continueline(bp, r);
	if (collapse(r, &redraw) == 0 && redraw)
		savepartial(bp, r);
	else
		bp->partiallen = 0;
	error(-1, "r \"%s\"", fmtesq(r, "\""));
	if (lp)
		sfputr(lp, fmtesq(r, "\""), '\n');
	if (bp->ignore && match(bp->ignore, r, 0))
		goto again;
	return r;
}

/*
 * execute dialogue script on stdin
 */

#define NESTING	64

#define ELSE	0x01
#define IF	0x02
#define KEPT	0x04
#define SKIP	0x08

struct Cond_s;
typedef struct Cond_s Cond_t;

struct Cond_s
{
	Cond_t*	next;
	Cond_t*	prev;
	char*	text;
	int	flags;
};

static int
dialogue(Sfio_t* mp, Sfio_t* lp, useconds_t delay, int timeout)
{
	unsigned long long	ull;
	useconds_t		ud;
	int			op;
	int			line;
	intmax_t		n;
	char*			s;
	char*			m;
	char*			e;
	char*			id;
	Vmalloc_t*		vm;
	Cond_t*			cond;
	Master_t*		master;
	int			status = 0;

	if (!(vm = vmopen()))
		outofmemory(0);
	vm->options = VM_INIT | VM_FREEONFAIL;
	vm->outofmemory = outofmemory;
	cond = vmnewof(vm, NULL, Cond_t, 1, 0);
	master = vmnewof(vm, NULL, Master_t, 1, 0);
	master->vm = vm;
	master->buf = vmnewof(vm, NULL, char, 2 * SFIO_BUFSIZE, 0);
	master->cur = master->end = master->buf;
	master->max = master->buf + 2 * SFIO_BUFSIZE - 1;
	master->saving = 0;
	errno = 0;
	id = error_info.id;
	error_info.id = NULL;
	line = error_info.line;
	error_info.line = 0;
	while (s = sfgetr(sfstdin, '\n', 1))
	{
		error_info.line++;
		while (isspace(*s))
			s++;
		if ((op = *s++) && isspace(*s))
			s++;
		switch (op)
		{
		case 0:
		case '#':
			break;
		case 'c':
		case 'w':
			if (cond->flags & SKIP)
				continue;
			if ((m = masterprompt(master)) && !masterline(mp, lp, m, NULL, 0, timeout, master))
				goto done;
			if (delay)
				usleep(delay * 1000);
			if (op == 'w')
				error(-1, "w \"%s\\r\"", s);
			else
				error(-1, "w \"%s\"", s);
			if ((n = stresc(s)) >= 0)
				s[n] = 0;
			if (sfputr(mp, s, op == 'w' ? '\n' : -1) < 0 || sfsync(mp))
			{
				error(ERROR_SYSTEM|2, "write failed");
				goto done;
			}
			if (delay)
				usleep(delay * 1000);
			break;
		case 'd':
			errno = 0;
			ull = strtoull(s, &e, 0);
			delay = (useconds_t)ull;
			if (delay != ull)
				errno = ERANGE;
			if (*e || errno == ERANGE)
			{
				error(ERROR_SYSTEM|2, "%s: invalid delay -- milliseconds expected", s);
				goto done;
			}
			break;
		case 'i':
			if (!cond->next)
				cond->next = vmnewof(vm, NULL, Cond_t, 1, 0);
			cond = cond->next;
			cond->flags = IF;
			if ((cond->prev->flags & SKIP) && !(cond->text = NULL) || !(cond->text = masterline(mp, lp, NULL, NULL, 0, timeout, master)))
				cond->flags |= KEPT|SKIP;
			else if (match(s, cond->text, 0))
				cond->flags |= KEPT;
			else
				cond->flags |= SKIP;
			break;
		case 'e':
			if (!(cond->flags & IF))
			{
				error(2, "no matching i for e");
				goto done;
			}
			if (!*s)
			{
				if (cond->flags & ELSE)
				{
					error(2, "i block already has a default e");
					goto done;
				}
				cond->flags |= ELSE;
				if (cond->flags & KEPT)
					cond->flags |= SKIP;
				else
				{
					cond->flags |= KEPT;
					cond->flags &= ~SKIP;
				}
			}
			else if ((cond->flags & KEPT) || !match(s, cond->text, 0))
				cond->flags |= SKIP;
			else
				cond->flags |= KEPT;
			break;
		case 'f':
			if (!(cond->flags & IF))
			{
				error(2, "no matching i for f");
				goto done;
			}
			cond = cond->prev;
			break;
		case 'm':
			if (cond->flags & SKIP)
				continue;
			if (sfputr(sfstderr, s, '\n') < 0 || sfsync(sfstderr))
			{
				error(ERROR_SYSTEM|2, "standard error write failed");
				goto done;
			}
			break;
		case 'p':
			if (cond->flags & SKIP)
				continue;
			if (!(m = masterline(mp, lp, s, NULL, 1, timeout, master)))
				goto done;
			if (!strneq(m, s, strlen(s)))
			{
				error(2, "expected \"%s\", got \"%s\"", s, fmtesq(m, "\""));
				goto done;
			}
			break;
		case 'r':
			if (cond->flags & SKIP)
				continue;
			if (!(m = masterline(mp, lp, NULL, s, s[0] == '?' && s[1] == '.' ? -1 : 1, timeout, master)))
				goto done;
			match(s, m, 1);
			break;
		case 's':
			errno = 0;
			ull = strtoull(s, &e, 0);
			ud = (useconds_t)ull;
			if (ull != ud)
				errno = ERANGE;
			if (*e || errno == ERANGE)
			{
				error(ERROR_SYSTEM|2, "%s: invalid delay -- milliseconds expected", s);
				goto done;
			}
			if (ud)
				usleep(ud * 1000);
			break;
		case 't':
			errno = 0;
			timeout = strtoi(s, &e, 0);
			if (*e || errno == ERANGE)
			{
				error(ERROR_SYSTEM|2, "%s: invalid timeout -- milliseconds expected", s);
				goto done;
			}
			break;
		case 'u':
			if (cond->flags & SKIP)
				continue;
			do
			{
				if (!(m = masterline(mp, lp, NULL, s, -1, timeout, master)))
				{
					match(s, m, 1);
					goto done;
				}
			} while (!match(s, m, 0));
			break;
		case 'v':
			errno = 0;
			error_info.trace = -strtoi(s, &e, 0);
			if (*e || errno == ERANGE)
			{
				error(ERROR_SYSTEM|2, "%s: invalid verbose level -- number expected", s);
				goto done;
			}
			break;
		case 'x':
			errno = 0;
			status = strtoi(s, &e, 0);
			if (*e || errno == ERANGE)
			{
				error(ERROR_SYSTEM|2, "%s: invalid exit code", s);
				goto done;
			}
			break;
		case 'I':
			if (master->ignore)
			{
				vmfree(vm, master->ignore);
				master->ignore = NULL;
			}
			if (*s)
				master->ignore = vmstrdup(vm, s);
			break;
		case 'L':
			if (error_info.id)
			{
				vmfree(vm, error_info.id);
				error_info.id = NULL;
			}
			if (*s)
				error_info.id = vmstrdup(vm, s);
			break;
		case 'P':
			if (master->prompt)
			{
				vmfree(vm, master->prompt);
				master->prompt = NULL;
			}
			if (*s)
				master->prompt = vmstrdup(vm, s);
			break;
		default:
			if (cond->flags & SKIP)
				continue;
			error(2, "'%c': unknown op", op);
			goto done;
		}
	}
	if (cond->prev)
		error(2, "missing 1 or more f statements");
 done:
	if (mp)
		sfclose(mp);
	error_info.id = id;
	error_info.line = line;
	vmclose(vm);
	return status ? status : error_info.errors != 0;
}

typedef struct Argv_s
{
	char**	argv;
	char*	args;
	int	argc;
} Argv_t;

int
b_pty(int argc, char** argv, Shbltin_t* context)
{
	int		master;
	int		minion;
	int		master_status;
	int		minion_status;
	int		fd;
	int		drop;
	int		n;
	char*		s;
	Proc_t*		proc;
	Sfio_t*		mp;
	Sfio_t*		lp;
	char		buf[64];

	useconds_t	delay = 0;
	char*		myname = argv[0];
	char*		log = NULL;
	char*		messages = NULL;
	char*		stty = NULL;
	int		session = 1;
	int		timeout = 1000;
	int		run_dialogue = 0;

	/*
	 * In case pty was invoked as a background job, unignore SIGINT
	 * and SIGQUIT; pty tests need these signals to work normally.
	 */
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);

	cmdinit(argc, argv, context, ERROR_CATALOG, 0);
	for (;;)
	{
		switch (optget(argv, usage))
		{
		case 'd':
			if (opt_info.num)
				run_dialogue = 1;
			continue;
		case 'D':
			error_info.trace = -(int)opt_info.num;
			continue;
		case 'l':
			log = opt_info.arg;
			/* FALLTHROUGH */
		case 'm':
			messages = opt_info.arg;
			continue;
		case 's':
			session = !!opt_info.num;
			continue;
		case 't':
			timeout = (int)opt_info.num;
			continue;
		case 'T':
			stty = opt_info.arg;
			continue;
		case 'w':
			delay = (useconds_t)opt_info.num;
			continue;
		case ':':
			break;
		case '?':
			return optselfdoc();
		}
		break;
	}
	argv += opt_info.index;
	if (!argv[0])
	{
		error(ERROR_exit(1), "command must be specified");
		UNREACHABLE();
	}
	if (mkpty(&master, &minion) < 0)
	{
		error(ERROR_system(1), "unable to create pty");
		UNREACHABLE();
	}
	if (!(mp = sfnew(NULL, NULL, (size_t)SFIO_UNBOUND, master, SFIO_READ|SFIO_WRITE)))
	{
		error(ERROR_system(1), "cannot open master stream");
		UNREACHABLE();
	}
	if (!run_dialogue && !isatty(sffileno(sfstdin)))
		noecho(minion);
	if (stty)
	{
		Argv_t* ap;
		size_t sz;
		n = 2;
		for (s = stty; *s; s++)
			if (isspace(*s))
				n++;
		sz = (size_t)(n + 2) * sizeof(char*) + (size_t)(s - stty + 1);
		if (!(ap = newof(0, Argv_t, 1, sz)))
			outofmemory(sz);
		ap->argc = n + 1;
		ap->argv = (char**)(ap + 1);
		ap->args = (char*)(ap->argv + n + 2);
		strcpy(ap->args, stty);
		/* set stty's argv[0] to our own, otherwise future error messages will ID as stty instead of pty */
		ap->argv[0] = myname;
		sfsprintf(ap->argv[1] = buf, sizeof(buf), "--fd=%d", minion);
		ap->argv[2] = s = ap->args;
		for (n = 2; *s; s++)
		{
			if (isspace(*s))
			{
				*s = 0;
				ap->argv[++n] = s + 1;
			}
		}
		ap->argv[n + 1] = NULL;
		b_stty(ap->argc, ap->argv, NULL);
		free(ap);
	}
	if (!log)
		lp = NULL;
	else if (!(lp = sfopen(NULL, log, "w")))
	{
		error(ERROR_system(1), "%s: cannot write", log);
		UNREACHABLE();
	}
	if (!(proc = runcmd(argv, minion, session)))
	{
		error(ERROR_system(1), "unable to run %s", argv[0]);
		UNREACHABLE();
	}
	ast_close(minion);
	if (messages)
	{
		drop = 1;
		if (strneq(messages, "/dev/fd/", 8))
			fd = atoi(messages + 8);
		else if (streq(messages, "/dev/stdout"))
			fd = 1;
		else if ((fd = open(messages, O_CREAT|O_WRONLY, MODE_666)) >= 0)
			drop = 0;
		else
		{
			error(ERROR_system(1), "%s: cannot redirect messages", messages);
			UNREACHABLE();
		}
		ast_dup2(fd, 2);
		if (drop)
			ast_close(fd);
	}
	minion_status = (run_dialogue ? dialogue : process)(mp, lp, delay, timeout);
	master_status = procclose(proc);
	if (lp && sfclose(lp))
	{
		error(ERROR_system(1), "%s: write error", log);
		UNREACHABLE();
	}
	if (error_info.errors)
		return 1;
	/*
	 * A dialogue hangs the command up when it is done with it, so its exit
	 * status says nothing about the dialogue: the status is then that of the
	 * dialogue itself, which is the code of its 'x' command.
	 */
	if (run_dialogue)
		return minion_status;
	/*
	 * No dialogue: the exit status is that of the command, or the
	 * number of the signal that terminated it with its 8th bit set.
	 */
	if (master_status & (1 << EXIT_BITS))
		master_status = 128 + EXIT_CODE(master_status);
	return master_status;
}

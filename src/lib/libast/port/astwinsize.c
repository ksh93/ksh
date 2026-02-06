/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
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
*                   Phong Vo <kpv@research.att.com>                    *
*                  Martijn Dekker <martijn@inlv.org>                   *
*               Anuradha Weeraman <anuradha@debian.org>                *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/

/*
 * AT&T Research
 * return terminal rows and cols
 */

#include <ast.h>
#include <ast_tty.h>

#if _sys_ioctl
#include <sys/ioctl.h>
#endif

#undef ioctl
#undef sleep
#define ioctl		______ioctl
#define sleep		______sleep

#if defined(TIOCGWINSZ)
#if _sys_stream && _sys_ptem
#include <sys/stream.h>
#include <sys/ptem.h>
#endif
#else
#if !defined(TIOCGSIZE) && !defined(TIOCGWINSZ)
#if _hdr_jioctl
#define jwinsize	winsize
#include <jioctl.h>
#else
#if _sys_jioctl
#define jwinsize	winsize
#include <sys/jioctl.h>
#endif
#endif
#endif
#endif

#undef	ioctl
#undef	sleep

static int		ttctl(int, int, void*);

void
astwinsize(int fd, int* rows, int* cols)
{
#ifdef	TIOCGWINSZ
#define NEED_ttctl
	struct winsize	ws;

	if (!ttctl(fd, TIOCGWINSZ, &ws) && ws.ws_col > 0 && ws.ws_row > 0)
	{
		if (rows) *rows = ws.ws_row;
		if (cols) *cols = ws.ws_col;
	}
	else
#else
#ifdef	TIOCGSIZE
#define NEED_ttctl
	struct ttysize	ts;

	if (!ttctl(fd, TIOCGSIZE, &ts) && ts.ts_lines > 0 && ts.ts_cols > 0)
	{
		if (rows) *rows = ts.ts_lines;
		if (cols) *cols = ts.ts_cols;
	}
	else
#else
#ifdef	JWINSIZE
#define NEED_ttctl
	struct winsize	ws;

	if (!ttctl(fd, JWINSIZE, &ws) && ws.bytesx > 0 && ws.bytesy > 0)
	{
		if (rows) *rows = ws.bytesy;
		if (cols) *cols = ws.bytesx;
	}
	else
#endif
#endif
#endif
	{
		char*		s;

		if (rows) *rows = (s = getenv("LINES")) ? strtol(s, NULL, 0) : 0;
		if (cols) *cols = (s = getenv("COLUMNS")) ? strtol(s, NULL, 0) : 0;
	}
}

#ifdef	NEED_ttctl

/*
 * tty ioctl() -- no cache
 */

static int
ttctl(int fd, int op, void* tt)
{
	int	v;

	if (fd < 0)
	{
		for (fd = 0; fd <= 2; fd++)
			if (!ioctl(fd, op, tt)) return 0;
		if ((fd = open("/dev/tty", O_RDONLY|O_cloexec)) >= 0)
		{
			v = ioctl(fd, op, tt);
			ast_close(fd);
			return v;
		}
	}
	else if (!ioctl(fd, op, tt)) return 0;
	return -1;
}

#endif

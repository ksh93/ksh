/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2025 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                  David Korn <dgk@research.att.com>                   *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*               Anuradha Weeraman <anuradha@debian.org>                *
*                                                                      *
***********************************************************************/

#ifndef _terminal_
#define _terminal_	1

/*
 * terminal interface
 */

#include <ast_tty.h>

extern int	tty_alt(int);
extern void	tty_cooked(int);
extern int	tty_get(int,struct termios*);
extern int	tty_raw(int,int);
extern int	tty_check(int);
extern int	tty_set(int, int, struct termios*);
extern int	sh_tcgetattr(int,struct termios*);
extern int	sh_tcsetattr(int,int,struct termios*);

#undef	tcgetattr
#undef	tcsetattr
#define tcgetattr(a,b)		sh_tcgetattr(a,b)
#define tcsetattr(a,b,c)	sh_tcsetattr(a,b,c)

#endif /* _terminal_ */

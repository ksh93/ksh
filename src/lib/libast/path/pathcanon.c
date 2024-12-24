/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2024 Contributors to ksh 93u+m           *
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
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
/*
 * Glenn Fowler
 * AT&T Research
 *
 * in-place path name canonicalization -- preserves the logical view
 * pointer to trailing 0 in path returned
 *
 *	remove redundant .'s and /'s
 *	move ..'s to the front
 *	/.. preserved (for pdu and newcastle hacks)
 *
 * longer pathname possible if (flags&PATH_PHYSICAL) involved
 * 0 returned on error and if (flags&(PATH_DOTDOT|PATH_EXISTS)) then path
 * will contain the components following the failure point
 */

#define _AST_API_H	1

#include <ast.h>
#include <ls.h>
#include <error.h>

char*
pathcanon(char* path, int flags)
{
	return pathcanon_20100601(path, PATH_MAX, flags);
}

#undef	_AST_API_H

#include <ast_api.h>

char*
pathcanon_20100601(char* path, size_t size, int flags)
{
	char*	p;
	char*	r;
	char*	s;
	char*	t;
	int	dots;
	char*	phys;
	char*	v;
	int	loop;
	int	oerrno;

	oerrno = errno;
	dots = loop = 0;
	phys = path;
	v = path + ((flags >> 5) & 01777);
	if (!size)
		size = strlen(path) + 1;
	if (*path == '/')
	{
		if (*(path + 1) == '/' && *astconf("PATH_LEADING_SLASHES", NULL, NULL) == '1')
			do path++; while (*path == '/' && *(path + 1) == '/');
		if (!*(path + 1))
			return path + 1;
	}
	p = r = s = t = path;
	for (;;)
		switch (*t++ = *s++)
		{
		case '.':
			dots++;
			break;
		case 0:
			s--;
			/* FALLTHROUGH */
		case '/':
			while (*s == '/') s++;
			switch (dots)
			{
			case 1:
				t -= 2;
				break;
			case 2:
				if ((flags & (PATH_DOTDOT|PATH_EXISTS)) == PATH_DOTDOT && (t - 2) >= v)
				{
					struct stat	st;

					*(t - 2) = 0;
					if (stat(phys, &st))
					{
						strcpy(path, s);
						return NULL;
					}
					*(t - 2) = '.';
				}
				if (t - 5 < r)
				{
					if (t - 4 == r) t = r + 1;
					else r = t;
				}
				else for (t -= 5; t > r && *(t - 1) != '/'; t--);
				break;
			case 3:
				r = t;
				break;
			default:
				if ((flags & PATH_PHYSICAL) && loop < 32 && (t - 1) > path)
				{
					char	c;
					char	buf[PATH_MAX];

					c = *(t - 1);
					*(t - 1) = '\0';
					dots = pathgetlink(phys, buf, sizeof(buf));
					*(t - 1) = c;
					if (dots > 0)
					{
						loop++;
						strcpy(buf + dots, s - (*s != 0));
						if (*buf == '/') p = r = path;
						v = s = t = p;
						strcpy(p, buf);
					}
					else if (dots < 0 && errno == ENOENT)
					{
						if (flags & PATH_EXISTS)
						{
							strcpy(path, s);
							return NULL;
						}
						flags &= ~(PATH_PHYSICAL|PATH_DOTDOT);
					}
					dots = 4;
				}
				break;
			}
			if (dots >= 4 && (flags & PATH_EXISTS) && (t - 1) >= v && (t > path + 1 || t > path && *(t - 1) && *(t - 1) != '/'))
			{
				struct stat	st;

				*(t - 1) = 0;
				if (stat(phys, &st))
				{
					strcpy(path, s);
					return NULL;
				}
				v = t;
				if (*s) *(t - 1) = '/';
			}
			if (!*s)
			{
				if (t > path && !*(t - 1)) t--;
				if (t == path) *t++ = '.';
				else if ((s <= path || *(s - 1) != '/') && t > path + 1 && *(t - 1) == '/') t--;
				*t = 0;
				errno = oerrno;
				return t;
			}
			dots = 0;
			p = t;
			break;
		default:
			dots = 4;
			break;
		}
}

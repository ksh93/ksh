/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
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
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * UID number -> user name
 */

#define getpwuid	______getpwuid

#include <ast.h>
#include <cdt.h>
#include <pwd.h>

#undef	getpwuid

extern struct passwd*	getpwuid(uid_t);

typedef struct Id_s
{
	Dtlink_t	link;
	uid_t		id;
	char		name[1];
} Id_t;

/*
 * return user name for given UID number
 */

char*
fmtuid(uid_t uid)
{
	Id_t*		ip;
	char*		name;
	struct passwd*	pw;
	size_t		z;

	static Dt_t*		dict;
	static Dtdisc_t		disc;

	if (!dict)
	{
		disc.key = offsetof(Id_t, id);
		disc.size = sizeof(uid_t);
		dict = dtopen(&disc, Dtset);
	}
	else if (ip = (Id_t*)dtmatch(dict, &uid))
		return ip->name;
	if (pw = getpwuid(uid))
	{
		name = pw->pw_name;
#if _WINIX
		if (streq(name, "Administrator"))
			name = "root";
#endif
	}
	else if (uid == 0)
		name = "root";
	else
	{
		name = fmtbuf(z = sizeof(uid) * 3 + 1);
		sfsprintf(name, z, "%I*d", sizeof(uid), uid);
	}
	if (dict && (ip = newof(0, Id_t, 1, strlen(name))))
	{
		ip->id = uid;
		strcpy(ip->name, name);
		dtinsert(dict, ip);
		return ip->name;
	}
	return name;
}

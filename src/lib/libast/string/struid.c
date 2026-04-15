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
 * user name -> UID
 */

#define getpwnam	______getpwnam
#define getpwuid	______getpwuid

#include <ast.h>
#include <cdt.h>
#include <pwd.h>

#undef	getpwnam
#undef	getpwuid

extern struct passwd*	getpwnam(const char*);
extern struct passwd*	getpwuid(uid_t);

typedef struct Id_s
{
	Dtlink_t	link;
	int		id;
	char		name[1];
} Id_t;

/*
 * return UID number for given user name
 * -1 on first error for a given name
 * -2 on subsequent errors for a given name
 */

int
struid(const char* name)
{
	Id_t*		ip;
	struct passwd*	pw;
	int		id;
	char*		e;

	static Dt_t*	dict;
	static Dtdisc_t	disc;

	if (!dict)
	{
		disc.key = offsetof(Id_t, name);
		dict = dtopen(&disc, Dtset);
	}
	else if (ip = (Id_t*)dtmatch(dict, name))
		return ip->id;
	if (pw = getpwnam(name))
		id = (int)pw->pw_uid;
	else
	{
		id = (int)strtol(name, &e, 0);
#if _WINIX
		if (!*e)
		{
			if (!getpwuid((uid_t)id))
				id = -1;
		}
		else if (streq(name, "root") && (pw = getpwnam("Administrator")))
			id = (int)pw->pw_uid;
		else
			id = -1;
#else
		if (*e || !getpwuid((uid_t)id))
			id = -1;
#endif
	}
	if (dict && (ip = newof(0, Id_t, 1, strlen(name))))
	{
		strcpy(ip->name, name);
		ip->id = id >= 0 ? id : -2;
		dtinsert(dict, ip);
	}
	return id;
}

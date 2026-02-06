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
 * access() EUID/EGID implementation
 */

#include <ast.h>
#include <ls.h>

#include "FEATURE/eaccess"

#if _lib_eaccess

NoN(eaccess)

#else

extern int
eaccess(const char* path, int flags)
{
#ifdef EFF_ONLY_OK
	return access(path, flags|EFF_ONLY_OK);
#else
#if _lib_euidaccess
	return euidaccess(path, flags);
#else
	int		mode;
	struct stat	st;

	static int	init;
	static uid_t	ruid;
	static uid_t	euid;
	static gid_t	rgid;
	static gid_t	egid;

	if (!init)
	{
		ruid = getuid();
		euid = geteuid();
		rgid = getgid();
		egid = getegid();
		init = (ruid == euid && rgid == egid) ? 1 : -1;
	}
	if (init > 0 || flags == F_OK)
		return access(path, flags);
	if (stat(path, &st))
		return -1;
	mode = 0;
	if (euid == 0)
	{
		if (!S_ISREG(st.st_mode) || !(flags & X_OK) || (st.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)))
			return 0;
		goto nope;
	}
	else if (euid == st.st_uid)
	{
		if (flags & R_OK)
			mode |= S_IRUSR;
		if (flags & W_OK)
			mode |= S_IWUSR;
		if (flags & X_OK)
			mode |= S_IXUSR;
	}
	else if (egid == st.st_gid)
	{
#if _lib_getgroups
	setgroup:
#endif
		if (flags & R_OK)
			mode |= S_IRGRP;
		if (flags & W_OK)
			mode |= S_IWGRP;
		if (flags & X_OK)
			mode |= S_IXGRP;
	}
	else
	{
#if _lib_getgroups
		int	n;

		static int	ngroups = -2;
		static gid_t*	groups;

		if (ngroups == -2)
		{
			if ((ngroups = getgroups(0, NULL)) <= 0)
				ngroups = (int)astconf_long(CONF_NGROUPS_MAX);
			if (!(groups = newof(0, gid_t, ngroups + 1, 0)))
				ngroups = -1;
			else
				ngroups = getgroups(ngroups, groups);
		}
		n = ngroups;
		while (--n >= 0)
			if (groups[n] == st.st_gid)
				goto setgroup;
#endif
		if (flags & R_OK)
			mode |= S_IROTH;
		if (flags & W_OK)
			mode |= S_IWOTH;
		if (flags & X_OK)
			mode |= S_IXOTH;
	}
	if ((st.st_mode & mode) == mode)
		return 0;
 nope:
	errno = EACCES;
	return -1;
#endif
#endif
}

#endif

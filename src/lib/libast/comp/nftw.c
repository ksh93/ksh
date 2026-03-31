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
 * nftw implementation
 */

#include <ast.h>
#include <ftw.h>

static int	nftw_flags;
static int	(*nftw_userf)(const char*, const struct stat*, int, struct FTW*);

static int
nftw_user(Ftw_t* ftw)
{
	int		n = ftw->info;
	struct FTW	nftw;
	struct stat	st;

	if (n & (FTW_C|FTW_NX))
		n = FTW_DNR;
	else if ((n & FTW_SL) && (!(nftw_flags & FTW_PHYSICAL) || stat(ftw->path, &st)))
		n = FTW_SLN;
	nftw.base = (int)(ftw->pathlen - ftw->namelen);
	nftw.level = (int)ftw->level;
	nftw.quit = 0;
	n = (*nftw_userf)(ftw->path, &ftw->statb, n, &nftw);
	ftw->status = (short)nftw.quit;
	return n;
}

int
nftw(const char* path, int(*userf)(const char*, const struct stat*, int, struct FTW*), int depth, int flags)
{
	NoP(depth);
	nftw_userf = userf;
	if (flags & FTW_CHDIR) flags &= ~FTW_DOT;
	else flags |= FTW_DOT;
	nftw_flags = flags;
	return ftwalk(path, nftw_user, flags, NULL);
}

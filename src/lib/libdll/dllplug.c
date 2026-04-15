/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1997-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2026 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  Martijn Dekker <martijn@inlv.org>                   *
*                                                                      *
***********************************************************************/
/*
 * Glenn Fowler
 * AT&T Research
 */

#include "dlllib.h"

/*
 * find and load lib plugin/module library name with optional version ver and dlopen() flags
 * at least one dlopen() is called to initialize dlerror()
 * if path!=0 then library path up to size chars copied to path with trailing 0
 * if name contains a directory prefix then library search is limited to the dir and siblings
 */

extern void*
dllplugin(const char* lib, const char* name, const char* ver, unsigned long rel, unsigned long* cur, int flags, char* path, size_t size)
{
	void*		dll;
	int		err;
	int		hit;
	Dllscan_t*	dls;
	Dllent_t*	dle;

	err = hit = 0;
	for (;;)
	{
		if (dls = dllsopen(lib, name, ver))
		{
			while (dle = dllsread(dls))
			{
				hit = 1;
				if (dll = dllopen(dle->path, flags|RTLD_GLOBAL|RTLD_PARENT))
				{
					if (!dllcheck(dll, dle->path, rel, cur))
					{
						err = state.error;
						dlclose(dll);
						dll = 0;
						continue;
					}
					if (path && size)
						strlcpy(path, dle->path, size);
					break;
				}
				else
				{
					errorf("dll", NULL, 1, "dllplugin: %s dlopen failed: %s", dle->path, dllerror(1));
					err = state.error;
				}
			}
			dllsclose(dls);
		}
		if (hit)
		{
			if (!dll)
				state.error = err;
			return dll;
		}
		if (!lib)
			break;
		lib = 0;
	}
	if (dll = dllopen(name, flags))
	{
		if (!dllcheck(dll, name, rel, cur))
		{
			dlclose(dll);
			dll = 0;
		}
		else if (path && size)
			strlcpy(path, name, size);
	}
	return dll;
}

extern void*
dllplug(const char* lib, const char* name, const char* ver, int flags, char* path, size_t size)
{
	return dllplugin(lib, name, ver, 0, NULL, flags, path, size);
}

/***********************************************************************
*                                                                      *
*              This file is part of the ksh 93u+m package              *
*             Copyright (c) 2021 Contributors to ksh 93u+m             *
*                    <https://github.com/ksh93/ksh>                    *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 1.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*          http://www.eclipse.org/org/documents/epl-v10.html           *
*         (with md5 checksum b35adb5213ca9657e911e9befb180842)         *
*                                                                      *
*                  Martijn Dekker <martijn@inlv.org>                   *
*                                                                      *
***********************************************************************/

#include <ast.h>
#include <error.h>

#if _hdr_linux_fs
#include <linux/fs.h>
#endif
#if _hdr_linux_msdos_fs
#include <linux/msdos_fs.h>
#endif
#if _hdr_sys_ioctl
#include <sys/ioctl.h>
#endif

#if _hdr_sys_ioctl && _hdr_linux_fs && defined(FS_IOC_GETFLAGS) && defined(FS_CASEFOLD_FL)
#define _linux_casefold	1
#endif
#if _hdr_sys_ioctl && _hdr_linux_msdos_fs && defined(FAT_IOCTL_GET_ATTRIBUTES)
#define _linux_fatfs	1
#endif

/*
 * Return 1 if the given path is on a case insensitive file system, 0 if not, -1 on error.
 */
int
pathicase(const char *path)
{
#ifdef _PC_PATH_ATTRIBUTES
	/* AT&T mystery system (AST UWIN?) */
	char *a = astconf("PATH_ATTRIBUTES", path, (char*)0);
	return a ? strchr(a,'c') != 0 : -1;
#elif _lib_pathconf && defined(_PC_CASE_SENSITIVE)
	/* macOS; QNX 7.0+ */
	long r = pathconf(path, _PC_CASE_SENSITIVE);
	return r < 0L ? -1 : r == 0L;
#elif _lib_pathconf && defined(_PC_CASE_INSENSITIVE)
	/* Cygwin */
	long r = pathconf(path, _PC_CASE_INSENSITIVE);
	return r < 0L ? -1 : r > 0L;
#elif _linux_fatfs
	/* Linux */
	int attr = 0, fd, r;
	if ((fd = open(path, O_RDONLY|O_NONBLOCK)) < 0)
		return -1;
	r = ioctl(fd, FAT_IOCTL_GET_ATTRIBUTES, &attr);
#   if _linux_casefold
	/* Linux 5.2+ */
	if (r < 0 && errno == ENOTTY)	/* if it's not VFAT/FAT32...*/
	{
		r = ioctl(fd, FS_IOC_GETFLAGS, &attr);
		close(fd);
		return r < 0 ? -1 : (attr & FS_CASEFOLD_FL) != 0;
	}
#   endif /* _linux_casefold */
	close(fd);
	return r < 0 ? (errno != ENOTTY ? -1 : 0) : 1;
#elif _WINIX || __APPLE__
	/* Windows or Mac without pathconf probe: assume case insensitive */
	return 1;
#else
	/* Not implemented */
	errno = ENOSYS;
	return -1;
#endif
}

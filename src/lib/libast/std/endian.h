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
 * Linux/GNU compatibility
 */

#ifndef _ENDIAN_H
#define _ENDIAN_H

#include <bytesex.h>

#define __LITTLE_ENDIAN	1234
#define __BIG_ENDIAN	4321
#define __PDP_ENDIAN	3412

#if defined (__USE_BSD) && !defined(__STRICT_ANSI__)

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN	__LITTLE_ENDIAN
#endif

#ifndef BIG_ENDIAN
#define BIG_ENDIAN	__BIG_ENDIAN
#endif

#ifndef PDP_ENDIAN
#define PDP_ENDIAN	__PDP_ENDIAN
#endif

#undef	BYTE_ORDER
#define BYTE_ORDER	__BYTE_ORDER

#endif

#endif

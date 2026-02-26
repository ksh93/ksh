/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2026 Contributors to ksh 93u+m           *
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
*                                                                      *
***********************************************************************/
/*
 *  national.h -  definitions for multibyte character sets
 *
 *   David Korn
 *   AT&T Labs
 *
 */

#ifndef _national_h_defined
#define _national_h_defined	1

#if SHOPT_MULTIBYTE
#   ifndef MARKER
#	define MARKER		0xdfff	/* Must be invalid character */
#   endif
#endif /* SHOPT_MULTIBYTE */

extern int sh_strchr(const char*,const char*);
extern int sh_strwidth(const char*);

#endif /* _national_h_defined */

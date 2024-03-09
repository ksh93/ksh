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
*                                                                      *
***********************************************************************/
/*
 * David Korn
 * AT&T Research
 *
 * Interface definitions for a stack-like storage library
 *
 */

#ifndef _STK_H
#define _STK_H

#include <sfio.h>

#define _Stk_data	_Stak_data

#define stkstd		(&_Stk_data)

#define	Stk_t		Sfio_t

/* option bits for stkopen() */
#define STK_SMALL	1		/* allocate small stack frames	*/
#define STK_NULL	2		/* return NULL on overflow	*/

#define	stkptr(sp,n)	((char*)((sp)->_data)+(n))
#define stktop(sp)	((char*)(sp)->_next)
#define	stktell(sp)	((sp)->_next-(sp)->_data)
#define stkseek(sp,n)	((n)==0?(void*)((sp)->_next=(sp)->_data):_stkseek(sp,n))

extern Sfio_t		_Stk_data;

extern Stk_t*		stkopen(int);
extern Stk_t*		stkinstall(Stk_t*, char*(*)(size_t));	/* deprecated */
extern void		stkoverflow(Stk_t*, void*(*)(size_t));
extern int		stkclose(Stk_t*);
extern unsigned int	stklink(Stk_t*);
extern void*		stkalloc(Stk_t*, size_t);
extern char*		stkcopy(Stk_t*, const char*);
extern void*		stkset(Stk_t*, void*, size_t);
extern void*		_stkseek(Stk_t*, ssize_t);
extern void*		stkfreeze(Stk_t*, size_t);
extern int		stkon(Stk_t*, char*);

#endif

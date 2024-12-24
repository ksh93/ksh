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
#ifndef _CDTLIB_H
#define _CDTLIB_H	1

/*	cdt library/method implementation header
**	this header is exported to the method libraries
**	Written by Kiem-Phong Vo (5/25/96)
*/

#include	<ast.h>
#if !_BLD_cdt
#include	<dlldefs.h>
#endif

#include	<cdt.h>
#include	<unistd.h>
#include	<aso.h>

#include	"debug.h"

/* for backward compatibility */
#define NIL(t)	NULL
#define reg	/* empty */

/* min #bits for a hash table. (1<<this) is table size */
#define DT_HTABLE	10

/* convenient types */
#if !defined(uint)
#define uint	unsigned int
#endif
#if !defined(uchar)
#define uchar	unsigned char
#endif

/* This struct holds private method data created on DT_OPEN */
struct _dtdata_s
{	unsigned int	lock;	/* general dictionary lock	*/
	unsigned int	type;	/* method type, control flags	*/
	ssize_t		size;	/* number of objects		*/
	Dtuser_t	user;	/* application's data		*/
	Dt_t		dict;	/* when DT_INDATA is requested	*/
};

/* this structure holds the plugin information */
typedef struct _dtlib_s
{
	char*		name;		/* short name */
	char*		description;	/* short description */
	char*		release;	/* release info */
	char*		prefix;		/* name prefix */
	Dtmethod_t**	methods;	/* method list */
} Dtlib_t;

/* these macros lock/unlock dictionaries. DTRETURN substitutes for "return" */
#define DTSETLOCK(dt)		(((dt)->data->type&DT_SHARE) ? asolock(&(dt)->data->lock,1,ASO_LOCK) : 0 )
#define DTCLRLOCK(dt)		(((dt)->data->type&DT_SHARE) ? asolock(&(dt)->data->lock,1,ASO_UNLOCK) : 0 )
#define DTRETURN(ob,rv)		do { (ob) = (rv); goto dt_return; } while(0)
#define DTERROR(dt, mesg) 	(!((dt)->disc && (dt)->disc->eventf) ? 0 : \
				  (*(dt)->disc->eventf)((dt),DT_ERROR,(mesg),(dt)->disc) )

/* announce completion of an operation of type (ty) on some object (ob) in dictionary (dt) */
#define DTANNOUNCE(dt,ob,ty)	( ((ob) && ((ty)&DT_TOANNOUNCE) && ((dt)->data->type&DT_ANNOUNCE) && \
				   (dt)->disc && (dt)->disc->eventf ) ? \
					(*(dt)->disc->eventf)((dt), DT_ANNOUNCE|(ty), (ob), (dt)->disc) : 0 )

/* map bits for upward compatibility */
#define DTTYPE(dt,ty)		((dt)->typef ? (*(dt)->typef)((dt), (ty)) : (ty) )

/* shorthands for fields in Dtlink_t.
** note that __hash is used as a hash value
** or as the position in the parent table.
*/
#define _left	lh.__left
#define _hash	lh.__hash
#define _ppos	lh.__hash

#define _rght	rh.__rght
#define _ptbl	rh.__ptbl

/* tree rotation/linking functions */
#define rrotate(x,y)	((x)->_left = (y)->_rght, (y)->_rght = (x))
#define lrotate(x,y)	((x)->_rght = (y)->_left, (y)->_left = (x))
#define rlink(r,x)	((r) = (r)->_left = (x) )
#define llink(l,x)	((l) = (l)->_rght = (x) )

#define RROTATE(x,y)	(rrotate(x,y), (x) = (y))
#define LROTATE(x,y)	(lrotate(x,y), (x) = (y))
#define RRSHIFT(x,t)	((t) = (x)->_left->_left, (x)->_left->_left = (t)->_rght, \
			 (t)->_rght = (x), (x) = (t) )
#define LLSHIFT(x,t)	((t) = (x)->_rght->_rght, (x)->_rght->_rght = (t)->_left, \
			 (t)->_left = (x), (x) = (t) )

extern Dtlink_t*	_dtmake(Dt_t*, void*, int);
extern void		_dtfree(Dt_t*, Dtlink_t*, int);
extern int		_dtlock(Dt_t*, int);

#endif /* _CDTLIB_H */

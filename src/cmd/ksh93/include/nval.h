/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
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
*         hyenias <58673227+hyenias@users.noreply.github.com>          *
*                   Chase <nicetrynsa@protonmail.ch>                   *
*                                                                      *
***********************************************************************/
#ifndef _NVAL_H
#define _NVAL_H
/*
 * David Korn
 * AT&T Labs
 *
 * Interface definitions of structures for name-value pairs
 * These structures are used for named variables, functions and aliases
 *
 * NOTE: this header defines a public libshell interface,
 * unless _BLD_ksh is defined as nonzero
 */


#include	<ast.h>
#include	<cdt.h>
#include	<option.h>

typedef struct Namval Namval_t;
typedef struct Namfun Namfun_t;
typedef struct Namdisc Namdisc_t;
typedef struct Nambfun Nambfun_t;
typedef struct Namarray Namarr_t;
typedef struct Namdecl Namdecl_t;

/*
 * Any place that assigns or compares the NV_* symbols below to a var should use 'nvflag_t' for the
 * type of the var rather than 'unsigned short', 'int', etc.
 */
typedef uint64_t nvflag_t;

/*
 * Poor man's alignof() -- the real one is not available in C99 and cannot
 * be portably reimplemented. This nv_alignof() version can only be used
 * with types defined in the dummy struct below. The _c* char entries are
 * anchors.  Alignment is calculated using the offset of the _d* item with
 * the desired type to the corresponding char.
 */
typedef struct
{
	char		_cSfdouble_t;
	Sfdouble_t	_dSfdouble_t;
	char		_cdouble;
	double		_ddouble;
	char		_cfloat;
	float		_dfloat;
	char		_cSflong_t;
	Sflong_t	_dSflong_t;
	char		_cint32_t;
	int32_t		_dint32_t;
	char		_cint16_t;
	int16_t		_dint16_t;
	char		_cpointer;
	void		*_dpointer;
	char		_cmax_align_t;
	max_align_t	_dmax_align_t;
} _Namval_align_;

#define nv_alignof(t)	( offsetof(_Namval_align_, _d##t) - offsetof(_Namval_align_, _c##t) )

/*
 * This defines the template for nodes that have their own assignment
 * and or lookup functions
 */
struct Namdisc
{
	size_t	dsize;
	void	(*putval)(Namval_t*, const char*, nvflag_t, Namfun_t*);
	char	*(*getval)(Namval_t*, Namfun_t*);
	Sfdouble_t	(*getnum)(Namval_t*, Namfun_t*);
	char	*(*setdisc)(Namval_t*, const char*, Namval_t*, Namfun_t*);
	Namval_t *(*createf)(Namval_t*, const char*, nvflag_t, Namfun_t*);
	Namfun_t *(*clonef)(Namval_t*, Namval_t*, nvflag_t, Namfun_t*);
	char	*(*namef)(Namval_t*, Namfun_t*);
	Namval_t *(*nextf)(Namval_t*, Dt_t*, Namfun_t*);
	Namval_t *(*typef)(Namval_t*, Namfun_t*);
	int	(*readf)(Namval_t*, Sfio_t*, int, Namfun_t*);
	int	(*writef)(Namval_t*, Sfio_t*, nvflag_t, Namfun_t*);
};

struct Namfun
{
	const Namdisc_t	*disc;
	char		nofree;
	unsigned int	subshell;
	size_t		dsize;
	Namfun_t	*next;
	char		*last;
	Namval_t	*type;
};

struct Nambfun
{
	Namfun_t        fun;
	ssize_t		num;
	const char	**bnames;
	Namval_t	*bltins[1];
};

/* This is an array template header */
struct Namarray
{
	Namfun_t	hdr;
	long		nelem;				/* number of elements */
	void	*(*fun)(Namval_t*,const char*,nvflag_t);/* associative arrays */
	void		*fixed;			/* for fixed-size arrays */
	Dt_t		*table;			/* for subscripts */
	void		*scope;			/* non-zero when scoped */
};

/* The context pointer for declaration command */
struct Namdecl
{
	Namval_t	*tp;			/* point to type */
	const char	*optstring;
	void		*optinfof;
};

/* attributes of name-value node attribute flags */

#define NV_DEFAULT ((nvflag_t)0)

/*
 * This defines the attributes for an attributed name-value pair node.
 *
 * NOTE: For NV_MINSZ and nv_namptr (below) to work correctly, nvlink
 * and nvname must be the first two members, in that order.
*/
struct Namval
{
	Dtlink_t	nvlink;		/* space for cdt links */
	char		*nvname;	/* pointer to name of the node */
	nvflag_t	nvflag; 	/* attributes */
	size_t  	nvsize;		/* size or base */
	Namfun_t	*nvfun;		/* pointer to trap functions */
	void		*nvalue;	/* pointer to any kind of value */
	void		*nvmeta;	/* pointer to any of various kinds of type-dependent data */
};

#define NV_CLASS	".sh.type"
#define NV_DATA		"_"	/* special class or instance variable */

/* For aligning and addressing Namval_t data headers or array members with the nvlink member chopped off */
#define NV_MINSZ	roundof(sizeof(struct Namval) - offsetof(struct Namval, nvname), nv_alignof(max_align_t))
#define nv_namptr(p,n)	((Namval_t*)((char*)(p) + (n) * NV_MINSZ - offsetof(struct Namval, nvname)))

/* The following attributes are for internal use */
#define NV_NOFREE	((nvflag_t)1 << 9)	/* don't free the space when releasing value */
#define NV_ARRAY	((nvflag_t)1 << 10)	/* node is an array */
#define NV_REF		((nvflag_t)1 << 14)	/* reference bit */
#define NV_TABLE	((nvflag_t)1 << 11)	/* node is a dictionary table */
#define NV_MINIMAL	((nvflag_t)1 << 12)	/* node does not contain all fields */
#if _BLD_ksh
#if SHOPT_OPTIMIZE
#define NV_NOOPTIMIZE	NV_TABLE		/* disable loop invariants optimizer */
#else
#define NV_NOOPTIMIZE	0
#endif /* SHOPT_OPTIMIZE */
#endif /* _BLD_ksh */

#define NV_INTEGER	((nvflag_t)1 << 1)	/* integer attribute */
/* The following attributes are valid only when NV_INTEGER is off */
#define NV_LTOU		((nvflag_t)1 << 2)	/* convert to uppercase */
#define NV_UTOL		((nvflag_t)1 << 3)	/* convert to lowercase */
#define NV_ZFILL	((nvflag_t)1 << 4)	/* right justify and fill with leading zeros */
#define NV_RJUST	((nvflag_t)1 << 5)	/* right justify and blank fill */
#define NV_LJUST	((nvflag_t)1 << 6)	/* left justify and blank fill */
#define NV_BINARY	((nvflag_t)1 << 8)	/* fixed size data buffer */
#define NV_RAW		NV_LJUST		/* used only with NV_BINARY */
#define NV_HOST		(NV_RJUST|NV_LJUST)	/* map to host filename */

/* The following attributes do not affect the value */
#define NV_RDONLY	((nvflag_t)1 << 0)	/* readonly bit */
#define NV_EXPORT	((nvflag_t)1 << 13)	/* export bit */
#define NV_TAGGED	((nvflag_t)1 << 15)	/* user define tag bit */

/* The following are used with NV_INTEGER */
#define NV_SHORT	(NV_RJUST)		/* when integers are not long */
#define NV_LONG		(NV_UTOL)		/* for long long and long double */
#define NV_UNSIGN	(NV_LTOU)		/* for unsigned quantities */
#define NV_DOUBLE	(NV_INTEGER|NV_ZFILL)	/* for floating point */
#define NV_EXPNOTE	(NV_LJUST)		/* for scientific notation */
#define NV_HEXFLOAT	(NV_LTOU)		/* for C99 base16 float notation */
#define NV_FLTSIZEZERO	-1			/* a float with size of 0 being <0 */

/* options of nv_open. */
#define NV_APPEND	((nvflag_t)1 << 32)	/* append value */
#define NV_MOVE		((nvflag_t)1 << 43)	/* for use with nv_clone */
#define NV_ADD		((nvflag_t)1 << 45)	/* add node if not found */
#define NV_ASSIGN	NV_NOFREE		/* assignment is possible */
#define NV_NOARRAY	((nvflag_t)1 << 37)	/* array name not possible */
#define NV_IARRAY	((nvflag_t)1 << 38)	/* for indexed array */
#define NV_NOREF	NV_REF			/* don't follow reference */
#define NV_IDENT	((nvflag_t)1 << 7)	/* name must be identifier */
#define NV_VARNAME	((nvflag_t)1 << 33)	/* name must be ?(.)id*(.id) */
#define NV_NOADD	((nvflag_t)1 << 34)	/* do not add node */
#define NV_NOSCOPE	((nvflag_t)1 << 35)	/* look only in current scope */
#define NV_NOFAIL	((nvflag_t)1 << 36)	/* return 0 on failure, no msg */
#define NV_NODISC	NV_IDENT		/* ignore disciplines */
#define NV_UNATTR	((nvflag_t)1 << 39)	/* unset attributes before assignment */
#define NV_GLOBAL	((nvflag_t)1 << 44)	/* create global variable, ignoring local scope */
/* Moved here from name.h */
#define NV_TYPE		((nvflag_t)1 << 40)
#define NV_STATIC	((nvflag_t)1 << 41)
#define NV_COMVAR	((nvflag_t)1 << 42)
#define NV_FARRAY	((nvflag_t)1 << 46)	/* fixed-size arrays */
/* Mask for flags pertaining to nv_open */

#define NV_FUNCT	NV_IDENT		/* option for nv_create */
#define NV_BLTINOPT	NV_ZFILL		/* mark builtins in libcmd */

#define NV_PUBLIC	(~(NV_NOSCOPE|NV_ASSIGN|NV_IDENT|NV_VARNAME|NV_NOADD))

/* numeric types */
#define NV_INT16	(NV_SHORT|NV_INTEGER)
#define NV_UINT16	(NV_UNSIGN|NV_SHORT|NV_INTEGER)
#define NV_INT32	(NV_INTEGER)
#define NV_UINT32	(NV_UNSIGN|NV_INTEGER)
#define NV_INT64	(NV_LONG|NV_INTEGER)
#define NV_UINT64	(NV_UNSIGN|NV_LONG|NV_INTEGER)
#define NV_FLOAT	(NV_SHORT|NV_DOUBLE)
#define NV_LDOUBLE	(NV_LONG|NV_DOUBLE)

/* check/isolate all the bit flags used for numeric types */
#define nv_isnum(np)	(nv_isattr(np,NV_INTEGER)?nv_isattr(np,NV_DOUBLE|NV_INTEGER|NV_LJUST|NV_LONG|NV_SHORT|NV_UNSIGN):0)

/* name-value pair macros */
#define nv_isattr(np,f)		((np)->nvflag & (f))
#define nv_onattr(n,f)		((n)->nvflag |= (f))
#define nv_offattr(n,f)		((n)->nvflag &= ~(f))
#define nv_isarray(np)		(nv_isattr((np),NV_ARRAY))
#define nv_close(np)		/* no-op */

/* The following are operations for associative arrays */
#define NV_AINIT	1	/* initialize */
#define NV_AFREE	2	/* free array */
#define NV_ANEXT	3	/* advance to next subscript */
#define NV_ANAME	4	/* return subscript name */
#define NV_ADELETE	5	/* delete current subscript */
#define NV_AADD		6	/* add subscript if not found */
#define NV_ACURRENT	7	/* return current subscript Namval_t* */
#define NV_ASETSUB	8	/* set current subscript */

/* The following are for nv_disc */
#define NV_FIRST	1
#define NV_LAST		2
#define NV_POP		3
#define NV_CLONE	4

/* The following are operations for nv_putsub() */
#define ARRAY_BITS	22
#define ARRAY_ADD	(1L<<ARRAY_BITS)	/* add subscript if not found */
#define ARRAY_SCAN	(2L<<ARRAY_BITS)	/* For ${array[@]} */
#define ARRAY_UNDEF	(4L<<ARRAY_BITS)	/* For ${array} */


/* These are disciplines provided by the library for use with nv_discfun */
#define NV_DCADD	0	/* used to add named disciplines */
#define NV_DCRESTRICT	1	/* variable that are restricted in rsh */

/* prototype for array interface */
extern Namarr_t	*nv_arrayptr(Namval_t*);
extern Namarr_t	*nv_setarray(Namval_t*,void*(*)(Namval_t*,const char*,nvflag_t));
extern void	*nv_associative(Namval_t*,const char*,nvflag_t);
extern int	nv_aindex(Namval_t*);
extern int	nv_nextsub(Namval_t*);
extern char	*nv_getsub(Namval_t*);
extern Namval_t	*nv_putsub(Namval_t*, char*, long);
extern Namval_t	*nv_opensub(Namval_t*);

/* name-value pair function prototypes */
extern int		nv_adddisc(Namval_t*, const char**, Namval_t**);
extern int		nv_clone(Namval_t*, Namval_t*, nvflag_t);
extern void		*nv_context(Namval_t*);
extern Namval_t		*nv_create(const char*, Dt_t*, nvflag_t,Namfun_t*);
extern void		nv_delete(Namval_t*, Dt_t*, nvflag_t);
extern Dt_t		*nv_dict(Namval_t*);
extern Sfdouble_t	nv_getn(Namval_t*, Namfun_t*);
extern Sfdouble_t	nv_getnum(Namval_t*);
extern char 		*nv_getv(Namval_t*, Namfun_t*);
extern char 		*nv_getval(Namval_t*);
extern Namfun_t		*nv_hasdisc(Namval_t*, const Namdisc_t*);
extern int		nv_isnull(Namval_t*);
extern Namfun_t		*nv_isvtree(Namval_t*);
extern Namval_t		*nv_lastdict(void);
extern Namval_t		*nv_mkinttype(char*, size_t, int, const char*, Namdisc_t*);
extern void 		nv_newattr(Namval_t*,nvflag_t,ssize_t);
extern void 		nv_newtype(Namval_t*);
extern Namval_t		*nv_open(const char*,Dt_t*,nvflag_t);
extern void 		nv_putval(Namval_t*,const char*,nvflag_t);
extern void 		nv_putv(Namval_t*,const char*,nvflag_t,Namfun_t*);
extern int		nv_rename(Namval_t*,nvflag_t);
extern void		nv_rehash(Namval_t*,void*);
extern int		nv_scan(Dt_t*,void(*)(Namval_t*,void*),void*,nvflag_t,nvflag_t);
extern char 		*nv_setdisc(Namval_t*,const char*,Namval_t*,Namfun_t*);
extern void		nv_setref(Namval_t*,Dt_t*,nvflag_t);
extern int		nv_settype(Namval_t*,Namval_t*,nvflag_t);
extern void 		nv_setvec(Namval_t*,int,int,char*[]);
extern void		nv_setvtree(Namval_t*);
extern size_t 		nv_setsize(Namval_t*,size_t);
extern Namfun_t		*nv_disc(Namval_t*,Namfun_t*,nvflag_t);
extern void 		nv_unset(Namval_t*,nvflag_t);
extern Namval_t		*nv_search(const char *, Dt_t*, nvflag_t);
extern char		*nv_name(Namval_t*);
extern Namval_t		*nv_type(Namval_t*);
extern void		nv_addtype(Namval_t*,const char*, Optdisc_t*, size_t);
extern const Namdisc_t	*nv_discfun(int);

#define nv_size(np)		nv_setsize((np),(size_t)-1)
#define nv_stack(np,nf)		nv_disc(np,nf,0)

#endif /* !_NVAL_H */

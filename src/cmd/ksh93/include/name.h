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
*                                                                      *
***********************************************************************/
#ifndef name_h_defined
#define name_h_defined
/*
 * This is the implementation header file for name-value pairs
 */

#include	<ast.h>
#include	<cdt.h>
#include	"nval.h"

/* used for arrays */

#define ARRAY_MAX 	(1L<<ARRAY_BITS) /* maximum number of elements in an array */
#define ARRAY_MASK	(ARRAY_MAX-1)	/* For index values */

#define ARRAY_INCR	32	/* number of elements to grow when array
				   bound exceeded.  Must be a power of 2 */
#define ARRAY_FILL	(8L<<ARRAY_BITS)	/* used with nv_putsub() */
#define ARRAY_NOCLONE	(16L<<ARRAY_BITS)	/* do not clone array disc */
#define ARRAY_NOCHILD   (32L<<ARRAY_BITS)	/* skip compound arrays */
#define ARRAY_SETSUB	(64L<<ARRAY_BITS)	/* set subscript */
#define ARRAY_NOSCOPE	(128L<<ARRAY_BITS)	/* top level scope only */
#define ARRAY_TREE	(256L<<ARRAY_BITS)	/* arrays of compound vars */
#if SHOPT_FIXEDARRAY
#   define ARRAY_FIXED	ARRAY_NOCLONE		/* For index values */
#endif /* SHOPT_FIXEDARRAY */
#define NV_FARRAY	0x10000000		/* fixed-size arrays */
#define NV_ASETSUB	8			/* set subscript */

/* These flags are used as options to array_get() */
#define ARRAY_ASSIGN	0
#define ARRAY_LOOKUP	1
#define ARRAY_DELETE	2


struct Namref
{
	Namval_t	*np;
	Namval_t	*table;
	Dt_t		*root;
	char		*sub;
#if SHOPT_FIXEDARRAY
	int		curi;
	char		dim;
#endif /* SHOPT_FIXEDARRAY */
};

/* This describes a user shell function node */
struct Ufunction
{
	int		*ptree;		/* address of parse tree */
	int		lineno;		/* line number of function start */
	short		argc;		/* number of references */
	short		running;	/* function is running */
	char		**argv;		/* reference argument list */
	Namval_t	*nspace;	/* pointer to name space */
	char		*fname;		/* file name where function defined */
	char		*help;		/* help string */
	Dt_t		*sdict;		/* dictionary for statics */
	Dt_t		*fdict;		/* dictionary node belongs to */
	Namval_t	*np;		/* function node pointer */
};

#ifndef ARG_RAW
    struct argnod;
#endif /* !ARG_RAW */

/* attributes of Namval_t items */

/* The following attributes are for internal use */
#define NV_NOCHANGE	(NV_EXPORT|NV_MINIMAL|NV_RDONLY|NV_TAGGED|NV_NOFREE|NV_ARRAY)
#define NV_ATTRIBUTES	(~(NV_NOSCOPE|NV_ARRAY|NV_NOARRAY|NV_IDENT|NV_ASSIGN|NV_REF|NV_VARNAME|NV_STATIC))
#define NV_PARAM	NV_NODISC	/* expansion use positional params */

/* This following are for use with nodes which are not name-values */
#define NV_TYPE		0x1000000
#define NV_STATIC	0x2000000
#define NV_COMVAR	0x4000000
#define NV_FUNCTION	(NV_RJUST|NV_FUNCT)	/* value is shell function */
#define NV_FPOSIX	NV_LJUST		/* POSIX function semantics */
#define NV_STATICF	NV_INTEGER		/* static class function */

#define NV_NOPRINT	(NV_LTOU|NV_UTOL)	/* do not print */
#define NV_NOALIAS	(NV_NOPRINT|NV_MINIMAL)
#define NV_NOEXPAND	NV_RJUST		/* do not expand alias */
#define NV_BLTIN	(NV_NOPRINT|NV_EXPORT)
#define BLT_ENV		(NV_RDONLY)		/* non-stoppable,
						 * can modify environment */
#define BLT_SPC		(NV_LJUST)		/* special built-ins */
#define BLT_EXIT	(NV_RJUST)		/* exit value can be > 255 or < 0 */
#define BLT_DCL		(NV_TAGGED)		/* declaration command */
#define BLT_NOSFIO	(NV_MINIMAL)		/* doesn't use sfio */
#define NV_OPTGET	(NV_BINARY)		/* function calls getopts */
#define nv_isref(n)	(nv_isattr((n),NV_REF|NV_TAGGED|NV_FUNCT)==NV_REF)
#define is_abuiltin(n)	(nv_isattr(n,NV_BLTIN|NV_INTEGER)==NV_BLTIN)
#define is_afunction(n)	(nv_isattr(n,NV_FUNCTION|NV_REF)==NV_FUNCTION)
#define nv_funtree(n)	(((struct Ufunction*)(n)->nvalue)->ptree)

/* NAMNOD MACROS */
/* ... for attributes */

#define nv_setattr(n,f)	((n)->nvflag = (f))
#define nv_context(n)	((void*)(n)->nvfun)		/* for builtins */
/* The following are for name references */
#define nv_refnode(n)	(((struct Namref*)(n)->nvalue)->np)
#define nv_reftree(n)	(((struct Namref*)(n)->nvalue)->root)
#define nv_reftable(n)	(((struct Namref*)(n)->nvalue)->table)
#define nv_refsub(n)	(((struct Namref*)(n)->nvalue)->sub)
#if SHOPT_FIXEDARRAY
#   define nv_refindex(n)	(((struct Namref*)(n)->nvalue)->curi)
#   define nv_refdimen(n)	(((struct Namref*)(n)->nvalue)->dim)
#endif /* SHOPT_FIXEDARRAY */

/* ... etc */

#define nv_setsize(n,s)	((n)->nvsize = (s))
#undef nv_size
#define nv_size(np)	((np)->nvsize)
#define _nv_hasget(np)  ((np)->nvfun && (np)->nvfun->disc && nv_hasget(np))
#define nv_isnull(np)	(!(np)->nvalue && !_nv_hasget(np))

/* ...	for arrays */

#define array_elem(ap)	((ap)->nelem&ARRAY_MASK)
#define array_assoc(ap)	((ap)->fun)

extern int		array_maxindex(Namval_t*);
extern char 		*nv_endsubscript(Namval_t*, char*, int);
extern Namfun_t 	*nv_enforcedisc(Namval_t*);
extern int		nv_arrayisset(Namval_t*, Namarr_t*);
extern int		nv_arraysettype(Namval_t*, Namval_t*,const char*,int);
extern int		nv_aimax(Namval_t*);
extern int		nv_atypeindex(Namval_t*, const char*);
extern void		nv_setlist(struct argnod*, int, Namval_t*);
#if SHOPT_OPTIMIZE
    extern void		nv_optimize(Namval_t*);
    extern void		nv_optimize_clear(Namval_t*);
#   define nv_setoptimize(v)	(sh.argaddr=(v))
#   define nv_getoptimize()	(sh.argaddr)
#else
#   define nv_optimize(np)		/* no-op */
#   define nv_optimize_clear(np)	/* no-op */
#   define nv_setoptimize(argaddr)	/* no-op */
#   define nv_getoptimize()		NULL
#endif /* SHOPT_OPTIMIZE */
extern void		nv_outname(Sfio_t*,char*, int);
extern void 		nv_unref(Namval_t*);
extern int		nv_hasget(Namval_t*);
extern void		clone_all_disc(Namval_t*, Namval_t*, int);
extern Namfun_t		*nv_clone_disc(Namfun_t*, int);
extern void		*nv_diropen(Namval_t*, const char*);
extern char		*nv_dirnext(void*);
extern void		nv_dirclose(void*);
extern char		*nv_getvtree(Namval_t*, Namfun_t*);
extern void		nv_attribute(Namval_t*, Sfio_t*, char*, int);
extern Namval_t		*nv_bfsearch(const char*, Dt_t*, Namval_t**, char**);
extern Namval_t		*nv_mktype(Namval_t**, int);
extern Namval_t		*nv_addnode(Namval_t*, int);
extern Namval_t		*nv_parent(Namval_t*);
extern Namval_t		*nv_mount(Namval_t*, const char *name, Dt_t*);
extern Namval_t		*nv_arraychild(Namval_t*, Namval_t*, int);
extern int		nv_compare(Dt_t*, void*, void*, Dtdisc_t*);
extern void		nv_outnode(Namval_t*,Sfio_t*, int, int);
extern int		nv_subsaved(Namval_t*, int);
extern void		nv_typename(Namval_t*, Sfio_t*);
extern Namval_t		*nv_typeparent(Namval_t*);
extern int		nv_istable(Namval_t*);
extern size_t		nv_datasize(Namval_t*, size_t*);
extern Namfun_t		*nv_mapchar(Namval_t*, const char*);
#if SHOPT_FIXEDARRAY
   extern int		nv_arrfixed(Namval_t*, Sfio_t*, int, char*);
#endif /* SHOPT_FIXEDARRAY */

extern const Namdisc_t	RESTRICTED_disc;
extern const Namdisc_t	ENUM_disc;
#if SHOPT_OPTIMIZE
extern const Namdisc_t	OPTIMIZE_disc;
#endif /* SHOPT_OPTIMIZE */
extern char		nv_local;
extern Dtdisc_t		_Nvdisc;
extern const char	*nv_discnames[];
extern const char	e_optincompat1[];
extern const char	e_optincompat2[];
extern const char	e_subscript[];
extern const char	e_nullset[];
extern const char	e_notset[];
extern const char	e_noparent[];
extern const char	e_notelem[];
extern const char	e_readonly[];
extern const char	e_badfield[];
extern const char	e_restricted[];
extern const char	e_ident[];
extern const char	e_varname[];
extern const char	e_noalias[];
extern const char	e_notrackedalias[];
extern const char	e_noarray[];
extern const char	e_notenum[];
extern const char	e_nounattr[];
extern const char	e_aliname[];
extern const char	e_badexport[];
extern const char	e_badref[];
extern const char	e_badsubscript[];
extern const char	e_rmref[];
extern const char	e_noref[];
extern const char	e_selfref[];
extern const char	e_staticfun[];
extern const char	e_badlocale[];
extern const char	e_redef[];
extern const char	e_required[];
extern const char	e_badappend[];
extern const char	e_unknowntype[];
extern const char	e_unknownmap[];
extern const char	e_mapchararg[];
extern const char	e_subcomvar[];
extern const char	e_badtypedef[];
extern const char	e_typecompat[];
extern const char	e_globalref[];
extern const char	e_tolower[];
extern const char	e_toupper[];
#endif /* name_h_defined */

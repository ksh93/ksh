/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
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
#ifndef _SFHDR_H
#define _SFHDR_H	1
#if !defined(_BLD_sfio) && !defined(_BLD_stdio)
#define _BLD_sfio	1
#endif

/*	Internal definitions for sfio.
**	Written by Kiem-Phong Vo
*/

#define _next		next
#define _endw		endw
#define _endr		endr
#define _endb		endb
#define _push		push
#define _flags		flags
#define _file		file
#define _data		data
#define _size		size
#define _val		val

#include	"FEATURE/sfio"
#include	"FEATURE/mmap"

#include	"sfio_t.h"

/* file system info */
#include	<ast.h>
#include	<ast_time.h>
#include	<ast_tty.h>
#include	<ls.h>

/* AST always provides multibyte handling */
#undef _hdr_wchar
#undef _lib_mbrtowc
#undef _lib_wcrtomb
#define _hdr_wchar	1
#define _lib_mbrtowc	1
#define _lib_wcrtomb	1

#if _mem_st_blksize_stat
#define _stat_blksize	1
#endif

#if _lib_localeconv && _hdr_locale
#define _lib_locale	1
#endif

#if !_mmap_worthy
#undef MAP_TYPE
#endif

#include	"FEATURE/float"

#include	<error.h>
#include	<ctype.h>

/* deal with multi-byte character and string conversions */
#if AST_NOMULTIBYTE
#undef	_has_multibyte
#else
#include	<wchar.h>
#define _has_multibyte		1
#define SFMBMAX			mbmax()
#define SFMBCPY(to,fr)		memcpy((to), (fr), sizeof(mbstate_t))
#define SFMBCLR(mb)		memset((mb), 0,  sizeof(mbstate_t))
#define SFMBSET(lhs,v)		(lhs = (v))
#define SFMBLEN(s,mb)		mbsize(s)
#define SFMBDCL(ms)		mbstate_t ms;
#endif /* AST_NOMULTIBYTE */

#if !_has_multibyte
#define _has_multibyte		0	/* no multibyte support	*/
#define SFMBCPY(to,fr)
#define SFMBCLR(mb)
#define SFMBSET(lhs,v)
#define SFMBDCL(mb)
#define SFMBLEN(s,mb)		(*(s) ? 1 : 0)
#endif /* _has_multibyte */

/* functions for polling readiness of streams */
#if _lib_select
#undef _lib_poll
#if _sys_select
#include	<sys/select.h>
#endif
#endif /*_lib_select_*/

#if _lib_poll
#include	<poll.h>
#define SFPOLL(pfd,n,tm)	poll((pfd),(ulong)(n),(tm))
#endif

#if _stream_peek
#include	<stropts.h>
#endif

#if _socket_peek
#include	<sys/socket.h>
#endif

/* to test for executable access mode of a file */
#ifndef X_OK
#define X_OK	01
#endif

/* Private flags in the "bits" field */
#define SFIO_MMAP	00000001	/* in memory mapping mode		*/
#define SFIO_BOTH	00000002	/* both read/write			*/
#define SFIO_HOLE	00000004	/* a hole of zero's was created		*/
#define SFIO_NULL	00000010	/* stream is /dev/null			*/
#define SFIO_SEQUENTIAL	00000020	/* sequential access			*/
#define SFIO_JUSTSEEK	00000040	/* just did a sfseek			*/
#define SFIO_PRIVATE	00000100	/* private stream to Sfio		*/
#define SFIO_ENDING	00000200	/* no re-io on interrupts at closing	*/
#define SFIO_WIDE	00000400	/* in wide mode - stdio only		*/
#define SFIO_PUTR	00001000	/* in sfputr()				*/

/* "bits" flags that must be cleared in sfclrlock */
#define SFIO_TMPBITS	00170000
#define SFIO_DCDOWN	00010000	/* recurse down the discipline stack	*/

#define SFIO_WCFORMAT	00020000	/* wchar_t formatting - stdio only	*/
#if _has_multibyte
#define SFWCSET(f)	((f)->bits |= SFIO_WCFORMAT)
#define SFWCGET(f,v)	(((v) = (f)->bits & SFIO_WCFORMAT), ((f)->bits &= ~SFIO_WCFORMAT) )
#else
#define SFWCSET(f)
#define SFWCGET(f,v)
#endif

#define SFIO_MVSIZE	00040000	/* f->size was reset in sfmove()	*/
#define SFMVSET(f)	(((f)->size *= SFIO_NMAP), ((f)->bits |= SFIO_MVSIZE) )
#define SFMVUNSET(f)	(!((f)->bits&SFIO_MVSIZE) ? 0 : \
				(((f)->bits &= ~SFIO_MVSIZE), ((f)->size /= SFIO_NMAP)) )

#define SFCLRBITS(f)	(SFMVUNSET(f), ((f)->bits &= ~SFIO_TMPBITS) )


/* bits for the mode field, SFIO_INIT defined in sfio_t.h */
#define SFIO_RC		00000010	/* peeking for a record			*/
#define SFIO_RV		00000020	/* reserve without read	or most write	*/
#define SFIO_LOCK	00000040	/* stream is locked for io op		*/
#define SFIO_PUSH	00000100	/* stream has been pushed		*/
#define SFIO_POOL	00000200	/* stream is in a pool but not current	*/
#define SFIO_PEEK	00000400	/* there is a pending peek		*/
#define SFIO_PKRD	00001000	/* did a peek read			*/
#define SFIO_GETR	00002000	/* did a getr on this stream		*/
#define SFIO_SYNCED	00004000	/* stream was synced			*/
#define SFIO_STDIO	00010000	/* given up the buffer to stdio		*/
#define SFIO_AVAIL	00020000	/* was closed, available for reuse	*/
#define SFIO_LOCAL	00100000	/* sentinel for a local call		*/

#ifdef DEBUG
#define ASSERT(p)	((p) ? 0 : (abort(),0) )
#else
#define ASSERT(p)
#endif

/* shorthands */
#define NIL(t)		NULL		/* for backward compatibility */
#define reg		/* empty */	/* for backward compatibility (was 'register') */
#ifndef uchar
#define uchar		unsigned char
#endif
#ifndef ulong
#define ulong		unsigned long
#endif
#ifndef uint
#define uint		unsigned int
#endif
#ifndef ushort
#define ushort		unsigned short
#endif

#define SECOND		1000	/* millisecond units */

/* macros to determine stream types from 'struct stat' data */
#ifndef S_IFDIR
#define S_IFDIR	0
#endif
#ifndef S_IFREG
#define S_IFREG	0
#endif
#ifndef S_IFCHR
#define S_IFCHR	0
#endif
#ifndef S_IFIFO
#define S_IFIFO	0
#endif
#ifndef S_ISOCK
#define S_ISOCK	0
#endif

#ifndef S_IFMT
#define S_IFMT	(S_IFDIR|S_IFREG|S_IFCHR|S_IFIFO|S_ISOCK)
#endif

#ifndef S_ISDIR
#define S_ISDIR(m)	(((m)&S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(m)	(((m)&S_IFMT) == S_IFREG)
#endif
#ifndef S_ISCHR
#define S_ISCHR(m)	(((m)&S_IFMT) == S_IFCHR)
#endif

#ifndef S_ISFIFO
#	if S_IFIFO
#		define S_ISFIFO(m)	(((m)&S_IFMT) == S_IFIFO)
#	else
#		define S_ISFIFO(m)	(0)
#	endif
#endif

#ifdef S_IRUSR
#define SFIO_CREATMODE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
#else
#define SFIO_CREATMODE	0666
#endif

/* set close-on-exec */
#ifdef F_SETFD
#	ifndef FD_CLOEXEC
#		define FD_CLOEXEC	1
#	endif /*FD_CLOEXEC*/
#	define SETCLOEXEC(fd)		((void)fcntl((fd),F_SETFD,FD_CLOEXEC))
#else
#	ifdef FIOCLEX
#		define SETCLOEXEC(fd)	((void)ioctl((fd),FIOCLEX,0))
#	else
#		define SETCLOEXEC(fd)
#	endif /*FIOCLEX*/
#endif /*F_SETFD*/

#define SFIO_FD_CLOEXEC			0x0001

/* function to get the decimal point for local environment */
#include "lclib.h"
#define SFSETLOCALE(dp,tp) \
	do if (*(dp) == 0) { \
		Lc_numeric_t*	lv = (Lc_numeric_t*)LCINFO(AST_LC_NUMERIC)->data; \
		*(dp) = lv->decimal; \
		*(tp) = lv->thousand; \
	} while (0)

/* stream pool structure. */
typedef struct _sfpool_s	Sfpool_t;
struct _sfpool_s
{	Sfpool_t*	next;
	int		mode;		/* type of pool			*/
	int		s_sf;		/* size of pool array		*/
	int		n_sf;		/* number currently in pool	*/
	Sfio_t**	sf;		/* array of streams		*/
	Sfio_t*		array[3];	/* start with 3			*/
};

/* reserve buffer structure */
typedef struct _sfrsrv_s	Sfrsrv_t;
struct _sfrsrv_s
{	ssize_t		slen;		/* last string length		*/
	ssize_t		size;		/* buffer size			*/
	uchar		data[1];	/* data buffer			*/
};

/* co-process structure */
typedef struct _sfproc_s	Sfproc_t;
struct _sfproc_s
{	int		pid;	/* process ID			*/
	uchar*		rdata;	/* read data being cached	*/
	int		ndata;	/* size of cached data		*/
	int		size;	/* buffer size			*/
	int		file;	/* saved file descriptor	*/
	int		sigp;	/* sigpipe protection needed	*/
};

/* extensions to sfvprintf/sfvscanf */
#define FP_SET(fp,fn)	(fp < 0 ? (fn += 1) : (fn = fp) )
#define FP_WIDTH	0
#define FP_PRECIS	1
#define FP_BASE		2
#define FP_STR		3
#define FP_SIZE		4
#define FP_INDEX	5	/* index size	*/

typedef struct _fmt_s		Fmt_t;
typedef struct _fmtpos_s	Fmtpos_t;
typedef union
{	int		i, *ip;
	long		l, *lp;
	short		h, *hp;
	uint		ui;
	ulong		ul;
	ushort		uh;
	Sflong_t	ll, *llp;
	Sfulong_t	lu;
	Sfdouble_t	ld;
	double		d;
	float		f;
#if _has_multibyte
	wchar_t		wc;
	wchar_t		*ws, **wsp;
#endif
	char		c, *s, **sp;
	uchar		uc, *us, **usp;
	void		*vp;
	Sffmt_t		*ft;
} Argv_t;

struct _fmt_s
{	char*		form;		/* format string		*/
	va_list		args;		/* corresponding arglist	*/
	SFMBDCL(mbs)			/* multibyte parsing state	*/

	char*		oform;		/* original format string	*/
	va_list		oargs;		/* original arg list		*/
	int		argn;		/* number of args already used	*/
	Fmtpos_t*	fp;		/* position list		*/

	Sffmt_t*	ft;		/* formatting environment	*/
	Sffmtevent_f	eventf;		/* event function		*/
	Fmt_t*		next;		/* stack frame pointer		*/
};

struct _fmtpos_s
{	Sffmt_t	ft;			/* environment			*/
	Argv_t	argv;			/* argument value		*/
	int	fmt;			/* original format		*/
	int	need[FP_INDEX];		/* positions depending on	*/
};

#define LEFTP		'('
#define RIGHTP		')'
#define QUOTE		'\''

#ifndef CHAR_BIT
#define CHAR_BIT	8
#endif

#define FMTSET(ft, frm,ags, fv, sz, flgs, wid,pr,bs, ts,ns) \
	((ft->form = (char*)frm), va_copy(ft->args,ags), \
	 (ft->fmt = fv), (ft->size = sz), \
	 (ft->flags = (flgs&SFFMT_SET)), \
	 (ft->width = wid), (ft->precis = pr), (ft->base = bs), \
	 (ft->t_str = ts), (ft->n_str = ns) )
#define FMTGET(ft, frm,ags, fv, sz, flgs, wid,pr,bs) \
	((frm = ft->form), va_copy(ags,ft->args), \
	 (fv = ft->fmt), (sz = ft->size), \
	 (flgs = (flgs&~(SFFMT_SET))|(ft->flags&SFFMT_SET)), \
	 (wid = ft->width), (pr = ft->precis), (bs = ft->base) )

/* format flags&types, must coexist with those in sfio.h */
#define SFFMT_FORBIDDEN 000077777777	/* for sfio.h only		*/
#define SFFMT_EFORMAT	001000000000	/* sfcvt converting %e		*/
#define SFFMT_MINUS	002000000000	/* minus sign			*/
#define SFFMT_AFORMAT	004000000000	/* sfcvt converting %a		*/
#define SFFMT_UPPER	010000000000	/* sfcvt converting upper	*/

#define SFFMT_TYPES	(SFFMT_SHORT|SFFMT_SSHORT | SFFMT_LONG|SFFMT_LLONG|\
			 SFFMT_LDOUBLE | SFFMT_IFLAG|SFFMT_JFLAG| \
			 SFFMT_TFLAG | SFFMT_ZFLAG )

/* type of elements to be converted */
#define SFFMT_INT	001		/* %d,%i 		*/
#define SFFMT_UINT	002		/* %u,o,x etc.		*/
#define SFFMT_FLOAT	004		/* %f,e,g etc.		*/
#define SFFMT_CHAR	010		/* %c,C			*/
#define SFFMT_POINTER	020		/* %p,n,s,S		*/
#define SFFMT_CLASS	040		/* %[			*/

/* local variables used across sf-functions */
typedef void  (*Sfnotify_f)(Sfio_t*, int, void*);
#define _Sfpage		(_Sfextern.sf_page)
#define _Sfpool		(_Sfextern.sf_pool)
#define _Sfpmove	(_Sfextern.sf_pmove)
#define _Sfstack	(_Sfextern.sf_stack)
#define _Sfnotify	(_Sfextern.sf_notify)
#define _Sfstdsync	(_Sfextern.sf_stdsync)
#define _Sfudisc	(&(_Sfextern.sf_udisc))
#define _Sfcleanup	(_Sfextern.sf_cleanup)
#define _Sfexiting	(_Sfextern.sf_exiting)
#define _Sfdone		(_Sfextern.sf_done)
typedef struct _sfextern_s
{	ssize_t			sf_page;
	struct _sfpool_s	sf_pool;
	int			(*sf_pmove)(Sfio_t*, int);
	Sfio_t*			(*sf_stack)(Sfio_t*, Sfio_t*);
	void			(*sf_notify)(Sfio_t*, int, void*);
	int			(*sf_stdsync)(Sfio_t*);
	struct _sfdisc_s	sf_udisc;
	void			(*sf_cleanup)(void);
	int			sf_exiting;
	int			sf_done;
} Sfextern_t;

/* get the real value of a byte in a coded long or ulong */
#define SFUVALUE(v)	(((ulong)(v))&(SFIO_MORE-1))
#define SFSVALUE(v)	((( long)(v))&(SFIO_SIGN-1))
#define SFBVALUE(v)	(((ulong)(v))&(SFIO_BYTE-1))

/* pick this many bits in each iteration of double encoding */
#define SFIO_PRECIS	7

/* grain size for buffer increment */
#define SFIO_GRAIN	1024
#define SFIO_PAGE		((ssize_t)(SFIO_GRAIN*sizeof(int)*2))

/* when the buffer is empty, certain io requests may be better done directly
   on the given application buffers. The below condition determines when.
*/
#define SFDIRECT(f,n)	(((ssize_t)(n) >= (f)->size) || \
			 ((n) >= SFIO_GRAIN && (ssize_t)(n) >= (f)->size/16 ) )

/* number of pages to memory map at a time */
#if _ptr_bits >= 64
#define SFIO_NMAP	1024
#else
#define SFIO_NMAP	32
#endif

#ifndef MAP_VARIABLE
#define MAP_VARIABLE	0
#endif
#ifndef _mmap_fixed
#define _mmap_fixed	0
#endif

/* set/unset sequential states for mmap */
#if _lib_madvise && defined(MADV_SEQUENTIAL) && defined(MADV_NORMAL)
#define SFMMSEQON(f,a,s) \
		do { int oerrno = errno; \
		     (void)madvise((caddr_t)(a),(size_t)(s),MADV_SEQUENTIAL); \
		     errno = oerrno; \
		} while(0)
#define SFMMSEQOFF(f,a,s) \
		do { int oerrno = errno; \
		     (void)madvise((caddr_t)(a),(size_t)(s),MADV_NORMAL); \
		     errno = oerrno; \
		} while(0)
#else
#define SFMMSEQON(f,a,s)
#define SFMMSEQOFF(f,a,s)
#endif

#define SFMUNMAP(f,a,s)		(munmap((caddr_t)(a),(size_t)(s)), \
				 ((f)->endb = (f)->endr = (f)->endw = (f)->next = \
				  (f)->data = NULL) )

/* the bottomless bit bucket */
#define DEVNULL		"/dev/null"
#define SFSETNULL(f)	((f)->extent = (Sfoff_t)(-1), (f)->bits |= SFIO_NULL)
#define SFISNULL(f)	((f)->extent < 0 && ((f)->bits&SFIO_NULL) )

#define SFKILL(f)	((f)->mode = (SFIO_AVAIL|SFIO_LOCK) )
#define SFKILLED(f)	(((f)->mode&(SFIO_AVAIL|SFIO_LOCK)) == (SFIO_AVAIL|SFIO_LOCK) )

/* exception types */
#define SFIO_EDONE	0	/* stop this operation and return	*/
#define SFIO_EDISC	1	/* discipline says it's ok		*/
#define SFIO_ESTACK	2	/* stack was popped			*/
#define SFIO_ECONT	3	/* can continue normally		*/

#define SETLOCAL(f)	((f)->mode |= SFIO_LOCAL)
#define GETLOCAL(f,v)	((v) = ((f)->mode&SFIO_LOCAL), (f)->mode &= ~SFIO_LOCAL, (v))
#define SFWRALL(f)	((f)->mode |= SFIO_RV)
#define SFISALL(f,v)	((((v) = (f)->mode&SFIO_RV) ? ((f)->mode &= ~SFIO_RV) : 0), \
			 ((v) || ((f)->flags&(SFIO_SHARE|SFIO_APPENDWR|SFIO_WHOLE)) ) )
#define SFSK(f,a,o,d)	(SETLOCAL(f),sfsk(f,(Sfoff_t)a,o,d))
#define SFRD(f,b,n,d)	(SETLOCAL(f),sfrd(f,b,n,d))
#define SFWR(f,b,n,d)	(SETLOCAL(f),sfwr(f,b,n,d))
#define SFSYNC(f)	(SETLOCAL(f),sfsync(f))
#define SFCLOSE(f)	(SETLOCAL(f),sfclose(f))
#define SFFLSBUF(f,n)	(SETLOCAL(f),_sfflsbuf(f,n))
#define SFFILBUF(f,n)	(SETLOCAL(f),_sffilbuf(f,n))
#define SFSETBUF(f,s,n)	(SETLOCAL(f),sfsetbuf(f,s,n))
#define SFWRITE(f,s,n)	(SETLOCAL(f),sfwrite(f,s,n))
#define SFREAD(f,s,n)	(SETLOCAL(f),sfread(f,s,n))
#define SFSEEK(f,p,t)	(SETLOCAL(f),sfseek(f,p,t))
#define SFNPUTC(f,c,n)	(SETLOCAL(f),sfnputc(f,c,n))
#define SFRAISE(f,e,d)	(SETLOCAL(f),sfraise(f,e,d))

/* lock/open a stream */
#define SFMODE(f,l)	((f)->mode & ~(SFIO_RV|SFIO_RC|((l) ? SFIO_LOCK : 0)) )
#define SFLOCK(f,l)	(void)((f)->mode |= SFIO_LOCK, (f)->endr = (f)->endw = (f)->data)
#define _SFOPENRD(f)	((f)->endr = (f)->endb)
#define _SFOPENWR(f)	((f)->endw = ((f)->flags&SFIO_LINE) ? (f)->data : (f)->endb)
#define _SFOPEN(f)	((f)->mode == SFIO_READ  ? _SFOPENRD(f) : \
			 (f)->mode == SFIO_WRITE ? _SFOPENWR(f) : \
			 ((f)->endw = (f)->endr = (f)->data) )
#define SFOPEN(f,l)	(void)((l) ? 0 : \
				((f)->mode &= ~(SFIO_LOCK|SFIO_RC|SFIO_RV), _SFOPEN(f), 0) )

/* check to see if the stream can be accessed */
#define SFFROZEN(f)	(((f)->mode&(SFIO_PUSH|SFIO_LOCK|SFIO_PEEK)) ? 1 : \
			 !((f)->mode&SFIO_STDIO) ? 0 : \
			 _Sfstdsync ? (*_Sfstdsync)(f) : (((f)->mode &= ~SFIO_STDIO),0) )


/* set discipline code */
#define SFDISC(f,dc,iof) \
	{	Sfdisc_t* d; \
		if(!(dc)) \
			d = (dc) = (f)->disc; \
		else 	d = (f->bits&SFIO_DCDOWN) ? ((dc) = (dc)->disc) : (dc); \
		while(d && !(d->iof))	d = d->disc; \
		if(d)	(dc) = d; \
	}
#define SFDCRD(f,buf,n,dc,rv) \
	{	int		dcdown = f->bits&SFIO_DCDOWN; f->bits |= SFIO_DCDOWN; \
		rv = (*dc->readf)(f,buf,n,dc); \
		if(!dcdown)	f->bits &= ~SFIO_DCDOWN; \
	}
#define SFDCWR(f,buf,n,dc,rv) \
	{	int		dcdown = f->bits&SFIO_DCDOWN; f->bits |= SFIO_DCDOWN; \
		rv = (*dc->writef)(f,buf,n,dc); \
		if(!dcdown)	f->bits &= ~SFIO_DCDOWN; \
	}
#define SFDCSK(f,addr,type,dc,rv) \
	{	int		dcdown = f->bits&SFIO_DCDOWN; f->bits |= SFIO_DCDOWN; \
		rv = (*dc->seekf)(f,addr,type,dc); \
		if(!dcdown)	f->bits &= ~SFIO_DCDOWN; \
	}

/* fast peek of a stream */
#define _SFAVAIL(f,s,n)	((n) = (f)->endb - ((s) = (f)->next) )
#define SFRPEEK(f,s,n)	(_SFAVAIL(f,s,n) > 0 ? (n) : \
				((n) = SFFILBUF(f,-1), (s) = (f)->next, (n)) )
#define SFWPEEK(f,s,n)	(_SFAVAIL(f,s,n) > 0 ? (n) : \
				((n) = SFFLSBUF(f,-1), (s) = (f)->next, (n)) )

/* more than this for a line buffer, we might as well flush */
#define HIFORLINE	128

/* string stream extent */
#define SFSTRSIZE(f)	{ Sfoff_t s = (f)->next - (f)->data; \
			  if(s > (f)->here) \
			    { (f)->here = s; if(s > (f)->extent) (f)->extent = s; } \
			}

/* control flags for open() */
#ifdef O_CREAT
#define _has_oflags	1
#else	/* for example, research UNIX */
#define _has_oflags	0
#define O_CREAT		004
#define O_TRUNC		010
#define O_APPEND	020
#define O_EXCL		040

#ifndef O_RDONLY
#define O_RDONLY	000
#endif
#ifndef O_WRONLY
#define O_WRONLY	001
#endif
#ifndef O_RDWR
#define O_RDWR		002
#endif
#endif /*O_CREAT*/

#ifndef O_BINARY
#define O_BINARY	000
#endif
#ifndef O_TEXT
#define O_TEXT		000
#endif
#ifndef O_TEMPORARY
#define O_TEMPORARY	000
#endif

#define SFIO_RADIX	64	/* maximum integer conversion base */

#define SFIO_MAXCHAR	((uchar)(~0))

/* floating point to ASCII conversion */
#define SFIO_MAXEXP10	6
#define SFIO_MAXPOW10	(1 << SFIO_MAXEXP10)
#if !_ast_fltmax_double
#define SFIO_FDIGITS	1024		/* max allowed fractional digits */
#define SFIO_IDIGITS	(8*1024)	/* max number of digits in int part */
#else
#define SFIO_FDIGITS	256		/* max allowed fractional digits */
#define SFIO_IDIGITS	1024		/* max number of digits in int part */
#endif
#define SFIO_MAXDIGITS	(((SFIO_FDIGITS+SFIO_IDIGITS)/sizeof(int) + 1)*sizeof(int))

/* tables for numerical translation */
#define _Sfpos10	(_Sftable.sf_pos10)
#define _Sfneg10	(_Sftable.sf_neg10)
#define _Sfdec		(_Sftable.sf_dec)
#define _Sfdigits	(_Sftable.sf_digits)
#define _Sfcvinitf	(_Sftable.sf_cvinitf)
#define _Sfcvinit	(_Sftable.sf_cvinit)
#define _Sffmtposf	(_Sftable.sf_fmtposf)
#define _Sffmtintf	(_Sftable.sf_fmtintf)
#define _Sfcv36		(_Sftable.sf_cv36)
#define _Sfcv64		(_Sftable.sf_cv64)
#define _Sftype		(_Sftable.sf_type)
#define _Sfieee		(&_Sftable.sf_ieee)
#define _Sffpow10	(_Sftable.sf_flt_pow10)
#define _Sfdpow10	(_Sftable.sf_dbl_pow10)
#define _Sflpow10	(_Sftable.sf_ldbl_pow10)

typedef struct _sftab_
{	Sfdouble_t	sf_pos10[SFIO_MAXEXP10];	/* positive powers of 10	*/
	Sfdouble_t	sf_neg10[SFIO_MAXEXP10];	/* negative powers of 10	*/
	uchar		sf_dec[200];		/* ASCII reps of values < 100	*/
	char*		sf_digits;		/* digits for general bases	*/
	int		(*sf_cvinitf)(void);	/* initialization function	*/
	int		sf_cvinit;		/* initialization state		*/
	Fmtpos_t*	(*sf_fmtposf)(Sfio_t*,const char*,va_list,Sffmt_t*,int);
	char*		(*sf_fmtintf)(const char*,int*);
	float*		sf_flt_pow10;		/* float powers of 10		*/
	double*		sf_dbl_pow10;		/* double powers of 10		*/
	Sfdouble_t*	sf_ldbl_pow10;		/* Sfdouble_t powers of 10	*/
	uchar		sf_cv36[SFIO_MAXCHAR+1];	/* conversion for base [2-36]	*/
	uchar		sf_cv64[SFIO_MAXCHAR+1];	/* conversion for base [37-64]	*/
	uchar		sf_type[SFIO_MAXCHAR+1];	/* conversion formats&types	*/
} Sftab_t;

/* thread-safe macro/function to initialize _Sfcv* conversion tables */
#define SFCVINIT()      (_Sfcvinit ? 1 : (_Sfcvinit = (*_Sfcvinitf)()) )

/* sfucvt() converts decimal integers to ASCII */
#define SFDIGIT(v,scale,digit) \
	{ if(v < 5*scale) \
		if(v < 2*scale) \
			if(v < 1*scale) \
				{ digit = '0'; } \
			else	{ digit = '1'; v -= 1*scale; } \
		else	if(v < 3*scale) \
				{ digit = '2'; v -= 2*scale; } \
			else if(v < 4*scale) \
				{ digit = '3'; v -= 3*scale; } \
			else	{ digit = '4'; v -= 4*scale; } \
	  else	if(v < 7*scale) \
			if(v < 6*scale) \
				{ digit = '5'; v -= 5*scale; } \
			else	{ digit = '6'; v -= 6*scale; } \
		else	if(v < 8*scale) \
				{ digit = '7'; v -= 7*scale; } \
			else if(v < 9*scale) \
				{ digit = '8'; v -= 8*scale; } \
			else	{ digit = '9'; v -= 9*scale; } \
	}
#define sfucvt(v,s,n,list,type,utype) \
	{ while((utype)v >= 10000) \
	  {	n = v; v = (type)(((utype)v)/10000); \
		n = (type)((utype)n - ((utype)v)*10000); \
	  	s -= 4; SFDIGIT(n,1000,s[0]); SFDIGIT(n,100,s[1]); \
			s[2] = *(list = (char*)_Sfdec + (n <<= 1)); s[3] = *(list+1); \
	  } \
	  if(v < 100) \
	  { if(v < 10) \
	    { 	s -= 1; s[0] = (char)('0'+v); \
	    } else \
	    { 	s -= 2; s[0] = *(list = (char*)_Sfdec + (v <<= 1)); s[1] = *(list+1); \
	    } \
	  } else \
	  { if(v < 1000) \
	    { 	s -= 3; SFDIGIT(v,100,s[0]); \
			s[1] = *(list = (char*)_Sfdec + (v <<= 1)); s[2] = *(list+1); \
	    } else \
	    {	s -= 4; SFDIGIT(v,1000,s[0]); SFDIGIT(v,100,s[1]); \
			s[2] = *(list = (char*)_Sfdec + (v <<= 1)); s[3] = *(list+1); \
	    } \
	  } \
	}

/* handy functions */
#undef min
#undef max
#define min(x,y)	((x) < (y) ? (x) : (y))
#define max(x,y)	((x) > (y) ? (x) : (y))

/* fast functions for memory copy and memory clear */
#define memclear(s,n)	memzero(s,n)

/* note that MEMCPY advances the associated pointers */
#define MEMCPY(to,fr,n) \
	switch(n) \
	{ default : memcpy(to,fr,n); to += n; fr += n; break; \
	  case  7 : *to++ = *fr++;	\
		/* FALLTHROUGH */	\
	  case  6 : *to++ = *fr++;	\
		/* FALLTHROUGH */	\
	  case  5 : *to++ = *fr++;	\
		/* FALLTHROUGH */	\
	  case  4 : *to++ = *fr++;	\
		/* FALLTHROUGH */	\
	  case  3 : *to++ = *fr++;	\
		/* FALLTHROUGH */	\
	  case  2 : *to++ = *fr++;	\
		/* FALLTHROUGH */	\
	  case  1 : *to++ = *fr++;	\
	}
#define MEMSET(s,c,n) \
	switch(n) \
	{ default : memset(s,(int)c,n); s += n; break; \
	  case  7 : *s++ = c;		\
		    /* FALLTHROUGH */	\
	  case  6 : *s++ = c;		\
		    /* FALLTHROUGH */	\
	  case  5 : *s++ = c;		\
		    /* FALLTHROUGH */	\
	  case  4 : *s++ = c;		\
		    /* FALLTHROUGH */	\
	  case  3 : *s++ = c;		\
		    /* FALLTHROUGH */	\
	  case  2 : *s++ = c;		\
		    /* FALLTHROUGH */	\
	  case  1 : *s++ = c;		\
	}

extern Sftab_t		_Sftable;

extern int		_sfpopen(Sfio_t*, int, int, int);
extern int		_sfpclose(Sfio_t*);
extern int		_sfexcept(Sfio_t*, int, ssize_t, Sfdisc_t*);
extern Sfrsrv_t*	_sfrsrv(Sfio_t*, ssize_t);
extern int		_sfsetpool(Sfio_t*);
extern char*		_sfcvt(void*,char*,size_t,int,int*,int*,int*,int);
extern char**		_sfgetpath(char*);

extern Sfextern_t	_Sfextern;

extern int		_sfmode(Sfio_t*, int, int);
extern int		_sftype(const char*, int*, int*);

#ifndef errno
extern int		errno;
#endif

/* for portable encoding of double values */
#ifndef frexpl
#if _ast_fltmax_double
#define frexpl		frexp
#endif
#if !_lib_frexpl
extern long double	frexpl(long double, int*);
#endif
#endif
#ifndef ldexpl
#if _ast_fltmax_double
#define ldexpl		ldexp
#endif
#if !_lib_ldexpl
extern long double	ldexpl(long double, int);
#endif
#endif

#endif /*_SFHDR_H*/

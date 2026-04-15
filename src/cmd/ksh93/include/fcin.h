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
#ifndef _FCIN_H
#define _FCIN_H
/*
 * David Korn
 * AT&T Labs
 *
 * Fast character input with sfio text streams and strings
 *
 */

#include	<sfio.h>

typedef struct _fcin
{
	Sfio_t		*_fcfile;	/* input file pointer */
	unsigned char	*fcbuff;	/* pointer to input buffer */
	unsigned char	*fclast;	/* pointer to end of input buffer */
	unsigned char	*fcptr;		/* pointer to next input char */
	unsigned char	fcchar;		/* saved character */
	short		fclen;		/* last multibyte char len */
	void (*fcfun)(Sfio_t*,const char*,ptrdiff_t,void*);	/* advance function */
	void		*context;	/* context pointer */
	ptrdiff_t	fcleft;		/* for multibyte boundary */
} Fcin_t;

#if SHOPT_MULTIBYTE
#   define fcmbget(x)	(mbwide()?_fcmbget(x):fcget())
    extern int		_fcmbget(short*);
#else
#   define fcmbget(x)	(fcget())
#endif
#define fcfile()	(_Fcin._fcfile)
#define fcgetc()	(*_Fcin.fcptr++ ? _Fcin.fcptr[-1] : fcfill())
#define fcget()		(*_Fcin.fcptr++)
#define fcpeek(n)	(_Fcin.fcptr[n])
#define fcseek(n)	((char*)(_Fcin.fcptr+=(n)))
#define fcfirst()	((char*)_Fcin.fcbuff)
#define fclast()	((char*)_Fcin.fclast)
#define fcleft()	(_Fcin.fclast-_Fcin.fcptr)
#define fcsopen(s)	(_Fcin._fcfile=NULL,_Fcin.fclen=1,_Fcin.fcbuff=_Fcin.fcptr=(unsigned char*)(s))
#define fcsave(x)	(*(x) = _Fcin)
#define fcrestore(x)	(_Fcin = *(x))
extern int		fcfill(void);
extern ssize_t		fcfopen(Sfio_t*);
extern int		fcclose(void);
extern void		fcnotify(void(*)(Sfio_t*,const char*,ptrdiff_t,void*),void*);

extern Fcin_t		_Fcin;		/* used by macros */

#endif /* !_FCIN_H */

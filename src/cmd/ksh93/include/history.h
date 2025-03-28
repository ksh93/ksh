/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2024 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                  David Korn <dgk@research.att.com>                   *
*                  Martijn Dekker <martijn@inlv.org>                   *
*                                                                      *
***********************************************************************/
#ifndef HIST_VERSION
/*
 *	Interface for history mechanism
 *	written by David Korn
 *
 * NOTE: this header defines a public libshell interface,
 * unless _BLD_ksh is defined as nonzero
 */

#include	<ast.h>

#define HIST_CHAR	'!'
#define HIST_VERSION	1		/* history file format version no. */

typedef struct
{
	Sfdisc_t	histdisc;	/* discipline for history */
	Sfio_t		*histfp;	/* history file stream pointer */
	char		*histname;	/* name of history file */
	int32_t		histind;	/* current command number index */
	int		histsize;	/* number of accessible history lines */
#ifdef _HIST_PRIVATE
	_HIST_PRIVATE
#endif /* _HIST_PRIVATE */
} History_t;

typedef struct
{
	int hist_command;
	int hist_line;
	int hist_char;
} Histloc_t;

#if SHOPT_SCRIPTONLY

#define hist_min(hp)	0
#define hist_max(hp)	0
#define sh_histinit()	0
#define hist_cancel(h)	0
#define hist_close(h)	0
#define hist_copy(h)	0
#define hist_eof(h)	0
#define hist_flush(h)	0
#define hist_list(a,out,c,d,e)	sfputr(out,sh_translate(e_unknown),'\n')
#define hist_match(a,b,c,d)	0
#define hist_tell(a,b)		0
#define hist_seek(a,b)		0
#define hist_word(a,b,c)	0

#else

/* the following are readonly */
extern const char	hist_fname[];

extern int _Hist;
#define hist_min(hp)	((_Hist=((int)((hp)->histind-(hp)->histsize)))>=0?_Hist:0)
#define hist_max(hp)	((int)((hp)->histind))
/* these are the history interface routines */
extern int		sh_histinit(void);
extern void 		hist_cancel(History_t*);
extern void 		hist_close(History_t*);
extern int		hist_copy(char*, int, int, int);
extern void 		hist_eof(History_t*);
extern Histloc_t	hist_find(History_t*,char*,int, int, int);
extern void 		hist_flush(History_t*);
extern void 		hist_list(History_t*,Sfio_t*, off_t, int, char*);
extern int		hist_match(History_t*,off_t, char*, int*);
extern off_t		hist_tell(History_t*,int);
extern off_t		hist_seek(History_t*,int);
extern int		hist_iswordbndry(char);
extern char 		*hist_word(char*, int, int);
#if !_BLD_ksh || SHOPT_ESH
    extern Histloc_t	hist_locate(History_t*,int, int, int);
#endif	/* SHOPT_ESH */

#endif /* SHOPT_SCRIPTONLY */

#endif /* HIST_VERSION */

/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2022 Contributors to ksh 93u+m           *
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
#ifndef SEARCHSIZE
/*
 *  edit.h -  common data structure for vi and emacs edit options
 *
 *   David Korn
 *   AT&T Labs
 *
 */

#define SEARCHSIZE	80

#include	"FEATURE/options"
#include        "FEATURE/locale"
#include	"FEATURE/setjmp"
#include	"terminal.h"

#define STRIP		0377
#define LOOKAHEAD	80

#if SHOPT_MULTIBYTE
#   ifndef ESS_MAXCHAR
#	include	"national.h"
#   endif /* ESS_MAXCHAR */
    typedef wchar_t genchar;
#   define CHARSIZE	(sizeof(wchar_t)<=2?3:sizeof(wchar_t))
#else
    typedef char genchar;
#   define CHARSIZE	1
#endif /* SHOPT_MULTIBYTE */

#define TABSIZE	8
#define PRSIZE	256
#define MAXLINE	1024		/* longest edit line permitted */

typedef struct _edit_pos
{
	unsigned short line;
	unsigned short col;
} Edpos_t;

typedef struct edit
{
	sigjmp_buf e_env;
	int	e_intr;
	int	e_kill;
	int	e_erase;
	int	e_werase;
	int	e_eof;
	int	e_lnext;
	int	e_plen;		/* length of prompt string */
	char	e_crlf;		/* zero if cannot return to beginning of line */
	char	e_keytrap;	/* set when in keytrap */
	int	e_llimit;	/* line length limit */
	int	e_hline;	/* current history line number */
	int	e_hloff;	/* line number offset for command */
	int	e_hismin;	/* minimum history line number */
	int	e_hismax;	/* maximum history line number */
	int	e_raw;		/* set when in raw mode or alt mode */
	int	e_cur;		/* current line position */
	int	e_eol;		/* end-of-line position */
	int	e_pcur;		/* current physical line position */
	int	e_peol;		/* end of physical line position */
	int	e_mode;		/* edit mode */
	int	e_lookahead;	/* index in look-ahead buffer */
	int	e_fcol;		/* first column */
	int	e_wsize;	/* width of display window */
#if SHOPT_MULTIBYTE
	int	e_savedwidth;	/* saved width of a character */
#endif /* SHOPT_MULTIBYTE */
	char	*e_outbase;	/* pointer to start of output buffer */
	char	*e_outptr;	/* pointer to position in output buffer */
	char	*e_outlast;	/* pointer to end of output buffer */
	genchar	*e_inbuf;	/* pointer to input buffer */
	char	*e_prompt;	/* pointer to trimmed final line of PS1 prompt, used when redrawing command line */
	genchar	*e_killbuf;	/* pointer to delete buffer */
	char	e_search[SEARCHSIZE];	/* search string */
	genchar	*e_physbuf;	/* temporary workspace buffer */
	int	e_lbuf[LOOKAHEAD];/* pointer to look-ahead buffer */
	int	e_fd;		/* file descriptor */
	int	e_ttyspeed;	/* line speed, also indicates tty parameters are valid */
	int	e_tabcount;
#ifdef _hdr_utime
	ino_t	e_tty_ino;
	dev_t	e_tty_dev;
	char	*e_tty;
#endif
#if SHOPT_OLDTERMIO
	char	e_echoctl;
	char	e_tcgeta;
	struct termio e_ott;
#endif
	int	*e_globals;	/* global variables */
	genchar	*e_window;	/* display window image */
	char	e_inmacro;	/* processing macro expansion */
	char	e_vi_insert[2];	/* for sh_keytrap */
	int32_t e_col;		/* for sh_keytrap */
	struct termios	e_ttyparm;      /* initial tty parameters */
	struct termios	e_nttyparm;     /* raw tty parameters */
	struct termios e_savetty;	/* saved terminal state */
	int	e_savefd;	/* file descriptor for saved terminal state */
	char	e_macro[4];	/* macro buffer */
	void	*e_vi;		/* vi specific data */
	void	*e_emacs;	/* emacs specific data */
	char	*e_stkptr;	/* saved stack pointer */
	int	e_stkoff;	/* saved stack offset */
	char	**e_clist;	/* completion list after <ESC>= */
	int	e_nlist;	/* number of elements on completion list */
#if SHOPT_ESH || SHOPT_VSH
	int	e_multiline;	/* allow multiple lines for editing */
#endif
	int	e_winsz;	/* columns in window */ 
	Edpos_t	e_curpos;	/* cursor line and column */
	Namval_t *e_default;	/* variable containing default value */
	Namval_t *e_term;	/* TERM variable */
	char 	e_termname[80];	/* terminal name */
} Edit_t;

#undef MAXWINDOW
#define MAXWINDOW	300	/* maximum width window */
#define FAST	2
#define SLOW	1
#define ESC	cntl('[')
#define	UEOF	-2			/* user eof char synonym */
#define	UINTR	-3			/* user intr char synonym */
#define	UERASE	-4			/* user erase char synonym */
#define	UKILL	-5			/* user kill char synonym */
#define	UWERASE	-6			/* user word erase char synonym */
#define	ULNEXT	-7			/* user next literal char synonym */

#if ( 'a' == 97) /* ASCII? */
#   define	cntl(x)		(x&037)
#else
#   define cntl(c) (c=='D'?55:(c=='E'?45:(c=='F'?46:(c=='G'?'\a':(c=='H'?'\b': \
		(c=='I'?'\t':(c=='J'?'\n':(c=='T'?60:(c=='U'?61:(c=='V'?50: \
		(c=='W'?38:(c=='Z'?63:(c=='['?39:(c==']'?29: \
		(c<'J'?c+1-'A':(c+10-'J'))))))))))))))))
#endif

extern void	ed_putchar(Edit_t*, int);
extern void	ed_ringbell(void);
extern void	ed_setup(Edit_t*,int, int);
extern void	ed_flush(Edit_t*);
extern int	ed_getchar(Edit_t*,int);
extern int	ed_virt_to_phys(Edit_t*,genchar*,genchar*,int,int,int);
extern int	ed_window(void);
extern void	ed_ungetchar(Edit_t*,int);
extern int	ed_viread(void*, int, char*, int, int);
extern int	ed_read(void*, int, char*, int, int);
extern int	ed_emacsread(void*, int, char*, int, int);
extern Edpos_t	ed_curpos(Edit_t*, genchar*, int, int, Edpos_t);
extern int	ed_setcursor(Edit_t*, genchar*, int, int, int);
extern int	ed_macro(Edit_t*,int);
extern int	ed_expand(Edit_t*, char[],int*,int*,int,int);
extern int	ed_fulledit(Edit_t*);
extern void	*ed_open(void);
#if SHOPT_MULTIBYTE
	extern int ed_internal(const char*, genchar*);
	extern int ed_external(const genchar*, char*);
	extern void ed_gencpy(genchar*,const genchar*);
	extern void ed_genncpy(genchar*,const genchar*,int);
	extern int ed_genlen(const genchar*);
#endif /* SHOPT_MULTIBYTE */

extern const char	e_runvi[];

#if SHOPT_HISTEXPAND

/* flags */

#define	HIST_EVENT	0x1	/* event designator seen */
#define HIST_QUESTION	0x2	/* question mark event designator */
#define	HIST_HASH	0x4	/* hash event designator */
#define HIST_WORDDSGN	0x8	/* word designator seen */
#define HIST_QUICKSUBST	0x10	/* quick substitution designator seen */
#define HIST_SUBSTITUTE	0x20	/* for substitution loop */
#define	HIST_NEWLINE	0x40	/* newline in squashed white space */

/* modifier flags */

#define	HIST_PRINT		0x100	/* print new command */
#define	HIST_QUOTE		0x200	/* quote resulting history line */
#define	HIST_QUOTE_BR		0x400	/* quote every word on space break */
#define	HIST_GLOBALSUBST	0x800	/* apply substitution globally */

#define	HIST_ERROR		0x1000	/* an error occurred */

/* flags to be returned */

#define	HIST_FLAG_RETURN_MASK	(HIST_EVENT|HIST_PRINT|HIST_ERROR)

extern int hist_expand(const char *, char **);

#endif /* SHOPT_HISTEXPAND */

#if SHOPT_ESH
extern void	emacs_redraw(void*);
#endif /* SHOPT_ESH */

#if SHOPT_VSH
extern void	vi_redraw(void*);
#endif /* SHOPT_VSH */

#endif /* !SEARCHSIZE */

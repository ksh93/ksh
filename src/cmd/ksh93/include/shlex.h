/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2023 Contributors to ksh 93u+m           *
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
#ifndef NOTSYM
/*
 *	UNIX shell
 *	Written by David Korn
 *	These are the definitions for the lexical analyzer
 */

#include	<cdt.h>
#include	"shnodes.h"
#include	"shtable.h"
#include	"lexstates.h"

/*
 * This structure allows for arbitrary depth nesting of (...), {...}, [...]
 */
struct _shlex_pvt_lexstate_
{
	char		incase;		/* 1 for case pattern, 2 after case */
	char		intest;		/* 1 inside [[ ... ]] */
	char		testop1;	/* 1 when unary test op legal */
	char		testop2;	/* 1 when binary test op legal */
	char		reservok;	/* >0 for reserved word legal */
	char		skipword;	/* next word can't be reserved */
	char		last_quote;	/* last multi-line quote character */
	char		nestedbrace;	/* ${var op {...}} */
};
struct _shlex_pvt_lexdata_
{
	char		nocopy;
	char		paren;
	char		dolparen;	/* set during the comsub() lexical analysis hack */
	unsigned short	dolparen_eqparen;	/* flags up =( ... ) within a comsub */
	char		dolparen_arithexp;	/* set while comsub() is lexing an arithmetic expansion */
	char		nest;
	char		docword;
	char		nested_tilde;
	char 		*docend;
	char		noarg;
	char		warn;
	char		message;
	char		arith;
	char 		*first;
	int		level;
	int		lastc;
	int		lex_state;
	int		docextra;
#if SHOPT_KIA
	off_t		kiaoff;
#endif
};

/*
 * Main lexer struct.
 */
typedef struct  _shlex_
{
	struct argnod	*arg;		/* current word */
	struct ionod	*heredoc;	/* pending here document list */
	int		token;		/* current token number */
	int		lastline;	/* last line number */
	int		lasttok;	/* previous token number */
	int		digits;		/* numerical value with word token */
	char		aliasok;	/* on when alias is legal */
	char		assignok;	/* on when name=value is legal */
	char		inexec;		/* on when processing exec */
	char		intypeset;	/* 1 when processing typeset, 2 when processing enum */
	char		comp_assign;	/* in compound assignment */
	char		comsub;		/* parsing command substitution */
	char		noreserv;	/* reserved works not legal */
	int		inlineno;	/* saved value of sh.inlineno */
	int		firstline;	/* saved value of sh.st.firstline */
	int		assignlevel;	/* nesting level for assignment */
#if SHOPT_KIA
	Sfio_t		*kiafile;	/* kia output file */
	Sfio_t		*kiatmp;	/* kia reference file */
	unsigned long	script;		/* script entity number */
	unsigned long	fscript;	/* script file entity number */
	unsigned long	current;	/* current entity number */
	unsigned long	unknown;	/* <unknown> entity number */
	off_t		kiabegin;	/* offset of first entry */
	char		*scriptname;	/* name of script file */
	Dt_t		*entity_tree;	/* for entity IDs */
#endif /* SHOPT_KIA */
	/* The following two struct members are considered private to lex.c */
	struct _shlex_pvt_lexdata_  lexd;
	struct _shlex_pvt_lexstate_  lex;
} Lex_t;

/* symbols for parsing */
#define NL		'\n'
#define NOTSYM		'!'
#define SYMRES		0400		/* reserved word symbols */
#define DOSYM		(SYMRES|01)
#define FISYM		(SYMRES|02)
#define ELIFSYM		(SYMRES|03)
#define ELSESYM		(SYMRES|04)
#define INSYM		(SYMRES|05)
#define THENSYM		(SYMRES|06)
#define DONESYM		(SYMRES|07)
#define ESACSYM		(SYMRES|010)
#define IFSYM		(SYMRES|011)
#define FORSYM		(SYMRES|012)
#define WHILESYM	(SYMRES|013)
#define UNTILSYM	(SYMRES|014)
#define CASESYM		(SYMRES|015)
#define FUNCTSYM	(SYMRES|016)
#define SELECTSYM	(SYMRES|017)
#define TIMESYM		(SYMRES|020)
#define NSPACESYM	(SYMRES|021)

#define SYMREP		01000		/* symbols for doubled characters */
#define BREAKCASESYM	(SYMREP|';')
#define ANDFSYM		(SYMREP|'&')
#define ORFSYM		(SYMREP|'|')
#define IOAPPSYM	(SYMREP|'>')
#define IODOCSYM	(SYMREP|'<')
#define EXPRSYM		(SYMREP|'(')
#define BTESTSYM 	(SYMREP|'[')
#define ETESTSYM	(SYMREP|']')

#define SYMMASK		0170000
#define SYMPIPE		010000	/* trailing '|' */
#define SYMLPAR		020000	/* trailing LPAREN */
#define SYMAMP		040000	/* trailing '&' */
#define SYMGT		0100000	/* trailing '>' */
#define SYMSEMI		0110000	/* trailing ';' */
#define SYMSHARP	0120000	/* trailing '#' */
#define IOSEEKSYM	(SYMSHARP|'<')
#define IOMOV0SYM	(SYMAMP|'<')
#define IOMOV1SYM	(SYMAMP|'>')
#define FALLTHRUSYM	(SYMAMP|';')
#define COOPSYM		(SYMAMP|'|')
#define IORDWRSYM	(SYMGT|'<')
#define IORDWRSYMT	(SYMSEMI|'<')
#define IOCLOBSYM	(SYMPIPE|'>')
#define IPROCSYM	(SYMLPAR|'<')
#define OPROCSYM	(SYMLPAR|'>')
#define EOFSYM		04000	/* end-of-file */
#define TESTUNOP	04001
#define TESTBINOP	04002
#define IOVNAME		04004

/* additional parser flag, others in <shell.h> */
#define SH_EMPTY	04
#define SH_NOIO		010
#define	SH_ASSIGN	020
#define	SH_FUNDEF	040
#define SH_ARRAY	0100
#define SH_SEMI		0200	/* semicolon after NL ok */

#define SH_COMPASSIGN	010	/* allow compound assignments only */

extern const char		e_unexpected[];
extern const char		e_unmatched[];
extern const char		e_endoffile[];
extern const char		e_newline[];

/* odd chars */
#define LBRACE	'{'
#define RBRACE	'}'
#define LPAREN	'('
#define RPAREN	')'
#define LBRACT	'['
#define RBRACT	']'

extern int		sh_lex(Lex_t*);
extern Shnode_t		*sh_dolparen(Lex_t*);
extern Lex_t		*sh_lexopen(Lex_t*, int);
extern void 		sh_lexskip(Lex_t*,int,int,int);
extern noreturn void 	sh_syntax(Lex_t*);
#if SHOPT_KIA
    extern int                  kiaclose(Lex_t *);
    extern unsigned long        kiaentity(Lex_t*, const char*,int,int,int,int,unsigned long,int,int,const char*);
#endif /* SHOPT_KIA */

#endif /* !NOTSYM */

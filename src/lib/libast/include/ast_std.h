/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2025 Contributors to ksh 93u+m           *
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
*                  Lev Kujawski <int21h@mailbox.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
/*
 * Advanced Software Technology Library
 * AT&T Research
 *
 * a union of standard headers that works
 * with local extensions enabled
 * and local omission compensation
 */

#ifndef _AST_STD_H
#define _AST_STD_H		1
#define _AST_STD_I		1

#include <ast_standards.h>
#include <ast_common.h>

#if _BLD_ast
#define _BLD_aso	1
#define _BLD_cdt	1
#define _BLD_sfio	1
#endif

#ifdef	_SFSTDIO_H
#define _SKIP_SFSTDIO_H
#else
#define _SFSTDIO_H
#ifndef FILE
#ifndef _SFIO_H
struct _sfio_s;
#endif
#define FILE		struct _sfio_s
#ifndef __FILE_typedef
#define __FILE_typedef	1
#endif
#ifndef _FILEDEFED
#define _FILEDEFED	1
#endif
#endif
#endif

#include <ast_lib.h>
#if !_lib_fork
#error In 2022, libast joined the 21st century and started requiring fork(2).
#endif
#include <ast_sys.h>
#include <ast_fcntl.h>
#include <ast_limits.h>

#ifdef	_SKIP_SFSTDIO_H
#undef	_SKIP_SFSTDIO_H
#else
#undef	_SFSTDIO_H
#undef	FILE
#endif

#ifndef FILE
#ifndef _SFIO_H
struct _sfio_s;
#endif
#define FILE            struct _sfio_s
#ifndef __FILE_typedef
#define __FILE_typedef  1
#endif
#ifndef _FILEDEFED
#define _FILEDEFED      1
#endif
#ifndef ____FILE_defined
#define ____FILE_defined 1
#endif
#ifndef __FILE_defined
#define __FILE_defined 1
#endif
#ifndef __DEFINED_FILE
#define __DEFINED_FILE 1
#endif
#endif

/* locale stuff */

#if !_hdr_locale

struct lconv
{
	char*	decimal_point;
	char*	thousands_sep;
	char*	grouping;
	char*	int_curr_symbol;
	char*	currency_symbol;
	char*	mon_decimal_point;
	char*	mon_thousands_sep;
	char*	mon_grouping;
	char*	positive_sign;
	char*	negative_sign;
	char	int_frac_digits;
	char	frac_digits;
	char	p_cs_precedes;
	char	p_sep_by_space;
	char	n_cs_precedes;
	char	n_sep_by_space;
	char	p_sign_posn;
	char	n_sign_posn;
};

#endif

#undef	getenv
#define getenv		_ast_getenv

#undef	setenviron
#define setenviron	_ast_setenviron

extern char*		getenv(const char*);

#undef	localeconv
#define localeconv	_ast_localeconv

#undef	setlocale
#define setlocale	_ast_setlocale

extern struct lconv*	localeconv(void);
extern char*		setenviron(const char*);
extern char*		setlocale(int, const char*);

#define AST_MESSAGE_SET		3	/* see <mc.h> mcindex()		*/

/*
 * maintain this order when adding categories
 * AST_LC_COUNT is the number of categories
 * The numbering must be consecutive and correspond to the order of lc_categories[] in comp/setlocale.c
 * The initializers in port/lc.c must also be kept in sync with this
 */

#define AST_LC_ALL		0
#define AST_LC_COLLATE		1
#define AST_LC_CTYPE		2
#define AST_LC_MESSAGES		3
#define AST_LC_MONETARY		4
#define AST_LC_NUMERIC		5
#define AST_LC_TIME		6
#define AST_LC_IDENTIFICATION	7
#define AST_LC_ADDRESS		8
#define AST_LC_NAME		9
#define AST_LC_TELEPHONE	10
#define AST_LC_MEASUREMENT	11
#define AST_LC_PAPER		12
#define AST_LC_COUNT		13	/* number of preceding AST_LC_* defines */
#define AST_LC_LANG		255

#define AST_LC_internal		1
#define AST_LC_7bit		(1 << 23)
#if AST_NOMULTIBYTE
#define AST_LC_utf8		0
#else
#define AST_LC_utf8		(1 << 24)
#endif /* AST_NOMULTIBYTE */
#define AST_LC_test		(1 << 25)
#define AST_LC_setenv		(1 << 26)
#define AST_LC_find		(1 << 27)
#define AST_LC_debug		(1 << 28)
#define AST_LC_setlocale	(1 << 29)
#define AST_LC_translate	(1 << 30)

#ifndef LC_ALL
#define LC_ALL			(-AST_LC_ALL)
#endif
#ifndef LC_COLLATE
#define LC_COLLATE		(-AST_LC_COLLATE)
#endif
#ifndef LC_CTYPE
#define LC_CTYPE		(-AST_LC_CTYPE)
#endif
#ifndef LC_MESSAGES
#define LC_MESSAGES		(-AST_LC_MESSAGES)
#endif
#ifndef LC_MONETARY
#define LC_MONETARY		(-AST_LC_MONETARY)
#endif
#ifndef LC_NUMERIC
#define LC_NUMERIC		(-AST_LC_NUMERIC)
#endif
#ifndef LC_TIME
#define LC_TIME			(-AST_LC_TIME)
#endif
#ifndef LC_ADDRESS
#define LC_ADDRESS		(-AST_LC_ADDRESS)
#endif
#ifndef LC_IDENTIFICATION
#define LC_IDENTIFICATION	(-AST_LC_IDENTIFICATION)
#endif
#ifndef LC_NAME
#define LC_NAME			(-AST_LC_NAME)
#endif
#ifndef LC_TELEPHONE
#define LC_TELEPHONE		(-AST_LC_TELEPHONE)
#endif
#ifndef LC_MEASUREMENT
#define LC_MEASUREMENT		(-AST_LC_MEASUREMENT)
#endif
#ifndef LC_PAPER
#define LC_PAPER		(-AST_LC_PAPER)
#endif
#ifndef LC_LANG
#define LC_LANG			(-AST_LC_LANG)
#endif

#undef	strcoll
#if _std_strcoll
#define strcoll		_ast_info.locale.collate
#else
#define strcoll		strcmp
#endif

/*
 * This struct defines all the global ast.* variables.
 * It is initialized in misc/state.c, and not by name -- so the order must be kept in sync.
 * Changing the order also breaks ABI compat for dynamic libraries.
 */
typedef struct
{
	char		*id;
	uint32_t	version;
	uint32_t	env_serial;	/* increased by setenviron */

	struct				/* ast.locale.* -- stuff set in setlocale.c */
	{
	int             (*collate)(const char*, const char*);  /* strcoll(3) is redefined as this, above */
	void		*uc2wc;		/* iconv descriptor for converting unicode to locale's wide char */
	uint32_t	serial;
	uint32_t	set;
	}		locale;

#if !AST_NOMULTIBYTE
	struct				/* ast.mb.* -- multibyte encoding/decoding state */
	{
	int		cur_max;	/* current maximum length in bytes of a character: > 1 == multibyte locale */
	uint32_t	sync;		/* length of invalid multibyte character */
	wchar_t		tmp_w;		/* scratch */
	int		tmp_i;		/* scratch */
	int		(*alpha)(wchar_t);
	int		(*conv)(char*, wchar_t);
	int		(*len)(const char*, size_t);
	int		(*towc)(wchar_t*, const char*, size_t);
	int		(*width)(wchar_t);
	size_t		(*xfrm)(char*, const char*, size_t);
	}		mb;
#endif
} _Ast_info_t;

/* ast is defined as _ast_info in ast.h */
extern _Ast_info_t	_ast_info;

/* direct macro access for bsd crossover */

#if !defined(memzero) && !defined(_lib_memzero)
#define memzero(b,n)	memset(b,0,n)
#endif

#if !defined(remove)
extern int		remove(const char*);
#endif

#if !defined(rename)
extern int		rename(const char*, const char*);
#endif

/*
 * and finally, standard interfaces hijacked by AST
 * _AST_STD_I delays headers that require <ast_map.h>
 */

#include <ast_map.h>

#undef	_AST_STD_I

#if _REGEX_H < 0
#undef	_REGEX_H
#include <regex.h>
#endif

#endif

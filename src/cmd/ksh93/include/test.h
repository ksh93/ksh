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
#ifndef TEST_ARITH
/*
 *	UNIX shell
 *	David Korn
 *	AT&T Labs
 *
 */

#include	"defs.h"
#include	"shtable.h"
/*
 *  These are the binary test operators
 *  See also shtab_testops[] in data/testops.c
 */

#define TEST_ARITH	040	/* arithmetic operators */
#define TEST_ANDOR	0200	/* logical operators: -a, -o */
#define TEST_STRCMP	0100	/* literal string comparison; turn off bit for pattern matching */

#define TEST_NE		(TEST_ARITH|9)
#define TEST_EQ		(TEST_ARITH|4)
#define TEST_GE		(TEST_ARITH|5)
#define TEST_GT		(TEST_ARITH|6)
#define TEST_LE		(TEST_ARITH|7)
#define TEST_LT		(TEST_ARITH|8)
#define TEST_OR		(TEST_ANDOR|1)
#define TEST_AND	(TEST_ANDOR|2)
#define TEST_SNE	(TEST_STRCMP|TEST_PNE)
#define TEST_SEQ	(TEST_STRCMP|TEST_PEQ)
#define TEST_PNE	1
#define TEST_PEQ	14
#define TEST_EF		3
#define TEST_NT		10
#define TEST_OT		12
#define TEST_SLT	16
#define TEST_SGT	17
#define TEST_REP	20

extern int test_unop(int, const char*);
extern int test_inode(const char*, const char*);
extern int test_binop(int, const char*, const char*);

extern const char	sh_opttest[];
extern const char	test_opchars[];
extern const char	e_argument[];
extern const char	e_missing[];
extern const char	e_badop[];
extern const char	e_tstbegin[];
extern const char	e_tstend[];

#endif /* TEST_ARITH */

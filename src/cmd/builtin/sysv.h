/*
 * Changes for banner(1)
 *    @(#)Copyright (c) 1995, Simon J. Gerraty.
 *
 *    This is free software.  It comes with NO WARRANTY.
 *    Permission to use, modify and distribute this source code
 *    is granted subject to the following conditions.
 *    1/ that the above copyright notice and this notice
 *    are preserved in all copies and that due credit be given
 *    to the author.
 *    2/ that any changes to this code are clearly commented
 *    as such so that the author does not get blamed for bugs
 *    other than his own.
 *
 *    Please send copies of changes and bug-fixes to:
 *    sjg@zen.void.oz.au
 */

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* clang-format off */
#include "fontkey.h"

/*
 * This is the real banner char set
 */
const char bandata_sysv[][HEIGHT] =
{
	{
		c_______,
		c_______,
		c_______,
		c_______,
		c_______,
		c_______,
		c_______,
		c_______,
		c_______
	},			/*   */

	{
		c_______,
		c__111__,
		c__111__,
		c__111__,
		c___1___,
		c_______,
		c__111__,
		c__111__,
		c_______
	},			/* ! */
	{
		c_______,
		c111_111,
		c111_111,
		c_1___1_,
		c_______,
		c_______,
		c_______,
		c_______,
		c_______
	},			/* " */
	{
		c_______,
		c__1_1__,
		c__1_1__,
		c1111111,
		c__1_1__,
		c1111111,
		c__1_1__,
		c__1_1__,
		c_______
	},			/* # */
	{
		c_______,
		c_11111_,
		c1__1__1,
		c1__1___,
		c_11111_,
		c___1__1,
		c1__1__1,
		c_11111_,
		c_______
	},			/* $ */
	{
		c_______,
		c111___1,
		c1_1__1_,
		c111_1__,
		c___1___,
		c__1_111,
		c_1__1_1,
		c1___111,
		c_______
	},			/* % */
	{
		c_______,
		c__11___,
		c_1__1__,
		c__11___,
		c_111___,
		c1___1_1,
		c1____1_,
		c_111__1,
		c_______
	},			/* & */
	{
		c_______,
		c__111__,
		c__111__,
		c___1___,
		c__1____,
		c_______,
		c_______,
		c_______,
		c_______
	},			/* ' */
	{
		c_______,
		c___11__,
		c__1____,
		c_1_____,
		c_1_____,
		c_1_____,
		c__1____,
		c___11__,
		c_______
	},			/* ( */
	{
		c_______,
		c__11___,
		c____1__,
		c_____1_,
		c_____1_,
		c_____1_,
		c____1__,
		c__11___,
		c_______
	},			/* ) */
	{
		c_______,
		c_______,
		c_1___1_,
		c__1_1__,
		c1111111,
		c__1_1__,
		c_1___1_,
		c_______,
		c_______
	},			/* * */
	{
		c_______,
		c_______,
		c___1___,
		c___1___,
		c_11111_,
		c___1___,
		c___1___,
		c_______,
		c_______
	},			/* + */
	{
		c_______,
		c_______,
		c_______,
		c_______,
		c__111__,
		c__111__,
		c___1___,
		c__1____,
		c_______
	},			/* , */
	{
		c_______,
		c_______,
		c_______,
		c_______,
		c_11111_,
		c_______,
		c_______,
		c_______,
		c_______
	},			/* - */
	{
		c_______,
		c_______,
		c_______,
		c_______,
		c_______,
		c__111__,
		c__111__,
		c__111__,
		c_______
	},			/* . */
	{
		c_______,
		c______1,
		c_____1_,
		c____1__,
		c___1___,
		c__1____,
		c_1_____,
		c1______,
		c_______
	},			/* / */
	{
		c_______,
		c__111__,
		c_1___1_,
		c1_____1,
		c1_____1,
		c1_____1,
		c_1___1_,
		c__111__,
		c_______
	},			/* 0 */
	{
		c_______,
		c___1___,
		c__11___,
		c_1_1___,
		c___1___,
		c___1___,
		c___1___,
		c_11111_,
		c_______
	},			/* 1 */
	{
		c_______,
		c_11111_,
		c1_____1,
		c______1,
		c_11111_,
		c1______,
		c1______,
		c1111111,
		c_______
	},			/* 2 */
	{
		c_______,
		c_11111_,
		c1_____1,
		c______1,
		c_11111_,
		c______1,
		c1_____1,
		c_11111_,
		c_______
	},			/* 3 */
	{
		c_______,
		c1______,
		c1____1_,
		c1____1_,
		c1____1_,
		c1111111,
		c_____1_,
		c_____1_,
		c_______
	},			/* 4 */
	{
		c_______,
		c1111111,
		c1______,
		c1______,
		c111111_,
		c______1,
		c1_____1,
		c_11111_,
		c_______
	},			/* 5 */
	{
		c_______,
		c_11111_,
		c1_____1,
		c1______,
		c111111_,
		c1_____1,
		c1_____1,
		c_11111_,
		c_______
	},			/* 6 */
	{
		c_______,
		c1111111,
		c1____1_,
		c____1__,
		c___1___,
		c__1____,
		c__1____,
		c__1____,
		c_______
	},			/* 7 */
	{
		c_______,
		c_11111_,
		c1_____1,
		c1_____1,
		c_11111_,
		c1_____1,
		c1_____1,
		c_11111_,
		c_______
	},			/* 8 */
	{
		c_______,
		c_11111_,
		c1_____1,
		c1_____1,
		c_111111,
		c______1,
		c1_____1,
		c_11111_,
		c_______
	},			/* 9 */
	{
		c_______,
		c___1___,
		c__111__,
		c___1___,
		c_______,
		c___1___,
		c__111__,
		c___1___,
		c_______
	},			/* : */
	{
		c_______,
		c__111__,
		c__111__,
		c_______,
		c__111__,
		c__111__,
		c___1___,
		c__1____,
		c_______
	},			/* ; */
	{
		c_______,
		c____1__,
		c___1___,
		c__1____,
		c_1_____,
		c__1____,
		c___1___,
		c____1__,
		c_______
	},			/* < */
	{
		c_______,
		c_______,
		c_______,
		c_11111_,
		c_______,
		c_11111_,
		c_______,
		c_______,
		c_______
	},			/* = */
	{
		c_______,
		c__1____,
		c___1___,
		c____1__,
		c_____1_,
		c____1__,
		c___1___,
		c__1____,
		c_______
	},			/* > */
	{
		c_______,
		c_11111_,
		c1_____1,
		c______1,
		c___111_,
		c___1___,
		c_______,
		c___1___,
		c_______
	},			/* ? */
	{
		c_______,
		c_11111_,
		c1_____1,
		c1_111_1,
		c1_111_1,
		c1_1111_,
		c1______,
		c_11111_,
		c_______
	},			/* @ */
	{
		c_______,
		c___1___,
		c__1_1__,
		c_1___1_,
		c1_____1,
		c1111111,
		c1_____1,
		c1_____1,
		c_______
	},			/* A */
	{
		c_______,
		c111111_,
		c1_____1,
		c1_____1,
		c111111_,
		c1_____1,
		c1_____1,
		c111111_,
		c_______
	},			/* B */
	{
		c_______,
		c_11111_,
		c1_____1,
		c1______,
		c1______,
		c1______,
		c1_____1,
		c_11111_,
		c_______
	},			/* C */
	{
		c_______,
		c111111_,
		c1_____1,
		c1_____1,
		c1_____1,
		c1_____1,
		c1_____1,
		c111111_,
		c_______
	},			/* D */
	{
		c_______,
		c1111111,
		c1______,
		c1______,
		c11111__,
		c1______,
		c1______,
		c1111111,
		c_______
	},			/* E */
	{
		c_______,
		c1111111,
		c1______,
		c1______,
		c11111__,
		c1______,
		c1______,
		c1______,
		c_______
	},			/* F */
	{
		c_______,
		c_11111_,
		c1_____1,
		c1______,
		c1__1111,
		c1_____1,
		c1_____1,
		c_11111_,
		c_______
	},			/* G */
	{
		c_______,
		c1_____1,
		c1_____1,
		c1_____1,
		c1111111,
		c1_____1,
		c1_____1,
		c1_____1,
		c_______
	},			/* H */
	{
		c_______,
		c__111__,
		c___1___,
		c___1___,
		c___1___,
		c___1___,
		c___1___,
		c__111__,
		c_______
	},			/* I */
	{
		c_______,
		c______1,
		c______1,
		c______1,
		c______1,
		c1_____1,
		c1_____1,
		c_11111_,
		c_______
	},			/* J */
	{
		c_______,
		c1____1_,
		c1___1__,
		c1__1___,
		c111____,
		c1__1___,
		c1___1__,
		c1____1_,
		c_______
	},			/* K */
	{
		c_______,
		c1______,
		c1______,
		c1______,
		c1______,
		c1______,
		c1______,
		c1111111,
		c_______
	},			/* L */
	{
		c_______,
		c1_____1,
		c11___11,
		c1_1_1_1,
		c1__1__1,
		c1_____1,
		c1_____1,
		c1_____1,
		c_______
	},			/* M */
	{
		c_______,
		c1_____1,
		c11____1,
		c1_1___1,
		c1__1__1,
		c1___1_1,
		c1____11,
		c1_____1,
		c_______
	},			/* N */
	{
		c_______,
		c1111111,
		c1_____1,
		c1_____1,
		c1_____1,
		c1_____1,
		c1_____1,
		c1111111,
		c_______
	},			/* O */
	{
		c_______,
		c111111_,
		c1_____1,
		c1_____1,
		c111111_,
		c1______,
		c1______,
		c1______,
		c_______
	},			/* P */
	{
		c_______,
		c_11111_,
		c1_____1,
		c1_____1,
		c1_____1,
		c1___1_1,
		c1____1_,
		c_1111_1,
		c_______
	},			/* Q */
	{
		c_______,
		c111111_,
		c1_____1,
		c1_____1,
		c111111_,
		c1___1__,
		c1____1_,
		c1_____1,
		c_______
	},			/* R */
	{
		c_______,
		c_11111_,
		c1_____1,
		c1______,
		c_11111_,
		c______1,
		c1_____1,
		c_11111_,
		c_______
	},			/* S */
	{
		c_______,
		c1111111,
		c___1___,
		c___1___,
		c___1___,
		c___1___,
		c___1___,
		c___1___,
		c_______
	},			/* T */
	{
		c_______,
		c1_____1,
		c1_____1,
		c1_____1,
		c1_____1,
		c1_____1,
		c1_____1,
		c_11111_,
		c_______
	},			/* U */
	{
		c_______,
		c1_____1,
		c1_____1,
		c1_____1,
		c1_____1,
		c_1___1_,
		c__1_1__,
		c___1___,
		c_______
	},			/* V */
	{
		c_______,
		c1_____1,
		c1__1__1,
		c1__1__1,
		c1__1__1,
		c1__1__1,
		c1__1__1,
		c_11_11_,
		c_______
	},			/* W */
	{
		c_______,
		c1_____1,
		c_1___1_,
		c__1_1__,
		c___1___,
		c__1_1__,
		c_1___1_,
		c1_____1,
		c_______
	},			/* X */
	{
		c_______,
		c1_____1,
		c_1___1_,
		c__1_1__,
		c___1___,
		c___1___,
		c___1___,
		c___1___,
		c_______
	},			/* Y */
	{
		c_______,
		c1111111,
		c_____1_,
		c____1__,
		c___1___,
		c__1____,
		c_1_____,
		c1111111,
		c_______
	},			/* Z */
	{
		c_______,
		c_11111_,
		c_1_____,
		c_1_____,
		c_1_____,
		c_1_____,
		c_1_____,
		c_11111_,
		c_______
	},			/* [ */
	{
		c_______,
		c1______,
		c_1_____,
		c__1____,
		c___1___,
		c____1__,
		c_____1_,
		c______1,
		c_______
	},			/* \ */
	{
		c_______,
		c_11111_,
		c_____1_,
		c_____1_,
		c_____1_,
		c_____1_,
		c_____1_,
		c_11111_,
		c_______
	},			/* ] */
	{
		c_______,
		c___1___,
		c__1_1__,
		c_1___1_,
		c_______,
		c_______,
		c_______,
		c_______,
		c_______
	},			/* ^ */
	{
		c_______,
		c_______,
		c_______,
		c_______,
		c_______,
		c_______,
		c_______,
		c1111111,
		c_______
	},			/* _ */
	{
		c_______,
		c__111__,
		c__111__,
		c___1___,
		c____1__,
		c_______,
		c_______,
		c_______,
		c_______
	},			/* ` */
	{
		c_______,
		c_______,
		c___11__,
		c__1__1_,
		c_1____1,
		c_111111,
		c_1____1,
		c_1____1,
		c_______
	},			/* a */
	{
		c_______,
		c_______,
		c_11111_,
		c_1____1,
		c_11111_,
		c_1____1,
		c_1____1,
		c_11111_,
		c_______
	},			/* b */
	{
		c_______,
		c_______,
		c__1111_,
		c_1____1,
		c_1_____,
		c_1_____,
		c_1____1,
		c__1111_,
		c_______
	},			/* c */
	{
		c_______,
		c_______,
		c_11111_,
		c_1____1,
		c_1____1,
		c_1____1,
		c_1____1,
		c_11111_,
		c_______
	},			/* d */
	{
		c_______,
		c_______,
		c_111111,
		c_1_____,
		c_11111_,
		c_1_____,
		c_1_____,
		c_111111,
		c_______
	},			/* e */
	{
		c_______,
		c_______,
		c_111111,
		c_1_____,
		c_11111_,
		c_1_____,
		c_1_____,
		c_1_____,
		c_______
	},			/* f */
	{
		c_______,
		c_______,
		c__1111_,
		c_1____1,
		c_1_____,
		c_1__111,
		c_1____1,
		c__1111_,
		c_______
	},			/* g */
	{
		c_______,
		c_______,
		c_1____1,
		c_1____1,
		c_111111,
		c_1____1,
		c_1____1,
		c_1____1,
		c_______
	},			/* h */
	{
		c_______,
		c_______,
		c____1__,
		c____1__,
		c____1__,
		c____1__,
		c____1__,
		c____1__,
		c_______
	},			/* i */
	{
		c_______,
		c_______,
		c______1,
		c______1,
		c______1,
		c______1,
		c_1____1,
		c__1111_,
		c_______
	},			/* j */
	{
		c_______,
		c_______,
		c_1____1,
		c_1___1_,
		c_1111__,
		c_1__1__,
		c_1___1_,
		c_1____1,
		c_______
	},			/* k */
	{
		c_______,
		c_______,
		c_1_____,
		c_1_____,
		c_1_____,
		c_1_____,
		c_1_____,
		c_111111,
		c_______
	},			/* l */
	{
		c_______,
		c_______,
		c_1____1,
		c_11__11,
		c_1_11_1,
		c_1____1,
		c_1____1,
		c_1____1,
		c_______
	},			/* m */
	{
		c_______,
		c_______,
		c_1____1,
		c_11___1,
		c_1_1__1,
		c_1__1_1,
		c_1___11,
		c_1____1,
		c_______
	},			/* n */
	{
		c_______,
		c_______,
		c__1111_,
		c_1____1,
		c_1____1,
		c_1____1,
		c_1____1,
		c__1111_,
		c_______
	},			/* o */
	{
		c_______,
		c_______,
		c_11111_,
		c_1____1,
		c_1____1,
		c_11111_,
		c_1_____,
		c_1_____,
		c_______
	},			/* p */
	{
		c_______,
		c_______,
		c__1111_,
		c_1____1,
		c_1____1,
		c_1__1_1,
		c_1___1_,
		c__111_1,
		c_______
	},			/* q */
	{
		c_______,
		c_______,
		c_11111_,
		c_1____1,
		c_1____1,
		c_11111_,
		c_1___1_,
		c_1____1,
		c_______
	},			/* r */
	{
		c_______,
		c_______,
		c__1111_,
		c_1_____,
		c__1111_,
		c______1,
		c_1____1,
		c__1111_,
		c_______
	},			/* s */
	{
		c_______,
		c_______,
		c__11111,
		c____1__,
		c____1__,
		c____1__,
		c____1__,
		c____1__,
		c_______
	},			/* t */
	{
		c_______,
		c_______,
		c_1____1,
		c_1____1,
		c_1____1,
		c_1____1,
		c_1____1,
		c__1111_,
		c_______
	},			/* u */
	{
		c_______,
		c_______,
		c_1____1,
		c_1____1,
		c_1____1,
		c_1____1,
		c__1__1_,
		c___11__,
		c_______
	},			/* v */
	{
		c_______,
		c_______,
		c_1____1,
		c_1____1,
		c_1____1,
		c_1_11_1,
		c_11__11,
		c_1____1,
		c_______
	},			/* w */
	{
		c_______,
		c_______,
		c_1____1,
		c__1__1_,
		c___11__,
		c___11__,
		c__1__1_,
		c_1____1,
		c_______
	},			/* x */
	{
		c_______,
		c_______,
		c__1___1,
		c___1_1_,
		c____1__,
		c____1__,
		c____1__,
		c____1__,
		c_______
	},			/* y */
	{
		c_______,
		c_______,
		c_111111,
		c_____1_,
		c____1__,
		c___1___,
		c__1____,
		c_111111,
		c_______
	},			/* z */
	{
		c_______,
		c__111__,
		c_1_____,
		c_1_____,
		c11_____,
		c_1_____,
		c_1_____,
		c__111__,
		c_______
	},			/* { */
	{
		c_______,
		c___1___,
		c___1___,
		c___1___,
		c_______,
		c___1___,
		c___1___,
		c___1___,
		c_______
	},			/* | */
	{
		c_______,
		c__111__,
		c_____1_,
		c_____1_,
		c_____11,
		c_____1_,
		c_____1_,
		c__111__,
		c_______
	},			/* } */
	{
		c_______,
		c_11____,
		c1__1__1,
		c____11_,
		c_______,
		c_______,
		c_______,
		c_______,
		c_______
	},			/* ~ */

	{
		c_______,
		c_1__1__,
		c1__1__1,
		c__1__1_,
		c_1__1__,
		c1__1__1,
		c__1__1_,
		c_1__1__,
		c1__1__1
	}			/* rub-out */
};
/* clang-format on */

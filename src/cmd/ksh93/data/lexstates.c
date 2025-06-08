/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2025 Contributors to ksh 93u+m           *
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

#include	"shopt.h"
#include	<ast.h>
#include	"lexstates.h"

/* The ST_* state table names are defined in include/lexstates.h */

/*
 * ST_BEGIN (0)
 * This is the initial state for tokens
 */
static const char sh_lexstate0[256] =
{
  /*	nul	soh	stx	etx	eot	enq	ack	bel	*/
	S_EOF,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,

  /*	bs	ht/tab	nl/lf	vt	ff	cr	so	si	*/
#if SHOPT_CRNL
	S_REG,	0,	S_NLTOK,S_REG,	S_REG,	0,	S_REG,	S_REG,
#else
	S_REG,	0,	S_NLTOK,S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
#endif /* SHOPT_CRNL */

  /*	dle	dc1	dc2	dc3	dc4	nak	syn	etb	*/
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,

  /*	can	em	sub	esc	fs	gs	rs	us	*/
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,

  /*	space	!	"	#	$	%	&	'	*/
	0,	S_REG,	S_REG,	S_COM,	S_REG,	S_REG,	S_OP,	S_REG,

  /*	(	)	*	+	,	-	.	/	*/
	S_OP,	S_OP,	S_REG,	S_REG,	S_REG,	S_REG,	S_NAME,	S_REG,

  /*	0	1	2	3	4	5	6	7	*/
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,

  /*	8	9	:	;	<	=	>	?	*/
	S_REG,	S_REG,	S_REG,	S_OP,	S_OP,	S_REG,	S_OP,	S_REG,

  /*	@	A	B	C	D	E	F	G	*/
	S_REG,	S_NAME,	S_NAME,	S_NAME,	S_NAME,	S_NAME,	S_NAME,	S_NAME,

  /*	H	I	J	K	L	M	N	O	*/
	S_NAME,	S_NAME,	S_NAME,	S_NAME,	S_NAME,	S_NAME,	S_NAME,	S_NAME,

  /*	P	Q	R	S	T	U	V	W	*/
	S_NAME,	S_NAME,	S_NAME,	S_NAME,	S_NAME,	S_NAME,	S_NAME,	S_NAME,

  /*	X	Y	Z	[	\	]	^	_	*/
	S_NAME,	S_NAME,	S_NAME,	S_REG,	S_REG,	S_REG,	S_REG,	S_NAME,

  /*	`	a	b	c	d	e	f	g	*/
	S_REG,	S_NAME,	S_NAME,	S_RES,	S_RES,	S_RES,	S_RES,	S_NAME,

  /*	h	i	j	k	l	m	n	o	*/
#if SHOPT_NAMESPACE
	S_NAME,	S_RES,	S_NAME,	S_NAME,	S_NAME,	S_NAME,	S_RES,	S_NAME,
#else
	S_NAME,	S_RES,	S_NAME,	S_NAME,	S_NAME,	S_NAME,	S_NAME,	S_NAME,
#endif /* SHOPT_NAMESPACE */

  /*	p	q	r	s	t	u	v	w	*/
	S_NAME,	S_NAME,	S_NAME,	S_RES,	S_RES,	S_RES,	S_NAME,	S_RES,

  /*	x	y	z	{	|	}	~	del	*/
	S_NAME,	S_NAME,	S_NAME,	S_BRACE,S_OP,	S_BRACE,S_TILDE,S_REG,

	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
};

/*
 * ST_NAME
 * This state is for identifiers
 */
static const char sh_lexstate1[256] =
{
  /*	nul	soh	stx	etx	eot	enq	ack	bel	*/
	S_EOF,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,

  /*	bs	ht/tab	nl/lf	vt	ff	cr	so	si	*/
#if SHOPT_CRNL
	S_REG,	S_BREAK,S_BREAK,S_REG,	S_REG,	S_BREAK,S_REG,	S_REG,
#else
	S_REG,	S_BREAK,S_BREAK,S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
#endif /* SHOPT_CRNL */

  /*	dle	dc1	dc2	dc3	dc4	nak	syn	etb	*/
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,

  /*	can	em	sub	esc	fs	gs	rs	us	*/
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,

  /*	space	!	"	#	$	%	&	'	*/
	S_BREAK,S_EPAT,	S_QUOTE,S_REG,	S_DOL,	S_EPAT,	S_BREAK,S_LIT,

  /*	(	)	*	+	,	-	.	/	*/
	S_BREAK,S_BREAK,S_PAT,	S_EPAT,	S_REG,	S_EPAT,	S_DOT,	S_REG,

  /*	0	1	2	3	4	5	6	7	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	8	9	:	;	<	=	>	?	*/
	0,	0,	S_REG,	S_BREAK,S_BREAK,S_EQ,	S_BREAK,S_PAT,

  /*	@	A	B	C	D	E	F	G	*/
	S_EPAT,	0,	0,	0,	0,	0,	0,	0,

  /*	H	I	J	K	L	M	N	O	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	P	Q	R	S	T	U	V	W	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	X	Y	Z	[	\	]	^	_	*/
	0,	0,	0,	S_BRACT,S_ESC,	S_REG,	S_REG,	0,

  /*	`	a	b	c	d	e	f	g	*/
	S_GRAVE,0,	0,	0,	0,	0,	0,	0,

  /*	h	i	j	k	l	m	n	o	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	p	q	r	s	t	u	v	w	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	x	y	z	{	|	}	~	del	*/
	0,	0,	0,	S_BRACE,S_BREAK,S_BRACE,S_EPAT,	S_REG,

	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,	S_REG,
};

/*
 * ST_NORM
 */
static const char sh_lexstate2[256] =
{
  /*	nul	soh	stx	etx	eot	enq	ack	bel	*/
	S_EOF,	0,	0,	0,	0,	0,	0,	0,

  /*	bs	ht/tab	nl/lf	vt	ff	cr	so	si	*/
#if SHOPT_CRNL
	0,	S_BREAK,S_BREAK,0,	0,	S_BREAK,0,	0,
#else
	0,	S_BREAK,S_BREAK,0,	0,	0,	0,	0,
#endif /* SHOPT_CRNL */

  /*	dle	dc1	dc2	dc3	dc4	nak	syn	etb	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	can	em	sub	esc	fs	gs	rs	us	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	space	!	"	#	$	%	&	'	*/
	S_BREAK,S_EPAT,	S_QUOTE,0,	S_DOL,	S_EPAT,	S_BREAK,S_LIT,

  /*	(	)	*	+	,	-	.	/	*/
	S_BREAK,S_BREAK,S_PAT,	S_EPAT,	0,	S_EPAT,	0,	0,

  /*	0	1	2	3	4	5	6	7	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	8	9	:	;	<	=	>	?	*/
	0,	0,	S_COLON,S_BREAK,S_BREAK,0,	S_BREAK,S_PAT,

  /*	@	A	B	C	D	E	F	G	*/
	S_EPAT,	0,	0,	0,	0,	0,	0,	0,

  /*	H	I	J	K	L	M	N	O	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	P	Q	R	S	T	U	V	W	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	X	Y	Z	[	\	]	^	_	*/
	0,	0,	0,	S_PAT,	S_ESC,	0,	0,	0,

  /*	`	a	b	c	d	e	f	g	*/
	S_GRAVE,0,	0,	0,	0,	0,	0,	0,

  /*	h	i	j	k	l	m	n	o	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	p	q	r	s	t	u	v	w	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	x	y	z	{	|	}	~	del	*/
	0,	0,	0,	S_BRACE,S_BREAK,S_BRACE,S_EPAT,	0,
};

/*
 * ST_LIT
 * for skipping over  '...'
 */
static const char sh_lexstate3[256] =
{
  /*	nul	soh	stx	etx	eot	enq	ack	bel	*/
	S_EOF,	0,	0,	0,	0,	0,	0,	0,

  /*	bs	ht/tab	nl/lf	vt	ff	cr	so	si	*/
	0,	0,	S_NL,	0,	0,	0,	0,	0,

  /*	dle	dc1	dc2	dc3	dc4	nak	syn	etb	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	can	em	sub	esc	fs	gs	rs	us	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	space	!	"	#	$	%	&	'	*/
	0,	0,	0,	0,	0,	0,	0,	S_LIT,

  /*	(	)	*	+	,	-	.	/	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	0	1	2	3	4	5	6	7	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	8	9	:	;	<	=	>	?	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	@	A	B	C	D	E	F	G	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	H	I	J	K	L	M	N	O	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	P	Q	R	S	T	U	V	W	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	X	Y	Z	[	\	]	^	_	*/
	0,	0,	0,	0,	S_ESC2,	0,	0,	0
};

/*
 * ST_QUOTE
 * for skipping over  "..." and `...`
 */
static const char sh_lexstate4[256] =
{
  /*	nul	soh	stx	etx	eot	enq	ack	bel	*/
	S_EOF,	0,	0,	0,	0,	0,	0,	0,

  /*	bs	ht/tab	nl/lf	vt	ff	cr	so	si	*/
	0,	0,	S_NL,	0,	0,	0,	0,	0,

  /*	dle	dc1	dc2	dc3	dc4	nak	syn	etb	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	can	em	sub	esc	fs	gs	rs	us	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	space	!	"	#	$	%	&	'	*/
	0,	0,	S_QUOTE,0,	S_DOL,	0,	0,	0,

  /*	(	)	*	+	,	-	.	/	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	0	1	2	3	4	5	6	7	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	8	9	:	;	<	=	>	?	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	@	A	B	C	D	E	F	G	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	H	I	J	K	L	M	N	O	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	P	Q	R	S	T	U	V	W	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	X	Y	Z	[	\	]	^	_	*/
	0,	0,	0,	0,	S_ESC,	0,	0,	0,

  /*	`	a	b	c	d	e	f	g	*/
	S_GRAVE,0,	0,	0,	0,	0,	0,	0,

  /*	h	i	j	k	l	m	n	o	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	p	q	r	s	t	u	v	w	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	x	y	z	{	|	}	~	del	*/
	0,	0,	0,	0,	0,	S_RBRA,	0,	0
};

/*
 * ST_NESTED, ST_QNEST
 * for skipping over ?(...), [...]
 */
static const char sh_lexstate5[256] =
{
  /*	nul	soh	stx	etx	eot	enq	ack	bel	*/
	S_EOF,	0,	0,	0,	0,	0,	0,	0,

  /*	bs	ht/tab	nl/lf	vt	ff	cr	so	si	*/
	0,	S_BLNK,	S_NL,	0,	0,	0,	0,	0,

  /*	dle	dc1	dc2	dc3	dc4	nak	syn	etb	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	can	em	sub	esc	fs	gs	rs	us	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	space	!	"	#	$	%	&	'	*/
	S_BLNK,	0,	S_QUOTE,0,	S_DOL,	0,	0,	S_LIT,

  /*	(	)	*	+	,	-	.	/	*/
	S_PUSH,	S_POP,	0,	0,	0,	0,	0,	0,

  /*	0	1	2	3	4	5	6	7	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	8	9	:	;	<	=	>	?	*/
	0,	0,	0,	S_POP,	0,	0,	0,	0,

  /*	@	A	B	C	D	E	F	G	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	H	I	J	K	L	M	N	O	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	P	Q	R	S	T	U	V	W	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	X	Y	Z	[	\	]	^	_	*/
	0,	0,	0,	S_BRACT,S_ESC,	S_POP,	0,	0,

  /*	`	a	b	c	d	e	f	g	*/
	S_GRAVE,0,	0,	0,	0,	0,	0,	0,

  /*	h	i	j	k	l	m	n	o	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	p	q	r	s	t	u	v	w	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	x	y	z	{	|	}	~	del	*/
	0,	0,	0,	S_BRACE,0,	S_POP,	S_TILDE,0
};

/*
 * ST_DOL
 * Defines valid expansion characters
 */
static const char sh_lexstate6[256] =
{
  /*	nul	soh	stx	etx	eot	enq	ack	bel	*/
	S_EOF,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,

  /*	bs	ht/tab	nl/lf	vt	ff	cr	so	si	*/
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,

  /*	dle	dc1	dc2	dc3	dc4	nak	syn	etb	*/
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,

  /*	can	em	sub	esc	fs	gs	rs	us	*/
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,

  /*	space	!	"	#	$	%	&	'	*/
	S_ERR,	S_SPC1,	S_ERR,	S_SPC1,	S_SPC2,	S_ERR,	S_ERR,	S_LIT,

  /*	(	)	*	+	,	-	.	/	*/
	S_PAR,	S_ERR,	S_SPC2,	S_ERR,	S_ERR,	S_SPC2,	S_ALP,	S_ERR,

  /*	0	1	2	3	4	5	6	7	*/
	S_DIG,	S_DIG,	S_DIG,	S_DIG,	S_DIG,	S_DIG,	S_DIG,	S_DIG,

  /*	8	9	:	;	<	=	>	?	*/
	S_DIG,	S_DIG,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_SPC2,

  /*	@	A	B	C	D	E	F	G	*/
	S_SPC1,	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,

  /*	H	I	J	K	L	M	N	O	*/
	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,

  /*	P	Q	R	S	T	U	V	W	*/
	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,

  /*	X	Y	Z	[	\	]	^	_	*/
	S_ALP,	S_ALP,	S_ALP,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ALP,

  /*	`	a	b	c	d	e	f	g	*/
	S_ERR,	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,

  /*	h	i	j	k	l	m	n	o	*/
	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,

  /*	p	q	r	s	t	u	v	w	*/
	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,	S_ALP,

  /*	x	y	z	{	|	}	~	del	*/
	S_ALP,	S_ALP,	S_ALP,	S_LBRA,	S_ERR,	S_RBRA,	S_ERR,	S_ERR,

	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
};

/*
 * ST_BRACE
 * for skipping over ${...} until modifier
 */
static const char sh_lexstate7[256] =
{
  /*	nul	soh	stx	etx	eot	enq	ack	bel	*/
	S_EOF,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,

  /*	bs	ht/tab	nl/lf	vt	ff	cr	so	si	*/
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,

  /*	dle	dc1	dc2	dc3	dc4	nak	syn	etb	*/
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,

  /*	can	em	sub	esc	fs	gs	rs	us	*/
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,

  /*	space	!	"	#	$	%	&	'	*/
	S_ERR,	0,	S_ERR,	S_MOD2,	0,	S_MOD2,	S_ERR,	S_ERR,

  /*	(	)	*	+	,	-	.	/	*/
	S_ERR,	S_ERR,	S_MOD1,	S_MOD1,	S_MOD2,	S_MOD1,	S_DOT,	S_MOD2,

  /*	0	1	2	3	4	5	6	7	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	8	9	:	;	<	=	>	?	*/
	0,	0,	S_MOD1,	S_ERR,	S_ERR,	S_MOD1,	S_ERR,	S_MOD1,

  /*	@	A	B	C	D	E	F	G	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	H	I	J	K	L	M	N	O	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	P	Q	R	S	T	U	V	W	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	X	Y	Z	[	\	]	^	_	*/
	0,	0,	0,	S_BRACT,S_ESC,	S_ERR,	S_MOD2,	0,

  /*	`	a	b	c	d	e	f	g	*/
	S_ERR,	0,	0,	0,	0,	0,	0,	0,

  /*	h	i	j	k	l	m	n	o	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	p	q	r	s	t	u	v	w	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	x	y	z	{	|	}	~	del	*/
	0,	0,	0,	S_ERR,	S_ERR,	S_POP,	S_ERR,	S_ERR,

	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,	S_ERR,
};

/*
 * ST_DOLNAME
 * This state is for $name
 */
static const char sh_lexstate8[256] =
{
  /*	nul	soh	stx	etx	eot	enq	ack	bel	*/
	S_EOF,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,

  /*	bs	ht/tab	nl/lf	vt	ff	cr	so	si	*/
	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,

  /*	dle	dc1	dc2	dc3	dc4	nak	syn	etb	*/
	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,

  /*	can	em	sub	esc	fs	gs	rs	us	*/
	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,

  /*	space	!	"	#	$	%	&	'	*/
	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,

  /*	(	)	*	+	,	-	.	/	*/
	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,

  /*	0	1	2	3	4	5	6	7	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	8	9	:	;	<	=	>	?	*/
	0,	0,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,

  /*	@	A	B	C	D	E	F	G	*/
	S_EDOL,	0,	0,	0,	0,	0,	0,	0,

  /*	H	I	J	K	L	M	N	O	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	P	Q	R	S	T	U	V	W	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	X	Y	Z	[	\	]	^	_	*/
	0,	0,	0,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	0,

  /*	`	a	b	c	d	e	f	g	*/
	S_EDOL,0,	0,	0,	0,	0,	0,	0,

  /*	h	i	j	k	l	m	n	o	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	p	q	r	s	t	u	v	w	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	x	y	z	{	|	}	~	del	*/
	0,	0,	0,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,

	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,
	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,
	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,
	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,
	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,
	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,
	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,
	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,
	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,
	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,
	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,
	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,
	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,
	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,
	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,
	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,	S_EDOL,
};

/*
 * ST_MACRO
 * This is used for macro expansion
 */
static const char sh_lexstate9[256] =
{
  /*	nul	soh	stx	etx	eot	enq	ack	bel	*/
	S_EOF,	0,	0,	0,	0,	0,	0,	0,

  /*	bs	ht/tab	nl/lf	vt	ff	cr	so	si	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	dle	dc1	dc2	dc3	dc4	nak	syn	etb	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	can	em	sub	esc	fs	gs	rs	us	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	space	!	"	#	$	%	&	'	*/
	0,	S_BRAOP,S_QUOTE,0,	S_DOL,	0,	S_PAT,	S_LIT,

  /*	(	)	*	+	,	-	.	/	*/
	S_PAT,	S_PAT,	S_PAT,	0,	S_COM,	S_BRAOP,S_DOT,	S_SLASH,

  /*	0	1	2	3	4	5	6	7	*/
	S_DIG,	S_DIG,	S_DIG,	S_DIG,	S_DIG,	S_DIG,	S_DIG,	S_DIG,

  /*	8	9	:	;	<	=	>	?	*/
	S_DIG,	S_DIG,	S_COLON,0,	0,	S_EQ,	0,	S_PAT,

  /*	@	A	B	C	D	E	F	G	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	H	I	J	K	L	M	N	O	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	P	Q	R	S	T	U	V	W	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	X	Y	Z	[	\	]	^	_	*/
	0,	0,	0,	S_BRACT,S_ESC,	S_ENDCH,S_BRAOP,0,

  /*	`	a	b	c	d	e	f	g	*/
	S_GRAVE,0,	0,	0,	0,	0,	0,	0,

  /*	h	i	j	k	l	m	n	o	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	p	q	r	s	t	u	v	w	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	x	y	z	{	|	}	~	del	*/
	0,	0,	0,	S_BRACE,S_PAT,	S_ENDCH,0,	0
};

/*
 * ST_MOD1
 * for skipping over a string S in ${v-S}, ${v+S}, ${v:-S}, ${v:+S}
 */
static const char sh_lexstate11[256] =
{
  /*	nul	soh	stx	etx	eot	enq	ack	bel	*/
	S_EOF,	0,	0,	0,	0,	0,	0,	0,

  /*	bs	ht/tab	nl/lf	vt	ff	cr	so	si	*/
	0,	0,	S_NL,	0,	0,	0,	0,	0,

  /*	dle	dc1	dc2	dc3	dc4	nak	syn	etb	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	can	em	sub	esc	fs	gs	rs	us	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	space	!	"	#	$	%	&	'	*/
	0,	0,	S_QUOTE,0,	S_DOL,	0,	0,	S_LIT,

  /*	(	)	*	+	,	-	.	/	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	0	1	2	3	4	5	6	7	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	8	9	:	;	<	=	>	?	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	@	A	B	C	D	E	F	G	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	H	I	J	K	L	M	N	O	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	P	Q	R	S	T	U	V	W	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	X	Y	Z	[	\	]	^	_	*/
	0,	0,	0,	0,	S_ESC,	0,	0,	0,

  /*	`	a	b	c	d	e	f	g	*/
	S_GRAVE,0,	0,	0,	0,	0,	0,	0,

  /*	h	i	j	k	l	m	n	o	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	p	q	r	s	t	u	v	w	*/
	0,	0,	0,	0,	0,	0,	0,	0,

  /*	x	y	z	{	|	}	~	del	*/
	0,	0,	0,	0,	0,	S_RBRA,	0,	0
};

/*
 * This must be kept synchronous with all the above and the ST_* definitions in lexstates.h
 */
const char *sh_lexstates[ST_NONE] =
{
	sh_lexstate0, sh_lexstate1, sh_lexstate2, sh_lexstate3,
	sh_lexstate4, sh_lexstate5, sh_lexstate6, sh_lexstate7,
	sh_lexstate8, sh_lexstate9, sh_lexstate5, sh_lexstate11
};


const char e_lexversion[]	= "%d: invalid binary script version";
const char e_lexspace[]		= "line %d: use space or tab to separate operators %c and %c";
const char e_lexslash[]		= "line %d: $ not preceded by \\";

/* syntax error messages */
const char e_syntaxerror[]	= "syntax error: ";
const char e_syntaxerror_at[]	= "syntax error at line %d: ";
const char e_unexpected[]	= "`%s' unexpected";
const char e_unmatched[]	= "`%s' unmatched";
const char e_emptysubscr[]	= "[]: empty subscript";
const char e_badreflist[]	= "invalid reference list";
const char e_heredoccomsub[]	= "`<<%s' here-document not contained within command substitution";
const char e_lexzerobyte[]	= "zero byte";
const char e_endoffile[]	= "end of file";
const char e_newline[]		= "newline";

/* noexec linter warning messages */
const char e_lexwarnvar[]	= "line %d: in '((%s))', using '$' as in '$%.*s' is slower and can introduce rounding errors";
const char e_lexarithwarn[]	= "line %d: %s is slower than ((%.*s%s";
const char e_lexobsolete1[]	= "line %d: `...` obsolete, use $(...)";
const char e_lexobsolete2[]	= "line %d: -a obsolete, use -e";
const char e_lexobsolete3[]	= "line %d: '=' obsolete, use '=='";
const char e_lexobsolete4[]	= "line %d: [[ ... %s ... ]] obsolete, use ((... %s ...))";
const char e_lexobsolete5[]	= "line %d: set %s obsolete";
const char e_lexobsolete6[]	= "line %d: `{' instead of `in' is obsolete";
const char e_lexusebrace[]	= "line %d: use braces to avoid ambiguities with $id[...]";
const char e_lexescape[]	= "line %d: escape %c to avoid ambiguities";
const char e_lexquote[]		= "line %d: quote %c to avoid ambiguities";
const char e_lexnested[]	= "line %d: spaces required for nested subshell";
const char e_lexbadchar[]	= "%c: invalid character in expression - %s";
const char e_lexfuture[]	= "line %d: \\ in front of %c reserved for future use";
const char e_lexlongquote[]	= "line %d: %c quote may be missing";
const char e_lexemptyfor[]	= "line %d: empty for list";
const char e_lextypeset[]	= "line %d: %s invalid typeset option order";
const char e_lexcharclass[]	= "line %d: '^' as first character in character class not portable";

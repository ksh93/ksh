/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1996-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2026 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/

/*
 * prng
 */

#include <fnv.h>

#define prng_description \
	"32 bit PRNG (pseudo random number generator) hash. " \
	"The following suboptions may be appended to \bprng\b:"
#define prng_options	"\
[+-mpy?The 32 bit PRNG multiplier.]:[number:=0x01000193]\
[+-add?The 32 bit PRNG addend.]:[number:=0]\
[+-init?The PRNG initial value.]:?[number:!0xffffffff:=0x811c9dc5]\
"
#define prng_match	"prng"
#define prng_done	long_done
#define prng_print	long_print
#define prng_data	long_data
#define prng_scale	0

typedef uint32_t Prngnum_t;

typedef struct Prng_s
{
	_SUM_PUBLIC_
	_SUM_PRIVATE_
	_INTEGRAL_PRIVATE_
	Prngnum_t		init;
	Prngnum_t		mpy;
	Prngnum_t		add;
} Prng_t;

static Sum_t*
prng_open(const Method_t* method, const char* name)
{
	Prng_t*	sum;
	const char*	s;
	const char*	t;
	const char*	v;
	ptrdiff_t	i;

	if (sum = newof(0, Prng_t, 1, 0))
	{
		sum->method = (Method_t*)method;
		sum->name = name;
	}
	s = name;
	while (*(t = s))
	{
		for (t = s, v = 0; *s && *s != '-'; s++)
			if (*s == '=' && !v)
				v = s;
		i = (v ? v : s) - t;
		if (isdigit(*t) || v && strneq(t, "mpy", (size_t)i) && (t = v + 1))
			sum->mpy = (Prngnum_t)strtoul(t, NULL, 0);
		else if (strneq(t, "add", (size_t)i))
			sum->add = v ? (Prngnum_t)strtoul(v + 1, NULL, 0) : ~sum->add;
		else if (strneq(t, "init", (size_t)i))
			sum->init = v ? (Prngnum_t)strtoul(v + 1, NULL, 0) : ~sum->init;
		if (*s == '-')
			s++;
	}
	if (!sum->mpy)
	{
		sum->mpy = FNV_MULT;
		if (!sum->init)
			sum->init = FNV_INIT;
	}
	return (Sum_t*)sum;
}

static int
prng_init(Sum_t* p)
{
	Prng_t*		sum = (Prng_t*)p;

	sum->sum = sum->init;
	return 0;
}

static int
prng_block(Sum_t* p, const void* s, size_t n)
{
	Prng_t*			sum = (Prng_t*)p;
	Prngnum_t	c = sum->sum;
	unsigned char*	b = (unsigned char*)s;
	unsigned char*	e = b + n;

	while (b < e)
		c = c * sum->mpy + sum->add + *b++;
	sum->sum = c;
	return 0;
}

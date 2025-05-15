/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2013 AT&T Intellectual Property          *
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
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
/*
 * Glenn Fowler
 * AT&T Research
 *
 * convert utf32 to utf8 in s
 * s must have room for at least UTF8_LEN_MAX bytes
 * return value is the number of chars placed in s
 * thanks Tom Duff
 */

#include <ast.h>

typedef struct Utf8_s
{
	uint32_t	range;
	unsigned short	prefix;
	unsigned short	shift;
} Utf8_t;

static const Utf8_t	ops[] =
{
	{ 0x00000080, 0x00,  0 },
	{ 0x00000800, 0xc0,  6 },
	{ 0x00010000, 0xe0, 12 },
	{ 0x00200000, 0xf0, 18 },
	{ 0x04000000, 0xf8, 24 },
	{ 0x80000000, 0xfc, 30 }
};

size_t
utf32toutf8(char* s, uint32_t w)
{
	int	i;
	char*	b;
	char	tmp[UTF8_LEN_MAX];

	if (!s)
		s = tmp;
	for (i = 0; i < elementsof(ops); i++)
	{
		if (w < ops[i].range)
		{
			b = s;
			*s++ = ops[i].prefix | (w >> ops[i].shift);
			switch (ops[i].shift)
			{
			case 30: *s++ = 0x80 | ((w >> 24) & 0x3f);
				 /* FALLTHROUGH */
			case 24: *s++ = 0x80 | ((w >> 18) & 0x3f);
				 /* FALLTHROUGH */
			case 18: *s++ = 0x80 | ((w >> 12) & 0x3f);
				 /* FALLTHROUGH */
			case 12: *s++ = 0x80 | ((w >>  6) & 0x3f);
				 /* FALLTHROUGH */
			case  6: *s++ = 0x80 | (w & 0x3f);
			}
			return s - b;
		}
	}
	return 0;
}

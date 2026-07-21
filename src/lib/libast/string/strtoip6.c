/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2026 Contributors to ksh 93u+m           *
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

#include <ast.h>
#include <ctype.h>
#include <ip6.h>

/*
 * convert string to ipv6 network byte order ip address
 * with optional prefix bits
 * pointer to first unused char placed in *e, even on error
 * return 0:ok <0:error
 */

#define COL		16
#define DOT		17
#define END		18
#define PFX		19

int
strtoip6(const char* s, char** e, unsigned char* addr, unsigned char* bits)
{
	unsigned char*	b = addr;
	unsigned char*	x = b + IP6ADDR;
	unsigned char*	z;
	int		c = 0;
	uint32_t	a;

	static unsigned char	lex[256];

	if (!lex[0])
	{
		size_t i;
		for (i = 0; i < sizeof(lex); ++i)
			lex[i] = END;
		lex['0'] = 0;
		lex['1'] = 1;
		lex['2'] = 2;
		lex['3'] = 3;
		lex['4'] = 4;
		lex['5'] = 5;
		lex['6'] = 6;
		lex['7'] = 7;
		lex['8'] = 8;
		lex['9'] = 9;
		lex['A'] = lex['a'] = 10;
		lex['B'] = lex['b'] = 11;
		lex['C'] = lex['c'] = 12;
		lex['D'] = lex['d'] = 13;
		lex['E'] = lex['e'] = 14;
		lex['F'] = lex['f'] = 15;
		lex[':'] = COL;
		lex['.'] = DOT;
		lex['/'] = PFX;
	}
	while (isspace(*s))
		s++;
	z = 0;
	a = 0;
	if (*s)
		for (;;)
		{
			switch (c = lex[*((unsigned char*)s++)])
			{
			case END:
			case PFX:
				if ((x - b) < 2)
					break;
				*b++ = (unsigned char)(a>>8);
				*b++ = (unsigned char)a;
				break;
			case COL:
				if ((x - b) < 2)
					break;
				*b++ = (unsigned char)(a>>8);
				*b++ = (unsigned char)a;
				a = 0;
				if (*s == ':')
				{
					if (z)
					{
						s--;
						break;
					}
					z = b;
					if ((c = lex[*((unsigned char*)++s)]) >= 16)
					{
						s++;
						break;
					}
				}
				continue;
			case DOT:
				if (b >= x)
				{
					s--;
					break;
				}
				*b++ = ((a >> 8) & 0xf) * 100 + ((a >> 4) & 0xf) * 10 + (a & 0xf);
				a = 0;
				for (;;)
				{
					switch (c = lex[*((unsigned char*)s++)])
					{
					case COL:
					case END:
					case PFX:
						if (b < x)
							*b++ = (unsigned char)a;
						a = 0;
						break;
					case DOT:
						if (b >= x)
							break;
						*b++ = (unsigned char)a;
						a = 0;
						continue;
					default:
						a = (a * 10) + (unsigned)c;
						continue;
					}
					break;
				}
				if (c == COL)
				{
					if (*s == ':')
					{
						if (z)
						{
							s--;
							break;
						}
						z = b;
						if ((c = lex[*((unsigned char*)++s)]) >= 16)
						{
							s++;
							break;
						}
					}
					if ((b - addr) == 6 && addr[0] == 0x20 && addr[1] == 0x02)
						continue;
				}
				break;
			default:
				a = (a << 4) | (unsigned)c;
				continue;
			}
			break;
		}
	if (b == addr)
		c = END + 1;
	else
	{
		if (z)
		{
			while (b > z)
				*--x = *--b;
			while (x > z)
				*--x = 0;
		}
		else
			while (b < x)
				*b++ = 0;
		if (bits)
		{
			if (c == PFX)
			{
				a = 0;
				while ((c = lex[*((unsigned char*)s++)]) < 10)
					a = a * 10 + (unsigned)c;
			}
			else
				a = 0xff;
			*bits = (unsigned char)a;
		}
	}
	if (e)
		*e = (char*)(s - 1);
	return c == END ? 0 : -1;
}

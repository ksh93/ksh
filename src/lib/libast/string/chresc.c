/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2013 AT&T Intellectual Property          *
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
#include <ast_wchar.h>
#include <error.h>
#include <iconv.h>

#if AST_NOMULTIBYTE

#define utf32towc(c)	(c,-1)

#else

/*
 * Convert Unicode code point to current locale's code point
 * (note: does *not* handle multibyte encoding such as UTF-8)
 */

static int
utf32towc(uint32_t utf32)
{
	char		*inbuf, *outbuf;
	size_t		inbytesleft, outbytesleft;
	char		tmp_in[UTF8_LEN_MAX+1], tmp_out[16];
	wchar_t		wchar;
	ptrdiff_t	bytelen;

	/* in ASCII range: no conversion needed (we only support supersets of ASCII) */
	if (utf32 <= 0x7F)
		return (int)utf32;
	/* in ASCII-only locales, only ASCII (0 - 0x7F) is valid */
	if (!mbwide() && (ast.locale.set & AST_LC_7bit))
		return -1;
	/* check for valid Unicode code point */
	if (utf32 > 0x10FFFF || utf32 >= 0xD800 && utf32 <= 0xDFFF || utf32 >= 0xFFFE && utf32 <= 0xFFFF)
		return -1;
	/* in UTF-8 locale: no conversion needed */
	if (ast.locale.set & AST_LC_utf8)
		return (int)utf32;
	/* open an iconv descriptor for converting from UTF-8 to the current locale --
	 * remember it across invocations; setlocale will close/reset it upon changing locale */
	if (ast.locale.uc2wc == (void*)(-1) && (ast.locale.uc2wc = iconv_open(getcodeset(), "UTF-8")) == (void*)(-1))
		ast.locale.uc2wc = 0;
	if (ast.locale.uc2wc == 0)
		return -1;
	inbytesleft = utf32toutf8(tmp_in, utf32);
	tmp_in[inbytesleft] = 0;
	inbuf = tmp_in;
	outbuf = tmp_out;
	outbytesleft = sizeof(tmp_out);
	if (iconv(ast.locale.uc2wc, &inbuf, &inbytesleft, &outbuf, &outbytesleft) == (size_t)-1 || inbytesleft)
		return -1;
	/* macOS iconv(3) in single-byte ISO-8859 locales tries to convert Unicode code points to equivalent
	 * multi-character strings, but we can't use that here, because chrexp() returns a single character.
	 * On Solaris/illumos, iconv fails to fail and returns '?' for unsupported codepoints. Reject both. */
	if (*(unsigned char*)tmp_out <= 0x7F)
		return -1;
	bytelen = outbuf - tmp_out;
	if (!mbwide())
		return bytelen==1 ? *(unsigned char*)tmp_out : -1;
	if (mb2wc(wchar, tmp_out, (size_t)bytelen) <= 0)
		return -1;
	return wchar;
}

#endif /* AST_NOMULTIBYTE */

/*
 * Glenn Fowler
 * AT&T Research
 *
 * return the next character in the string s
 * \ character constants are expanded
 * *p is updated to point to the next character in s
 * *m is 1 if return value is wide
 */

int
chrexp(const char* s, char** p, int* m, int flags)
{
	const char*	q;		/* end of loop through s */
	int		c;		/* current character */
	const char*	e;		/* flag for expanding hex values */
	const char*	b;		/* beginning of s */
	int		n;		/* number of hex digits */
	int		w;		/* set if expanding a wide character (> 2 digits, including leading zeros) */
	char		convert;	/* set if Unicode code point needs to be converted to the current locale */

	w = 0;
	for (;;)
	{
		convert = 0;
		b = s;
		switch (c = mbchar(s))
		{
		case 0:
			s--;
			break;
		case '\\':
			switch (c = *s++)
			{
			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
				if (!(flags & FMT_EXP_CHAR))
					goto noexpand;
				c -= '0';
				q = s + 2;
				while (s < q)
					switch (*s)
					{
					case '0': case '1': case '2': case '3':
					case '4': case '5': case '6': case '7':
						c = (c << 3) + *s++ - '0';
						break;
					default:
						q = s;
						break;
					}
				break;
			case 'a':
				if (!(flags & FMT_EXP_CHAR))
					goto noexpand;
				c = CC_bel;
				break;
			case 'b':
				if (!(flags & FMT_EXP_CHAR))
					goto noexpand;
				c = '\b';
				break;
			case 'c': /*DEPRECATED*/
			case 'C':
				if (!(flags & FMT_EXP_CHAR))
					goto noexpand;
				if (c = *s)
				{
					s++;
					if (c == '\\')
					{
						char*		r;

						c = chrexp(s - 1, &r, 0, flags);
						s = (const char*)r;
					}
					if (islower(c))
						c = toupper(c);
					c ^= 0x40; /* assumes ASCII */
				}
				break;
			case 'e': /*DEPRECATED*/
			case 'E':
				if (!(flags & FMT_EXP_CHAR))
					goto noexpand;
				c = CC_esc;
				break;
			case 'f':
				if (!(flags & FMT_EXP_CHAR))
					goto noexpand;
				c = '\f';
				break;
			case 'n':
				if (flags & FMT_EXP_NONL)
					continue;
				if (!(flags & FMT_EXP_LINE))
					goto noexpand;
				c = '\n';
				break;
			case 'r':
				if (flags & FMT_EXP_NOCR)
					continue;
				if (!(flags & FMT_EXP_LINE))
					goto noexpand;
				c = '\r';
				break;
			case 't':
				if (!(flags & FMT_EXP_CHAR))
					goto noexpand;
				c = '\t';
				break;
			case 'v':
				if (!(flags & FMT_EXP_CHAR))
					goto noexpand;
				c = CC_vt;
				break;
			case 'u':
				q = s + 4;
				goto wex;
			case 'U':
				q = s + 8;
			wex:
				if (!(flags & FMT_EXP_WIDE))
					goto noexpand;
				w = 1;
				convert = 1;
				goto hex;
			case 'x':
				q = s + 2;
			hex:
				b = e = s;
				n = 0;
				c = 0;
				while (!e || !q || s < q)
				{
					switch (*s)
					{
					case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
						c = (c << 4) + *s++ - 'a' + 10;
						n++;
						continue;
					case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
						c = (c << 4) + *s++ - 'A' + 10;
						n++;
						continue;
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9':
						c = (c << 4) + *s++ - '0';
						n++;
						continue;
					case '{':
					case '[':
						if (s != e)
							break;
						e = 0;
						s++;
						if (w && *s == 'U' && *(s + 1) == '+')
							s += 2;
						continue;
					case '}':
					case ']':
						if (!e)
							s++;
						break;
					default:
						break;
					}
					break;
				}
				if (n > 2 && (flags & FMT_EXP_WIDE))
					w = 1;
				if (n <= 2 && !(flags & FMT_EXP_CHAR) ||
					n > 2 && !(flags & FMT_EXP_WIDE) ||
					convert && (c = utf32towc((uint32_t)c)) <= 0)
				{
					s = b;
					goto noexpand;
				}
				break;
			case 0:
				s--;
				break;
			}
			break;
		default:
			if ((s - b) > 1)
				w = 1;
			break;
		}
		break;
	}
 normal:
	if (p)
		*p = (char*)s;
	if (m)
		*m = w;
	return c;
 noexpand:
	c = '\\';
	s--;
	goto normal;
}

int
chresc(const char* s, char** p)
{
	return chrexp(s, p, NULL, FMT_EXP_CHAR|FMT_EXP_LINE|FMT_EXP_WIDE);
}

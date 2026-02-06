/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
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

/*
 * Glenn Fowler
 * AT&T Research
 *
 * iconv intercept
 * minimally provides { UTF*<=>bin ASCII<=>EBCDIC* }
 */

#include <ast.h>
#include <dirent.h>
#include <error.h>

#define DEBUG_TRACE		0
#define _ICONV_LIST_PRIVATE_

#include <ccode.h>
#include <ctype.h>
#include <iconv.h>

#include "lclib.h"

#if !_lib_iconv_open

#define _ast_iconv_t		iconv_t
#define _ast_iconv_f		iconv_f
#define _ast_iconv_list_t	iconv_list_t
#define _ast_iconv_open		iconv_open
#define _ast_iconv		iconv
#define _ast_iconv_close	iconv_close
#define _ast_iconv_list		iconv_list
#define _ast_iconv_move		iconv_move
#define _ast_iconv_name		iconv_name
#define _ast_iconv_write	iconv_write

#endif

#define RETURN(e,n,fn) \
	if (*fn && !e) e = E2BIG; \
	if (e) { errno = e; return (size_t)(-1); } \
	return n;

typedef struct Map_s
{
	char*			name;
	const unsigned char*	map;
	_ast_iconv_f		fun;
	int			index;
} Map_t;

typedef struct Conv_s
{
	iconv_t			cvt;
	char*			buf;
	size_t			size;
	Map_t			from;
	Map_t			to;
} Conv_t;

static Conv_t*			freelist[4];
static int			freeindex;

static const char		name_local[] = "local";
static const char		name_native[] = "native";

static const _ast_iconv_list_t	codes[] =
{
	{
	"utf",
	"un|unicode|utf",
	"multibyte 8-bit unicode",
	"UTF-%s",
	"8",
	CC_UTF,
	},

	{
	"ume",
	"um|ume|utf?(-)7",
	"multibyte 7-bit unicode",
	"UTF-7",
	0,
	CC_UME,
	},

	{
	"euc",
	"(big|euc)*",
	"euc family",
	0,
	0,
	CC_ICONV,
	},

	{
	"dos",
	"dos?(-)?(855)",
	"dos code page",
	"DOS855",
	0,
	CC_ICONV,
	},

	{
	"ucs",
	"ucs?(-)?(2)?(be)|utf-16?(be)",
	"unicode runes",
	"UCS-%s",
	"2",
	CC_UCS,
	},

	{
	"ucs-le",
	"ucs?(-)?(2)le|utf-16le",
	"little endian unicode runes",
	"UCS-%sLE",
	"2",
	CC_SCU,
	},

	{ 0 },
};

/*
 * return canonical character code set name for m
 * if b!=0 then canonical name placed in b of size n
 * <ccode.h> index returned
 */

int
_ast_iconv_name(const char* m, char* b, size_t n)
{
	const _ast_iconv_list_t*	cp;
	const _ast_iconv_list_t*	bp;
	int				c;
	char*				e;
	ssize_t				sub[2];
	char				buf[16];
#if DEBUG_TRACE
	char*				o;
#endif

	if (!b)
	{
		b = buf;
		n = sizeof(buf);
	}
#if DEBUG_TRACE
	o = b;
#endif
	e = b + n - 1;
	bp = 0;
	n = 0;
	cp = ccmaplist(NULL);
#if DEBUG_TRACE
if (error_info.trace < DEBUG_TRACE) sfprintf(sfstderr, "%s: debug-%d: AHA%d _ast_iconv_name m=\"%s\"\n", error_info.id, error_info.trace, __LINE__, m);
#endif
	for (;;)
	{
#if DEBUG_TRACE
if (error_info.trace < DEBUG_TRACE) sfprintf(sfstderr, "%s: debug-%d: AHA%d _ast_iconv_name n=%d bp=%p cp=%p ccode=%d name=\"%s\"\n", error_info.id, error_info.trace, __LINE__, n, bp, cp, cp->ccode, cp->name);
#endif
		if (strgrpmatch(m, cp->match, sub, elementsof(sub) / 2, STR_MAXIMAL|STR_LEFT|STR_ICASE))
		{
			if (!(c = m[sub[1]]))
			{
				bp = cp;
				break;
			}
			if (sub[1] > n && !isalpha(c))
			{
				bp = cp;
				n = sub[1];
			}
		}
		if (cp->ccode < 0)
		{
			if (!(++cp)->name)
				break;
		}
		else if (!(cp = (const _ast_iconv_list_t*)ccmaplist((_ast_iconv_list_t*)cp)))
			cp = codes;
	}
	if (cp = bp)
	{
		if (cp->canon)
		{
			if (cp->index)
			{
				for (m += sub[1]; *m && !isalnum(*m); m++);
				if (!isdigit(*m))
					m = cp->index;
			}
			else
				m = "1";
			b += sfsprintf(b, e - b, cp->canon, m);
		}
		else if (cp->ccode == CC_NATIVE)
		{
			if ((locales[AST_LC_CTYPE]->flags & LC_default) || !locales[AST_LC_CTYPE]->charset || !(m = locales[AST_LC_CTYPE]->charset->code) || streq(m, "iso8859-1"))
				switch (CC_NATIVE)
				{
				case CC_EBCDIC:
					m = (const char*)"EBCDIC";
					break;
				case CC_EBCDIC_I:
					m = (const char*)"EBCDIC-I";
					break;
				case CC_EBCDIC_O:
					m = (const char*)"EBCDIC-O";
					break;
				default:
					m = (const char*)"ISO-8859-1";
					break;
				}
			b += sfsprintf(b, e - b, "%s", m);
		}
		*b = 0;
#if DEBUG_TRACE
if (error_info.trace < DEBUG_TRACE) sfprintf(sfstderr, "%s: debug-%d: AHA%d _ast_iconv_name ccode=%d canon=\"%s\"\n", error_info.id, error_info.trace, __LINE__, cp->ccode, o);
#endif
		return cp->ccode;
	}
	while (b < e && (c = *m++))
	{
		if (islower(c))
			c = toupper(c);
		*b++ = c;
	}
	*b = 0;
#if DEBUG_TRACE
if (error_info.trace < DEBUG_TRACE) sfprintf(sfstderr, "%s: debug-%d: AHA%d _ast_iconv_name ccode=%d canon=\"%s\"\n", error_info.id, error_info.trace, __LINE__, CC_ICONV, o);
#endif
	return CC_ICONV;
}

/*
 * convert UTF-8 to bin
 */

static size_t
utf2bin(_ast_iconv_t cd, char** fb, size_t* fn, char** tb, size_t* tn)
{
	unsigned char*		f;
	unsigned char*		fe;
	unsigned char*		t;
	unsigned char*		te;
	unsigned char*		p;
	int			c;
	int			w;
	size_t			n;
	int			e;

	NOT_USED(cd);
	e = 0;
	f = (unsigned char*)(*fb);
	fe = f + (*fn);
	t = (unsigned char*)(*tb);
	te = t + (*tn);
	while (t < te && f < fe)
	{
		p = f;
		c = *f++;
		if (c & 0x80)
		{
			if (!(c & 0x40))
			{
				f = p;
				e = EILSEQ;
				break;
			}
			if (c & 0x20)
			{
				w = (c & 0x0F) << 12;
				if (f >= fe)
				{
					f = p;
					e = EINVAL;
					break;
				}
				c = *f++;
				if (c & 0x40)
				{
					f = p;
					e = EILSEQ;
					break;
				}
				w |= (c & 0x3F) << 6;
			}
			else
				w = (c & 0x1F) << 6;
			if (f >= fe)
			{
				f = p;
				e = EINVAL;
				break;
			}
			c = *f++;
			w |= (c & 0x3F);
		}
		else
			w = c;
		*t++ = w;
	}
	*fn -= (char*)f - (*fb);
	*fb = (char*)f;
	*tn -= (n = (char*)t - (*tb));
	*tb = (char*)t;
	RETURN(e, n, fn);
}

/*
 * convert bin to UTF-8
 */

static size_t
bin2utf(_ast_iconv_t cd, char** fb, size_t* fn, char** tb, size_t* tn)
{
	unsigned char*		f;
	unsigned char*		fe;
	unsigned char*		t;
	unsigned char*		te;
	int			c;
	wchar_t			w;
	size_t			n;
	int			e;

	NOT_USED(cd);
	e = 0;
	f = (unsigned char*)(*fb);
	fe = f + (*fn);
	t = (unsigned char*)(*tb);
	te = t + (*tn);
	while (f < fe && t < te)
	{
#if AST_NOMULTIBYTE
		c = 1;
		w = *f;
#else
		if (!mbwide())
		{
			c = 1;
			w = *f;
		}
		else if ((c = (*ast.mb.towc)(&w, (char*)f, fe - f)) < 0)
		{
			e = EINVAL;
			break;
		}
		else if (!c)
			c = 1;
#endif /* AST_NOMULTIBYTE */
		if (!(w & ~0x7F))
			*t++ = w;
		else
		{
			if (!(w & ~0x7FF))
			{
				if (t >= (te - 2))
				{
					e = E2BIG;
					break;
				}
				*t++ = 0xC0 + (w >> 6);
			}
			else if (!(w & ~0xffff))
			{
				if (t >= (te - 3))
				{
					e = E2BIG;
					break;
				}
				*t++ = 0xE0 + (w >> 12);
				*t++ = 0x80 + ((w >> 6 ) & 0x3F);
			}
			else
			{
				e = EILSEQ;
				break;
			}
			*t++ = 0x80 + (w & 0x3F);
		}
		f += c;
	}
	*fn -= (n = (char*)f - (*fb));
	*fb = (char*)f;
	*tn -= (char*)t - (*tb);
	*tb = (char*)t;
	RETURN(e, n, fn);
}

static const unsigned char	ume_D[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'(),-./:?!\"#$%&*;<=>@[]^_`{|} \t\n";

static const unsigned char	ume_M[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static unsigned char		ume_d[UCHAR_MAX+1];

static unsigned char		ume_m[UCHAR_MAX+1];

#define NOE			0xFF
#define UMEINIT()		(ume_d[ume_D[0]]?0:umeinit())

/*
 * initialize the ume tables
 */

static int
umeinit(void)
{
	const unsigned char*	s;
	int			i;
	int			c;

	if (!ume_d[ume_D[0]])
	{
		s = ume_D;
		while (c = *s++)
			ume_d[c] = 1;
		memset(ume_m, NOE, sizeof(ume_m));
		for (i = 0; c = ume_M[i]; i++)
			ume_m[c] = i;
	}
	return 0;
}

/*
 * convert UTF-7 to bin
 */

static size_t
ume2bin(_ast_iconv_t cd, char** fb, size_t* fn, char** tb, size_t* tn)
{
	unsigned char*		f;
	unsigned char*		fe;
	unsigned char*		t;
	unsigned char*		te;
	unsigned char*		p;
	int			s;
	int			c;
	int			w;
	size_t			n;
	int			e;

	NOT_USED(cd);
	e = 0;
	UMEINIT();
	f = (unsigned char*)(*fb);
	fe = f + (*fn);
	t = (unsigned char*)(*tb);
	te = t + (*tn);
	s = 0;
	while (f < fe && t < te)
	{
		p = f;
		c = *f++;
		if (s)
		{
			if (c == '-' && s > 1)
				s = 0;
			else if ((w = ume_m[c]) == NOE)
			{
				s = 0;
				*t++ = c;
			}
			else if (f >= (fe - 2))
			{
				f = p;
				e = EINVAL;
				break;
			}
			else
			{
				s = 2;
				w = (w << 6) | ume_m[*f++];
				w = (w << 6) | ume_m[*f++];
				if (!(w & ~0xFF))
					*t++ = w;
				else if (t >= (te - 1))
				{
					f = p;
					e = E2BIG;
					break;
				}
				else
				{
					*t++ = (w >> 8) & 0xFF;
					*t++ = w & 0xFF;
				}
			}
		}
		else if (c == '+')
			s = 1;
		else
			*t++ = c;
	}
	*fn -= (char*)f - (*fb);
	*fb = (char*)f;
	*tn -= (n = (char*)t - (*tb));
	*tb = (char*)t;
	RETURN(e, n, fn);
}

/*
 * convert bin to UTF-7
 */

static size_t
bin2ume(_ast_iconv_t cd, char** fb, size_t* fn, char** tb, size_t* tn)
{
	unsigned char*		f;
	unsigned char*		fe;
	unsigned char*		t;
	unsigned char*		te;
	int			c;
	int			s;
	wchar_t			w;
	size_t			n;
	int			e;

	NOT_USED(cd);
	e = 0;
	UMEINIT();
	f = (unsigned char*)(*fb);
	fe = f + (*fn);
	t = (unsigned char*)(*tb);
	te = t + (*tn);
	s = 0;
	while (f < fe && t < (te - s))
	{
#if AST_NOMULTIBYTE
		c = 1;
		w = *f;
#else
		if (!mbwide())
		{
			c = 1;
			w = *f;
		}
		else if ((c = (*ast.mb.towc)(&w, (char*)f, fe - f)) < 0)
		{
			e = EINVAL;
			break;
		}
		else if (!c)
			c = 1;
#endif /* AST_NOMULTIBYTE */
		if (!(w & ~0x7F) && ume_d[w])
		{
			if (s)
			{
				s = 0;
				*t++ = '-';
			}
			*t++ = w;
		}
		else if (t >= (te - (4 + s)))
		{
			e = E2BIG;
			break;
		}
		else
		{
			if (!s)
			{
				s = 1;
				*t++ = '+';
			}
			*t++ = ume_M[(w >> 12) & 0x3F];
			*t++ = ume_M[(w >> 6) & 0x3F];
			*t++ = ume_M[w & 0x3F];
		}
		f += c;
	}
	if (s)
		*t++ = '-';
	*fn -= (n = (char*)f - (*fb));
	*fb = (char*)f;
	*tn -= (char*)t - (*tb);
	*tb = (char*)t;
	RETURN(e, n, fn);
}

/*
 * convert UCS-2 to bin with no byte swap
 */

static size_t
ucs2bin(_ast_iconv_t cd, char** fb, size_t* fn, char** tb, size_t* tn)
{
	unsigned char*		f;
	unsigned char*		fe;
	unsigned char*		t;
	unsigned char*		te;
	int			w;
	size_t			n;
	int			e;

	NOT_USED(cd);
	e = 0;
	f = (unsigned char*)(*fb);
	fe = f + (*fn);
	t = (unsigned char*)(*tb);
	te = t + (*tn);
	while (f < (fe - 1) && t < te)
	{
		w = *f++;
		w = (w << 8) | *f++;
		if (!(w & ~0xFF))
			*t++ = w;
		else if (t >= (te - 1))
		{
			f -= 2;
			e = E2BIG;
			break;
		}
		else
		{
			*t++ = (w >> 8) & 0xFF;
			*t++ = w & 0xFF;
		}
	}
	*fn -= (char*)f - (*fb);
	*fb = (char*)f;
	*tn -= (n = (char*)t - (*tb));
	*tb = (char*)t;
	RETURN(e, n, fn);
}

/*
 * convert bin to UCS-2 with no byte swap
 */

static size_t
bin2ucs(_ast_iconv_t cd, char** fb, size_t* fn, char** tb, size_t* tn)
{
	unsigned char*		f;
	unsigned char*		fe;
	unsigned char*		t;
	unsigned char*		te;
	int			c;
	wchar_t			w;
	size_t			n;
	int			e;

	NOT_USED(cd);
	e = 0;
	f = (unsigned char*)(*fb);
	fe = f + (*fn);
	t = (unsigned char*)(*tb);
	te = t + (*tn);
	while (f < fe && t < (te - 1))
	{
#if AST_NOMULTIBYTE
		c = 1;
		w = *f;
#else
		if (!mbwide())
		{
			c = 1;
			w = *f;
		}
		if ((c = (*ast.mb.towc)(&w, (char*)f, fe - f)) < 0)
		{
			e = EINVAL;
			break;
		}
		else if (!c)
			c = 1;
#endif /* AST_NOMULTIBYTE */
		*t++ = (w >> 8) & 0xFF;
		*t++ = w & 0xFF;
		f += c;
	}
	*fn -= (n = (char*)f - (*fb));
	*fb = (char*)f;
	*tn -= (char*)t - (*tb);
	*tb = (char*)t;
	RETURN(e, n, fn);
}

/*
 * convert UCS-2 to bin with byte swap
 */

static size_t
scu2bin(_ast_iconv_t cd, char** fb, size_t* fn, char** tb, size_t* tn)
{
	unsigned char*		f;
	unsigned char*		fe;
	unsigned char*		t;
	unsigned char*		te;
	int			w;
	size_t			n;
	int			e;

	NOT_USED(cd);
	e = 0;
	f = (unsigned char*)(*fb);
	fe = f + (*fn);
	t = (unsigned char*)(*tb);
	te = t + (*tn);
	while (f < (fe - 1) && t < te)
	{
		w = *f++;
		w = w | (*f++ << 8);
		if (!(w & ~0xFF))
			*t++ = w;
		else if (t >= (te - 1))
		{
			f -= 2;
			e = E2BIG;
			break;
		}
		else
		{
			*t++ = (w >> 8) & 0xFF;
			*t++ = w & 0xFF;
		}
	}
	*fn -= (char*)f - (*fb);
	*fb = (char*)f;
	*tn -= (n = (char*)t - (*tb));
	*tb = (char*)t;
	RETURN(e, n, fn);
}

/*
 * convert bin to UCS-2 with byte swap
 */

static size_t
bin2scu(_ast_iconv_t cd, char** fb, size_t* fn, char** tb, size_t* tn)
{
	unsigned char*		f;
	unsigned char*		fe;
	unsigned char*		t;
	unsigned char*		te;
	int			c;
	wchar_t			w;
	size_t			n;
	int			e;

	NOT_USED(cd);
	e = 0;
	f = (unsigned char*)(*fb);
	fe = f + (*fn);
	t = (unsigned char*)(*tb);
	te = t + (*tn);
	while (f < fe && t < (te - 1))
	{
#if AST_NOMULTIBYTE
		c = 1;
		w = *f;
#else
		if (!mbwide())
		{
			c = 1;
			w = *f;
		}
		else if ((c = (*ast.mb.towc)(&w, (char*)f, fe - f)) < 0)
		{
			e = EINVAL;
			break;
		}
		else if (!c)
			c = 1;
#endif /* AST_NOMULTIBYTE */
		*t++ = w & 0xFF;
		*t++ = (w >> 8) & 0xFF;
		f += c;
	}
	*fn -= (n = (char*)f - (*fb));
	*fb = (char*)f;
	*tn -= (char*)t - (*tb);
	*tb = (char*)t;
	RETURN(e, n, fn);
}

/*
 * open a character code conversion map from f to t
 */

_ast_iconv_t
_ast_iconv_open(const char* t, const char* f)
{
	Conv_t*	cc;
	int	fc;
	int	tc;
	int	i;

	char	fr[64];
	char	to[64];

#if DEBUG_TRACE
error(DEBUG_TRACE, "AHA#%d _ast_iconv_open f=%s t=%s\n", __LINE__, f, t);
#endif
	if (!t || !*t || *t == '-' && !*(t + 1) || !strcasecmp(t, name_local) || !strcasecmp(t, name_native))
		t = name_native;
	if (!f || !*f || *f == '-' && !*(f + 1) || !strcasecmp(t, name_local) || !strcasecmp(f, name_native))
		f = name_native;

	/*
	 * the AST identify is always (iconv_t)(0)
	 */

	if (t == f)
		return (iconv_t)(0);
	fc = _ast_iconv_name(f, fr, sizeof(fr));
	tc = _ast_iconv_name(t, to, sizeof(to));
#if DEBUG_TRACE
error(DEBUG_TRACE, "AHA#%d _ast_iconv_open f=%s:%s:%d t=%s:%s:%d\n", __LINE__, f, fr, fc, t, to, tc);
#endif
	if (fc != CC_ICONV && fc == tc || streq(fr, to))
		return (iconv_t)(0);

	/*
	 * first check the free list
	 */

	for (i = 0; i < elementsof(freelist); i++)
		if ((cc = freelist[i]) && streq(to, cc->to.name) && streq(fr, cc->from.name))
		{
			freelist[i] = 0;
#if _lib_iconv_open
			/*
			 * reset the shift state if any
			 */

			if (cc->cvt != (iconv_t)(-1))
				iconv(cc->cvt, NULL, NULL, NULL, NULL);
#endif
			return cc;
		}

	/*
	 * allocate a new one
	 */

	if (!(cc = newof(0, Conv_t, 1, strlen(to) + strlen(fr) + 2)))
		return (iconv_t)(-1);
	cc->to.name = (char*)(cc + 1);
	cc->from.name = strcopy(cc->to.name, to) + 1;
	strcpy(cc->from.name, fr);
	cc->cvt = (iconv_t)(-1);

	/*
	 * 8-bit maps are the easiest
	 */

	if (fc >= 0 && tc >= 0)
		cc->from.map = ccmap(fc, tc);
#if _lib_iconv_open
	else if ((cc->cvt = iconv_open(t, f)) != (iconv_t)(-1) || (cc->cvt = iconv_open(to, fr)) != (iconv_t)(-1))
		cc->from.fun = (_ast_iconv_f)iconv;
#endif
	else
	{
		switch (fc)
		{
		case CC_UTF:
			cc->from.fun = utf2bin;
			break;
		case CC_UME:
			cc->from.fun = ume2bin;
			break;
		case CC_UCS:
			cc->from.fun = ucs2bin;
			break;
		case CC_SCU:
			cc->from.fun = scu2bin;
			break;
		case CC_ASCII:
			break;
		default:
			if (fc < 0)
				goto nope;
			cc->from.map = ccmap(fc, CC_ASCII);
			break;
		}
		switch (tc)
		{
		case CC_UTF:
			cc->to.fun = bin2utf;
			break;
		case CC_UME:
			cc->to.fun = bin2ume;
			break;
		case CC_UCS:
			cc->to.fun = bin2ucs;
			break;
		case CC_SCU:
			cc->to.fun = bin2scu;
			break;
		case CC_ASCII:
			break;
		default:
			if (tc < 0)
				goto nope;
			cc->to.map = ccmap(CC_ASCII, tc);
			break;
		}
	}
	return (iconv_t)cc;
 nope:
	return (iconv_t)(-1);
}

/*
 * close a character code conversion map
 */

int
_ast_iconv_close(_ast_iconv_t cd)
{
	Conv_t*	cc;
	Conv_t*	oc;
	int	i;
	int	r = 0;

	if (cd == (_ast_iconv_t)(-1))
		return -1;
	if (!(cc = (Conv_t*)cd))
		return 0;

	/*
	 * add to the free list
	 */

	i = freeindex;
	for (;;)
	{
		if (++ i >= elementsof(freelist))
			i = 0;
		if (!freelist[i])
			break;
		if (i == freeindex)
		{
			if (++ i >= elementsof(freelist))
				i = 0;

			/*
			 * close the oldest
			 */

			if (oc = freelist[i])
			{
#if _lib_iconv_open
				if (oc->cvt != (iconv_t)(-1))
					r = iconv_close(oc->cvt);
#endif
				if (oc->buf)
					free(oc->buf);
				free(oc);
			}
			break;
		}
	}
	freelist[freeindex = i] = cc;
	return r;
}

/*
 * copy *fb size *fn to *tb size *tn
 * fb,fn tb,tn updated on return
 */

size_t
_ast_iconv(_ast_iconv_t cd, char** fb, size_t* fn, char** tb, size_t* tn)
{
	Conv_t*				cc = (Conv_t*)cd;
	unsigned char*		f;
	unsigned char*		t;
	unsigned char*		e;
	const unsigned char*	m;
	size_t			n;
	char*				b;
	char*				tfb;
	size_t				tfn;
	size_t				i;

	if (!fb || !*fb)
	{
		/* TODO: reset to the initial state */
		if (!tb || !*tb)
			return 0;
		/* TODO: write the initial state shift sequence */
		return 0;
	}
	n = *tn;
	if (cc)
	{
		if (cc->from.fun)
		{
			if (cc->to.fun)
			{
				if (!cc->buf && !(cc->buf = oldof(0, char, cc->size = SFIO_BUFSIZE, 0)))
				{
					errno = ENOMEM;
					return -1;
				}
				b = cc->buf;
				i = cc->size;
				tfb = *fb;
				tfn = *fn;
				if ((*cc->from.fun)(cc->cvt, &tfb, &tfn, &b, &i) == (size_t)(-1))
					return -1;
				tfn = b - cc->buf;
				tfb = cc->buf;
				n = (*cc->to.fun)(cc->cvt, &tfb, &tfn, tb, tn);
				i = tfb - cc->buf;
				*fb += i;
				*fn -= i;
				return n;
			}
			if ((*cc->from.fun)(cc->cvt, fb, fn, tb, tn) == (size_t)(-1))
				return -1;
			n -= *tn;
			if (m = cc->to.map)
			{
				e = (unsigned char*)(*tb);
				for (t = e - n; t < e; t++)
					*t = m[*t];
			}
			return n;
		}
		else if (cc->to.fun)
		{
			if (!(m = cc->from.map))
				return (*cc->to.fun)(cc->cvt, fb, fn, tb, tn);
			if (!cc->buf && !(cc->buf = oldof(0, char, cc->size = SFIO_BUFSIZE, 0)))
			{
				errno = ENOMEM;
				return -1;
			}
			if ((n = *fn) > cc->size)
				n = cc->size;
			f = (unsigned char*)(*fb);
			e = f + n;
			t = (unsigned char*)(b = cc->buf);
			while (f < e)
				*t++ = m[*f++];
			n = (*cc->to.fun)(cc->cvt, &b, fn, tb, tn);
			*fb += b - cc->buf;
			return n;
		}
	}
	if (n > *fn)
		n = *fn;
	if (cc && (m = cc->from.map))
	{
		f = (unsigned char*)(*fb);
		e = f + n;
		t = (unsigned char*)(*tb);
		while (f < e)
			*t++ = m[*f++];
	}
	else
		memcpy(*tb, *fb, n);
	*fb += n;
	*fn -= n;
	*tb += n;
	*tn -= n;
	return n;
}

#define OK		((size_t)-1)

/*
 * write *fb size *fn to op
 * fb,fn updated on return
 * total bytes written to op returned
 */

ssize_t
_ast_iconv_write(_ast_iconv_t cd, Sfio_t* op, char** fb, size_t* fn, Iconv_disc_t* disc)
{
	char*		fo = *fb;
	char*		tb;
	char*		ts;
	size_t*		e;
	size_t		tn;
	size_t		r;
	int		ok;
	Iconv_disc_t	compat;

	/*
	 * the old API had optional size_t* instead of Iconv_disc_t*
	 */

	if (!disc || disc->version < 20110101L || disc->version >= 30000101L)
	{
		e = (size_t*)disc;
		disc = &compat;
		iconv_init(disc, 0);
	}
	else
		e = 0;
	r = 0;
	tn = 0;
	ok = 1;
	while (ok && *fn > 0)
	{
		if (!(tb = (char*)sfreserve(op, -(tn + 1), SFIO_WRITE|SFIO_LOCKR)) || !(tn = sfvalue(op)))
		{
			if (!r)
				r = -1;
			break;
		}
		ts = tb;
#if DEBUG_TRACE
error(DEBUG_TRACE, "AHA#%d iconv_write ts=%p tn=%d", __LINE__, ts, tn);
		for (;;)
#else
		while (*fn > 0 && _ast_iconv(cd, fb, fn, &ts, &tn) == (size_t)(-1))
#endif
		{
#if DEBUG_TRACE
			ssize_t	_r;
error(DEBUG_TRACE, "AHA#%d iconv_write %d => %d `%-.*s'", __LINE__, *fn, tn, *fn, *fb);
			_r = _ast_iconv(cd, fb, fn, &ts, &tn);
error(DEBUG_TRACE, "AHA#%d iconv_write %d => %d [%d]", __LINE__, *fn, tn, _r);
			if (_r != (size_t)(-1) || !fn)
				break;
#endif
			switch (errno)
			{
			case E2BIG:
				break;
			case EINVAL:
				if (disc->errorf)
					(*disc->errorf)(NULL, disc, ERROR_SYSTEM|2, "incomplete multibyte sequence at offset %I*u", sizeof(fo), *fb - fo);
				goto bad;
			default:
				if (disc->errorf)
					(*disc->errorf)(NULL, disc, ERROR_SYSTEM|2, "invalid multibyte sequence at offset %I*u", sizeof(fo), *fb - fo);
			bad:
				disc->errors++;
				if (!(disc->flags & ICONV_FATAL))
				{
					if (!(disc->flags & ICONV_OMIT) && tn > 0)
					{
						*ts++ = (disc->fill >= 0) ? disc->fill : **fb;
						tn--;
					}
					(*fb)++;
					(*fn)--;
					continue;
				}
				ok = 0;
				break;
			}
			break;
		}
#if DEBUG_TRACE
error(DEBUG_TRACE, "AHA#%d iconv_write %d", __LINE__, ts - tb);
#endif
		sfwrite(op, tb, ts - tb);
		r += ts - tb;
	}
	if (e)
		*e = disc->errors;
	return r;
}

/*
 * move n bytes from ip to op
 */

ssize_t
_ast_iconv_move(_ast_iconv_t cd, Sfio_t* ip, Sfio_t* op, size_t n, Iconv_disc_t* disc)
{
	char*		fb;
	char*		fs;
	char*		tb;
	char*		ts;
	size_t*		e;
	size_t		fe;
	size_t		fn;
	size_t		fo;
	size_t		ft;
	size_t		tn;
	size_t		i;
	ssize_t		r = 0;
	int		ok = 1;
	int		locked;
	Iconv_disc_t	compat;

	/*
	 * the old API had optional size_t* instead of Iconv_disc_t*
	 */

	if (!disc || disc->version < 20110101L || disc->version >= 30000101L)
	{
		e = (size_t*)disc;
		disc = &compat;
		iconv_init(disc, 0);
	}
	else
		e = 0;
	tb = 0;
	fe = OK;
	ft = 0;
	fn = n;
	do
	{
		if (n != SFIO_UNBOUND)
			n = -((ssize_t)(n & (((size_t)(~0))>>1)));
		if ((!(fb = (char*)sfreserve(ip, n, locked = SFIO_LOCKR)) || !(fo = sfvalue(ip))) &&
		    (!(fb = (char*)sfreserve(ip, n, locked = 0)) || !(fo = sfvalue(ip))))
			break;
		fs = fb;
		fn = fo;
		if (!(tb = (char*)sfreserve(op, SFIO_UNBOUND, SFIO_WRITE|SFIO_LOCKR)))
		{
			if (!r)
				r = -1;
			break;
		}
		ts = tb;
		tn = sfvalue(op);
		while (fn > 0 && _ast_iconv(cd, &fs, &fn, &ts, &tn) == (size_t)(-1))
		{
			switch (errno)
			{
			case E2BIG:
				break;
			case EINVAL:
				if (fe == ft + (fo - fn))
				{
					fe = OK;
					if (disc->errorf)
						(*disc->errorf)(NULL, disc, ERROR_SYSTEM|2, "incomplete multibyte sequence at offset %I*u", sizeof(ft), ft + (fo - fn));
					goto bad;
				}
				fe = ft;
				break;
			default:
				if (disc->errorf)
					(*disc->errorf)(NULL, disc, ERROR_SYSTEM|2, "invalid multibyte sequence at offset %I*u", sizeof(ft), ft + (fo - fn));
			bad:
				disc->errors++;
				if (!(disc->flags & ICONV_FATAL))
				{
					if (!(disc->flags & ICONV_OMIT) && tn > 0)
					{
						*ts++ = (disc->fill >= 0) ? disc->fill : *fs;
						tn--;
					}
					fs++;
					fn--;
					continue;
				}
				ok = 0;
				break;
			}
			break;
		}
		sfwrite(op, tb, ts - tb);
		r += ts - tb;
		ts = tb;
		if (locked)
			sfread(ip, fb, fs - fb);
		else
			for (i = fn; --i >= (fs - fb);)
				sfungetc(ip, fb[i]);
		if (n != SFIO_UNBOUND)
		{
			if (n <= (fs - fb))
				break;
			n -= fs - fb;
		}
		ft += (fs - fb);
		if (fn == fo)
			fn++;
	} while (ok);
	if (fb && locked)
		sfread(ip, fb, 0);
	if (tb)
	{
		sfwrite(op, tb, 0);
		if (ts > tb)
		{
			sfwrite(op, tb, ts - tb);
			r += ts - tb;
		}
	}
	if (e)
		*e = disc->errors;
	return r;
}

/*
 * iconv_list_t iterator
 * call with arg 0 to start
 * prev return value is current arg
 */

_ast_iconv_list_t*
_ast_iconv_list(_ast_iconv_list_t* cp)
{
	if (!cp)
		return ccmaplist(NULL);
	if (cp->ccode >= 0)
		return (cp = ccmaplist(cp)) ? cp : (_ast_iconv_list_t*)codes;
	return (++cp)->name ? cp : NULL;
}

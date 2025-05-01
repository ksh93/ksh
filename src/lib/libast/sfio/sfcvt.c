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
*         hyenias <58673227+hyenias@users.noreply.github.com>          *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
#include	"sfhdr.h"

/*	Convert a floating point value to ASCII.
**
**	Written by Kiem-Phong Vo and Glenn Fowler (SFFMT_AFORMAT)
*/

static char		*lc_inf = "inf", *uc_inf = "INF";
static char		*lc_nan = "nan", *uc_nan = "NAN";
static char		*Zero = "0";
#define SFIO_INF	((_Sfi = 3), strlcpy(buf, (format & SFFMT_UPPER) ? uc_inf : lc_inf, size), buf)
#define SFIO_NAN	((_Sfi = 3), strlcpy(buf, (format & SFFMT_UPPER) ? uc_nan : lc_nan, size), buf)
#define SFIO_ZERO	((_Sfi = 1), strlcpy(buf, Zero, size), buf)
#define SFIO_INTPART	(SFIO_IDIGITS/2)

#if defined(__ia64__) && defined(signbit)
# if defined __GNUC__ && __GNUC__ >= 4
#  define __signbitl(f)			__builtin_signbitl(f)
# else
#  if _lib_copysignl
#    define __signbitl(f)	(int)(copysignl(1.0,(f))<0.0)
#  endif
# endif
#endif

#if !_lib_signbit
#if !_ast_fltmax_double
static int neg0ld(Sfdouble_t f)
{
	Sfdouble_t	z = 0;
	z = -z;
	return !memcmp(&f, &z, sizeof(f));
}
#endif
static int neg0d(double f)
{
	double		z = 0;
	z = -z;
	return !memcmp(&f, &z, sizeof(f));
}
#endif

#if UINT_DIG && UINT_DIG < (DBL_DIG-1) && !(ULONG_DIG && ULONG_DIG < (DBL_DIG-1))
#define CVT_LDBL_INT	int
#define CVT_LDBL_MAXINT	INT_MAX
#else
#define CVT_LDBL_INT	long
#define CVT_LDBL_MAXINT	LONG_MAX
#endif

#if UINT_DIG && UINT_DIG < (DBL_DIG-1) && !(ULONG_DIG && ULONG_DIG < (DBL_DIG-1))
#define CVT_DBL_INT	int
#define CVT_DBL_MAXINT	INT_MAX
#else
#define CVT_DBL_INT	long
#define CVT_DBL_MAXINT	LONG_MAX
#endif

char* _sfcvt(void*	vp,		/* pointer to value to convert	*/
	     char*	buf,		/* conversion goes here		*/
	     size_t	size,		/* size of buf			*/
	     int	n_digit,	/* number of digits wanted	*/
	     int*	decpt,		/* to return decimal point	*/
	     int*	sign,		/* to return sign		*/
	     int*	len,		/* return string length		*/
	     int	format)		/* conversion format		*/
{
	char			*sp;
	long			n, v;
	char			*ep, *b, *endsp, *t;
	int			x;
	_ast_flt_unsigned_max_t	m;

	static char		lx[] = "0123456789abcdef";
	static char		ux[] = "0123456789ABCDEF";

	*sign = *decpt = 0;

#if !_ast_fltmax_double
	if(format&SFFMT_LDOUBLE)
	{	Sfdouble_t	f = *(Sfdouble_t*)vp;

		if(isnanl(f))
		{
#if _lib_signbit
			if (signbit(f))
#else
			if (f < 0)
#endif
				*sign = 1;
			return SFIO_NAN;
		}
		if (n = isinf(f))
		{
#if _lib_signbit
			if (signbit(f))
#else
			if (n < 0 || f < 0)
#endif
				*sign = 1;
			return SFIO_INF;
		}

#if _lib_signbit
		if (signbit(f))
#else
		if (f < 0.0 || f == 0.0 && neg0ld(f))
#endif
		{	f = -f;
			*sign = 1;
		}

		if(f < LDBL_MIN)
			return SFIO_ZERO;
		if(f > LDBL_MAX)
			return SFIO_INF;

		if(format & SFFMT_AFORMAT)
		{	Sfdouble_t	g;
			b = sp = buf;
			ep = (format & SFFMT_UPPER) ? ux : lx;
			if(n_digit <= 0 || n_digit >= (size - 9))
				n_digit = size - 9;
			endsp = sp + n_digit + 1;

			g = frexpl(f, &x);
			*decpt = x;
			f = ldexpl(g, 8 * sizeof(m) - 3);

			for (;;)
			{	m = f;
				x = 8 * sizeof(m);
				while ((x -= 4) >= 0)
				{	*sp++ = ep[(m >> x) & 0xf];
					if (sp >= endsp)
						goto around;
				}
				f -= m;
				f = ldexpl(f, 8 * sizeof(m));
			}
		}

		n = 0;
		if(f >= (Sfdouble_t)CVT_LDBL_MAXINT)
		{	/* scale to a small enough number to fit an int */
			v = SFIO_MAXEXP10-1;
			do
			{	if(f < _Sfpos10[v])
					v -= 1;
				else
				{
					f *= _Sfneg10[v];
					if((n += (1<<v)) >= SFIO_IDIGITS)
						return SFIO_INF;
				}
			} while(f >= (Sfdouble_t)CVT_LDBL_MAXINT);
		}
		else if(f > 0.0 && f < 0.1)
		{	/* scale to avoid excessive multiply by 10 below */
			v = SFIO_MAXEXP10-1;
			do
			{	if(f <= _Sfneg10[v])
				{	f *= _Sfpos10[v];
					if((n += (1<<v)) >= SFIO_IDIGITS)
						return SFIO_INF;
				}
				else if (--v < 0)
					break;
			} while(f < 0.1);
			n = -n;
		}
		*decpt = (int)n;

		b = sp = buf + SFIO_INTPART;
		if((v = (CVT_LDBL_INT)f) != 0)
		{	/* translate the integer part */
			f -= (Sfdouble_t)v;

			sfucvt(v,sp,n,ep,CVT_LDBL_INT,unsigned CVT_LDBL_INT);

			n = b-sp;
			if((*decpt += (int)n) >= SFIO_IDIGITS)
				return SFIO_INF;
			b = sp;
			sp = buf + SFIO_INTPART;
		}
		else	n = 0;

		/* remaining number of digits to compute; add 1 for later rounding */
		n = (((format&SFFMT_EFORMAT) || *decpt <= 0) ? 1 : *decpt+1) - n;
		if(n_digit > 0)
		{	if(n_digit > LDBL_DIG)
				n_digit = LDBL_DIG;
			n += n_digit;
		}

		if((ep = (sp+n)) > (endsp = buf+(size-2)))
			ep = endsp;
		if(sp > ep)
			sp = ep;
		else
		{
			if((format&SFFMT_EFORMAT) && *decpt == 0 && f > 0.)
			{	Sfdouble_t	d;
				while((long)(d = f*10.) == 0)
				{	f = d;
					*decpt -= 1;
				}
			}

			while(sp < ep)
			{	/* generate fractional digits */
				if(f <= 0. && *decpt >= 0)
				{	/* fill with 0's */
					do { *sp++ = '0'; } while(sp < ep);
					goto done;
				}
				else if((n = (long)(f *= 10.)) < 10)
				{	*sp++ = '0' + n;
					f -= n;
				}
				else /* n == 10 */
				{	do { *sp++ = '9'; } while(sp < ep);
				}
			}
		}
	} else
#endif
	{	double	f = *(double*)vp;

		if(isnan(f))
		{
#if _lib_signbit
			if (signbit(f))
#else
			if (f < 0)
#endif
				*sign = 1;
			return SFIO_NAN;
		}
		if (n = isinf(f))
		{
#if _lib_signbit
			if (signbit(f))
#else
			if (n < 0 || f < 0)
#endif
				*sign = 1;
			return SFIO_INF;
		}

#if _lib_signbit
		if (signbit(f))
#else
		if (f < 0.0 || f == 0.0 && neg0d(f))
#endif
		{	f = -f;
			*sign = 1;
		}

		if(f < DBL_MIN)
			return SFIO_ZERO;
		if(f > DBL_MAX)
			return SFIO_INF;

		if(format & SFFMT_AFORMAT)
		{	double		g;
			b = sp = buf;
			ep = (format & SFFMT_UPPER) ? ux : lx;
			if(n_digit <= 0 || n_digit >= (size - 9))
				n_digit = size - 9;
			endsp = sp + n_digit + 1;

			g = frexp(f, &x);
			*decpt = x;
			f = ldexp(g, 8 * sizeof(m) - 3);

			for (;;)
			{	m = f;
				x = 8 * sizeof(m);
				while ((x -= 4) >= 0)
				{	*sp++ = ep[(m >> x) & 0xf];
					if (sp >= endsp)
						goto around;
				}
				f -= m;
				f = ldexp(f, 8 * sizeof(m));
			}
		}
		n = 0;
		if(f >= (double)CVT_DBL_MAXINT)
		{	/* scale to a small enough number to fit an int */
			v = SFIO_MAXEXP10-1;
			do
			{	if(f < _Sfpos10[v])
					v -= 1;
				else
				{	f *= _Sfneg10[v];
					if((n += (1<<v)) >= SFIO_IDIGITS)
						return SFIO_INF;
				}
			} while(f >= (double)CVT_DBL_MAXINT);
		}
		else if(f > 0.0 && f < 0.1)
		{	/* scale to avoid excessive multiply by 10 below */
			v = SFIO_MAXEXP10-1;
			do
			{	if(f <= _Sfneg10[v])
				{	f *= _Sfpos10[v];
					if((n += (1<<v)) >= SFIO_IDIGITS)
						return SFIO_INF;
				}
				else if(--v < 0)
					break;
			} while(f < 0.1);
			n = -n;
		}
		*decpt = (int)n;

		b = sp = buf + SFIO_INTPART;
		if((v = (CVT_DBL_INT)f) != 0)
		{	/* translate the integer part */
			f -= (double)v;

			sfucvt(v,sp,n,ep,CVT_DBL_INT,unsigned CVT_DBL_INT);

			n = b-sp;
			if((*decpt += (int)n) >= SFIO_IDIGITS)
				return SFIO_INF;
			b = sp;
			sp = buf + SFIO_INTPART;
		}
		else	n = 0;

		/* remaining number of digits to compute; add 1 for later rounding */
		n = (((format&SFFMT_EFORMAT) || *decpt <= 0) ? 1 : *decpt+1) - n;
		if(n_digit > 0)
		{	if(n_digit > DBL_DIG)
				n_digit = DBL_DIG;
			n += n_digit;
		}

		if((ep = (sp+n)) > (endsp = buf+(size-2)))
			ep = endsp;
		if(sp > ep)
			sp = ep;
		else
		{
			if((format&SFFMT_EFORMAT) && *decpt == 0 && f > 0.)
			{	double	d;
				while((long)(d = f*10.) == 0)
				{	f = d;
					*decpt -= 1;
				}
			}

			while(sp < ep)
			{	/* generate fractional digits */
				if(f <= 0. && *decpt >= 0)
				{	/* fill with 0's */
					do { *sp++ = '0'; } while(sp < ep);
					goto done;
				}
				else if((n = (long)(f *= 10.)) < 10)
				{	*sp++ = (char)('0' + n);
					f -= n;
				}
				else /* n == 10 */
				{	do { *sp++ = '9'; } while(sp < ep);
					break;
				}
			}
		}
	}

	if(ep <= b)
		ep = b+1;
	else if(ep < endsp)
	{	/* round the last digit */
		if (!(format&SFFMT_EFORMAT) && *decpt < 0)
			sp += *decpt-1;
		else
			--sp;
		*sp += 5;
		while(*sp > '9')
		{	if(sp > b)
			{	*sp = '0';
				*--sp += 1;
			}
			else
			{	/* next power of 10 and at beginning */
				*sp = '1';
				*decpt += 1;
				if(!(format&SFFMT_EFORMAT))
				{	/* add one more 0 for %f precision */
					if (sp != &ep[-1])
					{	/* prevents overwriting the previous 1 with 0 */
						ep[-1] = '0';
					}
					ep += 1;
				}
				break;
			}
		}
	}

 done:
	*--ep = '\0';
	if(len)
		*len = ep-b;
	return b;
 around:
	if (((m >> x) & 0xf) >= 8)
	{	t = sp - 1;
		for (;;)
		{	if (--t <= b)
			{	(*decpt)++;
				break;
			}
			switch (*t)
			{
			case 'f':
			case 'F':
				*t = '0';
				continue;
			case '9':
				*t = ep[10];
				break;
			default:
				(*t)++;
				break;
			}
			break;
		}
	}
	ep = sp + 1;
	goto done;
}

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
*               Glenn Fowler <glenn.s.fowler@gmail.com>                *
*                    David Korn <dgkorn@gmail.com>                     *
*                     Phong Vo <phongvo@gmail.com>                     *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*               K. Eugene Carlson <kvngncrlsn@gmail.com>               *
*                                                                      *
***********************************************************************/
/*
 * Glenn Fowler
 * AT&T Research
 *
 * time conversion support
 */

#include <tm.h>
#include <ctype.h>
#include <namval.h>

#include "FEATURE/tmlib"

#ifndef tzname
#	if !_dat_tzname
#		if _dat__tzname
#			undef	_dat_tzname
#			define _dat_tzname	1
#			define tzname		_tzname
#		endif
#	endif
#	if _dat_tzname && !defined(tzname)
		extern char*		tzname[];
#	endif
#endif

#define TM_type		(-1)

static const Namval_t		options[] =
{
	"adjust",	TM_ADJUST,
	"format",	TM_DEFAULT,
	"leap",		TM_LEAP,
	"subsecond",	TM_SUBSECOND,
	"type",		TM_type,
	"utc",		TM_UTC,
	0,		0
};

/*
 * 2007-03-19 move tm_info from _tm_info_ to (*_tm_infop_)
 *	      to allow future Tm_info_t growth
 */

static Tm_info_t	_tm_info_ = { 0 };
Tm_info_t*		_tm_infop_ = &_tm_info_;

static char*		tz_abbr;

#if _tzset_environ

static char	TZ[256];
static char*	TE[2];

struct tm*
_tm_localtime(const time_t* t)
{
	struct tm*	r;
	char*		e = NULL;
	char**		v = environ;

	if (TZ[0])
	{
		if (!environ || !*environ)
			environ = TE;
		else
			e = environ[0];
		environ[0] = TZ;
	}
	r = localtime(t);
	if (TZ[0])
	{
		if (environ != v)
			environ = v;
		else
			environ[0] = e;
	}
	return r;
}

#endif

/*
 * return minutes west of GMT for local time clock
 *
 * isdst will point to non-zero if DST is in effect
 * this routine also kicks in the local initialization
 */

static int
tzwest(time_t* clock, int* isdst)
{
	struct tm*	tp;
	int		n;
	int		m;
	int		h;
	time_t		epoch;

	/*
	 * convert to GMT assuming local time
	 */

	if (!(tp = gmtime(clock)))
	{
		/*
		 * some systems return 0 for negative time_t
		 */

		epoch = 0;
		clock = &epoch;
		tp = gmtime(clock);
	}
	n = tp->tm_yday;
	h = tp->tm_hour;
	m = tp->tm_min;

	/*
	 * tmlocaltime() handles DST and GMT offset
	 */

	tp = tmlocaltime(clock);
#if _mem_tm_zone_tm
	if (tp->tm_zone && !tz_abbr)
		tz_abbr = strdup(tp->tm_zone);
#endif
	if (n = tp->tm_yday - n)
	{
		if (n > 1)
			n = -1;
		else if (n < -1)
			n = 1;
	}
	*isdst = tp->tm_isdst;
	return (h - tp->tm_hour - n * 24) * 60 + m - tp->tm_min;
}

/*
 * stropt() option handler
 */

static int
tmopt(void* a, const void* p, int n, const char* v)
{
	Tm_zone_t*	zp;

	NoP(a);
	if (p)
		switch (((Namval_t*)p)->value)
		{
		case TM_DEFAULT:
			tm_info.deformat = (n && (n = (int)strlen(v)) > 0 && (n < 2 || v[n-2] != '%' || v[n-1] != '?')) ? strdup(v) : tm_info.format[TM_DEFAULT];
			break;
		case TM_type:
			tm_info.local->type = (n && *v) ? ((zp = tmtype(v, NULL)) ? zp->type : strdup(v)) : 0;
			break;
		default:
			if (n)
				tm_info.flags |= ((Namval_t*)p)->value;
			else
				tm_info.flags &= ~((Namval_t*)p)->value;
			break;
		}
	return 0;
}

/*
 * initialize the local timezone
 */

static void
tmlocal(time_t now)
{
	Tm_zone_t*		zp;
	char*			s;
	char*			e = NULL;
	int			utc_offset;	/* offset from Coordinated Universal Time (UTC) */
	int			utc_offset_old;
	int			isdst;		/* nonzero if daylight saving time */
	int			isdst_old;
	int			dst_offset;
	int			month;
	char*			t;
	struct tm*		tp;
	char			buf[16];

	static Tm_zone_t	local;

#if _tzset_environ
	{
		char**	v = environ;

		if (s = getenv("TZ"))
		{
			sfsprintf(TZ, sizeof(TZ), "TZ=%s", s);
			if (!environ || !*environ)
				environ = TE;
			else
				e = environ[0];
			environ[0] = TZ;
		}
		else
			TZ[0] = 0;
#endif
#if _lib_tzset
		tzset();
#endif
#if _tzset_environ
		if (environ != v)
			environ = v;
		else if (e)
			environ[0] = e;
	}
#endif
	tmlocale();

	/*
	 * tm_info.local
	 */

	tm_info.zone = tm_info.local = &local;
	utc_offset = tzwest(&now, &isdst);
	utc_offset_old = utc_offset;
	isdst_old = isdst;

	/*
	 * compute local DST offset by roaming
	 * through the last 12 months until tzwest() changes
	 * with a system-recognized DST change
	 */

	for (month = 1; month <= 12; month++)
	{
		now -= 31 * 24 * 60 * 60;
		/* only break offset calculations for a DST shift */
		if ((dst_offset = tzwest(&now, &isdst)) != utc_offset && ((!isdst && isdst_old) || (isdst && !isdst_old)))
		{
			dst_offset -= (utc_offset - utc_offset_old);
			utc_offset = utc_offset_old;
			if (!isdst)
			{
				utc_offset = dst_offset;
				dst_offset = utc_offset_old;
			}
			break;
		}
		utc_offset = dst_offset;
		if (month == 12)
		{
			utc_offset = utc_offset_old;
			dst_offset = utc_offset;
		}
	}
	dst_offset -= utc_offset;
	isdst = isdst_old;
	local.west = (short)utc_offset;
	local.dst = (short)dst_offset;

	/*
	 * now get the time zone names
	 */

	if (local.standard)
	{
		free(local.standard);
		local.standard = NULL;
	}
	if (local.daylight)
	{
		free(local.daylight);
		local.daylight = NULL;
	}
	if (tz_abbr)
	{
		if (!isdst)
			local.standard = strdup(tz_abbr);
		else
			local.daylight = strdup(tz_abbr);
	}
#if _dat_tzname
	if (tzname[0])
	{
		/*
		 * POSIX
		 */

		if (!local.standard)
			local.standard = strdup(tzname[0]);
		if (!local.daylight)
			local.daylight = strdup(tzname[1]);
	}
	else
#endif
	if ((s = getenv("TZNAME")) && *s)
	{
		/*
		 * BSD
		 */

		s = strdup(s);
		if (!local.standard)
			local.standard = s;
		if (!local.daylight && s && (s = strchr(s, ',')))
		{
			*s++ = 0;
			local.daylight = strdup(s);
		}
	}
	else
	{
		/*
		 * tm_data.zone table lookup
		 */

		t = 0;
		for (zp = tm_data.zone; zp->standard; zp++)
		{
			if (zp->type)
				t = zp->type;
			if (zp->west == utc_offset && zp->dst == dst_offset)
			{
				local.type = t;
				if (!local.standard)
					local.standard = strdup(zp->standard);
				if (!(s = zp->daylight))
				{
					e = (s = buf) + sizeof(buf);
					s = tmpoff(s, (size_t)(e - s), zp->standard, 0, 0);
					if (s < e - 1)
					{
						*s++ = ' ';
						tmpoff(s, (size_t)(e - s), tm_info.format[TM_DT], dst_offset, TM_DST);
					}
					s = buf;
				}
				if (!local.daylight)
					local.daylight = strdup(s);
				break;
			}
		}
		if (!zp->standard)
		{
			/*
			 * not in the table
			 */

			e = (s = buf) + sizeof(buf);
			s = tmpoff(s, (size_t)(e - s), tm_info.format[TM_UT], utc_offset, 0);
			if (!local.standard)
				local.standard = strdup(buf);
			if (s < e - 1)
			{
				*s++ = ' ';
				tmpoff(s, (size_t)(e - s), tm_info.format[TM_UT], dst_offset, TM_DST);
				if (!local.daylight)
					local.daylight = strdup(buf);
			}
		}
	}
	if (!local.standard && !local.west && !local.dst && (s = getenv("TZ")))
	{
		if ((zp = tmzone(s, &t, NULL, NULL)) && !*t)
		{
			local.standard = strdup(zp->standard);
			if (zp->daylight)
				local.daylight = strdup(zp->daylight);
			local.west = zp->west;
			local.dst = zp->dst;
		}
		else
			local.standard = strdup(s);
	}

	/*
	 * set the options
	 */

	stropt(getenv("TM_OPTIONS"), options, sizeof(*options), tmopt, NULL);

	/*
	 * the time zone type is probably related to the locale
	 */

	if (!local.type && local.standard)
	{
		s = local.standard;
		t = 0;
		for (zp = tm_data.zone; zp->standard; zp++)
		{
			if (zp->type)
				t = zp->type;
			if (tmword(s, NULL, zp->standard, NULL, 0))
			{
				local.type = t;
				break;
			}
		}
	}

	/*
	 * tm_info.flags
	 */

	if (!(tm_info.flags & TM_ADJUST))
	{
		now = (time_t)78811200;		/* Jun 30 1972 23:59:60 */
		tp = tmlocaltime(&now);
		if (tp->tm_sec != 60)
			tm_info.flags |= TM_ADJUST;
	}
	if (!(tm_info.flags & TM_UTC) && local.standard)
	{
		s = local.standard;
		zp = tm_data.zone;
		if (local.daylight)
			zp++;
		for (; !zp->type && zp->standard; zp++)
			if (tmword(s, NULL, zp->standard, NULL, 0))
			{
				tm_info.flags |= TM_UTC;
				break;
			}
	}
}

/*
 * initialize tm data
 */

void
tminit(Tm_zone_t* zp, time_t now, const char newzone)
{
	static uint64_t		serial = ~(uint64_t)0;

	if(tz_abbr)
		free(tz_abbr);
	tz_abbr = 0;
	if (serial != ast.env_serial)
	{
		serial = ast.env_serial;
		if (tm_info.local)
		{
			if (tm_info.local->standard)
				free(tm_info.local->standard);
			if (tm_info.local->daylight)
				free(tm_info.local->daylight);
			memset(tm_info.local, 0, sizeof(*tm_info.local));
			tm_info.local = 0;
		}
	}
	if (!tm_info.local || newzone)
		tmlocal(now);
	if (!zp || newzone)
		zp = tm_info.local;
	tm_info.zone = zp;
}

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

/*
 * return the record format string given a format descriptor
 */

#include <recfmt.h>
#include <ctype.h>

char*
fmtrec(Recfmt_t f, int fs)
{
	char*	b;
	char*	e;
	char*	s;
	long	n;
	char	del[2];

	b = s = fmtbuf(n = 32);
	e = b + n;
	switch (RECTYPE(f))
	{
	case REC_delimited:
		*s++ = 'd';
		if ((del[0] = (char)(REC_D_DELIMITER(f))) != '\n')
		{
			del[1] = 0;
			if (fs)
				sfsprintf(s, (size_t)(e - s), "0x%02x", *(unsigned char*)del);
			else
				sfsprintf(s, (size_t)(e - s), "%s", fmtquote(del, NULL, NULL, 1, 0));
		}
		else
			*s = 0;
		break;
	case REC_fixed:
		if (!fs)
			*s++ = 'f';
		sfsprintf(s, (size_t)(e - s), "%lu", REC_F_SIZE(f));
		break;
	case REC_variable:
		*s++ = 'v';
		if (n = REC_V_SIZE(f))
			s += sfsprintf(s, (size_t)(e - s), "%lu", n);
		if (REC_V_HEADER(f) != 4)
			s += sfsprintf(s, (size_t)(e - s), "h%u", REC_V_HEADER(f));
		if (REC_V_OFFSET(f) != 0)
			s += sfsprintf(s, (size_t)(e - s), "o%u", REC_V_OFFSET(f));
		if (REC_V_LENGTH(f) != 2)
			s += sfsprintf(s, (size_t)(e - s), "z%u", REC_V_LENGTH(f));
		if (REC_V_LITTLE(f) != 0)
			*s++ = 'l';
		if (REC_V_INCLUSIVE(f) == 0)
			*s++ = 'n';
		*s = 0;
		break;
	case REC_method:
		*s++ = 'm';
		switch (n = REC_M_INDEX(f))
		{
		case REC_M_data:
			sfsprintf(s, (size_t)(e - s), "data");
			break;
		case REC_M_path:
			sfsprintf(s, (size_t)(e - s), "path");
			break;
		default:
			sfsprintf(s, (size_t)(e - s), "%lu", n);
			break;
		}
		break;
	case REC_none:
		*s++ = 'n';
		*s = 0;
		break;
	default:
		sfsprintf(s, (size_t)(e - s), "u%u.0x%07x", RECTYPE(f), REC_U_ATTRIBUTES(f));
		break;
	}
	return b;
}

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
 * D. G. Korn
 * G. S. Fowler
 * AT&T Research
 *
 * match shell file patterns
 * this interface is a wrapper on regex
 *
 *	sh pattern	egrep RE	description
 *	----------	--------	-----------
 *	*		.*		0 or more chars
 *	?		.		any single char
 *	[.]		[.]		char class
 *	[!.]		[^.]		negated char class
 *	[[:.:]]		[[:.:]]		ctype class
 *	[[=.=]]		[[=.=]]		equivalence class
 *	[[...]]		[[...]]		collation element
 *	*(.)		(.)*		0 or more of
 *	+(.)		(.)+		1 or more of
 *	?(.)		(.)?		0 or 1 of
 *	(.)		(.)		1 of
 *	@(.)		(.)		1 of
 *	a|b		a|b		a or b
 *	\#				() subgroup back reference [1-9]
 *	a&b				a and b
 *	!(.)				none of
 *
 * \ used to escape metacharacters
 *
 *	*, ?, (, |, &, ), [, \ must be \'d outside of [...]
 *	only ] must be \'d inside [...]
 *
 */

#include <ast.h>
#include <regex.h>

static struct State_s
{
	regmatch_t*	match;
	ssize_t		nmatch;
} matchstate;

/*
 * subgroup match
 * 0 returned if no match
 * otherwise number of subgroups matched returned
 * match group begin offsets are even elements of sub
 * match group end offsets are odd elements of sub
 * the matched string is from s+sub[0] up to but not
 * including s+sub[1]
 */

ssize_t
strngrpmatch(const char* b, size_t z, const char* p, ssize_t* sub, ssize_t n, regflags_t flags)
{
	regex_t*	re;
	ssize_t*	end;
	ssize_t		i;
	regflags_t	reflags;

	/*
	 * 0 and empty patterns are special
	 */

	if (!p || !b)
	{
		if (!p && !b)
			regcache(NULL, 0, NULL);
		return 0;
	}
	if (!*p)
	{
		if (sub && n > 0)
			sub[0] = sub[1] = 0;
		return *b == 0;
	}

	/*
	 * convert flags
	 */

	if (flags & REG_ADVANCE)
		reflags = flags & ~REG_ADVANCE;
	else
	{
		reflags = REG_SHELL|REG_AUGMENTED;
		if (!(flags & STR_MAXIMAL))
			reflags |= REG_MINIMAL;
		if (flags & STR_GROUP)
			reflags |= REG_SHELL_GROUP;
		if (flags & STR_LEFT)
			reflags |= REG_LEFT;
		if (flags & STR_RIGHT)
			reflags |= REG_RIGHT;
		if (flags & STR_ICASE)
			reflags |= REG_ICASE;
	}
	if (!sub || n <= 0)
		reflags |= REG_NOSUB;
	if (!(re = regcache(p, reflags, NULL)))
		return 0;
	if (n > matchstate.nmatch)
	{
		if (!(matchstate.match = newof(matchstate.match, regmatch_t, (size_t)n, 0)))
			return 0;
		matchstate.nmatch = n;
	}
	if (regnexec(re, b, z, (size_t)n, matchstate.match, reflags & ~(REG_MINIMAL|REG_SHELL_GROUP|REG_LEFT|REG_RIGHT|REG_ICASE)))
		return 0;
	if (!sub || n <= 0)
		return 1;
	i = (ssize_t)re->re_nsub;
	end = sub + n * 2;
	for (n = 0; sub < end && n <= i; n++)
	{
		*sub++ = matchstate.match[n].rm_so;
		*sub++ = matchstate.match[n].rm_eo;
	}
	return i + 1;
}

/*
 * compare the string s with the shell pattern p
 * returns 1 for match 0 otherwise
 */

ssize_t
strmatch(const char* s, const char* p)
{
	return strngrpmatch(s, s ? strlen(s) : 0, p, NULL, 0, STR_MAXIMAL|STR_LEFT|STR_RIGHT);
}

/*
 * leading substring match
 * first char after end of substring returned
 * 0 returned if no match
 *
 * OBSOLETE: use strgrpmatch()
 */

char*
strsubmatch(const char* s, const char* p, regflags_t flags)
{
	ssize_t	match[2];

	return strngrpmatch(s, s ? strlen(s) : 0, p, match, 1, (flags ? STR_MAXIMAL : 0)|STR_LEFT) ? (char*)s + match[1] : NULL;
}

ssize_t
strgrpmatch(const char* b, const char* p, ssize_t* sub, ssize_t n, regflags_t flags)
{
	return strngrpmatch(b, b ? strlen(b) : 0, p, sub, n, flags);
}

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
 * regcomp() regex_t cache
 * AT&T Research
 */

#include <ast.h>
#include <regex.h>

#define CACHE		8		/* default # cached re's	*/
#define ROUND		64		/* pattern buffer size round	*/

typedef unsigned long Key_t;

typedef struct Cache_s
{
	char*		pattern;
	regex_t		re;
	unsigned long	serial;
	regflags_t	reflags;
	int		keep;
	size_t		size;
} Cache_t;

typedef struct State_s
{
	size_t		size;
	unsigned long	serial;
	char*		locale;
	Cache_t**	cache;
} State_t;

static State_t	matchstate;

/*
 * flush the cache
 */

static void
flushcache(void)
{
	ssize_t		i;

	for (i = (ssize_t)matchstate.size; i--;)
		if (matchstate.cache[i] && matchstate.cache[i]->keep)
		{
			matchstate.cache[i]->keep = 0;
			regfree(&matchstate.cache[i]->re);
		}
}

/*
 * return regcomp() compiled re for pattern and reflags
 */

regex_t*
regcache(const char* pattern, regflags_t reflags, int* status)
{
	Cache_t*	cp;
	ssize_t		i;
	int		j;
	char*		s;
	ssize_t		empty;
	ssize_t		unused;
	ssize_t		old;
	Key_t		key;

	/*
	 * 0 pattern flushes the cache and reflags>0 extends cache
	 */

	if (!pattern)
	{
		flushcache();
		j = 0;
		if (reflags > matchstate.size)
		{
			if (matchstate.cache = newof(matchstate.cache, Cache_t*, reflags, 0))
				matchstate.size = reflags;
			else
			{
				matchstate.size = 0;
				j = 1;
			}
		}
		if (status)
			*status = j;
		return NULL;
	}
	if (!matchstate.cache)
	{
		if (!(matchstate.cache = newof(0, Cache_t*, CACHE, 0)))
			return NULL;
		matchstate.size = CACHE;
	}

	/*
	 * flush the cache if the locale changed
	 * the AST setlocale() intercept maintains
	 * persistent setlocale() return values
	 */

	if ((s = setlocale(LC_CTYPE, NULL)) != matchstate.locale)
	{
		matchstate.locale = s;
		flushcache();
	}

	/*
	 * check if the pattern is in the cache
	 */

	for (i = 0; i < (ssize_t)sizeof(key) && pattern[i]; i++)
		((char*)&key)[i] = pattern[i];
	for (; i < (ssize_t)sizeof(key); i++)
		((char*)&key)[i] = 0;
	empty = unused = -1;
	old = 0;
	for (i = (ssize_t)matchstate.size; i--;)
		if (!matchstate.cache[i])
			empty = i;
		else if (!matchstate.cache[i]->keep)
			unused = i;
		else if (*(Key_t*)matchstate.cache[i]->pattern == key && !strcmp(matchstate.cache[i]->pattern, pattern) && matchstate.cache[i]->reflags == reflags)
			break;
		else if (!matchstate.cache[old] || matchstate.cache[old]->serial > matchstate.cache[i]->serial)
			old = i;
	if (i < 0)
	{
		if (unused < 0)
		{
			if (empty < 0)
				unused = old;
			else
				unused = empty;
		}
		if (!(cp = matchstate.cache[unused]) && !(cp = matchstate.cache[unused] = newof(0, Cache_t, 1, 0)))
		{
			if (status)
				*status = REG_ESPACE;
			return NULL;
		}
		if (cp->keep)
		{
			cp->keep = 0;
			regfree(&cp->re);
		}
		if ((i = (ssize_t)strlen(pattern) + 1) > (ssize_t)cp->size)
		{
			cp->size = (size_t)roundof(i, ROUND);
			if (!(cp->pattern = newof(cp->pattern, char, cp->size, 0)))
			{
				if (status)
					*status = REG_ESPACE;
				return NULL;
			}
		}
		strcpy(cp->pattern, pattern);
		while (++i < (ssize_t)sizeof(Key_t))
			cp->pattern[i] = 0;
		pattern = (const char*)cp->pattern;
		if (j = regcomp(&cp->re, pattern, reflags))
		{
			if (status)
				*status = j;
			return NULL;
		}
		cp->keep = 1;
		cp->reflags = reflags;
	}
	else
		cp = matchstate.cache[i];
	cp->serial = ++matchstate.serial;
	if (status)
		*status = 0;
	return &cp->re;
}

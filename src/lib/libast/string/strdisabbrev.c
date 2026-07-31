/***********************************************************************
*                                                                      *
*              This file is part of the ksh 93u+m package              *
*             Copyright (c) 2026 Contributors to ksh 93u+m             *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                  Martijn Dekker <martijn@inlv.org>                   *
*                                                                      *
***********************************************************************/

#include <ast.h>

/*
 * Disabbreviate a string (like an option or keyword):
 * match a string abbreviation against a list of strings.
 *
 * For details, see the manual: strdisabbrev(3)
 */

int strdisabbrev(const char *abbrev, const char *list[], int num, unsigned int flags)
{
	int	(*cmp)(const char *, const char *, size_t) = flags & DISABBREV_ICASE ? strncasecmp : strncmp;
	int	i, r, n = 0, found = -1;
	size_t	len = strlen(abbrev);

	for (i = 0; i < num; i++)
	{
		if ((r = (*cmp)(list[i], abbrev, len)) == 0)
		{
			/* identical to abbrev: never ambiguous (e.g., 'glob' when there is also 'globstar') */
			if (list[i][len] == '\0')
				return i;
			/* more than one prefix match: ambiguous */
			if (++n > 1)
				return -2;
			/* got a candidate */
			found = i;
		}
		/* sorted list: bail out if the first len chars of list[i] sort after abbrev */
		else if (r > 0 && flags & DISABBREV_SORTED)
			break;
	}
	return found;
}

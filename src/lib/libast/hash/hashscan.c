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
 * Glenn Fowler
 * AT&T Research
 *
 * hash table library
 */

#include "hashlib.h"

/*
 * hash table sequential scan
 *
 *	Hash_position_t*	pos;
 *	Hash_bucket_t*		b;
 *	pos = hashscan(tab, flags);
 *	while (b = hashnext(&pos)) ...;
 *	hashdone(pos);
 */

/*
 * return pos for scan on table
 */

Hash_position_t*
hashscan(Hash_table_t* tab, int flags)
{
	Hash_position_t*	pos;

	static Hash_bucket_t		empty;

	if (!(pos = newof(0, Hash_position_t, 1, 0))) return NULL;
	pos->tab = tab->root->last.table = tab;
	pos->bucket = &empty;
	pos->slot = tab->table - 1;
	pos->limit = tab->table + tab->size;
	if (tab->scope && !(flags & HASH_NOSCOPE))
	{
		pos->flags = HASH_SCOPE;
		do
		{
			Hash_bucket_t*	b;

			if (tab->frozen)
			{
				Hash_bucket_t**	sp = tab->table;
				Hash_bucket_t**	sx = tab->table + tab->size;

				while (sp < sx)
					for (b = *sp++; b; b = b->next)
						b->hash &= (unsigned long)~HASH_HIDDEN;
			}
		} while (tab = tab->scope);
		tab = pos->tab;
	}
	else pos->flags = 0;
	tab->frozen++;
	return pos;
}

/*
 * return next scan element
 */

Hash_bucket_t*
hashnext(Hash_position_t* pos)
{
	Hash_bucket_t*	b;

	if (!pos) return NULL;
	b = pos->bucket;
	for (;;)
	{
		if (!(b = b->next))
		{
			do
			{
				if (++pos->slot >= pos->limit)
				{
					pos->tab->frozen--;
					if (!pos->flags || !pos->tab->scope) return NULL;
					pos->tab = pos->tab->scope;
					pos->tab->root->last.table = pos->tab;
					pos->limit = (pos->slot = pos->tab->table) + pos->tab->size;
					pos->tab->frozen++;
				}
			} while (!(b = *pos->slot));
		}
		if (!(b->hash & HASH_DELETED) && (!(pos->tab->flags & HASH_VALUE) || b->value) && (!pos->flags || !(b->hash & (HASH_HIDDEN|HASH_HIDES)))) break;
		if (b->hash & HASH_HIDES)
		{
			Hash_bucket_t*	h = (Hash_bucket_t*)b->name;

			if (!(h->hash & HASH_HIDDEN))
			{
				h->hash |= HASH_HIDDEN;
				if (!(b->hash & HASH_DELETED)) break;
			}
		}
		else b->hash &= (unsigned long)~HASH_HIDDEN;
	}
	return pos->tab->root->last.bucket = pos->bucket = b;
}

/*
 * terminate scan
 */

void
hashdone(Hash_position_t* pos)
{
	if (pos)
	{
		if (pos->tab->frozen)
			pos->tab->frozen--;
		free(pos);
	}
}

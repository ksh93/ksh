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
 * change table size and rehash
 * size must be a power of 2
 */

void
hashsize(Hash_table_t* tab, ssize_t size)
{
	Hash_bucket_t**		old_s;
	Hash_bucket_t**		new_s;
	Hash_bucket_t*		old_b;
	Hash_bucket_t*		new_b;
	Hash_bucket_t**		old_sx;
	unsigned long		index;
	Hash_region_f		region;
	void*			handle;

	if (size > 0 && size != tab->size && !(size & (size - 1)))
	{
		if (region = tab->root->local->region)
		{
			handle = tab->root->local->handle;
			new_s = (Hash_bucket_t**)(*region)(handle, NULL, sizeof(Hash_bucket_t*) * (size_t)size, 0);
		}
		else new_s = newof(0, Hash_bucket_t*, (size_t)size, 0);
		if (!new_s) tab->flags |= HASH_FIXED;
		else
		{
			old_sx = (old_s = tab->table) + tab->size;
			tab->size = size;
			while (old_s < old_sx)
			{
				old_b = *old_s++;
				while (old_b)
				{
					new_b = old_b;
					old_b = old_b->next;
					index = new_b->hash;
					HASHMOD(tab, index);
					new_b->next = new_s[index];
					new_s[index] = new_b;
				}
			}
			if ((tab->flags & (HASH_RESIZE|HASH_STATIC)) != HASH_STATIC)
			{
				if (region) (*region)(handle, tab->table, 0, 0);
				else free(tab->table);
			}
			tab->table = new_s;
			tab->flags |= HASH_RESIZE;
		}
	}
}

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
 * AT&T Bell Laboratories
 *
 * hash table library
 */

#include "hashlib.h"

/*
 * push/pop/query hash table scope
 *
 *	bot==0		pop top scope
 *	bot==top	query
 *	bot!=0		push top on bot
 *
 * scope table pointer returned
 */

Hash_table_t*
hashview(Hash_table_t* top, Hash_table_t* bot)
{
	Hash_bucket_t*		b;
	Hash_bucket_t*		p;
	Hash_bucket_t**	sp;
	Hash_bucket_t**	sx;

	if (!top || top->frozen)
		bot = 0;
	else if (top == bot)
		bot = top->scope;
	else if (bot)
	{
		if (top->scope)
			bot = 0;
		else
		{
			sx = &top->table[top->size];
			sp = &top->table[0];
			while (sp < sx)
				for (b = *sp++; b; b = b->next)
					if (p = (Hash_bucket_t*)hashlook(bot, b->name, HASH_LOOKUP, NULL))
					{
						b->name = (p->hash & HASH_HIDES) ? p->name : (char*)b;
						b->hash |= HASH_HIDES;
					}
			top->scope = bot;
			bot->frozen++;
		}
	}
	else if (bot = top->scope)
	{
		sx = &top->table[top->size];
		sp = &top->table[0];
		while (sp < sx)
			for (b = *sp++; b; b = b->next)
				if (b->hash & HASH_HIDES)
				{
					b->hash &= (unsigned long)~HASH_HIDES;
					b->name = ((Hash_bucket_t*)b->name)->name;
				}
		top->scope = 0;
		bot->frozen--;
	}
	return bot;
}

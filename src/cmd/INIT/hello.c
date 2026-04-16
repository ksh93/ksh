/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1994-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2026 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/

/*
 * Compiler test used by bin/package to enforce C99 and block C++
 */

struct foo
{
	char *bar;
	/* flexible array member requires C99 */
	int baz[];
};

/* restrict requires C99, is an error in C++ */
void r(const char *restrict cp)
{
	(void)cp;
}

int main(void)
{
	/* designated struct initializer requires C99 */
	struct foo s = { .bar = "quux" };
	/* variable name 'new' is an error in C++ */
	int new = 0;

	r(s.bar);
	return new;
}

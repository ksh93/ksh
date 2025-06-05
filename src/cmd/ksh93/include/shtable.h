/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2025 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                  David Korn <dgk@research.att.com>                   *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
#ifndef _SHTABLE_H

/*
 * David Korn
 * AT&T Labs
 *
 * Interface definitions read-only data tables for shell
 *
 */

#define _SHTABLE_H	1

typedef struct shtable1
{
	const char	*sh_name;
	const unsigned	sh_number;
} Shtable_t;

struct shtable2
{
	const char	*sh_name;
	const unsigned	sh_number;
	const char	*sh_value;
};

struct shtable3
{
	const char	*sh_name;
	const unsigned	sh_number;
	int		(*sh_value)(int, char*[], Shbltin_t*);
};

#define sh_lookup(name,value)	(sh_locate(name,(Shtable_t*)(value),sizeof(*(value)))->sh_number)
extern const Shtable_t		shtab_testops[];
extern const Shtable_t		shtab_options[];
extern const Shtable_t		shtab_attributes[];
extern const struct shtable2	shtab_variables[];
extern const struct shtable2	shtab_aliases[];
extern const struct shtable2	shtab_signals[];
extern const struct shtable3	shtab_builtins[];
extern const Shtable_t		shtab_reserved[];
extern const Shtable_t		*sh_locate(const char*, const Shtable_t*, int);
extern int			sh_lookopt(const char*, int*);
extern Dt_t			*sh_inittree(const struct shtable2*);

#endif /* _SHTABLE_H */

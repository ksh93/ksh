/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2025 Contributors to ksh 93u+m           *
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
 * some systems may pull in <ast_common.h> and its <ast_map.h>
 * which we are in the process of generating ... this prevents it
 */

#define _def_map_ast	1

#include "FEATURE/lib"

int
main(void)
{
	printf("/*\n");
	printf(" * prototypes provided for standard interfaces hijacked\n");
	printf(" * by AST and mapped to _ast_* but already prototyped\n");
	printf(" * unmapped in native headers included by <ast_std.h>\n");
	printf(" */\n");
	printf("\n");
	/* use the libast glob functions rather than the native versions */
	printf("#undef	glob\n");
	printf("#define glob		_ast_glob\n");
	printf("#undef	globfree\n");
	printf("#define globfree	_ast_globfree\n");
	/* always rename AST signal(3) to _ast_signal; this avoids breakage when using ASan */
	printf("#undef	signal\n");
	printf("#define signal      	_ast_signal\n");
	/* do the same with sigunblock(), just to be sure (e.g., native QNX sigunblock() is different) */
	printf("#undef	sigunblock\n");
	printf("#define sigunblock      _ast_sigunblock\n");
	/* Override the native regex library in favor of libast's regex functions */
	printf("#undef	regaddclass\n");
	printf("#define regaddclass	_ast_regaddclass\n");
	printf("#undef	regalloc\n");
	printf("#define regalloc	_ast_regalloc\n");
	printf("#undef	regcache\n");
	printf("#define regcache	_ast_regcache\n");
	printf("#undef	regclass\n");
	printf("#define regclass	_ast_regclass\n");
	printf("#undef	regcmp\n");
	printf("#define regcmp		_ast_regcmp\n");
	printf("#undef	regcollate\n");
	printf("#define regcollate      _ast_regcollate\n");
	printf("#undef	regcomp\n");
	printf("#define regcomp		_ast_regcomp\n");
	printf("#undef	regerror\n");
	printf("#define regerror	_ast_regerror\n");
	printf("#undef	regex\n");
	printf("#define regex		_ast_regex\n");
	printf("#undef	regexec\n");
	printf("#define regexec		_ast_regexec\n");
	printf("#undef	regfatal\n");
	printf("#define regfatal	_ast_regfatal\n");
	printf("#undef	regfatalpat\n");
	printf("#define regfatalpat     _ast_regfatalpat\n");
	printf("#undef	regfree\n");
	printf("#define regfree		_ast_regfree\n");
	printf("#undef	regnexec\n");
	printf("#define regnexec	_ast_regnexec\n");
	printf("#undef	regstat\n");
	printf("#define regstat		_ast_regstat\n");
	printf("#undef	regsubcomp\n");
	printf("#define regsubcomp	_ast_regsubcomp\n");
	printf("#undef	regsubexec\n");
	printf("#define regsubexec	_ast_regsubexec\n");
	printf("#undef	regsubflags\n");
	printf("#define regsubflags	_ast_regsubflags\n");
	printf("#undef	regsubfree\n");
	printf("#define regsubfree	_ast_regsubfree\n");
#if _lib_strmode
	printf("#undef	strmode\n");
	printf("#define strmode		_ast_strmode\n");
#endif
	/* we always use the libast strdup implementation */
	printf("#undef	strdup\n");
	printf("#define strdup		_ast_strdup\n");
	printf("extern char*		strdup(const char*);\n");

	/*
	 * overriding <stdlib.h> strto*() is problematic to say the least
	 */

#if _std_strtol
#if !__CYGWIN__
	printf("#undef	strtol\n");
	printf("#define strtol		_ast_strtol\n");
	printf("#undef	strtoul\n");
	printf("#define strtoul		_ast_strtoul\n");
#endif
	printf("#undef	strtoll\n");
	printf("#define strtoll		_ast_strtoll\n");
	printf("#undef	strtoull\n");
	printf("#define strtoull	_ast_strtoull\n");
#endif
#if _std_strtod
	printf("#undef	strtod\n");
	printf("#define strtod		_ast_strtod\n");
#endif
#if _std_strtold
	printf("#undef	strtold\n");
	printf("#define strtold		_ast_strtold\n");
#endif
#if !__CYGWIN__
#if _npt_strtol || _std_strtol
#if _npt_strtol && !_std_strtol
	printf("#ifndef _ISOC99_SOURCE\n");
#endif
	printf("extern long		strtol(const char*, char**, int);\n");
#if _npt_strtol && !_std_strtol
	printf("#endif\n");
#endif
#endif
#if _npt_strtoul || _std_strtol
#if _npt_strtoul && !_std_strtol
	printf("#ifndef _ISOC99_SOURCE\n");
#endif
	printf("extern unsigned long	strtoul(const char*, char**, int);\n");
#if _npt_strtoul && !_std_strtol
	printf("#endif\n");
#endif
#endif
#endif
#if _npt_strtod || _std_strtod
#if _npt_strtod && !_std_strtod
	printf("#ifndef _ISOC99_SOURCE\n");
#endif
	printf("extern double		strtod(const char*, char**);\n");
#if _npt_strtod && !_std_strtod
	printf("#endif\n");
#endif
#endif
#if _npt_strtold || _std_strtold
#if _npt_strtold && !_std_strtold
	printf("#ifndef _ISOC99_SOURCE\n");
#endif
	printf("extern _ast_fltmax_t	strtold(const char*, char**);\n");
#if _npt_strtold && !_std_strtold
	printf("#endif\n");
#endif
#endif
#if _npt_strtoll || _std_strtol
#if _npt_strtoll && !_std_strtol
	printf("#ifndef _ISOC99_SOURCE\n");
#endif
	printf("extern _ast_intmax_t		strtoll(const char*, char**, int);\n");
#if _npt_strtoll && !_std_strtol
	printf("#endif\n");
#endif
#endif
#if _npt_strtoull || _std_strtol
#if _npt_strtoull && !_std_strtol
	printf("#ifndef _ISOC99_SOURCE\n");
#endif
	printf("extern unsigned _ast_intmax_t	strtoull(const char*, char**, int);\n");
#if _npt_strtoull && !_std_strtoul
	printf("#endif\n");
#endif
#endif
	return 0;
}

/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2024 Contributors to ksh 93u+m           *
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
#include "FEATURE/mmap"
#include "FEATURE/options"
#include "FEATURE/eaccess"
#include "FEATURE/api"
#include <sig.h>

#if _opt_map_libc && !defined(_map_libc)
#define _map_libc	1
#endif

/*
 * NOTE: for conditionally compiled AST functions, the preprocessor directives below
 * must be kept in sync with the directives in the corresponding C source files, so
 * that the rename is not done if the AST version of the function is not compiled
 * in; otherwise a build with _map_libc defined will fail with linking errors.
 */

int
main(void)
{
	printf("/*\n");
	printf(" * prototypes provided for standard interfaces hijacked\n");
	printf(" * by AST and mapped to _ast_* but already prototyped\n");
	printf(" * unmapped in native headers included by <ast_std.h>\n");
	printf(" */\n");
	printf("\n");
#if __MVS__
#undef	_map_libc
#define _map_libc	1
	printf("\n");
	printf("/* mvs.390 libc.dll routines can't be intercepted by user dlls */\n");
	printf("#undef	_mem_dd_fd_DIR\n");
	printf("#undef	_typ_long_double\n");
#endif
#if _map_libc
#undef	_map_malloc
#define _map_malloc	1
	printf("\n");
	printf("#define	_map_libc	1\n");
#endif
#if _map_libc || defined(__linux__)
	printf("#undef	basename\n");
	printf("#define basename	_ast_basename\n");
	printf("#undef	dirname\n");
	printf("#define dirname		_ast_dirname\n");
#endif
#if _map_libc
#if !_lib_eaccess
	printf("#undef	eaccess\n");
	printf("#define eaccess		_ast_eaccess\n");
#endif
	printf("#undef	fnmatch\n");
	printf("#define fnmatch		_ast_fnmatch\n");
	printf("#undef	fts_children\n");
	printf("#define fts_children    _ast_fts_children\n");
	printf("#undef	fts_close\n");
	printf("#define fts_close       _ast_fts_close\n");
	printf("#undef	fts_flags\n");
	printf("#define fts_flags       _ast_fts_flags\n");
	printf("#undef	fts_notify\n");
	printf("#define fts_notify      _ast_fts_notify\n");
	printf("#undef	fts_open\n");
	printf("#define fts_open	_ast_fts_open\n");
	printf("#undef	fts_read\n");
	printf("#define fts_read	_ast_fts_read\n");
	printf("#undef	fts_set\n");
	printf("#define fts_set		_ast_fts_set\n");
	printf("#undef	ftw\n");
	printf("#define ftw		_ast_ftw\n");
	printf("#undef	ftwalk\n");
	printf("#define ftwalk		_ast_ftwalk\n");
	printf("#undef	ftwflags\n");
	printf("#define ftwflags	_ast_ftwflags\n");
#if !_WINIX
	printf("#undef	getcwd\n");
	printf("#define getcwd		_ast_getcwd\n");
	printf("extern char*		getcwd(char*, size_t);\n");
#endif
#endif
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
#if _map_libc
#if !_lib_memdup
	printf("#undef	memdup\n");
	printf("#define memdup		_ast_memdup\n");
#endif
	printf("#undef	memhash\n");
	printf("#define memhash		_ast_memhash\n");
	printf("#undef	memsum\n");
	printf("#define memsum		_ast_memsum\n");
	printf("#undef	mkstemp\n");
	printf("#define mkstemp		_ast_mkstemp\n");
	printf("extern int		mkstemp(char*);\n");
	printf("#undef	mktemp\n");
	printf("#define mktemp		_ast_mktemp\n");
	printf("extern char*		mktemp(char*);\n");
	printf("#undef	mktime\n");
	printf("#define mktime		_ast_mktime\n");
	printf("#undef	nftw\n");
	printf("#define nftw		_ast_nftw\n");
	printf("#undef	optctx\n");
	printf("#define optctx		_ast_optctx\n");
	printf("#undef	optesc\n");
	printf("#define optesc		_ast_optesc\n");
	printf("#undef	optget\n");
	printf("#define optget		_ast_optget\n");
	printf("#undef	opthelp\n");
	printf("#define opthelp		_ast_opthelp\n");
	printf("#undef	optjoin\n");
	printf("#define optjoin		_ast_optjoin\n");
	printf("#undef	optstr\n");
	printf("#define optstr		_ast_optstr\n");
	printf("#undef	optusage\n");
	printf("#define optusage	_ast_optusage\n");
	printf("#undef	pathaccess\n");
	printf("#define pathaccess	_ast_pathaccess\n");
	printf("#undef	pathbin\n");
	printf("#define pathbin		_ast_pathbin\n");
	printf("#undef	pathcanon\n");
	printf("#define pathcanon	_ast_pathcanon\n");
	printf("#undef	pathcat\n");
	printf("#define pathcat		_ast_pathcat\n");
	printf("#undef	pathcd\n");
	printf("#define pathcd		_ast_pathcd\n");
	printf("#undef	pathcheck\n");
	printf("#define pathcheck	_ast_pathcheck\n");
	printf("#undef	pathexists\n");
	printf("#define pathexists	_ast_pathexists\n");
	printf("#undef	pathfind\n");
	printf("#define pathfind	_ast_pathfind\n");
	printf("#undef	pathgetlink\n");
	printf("#define pathgetlink	_ast_pathgetlink\n");
	printf("#undef	pathicase\n");
	printf("#define pathicase	_ast_pathicase\n");
	printf("#undef	pathinclude\n");
	printf("#define pathinclude	_ast_pathinclude\n");
	printf("#undef	pathkey\n");
	printf("#define pathkey		_ast_pathkey\n");
	printf("#undef	pathnative\n");
	printf("#define pathnative	_ast_pathnative\n");
	printf("#undef	pathpath\n");
	printf("#define pathpath	_ast_pathpath\n");
	printf("#undef	pathposix\n");
	printf("#define pathposix	_ast_pathposix\n");
	printf("#undef	pathprog\n");
	printf("#define pathprog	_ast_pathprog\n");
	printf("#undef	pathrepl\n");
	printf("#define pathrepl	_ast_pathrepl\n");
	printf("#undef	pathsetlink\n");
	printf("#define pathsetlink	_ast_pathsetlink\n");
	printf("#undef	pathshell\n");
	printf("#define pathshell	_ast_pathshell\n");
	printf("#undef	pathstat\n");
	printf("#define pathstat	_ast_pathstat\n");
	printf("#undef	pathtemp\n");
	printf("#define pathtemp	_ast_pathtemp\n");
	printf("#undef	pathtmp\n");
	printf("#define pathtmp		_ast_pathtmp\n");
	printf("#undef	procclose\n");
	printf("#define procclose	_ast_procclose\n");
	printf("#undef	procfree\n");
	printf("#define procfree	_ast_procfree\n");
	printf("#undef	procopen\n");
	printf("#define procopen	_ast_procopen\n");
	printf("#undef	procrun\n");
	printf("#define procrun		_ast_procrun\n");
	printf("#undef	putenv\n");
	printf("#define putenv		_ast_putenv\n");
	printf("#undef	re_comp\n");
	printf("#define re_comp		_ast_re_comp\n");
	printf("#undef	re_exec\n");
	printf("#define re_exec		_ast_re_exec\n");
#endif
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
	printf("#undef	regcomb\n");
	printf("#define regcomb		_ast_regcomb\n");
	printf("#undef	regcomp\n");
	printf("#define regcomp		_ast_regcomp\n");
	printf("#undef	regdecomp\n");
	printf("#define regdecomp	_ast_regdecomp\n");
	printf("#undef	regdup\n");
	printf("#define regdup		_ast_regdup\n");
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
	printf("#undef	regncomp\n");
	printf("#define regncomp	_ast_regncomp\n");
	printf("#undef	regnexec\n");
	printf("#define regnexec	_ast_regnexec\n");
	printf("#undef	regrecord\n");
	printf("#define regrecord       _ast_regrecord\n");
	printf("#undef	regrexec\n");
	printf("#define regrexec	_ast_regrexec\n");
	printf("#undef	regstat\n");
	printf("#define regstat		_ast_regstat\n");
	printf("#undef	regsub\n");
	printf("#define regsub		_ast_regsub\n");
	printf("#undef	regsubcomp\n");
	printf("#define regsubcomp	_ast_regsubcomp\n");
	printf("#undef	regsubexec\n");
	printf("#define regsubexec	_ast_regsubexec\n");
	printf("#undef	regsubflags\n");
	printf("#define regsubflags	_ast_regsubflags\n");
	printf("#undef	regsubfree\n");
	printf("#define regsubfree	_ast_regsubfree\n");
#if _map_libc
#if !(_std_remove || !_lib_unlink)
	printf("#undef	remove\n");
	printf("#define remove		_ast_remove\n");
	printf("extern int		remove(const char*);\n");
#endif
	printf("#undef	resolvepath\n");
	printf("#define resolvepath	_ast_resolvepath\n");
	printf("extern int		resolvepath(const char*, char*, size_t);\n");
#if !_lib_setenv
	printf("#undef	setenv\n");
	printf("#define setenv		_ast_setenv\n");
	printf("extern int		setenv(const char*, const char*, int);\n");
#endif
	printf("#undef	setenviron\n");
	printf("#define setenviron      _ast_setenviron\n");
	printf("#undef	sigcritical\n");
	printf("#define sigcritical      _ast_sigcritical\n");
#if !_lib_stracmp
	printf("#undef	stracmp\n");
	printf("#define stracmp		_ast_stracmp\n");
#endif
	printf("#undef	strcopy\n");
	printf("#define strcopy		_ast_strcopy\n");
	printf("#undef	strelapsed\n");
	printf("#define strelapsed	_ast_strelapsed\n");
	printf("#undef	stresc\n");
	printf("#define stresc		_ast_stresc\n");
	printf("#undef	streval\n");
	printf("#define streval		_ast_streval\n");
	printf("#undef	strexpr\n");
	printf("#define strexpr		_ast_strexpr\n");
	printf("#undef	strftime\n");
	printf("#define strftime	_ast_strftime\n");
	printf("#undef	strgid\n");
	printf("#define strgid		_ast_strgid\n");
	printf("#undef	strgrpmatch\n");
	printf("#define strgrpmatch	_ast_strgrpmatch\n");
	printf("#undef	strngrpmatch\n");
	printf("#define strngrpmatch	_ast_strngrpmatch\n");
	printf("#undef	strhash\n");
	printf("#define strhash		_ast_strhash\n");
	printf("#undef	strkey\n");
	printf("#define strkey		_ast_strkey\n");
#if !_lib_strlcat
	printf("#undef	strlcat\n");
	printf("#define strlcat		_ast_strlcat\n");
	printf("extern size_t		strlcat(char*, const char*, size_t);\n");
#endif
#if !_lib_strlcpy
	printf("#undef	strlcpy\n");
	printf("#define strlcpy		_ast_strlcpy\n");
	printf("extern size_t		strlcpy(char*, const char*, size_t);\n");
#endif
	printf("#undef	strlook\n");
	printf("#define strlook		_ast_strlook\n");
	printf("#undef	strmatch\n");
	printf("#define strmatch	_ast_strmatch\n");
#endif
#if _map_libc || _lib_strmode
	printf("#undef	strmode\n");
	printf("#define strmode		_ast_strmode\n");
#endif
#if _map_libc
#if !_lib_strnacmp
	printf("#undef	strnacmp\n");
	printf("#define strnacmp	_ast_strnacmp\n");
#endif
	printf("#undef	strncopy\n");
	printf("#define strncopy	_ast_strncopy\n");
	printf("#undef	strntod\n");
	printf("#define strntod		_ast_strntod\n");
	printf("#undef	strntol\n");
	printf("#define strntol		_ast_strntol\n");
	printf("#undef	strntold\n");
	printf("#define strntold	_ast_strntold\n");
	printf("#undef	strntoll\n");
	printf("#define strntoll	_ast_strntoll\n");
	printf("#undef	strntoul\n");
	printf("#define strntoul	_ast_strntoul\n");
	printf("#undef	strntoull\n");
	printf("#define strntoull	_ast_strntoull\n");
	printf("#undef	stropt\n");
	printf("#define stropt		_ast_stropt\n");
	printf("#undef	strperm\n");
	printf("#define strperm		_ast_strperm\n");
	printf("#undef	strpsearch\n");
	printf("#define strpsearch	_ast_strpsearch\n");
#if !_lib_strptime
	printf("#undef	strptime\n");
	printf("#define strptime	_ast_strptime\n");
#endif
	printf("#undef	strsearch\n");
	printf("#define strsearch	_ast_strsearch\n");
	printf("#undef	strsort\n");
	printf("#define strsort		_ast_strsort\n");
	printf("#undef	strsubmatch\n");
	printf("#define strsubmatch	_ast_strsubmatch\n");
	printf("#undef	strsum\n");
	printf("#define strsum		_ast_strsum\n");
	printf("#undef	strtape\n");
	printf("#define strtape		_ast_strtape\n");
	printf("#undef	strtoip4\n");
	printf("#define strtoip4	_ast_strtoip4\n");
	printf("#undef	strton\n");
	printf("#define strton		_ast_strton\n");
	printf("#undef	strtonll\n");
	printf("#define strtonll	_ast_strtonll\n");
	printf("#undef	struid\n");
	printf("#define struid		_ast_struid\n");
	printf("#undef	struniq\n");
	printf("#define struniq		_ast_struniq\n");
	printf("#undef	system\n");
	printf("#define system		_ast_system\n");
	printf("extern int		system(const char*);\n");
	printf("#undef	tempnam\n");
	printf("#define tempnam		_ast_tempnam\n");
	printf("extern char*		tempnam(const char*, const char*);\n");
	printf("#undef	tmpnam\n");
	printf("#define tmpnam		_ast_tmpnam\n");
	printf("extern char*		tmpnam(char*);\n");
	printf("#undef	touch\n");
	printf("#define touch		_ast_touch\n");
	printf("#undef	wordexp\n");
	printf("#define wordexp		_ast_wordexp\n");
	printf("#undef	wordfree\n");
	printf("#define wordfree	_ast_wordfree\n");
#if !_lib_unsetenv
	printf("#undef	unsetenv\n");
	printf("#define unsetenv	_ast_unsetenv\n");
#endif
#endif
	/* we always use the libast strdup implementation */
	printf("#undef	strdup\n");
	printf("#define strdup		_ast_strdup\n");
	printf("extern char*		strdup(const char*);\n");

	/*
	 * overriding <stdlib.h> strto*() is problematic to say the least
	 */

#if _map_libc || _std_strtol
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
#if _map_libc || _std_strtod
	printf("#undef	strtod\n");
	printf("#define strtod		_ast_strtod\n");
#endif
#if _map_libc || _std_strtold
	printf("#undef	strtold\n");
	printf("#define strtold		_ast_strtold\n");
#endif
#if !__CYGWIN__
#if _npt_strtol || _map_libc || _std_strtol
#if _npt_strtol && !_map_libc && !_std_strtol
	printf("#ifndef _ISOC99_SOURCE\n");
#endif
	printf("extern long		strtol(const char*, char**, int);\n");
#if _npt_strtol && !_map_libc && !_std_strtol
	printf("#endif\n");
#endif
#endif
#if _npt_strtoul || _map_libc || _std_strtol
#if _npt_strtoul && !_map_libc && !_std_strtol
	printf("#ifndef _ISOC99_SOURCE\n");
#endif
	printf("extern unsigned long	strtoul(const char*, char**, int);\n");
#if _npt_strtoul && !_map_libc && !_std_strtol
	printf("#endif\n");
#endif
#endif
#endif
#if _npt_strtod || _map_libc || _std_strtod
#if _npt_strtod && !_map_libc && !_std_strtod
	printf("#ifndef _ISOC99_SOURCE\n");
#endif
	printf("extern double		strtod(const char*, char**);\n");
#if _npt_strtod && !_map_libc && !_std_strtod
	printf("#endif\n");
#endif
#endif
	printf("#undef	extern\n");
#if _npt_strtold || _map_libc || _std_strtold
#if _npt_strtold && !_map_libc && !_std_strtold
	printf("#ifndef _ISOC99_SOURCE\n");
#endif
	printf("extern _ast_fltmax_t	strtold(const char*, char**);\n");
#if _npt_strtold && !_map_libc && !_std_strtold
	printf("#endif\n");
#endif
#endif
	printf("#undef	extern\n");
#if _npt_strtoll || _map_libc || _std_strtol
#if _npt_strtoll && !_map_libc && !_std_strtol
	printf("#ifndef _ISOC99_SOURCE\n");
#endif
	printf("extern _ast_intmax_t		strtoll(const char*, char**, int);\n");
#if _npt_strtoll && !_map_libc && !_std_strtol
	printf("#endif\n");
#endif
#endif
#if _npt_strtoull || _map_libc || _std_strtol
#if _npt_strtoull && !_map_libc && !_std_strtol
	printf("#ifndef _ISOC99_SOURCE\n");
#endif
	printf("extern unsigned _ast_intmax_t	strtoull(const char*, char**, int);\n");
#if _npt_strtoull && !_map_libc && !_std_strtoul
	printf("#endif\n");
#endif
#endif

	/*
	 * finally some features/api mediation
	 */

#if defined(_API_ast_MAP) && _map_libc
	{
		const char*	s;
		const char*	t;

		static const char	map[] = _API_ast_MAP;

		printf("\n");
		t = map;
		do
		{
			for (s = t; *t && *t != ' '; t++);
			printf("#define %-.*s	_ast_%-.*s\n", (int)(t - s), s, (int)(t - s), s);
		} while (*t++);
	}
#endif
	return 0;
}

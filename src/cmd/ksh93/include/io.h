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
/*
 *	UNIX shell
 *	David Korn
 *
 */

#ifndef _io_h_defined
#define _io_h_defined	1

#include	<ast.h>
#include	<sfio.h>
#include	"FEATURE/fchdir"

#ifndef IOBSIZE
#   define  IOBSIZE	(SFIO_BUFSIZE*sizeof(char*))
#endif /* IOBSIZE */
#define IOMAXTRY	20

/* used for output of shell errors */
#define ERRIO		2

#define IOREAD		001
#define IOWRITE		002
#define IODUP 		004
#define IOSEEK		010
#define IONOSEEK	020
#define IOTTY 		040
#define IOCLEX 		0100
#define IOCLOSE		(IOSEEK|IONOSEEK)

#define IOSUBSHELL	0x8000	/* must be larger than any file descriptor */
#define IOPICKFD	0x10000 /* file descriptor number was selected automatically */
#define IOHERESTRING	0x20000 /* allow here documents to be string streams */

#ifndef ARG_RAW
    struct ionod;
#endif /* !ARG_RAW */

/* if O_SEARCH/O_PATH is unreliable for fchdir, it's not worth using */
#if !_fchdir_osearch_compat
#undef O_SEARCH
#define O_SEARCH 0
#endif

/*
 * Check if there is an editor active while avoiding repetitive #if flaggery.
 * The 0 definition is used to optimize out code if no editor is compiled in.
 * (This is here and not in edit.h because io.h is far more widely included.)
 */
#if SHOPT_ESH && SHOPT_VSH
#define sh_editor_active()	(sh_isoption(SH_VI) || sh_isoption(SH_EMACS) || sh_isoption(SH_GMACS))
#elif SHOPT_ESH
#define sh_editor_active()	(sh_isoption(SH_EMACS) || sh_isoption(SH_GMACS))
#elif SHOPT_VSH
#define sh_editor_active()	(sh_isoption(SH_VI)!=0)
#else
#define sh_editor_active()	0
#endif

extern int	sh_iocheckfd(int);
extern void 	sh_ioinit(void);
extern int 	sh_iomovefd(int);
extern int	sh_iorenumber(int,int);
extern void 	sh_pclose(int[]);
extern int	sh_rpipe(int[],int);
extern void 	sh_iorestore(int,int);
extern Sfio_t 	*sh_iostream(int);
extern int	sh_redirect(struct ionod*,int);
extern void 	sh_iosave(int,int,char*);
extern int 	sh_iovalidfd(int);
extern int	sh_iosafefd(int);
extern int 	sh_inuse(int);
extern void 	sh_iounsave(void);
extern void	sh_iounpipe(void);
extern int	sh_chkopen(const char*);
extern int	sh_ioaccess(int,int);
extern int	sh_isdevfd(const char*);

/* the following are readonly */
extern const char	e_copexists[];
extern const char	e_query[];
extern const char	e_history[];
extern const char	e_argtype[];
extern const char	e_create[];
extern const char	e_tmpcreate[];
extern const char	e_exists[];
extern const char	e_file[];
extern const char	e_redirect[];
extern const char	e_formspec[];
extern const char	e_badregexp[];
extern const char	e_open[];
extern const char	e_notseek[];
extern const char	e_readscript[];
extern const char	e_noread[];
extern const char	e_badseek[];
extern const char	e_badwrite[];
extern const char	e_badpattern[];
extern const char	e_toomany[];
extern const char	e_pipe[];
extern const char	e_unknown[];
extern const char	e_devnull[];
extern const char	e_profile[];
extern const char	e_sysprofile[];
#if SHOPT_SYSRC
extern const char	e_sysrc[];
#endif
extern const char	e_stdprompt[];
extern const char	e_supprompt[];
extern const char	e_ambiguous[];

#endif /* _io_h_defined */

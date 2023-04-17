/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2023 Contributors to ksh 93u+m           *
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

#ifndef SH_VALNOD

#include        <option.h>
#include        "FEATURE/dynamic"
#include	<nval.h>

/* used for RANDNOD ($RANDOM) */
struct rand
{
	Namfun_t	hdr;
	unsigned int	rand_seed;
	int32_t		rand_last;
};
extern void sh_reseed_rand(struct rand *);
extern void sh_save_rand_seed(struct rand *, int);

#define RAND_SEED_INVALIDATED		-2
#define sh_invalidate_rand_seed()	(((struct rand*)RANDNOD->nvfun)->rand_last = RAND_SEED_INVALIDATED)

/* update ${.sh.level} and, if needed, restore the current scope */
#define update_sh_level() \
( \
	SH_LEVELNOD->nvalue.s = sh.fn_depth + sh.dot_depth, \
	sh.topscope != (Shscope_t*)sh.st.self ? sh_setscope(sh.topscope) : 0, \
	1 \
)

/* The following defines must be kept synchronous with shtab_variables[] in data/variables.c */

#define	PATHNOD		(sh.bltin_nodes)
#define PS1NOD		(sh.bltin_nodes+1)
#define PS2NOD		(sh.bltin_nodes+2)
#define IFSNOD		(sh.bltin_nodes+3)
#define PWDNOD		(sh.bltin_nodes+4)
#define HOME		(sh.bltin_nodes+5)
#define MAILNOD		(sh.bltin_nodes+6)
#define REPLYNOD	(sh.bltin_nodes+7)
#define SHELLNOD	(sh.bltin_nodes+8)
#define EDITNOD		(sh.bltin_nodes+9)
#define MCHKNOD		(sh.bltin_nodes+10)
#define RANDNOD		(sh.bltin_nodes+11)
#define ENVNOD		(sh.bltin_nodes+12)
#define HISTFILE	(sh.bltin_nodes+13)
#define HISTSIZE	(sh.bltin_nodes+14)
#define HISTEDIT	(sh.bltin_nodes+15)
#define HISTCUR		(sh.bltin_nodes+16)
#define FCEDNOD		(sh.bltin_nodes+17)
#define CDPNOD		(sh.bltin_nodes+18)
#define MAILPNOD	(sh.bltin_nodes+19)
#define PS3NOD		(sh.bltin_nodes+20)
#define OLDPWDNOD	(sh.bltin_nodes+21)
#define VISINOD		(sh.bltin_nodes+22)
#define COLUMNS		(sh.bltin_nodes+23)
#define LINES		(sh.bltin_nodes+24)
#define PPIDNOD		(sh.bltin_nodes+25)
#define L_ARGNOD	(sh.bltin_nodes+26)
#define TMOUTNOD	(sh.bltin_nodes+27)
#define SECONDS		(sh.bltin_nodes+28)
#define LINENO		(sh.bltin_nodes+29)
#define OPTARGNOD	(sh.bltin_nodes+30)
#define OPTINDNOD	(sh.bltin_nodes+31)
#define PS4NOD		(sh.bltin_nodes+32)
#define FPATHNOD	(sh.bltin_nodes+33)
#define LANGNOD		(sh.bltin_nodes+34)
#define LCALLNOD	(sh.bltin_nodes+35)
#define LCCOLLNOD	(sh.bltin_nodes+36)
#define LCTYPENOD	(sh.bltin_nodes+37)
#define LCMSGNOD	(sh.bltin_nodes+38)
#define LCNUMNOD	(sh.bltin_nodes+39)
#define LCTIMENOD	(sh.bltin_nodes+40)
#define FIGNORENOD	(sh.bltin_nodes+41)
#define VERSIONNOD	(sh.bltin_nodes+42)
#define JOBMAXNOD	(sh.bltin_nodes+43)
#define DOTSHNOD	(sh.bltin_nodes+44)
#define ED_CHRNOD	(sh.bltin_nodes+45)
#define ED_COLNOD	(sh.bltin_nodes+46)
#define ED_TXTNOD	(sh.bltin_nodes+47)
#define ED_MODENOD	(sh.bltin_nodes+48)
#define SH_NAMENOD	(sh.bltin_nodes+49)
#define SH_SUBSCRNOD	(sh.bltin_nodes+50)
#define SH_VALNOD	(sh.bltin_nodes+51)
#define SH_VERSIONNOD	(sh.bltin_nodes+52)
#define SH_MATCHNOD	(sh.bltin_nodes+53)
#define SH_COMMANDNOD	(sh.bltin_nodes+54)
#define SH_PATHNAMENOD	(sh.bltin_nodes+55)
#define SH_FUNNAMENOD	(sh.bltin_nodes+56)
#define SH_SUBSHELLNOD	(sh.bltin_nodes+57)
#define SH_LEVELNOD	(sh.bltin_nodes+58)
#define SH_LINENO	(sh.bltin_nodes+59)
#define SH_STATS	(sh.bltin_nodes+60)
#define SH_MATHNOD	(sh.bltin_nodes+61)
#define SH_PIDNOD	(sh.bltin_nodes+62)
#define SH_PPIDNOD	(sh.bltin_nodes+63)
#define SH_TILDENOD	(sh.bltin_nodes+64)
#define SHLVL		(sh.bltin_nodes+65)

#endif /* SH_VALNOD */

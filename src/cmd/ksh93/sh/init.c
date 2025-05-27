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
*         hyenias <58673227+hyenias@users.noreply.github.com>          *
*                 Harald van Dijk <harald@gigawatt.nl>                 *
*               K. Eugene Carlson <kvngncrlsn@gmail.com>               *
*             dnewhall <dnewhall@users.noreply.github.com>             *
*                                                                      *
***********************************************************************/
/*
 *
 * Shell initialization
 *
 *   David Korn
 *   AT&T Labs
 *
 */

#include	"shopt.h"
#include        "defs.h"
#include        <pwd.h>
#include        <tmx.h>
#include        <regex.h>
#include	<math.h>
#include	<ast_random.h>
#include        "variables.h"
#include        "path.h"
#include        "fault.h"
#include        "name.h"
#include	"edit.h"
#include	"jobs.h"
#include	"io.h"
#include	"shlex.h"
#include	"builtins.h"
#include	"FEATURE/time"
#include	"FEATURE/externs"
#include	"lexstates.h"
#include	"version.h"

#ifdef BUILD_DTKSH
#include <Dt/DtNlUtils.h>
#include <Dt/EnvControlP.h>
#include <X11/X.h>
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>
#include <X11/StringDefs.h>
#include <Xm/XmStrDefs.h>
#include <Xm/Xm.h>
#include <Xm/Protocols.h>
#include "dtksh.h"
#include "xmksh.h"
#include "dtkcmds.h"
#include "xmcvt.h"
#include "widget.h"
#include "extra.h"
#include "xmwidgets.h"
#include "msgs.h"
#endif /* BUILD_DTKSH */

#if _hdr_wctype
#include	<ast_wchar.h>
#include	<wctype.h>
#endif
#if !_typ_wctrans_t
#undef	wctrans_t
#define wctrans_t	sh_wctrans_t
typedef long wctrans_t;
#endif
#if !_lib_wctrans
#undef	wctrans
#define wctrans		sh_wctrans
static wctrans_t wctrans(const char *name)
{
	if(strcmp(name,e_tolower)==0)
		return 1;
	else if(strcmp(name,e_toupper)==0)
		return 2;
	return 0;
}
#endif
#if !_lib_towctrans
#undef	towctrans
#define towctrans	sh_towctrans
static int towctrans(int c, wctrans_t t)
{
#if _lib_towupper && _lib_towlower
	if(mbwide())
	{
		if(t==1 && iswupper((wint_t)c))
			c = (int)towlower((wint_t)c);
		else if(t==2 && iswlower((wint_t)c))
			c = (int)towupper((wint_t)c);
	}
	else
#endif
	if(t==1 && isupper(c))
		c = tolower(c);
	else if(t==2 && islower(c))
		c = toupper(c);
	return c;
}
#endif

char e_version[]	= "\n@(#)$Id: Version "
#if SHOPT_AUDIT
#define ATTRS		1
			"A"
#endif
#if SHOPT_BGX
#define ATTRS		1
			"J"
#endif
#if SHOPT_ACCT
#define ATTRS		1
			"L"
#endif
#if SHOPT_MULTIBYTE
#define ATTRS		1
			"M"
#endif
#if SHOPT_SCRIPTONLY
#define ATTRS		1
			"s"
#endif
#if ATTRS
			" "
#endif
			SH_RELEASE " $\0\n";

#define RANDMASK	0x7fff

#ifndef CHILD_MAX
#   define CHILD_MAX	(1*1024)
#endif
#ifndef CLK_TCK
#   define CLK_TCK	60
#endif /* CLK_TCK */

#ifndef environ
    extern char	**environ;
#endif

struct ifs
{
	Namfun_t	hdr;
	Namval_t	*ifsnp;
};

struct match
{
	Namfun_t	hdr;
	const char	*v;
	char		*val;
	char		*rval[2];
	int		*match;
	char		*nodes;
	char		*names;
	int		first;
	int		vsize;
	int		vlen;
	int		msize;
	int		nmatch;
	int		index;
	int		lastsub[2];
};

typedef struct _init_
{
	struct ifs	IFS_init;
	Namfun_t	PATH_init;
	Namfun_t	FPATH_init;
	Namfun_t	CDPATH_init;
	Namfun_t	SHELL_init;
	Namfun_t	ENV_init;
#if SHOPT_VSH || SHOPT_ESH
	Namfun_t	VISUAL_init;
	Namfun_t	EDITOR_init;
#endif
	Namfun_t	HISTFILE_init;
	Namfun_t	HISTSIZE_init;
	Namfun_t	OPTINDEX_init;
	Namfun_t	SECONDS_init;
	struct rand	RAND_init;
	Namfun_t	SRAND_init;
	Namfun_t	LINENO_init;
	Namfun_t	L_ARG_init;
	Namfun_t	SH_VERSION_init;
	struct match	SH_MATCH_init;
	Namfun_t	SH_MATH_init;
	Namfun_t	LC_TYPE_init;
	Namfun_t	LC_TIME_init;
	Namfun_t	LC_NUM_init;
	Namfun_t	LC_COLL_init;
	Namfun_t	LC_MSG_init;
	Namfun_t	LC_ALL_init;
	Namfun_t	LANG_init;
} Init_t;

static int		lctype;
static int		nvars;
static void		env_init(void);
static Init_t		*nv_init(void);
#if SHOPT_STATS
static void		stat_init(void);
#endif

/*
 * Exception callback routine for stk(3) and sh_*alloc wrappers.
 */
static noreturn void *nomemory(size_t s)
{
	errormsg(SH_DICT, ERROR_SYSTEM|ERROR_PANIC, "out of memory (needed %zu bytes)", s);
	UNREACHABLE();
}

/*
 * The following are wrapper functions for memory allocation.
 * These functions will error out if the allocation fails.
 */
void *sh_malloc(size_t size)
{
	void *cp;
	cp = malloc(size);
	if(!cp)
		nomemory(size);
	return cp;
}

void *sh_realloc(void *ptr, size_t size)
{
	void *cp;
	cp = realloc(ptr, size);
	if(!cp)
	{
		if(ptr)
			free(ptr);
		nomemory(size);
	}
	return cp;
}

void *sh_calloc(size_t nmemb, size_t size)
{
	void *cp;
	cp = calloc(nmemb, size);
	if(!cp)
		nomemory(size);
	return cp;
}

char *sh_strdup(const char *s)
{
	char *dup;
	dup = strdup(s);
	if(!dup)
		nomemory(strlen(s)+1);
	return dup;
}

void *sh_memdup(const void *s, size_t n)
{
	void *dup;
	dup = memdup(s, n);
	if(!dup)
		nomemory(n);
	return dup;
}

char *sh_getcwd(void)
{
	char *cwd;
	cwd = getcwd(NULL, 0);
	if(!cwd && errno==ENOMEM)
		nomemory(PATH_MAX);
	return cwd;
}

#if SHOPT_VSH || SHOPT_ESH
/* Trap for VISUAL and EDITOR variables */
static void put_ed(Namval_t *np,const char *val,int flags,Namfun_t *fp)
{
	const char *cp, *name=nv_name(np);
	int	newopt=0;
	if(*name=='E' && nv_getval(sh_scoped(VISINOD)))
		goto done;
	if(!(cp=val) && (*name=='E' || !(cp=nv_getval(sh_scoped(EDITNOD)))))
		goto done;
	/* turn on vi or emacs option if editor name is either */
	cp = path_basename(cp);
#if SHOPT_VSH
	if(strmatch(cp,"*[Vv][Ii]*"))
		newopt=SH_VI;
#endif
#if SHOPT_VSH && SHOPT_ESH
	else
#endif
#if SHOPT_ESH
	     if(strmatch(cp,"*gmacs*"))
		newopt=SH_GMACS;
	else if(strmatch(cp,"*macs*"))
		newopt=SH_EMACS;
#endif
	if(newopt)
	{
#if SHOPT_VSH
		sh_offoption(SH_VI);
#endif
#if SHOPT_ESH
		sh_offoption(SH_EMACS);
		sh_offoption(SH_GMACS);
#endif
		sh_onoption(newopt);
	}
done:
	nv_putv(np, val, flags, fp);
}
#endif /* SHOPT_VSH || SHOPT_ESH */

/* Trap for HISTFILE and HISTSIZE variables */
static void put_history(Namval_t *np,const char *val,int flags,Namfun_t *fp)
{
	void 	*histopen = sh.hist_ptr;
	char	*cp;
	if(val && histopen)
	{
		if(np==HISTFILE && (cp=nv_getval(np)) && strcmp(val,cp)==0)
			return;
		if(np==HISTSIZE && sh_arith(val)==nv_getnum(HISTSIZE))
			return;
		hist_close(sh.hist_ptr);
	}
	nv_putv(np, val, flags, fp);
	if(histopen)
	{
		if(val)
			sh_histinit();
		else
			hist_close(histopen);
	}
}

/* Trap for OPTINDEX */
static void put_optindex(Namval_t *np,const char *val,int flags,Namfun_t *fp)
{
	sh.st.opterror = sh.st.optchar = 0;
	nv_putv(np, val, flags, fp);
	if(!val)
		nv_disc(np,fp,NV_POP);
}

static Sfdouble_t nget_optindex(Namval_t *np, Namfun_t *fp)
{
	int32_t *lp = np->nvalue;
	NOT_USED(fp);
	return (Sfdouble_t)*lp;
}

static Namfun_t *clone_optindex(Namval_t *np, Namval_t *mp, int flags, Namfun_t *fp)
{
	Namfun_t *dp = (Namfun_t*)sh_malloc(sizeof(Namfun_t));
	NOT_USED(flags);
	memcpy(dp,fp,sizeof(Namfun_t));
	mp->nvalue = np->nvalue;
	dp->nofree = 0;
	return dp;
}


/* Trap for restricted variables FPATH, PATH, SHELL, ENV */
static void put_restricted(Namval_t *np,const char *val,int flags,Namfun_t *fp)
{
	int	path_scoped = 0, fpath_scoped=0;
	char	*name = nv_name(np);
	if(!(flags&NV_RDONLY) && sh_isoption(SH_RESTRICTED))
	{
		errormsg(SH_DICT,ERROR_exit(1),e_restricted,nv_name(np));
		UNREACHABLE();
	}
	if(np==PATHNOD	|| (path_scoped=(strcmp(name,PATHNOD->nvname)==0)))
	{
		/* Clear the hash table */
		nv_scan(sh_subtracktree(1),nv_rehash,NULL,NV_TAGGED,NV_TAGGED);
		if(path_scoped && !val)
			val = PATHNOD->nvalue;
	}
	if(val && !(flags&NV_RDONLY) && np->nvalue && strcmp(val,np->nvalue)==0)
		 return;
	if(np==FPATHNOD	|| (fpath_scoped=(strcmp(name,FPATHNOD->nvname)==0)))
		sh.pathlist = path_unsetfpath();
	nv_putv(np, val, flags, fp);
	sh.universe = 0;
	if(sh.pathlist)
	{
		val = np->nvalue;
		if(np==PATHNOD || path_scoped)
			sh.pathlist = path_addpath((Pathcomp_t*)sh.pathlist,val,PATH_PATH);
		else if(val && (np==FPATHNOD || fpath_scoped))
			sh.pathlist = path_addpath((Pathcomp_t*)sh.pathlist,val,PATH_FPATH);
		else
			return;
		if(!val && (flags&NV_NOSCOPE))
		{
			Namval_t *mp = dtsearch(sh.var_tree,np);
			if(mp && (val=nv_getval(mp)))
				nv_putval(mp,val,NV_RDONLY);
		}
	}
}

static void put_cdpath(Namval_t *np,const char *val,int flags,Namfun_t *fp)
{
	nv_putv(np, val, flags, fp);
	if(!sh.cdpathlist)
		return;
	val = np->nvalue;
	sh.cdpathlist = path_addpath((Pathcomp_t*)sh.cdpathlist,val,PATH_CDPATH);
}

static struct put_lang_defer_s
{
	Namval_t *np;
	const char *val;
	int flags;
	Namfun_t *fp;
	struct put_lang_defer_s *next;
} *put_lang_defer;

/* Trap for the LC_* and LANG variables */
static void put_lang(Namval_t *np,const char *val,int flags,Namfun_t *fp)
{
	int type;
	char *name;
	if (val && sh_isstate(SH_INIT) && !sh_isstate(SH_LCINIT))
	{
		/* defer setting locale while importing environment */
		struct put_lang_defer_s *p = sh_malloc(sizeof(struct put_lang_defer_s));
		p->np = np;
		p->val = val;
		p->flags = flags;
		p->fp = fp;
		p->next = put_lang_defer;
		put_lang_defer = p;
		return;
	}
	name = nv_name(np);
	if(name==(LCALLNOD)->nvname)
		type = LC_ALL;
	else if(name==(LCTYPENOD)->nvname)
		type = LC_CTYPE;
	else if(name==(LCMSGNOD)->nvname)
		type = LC_MESSAGES;
	else if(name==(LCCOLLNOD)->nvname)
		type = LC_COLLATE;
	else if(name==(LCNUMNOD)->nvname)
		type = LC_NUMERIC;
	else if(name==(LCTIMENOD)->nvname)
		type = LC_TIME;
#ifdef LC_LANG
	else if(name==(LANGNOD)->nvname)
		type = LC_LANG;
#else
#define LC_LANG		LC_ALL
	else if(name==(LANGNOD)->nvname && (!(name=nv_getval(LCALLNOD)) || !*name))
		type = LC_LANG;
#endif
	else
		type= -1;
	if(type>=0 || type==LC_ALL || type==LC_NUMERIC || type==LC_LANG)
	{
		char*		r;
		ast.locale.set |= AST_LC_setenv;
		r = setlocale(type,val?val:"");
		ast.locale.set ^= AST_LC_setenv;
		if(!r && val)
		{
			if(!sh_isstate(SH_INIT) || !sh_isoption(SH_LOGIN_SHELL))
				errormsg(SH_DICT,0,e_badlocale,val);
			return;
		}
	}
	nv_putv(np, val, flags, fp);
#if _lib_localeconv
	if(type==LC_ALL || type==LC_NUMERIC || type==LC_LANG)
	{
		struct lconv *lp = localeconv();
		char *cp = lp->decimal_point;
		/* Multibyte radix points are not (yet?) supported */
		sh.radixpoint = strlen(cp)==1 ? *cp : '.';
	}
#endif
}

/* Trap for IFS assignment and invalidates state table */
static void put_ifs(Namval_t *np,const char *val,int flags,Namfun_t *fp)
{
	struct ifs *ip = (struct ifs*)fp;
	ip->ifsnp = 0;
	if(!val)
	{
		fp = nv_stack(np, NULL);
		if(fp && !fp->nofree)
		{
			free(fp);
			fp = 0;
		}
	}
	if(val != np->nvalue)
		nv_putv(np, val, flags, fp);
	if(!val)
	{
		if(fp)
			fp->next = np->nvfun;
		np->nvfun = fp;
	}
}

/*
 * This is the lookup function for IFS
 * It keeps the sh.ifstable up to date
 */
static char* get_ifs(Namval_t *np, Namfun_t *fp)
{
	struct ifs *ip = (struct ifs*)fp;
	char *cp, *value;
	int c,n;
	value = nv_getv(np,fp);
	if(np!=ip->ifsnp)
	{
		ip->ifsnp = np;
		memset(sh.ifstable,0,(1<<CHAR_BIT));
		if(cp=value)
		{
			while(n = mbsize(cp), c = *(unsigned char*)cp++)
			{
				if(n>1)
				{
					cp += (n-1);
					sh.ifstable[c] = S_MBYTE;
					continue;
				}
				n = S_DELIM;
				/* Treat a repeated char as S_DELIM even if whitespace, unless --posix is on */
				if(c==*cp && !sh_isoption(SH_POSIX))
					cp++;
				else if(c=='\n')
					n = S_NL;
				else if(isspace(c))
					n = S_SPACE;
				sh.ifstable[c] = n;
			}
		}
		else
		{
			sh.ifstable[' '] = sh.ifstable['\t'] = S_SPACE;
			sh.ifstable['\n'] = S_NL;
		}
		sh.ifstable[0] = S_EOF;
	}
	return value;
}

/*
 * these functions are used to get and set the SECONDS variable
 */
#define dtime(tp) ((double)((tp)->tv_sec)+1e-6*((double)((tp)->tv_usec)))
#define tms	timeval

static void put_seconds(Namval_t *np,const char *val,int flags,Namfun_t *fp)
{
	double d;
	double *dp = np->nvalue;
	struct tms tp;
	if(!val)
	{
		nv_putv(np, val, flags, fp);
		fp = nv_stack(np, NULL);
		if(fp && !fp->nofree)
			free(fp);
		return;
	}
	if(!dp)
	{
		nv_setsize(np,3);
		nv_onattr(np,NV_DOUBLE);
		dp = np->nvalue = new_of(double,0);
	}
	nv_putv(np, val, flags, fp);
	d = *dp;
	timeofday(&tp);
	*dp = dtime(&tp)-d;
}

static char* get_seconds(Namval_t *np, Namfun_t *fp)
{
	int places = nv_size(np);
	struct tms tp;
	double d;
	double *dp = np->nvalue;
	double offset = dp ? *dp : 0;
	NOT_USED(fp);
	timeofday(&tp);
	d = dtime(&tp)- offset;
	sfprintf(sh.strbuf,"%.*f",places,d);
	return sfstruse(sh.strbuf);
}

static Sfdouble_t nget_seconds(Namval_t *np, Namfun_t *fp)
{
	struct tms tp;
	double *dp = np->nvalue;
	double offset = dp ? *dp : 0;
	NOT_USED(fp);
	timeofday(&tp);
	return dtime(&tp) - offset;
}

/*
 * Seeds the rand structure using the same algorithm as srand48()
 */
static void seed_rand_uint(struct rand *rp, unsigned int seed)
{
	rp->rand_seed[0] = 0x330e; /* Constant from POSIX spec. */
	rp->rand_seed[1] = (unsigned short)seed;
	rp->rand_seed[2] = (unsigned short)(seed >> 16);
}

/*
 * These four functions are used to get and set the RANDOM variable
 */
static void put_rand(Namval_t *np,const char *val,int flags,Namfun_t *fp)
{
	struct rand *rp = (struct rand*)fp;
	Sfdouble_t n;
	sh_save_rand_seed(rp, 0);
	if(!val)
	{
		fp = nv_stack(np, NULL);
		if(fp && !fp->nofree)
			free(fp);
		nv_unset(np,NV_RDONLY);
		return;
	}
	if(flags&NV_INTEGER)
		n = *(Sfdouble_t*)val;
	else
		n = sh_arith(val);
	seed_rand_uint(rp, (unsigned int)n);
	rp->rand_last = -1;
	if(!np->nvalue)
		np->nvalue = &rp->rand_last;
}

/*
 * get random number in range of 0 - 2**15
 * never pick same number twice in a row
 */
static Sfdouble_t nget_rand(Namval_t *np, Namfun_t *fp)
{
	struct rand *rp = (struct rand*)fp;
	int32_t cur;
	int32_t *lp = np->nvalue;
	int32_t last = *lp;
	sh_save_rand_seed(rp, 1);
	do
		/* don't use lower bits when generating large numbers */
		cur = (nrand48(rp->rand_seed) >> 3) & RANDMASK;
	while(cur==last);
	*lp = cur;
	return (Sfdouble_t)cur;
}

static char* get_rand(Namval_t *np, Namfun_t *fp)
{
	intmax_t n = (intmax_t)nget_rand(np,fp);
	return fmtint(n,1);
}

void sh_reseed_rand(struct rand *rp)
{
	seed_rand_uint(rp, arc4random());
	rp->rand_last = -1;
}

/*
 * The following three functions are for SRANDOM
 */

static void put_srand(Namval_t *np,const char *val,int flags,Namfun_t *fp)
{
	if(!val)  /* unset */
	{
		fp = nv_stack(np, NULL);
		if(fp && !fp->nofree)
			free(fp);
		nv_unset(np,NV_RDONLY);
		return;
	}
	if(sh_isstate(SH_INIT))
		return;
	if(flags&NV_INTEGER)
		sh.srand_upper_bound = *(Sfdouble_t*)val;
	else
		sh.srand_upper_bound = sh_arith(val);
}

static Sfdouble_t nget_srand(Namval_t *np, Namfun_t *fp)
{
	NOT_USED(np);
	NOT_USED(fp);
	return (Sfdouble_t)(sh.srand_upper_bound ? arc4random_uniform(sh.srand_upper_bound) : arc4random());
}

static char* get_srand(Namval_t *np, Namfun_t *fp)
{
	intmax_t n = (intmax_t)(sh.srand_upper_bound ? arc4random_uniform(sh.srand_upper_bound) : arc4random());
	NOT_USED(np);
	NOT_USED(fp);
	return fmtint(n,1);
}

/*
 * These three routines are for LINENO
 */
static Sfdouble_t nget_lineno(Namval_t *np, Namfun_t *fp)
{
	int d = 1;
	if(error_info.line >0)
		d = error_info.line;
	else if(error_info.context && error_info.context->line>0)
		d = error_info.context->line;
	NOT_USED(np);
	NOT_USED(fp);
	return (Sfdouble_t)d;
}

static void put_lineno(Namval_t *np,const char *val,int flags,Namfun_t *fp)
{
	Sfdouble_t n;
	if(!val)
	{
		fp = nv_stack(np, NULL);
		if(fp && !fp->nofree)
			free(fp);
		nv_unset(np,NV_RDONLY);
		return;
	}
	if(flags&NV_INTEGER)
		n = *(Sfdouble_t*)val;
	else
		n = sh_arith(val);
	sh.st.firstline += (int)(nget_lineno(np,fp) + 1 - n);
}

static char* get_lineno(Namval_t *np, Namfun_t *fp)
{
	intmax_t n = (intmax_t)nget_lineno(np,fp);
	return fmtint(n,1);
}

static char* get_lastarg(Namval_t *np, Namfun_t *fp)
{
	char	*cp;
	int	pid;
	NOT_USED(fp);
	if(sh_isstate(SH_INIT) && (cp=sh.lastarg) && *cp=='*' && (pid=strtol(cp+1,&cp,10)) && *cp=='*')
		nv_putval(np,cp+1,0);
	return sh.lastarg;
}

static void put_lastarg(Namval_t *np,const char *val,int flags,Namfun_t *fp)
{
	NOT_USED(fp);
	if(flags&NV_INTEGER)
	{
		sfprintf(sh.strbuf,"%.*Lg",12,*((Sfdouble_t*)val));
		val = sfstruse(sh.strbuf);
	}
	if(val)
		val = sh_strdup(val);
	if(sh.lastarg && !nv_isattr(np,NV_NOFREE))
		free(sh.lastarg);
	else
		nv_offattr(np,NV_NOFREE);
	sh.lastarg = (char*)val;
	nv_offattr(np,NV_EXPORT);
	np->nvmeta = NULL;
}

static void match2d(struct match *mp)
{
	Namval_t	*np;
	int		i;
	Namarr_t	*ap;
	nv_disc(SH_MATCHNOD, &mp->hdr, NV_POP);
	if(mp->nodes)
	{
		np = nv_namptr(mp->nodes, 0);
		for(i=0; i < mp->nmatch; i++)
		{
			np->nvname = mp->names + 3 * i;
			if(i > 9)
			{
				*np->nvname = '0' + i / 10;
				np->nvname[1] = '0' + (i % 10);
			}
			else
				*np->nvname = '0' + i;
			nv_putsub(np, NULL, 1);
			nv_putsub(np, NULL, 0);
			nv_putsub(SH_MATCHNOD, NULL, i);
			nv_arraychild(SH_MATCHNOD, np, 0);
			np = nv_namptr(np + 1, 0);
		}
	}
	if(ap = nv_arrayptr(SH_MATCHNOD))
		ap->nelem = mp->nmatch;
}

/*
 * store the most recent value for use in .sh.match
 * treat .sh.match as a two dimensional array
 */
void sh_setmatch(const char *v, int vsize, int nmatch, int match[], int index)
{
	Init_t		*ip = sh.init_context;
	struct match	*mp = &ip->SH_MATCH_init;
	int		i,n,x, savesub=sh.subshell;
	Namarr_t	*ap = nv_arrayptr(SH_MATCHNOD);
	Namval_t	*np;
	if(sh.intrace)
		return;
	sh.subshell = 0;
	if(index<0)
	{
		if(mp->nodes)
		{
			np = nv_namptr(mp->nodes,0);
			if(mp->index==0)
				match2d(mp);
			for(i=0; i < mp->nmatch; i++)
			{
				nv_disc(np,&mp->hdr,NV_LAST);
				nv_putsub(np,NULL,mp->index);
				for(x=mp->index; x >=0; x--)
				{
					n = i + x*mp->nmatch;
					if(mp->match[2*n+1]>mp->match[2*n])
						nv_putsub(np,Empty,ARRAY_ADD|x);
				}
				if((ap=nv_arrayptr(np)) && array_elem(ap)==0)
				{
					nv_putsub(SH_MATCHNOD,NULL,i);
					nv_unset(SH_MATCHNOD,NV_RDONLY);
				}
				np = nv_namptr(np+1,0);
			}
		}
		sh.subshell = savesub;
		return;
	}
	mp->index = index;
	if(index==0)
	{
		if(mp->nodes)
		{
			np = nv_namptr(mp->nodes,0);
			for(i=0; i < mp->nmatch; i++)
			{
				if(np->nvfun && np->nvfun != &mp->hdr)
				{
					free(np->nvfun);
					np->nvfun = 0;
				}
				np = nv_namptr(np+1,0);
			}
			free(mp->nodes);
			mp->nodes = 0;
		}
		mp->vlen = 0;
		if(ap && ap->hdr.next != &mp->hdr)
			free(ap);
		SH_MATCHNOD->nvalue = NULL;
		SH_MATCHNOD->nvfun = NULL;
		if(!(mp->nmatch=nmatch) || !v)
		{
			sh.subshell = savesub;
			return;
		}
		mp->nodes = sh_calloc(mp->nmatch*(NV_MINSZ+sizeof(void*)+3),1);
		mp->names = mp->nodes + mp->nmatch*(NV_MINSZ+sizeof(void*));
		np = nv_namptr(mp->nodes,0);
		nv_disc(SH_MATCHNOD,&mp->hdr,NV_LAST);
		for(i=nmatch; --i>=0;)
		{
			if(match[2*i]>=0)
				nv_putsub(SH_MATCHNOD,Empty,ARRAY_ADD|i);
		}
		mp->v = v;
		mp->first = match[0];
	}
	else
	{
		if(index==1)
			match2d(mp);
	}
	sh.subshell = savesub;
	if(mp->nmatch)
	{
		for(n=mp->first+(mp->v-v),vsize=0,i=0; i < 2*nmatch; i++)
		{
			if(match[i]>=0 && (match[i] - n) > vsize)
				vsize = match[i] -n;
		}
		index *= 2*mp->nmatch;
		i = (index+2*mp->nmatch)*sizeof(match[0]);
		if(i >= mp->msize)
			mp->match = sh_realloc(mp->match, mp->msize = 2*i);
		if(vsize >= mp->vsize)
			mp->val = sh_realloc(mp->val, mp->vsize = mp->vsize ? 2 * vsize : vsize + 1);
		memcpy(mp->match+index,match,nmatch*2*sizeof(match[0]));
		for(i=0; i < 2*nmatch; i++)
		{
			if(match[i]>=0)
				mp->match[index+i] -= n;
		}
		while(i < 2*mp->nmatch)
			mp->match[index+i++] = -1;
		if(index==0)
			v+= mp->first;
		memcpy(mp->val+mp->vlen,v,vsize-mp->vlen);
		mp->val[mp->vlen=vsize] = 0;
		mp->lastsub[0] = mp->lastsub[1] = -1;
	}
}

static char* get_match(Namval_t *np, Namfun_t *fp)
{
	struct match	*mp = (struct match*)fp;
	int		sub,sub2=0,n,i =!mp->index;
	char		*val;
	sub = nv_aindex(SH_MATCHNOD);
	if(sub<0)
		sub = 0;
	if(np!=SH_MATCHNOD)
		sub2 = nv_aindex(np);
	if(sub>=mp->nmatch)
		return NULL;
	if(sub2>0)
		sub += sub2*mp->nmatch;
	if(sub==mp->lastsub[!i])
		return mp->rval[!i];
	else if(sub==mp->lastsub[i])
		return mp->rval[i];
	n = mp->match[2*sub+1]-mp->match[2*sub];
	if(n<=0)
		return mp->match[2*sub]<0?Empty:"";
	val = mp->val+mp->match[2*sub];
	if(mp->val[mp->match[2*sub+1]]==0)
		return val;
	mp->index = i;
	if(mp->rval[i])
	{
		free(mp->rval[i]);
		mp->rval[i] = 0;
	}
	mp->rval[i] = (char*)sh_malloc(n+1);
	mp->lastsub[i] = sub;
	memcpy(mp->rval[i],val,n);
	mp->rval[i][n] = 0;
	return mp->rval[i];
}

static const Namdisc_t SH_MATCH_disc  = { sizeof(struct match), 0, get_match };

static char* get_version(Namval_t *np, Namfun_t *fp)
{
	return nv_getv(np,fp);
}

static Sfdouble_t nget_version(Namval_t *np, Namfun_t *fp)
{
	const char	*cp = e_version + sizeof e_version - 15;  /* version date */
	int		c;
	Sflong_t	t = 0;
	NOT_USED(np);
	NOT_USED(fp);
	while (c = *cp++)
		if (isdigit(c))
			t = 10 * t + c - '0';
	return (Sfdouble_t)t;
}

static const Namdisc_t SH_VERSION_disc	= {  0, 0, get_version, nget_version };


static const Namdisc_t IFS_disc		= {  sizeof(struct ifs), put_ifs, get_ifs };

/* Invalidate IFS state table */
void sh_invalidate_ifs(void)
{
	Namval_t *np = sh_scoped(IFSNOD);
	if(np)
	{
		struct ifs *ip = (struct ifs*)nv_hasdisc(np, &IFS_disc);
		if(ip)
			ip->ifsnp = 0;
	}
}

const Namdisc_t RESTRICTED_disc	= {  sizeof(Namfun_t), put_restricted };
static const Namdisc_t CDPATH_disc	= {  sizeof(Namfun_t), put_cdpath };
#if SHOPT_VSH || SHOPT_ESH
static const Namdisc_t EDITOR_disc	= {  sizeof(Namfun_t), put_ed };
#endif
static const Namdisc_t HISTFILE_disc	= {  sizeof(Namfun_t), put_history };
static const Namdisc_t OPTINDEX_disc	= {  sizeof(Namfun_t), put_optindex, 0, nget_optindex, 0, 0, clone_optindex };
static const Namdisc_t SECONDS_disc	= {  sizeof(Namfun_t), put_seconds, get_seconds, nget_seconds };
static const Namdisc_t RAND_disc	= {  sizeof(struct rand), put_rand, get_rand, nget_rand };
static const Namdisc_t SRAND_disc	= {  sizeof(Namfun_t), put_srand, get_srand, nget_srand };
static const Namdisc_t LINENO_disc	= {  sizeof(Namfun_t), put_lineno, get_lineno, nget_lineno };
static const Namdisc_t L_ARG_disc	= {  sizeof(Namfun_t), put_lastarg, get_lastarg };


#define MAX_MATH_ARGS	3

static char *name_math(Namval_t *np, Namfun_t *fp)
{
	NOT_USED(fp);
	sfprintf(sh.strbuf,".sh.math.%s",np->nvname);
	return sfstruse(sh.strbuf);
}

static const Namdisc_t	math_child_disc =
{
	0,0,0,0,0,0,0,
	name_math
};

static Namfun_t	 math_child_fun =
{
	&math_child_disc, 1, 0, sizeof(Namfun_t)
};

static void math_init(void)
{
	Namval_t	*np;
	char		*name;
	int		i;
	sh.mathnodes = (char*)sh_calloc(1,MAX_MATH_ARGS*(NV_MINSZ+5));
	name = sh.mathnodes+MAX_MATH_ARGS*NV_MINSZ;
	for(i=0; i < MAX_MATH_ARGS; i++)
	{
		np = nv_namptr(sh.mathnodes,i);
		np->nvfun = &math_child_fun;
		memcpy(name,"arg",3);
		name[3] = '1'+i;
		np->nvname = name;
		name+=5;
		nv_onattr(np,NV_MINIMAL|NV_NOFREE|NV_LDOUBLE|NV_RDONLY);
	}
}

static Namval_t *create_math(Namval_t *np,const char *name,int flag,Namfun_t *fp)
{
	NOT_USED(np);
	NOT_USED(flag);
	if(!name)
		return SH_MATHNOD;
	if(name[0]!='a' || name[1]!='r' || name[2]!='g' || name[4] || !isdigit(name[3]) || (name[3]=='0' || (name[3]-'0')>MAX_MATH_ARGS))
		return NULL;
	fp->last = (char*)&name[4];
	return nv_namptr(sh.mathnodes,name[3]-'1');
}

static char* get_math(Namval_t *np, Namfun_t *fp)
{
	Namval_t	*mp,fake;
	char		*val;
	int		first=0;
	NOT_USED(np);
	NOT_USED(fp);
	fake.nvname = ".sh.math.";
	mp = (Namval_t*)dtprev(sh.fun_tree,&fake);
	while(mp=(Namval_t*)dtnext(sh.fun_tree,mp))
	{
		if(strncmp(mp->nvname,".sh.math.",9))
			break;
		if(first++)
			sfputc(sh.strbuf,' ');
		sfputr(sh.strbuf,mp->nvname+9,-1);
	}
	val = sfstruse(sh.strbuf);
	return val;
}

static char *setdisc_any(Namval_t *np, const char *event, Namval_t *action, Namfun_t *fp)
{
	Namval_t	*mp,fake;
	char		*name;
	int		getname=0, off=stktell(sh.stk);
	NOT_USED(fp);
	fake.nvname = nv_name(np);
	if(!event)
	{
		if(!action)
		{
			mp = (Namval_t*)dtprev(sh.fun_tree,&fake);
			return (char*)dtnext(sh.fun_tree,mp);
		}
		getname = 1;
	}
	sfputr(sh.stk,fake.nvname,'.');
	sfputr(sh.stk,event,0);
	name = stkptr(sh.stk,off);
	mp = nv_search(name, sh.fun_tree, action?NV_ADD:0);
	stkseek(sh.stk,off);
	if(getname)
		return mp ? (char*)dtnext(sh.fun_tree,mp) : 0;
	if(action==np)
		action = mp;
	return action ? (char*)action : "";
}

static const Namdisc_t SH_MATH_disc  = { 0, 0, get_math, 0, setdisc_any, create_math, };

static const Namdisc_t LC_disc = {  sizeof(Namfun_t), put_lang };

/*
 * This function will get called whenever a configuration parameter changes
 */
static int newconf(const char *name, const char *path, const char *value)
{
	char *arg;
	NOT_USED(path);
	if(!name)
		setenviron(value);
	else if(strcmp(name,"UNIVERSE")==0 && strcmp(astconf(name,0,0),value))
	{
		sh.universe = 0;
		/* set directory in new universe */
		if(*(arg = path_pwd())=='/')
			chdir(arg);
		/* clear out old tracked alias */
		stkseek(sh.stk,0);
		sfputr(sh.stk,nv_getval(PATHNOD),0);
		nv_putval(PATHNOD,stkseek(sh.stk,0),NV_RDONLY);
	}
	return 1;
}

/*
 * return SH_TYPE_* bitmask for path
 * 0 for "not a shell"
 */
int sh_type(const char *path)
{
	const char*	s;
	int		t = 0;
	if (s = (const char*)strrchr(path, '/'))
	{
		if (*path == '-')
			t |= SH_TYPE_LOGIN;
		s++;
	}
	else
		s = path;
	if (*s == '-')
	{
		s++;
		t |= SH_TYPE_LOGIN;
	}
	for (;;)
	{
		if (!(t & SH_TYPE_KSH))
		{
			if (*s == 'k')
			{
				s++;
				t |= SH_TYPE_KSH;
				continue;
			}
		}
		if (!(t & SH_TYPE_RESTRICTED))
		{
			if (*s == 'r')
			{
				s++;
				t |= SH_TYPE_RESTRICTED;
				continue;
			}
		}
		break;
	}
#if _WINIX
	if (!(t & SH_TYPE_KSH) && *s == 's' && *(s+1) == 'h' && (!*(s+2) || *(s+2) == '.'))
#else
	if (!(t & SH_TYPE_KSH) && *s == 's' && *(s+1) == 'h' && !*(s+2))
#endif
		t |= SH_TYPE_POSIX;
	if (*s++ == 's' && (*s == 'h' || *s == 'u'))
	{
		s++;
		t |= SH_TYPE_SH;
		if ((t & SH_TYPE_KSH) && *s == '9' && *(s+1) == '3')
			s += 2;
#if _WINIX
		if (*s == '.' && *(s+1) == 'e' && *(s+2) == 'x' && *(s+3) == 'e')
			s += 4;
#endif
		if (!isalnum(*s))
			return t;
	}
	return t & ~(SH_TYPE_KSH|SH_TYPE_RESTRICTED);
}


/*
 * initialize the shell
 */
Shell_t *sh_init(int argc,char *argv[], Shinit_f userinit)
{
	int type = 0;
	static char *login_files[2];
	sh_onstate(SH_INIT);
	/* truncate final " $\0\n" from e_version for ${.sh.version} output (it's there for what(1) or ident(1)) */
	e_version[sizeof e_version - 5] = '\0';
	sh.current_pid = sh.pid = getpid();
	sh.current_ppid = sh.ppid = getppid();
	sh.userid = getuid();
	sh.euserid = geteuid();
	sh.groupid = getgid();
	sh.egroupid = getegid();
	sh.lim.child_max = (int)astconf_long(CONF_CHILD_MAX);
	sh.lim.clk_tck = (int)astconf_long(CONF_CLK_TCK);
	if(sh.lim.child_max <= 0)
		sh.lim.child_max = CHILD_MAX;
	if(sh.lim.clk_tck <= 0)
		sh.lim.clk_tck = CLK_TCK;
	sh.ed_context = ed_open();
	error_info.id = path_basename(argv[0]);
	umask(sh.mask = umask(0));
	sh.mac_context = sh_macopen();
	sh.arg_context = sh_argopen();
	sh.lex_context = sh_lexopen(0,1);
	sh.radixpoint = '.';  /* pre-locale init */
	sh.strbuf = sfstropen();
	stkoverflow(sh.stk = stkstd, nomemory);
	sfsetbuf(sh.strbuf,NULL,64);
	error_info.catalog = e_dict;
	sh.cpipe[0] = -1;
	sh.coutpipe = -1;
	sh_ioinit();
	/* initialize signal handling */
	sh_siginit();
	/* set up memory for name-value pairs */
	sh.init_context = nv_init();
	/* initialize shell type */
	if(argc>0)
	{
		type = sh_type(*argv);
		if(type&SH_TYPE_LOGIN)
			sh_onoption(SH_LOGIN_SHELL);
		if(type&SH_TYPE_POSIX)
		{
			sh_onoption(SH_POSIX);
			sh_onoption(SH_LETOCTAL);
		}
	}
	/* read the environment */
	env_init();
	if(!ENVNOD->nvalue)
	{
		sfprintf(sh.strbuf,"%s/.kshrc",nv_getval(HOME));
		nv_putval(ENVNOD,sfstruse(sh.strbuf),NV_RDONLY);
	}
	/* increase SHLVL */
	sh.shlvl++;
#if SHOPT_SPAWN
	{
		/*
		 * try to find the pathname for this interpreter
		 * try using environment variable _ or argv[0]
		 */
		char *cp=nv_getval(L_ARGNOD);
		char buff[PATH_MAX+1];
		size_t n;
		sh.shpath = 0;
		if((n = pathprog(NULL, buff, sizeof(buff))) > 0 && n <= sizeof(buff))
			sh.shpath = sh_strdup(buff);
		else if((cp && (sh_type(cp)&SH_TYPE_SH)) || (argc>0 && strchr(cp= *argv,'/')))
		{
			if(*cp=='/')
				sh.shpath = sh_strdup(cp);
			else if(cp = nv_getval(PWDNOD))
			{
				int offset = stktell(sh.stk);
				sfputr(sh.stk,cp,'/');
				sfputr(sh.stk,argv[0],-1);
				pathcanon(stkptr(sh.stk,offset),PATH_DOTDOT);
				sh.shpath = sh_strdup(stkptr(sh.stk,offset));
				stkseek(sh.stk,offset);
			}
		}
	}
#endif
	nv_putval(IFSNOD,(char*)e_sptbnl,NV_RDONLY);
	astconfdisc(newconf);
#if SHOPT_TIMEOUT
	sh.st.tmout = SHOPT_TIMEOUT;
#endif /* SHOPT_TIMEOUT */
	/* initialize jobs table */
	job_clear();
#if (SHOPT_ESH || SHOPT_VSH)
	sh_onoption(SH_MULTILINE);
#endif
	if(argc>0)
	{
		int dolv_index;
		/* check for restricted shell */
		if(type&SH_TYPE_RESTRICTED)
			sh_onoption(SH_RESTRICTED);
		/* look for options */
		/* sh.st.dolc is $#	*/
		if((sh.st.dolc = sh_argopts(-argc,argv)) < 0)
		{
			sh.exitval = 0;  /* self-doc was written to stdout */
			sh_done(0);
		}
		opt_info.disc = 0;
		dolv_index = (argc - 1) - sh.st.dolc;
		sh.st.dolv = argv + dolv_index;
		sh.st.repl_index = dolv_index;
		sh.st.repl_arg = argv[dolv_index];
		sh.st.dolv[0] = argv[0];
		if(sh.st.dolc < 1)
		{
			sh_onoption(SH_SFLAG);
			off_option(&sh.offoptions,SH_SFLAG);
		}
		if(!sh_isoption(SH_SFLAG))
		{
			sh.st.dolc--;
			sh.st.dolv++;
#if _WINIX
			{
				char*	name;
				name = sh.st.dolv[0];
				if(name[1]==':' && (name[2]=='/' || name[2]=='\\'))
				{
					char*	p;
					size_t	n;
					if((n = pathposix(name, NULL, 0)) > 0)
					{
						p = (char*)sh_malloc(++n);
						pathposix(name, p, n);
						name = p;
					}
					else
					{
						name[1] = name[0];
						name[0] = name[2] = '/';
					}
				}
			}
#endif /* _WINIX */
		}
	}
	/* set[ug]id scripts require the -p flag */
	if(sh.userid!=sh.euserid || sh.groupid!=sh.egroupid)
	{
#ifdef SHOPT_P_SUID
		/* require sh -p to run setuid and/or setgid */
		if(!sh_isoption(SH_PRIVILEGED) && sh.userid >= SHOPT_P_SUID)
		{
			setuid(sh.euserid=sh.userid);
			setgid(sh.egroupid=sh.groupid);
		}
		else
#endif /* SHOPT_P_SUID */
			sh_onoption(SH_PRIVILEGED);
	}
	else
		sh_offoption(SH_PRIVILEGED);
	/* shname for $0 in profiles and . scripts */
	if(sh_isdevfd(argv[1]))
		sh.shname = sh_strdup(argv[0]);
	else
		sh.shname = sh_strdup(sh.st.dolv[0]);
	/*
	 * return here for shell script execution
	 * but not for parenthesis subshells
	 */
	error_info.id = sh_strdup(sh.st.dolv[0]); /* error_info.id is $0 */
	sh.jmpbuffer = &sh.checkbase;
	sh_pushcontext(&sh.checkbase,SH_JMPSCRIPT);
	sh.st.self = &sh.global;
	sh.topscope = (Shscope_t*)sh.st.self;
	login_files[0] = (char*)e_profile;
	sh.login_files = login_files;
	sh.bltindata.version = SH_VERSION;
	sh.bltindata.shp = &sh;
	sh.bltindata.shrun = sh_run;
	sh.bltindata.shtrap = sh_trap;
	sh.bltindata.shexit = sh_exit;
	sh.bltindata.shbltin = sh_addbuiltin;
	sh.bltindata.shgetenv = sh_getenv;
	sh.bltindata.shsetenv = sh_setenviron;
	astintercept(&sh.bltindata,1);
	if(sh.userinit=userinit)
		(*userinit)(&sh, 0);
	error_info.exit = sh_exit;
#ifdef BUILD_DTKSH
	{
		int *lockedFds = LockKshFileDescriptors();
		(void) XtSetLanguageProc((XtAppContext)NULL, (XtLanguageProc)NULL, (XtPointer)NULL);
		DtNlInitialize();
		_DtEnvControl(DT_ENV_SET);
		UnlockKshFileDescriptors(lockedFds);
		dtksh_init();
	}
#endif /* BUILD_DTKSH */
	sh_offstate(SH_INIT);
	return &sh;
}

/*
 * Close all the dictionaries on a viewpath
 */
static void freeup_tree(Dt_t *tree)
{
	Dt_t	*view;
	while(view = dtview(tree, NULL))
	{
		dtclose(tree);
		tree = view;
	}
	dtclose(tree);
}

/*
 * Reinitialize before executing a script without a #! path.
 * This is done in a fork of the shell.
 */
void sh_reinit(void)
{
	Shopt_t opt;
	Namval_t *np,*npnext;
	Dt_t	*dp;
	sh_onstate(SH_INIT);
	sh_offstate(SH_FORKED);
	/* Reset shell options; inherit some */
	memset(&opt,0,sizeof(opt));
	if(sh_isoption(SH_POSIX))
		on_option(&opt,SH_POSIX);
#if SHOPT_ESH
	if(sh_isoption(SH_EMACS))
		on_option(&opt,SH_EMACS);
	if(sh_isoption(SH_GMACS))
		on_option(&opt,SH_GMACS);
#endif
#if SHOPT_VSH
	if(sh_isoption(SH_VI))
		on_option(&opt,SH_VI);
#endif
	sh.options = opt;
	/* Reset here-document */
	if(sh.heredocs)
	{
		sfclose(sh.heredocs);
		sh.heredocs = 0;
	}
	/* Reset arguments */
	if(sh.arglist)
		sh_argreset(sh.arglist,NULL);
	sh.shname = error_info.id = sh_strdup(sh.st.dolv[0]);
	/* Reset traps and signals */
	memset(sh.st.trapcom,0,(sh.st.trapmax+1)*sizeof(char*));
	sh_sigreset(0);
	sh.st.filename = sh_strdup(sh.lastarg);
	nv_delete(NULL, NULL, 0);
	job.exitval = 0;
	sh.inpipe = sh.outpipe = 0;
	job_clear();
	job.in_critical = 0;
	/* update $$, $PPID */
	sh.ppid = sh.current_ppid;
	sh.pid = sh.current_pid;
#if SHOPT_NAMESPACE
	if(sh.namespace)
	{
		dp=nv_dict(sh.namespace);
		if(dp==sh.var_tree)
			sh.var_tree = dtview(dp,0);
		nv_unset(sh.namespace,NV_RDONLY);
		sh.namespace = NULL;
	}
#endif /* SHOPT_NAMESPACE */
	/* Delete aliases */
	for(np = dtfirst(sh.alias_tree); np; np = npnext)
	{
		if((dp = sh.alias_tree)->walk)
			dp = dp->walk;	/* the dictionary in which the item was found */
		npnext = (Namval_t*)dtnext(sh.alias_tree,np);
		nv_unset(np,NV_RDONLY);
		nv_delete(np,dp,0);
	}
	/* Delete hash table entries */
	for(np = dtfirst(sh.track_tree); np; np = npnext)
	{
		if((dp = sh.track_tree)->walk)
			dp = dp->walk;	/* the dictionary in which the item was found */
		npnext = (Namval_t*)dtnext(sh.track_tree,np);
		nv_unset(np,NV_RDONLY);
		nv_delete(np,dp,0);
	}
	/* Delete types */
	for(np = dtfirst(sh.typedict); np; np = npnext)
	{
		if((dp = sh.typedict)->walk)
			dp = dp->walk;	/* the dictionary in which the item was found */
		npnext = (Namval_t*)dtnext(sh.typedict,np);
		nv_delete(np,dp,0);
	}
	/* Unset all variables (don't delete yet) */
	for(np = dtfirst(sh.var_tree); np; np = npnext)
	{
		int	nofree;
		npnext = (Namval_t*)dtnext(sh.var_tree,np);
		if(np==DOTSHNOD || np==L_ARGNOD)	/* TODO: unset these without crashing */
			continue;
		if(nv_isref(np))
			nv_unref(np);
		if(nv_isarray(np))
			nv_putsub(np,NULL,ARRAY_UNDEF);
		nofree = nv_isattr(np,NV_NOFREE);	/* note: returns bitmask, not boolean */
		nv_unset(np,NV_RDONLY);			/* also clears NV_NOFREE attr, if any */
		nv_setattr(np,nofree);
	}
	/* Delete functions and built-ins. Note: fun_tree has a viewpath to bltin_tree */
	for(np = dtfirst(sh.fun_tree); np; np = npnext)
	{
		if((dp = sh.fun_tree)->walk)
			dp = dp->walk;	/* the dictionary in which the item was found */
		npnext = (Namval_t*)dtnext(sh.fun_tree,np);
		if(dp==sh.bltin_tree)
		{
			if(np->nvalue)
				sh_addbuiltin(nv_name(np), np->nvalue, pointerof(1));
		}
		else if(is_afunction(np))
		{
			nv_unset(np,NV_RDONLY);
			nv_delete(np,dp,NV_FUNCTION);
		}
	}
	/* Delete all variables in a separate pass; this avoids 'no parent' errors while
	 * unsetting variables or discipline functions with dot names like .foo.bar */
	for(np = dtfirst(sh.var_tree); np; np = npnext)
	{
		if((dp = sh.var_tree)->walk)
			dp = dp->walk;	/* the dictionary in which the item was found */
		npnext = (Namval_t*)dtnext(sh.var_tree,np);
		/* cannot delete default variables */
		if(np >= sh.bltin_nodes && np < &sh.bltin_nodes[nvars])
			continue;
		nv_delete(np,dp,nv_isattr(np,NV_NOFREE));
	}
	/* Reset state for subshells, environment, job control, function calls and file descriptors */
	sh.subshell = sh.realsubshell = sh.comsub = sh.curenv = sh.jobenv = sh.inuse_bits = sh.fn_depth = sh.dot_depth = 0;
	sh.envlist = NULL;
	sh.last_root = NULL;
	/* Free up the dictionary trees themselves */
	freeup_tree(sh.fun_tree); /* includes sh.bltin_tree */
	freeup_tree(sh.alias_tree);
	freeup_tree(sh.track_tree);
	freeup_tree(sh.typedict);
	freeup_tree(sh.var_tree);
#if SHOPT_STATS
	free(sh.stats);
#endif
	/* Re-init variables, functions and built-ins */
	free(sh.bltin_cmds);
	free(sh.bltin_nodes);
	free(sh.mathnodes);
	free(sh.init_context);
	sh.init_context = nv_init();
	/* Re-import the environment (re-exported in exscript()) */
	env_init();
	/* Increase SHLVL */
	sh.shlvl++;
	/* call user init function, if any */
	if(sh.userinit)
		(*sh.userinit)(&sh, 1);
	sh_offstate(SH_INIT);
}

/*
 * set when creating a local variable of this name
 */
Namfun_t *nv_cover(Namval_t *np)
{
	if(np==IFSNOD || np==PATHNOD || np==SHELLNOD || np==FPATHNOD || np==CDPNOD || np==SECONDS || np==ENVNOD)
		return np->nvfun;
	if(np==LCALLNOD || np==LCTYPENOD || np==LCMSGNOD || np==LCCOLLNOD || np==LCNUMNOD || np==LCTIMENOD || np==LANGNOD)
		return np->nvfun;
	return NULL;
}

#if SHOPT_STATS
struct Stats
{
	Namfun_t	hdr;
	char		*nodes;
	int		numnodes;
	int		current;
};

static Namval_t *next_stat(Namval_t *np, Dt_t *root,Namfun_t *fp)
{
	struct Stats *sp = (struct Stats*)fp;
	NOT_USED(np);
	if(!root)
		sp->current = 0;
	else if(++sp->current>=sp->numnodes)
		return NULL;
	return nv_namptr(sp->nodes,sp->current);
}

static Namval_t *create_stat(Namval_t *np,const char *name,int flag,Namfun_t *fp)
{
	struct Stats		*sp = (struct Stats*)fp;
	const char		*cp=name;
	int			i=0,n;
	Namval_t		*nq=0;
	NOT_USED(flag);
	if(!name)
		return SH_STATS;
	while((i=*cp++) && i != '=' && i != '+' && i!='[');
	n = (cp-1) -name;
	for(i=0; i < sp->numnodes; i++)
	{
		nq = nv_namptr(sp->nodes,i);
		if((n==0||strncmp(name,nq->nvname,n)==0) && nq->nvname[n]==0)
			goto found;
	}
	nq = 0;
found:
	if(nq)
	{
		fp->last = (char*)&name[n];
		sh.last_table = SH_STATS;
	}
	else
	{
		errormsg(SH_DICT,ERROR_exit(1),e_notelem,n,name,nv_name(np));
		UNREACHABLE();
	}
	return nq;
}

static const Namdisc_t stat_disc =
{
	0, 0, 0, 0, 0,
	create_stat,
	0, 0,
	next_stat
};

static char *name_stat(Namval_t *np, Namfun_t *fp)
{
	NOT_USED(fp);
	sfprintf(sh.strbuf,".sh.stats.%s",np->nvname);
	return sfstruse(sh.strbuf);
}

static const Namdisc_t	stat_child_disc =
{
	0,0,0,0,0,0,0,
	name_stat
};

static Namfun_t	 stat_child_fun =
{
	&stat_child_disc, 1, 0, sizeof(Namfun_t)
};

static void stat_init(void)
{
	int		i,nstat = STAT_SUBSHELL+1;
	size_t		extrasize = nstat*(sizeof(int)+NV_MINSZ);
	struct Stats	*sp = sh_newof(0,struct Stats,1,extrasize);
	Namval_t	*np;
	sp->numnodes = nstat;
	sp->nodes = (char*)(sp+1);
	sh.stats = (int*)sh_calloc(sizeof(int),nstat);
	for(i=0; i < nstat; i++)
	{
		np = nv_namptr(sp->nodes,i);
		np->nvfun = &stat_child_fun;
		np->nvname = (char*)shtab_stats[i].sh_name;
		nv_onattr(np,NV_RDONLY|NV_MINIMAL|NV_NOFREE|NV_INTEGER);
		nv_setsize(np,10);
		np->nvalue = &sh.stats[i];
	}
	sp->hdr.dsize = sizeof(struct Stats) + extrasize;
	sp->hdr.disc = &stat_disc;
	nv_stack(SH_STATS,&sp->hdr);
	sp->hdr.nofree = 1;
	nv_setvtree(SH_STATS);
}
#endif /* SHOPT_STATS */

/*
 * Initialize the shell name and alias table
 */
static Init_t *nv_init(void)
{
	Sfdouble_t d=0;
	Init_t *ip = sh_newof(0,Init_t,1,0);
	sh.nvfun.last = (char*)&sh;
	sh.nvfun.nofree = 1;
	sh.var_base = sh.var_tree = sh_inittree(shtab_variables);
	SHLVL->nvalue = &sh.shlvl;
	ip->IFS_init.hdr.disc = &IFS_disc;
	ip->PATH_init.disc = &RESTRICTED_disc;
	ip->PATH_init.nofree = 1;
	ip->FPATH_init.disc = &RESTRICTED_disc;
	ip->FPATH_init.nofree = 1;
	ip->CDPATH_init.disc = &CDPATH_disc;
	ip->CDPATH_init.nofree = 1;
	ip->SHELL_init.disc = &RESTRICTED_disc;
	ip->SHELL_init.nofree = 1;
	ip->ENV_init.disc = &RESTRICTED_disc;
	ip->ENV_init.nofree = 1;
#if SHOPT_VSH || SHOPT_ESH
	ip->VISUAL_init.disc = &EDITOR_disc;
	ip->VISUAL_init.nofree = 1;
	ip->EDITOR_init.disc = &EDITOR_disc;
	ip->EDITOR_init.nofree = 1;
#endif
	ip->HISTFILE_init.disc = &HISTFILE_disc;
	ip->HISTFILE_init.nofree = 1;
	ip->HISTSIZE_init.disc = &HISTFILE_disc;
	ip->HISTSIZE_init.nofree = 1;
	ip->OPTINDEX_init.disc = &OPTINDEX_disc;
	ip->OPTINDEX_init.nofree = 1;
	ip->SECONDS_init.disc = &SECONDS_disc;
	ip->SECONDS_init.nofree = 1;
	ip->RAND_init.hdr.disc = &RAND_disc;
	ip->RAND_init.hdr.nofree = 1;
	ip->SRAND_init.disc = &SRAND_disc;
	ip->SRAND_init.nofree = 1;
	ip->SH_MATCH_init.hdr.disc = &SH_MATCH_disc;
	ip->SH_MATCH_init.hdr.nofree = 1;
	ip->SH_MATH_init.disc = &SH_MATH_disc;
	ip->SH_MATH_init.nofree = 1;
	ip->SH_VERSION_init.disc = &SH_VERSION_disc;
	ip->SH_VERSION_init.nofree = 1;
	ip->LINENO_init.disc = &LINENO_disc;
	ip->LINENO_init.nofree = 1;
	ip->L_ARG_init.disc = &L_ARG_disc;
	ip->L_ARG_init.nofree = 1;
	ip->LC_TYPE_init.disc = &LC_disc;
	ip->LC_TYPE_init.nofree = 1;
	ip->LC_TIME_init.disc = &LC_disc;
	ip->LC_TIME_init.nofree = 1;
	ip->LC_NUM_init.disc = &LC_disc;
	ip->LC_NUM_init.nofree = 1;
	ip->LC_COLL_init.disc = &LC_disc;
	ip->LC_COLL_init.nofree = 1;
	ip->LC_MSG_init.disc = &LC_disc;
	ip->LC_MSG_init.nofree = 1;
	ip->LC_ALL_init.disc = &LC_disc;
	ip->LC_ALL_init.nofree = 1;
	ip->LANG_init.disc = &LC_disc;
	ip->LANG_init.nofree = 1;
	nv_stack(IFSNOD, &ip->IFS_init.hdr);
	ip->IFS_init.hdr.nofree = 1;
	nv_stack(PATHNOD, &ip->PATH_init);
	nv_stack(FPATHNOD, &ip->FPATH_init);
	nv_stack(CDPNOD, &ip->CDPATH_init);
	nv_stack(SHELLNOD, &ip->SHELL_init);
	nv_stack(ENVNOD, &ip->ENV_init);
#if SHOPT_VSH || SHOPT_ESH
	nv_stack(VISINOD, &ip->VISUAL_init);
	nv_stack(EDITNOD, &ip->EDITOR_init);
#endif
	nv_stack(HISTFILE, &ip->HISTFILE_init);
	nv_stack(HISTSIZE, &ip->HISTSIZE_init);
	nv_stack(OPTINDNOD, &ip->OPTINDEX_init);
	nv_stack(SECONDS, &ip->SECONDS_init);
	nv_stack(L_ARGNOD, &ip->L_ARG_init);
	nv_putval(SECONDS, (char*)&d, NV_DOUBLE);
	nv_stack(RANDNOD, &ip->RAND_init.hdr);
	nv_putval(RANDNOD, (char*)&d, NV_DOUBLE);
	nv_stack(SRANDNOD, &ip->SRAND_init);
	sh_invalidate_rand_seed();
	nv_stack(LINENO, &ip->LINENO_init);
	SH_MATCHNOD->nvfun =  &ip->SH_MATCH_init.hdr;
	nv_putsub(SH_MATCHNOD,NULL,10);
	nv_stack(SH_MATHNOD, &ip->SH_MATH_init);
	nv_stack(SH_VERSIONNOD, &ip->SH_VERSION_init);
	nv_stack(LCTYPENOD, &ip->LC_TYPE_init);
	nv_stack(LCALLNOD, &ip->LC_ALL_init);
	nv_stack(LCMSGNOD, &ip->LC_MSG_init);
	nv_stack(LCCOLLNOD, &ip->LC_COLL_init);
	nv_stack(LCNUMNOD, &ip->LC_NUM_init);
	nv_stack(LCTIMENOD, &ip->LC_TIME_init);
	nv_stack(LANGNOD, &ip->LANG_init);
	PPIDNOD->nvalue = &sh.ppid;
	SH_PIDNOD->nvalue = &sh.current_pid;
	SH_PPIDNOD->nvalue = &sh.current_ppid;
	SH_SUBSHELLNOD->nvalue = &sh.realsubshell;
	TMOUTNOD->nvalue = &sh.st.tmout;
	MCHKNOD->nvalue = &sh_mailchk;
	OPTINDNOD->nvalue = &sh.st.optindex;
	SH_LEVELNOD->nvalue = &sh.level;
	sh.alias_tree = dtopen(&_Nvdisc,Dtoset);
	sh.track_tree = dtopen(&_Nvdisc,Dtset);
	sh.bltin_tree = sh_inittree((const struct shtable2*)shtab_builtins);
	sh.fun_base = sh.fun_tree = dtopen(&_Nvdisc,Dtoset);
	dtview(sh.fun_tree,sh.bltin_tree);
	nv_mount(DOTSHNOD, "type", sh.typedict=dtopen(&_Nvdisc,Dtoset));
	DOTSHNOD->nvalue = Empty;
	nv_onattr(DOTSHNOD,NV_RDONLY);
	SH_LINENO->nvalue = &sh.st.lineno;
	/* init KSH_VERSION as a nameref to .sh.version */
	{
		struct Namref *nrp;
		VERSIONNOD->nvalue = nrp = sh_newof(0,struct Namref,1,0);
	        nrp->np = SH_VERSIONNOD;
	        nrp->root = nv_dict(DOTSHNOD);
	        nrp->table = DOTSHNOD;
		nv_onattr(VERSIONNOD,NV_REF);
	}
	math_init();
#if SHOPT_STATS
	stat_init();
#endif
	return ip;
}

/*
 * initialize name-value pairs
 */
Dt_t *sh_inittree(const struct shtable2 *name_vals)
{
	Namval_t *np;
	const struct shtable2 *tp;
	unsigned n = 0;
	Dt_t *treep;
	Dt_t *base_treep, *dict = 0;
	for(tp=name_vals;*tp->sh_name;tp++)
		n++;
	np = (Namval_t*)sh_calloc(n,sizeof(Namval_t));
	if(name_vals==shtab_variables)
	{
		sh.bltin_nodes = np;
		nvars = n;
	}
	else if(name_vals==(const struct shtable2*)shtab_builtins)
		sh.bltin_cmds = np;
	base_treep = treep = dtopen(&_Nvdisc,Dtoset);
	for(tp=name_vals;*tp->sh_name;tp++,np++)
	{
		if((np->nvname = strrchr(tp->sh_name,'.')) && np->nvname!=((char*)tp->sh_name))
			np->nvname++;
		else
		{
			np->nvname = (char*)tp->sh_name;
			treep = base_treep;
		}
		np->nvmeta = NULL;
		if(name_vals==(const struct shtable2*)shtab_builtins)
			np->nvalue = ((struct shtable3*)tp)->sh_value;
		else
		{
			if(name_vals == shtab_variables)
				np->nvfun = &sh.nvfun;
			np->nvalue = (void*)tp->sh_value;
		}
		nv_setattr(np,tp->sh_number);
		if(nv_isattr(np,NV_TABLE))
			nv_mount(np,NULL,dict=dtopen(&_Nvdisc,Dtoset));
		if(nv_isattr(np,NV_INTEGER))
			nv_setsize(np,10);
		else
			nv_setsize(np,0);
		dtinsert(treep,np);
		if(nv_istable(np))
			treep = dict;
	}
	return treep;
}

/*
 * read in the process environment and set up name-value pairs
 * skip over items that are not name-value pairs
 */
static void env_init(void)
{
	char		*cp;
	char		**ep=environ;
	int		save_env_n = 0;
	if(ep)
	{
		while(cp = *ep++)
		{
			if(strncmp(cp,"KSH_VERSION=",12)==0)
				continue;
			if(!nv_open(cp,sh.var_tree,(NV_EXPORT|NV_IDENT|NV_ASSIGN|NV_NOFAIL)) && !sh.save_env_n)
			{	/*
				 * If the shell assignment via nv_open() failed, we cannot import this
				 * env var (invalid name); save it for sh_envgen() to pass it on to
				 * child processes. This does not need to be re-done after forking.
				 */
				save_env_n++;
				sh.save_env = sh_realloc(sh.save_env, save_env_n*sizeof(char*));
				sh.save_env[save_env_n-1] = cp;
			}
		}
	}
	if(save_env_n)
		sh.save_env_n = save_env_n;
	path_pwd();
	if((cp = nv_getval(SHELLNOD)) && (sh_type(cp)&SH_TYPE_RESTRICTED))
		sh_onoption(SH_RESTRICTED); /* restricted shell */
	/*
	 * Since AST setlocale() may use the environment (PATH, _AST_FEATURES),
	 * defer setting locale until all of the environment has been imported.
	 */
	sh_onstate(SH_LCINIT);
	while (put_lang_defer)
	{
		struct put_lang_defer_s *p = put_lang_defer;
		put_lang(p->np, p->val, p->flags, p->fp);
		put_lang_defer = p->next;
		free(p);
	}
	sh_offstate(SH_LCINIT);
}

/*
 * libshell compatibility functions
 * (libshell programs do not get the macros)
 */
#define BYPASS_MACRO

uint64_t sh_isoption BYPASS_MACRO (int opt)
{
	return sh_isoption(opt);
}

uint64_t sh_onoption BYPASS_MACRO (int opt)
{
	return sh_onoption(opt);
}

uint64_t sh_offoption BYPASS_MACRO (int opt)
{
	return sh_offoption(opt);
}

void	sh_sigcheck BYPASS_MACRO (void)
{
	sh_sigcheck();
}

/*
 * This code is for character mapped variables with wctrans()
 */
struct Mapchar
{
	Namfun_t	hdr;
	const char	*name;
	wctrans_t	trans;
	int		lctype;
};

static void put_trans(Namval_t *np,const char *val,int flags,Namfun_t *fp)
{
	struct Mapchar *mp = (struct Mapchar*)fp;
	int c, offset = stktell(sh.stk), off = offset;
	if(val)
	{
		if(mp->lctype!=lctype)
		{
			mp->lctype = lctype;
			mp->trans = wctrans(mp->name);
		}
		if(!mp->trans || (flags&NV_INTEGER))
			goto skip;
		while(c = mbchar(val))
		{
			c = towctrans(c,mp->trans);
			stkseek(sh.stk,off+c);
			stkseek(sh.stk,off);
			c  = mbconv(stkptr(sh.stk,off),c);
			off += c;
			stkseek(sh.stk,off);
		}
		sfputc(sh.stk,0);
		val = stkptr(sh.stk,offset);
	}
	else
	{
		nv_putv(np,val,flags,fp);
		nv_disc(np,fp,NV_POP);
		if(!(fp->nofree&1))
			free(fp);
		stkseek(sh.stk,offset);
		return;
	}
skip:
	nv_putv(np,val,flags,fp);
	stkseek(sh.stk,offset);
}

static const Namdisc_t TRANS_disc      = {  sizeof(struct Mapchar), put_trans };

Namfun_t	*nv_mapchar(Namval_t *np,const char *name)
{
	wctrans_t	trans = name?wctrans(name):0;
	struct Mapchar	*mp=0;
	int		low;
	size_t		n=0;
	if(np)
		mp = (struct Mapchar*)nv_hasdisc(np,&TRANS_disc);
	if(!name)
		return mp ? (Namfun_t*)mp->name : 0;
	if(!trans)
		return NULL;
	if(!np)
		return ((Namfun_t*)1);  /* non-dereferenceable non-NULL result to use as boolean true */
	if((low=strcmp(name,e_tolower)) && strcmp(name,e_toupper))
		n += strlen(name)+1;
	if(mp)
	{
		if(strcmp(name,mp->name)==0)
			return &mp->hdr;
		nv_disc(np,&mp->hdr,NV_POP);
		if(!(mp->hdr.nofree&1))
			free(mp);
	}
	mp = sh_newof(0,struct Mapchar,1,n);
	mp->trans = trans;
	mp->lctype = lctype;
	if(low==0)
		mp->name = e_tolower;
	else if(n==0)
		mp->name = e_toupper;
	else
	{
		mp->name = (char*)(mp+1);
		strcpy((char*)mp->name,name);
	}
	mp->hdr.disc =  &TRANS_disc;
	return &mp->hdr;
}

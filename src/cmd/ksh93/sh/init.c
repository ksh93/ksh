/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2022 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 1.0                  *
*                    by AT&T Intellectual Property                     *
*                                                                      *
*                A copy of the License is available at                 *
*          http://www.eclipse.org/org/documents/epl-v10.html           *
*         (with md5 checksum b35adb5213ca9657e911e9befb180842)         *
*                                                                      *
*              Information and Software Systems Research               *
*                            AT&T Research                             *
*                           Florham Park NJ                            *
*                                                                      *
*                  David Korn <dgk@research.att.com>                   *
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

#include        "defs.h"
#include        <stak.h>
#include        <ccode.h>
#include        <pwd.h>
#include        <tmx.h>
#include        <regex.h>
#include	<math.h>
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
#include	"FEATURE/dynamic"
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
		return(1);
	else if(strcmp(name,e_toupper)==0)
		return(2);
	return(0);
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
	return(c);
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
#if SHOPT_PFSH && _hdr_exec_attr
#define ATTRS		1
			"P"
#endif
#if SHOPT_REGRESS
#define ATTRS		1
			"R"
#endif
#if !_std_malloc && !_AST_std_malloc
#define ATTRS		1
			"v"	/* uses vmalloc */
#endif
#if ATTRS
			" "
#endif
			SH_RELEASE " $\0\n";

#define RANDMASK	0x7fff

#ifndef ARG_MAX
#   define ARG_MAX	(1*1024*1024)
#endif
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
	Namfun_t	LINENO_init;
	Namfun_t	L_ARG_init;
	Namfun_t	SH_VERSION_init;
	struct match	SH_MATCH_init;
	Namfun_t	SH_MATH_init;
#ifdef _hdr_locale
	Namfun_t	LC_TYPE_init;
	Namfun_t	LC_TIME_init;
	Namfun_t	LC_NUM_init;
	Namfun_t	LC_COLL_init;
	Namfun_t	LC_MSG_init;
	Namfun_t	LC_ALL_init;
	Namfun_t	LANG_init;
#endif /* _hdr_locale */
} Init_t;

static int		lctype;
static int		nvars;
static char		*env_init(void);
static void		env_import_attributes(char*);
static Init_t		*nv_init(void);
#if SHOPT_STATS
static void		stat_init(void);
#endif
static int		shlvl;
static int		rand_shift;

/*
 * out of memory routine for stak routines
 */
static noreturn char *nomemory(int unused)
{
	NOT_USED(unused);
	errormsg(SH_DICT, ERROR_SYSTEM|ERROR_PANIC, "out of memory");
	UNREACHABLE();
}

/*
 * The following are wrapper functions for memory allocation.
 * These functions will error out if the allocation fails.
 */
void *sh_malloc(size_t size)
{
	void *cp = malloc(size);
	if(!cp)
		nomemory(0);
	return(cp);
}

void *sh_realloc(void *ptr, size_t size)
{
	void *cp = realloc(ptr, size);
	if(!cp)
		nomemory(0);
	return(cp);
}

void *sh_calloc(size_t nmemb, size_t size)
{
	void *cp = calloc(nmemb, size);
	if(!cp)
		nomemory(0);
	return(cp);
}

char *sh_strdup(const char *s)
{
	char *dup = strdup(s);
	if(!dup)
		nomemory(0);
	return(dup);
}

void *sh_memdup(const void *s, size_t n)
{
	void *dup = memdup(s, n);
	if(!dup)
		nomemory(0);
	return(dup);
}

char *sh_getcwd(void)
{
	char *cwd = getcwd(NIL(char*), 0);
	if(!cwd && errno==ENOMEM)
		nomemory(0);
	return(cwd);
}

#if SHOPT_VSH || SHOPT_ESH
/* Trap for VISUAL and EDITOR variables */
static void put_ed(register Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	register const char *cp, *name=nv_name(np);
	register int	newopt=0;
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
static void put_history(register Namval_t* np,const char *val,int flags,Namfun_t *fp)
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
static void put_optindex(Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	sh.st.opterror = sh.st.optchar = 0;
	nv_putv(np, val, flags, fp);
	if(!val)
		nv_disc(np,fp,NV_POP);
}

static Sfdouble_t nget_optindex(register Namval_t* np, Namfun_t *fp)
{
	return((Sfdouble_t)*np->nvalue.lp);
}

static Namfun_t *clone_optindex(Namval_t* np, Namval_t *mp, int flags, Namfun_t *fp)
{
	Namfun_t *dp = (Namfun_t*)sh_malloc(sizeof(Namfun_t));
	memcpy((void*)dp,(void*)fp,sizeof(Namfun_t));
	mp->nvalue.lp = np->nvalue.lp;
	dp->nofree = 0;
	return(dp);
}


/* Trap for restricted variables FPATH, PATH, SHELL, ENV */
static void put_restricted(register Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	int	path_scoped = 0, fpath_scoped=0;
	char *name = nv_name(np);
	if(!(flags&NV_RDONLY) && sh_isoption(SH_RESTRICTED))
	{
		errormsg(SH_DICT,ERROR_exit(1),e_restricted,nv_name(np));
		UNREACHABLE();
	}
	if(np==PATHNOD	|| (path_scoped=(strcmp(name,PATHNOD->nvname)==0)))		
	{
		/* Clear the hash table */
		nv_scan(sh_subtracktree(1),nv_rehash,(void*)0,NV_TAGGED,NV_TAGGED);
		if(path_scoped && !val)
			val = PATHNOD->nvalue.cp;
	}
	if(val && !(flags&NV_RDONLY) && np->nvalue.cp && strcmp(val,np->nvalue.cp)==0)
		 return;
	if(np==FPATHNOD	|| (fpath_scoped=(strcmp(name,FPATHNOD->nvname)==0)))		
		sh.pathlist = (void*)path_unsetfpath();
	nv_putv(np, val, flags, fp);
	sh.universe = 0;
	if(sh.pathlist)
	{
		val = np->nvalue.cp;
		if(np==PATHNOD || path_scoped)
			sh.pathlist = (void*)path_addpath((Pathcomp_t*)sh.pathlist,val,PATH_PATH);
		else if(val && (np==FPATHNOD || fpath_scoped))
			sh.pathlist = (void*)path_addpath((Pathcomp_t*)sh.pathlist,val,PATH_FPATH);
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

static void put_cdpath(register Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	nv_putv(np, val, flags, fp);
	if(!sh.cdpathlist)
		return;
	val = np->nvalue.cp;
	sh.cdpathlist = (void*)path_addpath((Pathcomp_t*)sh.cdpathlist,val,PATH_CDPATH);
}

#ifdef _hdr_locale
    /* Trap for the LC_* and LANG variables */
    static void put_lang(Namval_t* np,const char *val,int flags,Namfun_t *fp)
    {
	int type;
	char *name = nv_name(np);
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
	if(!sh_isstate(SH_INIT) && (type>=0 || type==LC_ALL || type==LC_LANG))
	{
		char*		r;
#ifdef AST_LC_setenv
		ast.locale.set |= AST_LC_setenv;
#endif
		r = setlocale(type,val?val:"");
#ifdef AST_LC_setenv
		ast.locale.set ^= AST_LC_setenv;
#endif
		if(!r && val)
		{
			if(!sh_isstate(SH_INIT) || sh.login_sh==0)
				errormsg(SH_DICT,0,e_badlocale,val);
			return;
		}
	}
	nv_putv(np, val, flags, fp);
	if(CC_NATIVE!=CC_ASCII && (type==LC_ALL || type==LC_LANG || type==LC_CTYPE))
	{
		if(sh_lexstates[ST_BEGIN]!=sh_lexrstates[ST_BEGIN])
			free((void*)sh_lexstates[ST_BEGIN]);
		lctype++;
		if(ast.locale.set&(1<<AST_LC_CTYPE))
		{
			register int c;
			char *state[4];
			sh_lexstates[ST_BEGIN] = state[0] = (char*)sh_malloc(4*(1<<CHAR_BIT));
			memcpy(state[0],sh_lexrstates[ST_BEGIN],(1<<CHAR_BIT));
			sh_lexstates[ST_NAME] = state[1] = state[0] + (1<<CHAR_BIT);
			memcpy(state[1],sh_lexrstates[ST_NAME],(1<<CHAR_BIT));
			sh_lexstates[ST_DOL] = state[2] = state[1] + (1<<CHAR_BIT);
			memcpy(state[2],sh_lexrstates[ST_DOL],(1<<CHAR_BIT));
			sh_lexstates[ST_BRACE] = state[3] = state[2] + (1<<CHAR_BIT);
			memcpy(state[3],sh_lexrstates[ST_BRACE],(1<<CHAR_BIT));
			for(c=0; c<(1<<CHAR_BIT); c++)
			{
				if(state[0][c]!=S_REG)
					continue;
				if(state[2][c]!=S_ERR)
					continue;
				if(isblank(c))
				{
					state[0][c]=0;
					state[1][c]=S_BREAK;
					state[2][c]=S_BREAK;
					continue;
				}
				if(!isalpha(c))
					continue;
				state[0][c]=S_NAME;
				if(state[1][c]==S_REG)
					state[1][c]=0;
				state[2][c]=S_ALP;
				if(state[3][c]==S_ERR)
					state[3][c]=0;
			}
		}
		else
		{
			sh_lexstates[ST_BEGIN]=(char*)sh_lexrstates[ST_BEGIN];
			sh_lexstates[ST_NAME]=(char*)sh_lexrstates[ST_NAME];
			sh_lexstates[ST_DOL]=(char*)sh_lexrstates[ST_DOL];
			sh_lexstates[ST_BRACE]=(char*)sh_lexrstates[ST_BRACE];
		}
	}
    }
#endif /* _hdr_locale */

/* Trap for IFS assignment and invalidates state table */
static void put_ifs(register Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	register struct ifs *ip = (struct ifs*)fp;
	ip->ifsnp = 0;
	if(!val)
	{
		fp = nv_stack(np, NIL(Namfun_t*));
		if(fp && !fp->nofree)
		{
			free((void*)fp);
			fp = 0;
		}
	}
	if(val != np->nvalue.cp)
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
static char* get_ifs(register Namval_t* np, Namfun_t *fp)
{
	register struct ifs *ip = (struct ifs*)fp;
	register char *cp, *value;
	register int c,n;
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
				if(c== *cp)
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
	return(value);
}

/*
 * these functions are used to get and set the SECONDS variable
 */
#ifdef timeofday
#   define dtime(tp) ((double)((tp)->tv_sec)+1e-6*((double)((tp)->tv_usec)))
#   define tms	timeval
#else
#   define dtime(tp)	(((double)times(tp))/sh.lim.clk_tck)
#   define timeofday(a)
#endif

static void put_seconds(register Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	double d;
	struct tms tp;
	if(!val)
	{
		nv_putv(np, val, flags, fp);
		fp = nv_stack(np, NIL(Namfun_t*));
		if(fp && !fp->nofree)
			free((void*)fp);
		return;
	}
	if(!np->nvalue.dp)
	{
		nv_setsize(np,3);
		nv_onattr(np,NV_DOUBLE);
		np->nvalue.dp = new_of(double,0);
	}
	nv_putv(np, val, flags, fp);
	d = *np->nvalue.dp;
	timeofday(&tp);
	*np->nvalue.dp = dtime(&tp)-d;
}

static char* get_seconds(register Namval_t* np, Namfun_t *fp)
{
	register int places = nv_size(np);
	struct tms tp;
	double d, offset = (np->nvalue.dp?*np->nvalue.dp:0);
	NOT_USED(fp);
	timeofday(&tp);
	d = dtime(&tp)- offset;
	sfprintf(sh.strbuf,"%.*f",places,d);
	return(sfstruse(sh.strbuf));
}

static Sfdouble_t nget_seconds(register Namval_t* np, Namfun_t *fp)
{
	struct tms tp;
	double offset = (np->nvalue.dp?*np->nvalue.dp:0);
	NOT_USED(fp);
	timeofday(&tp);
	return(dtime(&tp)- offset);
}

/*
 * These four functions are used to get and set the RANDOM variable
 */
static void put_rand(register Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	struct rand *rp = (struct rand*)fp;
	register long n;
	sh_save_rand_seed(rp, 0);
	if(!val)
	{
		fp = nv_stack(np, NIL(Namfun_t*));
		if(fp && !fp->nofree)
			free((void*)fp);
		_nv_unset(np,NV_RDONLY);
		return;
	}
	if(flags&NV_INTEGER)
		n = *(double*)val;
	else
		n = sh_arith(val);
	srand(rp->rand_seed = (unsigned int)n);
	rp->rand_last = -1;
	if(!np->nvalue.lp)
		np->nvalue.lp = &rp->rand_last;
}

/*
 * get random number in range of 0 - 2**15
 * never pick same number twice in a row
 */
static Sfdouble_t nget_rand(register Namval_t* np, Namfun_t *fp)
{
	struct rand *rp = (struct rand*)fp;
	register long cur, last= *np->nvalue.lp;
	sh_save_rand_seed(rp, 1);
	do
		cur = (rand_r(&rp->rand_seed)>>rand_shift)&RANDMASK;
	while(cur==last);
	*np->nvalue.lp = cur;
	return((Sfdouble_t)cur);
}

static char* get_rand(register Namval_t* np, Namfun_t *fp)
{
	register long n = nget_rand(np,fp);
	return(fmtbase(n, 10, 0));
}

void sh_reseed_rand(struct rand *rp)
{
	struct tms		tp;
	unsigned int		time;
	static unsigned int	seq;
	timeofday(&tp);
	time = (unsigned int)remainder(dtime(&tp) * 10000.0, (double)UINT_MAX);
	srand(rp->rand_seed = sh.current_pid ^ time ^ ++seq);
	rp->rand_last = -1;
}

/*
 * These three routines are for LINENO
 */
static Sfdouble_t nget_lineno(Namval_t* np, Namfun_t *fp)
{
	int d = 1;
	if(error_info.line >0)
		d = error_info.line;
	else if(error_info.context && error_info.context->line>0)
		d = error_info.context->line;
	NOT_USED(np);
	NOT_USED(fp);
	return((Sfdouble_t)d);
}

static void put_lineno(Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	Sfdouble_t n;
	if(!val)
	{
		fp = nv_stack(np, NIL(Namfun_t*));
		if(fp && !fp->nofree)
			free((void*)fp);
		_nv_unset(np,NV_RDONLY);
		return;
	}
	if(flags&NV_INTEGER)
		n = (Sfdouble_t)(*(double*)val);
	else
		n = sh_arith(val);
	sh.st.firstline += (int)(nget_lineno(np,fp) + 1 - n);
}

static char* get_lineno(register Namval_t* np, Namfun_t *fp)
{
	long n = (long)nget_lineno(np,fp);
	return(fmtbase(n, 10, 0));
}

static char* get_lastarg(Namval_t* np, Namfun_t *fp)
{
	char	*cp;
	int	pid;
        if(sh_isstate(SH_INIT) && (cp=sh.lastarg) && *cp=='*' && (pid=strtol(cp+1,&cp,10)) && *cp=='*')
		nv_putval(np,cp+1,0);
	return(sh.lastarg);
}

static void put_lastarg(Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	if(flags&NV_INTEGER)
	{
		sfprintf(sh.strbuf,"%.*g",12,*((double*)val));
		val = sfstruse(sh.strbuf);
	}
	if(val)
		val = sh_strdup(val);
	if(sh.lastarg && !nv_isattr(np,NV_NOFREE))
		free((void*)sh.lastarg);
	else
		nv_offattr(np,NV_NOFREE);
	sh.lastarg = (char*)val;
	nv_offattr(np,NV_EXPORT);
	np->nvenv = 0;
}

static void match2d(struct match *mp)
{
	Namval_t	*np;
	int		i;
	Namarr_t	*ap;
	nv_disc(SH_MATCHNOD, &mp->hdr, NV_POP);
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
		nv_putsub(np, (char*)0, 1);
		nv_putsub(np, (char*)0, 0);
		nv_putsub(SH_MATCHNOD, (char*)0, i);
		nv_arraychild(SH_MATCHNOD, np, 0);
		np = nv_namptr(np + 1, 0);
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
	register int	i,n,x, savesub=sh.subshell;
	Namarr_t	*ap = nv_arrayptr(SH_MATCHNOD);
	Namval_t	*np;
	if(sh.intrace)
		return;
	sh.subshell = 0;
	if(index<0)
	{
		np = nv_namptr(mp->nodes,0);
		if(mp->index==0)
			match2d(mp);
		for(i=0; i < mp->nmatch; i++)
		{
			nv_disc(np,&mp->hdr,NV_LAST);
			nv_putsub(np,(char*)0,mp->index);
			for(x=mp->index; x >=0; x--)
			{
				n = i + x*mp->nmatch;
				if(mp->match[2*n+1]>mp->match[2*n])
					nv_putsub(np,Empty,ARRAY_ADD|x);
			}
			if((ap=nv_arrayptr(np)) && array_elem(ap)==0)
			{
				nv_putsub(SH_MATCHNOD,(char*)0,i);
				_nv_unset(SH_MATCHNOD,NV_RDONLY);
			}
			np = nv_namptr(np+1,0);
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
					free((void*)np->nvfun);
					np->nvfun = 0;
				}
				np = nv_namptr(np+1,0);
			}
			free((void*)mp->nodes);
			mp->nodes = 0;
		}
		mp->vlen = 0;
		if(ap && ap->hdr.next != &mp->hdr)
			free((void*)ap);
		SH_MATCHNOD->nvalue.cp = 0;
		SH_MATCHNOD->nvfun = 0;
		if(!(mp->nmatch=nmatch) && !v)
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
		{
			if(mp->msize)
				mp->match = (int*)sh_realloc(mp->match,2*i);
			else
				mp->match = (int*)sh_malloc(2*i);
			mp->msize = 2*i;
		}
		if(vsize >= mp->vsize)
		{
			if(mp->vsize)
				mp->val = (char*)sh_realloc(mp->val,x=2*vsize);
			else
				mp->val =  (char*)sh_malloc(x=vsize+1);
			mp->vsize = x;
		}
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

static char* get_match(register Namval_t* np, Namfun_t *fp)
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
		return(0);
	if(sub2>0)
		sub += sub2*mp->nmatch;
	if(sub==mp->lastsub[!i])
		return(mp->rval[!i]);
	else if(sub==mp->lastsub[i])
		return(mp->rval[i]);
	n = mp->match[2*sub+1]-mp->match[2*sub];
	if(n<=0)
		return(mp->match[2*sub]<0?Empty:"");
	val = mp->val+mp->match[2*sub];
	if(mp->val[mp->match[2*sub+1]]==0)
		return(val);
	mp->index = i;
	if(mp->rval[i])
	{
		free((void*)mp->rval[i]);
		mp->rval[i] = 0;
	}
	mp->rval[i] = (char*)sh_malloc(n+1);
	mp->lastsub[i] = sub;
	memcpy(mp->rval[i],val,n);
	mp->rval[i][n] = 0;
	return(mp->rval[i]);
}

static char *name_match(Namval_t *np, Namfun_t *fp)
{
	int sub = nv_aindex(SH_MATCHNOD);
	sfprintf(sh.strbuf,".sh.match[%d]",sub);
	return(sfstruse(sh.strbuf));
}

static const Namdisc_t SH_MATCH_disc = { sizeof(struct match), 0, get_match,
	0,0,0,0,name_match };

static char* get_version(register Namval_t* np, Namfun_t *fp)
{
	return(nv_getv(np,fp));
}

static Sfdouble_t nget_version(register Namval_t* np, Namfun_t *fp)
{
	register const char	*cp = e_version + strlen(e_version)-10;
	register int		c;
	Sflong_t		t = 0;
	NOT_USED(fp);

	while (c = *cp++)
		if (c >= '0' && c <= '9')
		{
			t *= 10;
			t += c - '0';
		}
	return((Sfdouble_t)t);
}

static const Namdisc_t SH_VERSION_disc	= {  0, 0, get_version, nget_version };


static const Namdisc_t IFS_disc		= {  sizeof(struct ifs), put_ifs, get_ifs };
const Namdisc_t RESTRICTED_disc	= {  sizeof(Namfun_t), put_restricted };
static const Namdisc_t CDPATH_disc	= {  sizeof(Namfun_t), put_cdpath }; 
#if SHOPT_VSH || SHOPT_ESH
static const Namdisc_t EDITOR_disc	= {  sizeof(Namfun_t), put_ed };
#endif
static const Namdisc_t HISTFILE_disc	= {  sizeof(Namfun_t), put_history };
static const Namdisc_t OPTINDEX_disc	= {  sizeof(Namfun_t), put_optindex, 0, nget_optindex, 0, 0, clone_optindex };
static const Namdisc_t SECONDS_disc	= {  sizeof(Namfun_t), put_seconds, get_seconds, nget_seconds };
static const Namdisc_t RAND_disc	= {  sizeof(struct rand), put_rand, get_rand, nget_rand };
static const Namdisc_t LINENO_disc	= {  sizeof(Namfun_t), put_lineno, get_lineno, nget_lineno };
static const Namdisc_t L_ARG_disc	= {  sizeof(Namfun_t), put_lastarg, get_lastarg };


#define MAX_MATH_ARGS	3

static char *name_math(Namval_t *np, Namfun_t *fp)
{
	sfprintf(sh.strbuf,".sh.math.%s",np->nvname);
	return(sfstruse(sh.strbuf));
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
	if(!name)
		return(SH_MATHNOD);
	if(name[0]!='a' || name[1]!='r' || name[2]!='g' || name[4] || !isdigit(name[3]) || (name[3]=='0' || (name[3]-'0')>MAX_MATH_ARGS))
		return(0);
	fp->last = (char*)&name[4];
	return(nv_namptr(sh.mathnodes,name[3]-'1'));
}

static char* get_math(register Namval_t* np, Namfun_t *fp)
{
	Namval_t	*mp,fake;
	char		*val;
	int		first=0;
	fake.nvname = ".sh.math.";
	mp = (Namval_t*)dtprev(sh.fun_tree,&fake);
	while(mp=(Namval_t*)dtnext(sh.fun_tree,mp))
	{
		if(memcmp(mp->nvname,".sh.math.",9))
			break;
		if(first++)
			sfputc(sh.strbuf,' ');
		sfputr(sh.strbuf,mp->nvname+9,-1);
	}
	val = sfstruse(sh.strbuf);
	return(val);
	
}

static char *setdisc_any(Namval_t *np, const char *event, Namval_t *action, Namfun_t *fp)
{
	Namval_t	*mp,fake;
	char		*name;
	int		getname=0, off=staktell();
	fake.nvname = nv_name(np);
	if(!event)
	{
		if(!action)
		{
			mp = (Namval_t*)dtprev(sh.fun_tree,&fake);
			return((char*)dtnext(sh.fun_tree,mp));
		}
		getname = 1;
	}
	stakputs(fake.nvname);
	stakputc('.');
	stakputs(event);
	stakputc(0);
	name  = stakptr(off);
	mp = nv_search(name, sh.fun_tree, action?NV_ADD:0);
	stakseek(off);
	if(getname)
		return(mp?(char*)dtnext(sh.fun_tree,mp):0);
	if(action==np)
		action = mp;
	return(action?(char*)action:"");
}

static const Namdisc_t SH_MATH_disc  = { 0, 0, get_math, 0, setdisc_any, create_math, };

#ifdef _hdr_locale
    static const Namdisc_t LC_disc	= {  sizeof(Namfun_t), put_lang };
#endif /* _hdr_locale */

/*
 * This function will get called whenever a configuration parameter changes
 */
static int newconf(const char *name, const char *path, const char *value)
{
	register char *arg;
	if(!name)
		setenviron(value);
	else if(strcmp(name,"UNIVERSE")==0 && strcmp(astconf(name,0,0),value))
	{
		sh.universe = 0;
		/* set directory in new universe */
		if(*(arg = path_pwd())=='/')
			chdir(arg);
		/* clear out old tracked alias */
		stakseek(0);
		stakputs(nv_getval(PATHNOD));
		stakputc(0);
		nv_putval(PATHNOD,stakseek(0),NV_RDONLY);
	}
	return(1);
}

#if	(CC_NATIVE != CC_ASCII)
    static void a2e(char *d, const char *s)
    {
	register const unsigned char *t;
	register int i;
	t = CCMAP(CC_ASCII, CC_NATIVE);
	for(i=0; i<(1<<CHAR_BIT); i++)
		d[t[i]] = s[i];
    }

    static void init_ebcdic(void)
    {
	int i;
	char *cp = (char*)sh_malloc(ST_NONE*(1<<CHAR_BIT));
	for(i=0; i < ST_NONE; i++)
	{
		a2e(cp,sh_lexrstates[i]);
		sh_lexstates[i] = cp;
		cp += (1<<CHAR_BIT);
	}
    }
#endif

/*
 * return SH_TYPE_* bitmask for path
 * 0 for "not a shell"
 */
int sh_type(register const char *path)
{
	register const char*	s;
	register int		t = 0;
	
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
		if (!(t & (SH_TYPE_PROFILE|SH_TYPE_RESTRICTED)))
		{
#if SHOPT_PFSH
			if (*s == 'p' && *(s+1) == 'f')
			{
				s += 2;
				t |= SH_TYPE_PROFILE;
				continue;
			}
#endif
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
	return t & ~(SH_TYPE_KSH|SH_TYPE_PROFILE|SH_TYPE_RESTRICTED);
}


/*
 * initialize the shell
 */
Shell_t *sh_init(register int argc,register char *argv[], Shinit_f userinit)
{
	static int beenhere;
	register int n;
	int type = 0;
	char *save_envmarker;
	static char *login_files[2];
	memfatal();
	n = strlen(e_version);
	if(e_version[n-1]=='$' && e_version[n-2]==' ')
		e_version[n-2]=0;
#if	(CC_NATIVE == CC_ASCII)
	memcpy(sh_lexstates,sh_lexrstates,ST_NONE*sizeof(char*));
#else
	init_ebcdic();
#endif
	if(!beenhere)
	{
		beenhere = 1;
#if SHOPT_REGRESS
		sh_regress_init();
#endif
		sh.current_pid = sh.pid = getpid();
		sh.ppid = getppid();
		sh.userid=getuid();
		sh.euserid=geteuid();
		sh.groupid=getgid();
		sh.egroupid=getegid();
		sh.lim.arg_max = astconf_long(CONF_ARG_MAX);
		sh.lim.child_max = (int)astconf_long(CONF_CHILD_MAX);
		sh.lim.clk_tck = (int)astconf_long(CONF_CLK_TCK);
		if(sh.lim.arg_max <=0)
			sh.lim.arg_max = ARG_MAX;
		if(sh.lim.child_max <=0)
			sh.lim.child_max = CHILD_MAX;
		if(sh.lim.clk_tck <=0)
			sh.lim.clk_tck = CLK_TCK;
		sh.ed_context = (void*)ed_open();
		error_info.id = path_basename(argv[0]);
	}
	umask(sh.mask=umask(0));
	sh.mac_context = sh_macopen();
	sh.arg_context = sh_argopen();
	sh.lex_context = (void*)sh_lexopen(0,1);
	sh.strbuf = sfstropen();
	sh.stk = stkstd;
	sfsetbuf(sh.strbuf,(char*)0,64);
	sh_onstate(SH_INIT);
	error_info.catalog = e_dict;
#if SHOPT_REGRESS
	{
		Opt_t*	nopt;
		Opt_t*	oopt;
		char*	a;
		char**	av = argv;
		char*	regress[3];

		regress[0] = "__regress__";
		regress[2] = 0;
		while ((a = *++av) && a[0] == '-' && (a[1] == 'I' || a[1] == '-' && a[2] == 'r'))
		{
			if (a[1] == 'I')
			{
				if (a[2])
					regress[1] = a + 2;
				else if (!(regress[1] = *++av))
					break;
			}
			else if (strncmp(a+2, "regress", 7))
				break;
			else if (a[9] == '=')
				regress[1] = a + 10;
			else if (!(regress[1] = *++av))
				break;
			nopt = optctx(0, 0);
			oopt = optctx(nopt, 0);
			b___regress__(2, regress, &sh.bltindata);
			optctx(oopt, nopt);
		}
	}
#endif
	sh.cpipe[0] = -1;
	sh.coutpipe = -1;
	for(n=0;n < 10; n++)
	{
		/* don't use lower bits when rand() generates large numbers */
		if(rand() > RANDMASK)
		{
			rand_shift = 3;
			break;
		}
	}
	sh_ioinit();
	/* initialize signal handling */
	sh_siginit();
	stakinstall(NIL(Stak_t*),nomemory);
	/* set up memory for name-value pairs */
	sh.init_context = nv_init();
	/* initialize shell type */
	if(argc>0)
	{
		type = sh_type(*argv);
		if(type&SH_TYPE_LOGIN)
			sh.login_sh = 2;
		if(type&SH_TYPE_POSIX)
		{
			sh_onoption(SH_POSIX);
			sh_onoption(SH_LETOCTAL);
		}
	}
	/* read the environment; don't import attributes yet, but save pointer to them */
	save_envmarker = env_init();
	if(!ENVNOD->nvalue.cp)
	{
		sfprintf(sh.strbuf,"%s/.kshrc",nv_getval(HOME));
		nv_putval(ENVNOD,sfstruse(sh.strbuf),NV_RDONLY);
	}
	*SHLVL->nvalue.ip +=1;
	nv_offattr(SHLVL,NV_IMPORT);
#if SHOPT_SPAWN
	{
		/*
		 * try to find the pathname for this interpreter
		 * try using environment variable _ or argv[0]
		 */
		char *cp=nv_getval(L_ARGNOD);
		char buff[PATH_MAX+1];
		sh.shpath = 0;
		if((n = pathprog(NiL, buff, sizeof(buff))) > 0 && n <= sizeof(buff))
			sh.shpath = sh_strdup(buff);
		else if((cp && (sh_type(cp)&SH_TYPE_SH)) || (argc>0 && strchr(cp= *argv,'/')))
		{
			if(*cp=='/')
				sh.shpath = sh_strdup(cp);
			else if(cp = nv_getval(PWDNOD))
			{
				int offset = staktell();
				stakputs(cp);
				stakputc('/');
				stakputs(argv[0]);
				pathcanon(stakptr(offset),PATH_DOTDOT);
				sh.shpath = sh_strdup(stakptr(offset));
				stakseek(offset);
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
#if SHOPT_ESH || SHOPT_VSH
	sh_onoption(SH_MULTILINE);
#endif
	if(argc>0)
	{
		int dolv_index;
		/* check for restricted shell */
		if(type&SH_TYPE_RESTRICTED)
			sh_onoption(SH_RESTRICTED);
#if SHOPT_PFSH
		/* check for profile shell */
		else if(type&SH_TYPE_PROFILE)
			sh_onoption(SH_PFSH);
#endif
		/* look for options */
		/* sh.st.dolc is $#	*/
		if((sh.st.dolc = sh_argopts(-argc,argv)) < 0)
		{
			sh.exitval = 2;
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
#if _lib_pathposix
					char*	p;

					if((n = pathposix(name, NIL(char*), 0)) > 0)
					{
						p = (char*)sh_malloc(++n);
						pathposix(name, p, n);
						name = p;
					}
					else
#endif
					{
						name[1] = name[0];
						name[0] = name[2] = '/';
					}
				}
			}
#endif /* _WINIX */
		}
		if(beenhere==1)
		{
			struct lconv*	lc;
			sh.decomma = (lc=localeconv()) && lc->decimal_point && *lc->decimal_point==',';
			beenhere = 2;
		}
	}
	/* import variable attributes from environment */
	if(!sh_isoption(SH_POSIX))
		env_import_attributes(save_envmarker);
#if SHOPT_PFSH
	if (sh_isoption(SH_PFSH))
	{
		struct passwd *pw = getpwuid(sh.userid);
		if(pw)
			sh.user = sh_strdup(pw->pw_name);
		
	}
#endif
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
#ifdef SHELLMAGIC
		/* careful of #! setuid scripts with name beginning with - */
		if(sh.login_sh && argv[1] && strcmp(argv[0],argv[1])==0)
		{
			errormsg(SH_DICT,ERROR_exit(1),e_prohibited);
			UNREACHABLE();
		}
#endif /*SHELLMAGIC*/
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
	sh.jmpbuffer = (void*)&sh.checkbase;
	sh_pushcontext(&sh.checkbase,SH_JMPSCRIPT);
	sh.st.self = &sh.global;
        sh.topscope = (Shscope_t*)sh.st.self;
	sh_offstate(SH_INIT);
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
	sh.exittrap = 0;
	sh.errtrap = 0;
	sh.end_fn = 0;
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
	return(&sh);
}

/*
 * Reinitialize before executing a script without a #! path.
 * This is done in a fork of the shell, so "exporting" is done by deleting all non-exported variables.
 */
int sh_reinit(char *argv[])
{
	Shopt_t opt;
	Namval_t *np,*npnext;
	Dt_t	*dp;
	int	nofree;
	sh_onstate(SH_INIT);
	sh.subshell = sh.realsubshell = sh.comsub = sh.curenv = sh.jobenv = sh.inuse_bits = sh.fn_depth = sh.dot_depth = 0;
	sh.envlist = NIL(struct argnod*);
	sh.last_root = sh.var_tree;
	if(sh.heredocs)
	{
		sfclose(sh.heredocs);
		sh.heredocs = 0;
	}
	/* Remove non-exported variables, first pass (see sh_envnolocal() in name.c) */
	nv_scan(sh.var_tree,sh_envnolocal,NIL(void*),NV_EXPORT,0);
	nv_scan(sh.var_tree,sh_envnolocal,NIL(void*),NV_ARRAY,NV_ARRAY);
#if SHOPT_NAMESPACE
	if(sh.namespace)
	{
		dp=nv_dict(sh.namespace);
		if(dp==sh.var_tree)
			sh.var_tree = dtview(dp,0);
		_nv_unset(sh.namespace,NV_RDONLY);
		sh.namespace = 0;
	}
#endif /* SHOPT_NAMESPACE */
	/* Delete remaining non-exported, non-default variables; remove attributes from exported variables */
	for(np = dtfirst(sh.var_tree); np; np = npnext)
	{
		if((dp = sh.var_tree)->walk)
			dp = dp->walk;
		npnext = (Namval_t*)dtnext(sh.var_tree,np);
		/* skip default variables (already handled by sh_envnolocal()) */
		if(np >= sh.bltin_nodes && np < &sh.bltin_nodes[nvars])
			continue;
		if(nv_isattr(np,NV_EXPORT))
		{
			if(sh_isoption(SH_POSIX))
			{
				char *cp = NIL(char*);
				/* do not export attributes */
				if(nv_isattr(np,NV_INTEGER))		/* any kind of numeric? */
				{
					cp = sh_strdup(nv_getval(np));	/* save string value */
					_nv_unset(np,NV_RDONLY);	/* free numeric value */
				}
				nv_setattr(np,NV_EXPORT);		/* turn off everything except export */
				if(cp)
					np->nvalue.cp = cp;		/* replace by string value */
			}
			else
			{
				/* export all attributes except readonly */
				nv_offattr(np,NV_RDONLY);
			}
			/* unset discipline */
			if(np->nvfun && np->nvfun->disc)
				np->nvfun->disc = NIL(const Namdisc_t*);
		}
		else
		{
			nofree = nv_isattr(np,NV_NOFREE);		/* note: returns bitmask, not boolean */
			_nv_unset(np,NV_RDONLY);			/* also clears NV_NOFREE attr, if any */
			nv_delete(np,dp,nofree);
		}
	}
	/* Delete types */
	for(np = dtfirst(sh.typedict); np; np = npnext)
	{
		if((dp = sh.typedict)->walk)
			dp = dp->walk;
		npnext = (Namval_t*)dtnext(sh.typedict,np);
		nv_delete(np,dp,0);
	}
	/* Delete functions/built-ins. Note: fun_tree has a viewpath to bltin_tree */
	for(np = dtfirst(sh.fun_tree); np; np = npnext)
	{
		if((dp = sh.fun_tree)->walk)
			dp = dp->walk;
		npnext = (Namval_t*)dtnext(sh.fun_tree,np);
		nv_delete(np, dp, NV_NOFREE);
	}
	while(dp = dtview(sh.fun_tree, NIL(Dt_t*)))
	{
		dtclose(sh.fun_tree);
		sh.fun_tree = dp;
	}
	dtclear(sh.fun_base = sh.fun_tree);
	/* Re-init built-ins as per nv_init() */
	free(sh.bltin_cmds);
	sh.bltin_tree = sh_inittree((const struct shtable2*)shtab_builtins);
	dtview(sh.fun_tree,sh.bltin_tree);
	/* Delete aliases */
	for(np = dtfirst(sh.alias_tree); np; np = npnext)
	{
		if((dp = sh.alias_tree)->walk)
			dp = dp->walk;
		npnext = (Namval_t*)dtnext(sh.alias_tree,np);
		nofree = nv_isattr(np,NV_NOFREE);			/* note: returns bitmask, not boolean */
		_nv_unset(np,NV_RDONLY);				/* also clears NV_NOFREE attr, if any */
		nv_delete(np,dp,nofree);
	}
	/* Delete hash table entries */
	for(np = dtfirst(sh.track_tree); np; np = npnext)
	{
		if((dp = sh.track_tree)->walk)
			dp = dp->walk;
		npnext = (Namval_t*)dtnext(sh.track_tree,np);
		nofree = nv_isattr(np,NV_NOFREE);			/* note: returns bitmask, not boolean */
		_nv_unset(np,NV_RDONLY);				/* also clears NV_NOFREE attr, if any */
		nv_delete(np,dp,nofree);
	}
	while(dp = dtview(sh.track_tree, NIL(Dt_t*)))
	{
		dtclose(sh.track_tree);
		sh.track_tree = dp;
	}
#if SHOPT_STATS
	/* Reset statistics */
	free(sh.stats);
	stat_init();
#endif
	/* Reset shell options; inherit some */
	memset((void*)&opt,0,sizeof(opt));
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
	if(sh_isoption(SH_VIRAW))
		on_option(&opt,SH_VIRAW);
#endif
	sh.options = opt;
	/* set up new args */
	if(argv)
		sh.arglist = sh_argcreate(argv);
	if(sh.arglist)
		sh_argreset(sh.arglist,NIL(struct dolnod*));
	sh.shname = error_info.id = sh_strdup(sh.st.dolv[0]);
	sh_offstate(SH_FORKED);
	/* Reset traps and signals */
	memset(sh.st.trapcom,0,(sh.st.trapmax+1)*sizeof(char*));
	sh_sigreset(0);
	/* increase SHLVL */
	if(!(SHLVL->nvalue.ip))
	{
		shlvl = 0;
		SHLVL->nvalue.ip = &shlvl;
		nv_onattr(SHLVL,NV_INTEGER|NV_EXPORT|NV_NOFREE);
	}
	*SHLVL->nvalue.ip +=1;
	nv_offattr(SHLVL,NV_IMPORT);
	sh.st.filename = sh_strdup(sh.lastarg);
	nv_delete((Namval_t*)0, (Dt_t*)0, 0);
	job.exitval = 0;
	sh.inpipe = sh.outpipe = 0;
	job_clear();
	job.in_critical = 0;
	sh.exittrap = 0;
	sh.errtrap = 0;
	sh.end_fn = 0;
	/* update ${.sh.pid}, $$, $PPID */
	sh.ppid = sh.current_pid;
	sh.current_pid = sh.pid = getpid();
	/* call user init function, if any */
	if(sh.userinit)
		(*sh.userinit)(&sh, 1);
	sh_offstate(SH_INIT);
	return(1);
}

/*
 * set when creating a local variable of this name
 */
Namfun_t *nv_cover(register Namval_t *np)
{
	if(np==IFSNOD || np==PATHNOD || np==SHELLNOD || np==FPATHNOD || np==CDPNOD || np==SECONDS || np==ENVNOD || np==LINENO)
		return(np->nvfun);
#ifdef _hdr_locale
	if(np==LCALLNOD || np==LCTYPENOD || np==LCMSGNOD || np==LCCOLLNOD || np==LCNUMNOD || np==LCTIMENOD || np==LANGNOD)
		return(np->nvfun);
#endif
	 return(0);
}

static const char *shdiscnames[] = { "tilde", 0};

#if SHOPT_STATS
struct Stats
{
	Namfun_t	hdr;
	char		*nodes;
	int		numnodes;
	int		current;
};

static Namval_t *next_stat(register Namval_t* np, Dt_t *root,Namfun_t *fp)
{
	struct Stats *sp = (struct Stats*)fp;
	if(!root)
		sp->current = 0;
	else if(++sp->current>=sp->numnodes)
		return(0);
	return(nv_namptr(sp->nodes,sp->current));
}

static Namval_t *create_stat(Namval_t *np,const char *name,int flag,Namfun_t *fp)
{
	struct Stats		*sp = (struct Stats*)fp;
	register const char	*cp=name;
	register int		i=0,n;
	Namval_t		*nq=0;
	if(!name)
		return(SH_STATS);
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
	return(nq);
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
	sfprintf(sh.strbuf,".sh.stats.%s",np->nvname);
	return(sfstruse(sh.strbuf));
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
	struct Stats	*sp = sh_newof(0,struct Stats,1,nstat*NV_MINSZ);
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
		np->nvalue.ip = &sh.stats[i];
	}
	sp->hdr.dsize = sizeof(struct Stats) + nstat*(sizeof(int)+NV_MINSZ);
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
	double d=0;
	Init_t *ip = sh_newof(0,Init_t,1,0);
	sh.nvfun.last = (char*)&sh;
	sh.nvfun.nofree = 1;
	sh.var_base = sh.var_tree = sh_inittree(shtab_variables);
	SHLVL->nvalue.ip = &shlvl;
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
#ifdef _hdr_locale
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
#endif /* _hdr_locale */
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
	sh_reseed_rand((struct rand *)RANDNOD->nvfun);
	nv_stack(LINENO, &ip->LINENO_init);
	SH_MATCHNOD->nvfun =  &ip->SH_MATCH_init.hdr;
	nv_putsub(SH_MATCHNOD,(char*)0,10);
	nv_stack(SH_MATHNOD, &ip->SH_MATH_init);
	nv_stack(SH_VERSIONNOD, &ip->SH_VERSION_init);
#ifdef _hdr_locale
	nv_stack(LCTYPENOD, &ip->LC_TYPE_init);
	nv_stack(LCALLNOD, &ip->LC_ALL_init);
	nv_stack(LCMSGNOD, &ip->LC_MSG_init);
	nv_stack(LCCOLLNOD, &ip->LC_COLL_init);
	nv_stack(LCNUMNOD, &ip->LC_NUM_init);
	nv_stack(LCTIMENOD, &ip->LC_TIME_init);
	nv_stack(LANGNOD, &ip->LANG_init);
#endif /* _hdr_locale */
	(PPIDNOD)->nvalue.pidp = (&sh.ppid);
	(SH_PIDNOD)->nvalue.pidp = (&sh.current_pid);
	(SH_SUBSHELLNOD)->nvalue.ip = (&sh.realsubshell);
	(TMOUTNOD)->nvalue.lp = (&sh.st.tmout);
	(MCHKNOD)->nvalue.lp = (&sh_mailchk);
	(OPTINDNOD)->nvalue.lp = (&sh.st.optindex);
	/* set up the seconds clock */
	sh.alias_tree = dtopen(&_Nvdisc,Dtoset);
	sh.track_tree = dtopen(&_Nvdisc,Dtset);
	sh.bltin_tree = sh_inittree((const struct shtable2*)shtab_builtins);
	sh.fun_base = sh.fun_tree = dtopen(&_Nvdisc,Dtoset);
	dtview(sh.fun_tree,sh.bltin_tree);
	nv_mount(DOTSHNOD, "type", sh.typedict=dtopen(&_Nvdisc,Dtoset));
	nv_adddisc(DOTSHNOD, shdiscnames, (Namval_t**)0);
	DOTSHNOD->nvalue.cp = Empty;
	nv_onattr(DOTSHNOD,NV_RDONLY);
	SH_LINENO->nvalue.ip = &sh.st.lineno;
	VERSIONNOD->nvalue.nrp = sh_newof(0,struct Namref,1,0);
        VERSIONNOD->nvalue.nrp->np = SH_VERSIONNOD;
        VERSIONNOD->nvalue.nrp->root = nv_dict(DOTSHNOD);
        VERSIONNOD->nvalue.nrp->table = DOTSHNOD;
	nv_onattr(VERSIONNOD,NV_REF);
	math_init();
#if SHOPT_STATS
	if(!sh.stats)
		stat_init();
#endif
	return(ip);
}

/*
 * initialize name-value pairs
 */

Dt_t *sh_inittree(const struct shtable2 *name_vals)
{
	register Namval_t *np;
	register const struct shtable2 *tp;
	register unsigned n = 0;
	register Dt_t *treep;
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
	treep->user = (void*)&sh;
	for(tp=name_vals;*tp->sh_name;tp++,np++)
	{
		if((np->nvname = strrchr(tp->sh_name,'.')) && np->nvname!=((char*)tp->sh_name))
			np->nvname++;
		else
		{
			np->nvname = (char*)tp->sh_name;
			treep = base_treep;
		}
		np->nvenv = 0;
		if(name_vals==(const struct shtable2*)shtab_builtins)
			np->nvalue.bfp = (Nambfp_f)((struct shtable3*)tp)->sh_value;
		else
		{
			if(name_vals == shtab_variables)
				np->nvfun = &sh.nvfun;
			np->nvalue.cp = (char*)tp->sh_value;
		}
		nv_setattr(np,tp->sh_number);
		if(nv_isattr(np,NV_TABLE))
			nv_mount(np,(const char*)0,dict=dtopen(&_Nvdisc,Dtoset));
		if(nv_isattr(np,NV_INTEGER))
			nv_setsize(np,10);
		else
			nv_setsize(np,0);
		dtinsert(treep,np);
		if(nv_istable(np))
			treep = dict;
	}
	return(treep);
}

/*
 * read in the process environment and set up name-value pairs
 * skip over items that are not name-value pairs
 *
 * Returns pointer to A__z env var from which to import attributes, or 0.
 */

static char *env_init(void)
{
	register char		*cp;
	register Namval_t	*np;
	register char		**ep=environ;
	char			*next = 0;	/* pointer to A__z env var */
	if(ep)
	{
		while(cp = *ep++)
		{
			/* The magic A__z env var is an invention of ksh88. See e_envmarker[]. */
			if(*cp=='A' && cp[1]=='_' && cp[2]=='_' && cp[3]=='z' && cp[4]=='=')
				next = cp + 4;
			else if(strncmp(cp,"KSH_VERSION=",12)==0)
				continue;
			else if(np = nv_open(cp,sh.var_tree,(NV_EXPORT|NV_IDENT|NV_ASSIGN|NV_NOFAIL)))
			{
				nv_onattr(np,NV_IMPORT);
				np->nvenv = cp;
				nv_close(np);
			}
			else  /* swap with front */
			{
				ep[-1] = environ[sh.nenv];
				environ[sh.nenv++] = cp;
			}
		}
	}
	if(nv_isnull(PWDNOD) || nv_isattr(PWDNOD,NV_TAGGED))
	{
		nv_offattr(PWDNOD,NV_TAGGED);
		path_pwd();
	}
	if((cp = nv_getval(SHELLNOD)) && (sh_type(cp)&SH_TYPE_RESTRICTED))
		sh_onoption(SH_RESTRICTED); /* restricted shell */
	return(next);
}

/*
 * Import variable attributes from magic A__z env var pointed to by 'next'.
 * If next == 0, this function does nothing.
 */
static void env_import_attributes(char *next)
{
	register char		*cp;
	register Namval_t	*np;
	while(cp=next)
	{
		if(next = strchr(++cp,'='))
			*next = 0;
		np = nv_search(cp+2,sh.var_tree,NV_ADD);
		if(np!=SHLVL && nv_isattr(np,NV_IMPORT|NV_EXPORT))
		{
			int flag = *(unsigned char*)cp-' ';
			int size = *(unsigned char*)(cp+1)-' ';
			if((flag&NV_INTEGER) && size==0)
			{
				/* check for floating */
				char *dp, *val = nv_getval(np);
				strtol(val,&dp,10);
				if(*dp=='.' || *dp=='e' || *dp=='E')
				{
					char *lp;
					flag |= NV_DOUBLE;
					if(*dp=='.')
					{
						strtol(dp+1,&lp,10);
						if(*lp)
							dp = lp;
					}
					if(*dp && *dp!='.')
					{
						flag |= NV_EXPNOTE;
						size = dp-val;
					}
					else
						size = strlen(dp);
					size--;
				}
			}
			flag &= ~NV_RDONLY;	/* refuse to import readonly attribute */
			if(!flag)
				continue;
			nv_newattr(np,flag|NV_IMPORT|NV_EXPORT,size);
		}
	}
	return;
}

/*
 * libshell compatibility functions
 * (libshell programs do not get the macros)
 */
#define BYPASS_MACRO

unsigned long sh_isoption BYPASS_MACRO (int opt)
{
	return(sh_isoption(opt));
}

unsigned long sh_onoption BYPASS_MACRO (int opt)
{
	return(sh_onoption(opt));
}

unsigned long sh_offoption BYPASS_MACRO (int opt)
{
	return(sh_offoption(opt));
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

static void put_trans(register Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	struct Mapchar *mp = (struct Mapchar*)fp;
	int	c,offset = staktell(),off=offset;
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
			stakseek(off+c);
			stakseek(off);
			c  = mbconv(stakptr(off),c);
			off += c;
			stakseek(off);
		}
		stakputc(0);
		val = stakptr(offset);
	}
	else
	{
		nv_putv(np,val,flags,fp);
		nv_disc(np,fp,NV_POP);
		if(!(fp->nofree&1))
			free((void*)fp);
		stakseek(offset);
		return;
	}
skip:
	nv_putv(np,val,flags,fp);
	stakseek(offset);
}

static const Namdisc_t TRANS_disc      = {  sizeof(struct Mapchar), put_trans };

Namfun_t	*nv_mapchar(Namval_t *np,const char *name)
{
	wctrans_t	trans = name?wctrans(name):0;
	struct Mapchar	*mp=0;
	int		n=0,low;
	if(np)
		mp = (struct Mapchar*)nv_hasdisc(np,&TRANS_disc);
	if(!name)
		return(mp?(Namfun_t*)mp->name:0);
	if(!trans)
		return(0);
	if(!np)
		return(((Namfun_t*)0)+1);
	if((low=strcmp(name,e_tolower)) && strcmp(name,e_toupper))
		n += strlen(name)+1;
	if(mp)
	{
		if(strcmp(name,mp->name)==0)
			return(&mp->hdr);
		nv_disc(np,&mp->hdr,NV_POP);
		if(!(mp->hdr.nofree&1))
			free((void*)mp);
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
	return(&mp->hdr);
}

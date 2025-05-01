/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2014 AT&T Intellectual Property          *
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
*                      Phi <phi.debian@gmail.com>                      *
*                                                                      *
***********************************************************************/
/*
 * echo [arg...]
 * print [-enprsvC] [-f format] [-u fd] [string ...]
 * printf [-v var] format [string ...]
 *
 *   David Korn
 *   AT&T Labs
 */

#include	"shopt.h"
#include	"defs.h"
#include	<error.h>
#include	"io.h"
#include	"name.h"
#include	"history.h"
#include	"builtins.h"
#include	"streval.h"
#include	<tmx.h>

union types_t
{
	unsigned char	c;
	short		h;
	int		i;
	long		l;
	Sflong_t	ll;
	Sfdouble_t	ld;
	double		d;
	float		f;
	char		*s;
	int		*ip;
	char		**p;
};

struct printf
{
	Sffmt_t		hdr;
	int		argsize;
	int		intvar;
	char		**argv0; /* see reload() below */
	char		**nextarg;
	char		*lastarg;
	char		cescape;
	char		err;
};

struct printmap
{
	size_t		size;
	char		*name;
	char		map[3];
	const char	*equivalent;
};

static const struct printmap  Pmap[] =
{
	3,	"csv",		"q+",	"%#q",
	3,	"ere",		"R",	"%R",
	4,	"html",		"H",	"%H",
	7,	"pattern",	"P",	"%P",
	3,	"url",		"H+",	"%#H",
	0,	0,		0,
};


static int		echolist(Sfio_t*, int, char**);
static int		extend(Sfio_t*,void*, Sffmt_t*);
static int		reload(int argn, char fmt, void* v, Sffmt_t* fe);
static char		*genformat(char*);
static int		fmtvecho(const char*, struct printf*);
static ssize_t		fmtbase64(Sfio_t*, char*, int);
struct print
{
	const char	*options;
	char		raw;
	char		echon;
};

static char* 	nullarg[] = { 0, 0 };
static int	exitval;

#if !SHOPT_ECHOPRINT
   int    B_echo(int argc, char *argv[],Shbltin_t *context)
   {
	static char bsd_univ;
	struct print prdata;
	prdata.options = sh_optecho+5;
	prdata.raw = prdata.echon = 0;
	NOT_USED(argc);
	NOT_USED(context);
	/* This mess is because /bin/echo on BSD is different */
	if(!sh.universe)
	{
		char *universe;
		if(universe=astconf("UNIVERSE",0,0))
			bsd_univ = (strcmp(universe,"ucb")==0);
		sh.universe = 1;
	}
	if(!bsd_univ)
		return b_print(0,argv,(Shbltin_t*)&prdata);
	prdata.options = sh_optecho;
	prdata.raw = 1;
	while(argv[1] && *argv[1]=='-')
	{
		if(strcmp(argv[1],"-n")==0)
			prdata.echon = 1;
#if !SHOPT_NOECHOE
		else if(strcmp(argv[1],"-e")==0)
			prdata.raw = 0;
		else if(strcmp(argv[1],"-ne")==0 || strcmp(argv[1],"-en")==0)
		{
			prdata.raw = 0;
			prdata.echon = 1;
		}
#endif /* SHOPT_NOECHOE */
		else
			break;
		argv++;
	}
	return b_print(0,argv,(Shbltin_t*)&prdata);
   }
#endif /* SHOPT_ECHOPRINT */

int    b_printf(int argc, char *argv[],Shbltin_t *context)
{
	struct print prdata;
	NOT_USED(argc);
	NOT_USED(context);
	memset(&prdata,0,sizeof(prdata));
	prdata.options = sh_optprintf;
	return b_print(-1,argv,(Shbltin_t*)&prdata);
}

static int infof(Opt_t* op, Sfio_t* sp, const char* s, Optdisc_t* dp)
{
	const struct printmap *pm;
	char c='%';
	NOT_USED(op);
	NOT_USED(s);
	NOT_USED(dp);
	for(pm=Pmap;pm->size>0;pm++)
		sfprintf(sp, "[+%c(%s)q?Equivalent to %s.]",c,pm->name,pm->equivalent);
	return 1;
}

/*
 * argc==0 when called from echo
 * argc==-1 when called from printf
 */
int    b_print(int argc, char *argv[], Shbltin_t *context)
{
	Sfio_t *outfile;
	int n, fd = 1;
	const char *options, *msg = e_file+4;
	char *format = 0;
#if !SHOPT_SCRIPTONLY
	int sflag = 0;
#endif /* !SHOPT_SCRIPTONLY */
	int nflag=0, rflag=0, vflag=0;
	Namval_t *vname=0;
	Optdisc_t disc;
	exitval = 0;
	memset(&disc, 0, sizeof(disc));
	disc.version = OPT_VERSION;
	disc.infof = infof;
	if(argc>0)
	{
		options = sh_optprint;
		nflag = rflag = 0;
		format = 0;
	}
	else
	{
		struct print *pp = (struct print*)context;
		options = pp->options;
		if(argc==0)
		{
			nflag = pp->echon;
			rflag = pp->raw;
			argv++;
			goto skip;
		}
	}
	opt_info.disc = &disc;
#if SHOPT_PRINTF_LEGACY
	/* POSIX-ignorant printf(1) compat, prong 1: prevent option parsing if first arg looks like format operand */
	if(argc<0 && argv[1] && (strchr(argv[1],'%') || strchr(argv[1],'\\')))
	{
		opt_info.index = 1;
		goto skipopts;
	}
#endif /* SHOPT_PRINTF_LEGACY */
	while((n = optget(argv,options))) switch(n)
	{
		case 'n':
			nflag++;
			break;
		case 'p':
		coprocess:
			fd = sh.coutpipe;
			msg = e_query;
			break;
		case 'f':
			format = opt_info.arg;
			break;
#if !SHOPT_SCRIPTONLY
		case 's':
			/* print to history file */
			if(!sh_histinit())
			{
				opt_info.disc = NULL;
				errormsg(SH_DICT,ERROR_system(1),e_history);
				UNREACHABLE();
			}
			fd = sffileno(sh.hist_ptr->histfp);
			sh_onstate(SH_HISTORY);
			sflag++;
			break;
#endif /* !SHOPT_SCRIPTONLY */
		case 'e':
			rflag = 0;
			break;
		case 'r':
			rflag = 1;
			break;
		case 'u':
			if(opt_info.arg[0]=='p' && opt_info.arg[1]==0)
				goto coprocess;
			fd = (int)strtol(opt_info.arg,&opt_info.arg,10);
			if(*opt_info.arg)
				fd = -1;
			else if(!sh_iovalidfd(fd))
				fd = -1;
#if SHOPT_SCRIPTONLY
			else if(!(sh.inuse_bits&(1<<fd)) && sh_inuse(fd))
#else
			else if(!(sh.inuse_bits&(1<<fd)) && (sh_inuse(fd) || (sh.hist_ptr && fd==sffileno(sh.hist_ptr->histfp))))
#endif /* SHOPT_SCRIPTONLY */
				fd = -1;
			break;
		case 'v':
			if(argc < 0)
			{
				/* prepare variable for printf -v varname */
				vname = nv_open(opt_info.arg, sh.var_tree, NV_VARNAME);
				if(!vname)
				{
					opt_info.disc = NULL;
					errormsg(SH_DICT, ERROR_exit(2), e_create, opt_info.arg);
					UNREACHABLE();
				}
			}
			else
				vflag='v';
			break;
		case 'C':
			vflag='C';
			break;
		case ':':
#if SHOPT_PRINTF_LEGACY
			/* POSIX-ignorant printf(1) compat, prong 2: treat erroneous first option as operand */
			if(argc<0 && (opt_info.index==1 || opt_info.index==2 && argv[1][0]=='-' && argv[1][1]=='-'))
			{
				opt_info.index = 1;
				goto skipopts;
			}
#endif /* SHOPT_PRINTF_LEGACY */
			/* The following is for backward compatibility */
			if(strcmp(opt_info.name,"-R")==0)
			{
				rflag = 1;
				if(error_info.errors==0)
				{
					argv += opt_info.index+1;
					/* special case test for -Rn */
					if(strchr(argv[-1],'n'))
						nflag++;
					if(*argv && strcmp(*argv,"-n")==0)
					{
						nflag++;
						argv++;
					}
					opt_info.disc = NULL;
					goto skip2;
				}
			}
			else
				errormsg(SH_DICT,2, "%s", opt_info.arg);
			break;
		case '?':
			opt_info.disc = NULL;
			errormsg(SH_DICT,ERROR_usage(2), "%s", opt_info.arg);
			UNREACHABLE();
	}
#if SHOPT_PRINTF_LEGACY
skipopts:
#endif /* SHOPT_PRINTF_LEGACY */
	opt_info.disc = NULL;
	argv += opt_info.index;
	if(error_info.errors || (argc<0 && !(format = *argv++)))
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NULL));
		UNREACHABLE();
	}
	if(vflag && format)
	{
		errormsg(SH_DICT,ERROR_usage(2),"-%c and -f are mutually exclusive",vflag);
		UNREACHABLE();
	}
skip:
	if(format)
		format = genformat(format);
	/* handle special case of '-' operand for print */
	if(argc>0 && *argv && strcmp(*argv,"-")==0 && strcmp(argv[-1],"--"))
		argv++;
	if(vname)
	{
		static Sfio_t *vbuf;
		if(!vbuf)
			vbuf = sfstropen();
		outfile = vbuf;
		goto printf_v;
	}
skip2:
	if(fd < 0)
	{
		errno = EBADF;
		n = 0;
	}
	else if(!(n=sh.fdstatus[fd]))
		n = sh_iocheckfd(fd);
	if(!(n&IOWRITE))
	{
		/* don't print error message for stdout for compatibility */
		if(fd==1)
			return 1;
		errormsg(SH_DICT,ERROR_system(1),msg);
		UNREACHABLE();
	}
	if(!(outfile=sh.sftable[fd]))
	{
		sh_onstate(SH_NOTRACK);
		n = SFIO_WRITE|((n&IOREAD)?SFIO_READ:0);
		sh.sftable[fd] = outfile = sfnew(NULL,sh.outbuff,IOBSIZE,fd,n);
		sh_offstate(SH_NOTRACK);
		sfpool(outfile,sh.outpool,SFIO_WRITE);
	}
	/* turn off share to guarantee atomic writes for printf */
	n = sfset(outfile,SFIO_SHARE|SFIO_PUBLIC,0);
printf_v:
	if(format)
	{
		/* printf style print */
		Sfio_t *pool;
		struct printf pdata;
		memset(&pdata, 0, sizeof(pdata));
		pdata.hdr.version = SFIO_VERSION;
		pdata.hdr.extf = extend;
		pdata.hdr.reloadf = reload;
		pdata.nextarg = argv;
		sh_offstate(SH_STOPOK);
		pool=sfpool(sfstderr,NULL,SFIO_WRITE);
		do
		{
			pdata.argv0 = pdata.nextarg;
			if(sh.trapnote&SH_SIGSET)
				break;
			pdata.hdr.form = format;
			sfprintf(outfile,"%!",&pdata);
		} while(*pdata.nextarg && pdata.nextarg!=argv);
		if(pdata.nextarg == nullarg && pdata.argsize>0)
			if(sfwrite(outfile,stkptr(sh.stk,stktell(sh.stk)),pdata.argsize) < 0)
				exitval = 1;
		sfpool(sfstderr,pool,SFIO_WRITE);
		if (pdata.err)
			exitval = 1;
	}
	else if(vflag)
	{
		while(*argv)
		{
			if(fmtbase64(outfile,*argv++,vflag=='C') < 0)
				exitval = 1;
			if(!nflag)
				if(sfputc(outfile,'\n') < 0)
					exitval = 1;
		}
	}
	else
	{
		/* echo style print */
		if(nflag && !argv[0])
		{
			if (sfsync(NULL) < 0)
				exitval = 1;
		}
		else if(echolist(outfile,rflag,argv) && !nflag)
			if(sfputc(outfile,'\n') < 0)
				exitval = 1;
	}
	if(vname)
		nv_putval(vname, sfstruse(outfile), 0);
#if !SHOPT_SCRIPTONLY
	else if(sflag)
	{
		hist_flush(sh.hist_ptr);
		sh_offstate(SH_HISTORY);
	}
#endif /* !SHOPT_SCRIPTONLY */
	else
	{
		if(n&SFIO_SHARE)
			sfset(outfile,SFIO_SHARE|SFIO_PUBLIC,1);
		if (sfsync(outfile) < 0)
			exitval = 1;
	}
	return exitval;
}

/*
 * echo the argument list onto <outfile>
 * if <raw> is non-zero then \ is not a special character.
 * returns 0 for \c otherwise 1.
 */
static int echolist(Sfio_t *outfile, int raw, char *argv[])
{
	char	*cp;
	int	n;
	struct printf pdata;
	pdata.cescape = 0;
	pdata.err = 0;
	while(!pdata.cescape && (cp= *argv++))
	{
		if(!raw  && (n=fmtvecho(cp,&pdata))>=0)
		{
			if(n)
				if(sfwrite(outfile,stkptr(sh.stk,stktell(sh.stk)),n) < 0)
					exitval = 1;
		}
		else
			if(sfputr(outfile,cp,-1) < 0)
				exitval = 1;
		if(*argv)
			if(sfputc(outfile,' ') < 0)
				exitval = 1;
		sh_sigcheck();
	}
	return !pdata.cescape;
}

/*
 * modified version of stresc for generating formats
 */
static char strformat(char *s)
{
	char*		t;
	int		c;
	char*		b;
	char*		p;
#if SHOPT_MULTIBYTE && defined(FMT_EXP_WIDE)
	int		w;
#endif
	b = t = s;
	for (;;)
	{
		switch (c = *s++)
		{
		    case '\\':
			if(*s==0)
				break;
#if SHOPT_MULTIBYTE && defined(FMT_EXP_WIDE)
			c = chrexp(s - 1, &p, &w, FMT_EXP_CHAR|FMT_EXP_LINE|FMT_EXP_WIDE);
#else
			c = chresc(s - 1, &p);
#endif
			s = p;
#if SHOPT_MULTIBYTE
#if defined(FMT_EXP_WIDE)
			if(w)
			{
				t += mbwide() ? mbconv(t, c) : wc2utf8(t, c);
				continue;
			}
#else
			if(c>UCHAR_MAX && mbwide())
			{
				t += mbconv(t, c);
				continue;
			}
#endif /* FMT_EXP_WIDE */
#endif /* SHOPT_MULTIBYTE */
			if(c=='%')
				*t++ = '%';
			else if(c==0)
			{
				*t++ = '%';
				c = 'Z';
			}
			break;
		    case 0:
			*t = 0;
			return t - b;
		}
		*t++ = c;
	}
}

static char *genformat(char *format)
{
	char *fp;
	stkseek(sh.stk,0);
	sfputr(sh.stk,format,-1);
	fp = stkfreeze(sh.stk,1);
	strformat(fp);
	return fp;
}

static char *fmthtml(const char *string, int flags)
{
	const char *cp = string, *op;
	int c, offset = stktell(sh.stk);
	/*
	 * The only multibyte locale ksh currently supports is UTF-8, which is a superset of ASCII. So, if we're on an
	 * EBCDIC system, below we attempt to convert EBCDIC to ASCII only if we're not in a multibyte locale (mbwide()).
	 */
	mbinit();
	if(!(flags&SFFMT_ALTER))
	{
		/* Encode for HTML and XML, for main text and single- and double-quoted attributes. */
		while(op = cp, c = mbchar(cp))
		{
			if(mbwide() && c < 0)		/* invalid multibyte char */
				sfputc(sh.stk,'?');
			else if(c == 60)		/* < */
				sfputr(sh.stk,"&lt;",-1);
			else if(c == 62)		/* > */
				sfputr(sh.stk,"&gt;",-1);
			else if(c == 38)		/* & */
				sfputr(sh.stk,"&amp;",-1);
			else if(c == 34)		/* " */
				sfputr(sh.stk,"&quot;",-1);
			else if(c == 39)		/* ' (&apos; is not HTML) */
				sfputr(sh.stk,"&#39;",-1);
			else
				sfwrite(sh.stk, op, cp-op);
		}
	}
	else
	{
		/* Percent-encode for URI. Ref.: RFC 3986, section 2.3 */
		if(mbwide())
		{
			while(op = cp, c = mbchar(cp))
			{
				if(c < 0)
					sfputr(sh.stk,"%3F",-1);
				else if(c < 128 && strchr(URI_RFC3986_UNRESERVED, c))
					sfputc(sh.stk,c);
				else
					while(c = *(unsigned char*)op++, op <= cp)
						sfprintf(sh.stk, "%%%02X", c);
			}
		}
		else
		{
			while(c = *(unsigned char*)cp++)
			{
				if(strchr(URI_RFC3986_UNRESERVED, c))
					sfputc(sh.stk,c);
				else
					sfprintf(sh.stk, "%%%02X", c);
			}
		}
	}
	sfputc(sh.stk,0);
	return stkptr(sh.stk,offset);
}

static ssize_t fmtbase64(Sfio_t *iop, char *string, int alt)
{
	char			*cp;
	Sfdouble_t		d;
	ssize_t			size;
	Namval_t		*np = nv_open(string, NULL, NV_VARNAME|NV_NOADD);
	Namarr_t		*ap;
	static union types_t	number;
	if(!np || nv_isnull(np))
	{
		if(sh_isoption(SH_NOUNSET))
		{
			errormsg(SH_DICT,ERROR_exit(1),e_notset,string);
			UNREACHABLE();
		}
		return 0;
	}
	if(nv_isattr(np,NV_INTEGER))
	{
		d = nv_getnum(np);
		if(nv_isattr(np,NV_DOUBLE))
		{
			if(nv_isattr(np,NV_LONG))
			{
				size = sizeof(Sfdouble_t);
				number.ld = d;
			}
			else if(nv_isattr(np,NV_SHORT))
			{
				size = sizeof(float);
				number.f = (float)d;
			}
			else
			{
				size = sizeof(double);
				number.d = (double)d;
			}
		}
		else
		{
			if(nv_isattr(np,NV_LONG))
			{
				size =  sizeof(Sflong_t);
				number.ll = (Sflong_t)d;
			}
			else if(nv_isattr(np,NV_SHORT))
			{
				size =  sizeof(short);
				number.h = (short)d;
			}
			else
			{
				size =  sizeof(short);
				number.i = (int)d;
			}
		}
		return sfwrite(iop, &number, size);
	}
	if(nv_isattr(np,NV_BINARY))
	{
		Namfun_t *fp;
		for(fp=np->nvfun; fp;fp=fp->next)
		{
			if(fp->disc && fp->disc->writef)
				break;
		}
		if(fp)
			return (*fp->disc->writef)(np, iop, 0, fp);
		else
		{
			int n = nv_size(np);
			if(nv_isarray(np))
			{
				nv_onattr(np,NV_RAW);
				cp = nv_getval(np);
				nv_offattr(np,NV_RAW);
			}
			else
				cp = np->nvalue;
			if((size = n)==0)
				size = strlen(cp);
			size = sfwrite(iop, cp, size);
			return n?n:size;
		}
	}
	else if(nv_isarray(np) && (ap=nv_arrayptr(np)) && array_elem(ap) && (ap->nelem&(ARRAY_UNDEF|ARRAY_SCAN)))
	{
		nv_outnode(np,iop,(alt?-1:0),0);
		if(sfputc(iop,')') < 0)
			exitval = 1;
		return sftell(iop);
	}
	else
	{
		if(alt && nv_isvtree(np))
			nv_onattr(np,NV_EXPORT);
		else
			alt = 0;
		cp = nv_getval(np);
		if(alt)
			nv_offattr(np,NV_EXPORT);
		if(!cp)
			return 0;
		size = strlen(cp);
		return sfwrite(iop,cp,size);
	}
}

static int varname(const char *str, int n)
{
	int c,dot=1,len=1;
	if(n < 0)
	{
		if(*str=='.')
			str++;
		n = strlen(str);
	}
	for(;n > 0; n-=len)
	{
#if SHOPT_MULTIBYTE
		len = mbsize(str);
		c = mbchar(str);
#else
		c = *(unsigned char*)str++;
#endif
		if(dot && !(isalpha(c)||c=='_'))
			break;
		else if(dot==0 && !(isalnum(c) || c=='_' || c == '.'))
			break;
		dot = (c=='.');
	}
	return n==0;
}

static const char *mapformat(Sffmt_t *fe)
{
	const struct printmap *pm = Pmap;
	while(pm->size>0)
	{
		if(pm->size==fe->n_str && strncmp(pm->name,fe->t_str,fe->n_str)==0)
			return pm->map;
		pm++;
	}
	return NULL;
}

static int extend(Sfio_t* sp, void* v, Sffmt_t* fe)
{
	char*		lastchar = "";
	Sfdouble_t	d;
	Sfdouble_t	longmin = LDBL_LLONG_MIN;
	Sfdouble_t	longmax = LDBL_LLONG_MAX;
	int		format = fe->fmt;
	int		n;
	int		fold = fe->base;
	union types_t*	value = (union types_t*)v;
	struct printf*	pp = (struct printf*)fe;
	char*		argp = *pp->nextarg;
	char		*w,*s;
	NOT_USED(sp);
	if(fe->n_str>0 && (format=='T'||format=='Q') && varname(fe->t_str,fe->n_str) && (!argp || varname(argp,-1)))
	{
		if(argp)
			pp->lastarg = argp;
		else
			argp = pp->lastarg;
		if(argp)
		{
			sfprintf(sh.strbuf,"%s.%.*s",argp,fe->n_str,fe->t_str);
			argp = sfstruse(sh.strbuf);
		}
	}
	else
		pp->lastarg = 0;
	fe->flags |= SFFMT_VALUE;
	if(!argp || format=='Z')
	{
		switch(format)
		{
		case 'c':
			value->c = 0;
			fe->flags &= ~SFFMT_LONG;
			break;
		case 'q':
			format = 's';
			/* FALLTHROUGH */
		case 's':
		case 'H':
		case 'B':
		case 'P':
		case 'R':
		case 'Z':
		case 'b':
			fe->fmt = 's';
			fe->size = -1;
			fe->base = -1;
			value->s = "";
			fe->flags &= ~SFFMT_LONG;
			break;
		case 'a':
		case 'e':
		case 'f':
		case 'g':
		case 'A':
		case 'E':
		case 'F':
		case 'G':
			if(SFFMT_LDOUBLE)
				value->ld = 0.;
			else
				value->d = 0.;
			break;
		case 'n':
			value->ip = &pp->intvar;
			break;
		case 'Q':
			value->ll = 0;
			break;
		case 'T':
			fe->fmt = 'd';
			tm_info.flags = 0;
			value->ll = tmxgettime();
			break;
		case '.':
			fe->fmt = 'd';
			value->ll = 0;
			break;
		default:
			if(!strchr("DdXxoUu",format))
			{
				errormsg(SH_DICT,ERROR_exit(1),e_formspec,format);
				UNREACHABLE();
			}
			fe->fmt = 'd';
			value->ll = 0;
			break;
		}
	}
	else
	{
		switch(format)
		{
		case 'p':
			value->p = (char**)strtol(argp,&lastchar,10);
			break;
		case 'n':
		{
			Namval_t *np;
			np = nv_open(argp,sh.var_tree,NV_VARNAME|NV_NOARRAY);
			nv_unset(np,0);
			nv_onattr(np,NV_INTEGER);
			if (np->nvalue = new_of(int32_t,0))
				*((int32_t*)np->nvalue) = 0;
			nv_setsize(np,10);
			if(sizeof(int)==sizeof(int32_t))
				value->ip = np->nvalue;
			else
			{
				int32_t sl = 1;
				value->ip = (int*)(((char*)np->nvalue) + (*((char*)&sl) ? 0 : sizeof(int)));
			}
			break;
		}
		case 'q':
			if(fe->n_str)
			{
				const char *fp = mapformat(fe);
				if(fp)
				{
					format = *fp;
					if(fp[1])
						fe->flags |=SFFMT_ALTER;
				}
			}
			/* FALLTHROUGH */
		case 'b':
		case 's':
		case 'B':
		case 'H':
		case 'P':
		case 'R':
			fe->fmt = 's';
			fe->size = -1;
			if(format=='s' && fe->base>=0)
			{
				value->p = pp->nextarg;
				pp->nextarg = nullarg;
			}
			else
			{
				fe->base = -1;
				value->s = argp;
			}
			fe->flags &= ~SFFMT_LONG;
			break;
		case 'c':
			if(mbwide() && (n = mbsize(argp)) > 1)
			{
				fe->fmt = 's';
				fe->size = n;
				value->s = argp;
			}
			else if(fe->base >=0)
				value->s = argp;
			else
				value->c = *argp;
			fe->flags &= ~SFFMT_LONG;
			break;
		case 'o':
		case 'x':
		case 'X':
		case 'u':
		case 'U':
			longmax = LDBL_ULLONG_MAX;
			/* FALLTHROUGH */
		case '.':
			if(fe->size==2 && strchr("bcsqHPRQTZ",*fe->form))
			{
				value->ll = ((unsigned char*)argp)[0];
				break;
			}
			/* FALLTHROUGH */
		case 'd':
		case 'D':
		case 'i':
			switch(*argp)
			{
			case '\'':
			case '"':
				w = argp + 1;
				if(mbwide() && mbsize(w) > 1)
					value->ll = mbchar(w);
				else
					value->ll = *(unsigned char*)w++;
				if(w[0] && (w[0] != argp[0] || w[1]))
				{
					errormsg(SH_DICT,ERROR_warn(0),e_charconst,argp);
					pp->err = 1;
				}
				break;
			default:
				if(sh.bltinfun==b_printf && sh_isoption(SH_POSIX))
				{
					/* POSIX requires evaluating a number here, not an arithmetic expression */
					d = (Sfdouble_t)strtoll(argp,&lastchar,0);
					if(*lastchar)
						errormsg(SH_DICT,ERROR_exit(0),e_number,argp);
				}
				else
					d = sh_strnum(argp,&lastchar,0);
				if(d<longmin)
				{
					errormsg(SH_DICT,ERROR_warn(0),e_overflow,argp);
					pp->err = 1;
					d = longmin;
				}
				else if(d>longmax)
				{
					errormsg(SH_DICT,ERROR_warn(0),e_overflow,argp);
					pp->err = 1;
					d = longmax;
				}
				value->ll = (Sflong_t)d;
				if(lastchar == argp)
				{
					value->ll = *argp;
					lastchar = "";
				}
				break;
			}
			fe->size = sizeof(value->ll);
			break;
		case 'a':
		case 'e':
		case 'f':
		case 'g':
		case 'A':
		case 'E':
		case 'F':
		case 'G':
			switch(*argp)
			{
			    case '\'':
			    case '"':
				d = ((unsigned char*)argp)[1];
				if(argp[2] && (argp[2] != argp[0] || argp[3]))
				{
					errormsg(SH_DICT,ERROR_warn(0),e_charconst,argp);
					pp->err = 1;
				}
				break;
			    default:
				if(sh.bltinfun==b_printf && sh_isoption(SH_POSIX))
				{
					/* POSIX requires evaluating a number here, not an arithmetic expression */
					d = strtold(argp,&lastchar);
					if(*lastchar)
						errormsg(SH_DICT,ERROR_exit(0),e_number,argp);
				}
				else
					d = sh_strnum(argp,&lastchar,0);
				break;
			}
			if(SFFMT_LDOUBLE)
			{
				value->ld = d;
				fe->size = sizeof(value->ld);
			}
			else
			{
				value->d = d;
				fe->size = sizeof(value->d);
			}
			break;
		case 'Q':
			value->ll = (Sflong_t)strelapsed(argp,&lastchar,1);
			break;
		case 'T':
			value->ll = (Sflong_t)tmxdate(argp,&lastchar,TMX_NOW);
			break;
		default:
			value->ll = 0;
			fe->fmt = 'd';
			fe->size = sizeof(value->ll);
			errormsg(SH_DICT,ERROR_exit(1),e_formspec,format);
			UNREACHABLE();
		}
		if (format == '.')
			value->i = value->ll;
		if(*lastchar)
		{
			errormsg(SH_DICT,ERROR_warn(0),e_argtype,format);
			pp->err = 1;
		}
		pp->nextarg++;
	}
	switch(format)
	{
	case 'Z':
		fe->fmt = 'c';
		fe->base = -1;
		value->c = 0;
		break;
	case 'b':
		if((n=fmtvecho(value->s,pp))>=0)
		{
			if(pp->nextarg == nullarg)
			{
				pp->argsize = n;
				return -1;
			}
			value->s = stkptr(sh.stk,stktell(sh.stk));
			fe->size = n;
		}
		break;
	case 'B':
		if(!sh.strbuf2)
			sh.strbuf2 = sfstropen();
		fe->size = fmtbase64(sh.strbuf2,value->s, fe->flags&SFFMT_ALTER);
		value->s = sfstruse(sh.strbuf2);
		fe->flags |= SFFMT_SHORT;
		break;
	case 'H':
		value->s = fmthtml(value->s, fe->flags);
		break;
	case 'q':
		value->s = sh_fmtqf(value->s, !!(fe->flags & SFFMT_ALTER), fold);
		break;
	case 'P':
		s = fmtmatch(value->s);
		if(!s || *s==0)
		{
			errormsg(SH_DICT,ERROR_exit(1),e_badregexp,value->s);
			UNREACHABLE();
		}
		value->s = s;
		break;
	case 'R':
		s = fmtre(value->s);
		if(!s || *s==0)
		{
			errormsg(SH_DICT,ERROR_exit(1),e_badregexp,value->s);
			UNREACHABLE();
		}
		value->s = s;
		break;
	case 'Q':
		if (fe->n_str>0)
		{
			fe->fmt = 'd';
			fe->size = sizeof(value->ll);
		}
		else
		{
			value->s = fmtelapsed(value->ll, 1);
			fe->fmt = 's';
			fe->size = -1;
		}
		break;
	case 'T':
		if(fe->n_str>0)
		{
			n = fe->t_str[fe->n_str];
			fe->t_str[fe->n_str] = 0;
			value->s = fmttmx(fe->t_str, value->ll);
			fe->t_str[fe->n_str] = n;
		}
		else value->s = fmttmx(NULL, value->ll);
		fe->fmt = 's';
		fe->size = -1;
		break;
	}
	return 0;
}

/*
 * reload() is called when the caller wants to solve a fp[x] cache miss, i.e.,
 * a type mismatch between the cached type/value and the desired new type.
 * This keeps the fp[x] cache intact, it just fills up the new value (v)
 * with a conversion of the fp[x] type/value to the new type/value.
 * This is a single-use value; the conversion is not cached.
 * The conversion is handled by the extend() function that uses the current
 * nextarg (argv[]) and pushes it, i.e., *nextarg++.
 * To trick extend(), we back up nextarg, set nextarg to argn, do extend()
 * and restore nextarg.
 *
 * fmt==0 is a special case to handle indexed jumps like '%s $5s'.
 * In that case, argv[0] and argv[4] are consumed and nextarg push
 * to &argv[5] argv[1..3] is ignored.
 */
static int reload(int argn, char fmt, void* v, Sffmt_t* fe)
{
	struct printf*	pp = (struct printf*)fe;
	int		r;
	int		n;
	if(fmt == 0)
	{
		/* Set nextarg */
		n = 0;
		if(pp->nextarg != nullarg)
		{
			n = pp->nextarg - pp->argv0;
			pp->nextarg = pp->argv0;
			while(argn && *pp->nextarg)
				argn--, pp->nextarg++;
		}
		return n;
	}
	/*
	 * fmt!=0 ==> Late conversion on type mismatch on fp[x], i.e., %1$s %1$d
	 * fp[1-1].fmt='s' ==> %1$d wants an int, go convert.
	 */
	n = pp->nextarg - pp->argv0;
	pp->nextarg = pp->argv0 + argn;
	fe->fmt = fmt;
	r = extend(0,v,fe);
	pp->nextarg = pp->argv0 + n;
	return r;
}

/*
 * construct System V echo string out of <cp>
 * If there are no escape sequences, returns -1
 * Otherwise, puts null-terminated result on stack, but doesn't freeze it
 * returns length of output.
 */
static int fmtvecho(const char *string, struct printf *pp)
{
	const char *cp = string, *cpmax;
	int c;
	int offset = stktell(sh.stk);
	int chlen;
	if(mbwide())
	{
		while(1)
		{
			if ((chlen = mbsize(cp)) > 1)
				/* Skip over multibyte characters */
				cp += chlen;
			else if((c= *cp++)==0 || c == '\\')
				break;
		}
	}
	else
		while((c= *cp++) && (c!='\\'))
			;
	if(c==0)
		return -1;
	c = --cp - string;
	if(c>0)
		sfwrite(sh.stk,string,c);
	for(; c= *cp; cp++)
	{
		if (mbwide() && ((chlen = mbsize(cp)) > 1))
		{
			sfwrite(sh.stk,cp,chlen);
			cp +=  (chlen-1);
			continue;
		}
		if( c=='\\') switch(*++cp)
		{
			case 'E':
				c = '\033'; /* ASCII */
				break;
			case 'a':
				c = '\a';
				break;
			case 'b':
				c = '\b';
				break;
			case 'c':
				pp->cescape++;
				pp->nextarg = nullarg;
				goto done;
			case 'f':
				c = '\f';
				break;
			case 'n':
				c = '\n';
				break;
			case 'r':
				c = '\r';
				break;
			case 'v':
				c = '\v';
				break;
			case 't':
				c = '\t';
				break;
			case '\\':
				c = '\\';
				break;
			case '0':
				c = 0;
				cpmax = cp + 4;
				while(++cp<cpmax && *cp>='0' && *cp<='7')
				{
					c <<= 3;
					c |= (*cp-'0');
				}
				/* FALLTHROUGH */
			default:
				cp--;
		}
		sfputc(sh.stk,c);
	}
done:
	c = stktell(sh.stk)-offset;
	sfputc(sh.stk,0);
	stkseek(sh.stk,offset);
	return c;
}

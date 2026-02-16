/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2026 Contributors to ksh 93u+m           *
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
*                  Lev Kujawski <int21h@mailbox.org>                   *
*                                                                      *
***********************************************************************/
/*
 * export [-p] [arg...]
 * readonly [-p] [arg...]
 * typeset [options] [arg...]
 * autoload [options] [arg...]
 * compound [options] [arg...]
 * float [options] [arg...]
 * functions [options] [arg...]
 * integer [options] [arg...]
 * nameref [options] [arg...]
 * alias [-ptx] [arg...]
 * unalias [-a] [arg...]
 * hash [-r] [utility...]
 * builtin [-dlps] [-f file] [name...]
 * set [options] [name...]
 * unset [-fnv] [name...]
 *
 *   David Korn
 *   AT&T Labs
 *
 */

#include	"shopt.h"
#include	"defs.h"
#include	<error.h>
#include	"path.h"
#include	"name.h"
#include	"history.h"
#include	"builtins.h"
#include	"variables.h"
#if SHOPT_DYNAMIC
#include	<dlldefs.h>
#endif

/*
 * The first two fields must correspond with those in 'struct adata' in name.c and nvdisc.c
 * (those fields are used via a type conversion in scanfilter() in name.c)
 */
struct tdata
{
	Namval_t	*tp;
	const char	*wctname;
	Sfio_t  	*outfile;
	char    	*prefix;
	char    	*tname;
	char		*help;
	short     	aflag;
	short     	pflag;
	int     	argnum;
	int     	scanmask;
	Dt_t 		*scanroot;
	char    	**argnam;
	int		indent;
	int		noref;
};


static int	print_namval(Sfio_t*, Namval_t*, int, struct tdata*);
static void	print_attribute(Namval_t*,void*);
static void	print_all(Sfio_t*, Dt_t*, struct tdata*);
static void	print_scan(Sfio_t*, int, Dt_t*, int, struct tdata*);
static int	unall(int, char**, Dt_t*);
static int	setall(char**, int, Dt_t*, struct tdata*);
static void	pushname(Namval_t*,void*);
static void(*nullscan)(Namval_t*,void*);

/*
 * Note export and readonly are the same
 */
#if 0
    /* for the dictionary generator */
    int    b_export(int argc,char *argv[],Shbltin_t *context){}
#endif
int    b_readonly(int argc,char *argv[],Shbltin_t *context)
{
	int flag;
	char *command = argv[0];
	struct tdata tdata;
	NOT_USED(argc);
	NOT_USED(context);
	memset(&tdata,0,sizeof(tdata));
	tdata.aflag = '-';
	while((flag = optget(argv,*command=='e'?sh_optexport:sh_optreadonly))) switch(flag)
	{
		case 'p':
			tdata.prefix = command;
			break;
		case ':':
			errormsg(SH_DICT,2, "%s", opt_info.arg);
			break;
		case '?':
			/* self-doc: write to standard output */
			error(ERROR_USAGE|ERROR_OUTPUT, STDOUT_FILENO, "%s", opt_info.arg);
			return 0;
	}
	if(error_info.errors)
	{
		errormsg(SH_DICT,ERROR_usage(2),optusage(NULL));
		UNREACHABLE();
	}
	argv += (opt_info.index-1);
	if(*command=='r')
		flag = (NV_ASSIGN|NV_RDONLY|NV_VARNAME);
	else
		flag = (NV_ASSIGN|NV_EXPORT|NV_IDENT);
	if(!sh.prefix)
		sh.prefix = Empty;
	return setall(argv,flag,sh.var_tree, &tdata);
}

/*
 * 'alias' and 'hash' builtins
 */
#if 0
    /* for the dictionary generator */
    int    b_hash(int argc,char *argv[],Shbltin_t *context){}
#endif
int    b_alias(int argc,char *argv[],Shbltin_t *context)
{
	unsigned flag = NV_NOARRAY|NV_NOSCOPE|NV_ASSIGN;
	Dt_t *troot;
	int rflag=0, xflag=0, n;
	struct tdata tdata;
	NOT_USED(argc);
	NOT_USED(context);
	memset(&tdata,0,sizeof(tdata));
	troot = sh.alias_tree;
	if(*argv[0]=='h')
		flag |= NV_TAGGED;
	if(argv[1])
	{
		opt_info.offset = 0;
		opt_info.index = 1;
		*opt_info.option = 0;
		tdata.argnum = 0;
		tdata.aflag = *argv[1];
		while((n = optget(argv, *argv[0]=='h' ? sh_opthash : sh_optalias))) switch(n)
		{
		    case 'p':
			tdata.prefix = argv[0];
			tdata.pflag = 1;
			break;
		    case 't':
			flag |= NV_TAGGED;
			break;
		    case 'x':
			/* obsolete, ignored */
			xflag = 1;
			break;
		    case 'r':
			rflag=1;
			break;
		    case ':':
			if(sh.shcomp)
				return 2;  /* don't print usage info while shcomp is compiling */
			errormsg(SH_DICT,2, "%s", opt_info.arg);
			break;
		    case '?':
			if(sh.shcomp)
				return 0;
			/* self-doc: write to standard output */
			error(ERROR_USAGE|ERROR_OUTPUT, STDOUT_FILENO, "%s", opt_info.arg);
			return 0;
		}
		if(error_info.errors)
		{
			errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NULL));
			UNREACHABLE();
		}
		argv += (opt_info.index-1);
	}
	/* 'alias -t', 'hash' */
	if(flag&NV_TAGGED)
	{
		if(xflag)
			return 0;			/* do nothing for 'alias -tx' */
		if(tdata.pflag)
		{
			troot = sh_subtracktree(0);	/* use existing hash table */
			tdata.aflag = '+';		/* for 'alias -pt', don't add anything to the hash table */
		}
		else
		{
			troot = sh_subtracktree(1);	/* use hash table, creating a new one if needed */
			tdata.aflag = '-';		/* make setall() treat 'hash' like 'alias -t' */
		}
		if(rflag)				/* hash -r: clear hash table */
			nv_scan(troot,nv_rehash,NULL,NV_TAGGED,NV_TAGGED);
	}
	return setall(argv,flag,troot,&tdata);
}


#if 0
    /* for the dictionary generator */
    int    b_autoload(int argc,char *argv[],Shbltin_t *context){}
    int    b_compound(int argc,char *argv[],Shbltin_t *context){}
    int    b_float(int argc,char *argv[],Shbltin_t *context){}
    int    b_functions(int argc,char *argv[],Shbltin_t *context){}
    int    b_integer(int argc,char *argv[],Shbltin_t *context){}
    int    b_nameref(int argc,char *argv[],Shbltin_t *context){}
#endif
int    b_typeset(int argc,char *argv[],Shbltin_t *context)
{
	int		n, flag = NV_VARNAME|NV_ASSIGN;
	struct tdata	tdata;
	const char	*optstring = sh_opttypeset;
	Namdecl_t 	*ntp = (Namdecl_t*)context->ptr;
	Dt_t		*troot;
	int		isfloat=0, isadjust=0, shortint=0, sflag=0;

	memset(&tdata,0,sizeof(tdata));
	troot = sh.var_tree;
	if(ntp)					/* type declaration command added using 'typeset -T' or 'enum' */
	{
		tdata.tp = ntp->tp;
		opt_info.disc = (Optdisc_t*)ntp->optinfof;
		optstring = ntp->optstring;
	}
	else if(argv[0][0] != 't')		/* not <t>ypeset */
	{
		char **new_argv = stkalloc(sh.stk, (argc + 2) * sizeof(char*));
		error_info.id = new_argv[0] = SYSTYPESET->nvname;
		if(argv[0][0] == 'a')		/* <a>utoload == typeset -fu */
			new_argv[1] = "-fu";
		else if(argv[0][0] == 'c')	/* <c>ompound == typeset -C */
			new_argv[1] = "-C";
		else if(argv[0][1] == 'l')	/* f<l>oat == typeset -lE */
			new_argv[1] = "-lE";
		else if(argv[0][1] == 'u')	/* f<u>nctions == typeset -f */
			new_argv[1] = "-f";
		else if(argv[0][0] == 'i')	/* <i>nteger == typeset -li */
			new_argv[1] = "-li";
		else if(argv[0][0] == 'n')	/* <n>ameref == typeset -n */
			new_argv[1] = "-n";
		else
		{
			errormsg(SH_DICT, ERROR_PANIC, e_internal);
			UNREACHABLE();
		}
		for (n = 1; n <= argc; n++)
			new_argv[n + 1] = argv[n];
		argc++;
		argv = new_argv;
	}
	while((n = optget(argv,optstring)))
	{
		if(tdata.aflag==0)
			tdata.aflag = *opt_info.option;
		switch(n)
		{
			case 'a':
				flag |= NV_IARRAY;
				if(opt_info.arg && !(opt_info.arg[0]=='[' && opt_info.arg[strlen(opt_info.arg)-1]==']'))
				{
					opt_info.index--;
					goto endargs;
				}
				tdata.tname = opt_info.arg;
				break;
			case 'A':
				flag |= NV_ARRAY;
				break;
			case 'C':
				flag |= NV_COMVAR;
				break;
			case 'E':
				/* The following is for ksh88 compatibility */
				if(opt_info.offset && !strchr(argv[opt_info.index],'E'))
				{
					tdata.argnum = (int)opt_info.num;
					break;
				}
				/* FALLTHROUGH */
			case 'F':
			case 'X':
				if(!opt_info.arg || (tdata.argnum = opt_info.num) <0)
					tdata.argnum = (n=='X'?2*sizeof(Sfdouble_t):10);
				else if (tdata.argnum==0)
					tdata.argnum = NV_FLTSIZEZERO;
				isfloat = 1;
				if(shortint)
				{
					shortint = 0;
					flag &= ~NV_INT16;
				}
				if(n=='E')
				{
					flag &= ~NV_HEXFLOAT;
					flag |= NV_EXPNOTE;
				}
				else if(n=='X')
				{
					flag &= ~NV_EXPNOTE;
					flag |= NV_HEXFLOAT;
				}
				else
					/* n=='F' Remove possible collision with NV_UNSIGN/NV_HEXFLOAT
					   and allow it to not be covered up by -E */
					flag &= ~(NV_HEXFLOAT|NV_EXPNOTE);
				break;
			case 'b':
				flag |= NV_BINARY;
				break;
			case 'm':
				flag |= NV_MOVE;
				break;
			case 'n':
				flag &= ~NV_VARNAME;
				flag |= (NV_REF|NV_IDENT);
				break;
			case 'H':
				flag |= NV_HOST;
				break;
			case 'T':
				flag |= NV_TYPE;
				tdata.prefix = opt_info.arg;
				break;
			case 'L': case 'Z': case 'R':
				if(tdata.argnum==0)
					tdata.argnum = (int)opt_info.num;
				if(tdata.argnum < 0)
				{
					errormsg(SH_DICT,ERROR_exit(1), e_badfield, tdata.argnum);
					UNREACHABLE();
				}
				isadjust = 1;
				if(n=='Z')
					flag |= NV_ZFILL;
				else
				{
					flag &= ~(NV_LJUST|NV_RJUST);
					flag |= (n=='L'?NV_LJUST:NV_RJUST);
				}
				break;
			case 'M':
				if((tdata.wctname = opt_info.arg) && !nv_mapchar(NULL,tdata.wctname))
				{
					errormsg(SH_DICT, ERROR_exit(1),e_unknownmap, tdata.wctname);
					UNREACHABLE();
				}
				if(tdata.wctname && strcmp(tdata.wctname,e_tolower)==0)
					flag |= NV_UTOL;
				else
					flag |= NV_LTOU;
				if(!tdata.wctname)
					flag |= NV_UTOL;
				break;
			case 'f':
				flag &= ~(NV_VARNAME|NV_ASSIGN);
				troot = sh.fun_tree;
				break;
			case 'i':
				if(!opt_info.arg || (tdata.argnum = opt_info.num) <2 || tdata.argnum >64)
					tdata.argnum = 10;
				if(isfloat)
				{
					isfloat = 0;
					flag &= ~(NV_HEXFLOAT|NV_EXPNOTE);
				}
				if(shortint)
				{
					flag &= ~NV_LONG;
					flag |= NV_INT16;
				}
				else
					flag |= NV_INTEGER;
				break;
			case 'l':
				if(shortint)
				{
					shortint = 0;
					/* Turn off the NV_INT16 bits except the NV_INTEGER bit */
					flag &= ~(NV_INT16 & ~NV_INTEGER);
				}
				tdata.wctname = e_tolower;
				flag |= NV_UTOL;
				break;
			case 'p':
				tdata.prefix = argv[0];
				tdata.pflag = 1;
				flag &= ~NV_ASSIGN;
				break;
			case 'r':
				flag |= NV_RDONLY;
				break;
			case 'S':
				sflag=1;
				break;
			case 'h':
				tdata.help = opt_info.arg;
				break;
			case 's':
				if(!isfloat)
				{
					shortint=1;
					if(flag&NV_INTEGER)
					{
						flag &= ~NV_LONG;
						flag |= NV_INT16;
					}
				}
				break;
			case 't':
				flag |= NV_TAGGED;
				break;
			case 'u':
				if(!isfloat)
				{
					tdata.wctname = e_toupper;
					flag |= NV_LTOU;
				}
				break;
			case 'x':
				flag &= ~NV_VARNAME;
				flag |= (NV_EXPORT|NV_IDENT);
				break;
			case 'g':
				flag |= NV_GLOBAL;
				break;
			case ':':
				errormsg(SH_DICT,2, "%s", opt_info.arg);
				break;
			case '?':
				opt_info.disc = 0;
				/* self-doc: write to standard output */
				error(ERROR_USAGE|ERROR_OUTPUT, STDOUT_FILENO, "%s", opt_info.arg);
				return 0;
		}
	}
endargs:
	argv += opt_info.index;
	opt_info.disc = 0;
	/* handle argument of + and - specially */
	if(*argv && argv[0][1]==0 && (*argv[0]=='+' || *argv[0]=='-'))
		tdata.aflag = *argv[0];
	else
		argv--;
	if((flag&NV_ZFILL) && !(flag&NV_LJUST))
		flag |= NV_RJUST;
	if((isfloat || flag&NV_INTEGER) && isadjust)
	{
		errormsg(SH_DICT,2,e_optincompat2,"-i/-F/-E/-X","-L/-R/-Z");
		error_info.errors++;
	}
	if((flag&NV_BINARY) && (flag&(NV_LJUST|NV_UTOL|NV_LTOU)))
	{
		errormsg(SH_DICT,2,e_optincompat2,"-b","-L/-u/-l");
		error_info.errors++;
	}
	if((flag&NV_MOVE) && (flag&~(NV_MOVE|NV_VARNAME|NV_ASSIGN)))
	{
		errormsg(SH_DICT,2,e_optincompat1,"-m");
		error_info.errors++;
	}
	if((flag&NV_REF) && (flag&~(NV_REF|NV_IDENT|NV_ASSIGN|NV_GLOBAL)))
	{
		errormsg(SH_DICT,2,e_optincompat2,"-n","other options except -g");
		error_info.errors++;
	}
	if((flag&NV_TYPE) && (flag&~(NV_TYPE|NV_VARNAME|NV_ASSIGN)))
	{
		errormsg(SH_DICT,2,e_optincompat1,"-T");
		error_info.errors++;
	}
	if(troot==sh.fun_tree && ((isfloat || flag&~(NV_FUNCT|NV_TAGGED|NV_EXPORT|NV_LTOU))))
	{
		errormsg(SH_DICT,2,e_optincompat2,"-f","other options except -t and -u");
		error_info.errors++;
	}
	if(sflag && troot==sh.fun_tree)
	{
		/* static function */
		sflag = 0;
		flag |= NV_STATICF;
	}
	if(error_info.errors)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s", optusage(NULL));
		UNREACHABLE();
	}
	if(sizeof(char*)<8 && tdata.argnum > SHRT_MAX)
	{
		errormsg(SH_DICT,ERROR_exit(2),"option argument cannot be greater than %d",SHRT_MAX);
		UNREACHABLE();
	}
	if((flag&NV_GLOBAL) && sh.mktype)
	{
		errormsg(SH_DICT,ERROR_exit(2),"-g: type members cannot be global");
		UNREACHABLE();
	}
	if(isfloat)
		flag |= NV_DOUBLE;
	if(sflag)
	{
		if(sh.mktype)
			flag |= NV_REF|NV_TAGGED;
		else if(!sh.typeinit)
			flag |= NV_STATIC|NV_IDENT;
	}
	if(sh.fn_depth && !tdata.pflag)
		flag |= NV_NOSCOPE;
	if(tdata.help)
		tdata.help = sh_strdup(tdata.help);
	if(flag&NV_TYPE)
	{
		Stk_t *stkp = sh.stk;
#if SHOPT_NAMESPACE
		int off = 0;
#endif /* SHOPT_NAMESPACE */
		int offset = stktell(stkp);
		if(!tdata.prefix)
			return sh_outtype(sfstdout);
		sfputr(stkp,NV_CLASS,-1);
#if SHOPT_NAMESPACE
		if(sh.namespace)
		{
			off = stktell(stkp)+1;
			sfputr(stkp,nv_name(sh.namespace),'.');
		}
		else
#endif /* SHOPT_NAMESPACE */
		if(NV_CLASS[sizeof(NV_CLASS)-2]!='.')
			sfputc(stkp,'.');
		sfputr(stkp,tdata.prefix,0);
		tdata.tp = nv_open(stkptr(stkp,offset),sh.var_tree,NV_VARNAME|NV_NOARRAY);
#if SHOPT_NAMESPACE
		if(!tdata.tp && off)
		{
			*stkptr(stkp,off)=0;
			tdata.tp = nv_open(stkptr(stkp,offset),sh.var_tree,NV_VARNAME|NV_NOARRAY);
		}
#endif /* SHOPT_NAMESPACE */
		stkseek(stkp,offset);
		if(!tdata.tp)
		{
			errormsg(SH_DICT,ERROR_exit(1),"%s: unknown type",tdata.prefix);
			UNREACHABLE();
		}
		else if(nv_isnull(tdata.tp) && sh.envlist)   /* only create a type command if there were assignment(s) */
			nv_newtype(tdata.tp);
		tdata.tp->nvmeta = tdata.help;
		flag &= ~NV_TYPE;
		if(nv_isattr(tdata.tp,NV_TAGGED))
		{
			nv_offattr(tdata.tp,NV_TAGGED);
			return 0;
		}
	}
	else if(tdata.aflag==0 && ntp && ntp->tp)
		tdata.aflag = '-';
	if(!sh.mktype)
		tdata.help = 0;
	if(tdata.aflag=='+' && (flag&(NV_ARRAY|NV_IARRAY|NV_COMVAR)) && argv[1])
	{
		errormsg(SH_DICT,ERROR_exit(1),e_nounattr);
		UNREACHABLE();
	}
	return setall(argv,flag,troot,&tdata);
}

static void print_value(Sfio_t *iop, Namval_t *np, struct tdata *tp)
{
	char		 *name;
	int		aflag=tp->aflag;
	Namval_t	*table;
	if(nv_isnull(np))
	{
		if(!np->nvflag)
			return;
		aflag = '+';
	}
	else if(nv_istable(np))
	{
		Dt_t	*root = sh.last_root;
		Namval_t *nsp = sh.namespace;
		char *cp;
		if(!tp->pflag)
			return;
		cp = name = nv_name(np);
		if(*name=='.')
			name++;
		if(tp->indent)
			sfnputc(iop,'\t',tp->indent);
		sfprintf(iop,"namespace %s\n", name);
		if(tp->indent)
			sfnputc(iop,'\t',tp->indent);
		sfprintf(iop,"{\n", name);
		tp->indent++;
		/* output types from namespace */
		sh.namespace = 0;
		sh.prefix = nv_name(np)+1;
		sh_outtype(iop);
		sh.prefix = 0;
		sh.namespace = np;
		sh.last_root = root;
		/* output variables from namespace */
		print_scan(iop,NV_NOSCOPE,nv_dict(np),aflag=='+',tp);
		tp->wctname = cp;
		sh.namespace = 0;
		/* output functions from namespace */
		print_scan(iop,NV_FUNCTION|NV_NOSCOPE,sh.fun_tree,aflag=='+',tp);
		tp->wctname = 0;
		sh.namespace = nsp;
		if(--tp->indent)
			sfnputc(iop,'\t',tp->indent);
		sfwrite(iop,"}\n",2);
		return;
	}
	if(tp->prefix && *tp->prefix=='a' && !nv_isattr(np,NV_TAGGED))
		sfprintf(iop,"%s ", tp->prefix);
	table = sh.last_table;
	sfputr(iop,nv_name(np),aflag=='+'?'\n':'=');
	sh.last_table = table;
	if(aflag=='+')
		return;
	if(nv_isarray(np) && nv_arrayptr(np))
	{
		nv_outnode(np,iop,-1,0);
		sfwrite(iop,")\n",2);
	}
	else
	{
		if(nv_isvtree(np))
			nv_onattr(np,NV_EXPORT);
		if(!(name = nv_getval(np)))
			name = Empty;
		if(!nv_isvtree(np))
			name = sh_fmtq(name);
		sfputr(iop,name,'\n');
	}
}

static int     setall(char **argv,int flag,Dt_t *troot,struct tdata *tp)
{
	char *name;
	char *last = 0;
	int nvflags=(flag&(NV_ARRAY|NV_NOARRAY|NV_VARNAME|NV_IDENT|NV_ASSIGN|NV_STATIC|NV_MOVE));
	int r=0, ref=0, comvar=(flag&NV_COMVAR),iarray=(flag&NV_IARRAY);
	Dt_t *save_vartree = NULL;
#if SHOPT_NAMESPACE
	Namval_t *save_namespace = NULL;
#endif
	if(flag&NV_GLOBAL)
	{
		save_vartree = sh.var_tree;
		troot = sh.var_tree = sh.var_base;
#if SHOPT_NAMESPACE
		save_namespace = sh.namespace;
		sh.namespace = NULL;
#endif
	}
	if(!sh.prefix)
	{
		if(!tp->pflag)
			nvflags |= NV_NOSCOPE;
	}
	else if(*sh.prefix==0)
		sh.prefix = 0;
	if(*argv[0]=='+')
		nvflags |= NV_NOADD;
	flag &= ~(NV_NOARRAY|NV_NOSCOPE|NV_VARNAME|NV_IDENT|NV_STATIC|NV_COMVAR|NV_IARRAY);
	if(argv[1])
	{
		if(flag&NV_REF)
		{
			flag &= ~NV_REF;
			ref=1;
			if(tp->aflag!='-')
				nvflags |= NV_NOREF;
		}
		if(tp->pflag)
			nvflags |= (NV_NOREF|NV_NOADD|NV_NOFAIL);
		while(name = *++argv)
		{
			unsigned newflag;
			Namval_t *np;
			Namarr_t	*ap=0;
			Namval_t	*mp;
			unsigned curflag;
			if(troot == sh.fun_tree)
			{
				/*
				 * functions can be exported or
				 * traced but not set
				 */
				flag &= ~NV_ASSIGN;
				if(flag&NV_LTOU)
				{
					/* Function names cannot be special builtin */
					if((np=nv_search(name,sh.bltin_tree,0)) && nv_isattr(np,BLT_SPC))
					{
						errormsg(SH_DICT,ERROR_exit(1),e_badfun,name);
						UNREACHABLE();
					}
#if SHOPT_NAMESPACE
					if(sh.namespace)
						np = sh_fsearch(name,NV_ADD|NV_NOSCOPE);
					else
#endif /* SHOPT_NAMESPACE */
					np = nv_open(name,sh_subfuntree(1),NV_NOARRAY|NV_IDENT|NV_NOSCOPE);
				}
				else
				{
					if(sh.prefix)
					{
						sfprintf(sh.strbuf,"%s.%s",sh.prefix,name);
						name = sfstruse(sh.strbuf);
					}
#if SHOPT_NAMESPACE
					np = 0;
					if(sh.namespace)
						np = sh_fsearch(name,NV_NOSCOPE);
					if(!np)
#endif /* SHOPT_NAMESPACE */
					{
						if(np=nv_search(name,troot,0))
						{
							if(!is_afunction(np))
								np = 0;
						}
						else if(strncmp(name,".sh.math.",9)==0 && sh_mathstd(name+9))
							continue;
					}
				}
				if(np && ((flag&NV_LTOU) || !nv_isnull(np) || nv_isattr(np,NV_LTOU)))
				{
					if(flag==0 && !tp->help)
					{
						print_namval(sfstdout,np,tp->aflag=='+',tp);
						continue;
					}
					if(sh.subshell && !sh.subshare)
						sh_subfork();
					if(tp->aflag=='-')
						nv_onattr(np,flag|NV_FUNCTION);
					else if(tp->aflag=='+')
						nv_offattr(np,flag);
				}
				else
					r++;
				if(tp->help)
				{
					int offset = stktell(sh.stk);
					if(!np)
					{
						sfputr(sh.stk,sh.prefix,'.');
						sfputr(sh.stk,name,0);
						np = nv_search(stkptr(sh.stk,offset),troot,0);
						stkseek(sh.stk,offset);
					}
					if(np && np->nvalue)
						((struct Ufunction*)np->nvalue)->help = tp->help;
				}
				continue;
			}
			/* tracked alias */
			if(troot==sh.track_tree && tp->aflag=='-')
			{
				Pathcomp_t *pp;
				sh_offstate(SH_DEFPATH);  /* 'command -p hash foo' should work to create 'foo=/bin/foo' */
				if(!(pp = path_absolute(name,NULL,2)))
					errormsg(SH_DICT,ERROR_exit(0),e_found,name);
				path_settrackedalias(name,pp);
				r |= !pp;
				continue;
			}
			if(troot==sh.alias_tree && sh.subshell && !sh.subshare && strchr(name,'='))
				sh_subfork();	/* avoid affecting the parent shell's alias table */
			np = nv_open(name,troot,nvflags|((nvflags&NV_ASSIGN)?0:NV_ARRAY)|((iarray|(nvflags&(NV_REF|NV_NOADD)==NV_REF))?NV_FARRAY:0));
			if(!np || (troot==sh.track_tree && nv_isattr(np,NV_NOALIAS)))
			{
				if(troot==sh.alias_tree || troot==sh.track_tree)
				{
					if(!sh.shcomp)
						sfprintf(sfstderr,sh_translate(troot==sh.alias_tree ? e_noalias: e_notrackedalias),name);
					r++;
				}
				continue;
			}
			if(np->nvflag&NV_RDONLY && !tp->pflag
			&& (tp->aflag=='+' || flag & ~(NV_ASSIGN|NV_RDONLY|NV_EXPORT)))	/* allow readonly/export on readonly vars */
			{
				errormsg(SH_DICT,ERROR_exit(1),e_readonly,nv_name(np));
				UNREACHABLE();
			}
			if(nv_isnull(np) && !nv_isarray(np) && nv_isattr(np,NV_NOFREE))
				nv_offattr(np,NV_NOFREE);
			else if(tp->tp && !nv_isattr(np,NV_MINIMAL|NV_EXPORT) && (mp = np->nvmeta) && (ap = nv_arrayptr(mp)) && (ap->nelem & ARRAY_TREE))
			{
				errormsg(SH_DICT,ERROR_exit(1),e_typecompat,nv_name(np));
				UNREACHABLE();
			}
			else if((ap=nv_arrayptr(np)) && nv_aindex(np)>0 && ap->nelem==1 && nv_getval(np)==Empty)
			{
				ap->nelem++;
				nv_unset(np,0);
				ap->nelem--;
			}
			else if(iarray && ap && ap->fun)
			{
				errormsg(SH_DICT,ERROR_exit(1),"cannot change associative array %s to indexed array",nv_name(np));
				UNREACHABLE();
			}
			else if( (iarray||(flag&NV_ARRAY)) && nv_isvtree(np) && !nv_type(np))
				nv_unset(np,NV_EXPORT);
			if(tp->pflag)
			{
				if(!nv_istable(np))
					nv_attribute(np,sfstdout,tp->prefix,1);
				print_value(sfstdout,np,tp);
				continue;
			}
			if(flag==NV_ASSIGN && !ref && tp->aflag!='-' && !strchr(name,'='))
			{
				if(troot!=sh.var_tree && (nv_isnull(np) || !print_namval(sfstdout,np,0,tp)))
				{
					if(!sh.shcomp)
						sfprintf(sfstderr,sh_translate(e_noalias),name);
					r++;
				}
				if(!comvar && !iarray)
					continue;
			}
			if(troot==sh.var_tree)
			{
				if(sh.subshell && !sh.subshare)
				{
					/*
					 * Create local scope for virtual subshell. Variables with discipline functions
					 * (LC_*, LINENO, etc.) need to be cloned, as moving them will remove the discipline.
					 */
					if((flag&NV_ARRAY) && !sh.envlist && !nv_isnull(np))
						sh_subfork();	/* work around https://github.com/ksh93/ksh/issues/409 */
					else
						sh_assignok(np, !nv_isattr(np,NV_NODISC|NV_ARRAY) && !nv_isvtree(np));
				}
				if(iarray)
				{
					if(tp->tname)
						nv_atypeindex(np,tp->tname+1);
					else if(nv_isnull(np))
						nv_onattr(np,NV_ARRAY|(comvar?NV_NOFREE:0));
					else
					{
						if(ap && comvar)
							ap->nelem |= ARRAY_TREE;
						nv_putsub(np, NULL, 0);
					}
				}
				else if(nvflags&NV_ARRAY)
				{
					if(comvar)
					{
						Namarr_t *ap=nv_arrayptr(np);
						if(ap)
							ap->nelem |= ARRAY_TREE;
						else
						{
							nv_unset(np,NV_RDONLY);
							nv_onattr(np,NV_NOFREE);
						}
					}
					nv_setarray(np,nv_associative);
				}
				else if(comvar && !nv_isvtree(np) && !nv_rename(np,flag|NV_COMVAR))
					nv_setvtree(np);
			}
			if(flag&NV_MOVE)
			{
				nv_rename(np, flag);
				continue;
			}
			if(tp->tp && nv_type(np)!=tp->tp)
			{
				nv_settype(np,tp->tp,tp->aflag=='-'?0:NV_APPEND);
				flag = (np->nvflag&NV_NOCHANGE);
			}
			flag &= ~NV_ASSIGN;
			if(last=strchr(name,'='))
				*last = 0;
			if (sh.typeinit)
				continue;
			curflag = np->nvflag;
			if(!(flag&NV_INTEGER) && (flag&(NV_LTOU|NV_UTOL)))
			{
				Namfun_t *fp;
				char  *cp;
				if(!tp->wctname)
				{
					errormsg(SH_DICT,ERROR_exit(1),e_mapchararg,nv_name(np));
					UNREACHABLE();
				}
				cp = (char*)nv_mapchar(np,0);
				if(fp=nv_mapchar(np,tp->wctname))
				{
					if(tp->aflag=='+')
					{
						if(cp && strcmp(cp,tp->wctname)==0)
						{
							nv_disc(np,fp,NV_POP);
							if(!(fp->nofree&1))
								free(fp);
							nv_offattr(np,flag&(NV_LTOU|NV_UTOL));
						}
					}
					else if(!cp || strcmp(cp,tp->wctname))
					{
						nv_disc(np,fp,NV_LAST);
						nv_onattr(np,flag&(NV_LTOU|NV_UTOL));
					}
				}
			}
			if (tp->aflag == '-')
			{
				if((flag&NV_EXPORT) && (strchr(name,'.') || nv_isvtree(np)))
				{
					errormsg(SH_DICT,ERROR_exit(1),e_badexport,name);
					UNREACHABLE();
				}
				newflag = curflag;
				if(flag&~NV_NOCHANGE)
					newflag &= NV_NOCHANGE;
				newflag |= flag;
				if (flag & (NV_LJUST|NV_RJUST))
				{
					if(!(flag&NV_RJUST))
						newflag &= ~NV_RJUST;
					else if(!(flag&NV_LJUST))
						newflag &= ~NV_LJUST;
				}
			}
			else
				newflag = curflag & ~flag;
			if (tp->aflag && (tp->argnum || (curflag!=newflag)))
			{
				if(np==SH_LEVELNOD)
					return r;
				if(sh.subshell)
					sh_assignok(np,1);
				if(troot!=sh.var_tree)
					nv_setattr(np,newflag&~NV_ASSIGN);
				else
				{
					if(np->nvfun && !nv_isarray(np) && name[strlen(name)-1]=='.')
						newflag |= NV_NODISC;
					if(flag&NV_RDONLY && !tp->argnum && !(flag&(NV_INTEGER|NV_BINARY)) && !(flag&(NV_LJUST|NV_RJUST|NV_ZFILL)))
						/* New requested attribute(s) are readonly, have a provided or defaulted size of 0, and are
						   not a string justification nor numeric. Justified or binary strings can have a size of 0. */
						nv_newattr(np, newflag&~NV_ASSIGN, np->nvsize);
					else
						nv_newattr(np, newflag&~NV_ASSIGN, tp->argnum);
				}
			}
			if(tp->help && !nv_isattr(np,NV_MINIMAL|NV_EXPORT))
			{
				np->nvmeta = tp->help;
				nv_onattr(np,NV_EXPORT);
			}
			if(last)
				*last = '=';
			/* set or unset references */
			if(ref)
			{
				if(tp->aflag=='-')
				{
					Dt_t *hp=0;
					if(nv_isattr(np,NV_PARAM) && sh.st.prevst)
					{
						if(!(hp=(Dt_t*)sh.st.prevst->save_tree))
							hp = dtvnext(sh.var_tree);
					}
					if(sh.mktype)
						nv_onattr(np,NV_REF|NV_FUNCT);
					else
						nv_setref(np,hp,NV_VARNAME);
				}
				else
					nv_unref(np);
			}
		}
	}
	else
	{
		if(sh.prefix)
			errormsg(SH_DICT,2, e_subcomvar,sh.prefix);
		if(tp->aflag)
		{
			if(troot==sh.fun_tree)
			{
				flag |= NV_FUNCTION;
				tp->prefix = 0;
			}
			else if(troot==sh.var_tree)
			{
				flag |= (nvflags&NV_ARRAY);
				if(iarray)
					flag |= NV_ARRAY|NV_IARRAY;
				if(comvar)
					flag |= NV_TABLE;
				if(!(flag&~NV_ASSIGN))
					tp->noref = 1;
			}
			if((flag&(NV_UTOL|NV_LTOU)) ==(NV_UTOL|NV_LTOU))
			{
				print_scan(sfstdout,flag&~NV_UTOL,troot,tp->aflag=='+',tp);
				flag &= ~NV_LTOU;
			}
			print_scan(sfstdout,flag,troot,tp->aflag=='+',tp);
			if(tp->noref)
			{
				tp->noref = 0;
				print_scan(sfstdout,flag|NV_REF,troot,tp->aflag=='+',tp);
			}
		}
		else if(troot==sh.alias_tree)
			print_scan(sfstdout,0,troot,0,tp);
		else
			print_all(sfstdout,troot,tp);
		sfsync(sfstdout);
	}
	/* This is to handle cases where more than 255 non-existent
	   aliases were passed to the alias command. */
	if(r>255)
	{
		r &= SH_EXITMASK;
		if(r==0)
			r = 1;  /* ensure the exit status is at least 1 */
	}
	if(flag&NV_GLOBAL)
	{
		sh.var_tree = save_vartree;
#if SHOPT_NAMESPACE
		sh.namespace = save_namespace;
#endif
	}
	return r;
}

#if SHOPT_DYNAMIC

typedef void (*Libinit_f)(int,void*);

#define GROWLIB	4

Libcomp_t		*liblist;
static int		nlib;
static int		maxlib;

/*
 * add library to loaded list
 * call (*lib_init)() on first load if defined
 * always move to head of search list
 * return: 0: already loaded 1: first load
 */
int sh_addlib(void* dll, char* name, Pathcomp_t* pp)
{
	int		n;
	int		r;
	Libinit_f	initfn;
	Shbltin_t	*sp = &sh.bltindata;

	sp->nosfio = 0;
	for (n = r = 0; n < nlib; n++)
	{
		if (r)
			liblist[n-1] = liblist[n];
		else if (liblist[n].dll == dll)
			r++;
	}
	if (r)
		nlib--;
	else if ((initfn = (Libinit_f)dlllook(dll, "lib_init")))
		(*initfn)(0,sp);
	if (nlib >= maxlib)
	{
		maxlib += GROWLIB;
		liblist = sh_newof(liblist, Libcomp_t, maxlib+1, 0);
	}
	liblist[nlib].dll = dll;
	liblist[nlib].attr = (sp->nosfio?BLT_NOSFIO:0);
	if (name)
		liblist[nlib].lib = sh_strdup(name);
	if (pp)
	{
		liblist[nlib].dev = pp->dev;
		liblist[nlib].ino = pp->ino;
	}
	nlib++;
	return !r;
}

Shbltin_f sh_getlib(char* sym, Pathcomp_t* pp)
{
	int	n;

	for (n = 0; n < nlib; n++)
		if (liblist[n].ino == pp->ino && liblist[n].dev == pp->dev)
			return (Shbltin_f)dlllook(liblist[n].dll, sym);
	return 0;
}

#endif /* SHOPT_DYNAMIC */

/*
 * add change or list built-ins
 * adding builtins requires dlopen() interface
 */
int	b_builtin(int argc,char *argv[],Shbltin_t *context)
{
	char *arg=0, *name;
	int n, r=0, flag=0;
	Namval_t *np;
	long dlete=0;
	struct tdata tdata;
	Shbltin_f addr;
	Stk_t	*stkp;
	char *errmsg;
#if SHOPT_DYNAMIC
	int list = 0;
#endif
	NOT_USED(argc);
	NOT_USED(context);
	memset(&tdata,0,sizeof(tdata));
	stkp = sh.stk;
	if(!sh.pathlist)
		path_absolute(argv[0],NULL,0);
	while (n = optget(argv,sh_optbuiltin)) switch (n)
	{
	    case 's':
		flag = BLT_SPC;
		break;
	    case 'd':
		dlete=1;
		break;
	    case 'f':
#if SHOPT_DYNAMIC
		arg = opt_info.arg;
#else
		errormsg(SH_DICT,2, "adding built-ins not supported");
		error_info.errors++;
#endif /* SHOPT_DYNAMIC */
		break;
	    case 'l':
#if SHOPT_DYNAMIC
		list = 1;
#endif
	        break;
	    case 'p':
		tdata.prefix = argv[0];
		break;
	    case ':':
		errormsg(SH_DICT,2, "%s", opt_info.arg);
		break;
	    case '?':
		/* self-doc: write to standard output */
		error(ERROR_USAGE|ERROR_OUTPUT, STDOUT_FILENO, "%s", opt_info.arg);
		return 0;
	}
	argv += opt_info.index;
	if(error_info.errors)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s", optusage(NULL));
		UNREACHABLE();
	}
	if(arg || *argv)
	{
		if(sh_isoption(SH_RESTRICTED))
		{
			errormsg(SH_DICT,ERROR_exit(1),e_restricted,argv[-opt_info.index]);
			UNREACHABLE();
		}
		if(sh.subshell && !sh.subshare)
			sh_subfork();
	}
#if SHOPT_DYNAMIC
	if(arg)
	{
		unsigned long ver;
		char path[PATH_MAX];
		void *library;
		if(!(library = dllplugin(SH_ID, arg, NULL, SH_PLUGIN_VERSION, &ver, RTLD_LAZY, path, sizeof(path))))
		{
			errormsg(SH_DICT,ERROR_exit(0),"%s: %s",arg,dllerror(0));
			return 1;
		}
		if(list)
			sfprintf(sfstdout, "%s %08lu %s\n", arg, ver, path);
		sh_addlib(library,arg,NULL);
	}
	else
#endif /* SHOPT_DYNAMIC */
	if(*argv==0 && !dlete)
	{
#if SHOPT_DYNAMIC
		if(tdata.prefix)
			for(n = 0; n < nlib; n++)
				sfprintf(sfstdout, "%s -f %s\n", tdata.prefix, liblist[n].lib);
#endif
		print_scan(sfstdout, flag, sh.bltin_tree, 1, &tdata);
		return 0;
	}
	r = 0;
	flag = stktell(stkp);
	while(arg = *argv)
	{
		if(tdata.prefix)
		{
			if(np = nv_search(arg,sh.bltin_tree,0))
				sfprintf(sfstdout,"%s %s\n",tdata.prefix,arg);
			else
			{
				errormsg(SH_DICT,ERROR_exit(0),"%s: %s",*argv,"not found");
				r = 1;
			}
			argv++;
			continue;
		}
		name = path_basename(arg);
		sfwrite(stkp,"b_",2);
		sfputr(stkp,name,0);
		errmsg = 0;
		addr = 0;
#if SHOPT_DYNAMIC
		if(dlete || liblist)
			for(n=(nlib?nlib:dlete); --n>=0;)
			{
				if(!dlete && !liblist[n].dll)
					continue;
				if(dlete || (addr = (Shbltin_f)dlllook(liblist[n].dll,stkptr(stkp,flag))))
#else
		if(dlete)
			for(n=dlete; --n>=0;)
			{
				if(dlete)
#endif /* SHOPT_DYNAMIC */
				{
					if(np = sh_addbuiltin(arg, addr,pointerof(dlete)))
					{
						if(dlete || nv_isattr(np,BLT_SPC))
							errmsg = "restricted name";
#if SHOPT_DYNAMIC
						else
							nv_onattr(np,liblist[n].attr);
#endif /* SHOPT_DYNAMIC */
					}
					break;
				}
			}
		if(!addr && (np = nv_search(arg,sh.bltin_tree,0)))
		{
			if(nv_isattr(np,BLT_SPC))
				errmsg = "restricted name";
			addr = funptr(np);
		}
		if(!dlete && !addr && !(np=sh_addbuiltin(arg,NULL,0)))
			errmsg = "not found";
		if(errmsg)
		{
			errormsg(SH_DICT,ERROR_exit(0),"%s: %s",*argv,errmsg);
			r = 1;
		}
		stkseek(stkp,flag);
		argv++;
	}
	return r;
}

int    b_set(int argc,char *argv[],Shbltin_t *context)
{
	struct tdata tdata;
	NOT_USED(context);
	memset(&tdata,0,sizeof(tdata));
	tdata.prefix=0;
	if(argv[1])
	{
		if(sh_argopts(argc,argv) < 0)
			return 0;  /* self-doc was written to standard output */
		if(sh_isoption(SH_VERBOSE))
			sh_onstate(SH_VERBOSE);
		else
			sh_offstate(SH_VERBOSE);
	}
	else
		/* scan name chain and print */
		print_scan(sfstdout,0,sh.var_tree,0,&tdata);
	return 0;
}

/*
 * The removing of Shell variable names, aliases, and functions
 * is performed here.
 * Unset functions with unset -f
 * Non-existent items being deleted give non-zero exit status
 */

int    b_unalias(int argc,char *argv[],Shbltin_t *context)
{
	NOT_USED(context);
	return unall(argc,argv,sh.alias_tree);
}

int    b_unset(int argc,char *argv[],Shbltin_t *context)
{
	NOT_USED(context);
	return unall(argc,argv,sh.var_tree);
}

static int unall(int argc, char **argv, Dt_t *troot)
{
	Namval_t *np;
	const char *name;
	volatile int r;
	Dt_t	*dp;
	int nflag=0,all=0,isfun,jmpval;
	struct checkpt buff;
	NOT_USED(argc);
	if(troot==sh.alias_tree)
		name = sh_optunalias;
	else
		name = sh_optunset;
	while(r = optget(argv,name)) switch(r)
	{
		case 'f':
			troot = sh_subfuntree(1);
			break;
		case 'a':
			all=1;
			break;
		case 'n':
			nflag = NV_NOREF;
			/* FALLTHROUGH */
		case 'v':
			troot = sh.var_tree;
			break;
		case ':':
			errormsg(SH_DICT,2, "%s", opt_info.arg);
			break;
		case '?':
			/* self-doc: write to standard output */
			error(ERROR_USAGE|ERROR_OUTPUT, STDOUT_FILENO, "%s", opt_info.arg);
			return 0;
	}
	argv += opt_info.index;
	if(error_info.errors || (*argv==0 &&!all))
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NULL));
		UNREACHABLE();
	}
	if(!troot)
		return 1;
	r = 0;
	if(troot==sh.var_tree)
		nflag |= NV_VARNAME;
	else
		nflag = NV_NOSCOPE;
	if(all)
	{
		if(dtfirst(troot))
		{
			if(troot==sh.alias_tree && sh.subshell && !sh.subshare)
				sh_subfork();	/* avoid affecting the parent shell's alias table */
			dtclear(troot);
		}
		return r;
	}
	while(name = *argv++)
	{
		sh_pushcontext(&buff,1);
		jmpval = sigsetjmp(buff.buff,0);
		np = 0;
		if(jmpval==0)
		{
#if SHOPT_NAMESPACE
			if(sh.namespace && troot==sh.fun_tree && !sh.prefix && *name!='.')
			{
				char *nsname;
				Namval_t *np2;
				/* prefix the namespace name */
				sfputr(sh.strbuf,nv_name(sh.namespace),'.');
				sfputr(sh.strbuf,name,'\0');
				nsname = sfstruse(sh.strbuf);
				np = nv_search(nsname,troot,NV_NOSCOPE);
				if(troot!=sh.fun_base && !np && (np2=nv_search(nsname,troot,0)) && is_afunction(np2))
				{	/* create dummy virtual subshell node without NV_FUNCTION attribute */
					nv_open(nsname,troot,NV_NOSCOPE);
					return r;
				}
			}
			if(!np)
#endif /* SHOPT_NAMESPACE */
			np=nv_open(name,troot,NV_NOADD|nflag);
		}
		sh_popcontext(&buff);
		if(jmpval)
		{
			r = 1;
			continue;
		}
		if(np)
		{
			if(is_abuiltin(np) || nv_isattr(np,NV_RDONLY))
			{
				if(nv_isattr(np,NV_RDONLY))
					errormsg(SH_DICT,ERROR_warn(0),e_readonly, nv_name(np));
				r = 1;
				continue;
			}
			isfun = is_afunction(np);
			if(troot==sh.var_tree)
			{
#if SHOPT_FIXEDARRAY
				Namarr_t *ap;
				if((ap=nv_arrayptr(np)) && !ap->fixed  && name[strlen(name)-1]==']' && !nv_getsub(np))
#else
				if(nv_isarray(np) && name[strlen(name)-1]==']' && !nv_getsub(np))
#endif /* SHOPT_FIXEDARRAY */
				{
					r=1;
					continue;
				}
				if(sh.subshell)
				{
					/*
					 * Create local scope for virtual subshell. Variables with discipline functions
					 * (LC_*, LINENO, etc.) need to be cloned, as moving them will remove the discipline.
					 */
					sh_assignok(np, !nv_isattr(np,NV_NODISC|NV_ARRAY) && !nv_isvtree(np));
				}
			}
			if(!nv_isnull(np) || nv_size(np) || nv_isattr(np,~(NV_MINIMAL|NV_NOFREE)))
			{
				Namarr_t *ap;
				if(sh.subshell && !sh.subshare && nv_isattr(np,NV_ARRAY|NV_NOFREE)==NV_ARRAY && (ap=nv_arrayptr(np)) && array_assoc(ap))
				{
					Namval_t *onp = nv_associative(np,0,NV_ACURRENT);
					while(nv_associative(np,0,NV_ANEXT))
					{
						Namval_t *mp = nv_associative(np,0,NV_ACURRENT);
						if(mp && mp->nvalue && mp->nvalue!=Empty)
							nv_offattr(mp,NV_NOFREE);
					}
					nv_associative(np,(char*)onp,NV_ASETSUB);
				}
				nv_unset(np,0);
			}
			if(troot==sh.var_tree && sh.st.real_fun && (dp=sh.var_tree->walk) && dp==sh.st.real_fun->sdict)
				nv_delete(np,dp,NV_NOFREE);
			else if(isfun)
			{
				if(troot!=sh.fun_base)
					np->nvflag = 0;	/* invalidate */
				else if(!(np->nvalue && ((struct Ufunction*)np->nvalue)->running))
					nv_delete(np,troot,0);
			}
			/* The alias has been unset by call to nv_unset, remove it from the tree */
			else if(troot==sh.alias_tree)
			{
				if(sh.subshell && !sh.subshare)
					sh_subfork();	/* avoid affecting the parent shell's alias table */
				nv_unset(np,nv_isattr(np,NV_NOFREE));
				nv_delete(np,troot,0);
			}
		}
		else if(troot==sh.alias_tree)
			r = 1;
		else if(troot==sh.fun_tree && troot!=sh.fun_base && !np && (np=nv_search(name,troot,0)) && is_afunction(np))
			nv_open(name,troot,NV_NOSCOPE); /* create dummy virtual subshell node without NV_FUNCTION attribute */
	}
	return r;
}

/*
 * print out the name and value of a name-value pair <np>
 */
static int print_namval(Sfio_t *file,Namval_t *np,int flag, struct tdata *tp)
{
	char	*cp;
	int	indent=tp->indent, outname=0, isfun;
	char	tempexport=0;
	sh_sigcheck();
	if(flag)
		flag = '\n';
	if(tp->noref && nv_isref(np))
		return 0;
	if(sh.shcomp)
		return 1;  /* print nothing while shcomp is compiling */
	if(nv_isattr(np,NV_NOPRINT|NV_INTEGER)==NV_NOPRINT)
	{
		if(is_abuiltin(np))
		{
			if(tp->prefix)
				sfputr(file,tp->prefix,' ');
			sfputr(file,nv_name(np),'\n');
		}
		return 0;
	}
	if(nv_istable(np))
	{
		print_value(file,np,tp);
		return 0;
	}
	isfun = is_afunction(np);
	if(tp->prefix)
	{
		outname = (*tp->prefix=='t' && (!nv_isnull(np) || nv_isattr(np,NV_FLOAT|NV_RDONLY|NV_BINARY|NV_RJUST|NV_NOPRINT)));
		if(tp->scanroot==sh.track_tree && *tp->prefix=='a')
			tp->prefix = "alias -t";
		if(indent && (isfun || outname || *tp->prefix!='t'))
		{
			sfnputc(file,'\t',indent);
			indent = 0;
		}
		if(!isfun)
		{
			if(*tp->prefix=='t')
				nv_attribute(np,tp->outfile,tp->prefix,tp->aflag);
			else
				sfputr(file,tp->prefix,' ');
		}
	}
	if(isfun)
	{
		char *fname=0;
		struct Ufunction *rp = np->nvalue;
		if(nv_isattr(np,NV_NOFREE))
			return 0;
		if(!flag && !rp)
			sfputr(file,"typeset -fu",' ');
		else if(!flag && !nv_isattr(np,NV_FPOSIX))
			sfputr(file,"function",' ');
		cp = nv_name(np);
		if(tp->wctname)
			cp += strlen(tp->wctname)+1;
		sfputr(file,cp,-1);
		if(nv_isattr(np,NV_FPOSIX))
			sfwrite(file,"()",2);
		else if(rp)
		{
			int i;
			/* output function reference list (for .sh.math.* functions) */
			for(i = 0; i < rp->argc; i++)
				sfprintf(file," %s",rp->argv[i]);
		}
		if(rp && rp->ptree)
			fname = rp->fname;
		else
			flag = '\n';
		if(flag)
		{
			if(tp->pflag && rp && rp->ptree)
				sfprintf(file," #line %d %s\n", rp->lineno, fname ? sh_fmtq(fname) : Empty);
			else
				sfputc(file, '\n');
		}
		else
		{
			sfputc(file, '\n');
			sh_deparse(file, (Shnode_t*)(rp->ptree), 2 | nv_isattr(np,NV_FPOSIX), 0);
		}
		return nv_size(np) + 1;
	}
	if(nv_arrayptr(np))
	{
		if(indent)
			sfnputc(file,'\t',indent);
		print_value(file,np,tp);
		return 0;
	}
	if(nv_isvtree(np) && !nv_isattr(np,NV_EXPORT))
	{
		/*
		 * Compound variable. Repurpose NV_EXPORT to tell walk_tree() in nvdisc.c not
		 * to indent the nv_getval() output. Also turn it back off, or bugs will happen.
		 */
		nv_onattr(np,NV_EXPORT);
		tempexport++;
	}
	cp = nv_getval(np);
	if(tempexport)
		nv_offattr(np,NV_EXPORT);
	if(cp)
	{
		if(indent)
			sfnputc(file,'\t',indent);
		sfputr(file,nv_name(np),-1);
		if(!flag)
			flag = '=';
		sfputc(file,flag);
		if(flag != '\n')
		{
			if(nv_isref(np) && nv_refsub(np))
			{
				sfputr(file,sh_fmtq(cp),-1);
				sfprintf(file,"[%s]\n", sh_fmtq(nv_refsub(np)));
			}
			else
				sfputr(file,nv_isvtree(np)?cp:sh_fmtq(cp),'\n');
		}
		return 1;
	}
	else if(outname || (tp->scanmask && tp->scanroot==sh.var_tree))
		sfputr(file,nv_name(np),'\n');
	return 0;
}

/*
 * print attributes at all nodes
 */
static void	print_all(Sfio_t *file,Dt_t *root, struct tdata *tp)
{
	tp->outfile = file;
	nv_scan(root, print_attribute, tp, 0, 0);
}

/*
 * print the attributes of name value pair give by <np>
 */
static void	print_attribute(Namval_t *np,void *data)
{
	struct tdata *dp = (struct tdata*)data;
	nv_attribute(np,dp->outfile,dp->prefix,dp->aflag);
}

/*
 * print the nodes in tree <root> which have attributes <flag> set
 * if <option> is non-zero, no subscript or value is printed
 */
static void print_scan(Sfio_t *file, int flag, Dt_t *root, int option,struct tdata *tp)
{
	char **argv;
	Namval_t *np;
	int namec;
	Namval_t *onp = 0;
	char	*name=0;
	size_t	len;
	sh.last_table=0;
	flag &= ~NV_ASSIGN;
	tp->scanmask = flag&~NV_NOSCOPE;
	tp->scanroot = root;
	tp->outfile = file;
	if(!tp->prefix && tp->tp)
		tp->prefix = nv_name(tp->tp);
	if(flag&NV_INTEGER)
		tp->scanmask |= (NV_DOUBLE|NV_EXPNOTE);
	if(flag==NV_LTOU || flag==NV_UTOL)
		tp->scanmask |= NV_UTOL|NV_LTOU;
	namec = nv_scan(root, nullscan, tp, tp->scanmask, flag&~NV_IARRAY);
	argv = tp->argnam  = stkalloc(sh.stk,(namec+1)*sizeof(char*));
	namec = nv_scan(root, pushname, tp, tp->scanmask, flag&~NV_IARRAY);
	if(ast.locale.transform)
		strsort(argv,namec,ast.locale.collate);
	if(namec==0 && sh.namespace && nv_dict(sh.namespace)==root)
	{
		sfnputc(file,'\t',tp->indent);
		sfwrite(file,":\n",2);
	}
	else while(namec--)
	{
		if((np=nv_search(*argv++,root,0)) && np!=onp && (!nv_isnull(np) || np->nvfun || nv_isattr(np,~NV_NOFREE)))
		{
			onp = np;
			if(name)
			{
				char *newname = nv_name(np);
				if(strncmp(name,newname,len)==0 && newname[len]== '.')
					continue;
				name = 0;
			}
			if(flag&NV_ARRAY)
			{
				if(nv_aindex(np)>=0)
				{
					if(!(flag&NV_IARRAY))
						continue;
				}
				else if((flag&NV_IARRAY))
					continue;
			}
			tp->scanmask = flag&~NV_NOSCOPE;
			tp->scanroot = root;
			print_namval(file,np,option,tp);
			if(!is_abuiltin(np) && nv_isvtree(np))
			{
				name = nv_name(np);
				len = strlen(name);
			}
		}
	}
}

/*
 * add the name of the node to the argument list argnam
 */
static void pushname(Namval_t *np,void *data)
{
	struct tdata *tp = (struct tdata*)data;
	*tp->argnam++ = nv_name(np);
}

/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2021 Contributors to ksh 93u+m           *
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
 * bg [job...]
 * disown [job...]
 * exec [-c] [-a name] [command [arg ...]]
 * eval [arg...]
 * fg [job...]
 * jobs [-lnp] [job...]
 * let expr...
 * redirect [redirection...]
 * source file [arg...]
 * . file [arg...]
 * :, true, false
 * wait [job...]
 * shift [n]
 * times
 *
 *   David Korn
 *   AT&T Labs
 *
 */

#include	"defs.h"
#include	"variables.h"
#include	"shnodes.h"
#include	"path.h"
#include	"io.h"
#include	"name.h"
#include	"history.h"
#include	"builtins.h"
#include	"jobs.h"

#include	<math.h>
#include	"FEATURE/locale"
#include	"FEATURE/time"
#if _lib_getrusage
#include	<sys/resource.h>
#else
#include	<times.h>
#endif

#define DOTMAX	MAXDEPTH	/* maximum level of . nesting */

/*
 * Handler function for nv_scan() that unsets a variable's export attribute
 */
static void     noexport(register Namval_t* np, void *data)
{
	NOT_USED(data);
	nv_offattr(np,NV_EXPORT);
}

/*
 * 'exec' special builtin and 'redirect' builtin
 */
#if 0
/* for the dictionary generator */
int    b_redirect(int argc,char *argv[],Shbltin_t *context){}
#endif
int    b_exec(int argc,char *argv[], Shbltin_t *context)
{
	register int n;
	struct checkpt *pp;
	const char *pname;
	int	clear = 0;
	char	*arg0 = 0;
	NOT_USED(argc);
	sh.st.ioset = 0;
	while (n = optget(argv, *argv[0]=='r' ? sh_optredirect : sh_optexec)) switch (n)
	{
	    case 'a':
		arg0 = opt_info.arg;
		break;
	    case 'c':
		clear=1;
		break;
	    case ':':
		errormsg(SH_DICT,2, "%s", opt_info.arg);
		break;
	    case '?':
		errormsg(SH_DICT,ERROR_usage(0), "%s", opt_info.arg);
		return(2);
	}
	if(error_info.errors)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage((char*)0));
		UNREACHABLE();
	}
	if(*argv[0]=='r' && argv[opt_info.index])  /* 'redirect' supports no args */
	{
		errormsg(SH_DICT,ERROR_exit(2),"%s: %s",e_badsyntax,argv[opt_info.index]);
		UNREACHABLE();
	}
	argv += opt_info.index;
	if(!*argv)
		return(0);

	/* from here on, it's 'exec' with args, so we're replacing the shell */
	if(sh_isoption(SH_RESTRICTED))
	{
		errormsg(SH_DICT,ERROR_exit(1),e_restricted,argv[0]);
		UNREACHABLE();
	}
	else
	{
		register struct argnod *arg=sh.envlist;
		register Namval_t* np;
		register char *cp;
		if(sh.subshell && !sh.subshare)
			sh_subfork();
		if(clear)
			nv_scan(sh.var_tree,noexport,0,NV_EXPORT,NV_EXPORT);
		while(arg)
		{
			if((cp=strchr(arg->argval,'=')) &&
				(*cp=0,np=nv_search(arg->argval,sh.var_tree,0)))
			{
				nv_onattr(np,NV_EXPORT);
				sh_envput(sh.env,np);
			}
			if(cp)
				*cp = '=';
			arg=arg->argnxt.ap;
		}
		pname = argv[0];
		if(arg0)
			argv[0] = arg0;
#ifdef JOBS
		if(job_close() < 0)
			return(1);
#endif /* JOBS */
		/* if the main shell is about to be replaced, decrease SHLVL to cancel out a subsequent increase */
		if(!shgd->realsubshell)
			(*SHLVL->nvalue.ip)--;
		/* force bad exec to terminate shell */
		pp = (struct checkpt*)sh.jmplist;
		pp->mode = SH_JMPEXIT;
		sh_sigreset(2);
		sh_freeup();
		path_exec(pname,argv,NIL(struct argnod*));
	}
	return(1);
}

int    b_let(int argc,char *argv[],Shbltin_t *context)
{
	register int r;
	register char *arg;
	Shell_t *shp = context->shp;
	NOT_USED(argc);
	while (r = optget(argv,sh_optlet)) switch (r)
	{
	    case ':':
		errormsg(SH_DICT,2, "%s", opt_info.arg);
		break;
	    case '?':
		errormsg(SH_DICT,ERROR_usage(2), "%s", opt_info.arg);
		UNREACHABLE();
	}
	argv += opt_info.index;
	if(error_info.errors || !*argv)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage((char*)0));
		UNREACHABLE();
	}
	while(arg= *argv++)
		r = !sh_arith(arg);
	return(r);
}

int    b_eval(int argc,char *argv[], Shbltin_t *context)
{
	register int r;
	register Shell_t *shp = context->shp;
	NOT_USED(argc);
	while (r = optget(argv,sh_opteval)) switch (r)
	{
	    case ':':
		errormsg(SH_DICT,2, "%s", opt_info.arg);
		break;
	    case '?':
		errormsg(SH_DICT,ERROR_usage(0), "%s",opt_info.arg);
		return(2);
	}
	if(error_info.errors)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage((char*)0));
		UNREACHABLE();
	}
	argv += opt_info.index;
	if(*argv && **argv)
	{
		sh_offstate(SH_MONITOR);
		sh_eval(sh_sfeval(argv),0);
	}
	return(shp->exitval);
}

#if 0
    /* for the dictionary generator */
    int	b_source(register int n,char *argv[],Shbltin_t *context){}
#endif
int    b_dot_cmd(register int n,char *argv[],Shbltin_t *context)
{
	register char *script;
	register Namval_t *np;
	register int jmpval;
	register Shell_t *shp = context->shp;
	struct sh_scoped savst, *prevscope = shp->st.self;
	char *filename=0, *buffer=0, *tofree;
	int	fd, infunction = 0;
	struct dolnod   *saveargfor;
	volatile struct dolnod   *argsave=0;
	struct checkpt buff;
	Sfio_t *iop=0;
	short level;
	while (n = optget(argv,sh_optdot)) switch (n)
	{
	    case ':':
		errormsg(SH_DICT,2, "%s", opt_info.arg);
		break;
	    case '?':
		errormsg(SH_DICT,ERROR_usage(0), "%s",opt_info.arg);
		return(2);
	}
	argv += opt_info.index;
	script = *argv;
	if(error_info.errors || !script)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage((char*)0));
		UNREACHABLE();
	}
	if(shp->dot_depth+1 > DOTMAX)
	{
		errormsg(SH_DICT,ERROR_exit(1),e_toodeep,script);
		UNREACHABLE();
	}
	if(!(np=shp->posix_fun))
	{
		/* check for KornShell style function first */
		np = nv_search(script,shp->fun_tree,0);
		if(np && is_afunction(np) && !nv_isattr(np,NV_FPOSIX) && !(sh_isoption(SH_POSIX) && shp->bltindata.bnode==SYSDOT))
		{
			if(!np->nvalue.ip)
			{
				path_search(script,NIL(Pathcomp_t**),0);
				if(np->nvalue.ip)
				{
					if(nv_isattr(np,NV_FPOSIX))
						np = 0;
				}
				else
				{
					errormsg(SH_DICT,ERROR_exit(1),e_found,script);
					UNREACHABLE();
				}
			}
		}
		else
			np = 0;
		if(!np)
		{
			if((fd=path_open(script,path_get(script))) < 0)
			{
				errormsg(SH_DICT,ERROR_system(1),e_open,script);
				UNREACHABLE();
			}
			filename = path_fullname(stkptr(sh.stk,PATH_OFFSET));
		}
	}
	if(np)
	{
		infunction = sh_isstate(SH_INFUNCTION);
		sh_onstate(SH_INFUNCTION);
	}
	*prevscope = shp->st;
	shp->st.lineno = np?((struct functnod*)nv_funtree(np))->functline:1;
	shp->st.var_local = shp->st.save_tree = shp->var_tree;
	if(filename)
	{
		shp->st.filename = filename;
		shp->st.lineno = 1;
	}
	level  = shp->fn_depth+shp->dot_depth+1;
	nv_putval(SH_LEVELNOD,(char*)&level,NV_INT16);
	shp->st.prevst = prevscope;
	shp->st.self = &savst;
	shp->topscope = (Shscope_t*)shp->st.self;
	prevscope->save_tree = shp->var_tree;
	tofree = shp->st.filename;
	if(np)
		shp->st.filename = np->nvalue.rp->fname;
	nv_putval(SH_PATHNAMENOD, shp->st.filename ,NV_NOFREE);
	shp->posix_fun = 0;
	if(np || argv[1])
		argsave = sh_argnew(argv,&saveargfor);
	sh_pushcontext(shp,&buff,SH_JMPDOT);
	jmpval = sigsetjmp(buff.buff,0);
	if(jmpval == 0)
	{
		shp->dot_depth++;
		if(np)
			sh_exec((Shnode_t*)(nv_funtree(np)),sh_isstate(SH_ERREXIT));
		else
		{
			buffer = sh_malloc(IOBSIZE+1);
			iop = sfnew(NIL(Sfio_t*),buffer,IOBSIZE,fd,SF_READ);
			sh_offstate(SH_NOFORK);
			sh_eval(iop,sh_isstate(SH_PROFILE)?SH_FUNEVAL:0);
		}
	}
	sh_popcontext(shp,&buff);
	if(buffer)
		free(buffer);
	if(!np)
		free(tofree);
	shp->dot_depth--;
	if((np || argv[1]) && jmpval!=SH_JMPSCRIPT)
		sh_argreset((struct dolnod*)argsave,saveargfor);
	else
	{
		prevscope->dolc = shp->st.dolc;
		prevscope->dolv = shp->st.dolv;
	}
	if (shp->st.self != &savst)
		*shp->st.self = shp->st;
	if(!infunction)
		sh_offstate(SH_INFUNCTION);
	/* only restore the top Shscope_t portion for POSIX functions */
	memcpy((void*)&shp->st, (void*)prevscope, sizeof(Shscope_t));
	shp->topscope = (Shscope_t*)prevscope;
	nv_putval(SH_PATHNAMENOD, shp->st.filename ,NV_NOFREE);
	if(jmpval && jmpval!=SH_JMPFUN)
		siglongjmp(*shp->jmplist,jmpval);
	return(shp->exitval);
}

/*
 * null, true command
 */
int    b_true(int argc,register char *argv[],Shbltin_t *context)
{
	NOT_USED(argc);
	NOT_USED(argv[0]);
	NOT_USED(context);
	return(0);
}

/*
 * false command
 */
int    b_false(int argc,register char *argv[], Shbltin_t *context)
{
	NOT_USED(argc);
	NOT_USED(argv[0]);
	NOT_USED(context);
	return(1);
}

int    b_shift(register int n, register char *argv[], Shbltin_t *context)
{
	register char *arg;
	register Shell_t *shp = context->shp;
	while((n = optget(argv,sh_optshift))) switch(n)
	{
		case ':':
			errormsg(SH_DICT,2, "%s", opt_info.arg);
			break;
		case '?':
			errormsg(SH_DICT,ERROR_usage(0), "%s",opt_info.arg);
			return(2);
	}
	if(error_info.errors)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage((char*)0));
		UNREACHABLE();
	}
	argv += opt_info.index;
	n = ((arg= *argv)?(int)sh_arith(arg):1);
	if(n<0 || shp->st.dolc<n)
	{
		errormsg(SH_DICT,ERROR_exit(1),e_number,arg);
		UNREACHABLE();
	}
	else
	{
		shp->st.dolv += n;
		shp->st.dolc -= n;
	}
	return(0);
}

int    b_wait(int n,register char *argv[],Shbltin_t *context)
{
	register Shell_t *shp = context->shp;
	while((n = optget(argv,sh_optwait))) switch(n)
	{
		case ':':
			errormsg(SH_DICT,2, "%s", opt_info.arg);
			break;
		case '?':
			errormsg(SH_DICT,ERROR_usage(2), "%s",opt_info.arg);
			UNREACHABLE();
	}
	if(error_info.errors)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage((char*)0));
		UNREACHABLE();
	}
	argv += opt_info.index;
	job_bwait(argv);
	return(shp->exitval);
}

#ifdef JOBS
#   if 0
    /* for the dictionary generator */
	int    b_fg(int n,char *argv[],Shbltin_t *context){}
	int    b_disown(int n,char *argv[],Shbltin_t *context){}
#   endif
int    b_bg(register int n,register char *argv[],Shbltin_t *context)
{
	register int flag = **argv;
	register Shell_t *shp = context->shp;
	register const char *optstr = sh_optbg; 
	if(*argv[0]=='f')
		optstr = sh_optfg;
	else if(*argv[0]=='d')
		optstr = sh_optdisown;
	while((n = optget(argv,optstr))) switch(n)
	{
	    case ':':
		errormsg(SH_DICT,2, "%s", opt_info.arg);
		break;
	    case '?':
		errormsg(SH_DICT,ERROR_usage(2), "%s",opt_info.arg);
		UNREACHABLE();
	}
	if(error_info.errors)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage((char*)0));
		UNREACHABLE();
	}
	argv += opt_info.index;
	if(!sh_isstate(SH_MONITOR))
	{
		errormsg(SH_DICT,ERROR_exit(1),e_no_jctl);
		UNREACHABLE();
	}
	if(flag=='d' && *argv==0)
		argv = (char**)0;
	if(job_walk(sfstdout,job_switch,flag,argv))
	{
		errormsg(SH_DICT,ERROR_exit(1),e_no_job);
		UNREACHABLE();
	}
	return(shp->exitval);
}

int    b_jobs(register int n,char *argv[],Shbltin_t *context)
{
	register int flag = 0;
	register Shell_t *shp = context->shp;
	while((n = optget(argv,sh_optjobs))) switch(n)
	{
	    case 'l':
		flag = JOB_LFLAG;
		break;
	    case 'n':
		flag = JOB_NFLAG;
		break;
	    case 'p':
		flag = JOB_PFLAG;
		break;
	    case ':':
		errormsg(SH_DICT,2, "%s", opt_info.arg);
		break;
	    case '?':
		errormsg(SH_DICT,ERROR_usage(2), "%s",opt_info.arg);
		UNREACHABLE();
	}
	argv += opt_info.index;
	if(error_info.errors)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage((char*)0));
		UNREACHABLE();
	}
	if(*argv==0)
		argv = (char**)0;
	if(job_walk(sfstdout,job_list,flag,argv))
	{
		errormsg(SH_DICT,ERROR_exit(1),e_no_job);
		UNREACHABLE();
	}
	job_wait((pid_t)0);
	return(shp->exitval);
}
#endif

/*
 * times command
 */
static void	print_times(struct timeval utime, struct timeval stime)
{
	int ut_min = utime.tv_sec / 60;
	int ut_sec = utime.tv_sec % 60;
	int ut_ms = utime.tv_usec / 1000;
	int st_min = stime.tv_sec / 60;
	int st_sec = stime.tv_sec % 60;
	int st_ms = stime.tv_usec / 1000;
	char radix = GETDECIMAL(0);
	sfprintf(sfstdout, "%dm%02d%c%03ds %dm%02d%c%03ds\n", ut_min, ut_sec, radix, ut_ms, st_min, st_sec, radix, st_ms);
}
#if _lib_getrusage
/* getrusage tends to have higher precision */
static void	print_cpu_times(void)
{
	struct rusage usage;
	/* Print the time (user & system) consumed by the shell. */
	getrusage(RUSAGE_SELF, &usage);
	print_times(usage.ru_utime, usage.ru_stime);
	/* Print the time (user & system) consumed by the child processes of the shell. */
	getrusage(RUSAGE_CHILDREN, &usage);
	print_times(usage.ru_utime, usage.ru_stime);
}
#else  /* _lib_getrusage */
static void	print_cpu_times(void)
{
	struct timeval utime, stime;
	double dtime;
	int clk_tck = shgd->lim.clk_tck;
	struct tms cpu_times;
	times(&cpu_times);
	/* Print the time (user & system) consumed by the shell. */
	dtime = (double)cpu_times.tms_utime / clk_tck;
	utime.tv_sec = dtime / 60;
	utime.tv_usec = 1000000 * (dtime - utime.tv_sec);
	dtime = (double)cpu_times.tms_stime / clk_tck;
	stime.tv_sec = dtime / 60;
	stime.tv_usec = 1000000 * (dtime - utime.tv_sec);
	print_times(utime, stime);
	/* Print the time (user & system) consumed by the child processes of the shell. */
	dtime = (double)cpu_times.tms_cutime / clk_tck;
	utime.tv_sec = dtime / 60;
	utime.tv_usec = 1000000 * (dtime - utime.tv_sec);
	dtime = (double)cpu_times.tms_cstime / clk_tck;
	stime.tv_sec = dtime / 60;
	stime.tv_usec = 1000000 * (dtime - utime.tv_sec);
	print_times(utime, stime);
}
#endif  /* _lib_getrusage */
int	b_times(int argc, char *argv[], Shbltin_t *context)
{
	NOT_USED(context);
	/* No options or operands are supported, except --man, etc. */
	if (argc = optget(argv, sh_opttimes)) switch (argc)
	{
	    case ':':
		errormsg(SH_DICT, 2, "%s", opt_info.arg);
		errormsg(SH_DICT, ERROR_usage(2), "%s", optusage((char*)0));
		UNREACHABLE();
	    default:
		errormsg(SH_DICT, ERROR_usage(0), "%s", opt_info.arg);
		return(2);
	}
	if (argv[opt_info.index])
	{
		errormsg(SH_DICT, ERROR_exit(2), e_toomanyops);
		UNREACHABLE();
	}
	/* Get & print the times */
	print_cpu_times();
	return(0);
}

#ifdef _cmd_universe
/*
 * There are several universe styles that are masked by the getuniv(),
 * setuniv() calls.
 */
int	b_universe(int argc, char *argv[],Shbltin_t *context)
{
	register char *arg;
	register int n;
	NOT_USED(context);
	while((n = optget(argv,sh_optuniverse))) switch(n)
	{
	    case ':':
		errormsg(SH_DICT,2, "%s", opt_info.arg);
		break;
	    case '?':
		errormsg(SH_DICT,ERROR_usage(2), "%s",opt_info.arg);
		UNREACHABLE();
	}
	argv += opt_info.index;
	argc -= opt_info.index;
	if(error_info.errors || argc>1)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage((char*)0));
		UNREACHABLE();
	}
	if(arg = argv[0])
	{
		if(!astconf("UNIVERSE",0,arg))
		{
			errormsg(SH_DICT,ERROR_exit(1), e_badname,arg);
			UNREACHABLE();
		}
	}
	else
	{
		if(!(arg=astconf("UNIVERSE",0,0)))
		{
			errormsg(SH_DICT,ERROR_exit(1),e_nouniverse);
			UNREACHABLE();
		}
		else
			sfputr(sfstdout,arg,'\n');
	}
	return(0);
}
#endif /* cmd_universe */

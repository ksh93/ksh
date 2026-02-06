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

#include	"shopt.h"
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

/*
 * Handler function for nv_scan() that unsets a variable's export attribute
 */
static void     noexport(Namval_t* np, void *data)
{
	NOT_USED(data);
	if(sh.subshell && !sh.subshare)
		sh_assignok(np,0);
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
	int	n;
	const char *pname;
	int	clear = 0;
	char	*arg0 = 0;
	NOT_USED(context);
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
		/* self-doc: write to standard output */
		error(ERROR_USAGE|ERROR_OUTPUT, STDOUT_FILENO, "%s", opt_info.arg);
		return 0;
	}
	if(error_info.errors)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NULL));
		UNREACHABLE();
	}
	if(*argv[0]=='r' && argv[opt_info.index])  /* 'redirect' supports no args */
	{
		errormsg(SH_DICT,ERROR_exit(2),"%s: %s",e_badsyntax,argv[opt_info.index]);
		UNREACHABLE();
	}
	argv += opt_info.index;
	if(!*argv)
		return 0;

	/* from here on, it's 'exec' with args, so we're replacing the shell */
	if(sh_isoption(SH_RESTRICTED))
		errormsg(SH_DICT,ERROR_exit(1),e_restricted,argv[0]);
	else
	{
		struct argnod *arg=sh.envlist;
		Namval_t* np;
		char *cp;
#if !_execve_ignores_argv0
		if(arg0 && sh.subshell && !sh.subshare)
			sh_subfork();
#endif /* !_execve_ignores_argv0 */
		if(clear)
			nv_scan(sh.var_tree,noexport,0,NV_EXPORT,NV_EXPORT);
		while(arg)
		{
			if((cp=strchr(arg->argval,'=')) &&
				(*cp=0,np=nv_search(arg->argval,sh.var_tree,0)))
			{
				nv_onattr(np,NV_EXPORT);
				env_change();
			}
			if(cp)
				*cp = '=';
			arg=arg->argnxt.ap;
		}
		pname = argv[0];
		if(arg0)
#if _execve_ignores_argv0
			error(ERROR_warn(0),"-a %s: %s",arg0,sh_translate(e_nosupport));
#else
			argv[0] = arg0;
#endif /* _execve_ignores_argv0 */
		if(job_close() < 0)
			return 1;
		/* if the main shell is about to be replaced, decrease SHLVL to cancel out a subsequent increase */
		if(!sh.realsubshell)
			sh.shlvl--;
		sh_onstate(SH_EXEC);
		if(sh.subshell && !sh.subshare)
		{
			struct dolnod *dp = stkalloc(sh.stk, sizeof(struct dolnod) + ARG_SPARE*sizeof(char*) + argc*sizeof(char*));
			struct comnod *t = stkalloc(sh.stk,sizeof(struct comnod));
			memset(t, 0, sizeof(struct comnod));
			dp->dolnum = argc;
			dp->dolbot = ARG_SPARE;
			memcpy(dp->dolval+ARG_SPARE, argv, (argc+1)*sizeof(char*));
			t->comarg.dp = dp;
			sh_exec((Shnode_t*)t,sh_isstate(SH_ERREXIT));
			sh_offstate(SH_EXEC);
			siglongjmp(*sh.jmplist,SH_JMPEXIT);
		}
		sh_sigreset(2);
		sh_freeup();
		path_exec(pname,argv,NULL);
	}
	UNREACHABLE();
}

int    b_let(int argc,char *argv[],Shbltin_t *context)
{
	int r;
	char *arg;
	NOT_USED(argc);
	NOT_USED(context);
	while (r = optget(argv,sh_optlet)) switch (r)
	{
	    case ':':
		errormsg(SH_DICT,2, "%s", opt_info.arg);
		break;
	    case '?':
		/* self-doc: write to standard output */
		error(ERROR_USAGE|ERROR_OUTPUT, STDOUT_FILENO, "%s", opt_info.arg);
		return 0;
	}
	argv += opt_info.index;
	if(error_info.errors || !*argv)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NULL));
		UNREACHABLE();
	}
	while(arg= *argv++)
		r = !sh_arith(arg);
	return r;
}

int    b_eval(int argc,char *argv[], Shbltin_t *context)
{
	int r;
	NOT_USED(argc);
	NOT_USED(context);
	while (r = optget(argv,sh_opteval)) switch (r)
	{
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
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NULL));
		UNREACHABLE();
	}
	argv += opt_info.index;
	if(*argv && **argv)
		sh_eval(sh_sfeval(argv),0);
	return sh.exitval;
}

#if 0
    /* for the dictionary generator */
    int	b_source(int n,char *argv[],Shbltin_t *context){}
#endif
int    b_dot_cmd(int n,char *argv[],Shbltin_t *context)
{
	char *script;
	Namval_t *np;
	int jmpval;
	struct sh_scoped savst, *prevscope = sh.st.self;
	char *filename=0, *buffer=0, *tofree;
	int	fd;
	struct dolnod   *saveargfor;
	volatile struct dolnod   *argsave=0;
	struct checkpt buff;
	Sfio_t *iop=0;
	while (n = optget(argv,sh_optdot)) switch (n)
	{
	    case ':':
		errormsg(SH_DICT,2, "%s", opt_info.arg);
		break;
	    case '?':
		/* self-doc: write to standard output */
		error(ERROR_USAGE|ERROR_OUTPUT, STDOUT_FILENO, "%s", opt_info.arg);
		return 0;
	}
	argv += opt_info.index;
	script = *argv;
	if(error_info.errors || !script)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NULL));
		UNREACHABLE();
	}
	if(sh.dot_depth >= MAXDEPTH)
	{
		errormsg(SH_DICT,ERROR_exit(1),e_toodeep,script);
		UNREACHABLE();
	}
	/* check for KornShell style function first */
	np = nv_search(script,sh.fun_tree,0);
	if(np && is_afunction(np) && !nv_isattr(np,NV_FPOSIX) && !(sh_isoption(SH_POSIX) && context->bnode==SYSDOT))
	{
		if(!np->nvalue)
		{
			path_search(script,NULL,0);
			if(np->nvalue)
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
		/* open the dot script */
		if((fd=path_open(script,path_get(script))) < 0)
		{
			errormsg(SH_DICT,ERROR_system(1),e_open,script);
			UNREACHABLE();
		}
		filename = path_fullname(stkptr(sh.stk,PATH_OFFSET));
	}
	*prevscope = sh.st;
	sh.st.lineno = np?((struct functnod*)nv_funtree(np))->functline:1;
	sh.st.save_tree = sh.var_tree;
	if(filename)
		sh.st.filename = filename;
	sh.st.prevst = prevscope;
	sh.st.self = &savst;
	sh.topscope = (Shscope_t*)sh.st.self;
	prevscope->save_tree = sh.var_tree;
	tofree = sh.st.filename;
	if(np)
		sh.st.filename = ((struct Ufunction*)np->nvalue)->fname;
	nv_putval(SH_PATHNAMENOD, sh.st.filename ,NV_NOFREE);
	if(np || argv[1])
		argsave = sh_argnew(argv,&saveargfor);
	sh_pushcontext(&buff,SH_JMPDOT);
	errorpush(&buff.err,0);
	error_info.id = argv[0];
	jmpval = sigsetjmp(buff.buff,0);
	if(jmpval == 0)
	{
		sh.dot_depth++;
		update_sh_level();
		if(np)
		{
			/* execute the function as though it were a dot script */
			sh_exec((Shnode_t*)(nv_funtree(np)),sh_isstate(SH_ERREXIT));
		}
		else
		{
			/* run the dot script */
			buffer = sh_malloc(IOBSIZE+1);
			iop = sfnew(NULL,buffer,IOBSIZE,fd,SFIO_READ);
			sh_offstate(SH_NOFORK);
			sh_eval(iop,sh_isstate(SH_PROFILE)?SH_FUNEVAL:0);
		}
	}
	sh_popcontext(&buff);
	if(buffer)
		free(buffer);
	if(!np)
		free(tofree);
	sh.dot_depth--;
	update_sh_level();
	if((np || argv[1]) && jmpval!=SH_JMPSCRIPT)
		sh_argreset((struct dolnod*)argsave,saveargfor);
	else
	{
		prevscope->dolc = sh.st.dolc;
		prevscope->dolv = sh.st.dolv;
	}
	if (sh.st.self != &savst)
		*sh.st.self = sh.st;
	/* only restore the top Shscope_t portion for functions */
	memcpy(&sh.st, prevscope, sizeof(Shscope_t));
	sh.topscope = (Shscope_t*)prevscope;
	nv_putval(SH_PATHNAMENOD, sh.st.filename ,NV_NOFREE);
	if(jmpval && jmpval!=SH_JMPFUN)
		siglongjmp(*sh.jmplist,jmpval);
	return sh.exitval;
}

/*
 * null, true command
 */
int    b_true(int argc,char *argv[],Shbltin_t *context)
{
	NOT_USED(argc);
	NOT_USED(argv[0]);
	NOT_USED(context);
	return 0;
}

/*
 * false command
 */
int    b_false(int argc,char *argv[], Shbltin_t *context)
{
	NOT_USED(argc);
	NOT_USED(argv[0]);
	NOT_USED(context);
	return 1;
}

int    b_shift(int n, char *argv[], Shbltin_t *context)
{
	char *arg;
	NOT_USED(context);
	while((n = optget(argv,sh_optshift))) switch(n)
	{
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
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NULL));
		UNREACHABLE();
	}
	argv += opt_info.index;
	n = ((arg= *argv)?(int)sh_arith(arg):1);
	if(n < 0 || sh.st.dolc < n)
	{
		errormsg(SH_DICT,ERROR_exit(1),e_number,arg);
		UNREACHABLE();
	}
	else
	{
		sh.st.dolv += n;
		sh.st.dolc -= n;
	}
	return 0;
}

int    b_wait(int n,char *argv[],Shbltin_t *context)
{
	NOT_USED(context);
	while((n = optget(argv,sh_optwait))) switch(n)
	{
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
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NULL));
		UNREACHABLE();
	}
	argv += opt_info.index;
	job_bwait(argv);
	return sh.exitval;
}

#if 0
    /* for the dictionary generator */
	int    b_fg(int n,char *argv[],Shbltin_t *context){}
	int    b_disown(int n,char *argv[],Shbltin_t *context){}
#endif
int    b_bg(int n,char *argv[],Shbltin_t *context)
{
	int flag = **argv;
	const char *optstr = sh_optbg;
	NOT_USED(context);
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
		/* self-doc: write to standard output */
		error(ERROR_USAGE|ERROR_OUTPUT, STDOUT_FILENO, "%s", opt_info.arg);
		return 0;
	}
	if(error_info.errors)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NULL));
		UNREACHABLE();
	}
	argv += opt_info.index;
	if(!sh_isstate(SH_MONITOR))
	{
		errormsg(SH_DICT,ERROR_exit(1),e_no_jctl);
		UNREACHABLE();
	}
	if(flag=='d' && *argv==0)
		argv = NULL;
	if(job_walk(sfstdout,job_switch,flag,argv))
	{
		errormsg(SH_DICT,ERROR_exit(1),e_no_job);
		UNREACHABLE();
	}
	return sh.exitval;
}

int    b_jobs(int n,char *argv[],Shbltin_t *context)
{
	int flag = 0;
	NOT_USED(context);
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
		/* self-doc: write to standard output */
		error(ERROR_USAGE|ERROR_OUTPUT, STDOUT_FILENO, "%s", opt_info.arg);
		return 0;
	}
	argv += opt_info.index;
	if(error_info.errors)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NULL));
		UNREACHABLE();
	}
	if(*argv==0)
		argv = NULL;
	if(job_walk(sfstdout,job_list,flag,argv))
	{
		errormsg(SH_DICT,ERROR_exit(1),e_no_job);
		UNREACHABLE();
	}
	job_wait(0);
	return sh.exitval;
}

/*
 * times command
 */
static void	print_times(struct timeval utime, struct timeval stime)
{
	Sfulong_t ut_min = utime.tv_sec / 60;
	Sfulong_t ut_sec = utime.tv_sec % 60;
	Sfulong_t ut_ms = utime.tv_usec / 1000;
	Sfulong_t st_min = stime.tv_sec / 60;
	Sfulong_t st_sec = stime.tv_sec % 60;
	Sfulong_t st_ms = stime.tv_usec / 1000;
	sfprintf(sfstdout, sh_isoption(SH_POSIX) ? "%jum%ju%c%03jus %jum%ju%c%03jus\n" : "%jum%02ju%c%03jus %jum%02ju%c%03jus\n",
		ut_min, ut_sec, sh.radixpoint, ut_ms, st_min, st_sec, sh.radixpoint, st_ms);
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
	Sfdouble_t dtime;
	int clk_tck = sh.lim.clk_tck;
	struct tms cpu_times;
	times(&cpu_times);
	/* Print the time (user & system) consumed by the shell. */
	dtime = (Sfdouble_t)cpu_times.tms_utime / clk_tck;
	utime.tv_sec = dtime / 60;
	utime.tv_usec = 1000000 * (dtime - utime.tv_sec);
	dtime = (Sfdouble_t)cpu_times.tms_stime / clk_tck;
	stime.tv_sec = dtime / 60;
	stime.tv_usec = 1000000 * (dtime - utime.tv_sec);
	print_times(utime, stime);
	/* Print the time (user & system) consumed by the child processes of the shell. */
	dtime = (Sfdouble_t)cpu_times.tms_cutime / clk_tck;
	utime.tv_sec = dtime / 60;
	utime.tv_usec = 1000000 * (dtime - utime.tv_sec);
	dtime = (Sfdouble_t)cpu_times.tms_cstime / clk_tck;
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
		errormsg(SH_DICT, ERROR_usage(2), "%s", optusage(NULL));
		UNREACHABLE();
	    default:
		errormsg(SH_DICT, ERROR_usage(0), "%s", opt_info.arg);
		return 2;
	}
	if (argv[opt_info.index])
	{
		errormsg(SH_DICT, ERROR_exit(2), e_toomanyops);
		UNREACHABLE();
	}
	/* Get & print the times */
	print_cpu_times();
	return 0;
}

#if _cmd_universe
/*
 * There are several universe styles that are masked by the getuniv(),
 * setuniv() calls.
 */
int	b_universe(int argc, char *argv[],Shbltin_t *context)
{
	char *arg;
	int n;
	NOT_USED(context);
	while((n = optget(argv,sh_optuniverse))) switch(n)
	{
	    case ':':
		errormsg(SH_DICT,2, "%s", opt_info.arg);
		break;
	    case '?':
		/* self-doc: write to standard output */
		error(ERROR_USAGE|ERROR_OUTPUT, STDOUT_FILENO, "%s", opt_info.arg);
		return 0;
	}
	argv += opt_info.index;
	argc -= opt_info.index;
	if(error_info.errors || argc>1)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NULL));
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
	return 0;
}
#endif /* cmd_universe */

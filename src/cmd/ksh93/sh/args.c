/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
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
*                      Phi <phi.debian@gmail.com>                      *
*                                                                      *
***********************************************************************/
/*
 * UNIX shell
 *
 * S. R. Bourne
 * Rewritten by David Korn
 * AT&T Labs
 *
 */

#include	"shopt.h"
#include	"defs.h"
#include	"path.h"
#include	"jobs.h"
#include	"builtins.h"
#include	"terminal.h"
#include	"edit.h"
#include	"FEATURE/poll"
#if SHOPT_KIA
#   include	"shlex.h"
#endif
#if SHOPT_KIA || SHOPT_DEVFD
#   include	"io.h"
#endif

#define SORT		1
#define PRINT		2

static	char		*null;

/* The following order is determined by sh_optset */
static  const char optksh[] =
	"Dircabefhkmnpstuvx"
#if SHOPT_BRACEPAT
	"B"
#endif
	"CGEl"
#if SHOPT_HISTEXPAND
	"H"
#endif
	;
static const int flagval[]  =
{
	SH_DICTIONARY, SH_INTERACTIVE, SH_RESTRICTED, SH_CFLAG,
	SH_ALLEXPORT, SH_NOTIFY, SH_ERREXIT, SH_NOGLOB, SH_TRACKALL,
	SH_KEYWORD, SH_MONITOR, SH_NOEXEC, SH_PRIVILEGED, SH_SFLAG, SH_TFLAG,
	SH_NOUNSET, SH_VERBOSE,  SH_XTRACE,
#if SHOPT_BRACEPAT
	SH_BRACEEXPAND,
#endif
	SH_NOCLOBBER, SH_GLOBSTARS, SH_RC, SH_LOGIN_SHELL,
#if SHOPT_HISTEXPAND
        SH_HISTEXPAND,
#endif
	0 
};

#define NUM_OPTS	(sizeof(flagval)/sizeof(*flagval))

typedef struct _arg_
{
	struct dolnod	*argfor; /* linked list of blocks to be cleaned up */
	struct dolnod	*dolh;
	char flagadr[NUM_OPTS+1];
#if SHOPT_KIA
	char	*kiafile;
#endif /* SHOPT_KIA */
} Arg_t;

static int 		arg_expand(struct argnod*,struct argnod**,int);
static void 		argset(Arg_t*, char *[]);
static void		applyopts(Shopt_t);


/* ======== option handling	======== */

void *sh_argopen(void)
{
	return sh_newof(0,Arg_t,1,0);
}

static int infof(Opt_t* op, Sfio_t* sp, const char* s, Optdisc_t* dp)
{
	if(*s!=':')
		sfputr(sp,sh_set,-1);
	return 1;
}

/*
 *  This routine turns options on and off
 *  The options "PDicr" are illegal from set command.
 *  The -o option is used to set option by name
 *  This routine returns the number of non-option arguments
 */
int sh_argopts(int argc,char *argv[])
{
	int		n,o;
	Arg_t		*ap = (Arg_t*)(sh.arg_context);
#if SHOPT_KIA
	Lex_t		*lp = (Lex_t*)(sh.lex_context);
#endif
	Shopt_t		newflags;
	int		defaultflag=0, setflag=0, action=0, trace=(int)sh_isoption(SH_XTRACE);
	Namval_t *np = NULL;
	const char *cp;
	int verbose,f;
	Optdisc_t disc;
	newflags=sh.options;
	memset(&disc, 0, sizeof(disc));
	disc.version = OPT_VERSION;
	disc.infof = infof;
	opt_info.disc = &disc;

	if(argc>0)
		setflag = 4;
	else
		argc = -argc;
	while((n = optget(argv,setflag?sh_optset:sh_optksh)))
	{
		o=0;
		f=*opt_info.option=='-' && (opt_info.num || opt_info.arg);
		switch(n)
		{
	 	    case 'A':
			np = nv_open(opt_info.arg,sh.var_tree,NV_ARRAY|NV_VARNAME);
			if(f)
				nv_unset(np);
			continue;
		    case 'o':	/* set options */
		    byname:
			if(!opt_info.arg||!*opt_info.arg||*opt_info.arg=='-')
			{
				action = PRINT;
				verbose = (f?PRINT_VERBOSE:PRINT_NO_HEADER)|
					  ((opt_info.arg&&(!*opt_info.arg||*opt_info.arg=='-'))?(PRINT_TABLE|PRINT_NO_HEADER):0);
				continue;
			}
			o = sh_lookopt(opt_info.arg,&f);
			if(o<=0 || (setflag && (o&SH_COMMANDLINE)))
			{
				errormsg(SH_DICT,2, "%s: %s option", opt_info.arg, o<0 ? "ambiguous" : "unknown");
				error_info.errors++;
			}
			o &= 0xff;
			if(sh_isoption(SH_RESTRICTED) && !f && o==SH_RESTRICTED)
			{
				errormsg(SH_DICT,ERROR_exit(1), e_restricted, opt_info.arg);
				UNREACHABLE();
			}
			break;
		    case -6:	/* --default */
			{
				const Shtable_t *tp;
				for(tp=shtab_options; o = tp->sh_number; tp++)
				{
					if(!(o&SH_COMMANDLINE) && (o&=0xff)!=SH_RESTRICTED && is_option(&newflags,o))
					{
						off_option(&newflags,o);
						if(o==SH_POSIX)
							sh_invalidate_ifs();
					}
				}
			}
			defaultflag++;
		    	continue;
		    case -7:	/* --state */
			f = 0;
		    	goto byname;
	 	    case 'D':
			on_option(&newflags,SH_NOEXEC);
			goto skip;
		    case 's':
			if(setflag)
			{
				action = SORT;
				continue;
			}
#if SHOPT_KIA
			goto skip;
		    case 'R':
			if(setflag)
				n = ':';
			else
			{
				ap->kiafile = opt_info.arg;
				n = 'n';
			}
#endif /* SHOPT_KIA */
#if SHOPT_REGRESS
			goto skip;
		    case 'I':
			continue;
#endif /* SHOPT_REGRESS */
			/* FALLTHROUGH */
		    skip:
		    default:
			if(cp=strchr(optksh,n))
				o = flagval[cp-optksh];
			break;
		    case -5:	/* --posix must be handled explicitly to stop AST optget(3) overriding it */
		    case ':':
			if(opt_info.name[0]=='-'&&opt_info.name[1]=='-')
			{
				opt_info.arg = argv[opt_info.index-1] + 2;
				f = 1;
				goto byname;
			}
			errormsg(SH_DICT,2, "%s", opt_info.arg);
			continue;
		    case '?':
			errormsg(SH_DICT,ERROR_usage(0), "%s", opt_info.arg);
			return -1;
		}
		if(f)
		{
#if SHOPT_ESH && SHOPT_VSH
			if(o==SH_VI || o==SH_EMACS || o==SH_GMACS)
			{
				off_option(&newflags,SH_VI);
				off_option(&newflags,SH_EMACS);
				off_option(&newflags,SH_GMACS);
			}
#elif SHOPT_ESH
			if(o==SH_EMACS || o==SH_GMACS)
			{
				off_option(&newflags,SH_EMACS);
				off_option(&newflags,SH_GMACS);
			}
#endif /* SHOPT_ESH && SHOPT_VSH */
			if(o==SH_POSIX && !defaultflag)
			{
#if SHOPT_BRACEPAT
				off_option(&newflags,SH_BRACEEXPAND);
#endif
				on_option(&newflags,SH_LETOCTAL);
				sh_invalidate_ifs();
			}
			on_option(&newflags,o);
			off_option(&sh.offoptions,o);
		}
		else
		{
			if ((o == SH_RESTRICTED) && sh_isoption(SH_RESTRICTED))
			{
				errormsg(SH_DICT,ERROR_exit(1),e_restricted,"r"); /* set -r cannot be unset */
				UNREACHABLE();
			}
			if(o==SH_POSIX && !defaultflag)
			{
#if SHOPT_BRACEPAT
				on_option(&newflags,SH_BRACEEXPAND);
#endif
				off_option(&newflags,SH_LETOCTAL);
				sh_invalidate_ifs();
			}
			if(o==SH_XTRACE)
				trace = 0;
			off_option(&newflags,o);
			if(setflag==0)
				on_option(&sh.offoptions,o);
		}
	}
	if(error_info.errors)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NULL));
		UNREACHABLE();
	}
	/* check for '-' or '+' argument */
	if((cp=argv[opt_info.index]) && cp[1]==0 && (*cp=='+' || *cp=='-') &&
		strcmp(argv[opt_info.index-1],"--"))
	{
		opt_info.index++;
		off_option(&newflags,SH_XTRACE);
		off_option(&newflags,SH_VERBOSE);
		trace = 0;
	}
	if(trace)
		sh_trace(argv,1);
	argc -= opt_info.index;
	argv += opt_info.index;
	if(action==PRINT)
		sh_printopts(newflags,verbose,0);
	if(setflag)
	{
		if(action==SORT)
		{
			if(argc>0)
				strsort(argv,argc,strcoll);
			else
				strsort(sh.st.dolv+1,sh.st.dolc,strcoll);
		}
		if(np)
			nv_setvec(np,0,argc,argv);
		else if(argc>0 || ((cp=argv[-1]) && strcmp(cp,"--")==0))
			argset(ap,argv-1);
	}
	else if(is_option(&newflags,SH_CFLAG))
	{
		if(!(sh.comdiv = *argv++))
		{
			errormsg(SH_DICT,2,e_cneedsarg);
			errormsg(SH_DICT,ERROR_usage(2),optusage(NULL));
			UNREACHABLE();
		}
		argc--;
	}
	/* SH_INTERACTIVE and SH_PRIVILEGED are handled in applyopts() */
	applyopts(newflags);
#if SHOPT_KIA
	if(ap->kiafile)
	{
		if(!argv[0])
		{
			errormsg(SH_DICT,ERROR_usage(2),"-R requires scriptname");
			UNREACHABLE();
		}
		if(!(lp->kiafile=sfopen(NULL,ap->kiafile,"w+")))
		{
			errormsg(SH_DICT,ERROR_system(3),e_create,ap->kiafile);
			UNREACHABLE();
		}
		if(!(lp->kiatmp=sftmp(2*SF_BUFSIZE)))
		{
			errormsg(SH_DICT,ERROR_system(3),e_tmpcreate);
			UNREACHABLE();
		}
		sfputr(lp->kiafile,";vdb;CIAO/ksh",'\n');
		lp->kiabegin = sftell(lp->kiafile);
		lp->entity_tree = dtopen(&_Nvdisc,Dtbag);
		lp->scriptname = sh_strdup(sh_fmtq(argv[0]));
		lp->script=kiaentity(lp,lp->scriptname,-1,'p',-1,0,0,'s',0,"");
		lp->fscript=kiaentity(lp,lp->scriptname,-1,'f',-1,0,0,'s',0,"");
		lp->unknown=kiaentity(lp,"<unknown>",-1,'p',-1,0,0,'0',0,"");
		kiaentity(lp,"<unknown>",-1,'p',0,0,lp->unknown,'0',0,"");
		lp->current = lp->script;
		ap->kiafile = 0;
	}
#endif /* SHOPT_KIA */
	return argc;
}

/* apply new options */

static void applyopts(Shopt_t newflags)
{
	/* cannot set -n for interactive shells since there is no way out */
	if(sh_isoption(SH_INTERACTIVE))
		off_option(&newflags,SH_NOEXEC);
	if(is_option(&newflags,SH_PRIVILEGED))
		on_option(&newflags,SH_NOUSRPROFILE);
	if(!sh_isstate(SH_INIT) && is_option(&newflags,SH_PRIVILEGED) != sh_isoption(SH_PRIVILEGED)
	|| sh_isstate(SH_INIT) && is_option(&sh.offoptions,SH_PRIVILEGED) && sh.userid!=sh.euserid)
	{
		if(!is_option(&newflags,SH_PRIVILEGED))
		{
			setuid(sh.userid);
			setgid(sh.groupid);
			if(sh.euserid==0)
			{
				sh.euserid = sh.userid;
				sh.egroupid = sh.groupid;
			}
		}
		else if((sh.userid!=sh.euserid && setuid(sh.euserid)<0) ||
			(sh.groupid!=sh.egroupid && setgid(sh.egroupid)<0) ||
			(sh.userid==sh.euserid && sh.groupid==sh.egroupid))
				off_option(&newflags,SH_PRIVILEGED);
	}
	/* sync monitor (part of job control) state with -o monitor option change */
	if(!sh_isoption(SH_MONITOR) && is_option(&newflags,SH_MONITOR))
		sh_onstate(SH_MONITOR);
	else if(sh_isoption(SH_MONITOR) && !is_option(&newflags,SH_MONITOR))
		sh_offstate(SH_MONITOR);
	sh.options = newflags;
}

/*
 * returns the value of $-
 */
char *sh_argdolminus(void* context)
{
	Arg_t *ap = (Arg_t*)context;
	const char *cp=optksh;
	char *flagp=ap->flagadr;
	while(cp< &optksh[NUM_OPTS])
	{
		int n = flagval[cp-optksh];
		if(sh_isoption(n))
			*flagp++ = *cp;
		cp++;
	}
	*flagp = 0;
	return ap->flagadr;
}

/*
 * set up positional parameters 
 */
static void argset(Arg_t *ap,char *argv[])
{
	sh_argfree(ap->dolh,0);
	ap->dolh = sh_argcreate(argv);
	/* link into chain */
	ap->dolh->dolnxt = ap->argfor;
	ap->argfor = ap->dolh;
	sh.st.dolc = ap->dolh->dolnum-1;
	sh.st.dolv = ap->dolh->dolval;
}

/*
 * free the argument list if the use count is 1
 * If count is greater than 1 decrement count and return same blk
 * Free the argument list if the use count is 1 and return next blk
 * Delete the blk from the argfor chain
 * If flag is set, then the block dolh is not freed
 */
struct dolnod *sh_argfree(struct dolnod *blk,int flag)
{
	struct dolnod*	argr=blk;
	struct dolnod*	argblk;
	Arg_t *ap = (Arg_t*)sh.arg_context;
	if(argblk=argr)
	{
		if((--argblk->dolrefcnt)==0)
		{
			argr = argblk->dolnxt;
			if(flag && argblk==ap->dolh)
				ap->dolh->dolrefcnt = 1;
			else
			{
				/* delete from chain */
				if(ap->argfor == argblk)
					ap->argfor = argblk->dolnxt;
				else
				{
					for(argr=ap->argfor;argr;argr=argr->dolnxt)
						if(argr->dolnxt==argblk)
							break;
					if(!argr)
						return NULL;
					argr->dolnxt = argblk->dolnxt;
					argr = argblk->dolnxt;
				}
				free(argblk);
			}
		}
	}
	return argr;
}

/*
 * grab space for arglist and copy args
 * The strings are copied after the argument vector
 */
struct dolnod *sh_argcreate(char *argv[])
{
	struct dolnod *dp;
	char **pp=argv, *sp;
	int 	n;
	size_t	size=0;
	/* count args and number of bytes of arglist */
	while(sp= *pp++)
		size += strlen(sp);
	n = (pp - argv)-1;
	dp=new_of(struct dolnod,n*sizeof(char*)+size+n);
	dp->dolrefcnt=1;	/* use count */
	dp->dolnum = n;
	dp->dolnxt = 0;
	pp = dp->dolval;
	sp = (char*)dp + sizeof(struct dolnod) + n*sizeof(char*);
	while(n--)
	{
		*pp++ = sp;
		sp = strcopy(sp, *argv++) + 1;
	}
	*pp = NULL;
	return dp;
}

/*
 *  used to set new arguments for functions
 */
struct dolnod *sh_argnew(char *argi[], struct dolnod **savargfor)
{
	Arg_t *ap = (Arg_t*)sh.arg_context;
	struct dolnod *olddolh = ap->dolh;
	*savargfor = ap->argfor;
	ap->dolh = 0;
	ap->argfor = 0;
	argset(ap,argi);
	return olddolh;
}

/*
 * reset arguments as they were before function
 */
void sh_argreset(struct dolnod *blk, struct dolnod *afor)
{
	Arg_t *ap = (Arg_t*)sh.arg_context;
	while(ap->argfor=sh_argfree(ap->argfor,0));
	ap->argfor = afor;
	if(ap->dolh = blk)
	{
		sh.st.dolc = ap->dolh->dolnum-1;
		sh.st.dolv = ap->dolh->dolval;
	}
}

/*
 * increase the use count so that an argset will not make it go away
 */
struct dolnod *sh_arguse(void)
{
	struct dolnod *dh;
	Arg_t *ap = (Arg_t*)sh.arg_context;
	if(dh=ap->dolh)
		dh->dolrefcnt++;
	return dh;
}

/*
 *  Print option settings on standard output
 *  if mode is inclusive or of PRINT_*
 *  if <mask> is set, only options with this mask value are displayed
 */
void sh_printopts(Shopt_t oflags,int mode, Shopt_t *mask)
{
	const Shtable_t *tp;
	const char *name;
	int on;
	int value;
	if(!(mode&PRINT_NO_HEADER))
		sfputr(sfstdout,sh_translate(e_heading),'\n');
	if(mode&PRINT_TABLE)
	{
		int	w;
		int	c;
		int	r;
		int	i;

		c = 0;
		for(tp=shtab_options; value=tp->sh_number; tp++)
		{
			if(mask && !is_option(mask,value&0xff))
				continue;
			name = tp->sh_name;
			if(name[0] == 'n' && name[1] == 'o' && name[2] != 't')
				name += 2;
			if(c<(w=strlen(name)))
				c = w;
		}
		c += 4;
		if((w = ed_window()) < (2*c))
			w = 2*c;
		r = w / c;
		i = 0;
		for(tp=shtab_options; value=tp->sh_number; tp++)
		{
			if(mask && !is_option(mask,value&0xff))
				continue;
			on = !!is_option(&oflags,value);
			value &= 0xff;
			name = tp->sh_name;
			if(name[0] == 'n' && name[1] == 'o' && name[2] != 't')
			{
				name += 2;
				on = !on;
			}
			if(++i>=r)
			{
				i = 0;
				sfprintf(sfstdout, "%s%s\n", on ? "" : "no", name);
			}
			else
				sfprintf(sfstdout, "%s%-*s", on ? "" : "no", on ? c : (c-2), name);
		}
		if(i)
			sfputc(sfstdout,'\n');
		return;
	}
#if SHOPT_VSH
	on_option(&oflags,SH_VIRAW);
#endif
	if(!(mode&(PRINT_ALL|PRINT_VERBOSE))) /* only print set options */
		sfwrite(sfstdout,"set --default",13);
	for(tp=shtab_options; value=tp->sh_number; tp++)
	{
		if(mask && !is_option(mask,value&0xff))
			continue;
		on = !!is_option(&oflags,value);
		name = tp->sh_name;
		if(name[0] == 'n' && name[1] == 'o' && name[2] != 't')
		{
			name += 2;
			on = !on;
		}
		if(mode&PRINT_VERBOSE)
		{
			sfputr(sfstdout,name,' ');
			sfnputc(sfstdout,' ',24-strlen(name));
			sfputr(sfstdout,on ? "on" : "off",'\n');
		}
		else if(mode&PRINT_ALL) /* print unset options also */
			sfprintf(sfstdout, "set %co %s\n", on?'-':'+', name);
		else if(!(value&SH_COMMANDLINE) && is_option(&oflags,value&0xff))
			sfprintf(sfstdout, " %s%s%s","--", on?"":"no", name);
	}
	if(!(mode&(PRINT_VERBOSE|PRINT_ALL)))
		sfputc(sfstdout,'\n');
}

/*
 * build an argument list
 */
char **sh_argbuild(int *nargs, const struct comnod *comptr,int flag)
{
	struct argnod *argp=0;
	struct argnod *arghead=0;
	sh.xargmin = 0;
	{
		const struct comnod *ac = comptr;
		int n;
		/* see if the arguments have already been expanded */
		if(!ac->comarg)
		{
			*nargs = 0;
			return &null;
		}
		else if(!(ac->comtyp&COMSCAN))
		{
			struct dolnod *ap = (struct dolnod*)ac->comarg;
			*nargs = ap->dolnum;
			return ap->dolval+ap->dolbot;
		}
		*nargs = 0;
		if(ac)
		{
			argp = ac->comarg;
			while(argp)
			{
				n = arg_expand(argp,&arghead,flag);
				if(n>1)
				{
					if(sh.xargmin==0)
						sh.xargmin = *nargs;
					sh.xargmax = *nargs+n;
				}
				*nargs += n;
				argp = argp->argnxt.ap;
			}
			argp = arghead;
		}
	}
	{
		char	**comargn;
		int	argn, argi;
		char	**comargm;
		/*
		 * When argbuild is aborted (longjmp) from a discipline function (unset
		 * var access in discipline), we count an arg that is unset at the end of
		 * the list, generating a double NULL at the end. To avoid a potential
		 * null pointer dereference later on, use argi to recount the arguments.
		 * TODO: find/fix root cause, eliminate argi
		 */
		argn = *nargs + 1;	/* allow room to prepend args */
		comargn=(char**)stkalloc(sh.stk,(unsigned)(argn+1)*sizeof(char*));
		comargm = comargn += argn;
		*comargn = NULL;
		if(!argp)
		{
			/* reserve an extra null pointer */
			*--comargn = 0;
			return comargn;
		}
		argi = 0;
		while(argp)
		{
			struct argnod *nextarg = argp->argchn.ap;
			argp->argchn.ap = 0;
			*--comargn = argp->argval;
			if(!(argp->argflag&ARG_RAW))
				sh_trim(*comargn);
			if(!(argp=nextarg) || (argp->argflag&ARG_MAKE))
			{
				if((argn=comargm-comargn)>1)
					strsort(comargn,argn,strcoll);
				comargm = comargn;
			}
			argi++;
		}
		sh.last_table = 0;
		*nargs=argi;
		return comargn;
	}
}

#if _pipe_socketpair && !_socketpair_devfd
#   define sh_pipe(a)	sh_rpipe(a)
#endif

struct argnod *sh_argprocsub(struct argnod *argp)
{
	/* argument of the form <(cmd) or >(cmd) */
	struct argnod *ap;
	int fd, pv[3];
	int savestates = sh_getstate();
	char savejobcontrol = job.jobcontrol;
	unsigned int savesubshell = sh.subshell;
	ap = (struct argnod*)stkseek(sh.stk,ARGVAL);
	ap->argflag |= ARG_MAKE;
	ap->argflag &= ~ARG_RAW;
	fd = argp->argflag&ARG_RAW;
	if(fd==0 && sh.subshell)
		sh_subtmpfile();
#if SHOPT_DEVFD
	sfwrite(sh.stk,e_devfdNN,8);
	pv[2] = 0;
	sh_pipe(pv);
#else
	pv[0] = -1;
	while(sh.fifo = pathtemp(0,0,0,"ksh.fifo",0), sh.fifo && mkfifo(sh.fifo,0)<0)
	{
		if(errno==EEXIST || errno==EACCES || errno==ENOENT || errno==ENOTDIR || errno==EROFS)
			continue;		/* lost race (name conflict or tmp dir change); try again */
		sh.fifo = 0;
		break;
	}
	if(!sh.fifo)
	{
		errormsg(SH_DICT, ERROR_SYSTEM|ERROR_PANIC, "process substitution: FIFO creation failed");
		UNREACHABLE();
	}
	chmod(sh.fifo,S_IRUSR|S_IWUSR);	/* mkfifo + chmod works regardless of umask */
	sfputr(sh.stk,sh.fifo,0);
#endif /* SHOPT_DEVFD */
	sfputr(sh.stk,fmtbase((intmax_t)pv[fd],10,0),0);
	ap = (struct argnod*)stkfreeze(sh.stk,0);
	sh.inpipe = sh.outpipe = 0;
	/* turn off job control */
	sh_offstate(SH_INTERACTIVE);
	sh_offstate(SH_MONITOR);
	sh_offstate(SH_PROFILE);
	job.jobcontrol = 0;
	/* run the process substitution */
	sh.subshell = 0;
	if(fd)
		sh.inpipe = pv;
	else
		sh.outpipe = pv;
	sh_exec((Shnode_t*)argp->argchn.ap,(int)sh_isstate(SH_ERREXIT));
	/* restore the previous state */
	sh.subshell = savesubshell;
	job.jobcontrol = savejobcontrol;
	sh_setstate(savestates);
#if SHOPT_DEVFD
	sh_close(pv[1-fd]);
	sh_iosave(-pv[fd], sh.topfd, NULL);
#else
	/* remember the FIFO for cleanup in case the command never opens it (see fifo_cleanup(), xec.c) */
	if(!sh.fifo_tree)
		sh.fifo_tree = dtopen(&_Nvdisc,Dtoset);
	nv_search(sh.fifo,sh.fifo_tree,NV_ADD);
	free(sh.fifo);
	sh.fifo = 0;
#endif /* SHOPT_DEVFD */
	return ap;
}

/* Argument expansion */
static int arg_expand(struct argnod *argp, struct argnod **argchain,int flag)
{
	int count = 0;
	argp->argflag &= ~ARG_MAKE;
	if(*argp->argval==0 && (argp->argflag&ARG_EXP))
	{
		struct argnod *ap;
		ap = sh_argprocsub(argp);
		ap->argchn.ap = *argchain;
		*argchain = ap;
		count++;
	}
	else
	if(!(argp->argflag&ARG_RAW))
	{
#if SHOPT_OPTIMIZE
		struct argnod *ap;
		sh_stats(STAT_ARGEXPAND);
		if(flag&ARG_OPTIMIZE)
			argp->argchn.ap=0;
		if(ap=argp->argchn.ap)
		{
			sh_stats(STAT_ARGHITS);
			count = 1;
			ap->argchn.ap = *argchain;
			ap->argflag |= ARG_RAW;
			ap->argflag &= ~ARG_EXP;
			*argchain = ap;
		}
		else
#endif /* SHOPT_OPTIMIZE */
		count = sh_macexpand(argp,argchain,flag);
	}
	else
	{
		argp->argchn.ap = *argchain;
		*argchain = argp;
		argp->argflag |= ARG_MAKE;
		count++;
	}
	return count;
}

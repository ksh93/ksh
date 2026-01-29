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
 * trap  [-lp]  action sig...
 * kill  [-lL] [sig...]
 * kill  [-n signum] [-s signame] pid...
 * stop  job...
 * suspend
 *
 *   David Korn
 *   AT&T Labs
 *   research!dgk
 *
 */

#include	"shopt.h"
#include	"defs.h"
#include	"jobs.h"
#include	"builtins.h"

#define L_FLAG	1
#define S_FLAG	2

static const char trapfmt[] = "trap -- %s %s\n";

static int	sig_number(const char*);
static void	sig_list(int);

int	b_trap(int argc,char *argv[],Shbltin_t *context)
{
	char *arg = argv[1];
	int sig, clear = 0, dflag = 0, lflag = 0, pflag = 0;
	NOT_USED(argc);
	NOT_USED(context);
	while (sig = optget(argv, sh_opttrap)) switch (sig)
	{
	    case 'l':
		lflag=1;
		break;
	    case 'p':
		pflag=1;
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
	if(lflag)
	{
		sig_list(-1);
		return(0);
	}
	if(arg = *argv)
	{
		char *action = arg;
		if(!dflag && !pflag)
		{
			/* first argument all digits or - means clear */
			while(isdigit(*arg))
				arg++;
			clear = (arg!=action && *arg==0);
			if(!clear)
			{
				++argv;
				if(*action=='-' && action[1]==0)
					clear++;
				/*
				 * NOTE: 2007-11-26: workaround for tests/signal.sh
				 * if function semantics can be worked out then it
				 * may merit a -d/--default option
				 */
				else if(*action=='+' && action[1]==0 && sh.st.self == &sh.global && !sh_isoption(SH_POSIX))
				{
					clear++;
					dflag++;
				}
			}
			if(!argv[0])
			{
				errormsg(SH_DICT,ERROR_exit(1),e_condition);
				UNREACHABLE();
			}
		}
		while(arg = *argv++)
		{
			sig = sig_number(arg);
			if(sig<0)
			{
				errormsg(SH_DICT,2,e_trap,arg);
				return 1;
			}
			/* internal traps */
			if(sig&SH_TRAP)
			{
				char **trap = (sh.st.otrap ? sh.st.otrap : sh.st.trap);
				sig &= ~SH_TRAP;
				if(sig>SH_DEBUGTRAP)
				{
					errormsg(SH_DICT,2,e_trap,arg);
					return 1;
				}
				if(pflag)
				{
					if(arg=trap[sig])
						sfputr(sfstdout,sh_fmtq(arg),'\n');
					continue;
				}
				sh.st.otrap = 0;
				if(sh.st.trap[sig])
					free(sh.st.trap[sig]);
				sh.st.trap[sig] = 0;
				if(!clear && *action)
					sh.st.trap[sig] = sh_strdup(action);
				if(sig == SH_DEBUGTRAP)
				{
					if(sh.st.trap[sig])
						sh.trapnote |= SH_SIGTRAP;
					else
						sh.trapnote = 0;

				}
				continue;
			}
			if(sig > sh.sigmax)
			{
				errormsg(SH_DICT,2,e_trap,arg);
				return 1;
			}
			else if(pflag)
			{
				char **trapcom = (sh.st.otrapcom ? sh.st.otrapcom : sh.st.trapcom);
				if(arg=trapcom[sig])
					sfputr(sfstdout,arg,'\n');
			}
			else if(clear)
			{
				sh_sigclear(sig);
				if(dflag)
					signal(sig,SIG_DFL);
			}
			else
			{
				/*
				 * Trap or ignore a real signal. A virtual subshell needs to fork in
				 * order to receive signals correctly and (because other commands
				 * may cause a virtual subshell to fork) to ensure a persistent PID.
				 */
				if(sh.subshell && !sh.subshare)
					sh_subfork();
				if(sig >= sh.st.trapmax)
					sh.st.trapmax = sig+1;
				arg = sh.st.trapcom[sig];
				sh_sigtrap(sig);
				sh.st.trapcom[sig] = (sh.sigflag[sig]&SH_SIGOFF) ? Empty : sh_strdup(action);
				if(arg && arg != Empty)
					free(arg);
			}
		}
		/*
		 * Set a flag for sh_exec() to disable exec-without-fork optimizations if any trap is set and non-empty.
		 * (In ksh functions, there may be parent scope traps, so do not reset to 0 if in a ksh function.)
		 */
		if(sh.fn_depth==0)
			sh.st.trapdontexec = 0;
		if(!sh.st.trapdontexec)
		{
			/* EXIT and real signals */
			for(sig=0; sig<=sh.sigmax; sig++)
			{
				/* these cannot be trapped */
				if(sig==SIGKILL || sig==SIGSTOP)
					continue;
				if(sh.st.trapcom[sig] && *sh.st.trapcom[sig])
				{
					sh.st.trapdontexec++;
					break;
				}
			}
		}
		if(!sh.st.trapdontexec)
		{
			/* other pseudosignals -- exclude DEBUG as it is executed before the command */
			for(sig=0; sig<SH_DEBUGTRAP; sig++)
			{
				if(sh.st.trap[sig] && *sh.st.trap[sig])
				{
					sh.st.trapdontexec++;
					break;
				}
			}
		}
	}
	else /* print out current traps */
		sig_list(-2);
	return 0;
}

#if 0
    /* for the dictionary generator */
    int    b_stop(int argc,char *argv[],Shbltin_t *context){}
#endif
int	b_kill(int argc,char *argv[],Shbltin_t *context)
{
	char *signame;
	int sig=SIGTERM, flag=0, n;
	int usemenu = 0;
	NOT_USED(argc);
	NOT_USED(context);
	if(**argv == 's')	/* <s>top == kill -s STOP */
	{
		flag |= S_FLAG;
		signame = "STOP";
	}
	while((n = optget(argv, **argv == 's' ? sh_optstop : sh_optkill))) switch(n)
	{
		case ':':
			if((signame=argv[opt_info.index++]) && (sig=sig_number(signame+1))>=0)
				goto endopts;
			opt_info.index--;
			errormsg(SH_DICT,2, "%s", opt_info.arg);
			break;
		case 'n':
			sig = (int)opt_info.num;
			goto endopts;
		case 's':
			flag |= S_FLAG;
			signame = opt_info.arg;
			goto endopts;
		case 'L':
			usemenu = -1;
			/* FALLTHROUGH */
		case 'l':
			flag |= L_FLAG;
			break;
		case '?':
			/* self-doc: write to standard output */
			error(ERROR_USAGE|ERROR_OUTPUT, STDOUT_FILENO, "%s", opt_info.arg);
			return 0;
	}
endopts:
	argv += opt_info.index;
	if(*argv && strcmp(*argv,"--")==0 && strcmp(*(argv-1),"--")!=0)
		argv++;
	if(error_info.errors || flag==(L_FLAG|S_FLAG) || (!(*argv) && !(flag&L_FLAG)))
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s", optusage(NULL));
		UNREACHABLE();
	}
	/* just in case we send a kill -9 $$ */
	sfsync(sfstderr);
	if(flag&L_FLAG)
	{
		if(!(*argv))
			sig_list(usemenu);
		else while(signame = *argv++)
		{
			if(isdigit(*signame))
				sig_list(((int)strtol(signame, NULL, 10)&0177)+1);
			else
			{
				if((sig=sig_number(signame))<0)
				{
					sh.exitval = 2;
					errormsg(SH_DICT,ERROR_exit(1),e_nosignal,signame);
					UNREACHABLE();
				}
				sfprintf(sfstdout,"%d\n",sig);
			}
		}
		return sh.exitval;
	}
	if(flag&S_FLAG)
	{
		if((sig=sig_number(signame)) < 0 || sig > sh.sigmax)
		{
			errormsg(SH_DICT,ERROR_exit(1),e_nosignal,signame);
			UNREACHABLE();
		}
	}
	if(job_walk(sfstdout,job_kill,sig,argv))
		sh.exitval = 1;
	return sh.exitval;
}

/*
 * former default alias suspend='kill -s STOP $$'
 */
int	b_suspend(int argc,char *argv[],Shbltin_t *context)
{
	int n;
	NOT_USED(argc);
	NOT_USED(context);
	while((n = optget(argv, sh_optsuspend))) switch(n)
	{
		case ':':
			errormsg(SH_DICT,2, "%s", opt_info.arg);
			break;
		case '?':
			/* self-doc: write to standard output */
			error(ERROR_USAGE|ERROR_OUTPUT, STDOUT_FILENO, "%s", opt_info.arg);
			return 0;
	}
	if(error_info.errors)	/* no options supported (except AST --man, etc.) */
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s", optusage(NULL));
		UNREACHABLE();
	}
	if(argv[opt_info.index])	/* no operands supported */
	{
		errormsg(SH_DICT, ERROR_exit(2), e_toomanyops);
		UNREACHABLE();
	}
	if(sh_isoption(SH_LOGIN_SHELL))
	{
		errormsg(SH_DICT, ERROR_exit(1), "cannot suspend a login shell");
		UNREACHABLE();
	}
	if(kill(sh.pid, SIGSTOP) != 0)
	{
		errormsg(SH_DICT, ERROR_exit(1), "could not signal main shell at PID %jd", (Sflong_t)sh.pid);
		UNREACHABLE();
	}
	return 0;
}

/*
 * Given the name or number of a signal return the signal number
 */
static int sig_number(const char *string)
{
	const Shtable_t	*tp;
	int		n, o, sig=0;
	char		*last, *name;
	if(isdigit(*string))
	{
		n = strtol(string,&last,10);
		if(*last)
			n = -1;
	}
	else
	{
		int c;
		o = stktell(sh.stk);
		do
		{
			c = *string++;
			if(islower(c))
				c = toupper(c);
			sfputc(sh.stk,c);
		}
		while(c);
		stkseek(sh.stk,o);
		if(strncmp(stkptr(sh.stk,o),"SIG",3)==0)
		{
			sig = 1;
			o += 3;
			if(isdigit(*stkptr(sh.stk,o)))
			{
				n = strtol(stkptr(sh.stk,o),&last,10);
				if(!*last)
					return n;
			}
		}
		tp = sh_locate(stkptr(sh.stk,o),(const Shtable_t*)shtab_signals,sizeof(*shtab_signals));
		n = tp->sh_number;
		if(sig==1 && (n>=(SH_TRAP-1) && n < (1<<SH_SIGBITS)))
		{
			/* sig prefix cannot match internal traps */
			n = 0;
			tp = (Shtable_t*)((char*)tp + sizeof(*shtab_signals));
			if(strcmp(stkptr(sh.stk,o),tp->sh_name)==0)
				n = tp->sh_number;
		}
		if((n>>SH_SIGBITS)&SH_SIGRUNTIME)
			n = sh.sigruntime[(n&((1<<SH_SIGBITS)-1))-1];
		else
		{
			n &= (1<<SH_SIGBITS)-1;
			if(n < SH_TRAP)
				n--;
		}
		if(n<0 && sh.sigruntime[1] && (name=stkptr(sh.stk,o)) && *name++=='R' && *name++=='T')
		{
			/* Real-time signals */
			if(name[0]=='M' && name[1]=='I' && name[2]=='N' && name[3]=='+')	/* MIN+ */
			{
				if((sig=(int)strtol(name+4,&name,10)) >= 0 && !*name)
					n = sh.sigruntime[SH_SIGRTMIN] + sig;
			}
			else if(name[0]=='M' && name[1]=='A' && name[2]=='X' && name[3]=='-')	/* MAX- */
			{
				if((sig=(int)strtol(name+4,&name,10)) >= 0 && !*name)
					n = sh.sigruntime[SH_SIGRTMAX] - sig;
			}
			else if((sig=(int)strtol(name,&name,10)) > 0 && !*name)
				n = sh.sigruntime[SH_SIGRTMIN] + sig - 1;
			if(n < sh.sigruntime[SH_SIGRTMIN] || n > sh.sigruntime[SH_SIGRTMAX])
				n = -1;
		}
	}
	return n;
}

/*
 * synthesize signal name for sig in buf
 * pfx!=0 prepends SIG to default signal number
 */
static char* sig_name(int sig, char* buf, int pfx)
{
	int	i;

	i = 0;
	if(sig > sh.sigruntime[SH_SIGRTMIN] && sig < sh.sigruntime[SH_SIGRTMAX])
	{
		buf[i++] = 'R';
		buf[i++] = 'T';
		buf[i++] = 'M';
		if(sig > sh.sigruntime[SH_SIGRTMIN] + (sh.sigruntime[SH_SIGRTMAX] - sh.sigruntime[SH_SIGRTMIN]) / 2)
		{
			buf[i++] = 'A';
			buf[i++] = 'X';
			buf[i++] = '-';
			sig = sh.sigruntime[SH_SIGRTMAX] - sig;
		}
		else
		{
			buf[i++] = 'I';
			buf[i++] = 'N';
			buf[i++] = '+';
			sig = sig - sh.sigruntime[SH_SIGRTMIN];
		}
	}
	else if(pfx)
	{
		buf[i++] = 'S';
		buf[i++] = 'I';
		buf[i++] = 'G';
	}
	i += sfsprintf(buf+i, 8, "%d", sig);
	buf[i] = 0;
	return buf;
}

/*
 * if <flag> is positive, then print signal name corresponding to <flag>
 * if <flag> is zero, then print all signal names
 * if <flag> is -1, then print all signal names in menu format
 * if <flag> is <-1, then print all traps
 */
static void sig_list(int flag)
{
	const struct shtable2	*tp;
	int sig;
	char *sname;
	char name[10];
	const char *names[SH_TRAP];
	const char *traps[SH_DEBUGTRAP+1];
	tp=shtab_signals;
	if(flag<=0)
	{
		/* not all signals may be defined, so initialize */
		for(sig=sh.sigmax; sig>=0; sig--)
			names[sig] = 0;
		for(sig=SH_DEBUGTRAP; sig>=0; sig--)
			traps[sig] = 0;
	}
	for(; *tp->sh_name; tp++)
	{
		sig = tp->sh_number&((1<<SH_SIGBITS)-1);
		if (((tp->sh_number>>SH_SIGBITS) & SH_SIGRUNTIME) && (sig = sh.sigruntime[sig-1]+1) == 1)
			continue;
		if(sig==flag)
		{
			sfprintf(sfstdout,"%s\n",tp->sh_name);
			return;
		}
		else if(sig&SH_TRAP)
			traps[sig&~SH_TRAP] = (char*)tp->sh_name;
		else if(sig-- && sig < elementsof(names))
			names[sig] = (char*)tp->sh_name;
	}
	if(flag > 0)
		sfputr(sfstdout, sig_name(flag-1,name,0), '\n');
	else if(flag<-1)
	{
		/* print the traps */
		char *trap,**trapcom;
		sig = sh.st.trapmax;
		/* use parent traps if otrapcom is set (for $(trap)  */
		trapcom = (sh.st.otrapcom ? sh.st.otrapcom : sh.st.trapcom);
		while(--sig >= 0)
		{
			if(!(trap=trapcom[sig]))
				continue;
			if(sig > sh.sigmax || !(sname=(char*)names[sig]))
				sname = sig_name(sig,name,1);
			sfprintf(sfstdout,trapfmt,sh_fmtq(trap),sname);
		}
		for(sig=SH_DEBUGTRAP; sig>=0; sig--)
		{
			if(!(trap=sh.st.otrap ? sh.st.otrap[sig] : sh.st.trap[sig]))
				continue;
			sfprintf(sfstdout,trapfmt,sh_fmtq(trap),traps[sig]);
		}
	}
	else
	{
		/* print all the signal names */
		for(sig=1; sig <= sh.sigmax; sig++)
		{
			if(!(sname=(char*)names[sig]))
			{
				sname = sig_name(sig,name,1);
				if(flag)
					sname = stkcopy(sh.stk,sname);
			}
			if(flag)
				names[sig] = sname;
			else
				sfputr(sfstdout,sname,'\n');
		}
		if(flag)
		{
			names[sig] = 0;
			sh_menu(sfstdout,sh.sigmax,(char**)names+1);
		}
	}
}

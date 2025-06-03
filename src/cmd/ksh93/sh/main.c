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
*                  Lev Kujawski <int21h@mailbox.org>                   *
*                                                                      *
***********************************************************************/
/*
 * UNIX shell
 *
 * S. R. Bourne
 * Rewritten By David Korn
 * AT&T Labs
 *
 */

#include	"shopt.h"
#include	<ast.h>
#include	<sfio.h>
#include	<ls.h>
#include	<fcin.h>
#include	"defs.h"
#include	"variables.h"
#include	"path.h"
#include	"io.h"
#include	"jobs.h"
#include	"shlex.h"
#include	"shnodes.h"
#include	"history.h"
#include	"timeout.h"
#include	"FEATURE/time"
#include	"FEATURE/externs"
#ifdef	_hdr_nc
#   include	<nc.h>
#endif	/* _hdr_nc */

/* These routines are referenced by this module */
static void	exfile(Sfio_t*,int);
static void	chkmail(char*);
#if !defined(_NEXT_SOURCE) && !defined(__sun)
    static void	fixargs(char**,int);
#   undef fixargs_disabled
#else
#   define fixargs(a,b)
#   define fixargs_disabled	1
#endif

#ifndef environ
    extern char	**environ;
#endif

static struct stat lastmail;
static time_t	mailtime;
static char	beenhere = 0;

/*
 * search for file and exfile() it if it exists
 * 1 returned if file found, 0 otherwise
 */
static int sh_source(Sfio_t *iop, const char *file)
{
	char*	oid;
	char*	nid;
	int	fd;

	if (!file || !*file || (fd = path_open(file, NULL)) < 0)
		return 0;
	oid = error_info.id;
	nid = error_info.id = sh_strdup(file);
	sh.st.filename = path_fullname(stkptr(sh.stk,PATH_OFFSET));
	exfile(iop, fd);
	error_info.id = oid;
	free(nid);
	return 1;
}

#ifdef S_ISSOCK
#define REMOTE(m)	(S_ISSOCK(m)||!(m))
#else
#define REMOTE(m)	!(m)
#endif

noreturn void sh_main(int ac, char *av[], Shinit_f userinit)
{
	char		*name;
	int		fdin;
	Sfio_t		*iop;
	struct stat	statb;
	int		i;
	int		rshflag;	/* set for restricted shell */
	char		*command;
#ifdef	_hdr_nc
	_NutConf(_NC_SET_SUFFIXED_SEARCHING, 1);
#endif	/* _hdr_nc */
	fixargs(av,0);
	sh_init(ac,av,userinit);
	time(&mailtime);
	if(rshflag=sh_isoption(SH_RESTRICTED))
		sh_offoption(SH_RESTRICTED);
	if(sigsetjmp(*((sigjmp_buf*)sh.jmpbuffer),0))
	{
		/* begin script execution here */
		sh_reinit();
	}
	command = error_info.id;
	path_pwd();
	iop = NULL;
	if(sh_isoption(SH_POSIX))
		sh_onoption(SH_LETOCTAL);
#if SHOPT_BRACEPAT
	else
		sh_onoption(SH_BRACEEXPAND);
#endif
	if(!beenhere)
	{
		beenhere++;
		sh_onstate(SH_PROFILE);
		sh.sigflag[SIGTSTP] |= SH_SIGIGNORE;
		/* decide whether shell is interactive */
		if(!sh_isoption(SH_INTERACTIVE) && !sh_isoption(SH_TFLAG) && !sh_isoption(SH_CFLAG) &&
		   sh_isoption(SH_SFLAG) && tty_check(0) && tty_check(ERRIO))
			sh_onoption(SH_INTERACTIVE);
		if(sh_isoption(SH_INTERACTIVE))
		{
			const struct shtable2 *tp;
			sh_onoption(SH_BGNICE);
			sh_onoption(SH_RC);
			/* preset aliases for interactive ksh/sh */
			for(tp = shtab_aliases; *tp->sh_name; tp++)
			{
				Namval_t *np = sh_calloc(1,sizeof(Namval_t));
				np->nvname = (char*)tp->sh_name;	/* alias name */
				np->nvflag = tp->sh_number;		/* attributes (must include NV_NOFREE) */
				np->nvalue = (void*)tp->sh_value;	/* non-freeable value */
				dtinstall(sh.alias_tree,np);
			}
		}
#if SHOPT_REMOTE
		/*
		 * Building ksh with SHOPT_REMOTE=1 causes ksh to set --rc if stdin is
		 * a socket (presumably part of a remote shell invocation).
		 */
		if(!sh_isoption(SH_RC) && !fstat(0, &statb) && REMOTE(statb.st_mode))
			sh_onoption(SH_RC);
#endif
		for(i=0; i<elementsof(sh.offoptions.v); i++)
			sh.options.v[i] &= ~sh.offoptions.v[i];
		if(sh_isoption(SH_INTERACTIVE))
		{
#ifdef SIGXCPU
			signal(SIGXCPU,SIG_DFL);
#endif /* SIGXCPU */
#ifdef SIGXFSZ
			signal(SIGXFSZ,SIG_DFL);
#endif /* SIGXFSZ */
			sh_onoption(SH_MONITOR);
		}
		job_init();
		if(sh_isoption(SH_LOGIN_SHELL))
		{
			/*	system profile	*/
			sh_source(iop, e_sysprofile);
			if(!sh_isoption(SH_NOUSRPROFILE) && !sh_isoption(SH_PRIVILEGED))
			{
				char **files = sh.login_files;
				while ((name = *files++) && !sh_source(iop, sh_mactry(name)));
			}
		}
		/* make sure PWD is set up correctly */
		path_pwd();
		if(!sh_isoption(SH_NOEXEC))
		{
			if(!sh_isoption(SH_NOUSRPROFILE) && !sh_isoption(SH_PRIVILEGED) && sh_isoption(SH_RC))
			{
				if(name = sh_mactry(nv_getval(ENVNOD)))
					name = *name ? sh_strdup(name) : NULL;
#if SHOPT_SYSRC
				if(!strmatch(name, "?(.)/./*"))
					sh_source(iop, e_sysrc);
#endif
				if(name)
				{
					sh_source(iop, name);
					free(name);
				}
			}
			else if(sh_isoption(SH_INTERACTIVE) && sh_isoption(SH_PRIVILEGED))
				sh_source(iop, e_suidprofile);
		}
		sh.st.cmdname = error_info.id = command;
		sh.sigflag[SIGTSTP] &= ~(SH_SIGIGNORE);
		sh_offstate(SH_PROFILE);
		if(rshflag)
			sh_onoption(SH_RESTRICTED);
		/* open input file if specified */
		if(sh.comdiv)
		{
		shell_c:
			iop = sfnew(NULL,sh.comdiv,strlen(sh.comdiv),0,SFIO_STRING|SFIO_READ);
		}
		else
		{
			name = error_info.id;
			error_info.id = sh.shname;
			if(sh_isoption(SH_SFLAG))
				fdin = 0;
			else
			{
				char *sp;
				/* open stream should have been passed into shell */
				if(strmatch(name,e_devfdNN))
				{
					fdin = (int)strtol(name+8, NULL, 10);
					if(fstat(fdin,&statb)<0)
					{
						errormsg(SH_DICT,ERROR_system(1),e_open,name);
						UNREACHABLE();
					}
					name = av[0];
					sh_offoption(SH_VERBOSE);
					sh_offoption(SH_XTRACE);
				}
				else
				{
					int isdir = 0;
					if((fdin=sh_open(name,O_RDONLY,0))>=0 &&(fstat(fdin,&statb)<0 || S_ISDIR(statb.st_mode)))
					{
						close(fdin);
						isdir = 1;
						fdin = -1;
					}
					else
						sh.st.filename = path_fullname(name);
					sp = 0;
					if(fdin < 0 && !strchr(name,'/'))
					{
						if(path_absolute(name,NULL,0))
							sp = stkptr(sh.stk,PATH_OFFSET);
						if(sp)
						{
							if((fdin=sh_open(sp,O_RDONLY,0))>=0)
								sh.st.filename = path_fullname(sp);
						}
					}
					if(fdin<0)
					{
						if(isdir)
							errno = EISDIR;
						 error_info.id = av[0];
						if(sp || errno!=ENOENT)
						{
							errormsg(SH_DICT,ERROR_system(ERROR_NOEXEC),e_open,name);
							UNREACHABLE();
						}
						/* try sh -c 'name "$@"' */
						sh_onoption(SH_CFLAG);
						sh.comdiv = (char*)sh_malloc(strlen(name)+7);
						name = strcopy(sh.comdiv,name);
						if(sh.st.dolc)
							strcopy(name," \"$@\"");
						goto shell_c;
					}
					if(fdin==0)
						fdin = sh_iomovefd(fdin);
				}
				sh.readscript = sh.shname;
			}
			error_info.id = name;
#if SHOPT_ACCT
			sh_accinit();
			if(fdin != 0)
				sh_accbegin(error_info.id);
#endif	/* SHOPT_ACCT */
		}
		/* If the shell is initialised with std{in,out,err} closed, make the shell's FD state reflect that. */
		for(i=0; i<=2; i++)
			if(fcntl(i,F_GETFD,NULL)==-1 && errno==EBADF)	/* closed at OS level? */
				sh_close(i); 				/* update shell FD state */
	}
	else
	{
		/* beenhere > 0: We're in a forked child, about to execute a script without a hashbang path. */
		fdin = sh.infd;
		fixargs(sh.st.dolv,1);
	}
	sh_winsize();
	if(!sh.columns)
		sh.columns = 80;
	if(!sh.lines)
		sh.lines = 24;
	if(sh_isoption(SH_INTERACTIVE))
	{
		sh_onstate(SH_INTERACTIVE);
#if SHOPT_ESH
		/* do not leave users without a line editor */
		if(!sh_editor_active() && !is_option(&sh.offoptions,SH_EMACS))
			sh_onoption(SH_EMACS);
#endif /* SHOPT_ESH */
	}
	else
	{
		/* keep $COLUMNS and $LINES up to date even for scripts that don't trap SIGWINCH */
#ifdef SIGWINCH
		signal(SIGWINCH,sh_fault);
#endif /* SIGWINCH */
	}
	/* (Re)set PS4 and IFS, but don't export these now even if allexport is on. */
	i = (sh_isoption(SH_ALLEXPORT) != 0);
	sh_offoption(SH_ALLEXPORT);
	if(nv_isnull(PS4NOD))
		nv_putval(PS4NOD,e_traceprompt,NV_RDONLY);
	nv_putval(IFSNOD,(char*)e_sptbnl,NV_RDONLY);
	if(i)
		sh_onoption(SH_ALLEXPORT);
	/* Start main execution loop. */
	exfile(iop,fdin);
	sh_done(0);
}

/*
 * iop is not null when the input is a string
 * fdin is the input file descriptor
 */
static void	exfile(Sfio_t *iop,int fno)
{
	time_t curtime;
	Shnode_t *t;
	int maxtry=IOMAXTRY, tdone=0, execflags;
	int states,jmpval;
	struct checkpt buff;
	sh_pushcontext(&buff,SH_JMPERREXIT);
	/* open input stream */
	nv_putval(SH_PATHNAMENOD, sh.st.filename, NV_NOFREE);
	if(!iop)
	{
		if(fno > 0)
		{
			if(fno < 10)
			{
				int r = sh_fcntl(fno,F_dupfd_cloexec,10);
				if(r >= 10)
				{
					if(F_dupfd_cloexec != F_DUPFD)
						sh.fdstatus[r] = sh.fdstatus[fno]|IOCLEX;
					else
						sh.fdstatus[r] = sh.fdstatus[fno];
					sh_close(fno);
					fno = r;
				}
			}
			if(!(sh.fdstatus[fno]&IOCLEX))
				sh_fcntl(fno,F_SETFD,FD_CLOEXEC);
			iop = sh_iostream(fno);
		}
		else
			iop = sfstdin;
	}
	else
		fno = -1;
	sh.infd = fno;
	if(sh_isstate(SH_INTERACTIVE))
	{
		if(nv_isnull(PS1NOD))
			nv_putval(PS1NOD,(sh.euserid?e_stdprompt:e_supprompt),NV_RDONLY);
		sh_sigdone();
		if(sh_histinit())
			sh_onoption(SH_HISTORY);
	}
	else
	{
		if(!sh_isstate(SH_PROFILE))
		{
			buff.mode = SH_JMPEXIT;
			sh_onoption(SH_TRACKALL);
		}
		if(sh_isoption(SH_MONITOR))
			sh_onstate(SH_MONITOR);
		sh_offstate(SH_HISTORY);
		sh_offoption(SH_HISTORY);
	}
	states = sh_getstate();
	jmpval = sigsetjmp(buff.buff,0);
	if(jmpval)
	{
		Sfio_t *top;
		sh_iorestore(0,jmpval);
		hist_flush(sh.hist_ptr);
		sfsync(sh.outpool);
		sh.st.breakcnt = 0;
		/* check for return from profile or env file */
		if(sh_isstate(SH_PROFILE) && (jmpval==SH_JMPFUN || jmpval==SH_JMPEXIT))
		{
			sh_setstate(states);
			goto done;
		}
		if(!sh_isoption(SH_INTERACTIVE) || sh_isstate(SH_FORKED) || (jmpval > SH_JMPERREXIT && job_close() >=0))
		{
			sh_offstate(SH_INTERACTIVE);
			sh_offstate(SH_MONITOR);
			goto done;
		}
		exitset();
		/* skip over remaining input */
		if(top = fcfile())
		{
			while(fcget()>0);
			fcclose();
			while(top=sfstack(iop,SFIO_POPSTACK))
				sfclose(top);
		}
		/*
		 * Reset the lexer state and make sure the heredocs file is
		 * closed and set to NULL. For now we only do this when we get
		 * here in an interactive shell and we have a leftover heredoc.
		 */
		if(sh_isstate(SH_INTERACTIVE) && jmpval==SH_JMPERREXIT && sh.heredocs)
		{
			Lex_t *lp;
			sfclose(sh.heredocs);
			sh.heredocs = NULL;
			lp = (Lex_t*)sh.lex_context;
			lp->heredoc = NULL;
			sh_lexopen(lp,0);
		}
		/* make sure that we own the terminal */
		tcsetpgrp(job.fd,sh.pid);
	}
	/* error return here */
	sfclrerr(iop);
	sh_setstate(states);
	sh.st.optindex = 1;
	opt_info.offset = 0;
	sh.st.loopcnt = 0;
	sh.trapnote = 0;
	sh.intrap = 0;
	error_info.line = 1;
	sh.inlineno = 1;
	sh.binscript = 0;
	if(sfeof(iop))
		goto eof_or_error;
	/* command loop */
	while(1)
	{
		sh.nextprompt = 1;
		sh_freeup();
		stkset(sh.stk,NULL,0);
		sh_offstate(SH_STOPOK);
		sh_offstate(SH_ERREXIT);
		sh_offstate(SH_VERBOSE);
		sh_offstate(SH_TIMING);
		sh_offstate(SH_GRACE);
		sh_offstate(SH_TTYWAIT);
		if(sh_isoption(SH_VERBOSE))
			sh_onstate(SH_VERBOSE);
		sh_onstate(SH_ERREXIT);
		/* -eim flags don't apply to profiles */
		if(sh_isstate(SH_PROFILE))
		{
			sh_offstate(SH_INTERACTIVE);
			sh_offstate(SH_ERREXIT);
			sh_offstate(SH_MONITOR);
		}
		if(sh_isstate(SH_INTERACTIVE) && !tdone)
		{
			char *mail;
			sh_offstate(SH_MONITOR);
			if(sh_isoption(SH_MONITOR))
				sh_onstate(SH_MONITOR);
			if(job.pwlist)
			{
				job_walk(sfstderr,job_list,JOB_NFLAG,NULL);
				job_wait(0);
			}
			if((mail=nv_getval(MAILPNOD)) || (mail=nv_getval(MAILNOD)))
			{
				time(&curtime);
				if ((curtime - mailtime) >= sh_mailchk)
				{
					chkmail(mail);
					mailtime = curtime;
				}
			}
			if(sh.hist_ptr)
				hist_eof(sh.hist_ptr);
			/* sets timeout for command entry */
			sh.timeout = sh.st.tmout;
#if SHOPT_TIMEOUT
			if(sh.timeout <= 0 || sh.timeout > SHOPT_TIMEOUT)
				sh.timeout = SHOPT_TIMEOUT;
#endif /* SHOPT_TIMEOUT */
			sh.inlineno = 1;
			error_info.line = 1;
			sh.trapnote = 0;
			if(buff.mode == SH_JMPEXIT)
			{
				buff.mode = SH_JMPERREXIT;
#ifdef DEBUG
				errormsg(SH_DICT,ERROR_warn(0),"%jd: mode changed to SH_JMPERREXIT",(Sflong_t)sh.current_pid);
#endif
			}
		}
		errno = 0;
		if(tdone || !sfreserve(iop,0,0))
		{
			int	sferr;
		eof_or_error:
			sferr = sferror(iop);
			if(sh_isstate(SH_INTERACTIVE))
			{
				if(!sferr)
				{
					if(--maxtry>0 && sh_isoption(SH_IGNOREEOF)
					&& !sferror(sfstderr) && (sh.fdstatus[fno]&IOTTY))
					{
						sfclrerr(iop);
						errormsg(SH_DICT,0,e_logout);
						continue;
					}
					else if(job_close()<0)
						continue;
				}
			}
			else if(errno && sferr)
			{
				/* Error reading from running script; panic */
				errormsg(SH_DICT,ERROR_SYSTEM|ERROR_PANIC,e_readscript);
				UNREACHABLE();
			}
			if(errno==0 && sferr && --maxtry>0)
			{
				sfclrlock(iop);
				sfclrerr(iop);
				continue;
			}
			sh.exitval = sh.savexit;
			goto done;
		}
		sh.exitval = sh.savexit;
		maxtry = IOMAXTRY;
		if(sh_isstate(SH_INTERACTIVE) && sh.hist_ptr)
		{
			job_wait(0);
			hist_eof(sh.hist_ptr);
			sfsync(sfstderr);
		}
		if(sh_isoption(SH_HISTORY))
			sh_onstate(SH_HISTORY);
		job.waitall = job.curpgid = 0;
		error_info.flags |= ERROR_INTERACTIVE;
		t = (Shnode_t*)sh_parse(iop,0);
		if(!sh_isstate(SH_INTERACTIVE) && !sh_isoption(SH_CFLAG))
			error_info.flags &= ~ERROR_INTERACTIVE;
		sh.readscript = 0;
		if(sh_isstate(SH_INTERACTIVE) && sh.hist_ptr)
			hist_flush(sh.hist_ptr);
		sh_offstate(SH_HISTORY);
		if(t)
		{
			execflags = sh_state(SH_ERREXIT)|sh_state(SH_INTERACTIVE);
			/* The last command may not have to fork */
			if(!sh_isstate(SH_PROFILE) && !sh_isstate(SH_INTERACTIVE) &&
				(fno<0 || !(sh.fdstatus[fno]&(IOTTY|IONOSEEK)))
				&& !sfreserve(iop,0,0))
			{
					execflags |= sh_state(SH_NOFORK);
			}
			sh.dont_optimize_builtins = 0;
			sh.st.breakcnt = 0;
			sh_exec(t,execflags);
			if(sh.forked)
			{
				sh_offstate(SH_INTERACTIVE);
				goto done;
			}
			/* This is for sh -t */
			if(sh_isoption(SH_TFLAG) && !sh_isstate(SH_PROFILE))
				tdone++;
		}
	}
done:
	sh_popcontext(&buff);
	if(sh_isstate(SH_INTERACTIVE))
	{
		if(isatty(0) && !sh_isoption(SH_CFLAG))
			sfputc(sfstderr,'\n');
		job_close();
	}
	if(jmpval == SH_JMPSCRIPT)
		siglongjmp(*sh.jmplist,jmpval);
	else if(jmpval == SH_JMPEXIT)
		sh_done(0);
	if(fno>0)
		sh_close(fno);
	if(sh.st.filename)
		free(sh.st.filename);
	sh.st.filename = 0;
}


/* prints out messages if files in list have been modified since last call */
static void chkmail(char *files)
{
	char		*cp,*sp,*qp;
	char		save;
	struct argnod	*arglist=0;
	int		offset = stktell(sh.stk);
	char	 	*savstak = stkptr(sh.stk,0);
	struct stat	statb;
	if(*(cp=files) == 0)
		return;
	sp = cp;
	do
	{
		/* skip to : or end of string saving first '?' */
		for(qp=0;*sp && *sp != ':';sp++)
			if((*sp == '?' || *sp=='%') && qp == 0)
				qp = sp;
		save = *sp;
		*sp = 0;
		/* change '?' to end-of-string */
		if(qp)
			*qp = 0;
		do
		{
			/* see if time has been modified since last checked
			 * and the access time <= the modification time
			 */
			if(stat(cp,&statb) >= 0 && statb.st_mtime >= mailtime
				&& statb.st_atime <= statb.st_mtime)
			{
				/* check for directory */
				if(!arglist && S_ISDIR(statb.st_mode))
				{
					/* generate list of directory entries */
					path_complete(cp,"/*",&arglist);
				}
				else
				{
					/*
					 * If the file has shrunk,
					 * or if the size is zero
					 * then don't print anything
					 */
					if(statb.st_size &&
						(  statb.st_ino != lastmail.st_ino
						|| statb.st_dev != lastmail.st_dev
						|| statb.st_size > lastmail.st_size))
					{
						/* save and restore $_ */
						char *save = sh.lastarg;
						sh.lastarg = cp;
						errormsg(SH_DICT,0,sh_mactry(qp?qp+1:(char*)e_mailmsg));
						sh.lastarg = save;
					}
					lastmail = statb;
					break;
				}
			}
			if(arglist)
			{
				cp = arglist->argval;
				arglist = arglist->argchn.ap;
			}
			else
				cp = 0;
		}
		while(cp);
		if(qp)
			*qp = '?';
		*sp++ = save;
		cp = sp;
	}
	while(save);
	stkset(sh.stk,savstak,offset);
}

#undef PSTAT
#if _lib_pstat && _sys_pstat
#   include	<sys/pstat.h>
#   define PSTAT	1
#endif

#if !defined(fixargs_disabled)
/*
 * fix up command line for ps command
 *
 * This function is invoked when ksh needs to run a script without a
 * #!/hashbang/path. Instead of letting the kernel invoke a shell, ksh
 * exfile()s the script itself from sh_main(). In the forked child, it calls
 * fixargs() to set the argument list in the environment to the args of the
 * new script, so that 'ps' and /proc/PID/cmdline show the expected output.
 *
 * mode is 0 for initialization
 */
static void fixargs(char **argv, int mode)
{
#   if PSTAT
	char *cp;
	int offset=0,size;
	static int command_len;
	char *buff;
	union pstun un;
	if(mode==0)
	{
		struct pst_static st;
		un.pst_static = &st;
		if(pstat(PSTAT_STATIC, un, sizeof(struct pst_static), 1, 0)<0)
			return;
		command_len = st.command_length;
		return;
	}
	stkseek(sh.stk,command_len+2);
	buff = stkseek(sh.stk,0);
	if(command_len==0)
		return;
	while((cp = *argv++) && offset < command_len)
	{
		if(offset + (size=strlen(cp)) >= command_len)
			size = command_len - offset;
		memcpy(buff+offset,cp,size);
		offset += size;
		buff[offset++] = ' ';
	}
	offset--;
	memset(&buff[offset], 0, command_len - offset + 1);
	un.pst_command = stkptr(sh.stk,0);
	pstat(PSTAT_SETCMD,un,0,0,0);
#   elif _lib_setproctitle
#	define CMDMAXLEN 255
	char *cp;
	int offset=0,size;
	char buff[CMDMAXLEN + 1];
	if(mode==0)
		return;
	while((cp = *argv++) && offset < CMDMAXLEN)
	{
		if(offset + (size=strlen(cp)) >= CMDMAXLEN)
			size = CMDMAXLEN - offset;
		memcpy(buff+offset,cp,size);
		offset += size;
		buff[offset++] = ' ';
	}
	buff[--offset] = '\0';
	setproctitle("%s",buff);
#	undef CMDMAXLEN
#   else
	/* Generic version, works on at least Linux and macOS */
	char *cp;
	int offset=0,size;
	static int buffsize;
	static char *buff;
	if(mode==0)
	{
		int i;
		buff = argv[0];
		for(i=0; argv[i]; i++)
			buffsize += strlen(argv[i]) + 1;
		if(buffsize < 128 && buff + buffsize == *environ)
		{
			/* Move the environment to make space for a larger command line buffer */
			for(i=0; environ[i]; i++)
			{
				buffsize += strlen(environ[i]) + 1;
				environ[i] = sh_strdup(environ[i]);
			}
		}
		return;
	}
	while((cp = *argv++) && offset < buffsize)
	{
		if(offset + (size=strlen(cp)) >= buffsize)
			size = buffsize - offset;
		memcpy(buff+offset,cp,size);
		offset += size;
		buff[offset++] = ' ';
	}
	offset--;
	memset(&buff[offset], 0, buffsize - offset + 1);
#   endif
}
#endif /* !fixargs_disabled */

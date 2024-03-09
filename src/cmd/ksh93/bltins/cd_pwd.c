/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2024 Contributors to ksh 93u+m           *
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
 * cd [-L] [-Pe] [dirname]
 * cd [-L] [-Pe] [old] [new]
 * pwd [-LP]
 *
 *   David Korn
 *   AT&T Labs
 *   research!dgk
 *
 */

#include	"shopt.h"
#include	"defs.h"
#include	<error.h>
#include	"variables.h"
#include	"path.h"
#include	"name.h"
#include	"builtins.h"
#include	<ls.h>
#include	"test.h"

/*
 * Invalidate path name bindings to relative paths
 */
static void rehash(Namval_t *np,void *data)
{
	Pathcomp_t *pp = (Pathcomp_t*)np->nvalue.cp;
	if(pp && *pp->name!='/')
		nv_rehash(np,data);
}

int	b_cd(int argc, char *argv[],Shbltin_t *context)
{
	char *dir;
	Pathcomp_t *cdpath = 0;
	const char *dp;
	int saverrno=0;
	int rval,pflag=0,eflag=0,ret=1;
	char *oldpwd;
	Namval_t *opwdnod, *pwdnod;
	NOT_USED(context);
	while((rval = optget(argv,sh_optcd))) switch(rval)
	{
		case 'e':
			eflag = 1;
			break;
		case 'L':
			pflag = 0;
			break;
		case 'P':
			pflag = 1;
			break;
		case ':':
			if(sh_isoption(SH_RESTRICTED))
				break;
			errormsg(SH_DICT,2, "%s", opt_info.arg);
			break;
		case '?':
			if(sh_isoption(SH_RESTRICTED))
				break;
			errormsg(SH_DICT,ERROR_usage(2), "%s", opt_info.arg);
			UNREACHABLE();
	}
	if(pflag && eflag)
		ret = 2;  /* exit status is 2 if -eP are both on and chdir failed */
	if(sh_isoption(SH_RESTRICTED))
	{
		/* restricted shells cannot change the directory */
		errormsg(SH_DICT,ERROR_exit(ret),e_restricted+4);
		UNREACHABLE();
	}
	argv += opt_info.index;
	argc -= opt_info.index;
	dir =  argv[0];
	if(error_info.errors>0 || argc>2)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NULL));
		UNREACHABLE();
	}
	oldpwd = path_pwd();
	opwdnod = sh_scoped(OLDPWDNOD);
	pwdnod = sh_scoped(PWDNOD);
	if(sh.subshell)
	{
		/* clone $OLDPWD and $PWD into the subshell's scope */
		sh_assignok(opwdnod,1);
		sh_assignok(pwdnod,1);
	}
	if(argc==2)
		dir = sh_substitute(oldpwd,dir,argv[1]);
	else if(!dir)
		dir = nv_getval(sh_scoped(HOME));
	else if(*dir == '-' && dir[1]==0)
		dir = nv_getval(opwdnod);
	if(!dir || *dir==0)
	{
		errormsg(SH_DICT,ERROR_exit(ret),argc==2?e_subst+4:e_direct);
		UNREACHABLE();
	}
	/*
	 * If sh_subshell() in subshell.c cannot use fchdir(2) to restore the PWD using a saved file descriptor,
	 * we must fork any virtual subshell now to avoid the possibility of ending up in the wrong PWD on exit.
	 */
	if(sh.subshell && !sh.subshare)
	{
#if _lib_fchdir
		if(!test_inode(sh.pwd,e_dot))
#endif
			sh_subfork();
	}
	/*
	 * Do $CDPATH processing, except if the path is absolute or the first component is '.' or '..'
	 */
	if(dir[0] != '/'
#if _WINIX
	&& dir[1] != ':'  /* on Windows, an initial drive letter plus ':' denotes an absolute path */
#endif /* _WINIX */
	&& !(dir[0]=='.' && (dir[1]=='/' || dir[1]==0))
	&& !(dir[0]=='.' && dir[1]=='.' && (dir[2]=='/' || dir[2]==0)))
	{
		if((dp=sh_scoped(CDPNOD)->nvalue.cp) && !(cdpath = (Pathcomp_t*)sh.cdpathlist))
		{
			if(cdpath=path_addpath(NULL,dp,PATH_CDPATH))
				sh.cdpathlist = cdpath;
		}
	}
	if(*dir!='/')
	{
		/* check for leading .. */
		char *cp;
		sfprintf(sh.strbuf,"%s",dir);
		cp = sfstruse(sh.strbuf);
		pathcanon(cp, 0);
		if(cp[0]=='.' && cp[1]=='.' && (cp[2]=='/' || cp[2]==0))
		{
			if(!sh.strbuf2)
				sh.strbuf2 = sfstropen();
			sfprintf(sh.strbuf2,"%s/%s",oldpwd,cp);
			dir = sfstruse(sh.strbuf2);
			pathcanon(dir, 0);
		}
	}
	rval = -1;
	do
	{
		dp = cdpath?cdpath->name:"";
		cdpath = path_nextcomp(cdpath,dir,0);
#if _WINIX
		if(*stkptr(sh.stk,PATH_OFFSET+1)==':' && isalpha(*stkptr(sh.stk,PATH_OFFSET)))
		{
			*stkptr(sh.stk,PATH_OFFSET+1) = *stkptr(sh.stk,PATH_OFFSET);
			*stkptr(sh.stk,PATH_OFFSET)='/';
		}
#endif /* _WINIX */
		if(*stkptr(sh.stk,PATH_OFFSET)!='/')
		{
			char *last = stkfreeze(sh.stk,1);
			stkseek(sh.stk,PATH_OFFSET);
			sfputr(sh.stk,oldpwd,-1);
			/* don't add '/' if oldpwd is / itself */
			if(*oldpwd!='/' || oldpwd[1])
				sfputc(sh.stk,'/');
			sfputr(sh.stk,last+PATH_OFFSET,0);
		}
		if(!pflag)
		{
			char *cp;
			stkseek(sh.stk,PATH_MAX+PATH_OFFSET);
			if(*(cp=stkptr(sh.stk,PATH_OFFSET))=='/')
				if(!pathcanon(cp,PATH_DOTDOT))
					continue;
		}
		if((rval=chdir(path_relative(stkptr(sh.stk,PATH_OFFSET)))) >= 0)
			goto success;
		if(errno!=ENOENT && saverrno==0)
			saverrno=errno;
	}
	while(cdpath);
	if(rval<0 && *dir=='/' && *(path_relative(stkptr(sh.stk,PATH_OFFSET)))!='/')
		rval = chdir(dir);
	/* use absolute chdir() if relative chdir() fails */
	if(rval<0)
	{
		if(saverrno)
			errno = saverrno;
		errormsg(SH_DICT,ERROR_system(ret),"%s:",dir);
		UNREACHABLE();
	}
success:
	if(dir == nv_getval(opwdnod) || argc==2)
		dp = dir;	/* print out directory for cd - */
	if(pflag)
	{
		dir = stkptr(sh.stk,PATH_OFFSET);
		if (!(dir=pathcanon(dir,PATH_PHYSICAL)))
		{
			dir = stkptr(sh.stk,PATH_OFFSET);
			errormsg(SH_DICT,ERROR_system(ret),"%s:",dir);
			UNREACHABLE();
		}
		stkseek(sh.stk,dir-stkptr(sh.stk,0));
	}
	dir = stkfreeze(sh.stk,1)+PATH_OFFSET;
	if(*dp && (*dp!='.'||dp[1]) && strchr(dir,'/'))
		sfputr(sfstdout,dir,'\n');
	nv_putval(opwdnod,oldpwd,NV_RDONLY);
	free((void*)sh.pwd);
	if(*dir == '/')
	{
		size_t len = strlen(dir);
		/* delete trailing '/' */
		while(--len>0 && dir[len]=='/')
			dir[len] = 0;
		nv_putval(pwdnod,dir,NV_RDONLY);
		nv_onattr(pwdnod,NV_EXPORT);
		sh.pwd = sh_strdup(dir);
	}
	else
	{
		/* pathcanon() failed to canonicalize the directory, which happens when 'cd' is invoked from a
		   nonexistent PWD with a relative path as the argument. Reinitialize $PWD as it will be wrong. */
		sh.pwd = NULL;
		path_pwd();
		if(*sh.pwd != '/')
		{
			errormsg(SH_DICT,ERROR_system(ret),e_direct);
			UNREACHABLE();
		}
	}
	nv_scan(sh_subtracktree(1),rehash,NULL,NV_TAGGED,NV_TAGGED);
	path_newdir(sh.pathlist);
	path_newdir(sh.cdpathlist);
	if(pflag && eflag)
	{
		/* Verify the current working directory matches $PWD */
		return !test_inode(e_dot,nv_getval(pwdnod));
	}
	return 0;
}

int	b_pwd(int argc, char *argv[],Shbltin_t *context)
{
	int n, flag = 0;
	char *cp;
	NOT_USED(argc);
	NOT_USED(context);
	while((n = optget(argv,sh_optpwd))) switch(n)
	{
		case 'L':
			flag = 0;
			break;
		case 'P':
			flag = 1;
			break;
		case ':':
			errormsg(SH_DICT,2, "%s", opt_info.arg);
			break;
		case '?':
			errormsg(SH_DICT,ERROR_usage(2), "%s", opt_info.arg);
			UNREACHABLE();
	}
	if(error_info.errors)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NULL));
		UNREACHABLE();
	}
	if(*(cp = path_pwd()) != '/' || !test_inode(cp,e_dot))
	{
		errormsg(SH_DICT,ERROR_system(1), e_pwd);
		UNREACHABLE();
	}
	if(flag)
	{
		cp = strcpy(stkseek(sh.stk,strlen(cp)+PATH_MAX),cp);
		pathcanon(cp,PATH_PHYSICAL);
	}
	sfputr(sfstdout,cp,'\n');
	return 0;
}

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
#pragma prototyped
/*
 * cd [-LP]  [dirname]
 * cd [-LP]  [old] [new]
 * pwd [-LP]
 *
 *   David Korn
 *   AT&T Labs
 *   research!dgk
 *
 */

#include	"defs.h"
#include	<stak.h>
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
static void rehash(register Namval_t *np,void *data)
{
	Pathcomp_t *pp = (Pathcomp_t*)np->nvalue.cp;
	if(pp && *pp->name!='/')
		nv_rehash(np,data);
}

int	b_cd(int argc, char *argv[],Shbltin_t *context)
{
	register char *dir;
	Pathcomp_t *cdpath = 0;
	register const char *dp;
	register Shell_t *shp = context->shp;
	int saverrno=0;
	int rval,flag=0;
	char *oldpwd;
	Namval_t *opwdnod, *pwdnod;
	if(sh_isoption(SH_RESTRICTED))
	{
		errormsg(SH_DICT,ERROR_exit(1),e_restricted+4);
		UNREACHABLE();
	}
	while((rval = optget(argv,sh_optcd))) switch(rval)
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
	argv += opt_info.index;
	argc -= opt_info.index;
	dir =  argv[0];
	if(error_info.errors>0 || argc >2)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage((char*)0));
		UNREACHABLE();
	}
	oldpwd = path_pwd(shp,0);
	opwdnod = sh_scoped(shp,OLDPWDNOD);
	pwdnod = sh_scoped(shp,PWDNOD);
	if(oldpwd == e_dot && pwdnod->nvalue.cp)
		oldpwd = (char*)pwdnod->nvalue.cp;  /* if path_pwd() failed to get the pwd, use $PWD */
	if(shp->subshell)
	{
		opwdnod = sh_assignok(opwdnod,1);
		pwdnod = sh_assignok(pwdnod,1);
	}
	if(argc==2)
		dir = sh_substitute(oldpwd,dir,argv[1]);
	else if(!dir)
		dir = nv_getval(HOME);
	else if(*dir == '-' && dir[1]==0)
		dir = nv_getval(opwdnod);
	if(!dir || *dir==0)
	{
		errormsg(SH_DICT,ERROR_exit(1),argc==2?e_subst+4:e_direct);
		UNREACHABLE();
	}
	/*
	 * If sh_subshell() in subshell.c cannot use fchdir(2) to restore the PWD using a saved file descriptor,
	 * we must fork any virtual subshell now to avoid the possibility of ending up in the wrong PWD on exit.
	 */
	if(shp->subshell && !shp->subshare)
	{
#if _lib_fchdir
		if(!test_inode(nv_getval(pwdnod),e_dot))
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
		if(!(cdpath = (Pathcomp_t*)shp->cdpathlist) && (dp=sh_scoped(shp,CDPNOD)->nvalue.cp))
		{
			if(cdpath=path_addpath(shp,(Pathcomp_t*)0,dp,PATH_CDPATH))
			{
				shp->cdpathlist = (void*)cdpath;
				cdpath->shp = shp;
			}
		}
	}
	if(*dir!='/')
	{
		/* check for leading .. */
		char *cp;
		sfprintf(shp->strbuf,"%s",dir);
		cp = sfstruse(shp->strbuf);
		pathcanon(cp, 0);
		if(cp[0]=='.' && cp[1]=='.' && (cp[2]=='/' || cp[2]==0))
		{
			if(!shp->strbuf2)
				shp->strbuf2 = sfstropen();
			sfprintf(shp->strbuf2,"%s/%s",oldpwd,cp);
			dir = sfstruse(shp->strbuf2);
			pathcanon(dir, 0);
		}
	}
	rval = -1;
	do
	{
		dp = cdpath?cdpath->name:"";
		cdpath = path_nextcomp(shp,cdpath,dir,0);
#if _WINIX
                if(*stakptr(PATH_OFFSET+1)==':' && isalpha(*stakptr(PATH_OFFSET)))
		{
			*stakptr(PATH_OFFSET+1) = *stakptr(PATH_OFFSET);
			*stakptr(PATH_OFFSET)='/';
		}
#endif /* _WINIX */
                if(*stakptr(PATH_OFFSET)!='/')

		{
			char *last=(char*)stakfreeze(1);
			stakseek(PATH_OFFSET);
			stakputs(oldpwd);
			/* don't add '/' of oldpwd is / itself */
			if(*oldpwd!='/' || oldpwd[1])
				stakputc('/');
			stakputs(last+PATH_OFFSET);
			stakputc(0);
		}
		if(!flag)
		{
			register char *cp;
			stakseek(PATH_MAX+PATH_OFFSET);
			if(*(cp=stakptr(PATH_OFFSET))=='/')
				if(!pathcanon(cp,PATH_DOTDOT))
					continue;
		}
		if((rval=chdir(path_relative(shp,stakptr(PATH_OFFSET)))) >= 0)
			goto success;
		if(errno!=ENOENT && saverrno==0)
			saverrno=errno;
	}
	while(cdpath);
	if(rval<0 && *dir=='/' && *(path_relative(shp,stakptr(PATH_OFFSET)))!='/')
		rval = chdir(dir);
	/* use absolute chdir() if relative chdir() fails */
	if(rval<0)
	{
		if(saverrno)
			errno = saverrno;
		errormsg(SH_DICT,ERROR_system(1),"%s:",dir);
		UNREACHABLE();
	}
success:
	if(dir == nv_getval(opwdnod) || argc==2)
		dp = dir;	/* print out directory for cd - */
	if(flag)
	{
		dir = stakptr(PATH_OFFSET);
		if (!(dir=pathcanon(dir,PATH_PHYSICAL)))
		{
			dir = stakptr(PATH_OFFSET);
			errormsg(SH_DICT,ERROR_system(1),"%s:",dir);
			UNREACHABLE();
		}
		stakseek(dir-stakptr(0));
	}
	dir = (char*)stakfreeze(1)+PATH_OFFSET;
	if(*dp && (*dp!='.'||dp[1]) && strchr(dir,'/'))
		sfputr(sfstdout,dir,'\n');
	nv_putval(opwdnod,oldpwd,NV_RDONLY);
	if(*dir == '/')
	{
		flag = strlen(dir);
		/* delete trailing '/' */
		while(--flag>0 && dir[flag]=='/')
			dir[flag] = 0;
		nv_putval(pwdnod,dir,NV_RDONLY);
		nv_onattr(pwdnod,NV_EXPORT);
		if(shp->pwd)
			free((void*)shp->pwd);
		shp->pwd = sh_strdup(pwdnod->nvalue.cp);
	}
	else
	{
		/* pathcanon() failed to canonicalize the directory, which happens when 'cd' is invoked from a
		   nonexistent PWD with a relative path as the argument. Reinitialize $PWD as it will be wrong. */
		if(shp->pwd)
			free((void*)shp->pwd);
		shp->pwd = NIL(const char*);
		path_pwd(shp,0);
		if(*shp->pwd != '/')
		{
			errormsg(SH_DICT,ERROR_system(1),e_direct);
			UNREACHABLE();
		}
	}
	nv_scan(sh_subtracktree(1),rehash,(void*)0,NV_TAGGED,NV_TAGGED);
	path_newdir(shp,shp->pathlist);
	path_newdir(shp,shp->cdpathlist);
	return(0);
}

int	b_pwd(int argc, char *argv[],Shbltin_t *context)
{
	register int n, flag = 0;
	register char *cp;
	register Shell_t *shp = context->shp;
	NOT_USED(argc);
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
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage((char*)0));
		UNREACHABLE();
	}
	if(*(cp = path_pwd(shp,0)) != '/')
	{
		errormsg(SH_DICT,ERROR_system(1), e_pwd);
		UNREACHABLE();
	}
	if(flag)
	{
		cp = strcpy(stakseek(strlen(cp)+PATH_MAX),cp);
		pathcanon(cp,PATH_PHYSICAL);
	}
	sfputr(sfstdout,cp,'\n');
	return(0);
}

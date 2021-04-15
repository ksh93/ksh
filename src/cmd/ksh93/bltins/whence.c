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
 * command [-pvVx] name [arg...]
 * whence [-afpPqtv] name...
 *
 *   David Korn
 *   AT&T Labs
 *
 */

#include	"defs.h"
#include	<error.h>
#include	"shtable.h"
#include	"name.h"
#include	"path.h"
#include	"shlex.h"
#include	"builtins.h"

#define P_FLAG	(1 << 0)
#define V_FLAG	(1 << 1)
#define A_FLAG	(1 << 2)
#define F_FLAG	(1 << 3)
#define X_FLAG	(1 << 4)
#define Q_FLAG	(1 << 5)
#define T_FLAG	(1 << 6)

static int whence(Shell_t *,char**, int);

/*
 * command is called with argc==0 when checking for -V or -v option
 * In this case return 0 when -v or -V or unknown option, otherwise
 *   the shift count to the command is returned
 */
int	b_command(register int argc,char *argv[],Shbltin_t *context)
{
	register int n, flags=0;
	register Shell_t *shp = context->shp;
	opt_info.index = opt_info.offset = 0;
	while((n = optget(argv,sh_optcommand))) switch(n)
	{
	    case 'p':
		if(sh_isoption(SH_RESTRICTED))
		{
			 errormsg(SH_DICT,ERROR_exit(1),e_restricted,"-p");
			 UNREACHABLE();
		}
		sh_onstate(SH_DEFPATH);
		break;
	    case 'v':
		flags |= X_FLAG;
		break;
	    case 'V':
		flags |= V_FLAG;
		break;
	    case 'x':
		flags |= P_FLAG;
		break;
	    case ':':
		if(argc==0)
			return(0);
		errormsg(SH_DICT,2, "%s", opt_info.arg);
		break;
	    case '?':
		if(argc==0)
			return(0);
		errormsg(SH_DICT,ERROR_usage(2), "%s", opt_info.arg);
		UNREACHABLE();
	}
	argv += opt_info.index;
	if(argc==0)
	{
		if((flags & (X_FLAG|V_FLAG)) || !*argv)
			return(0);	/* return no offset now; sh_exec() will treat command -v/-V/(null) as normal builtin */
		if(flags & P_FLAG)
			sh_onstate(SH_XARG);
		return(opt_info.index); /* offset for sh_exec() to remove 'command' prefix + options */
	}
	if(error_info.errors)
	{
		errormsg(SH_DICT,ERROR_usage(2),"%s", optusage((char*)0));
		UNREACHABLE();
	}
	if(!*argv)
		return((flags & (X_FLAG|V_FLAG)) != 0 ? 2 : 0);
	return(whence(shp,argv, flags));
}

/*
 * for the whence and type commands
 */
int	b_whence(int argc,char *argv[],Shbltin_t *context)
{
	register int flags=0, n;
	register Shell_t *shp = context->shp;
	NOT_USED(argc);
	if(*argv[0]=='t')
		flags = V_FLAG;  /* <t>ype == whence -v */
	while((n = optget(argv,sh_optwhence))) switch(n)
	{
	    case 'a':
		flags |= A_FLAG;
		/* FALLTHROUGH */
	    case 'v':
		flags |= V_FLAG;
		break;
	    case 't':
		flags |= T_FLAG;
		break;
	    case 'f':
		flags |= F_FLAG;
		break;
	    case 'P':
	    case 'p':
		flags |= P_FLAG;
		flags &= ~V_FLAG;
		break;
	    case 'q':
		flags |= Q_FLAG;
		break;
	    case ':':
		errormsg(SH_DICT,2, "%s", opt_info.arg);
		break;
	    case '?':
		errormsg(SH_DICT,ERROR_usage(2), "%s", opt_info.arg);
		UNREACHABLE();
	}
	if(flags&T_FLAG)
		flags &= ~V_FLAG;
	argv += opt_info.index;
	if(error_info.errors || !*argv)
	{
		errormsg(SH_DICT,ERROR_usage(2),optusage((char*)0));
		UNREACHABLE();
	}
	return(whence(shp, argv, flags));
}

static int whence(Shell_t *shp,char **argv, register int flags)
{
	register const char *name;
	register Namval_t *np;
	register const char *cp;
	register int aflag, ret = 0;
	register const char *msg;
	Namval_t *nq;
	char *notused;
	Pathcomp_t *pp;
	if(flags&Q_FLAG)
		flags &= ~A_FLAG;
	while(name= *argv++)
	{
		aflag = ((flags&A_FLAG)!=0);
		cp = 0;
		np = 0;
		if(flags&P_FLAG)
			goto search;
		if(flags&Q_FLAG)
			goto bltins;
		/* reserved words first */
		if(sh_lookup(name,shtab_reserved))
		{
			if(flags&T_FLAG)
				sfprintf(sfstdout,"keyword\n");
			else
				sfprintf(sfstdout,"%s%s\n",name,(flags&V_FLAG)?sh_translate(is_reserved):"");
			if(!aflag)
				continue;
			aflag++;
		}
		/* non-tracked aliases */
		if((np=nv_search(name,shp->alias_tree,0))
			&& !nv_isnull(np) && !nv_isattr(np,NV_TAGGED)
			&& (cp=nv_getval(np))) 
		{
			if(flags&V_FLAG)
			{
				msg = sh_translate(is_alias);
				sfprintf(sfstdout,msg,name);
			}
			if(flags&T_FLAG)
				sfputr(sfstdout,"alias",'\n');
			else
				sfputr(sfstdout,sh_fmtq(cp),'\n');
			if(!aflag)
				continue;
			cp = 0;
			aflag++;
		}
	bltins:
		/* functions */
		if(!(flags&F_FLAG) && (np = nv_bfsearch(name, shp->fun_tree, &nq, &notused)) && is_afunction(np))
		{
			if(flags&Q_FLAG)
				continue;
			if(!(flags&T_FLAG))
				sfputr(sfstdout,name,-1);
			if(flags&V_FLAG)
			{
				if(nv_isnull(np))
				{
					sfprintf(sfstdout,sh_translate(is_ufunction));
					pp = 0;
					while(!path_search(shp,name,&pp,3) && pp && (pp = pp->next))
						;
					if(*stakptr(PATH_OFFSET)=='/')
						sfprintf(sfstdout,sh_translate(e_autoloadfrom),sh_fmtq(stakptr(PATH_OFFSET)));
				}
				else
					sfprintf(sfstdout,sh_translate(is_function));
			}
			else if(flags&T_FLAG)
				sfprintf(sfstdout,"function");
			sfputc(sfstdout,'\n');
			if(!aflag)
				continue;
			aflag++;
		}
		/* built-ins */
		if((np = nv_bfsearch(name, shp->bltin_tree, &nq, &notused)) && !nv_isnull(np))
		{
			if(flags&V_FLAG)
				if(nv_isattr(np,BLT_SPC))
					cp = sh_translate(is_spcbuiltin);
				else
					cp = sh_translate(is_builtin);
			else
				cp = "";
			if(flags&Q_FLAG)
				continue;
			if(flags&T_FLAG)
				sfprintf(sfstdout,"builtin\n");
			else
				sfprintf(sfstdout,"%s%s\n",name,cp);
			if(!aflag)
				continue;
			aflag++;
		}
	search:
		pp = 0;
		do
		{
			int maybe_undef_fn = 0;  /* flag for possible undefined (i.e. autoloadable) function */
			/*
			 * See comments in sh/path.c for info on what path_search()'s true/false return values mean
			 */
			if(path_search(shp, name, &pp, aflag>1 ? 3 : 2))
			{
				cp = name;
				if(*cp!='/')
				{
					if(flags&(P_FLAG|F_FLAG)) /* Ignore functions when passed -f or -p */
						cp = 0;
					else
						maybe_undef_fn = 1;
				}
			}
			else
			{
				cp = stakptr(PATH_OFFSET);
				if(*cp==0)
					cp = 0;
			}
			if(flags&Q_FLAG)
			{
				/* Since -q ignores -a, return on the first non-match */
				if(!cp)
					return(1);
			}
			else if(maybe_undef_fn)
			{
				/* Skip defined function or builtin (already done above) */
				if(!nv_search(cp,shp->fun_tree,0))
				{
					/* Undefined/autoloadable function on FPATH */
					sfputr(sfstdout,sh_fmtq(cp),-1);
					if(flags&T_FLAG)
						sfprintf(sfstdout,"function");
					else if(flags&V_FLAG)
					{
						sfprintf(sfstdout,sh_translate(is_ufunction));
						sfprintf(sfstdout,sh_translate(e_autoloadfrom),sh_fmtq(stakptr(PATH_OFFSET)));
					}
					sfputc(sfstdout,'\n');
				}
			}
			else if(cp)
			{
				cp = path_fullname(shp,cp);  /* resolve '.' & '..' */
				int is_pathbound_builtin = 0;
				if(flags&(V_FLAG|T_FLAG))
				{
					if(!(flags&T_FLAG))
						sfputr(sfstdout,sh_fmtq(name),' ');
					/* built-in version of program */
					if(nv_search(cp,shp->bltin_tree,0))
					{
						if(flags&T_FLAG)
							is_pathbound_builtin = 1;
						else
							msg = sh_translate(is_builtver);
					}
					/* tracked aliases next */
					else if(!sh_isstate(SH_DEFPATH)
					&& (np = nv_search(name,shp->track_tree,0))
					&& !nv_isattr(np,NV_NOALIAS)
					&& strcmp(cp,nv_getval(np))==0)
						msg = sh_translate(is_talias);
					else
						msg = sh_translate("is");
					if(!(flags&T_FLAG))
						sfputr(sfstdout,msg,' ');
				}
				if(flags&T_FLAG)
					sfputr(sfstdout,is_pathbound_builtin ? "builtin" : "file",'\n');
				else
					sfputr(sfstdout,sh_fmtq(cp),'\n');
				free((char*)cp);
			}
			else if(aflag<=1) 
			{
				ret = 1;
				if(flags&V_FLAG)
					 errormsg(SH_DICT,ERROR_exit(0),e_found,sh_fmtq(name));
			}
			/* If -a is active, continue to the next result */
			if(aflag)
			{
				if(aflag<=1)
					aflag++;
				if(pp)
					pp = pp->next;
			}
			else
				pp = 0;
		} while(pp);
	}
	return(ret);
}

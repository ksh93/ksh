/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1992-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2023 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  David Korn <dgk@research.att.com>                   *
*                  Martijn Dekker <martijn@inlv.org>                   *
*                                                                      *
***********************************************************************/
/*
 * Glenn Fowler
 * AT&T Research
 *
 * cp/ln/mv -- copy/link/move files
 */

#include <cmd.h>

int
b_mv(int argc, char** argv, Shbltin_t* context)
{
	/*
	 * bug-871: Phi: Catch xdev mv
	 * If any of the src is not on same dev as the dst one it is an
	 * xdev move and b_cp() in MV mode don't handle it correctly.
	 * So the gross hack here is a work around, on xdev move
	 * we do a cp -r src... dst (which seems to work)
	 * then on success we do a rm -r src...
	 * On degenerate case, i.e case where we can not figure out the dst dev
         * we go the xdev path (cp;rm) and let cp deal with errors.
	 */
	int		ac;
	char		**av;
	struct stat	sts, std;
	int		i;	  
	int		xdev=0;
       
	if(argc>2)
	{
		if(stat(argv[argc-1], &std))
		{ std.st_dev=0;			
		}
		for(i=1;i<argc-1;i++)
		{ 
			if(stat(argv[i], &sts))
				sts.st_dev=0;

			if(sts.st_dev!=std.st_dev)
			     xdev++;
		}
		if(xdev)
		{
			ac=argc+1;
			av=malloc( (ac+1)*sizeof(char*) );
			if(av) /* Let fallthrough report av==0 */
			{
				av[0]="XM"; /* Our sig for b_cp() */
				av[1]="-r";
				for(i=1;i<=argc;i++)
				{ av[1+i]=argv[i];
				}
				i=b_cp(ac,av,context);
				if(i)
				{
					error(1|ERROR_WARNING,
					      "src not removed due to errors");

					free(av);
					return(i);
				}
				av[0]="rm";
                                av[1]="-rfd";
                                ac--;
                                av[ac]=0;
				for(i=2;i<ac;i++)
				{ if(av[i][0]=='-')
                                  { av[i]="-r";
                                  }
				}
                                i=b_rm(ac,av,context);
				free(av);
				return(i);
			}
		}
	}
	
	return b_cp(argc, argv, context);
}

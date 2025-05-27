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
*                                                                      *
***********************************************************************/
/*
 * test expression
 * [ expression ]
 *
 *   David Korn
 *   AT&T Labs
 *
 */

#include	"shopt.h"
#include	"defs.h"
#include	<error.h>
#include	<ls.h>
#include	<regex.h>
#include	"io.h"
#include	"terminal.h"
#include	"test.h"
#include	"builtins.h"
#include	"FEATURE/externs"
#include	"FEATURE/poll"
#include	<tmx.h>

#if !_lib_setregid
#   undef _lib_setreuid
#endif /* _lib_setregid */

#ifdef S_ISSOCK
#   if _pipe_socketpair
#       if _socketpair_shutdown_mode
#           define isapipe(f,p) (test_stat(f,p)>=0&&(S_ISFIFO((p)->st_mode)||(S_ISSOCK((p)->st_mode)&&(p)->st_ino&&((p)->st_mode&(S_IRUSR|S_IWUSR))!=(S_IRUSR|S_IWUSR))))
#       else
#           define isapipe(f,p) (test_stat(f,p)>=0&&(S_ISFIFO((p)->st_mode)||S_ISSOCK((p)->st_mode)&&(p)->st_ino))
#       endif
#   else
#       define isapipe(f,p) (test_stat(f,p)>=0&&(S_ISFIFO((p)->st_mode)||S_ISSOCK((p)->st_mode)&&(p)->st_ino))
#   endif
#   define isasock(f,p) (test_stat(f,p)>=0&&S_ISSOCK((p)->st_mode))
#else
#   define isapipe(f,p) (test_stat(f,p)>=0&&S_ISFIFO((p)->st_mode))
#   define isasock(f,p) (0)
#endif

#define permission(a,f)		(sh_access(a,f)==0)
static int	test_time(const char*, const char*);
static int	test_stat(const char*, struct stat*);
static int	test_mode(const char*);

/* single char string compare */
#define c_eq(a,c)	(*a==c && *(a+1)==0)
/* two character string compare */
#define c2_eq(a,c1,c2)	(*a==c1 && *(a+1)==c2 && *(a+2)==0)

struct test
{
	int     ap;
	int     ac;
	char    **av;
};

static char *nxtarg(struct test*,int);
static int expr(struct test*,int);
static int e3(struct test*,int);

/* check for -a or -o in POSIX mode */
static inline int posix_andor(char *arg)
{
	return sh_isoption(SH_POSIX) && (sh_lookup(arg,shtab_testops) & TEST_ANDOR);
}

static int test_strmatch(const char *str, const char *pat)
{
	int match[2*(MATCH_MAX+1)],n;
	int c, m=0;
	const char *cp=pat;
	while(c = *cp++)
	{
		if(c=='(')
			m++;
		if(c=='\\' && *cp)
			cp++;
	}
	if(m)
		m++;
	else
		match[0] = 0;
	if(m >  elementsof(match)/2)
		m = elementsof(match)/2;
	n = strgrpmatch(str, pat, (ssize_t*)match, m, STR_GROUP|STR_MAXIMAL|STR_LEFT|STR_RIGHT|STR_INT);
	if(m==0 && n==1)
		match[1] = (int)strlen(str);
	if(n)
		sh_setmatch(str, -1, n, match, 0);
	return n;
}

int b_test(int argc, char *argv[],Shbltin_t *context)
{
	struct test tdata;
	char *cp = argv[0];
	int not;

	NOT_USED(context);
	tdata.av = argv;
	tdata.ap = 1;
	if(c_eq(cp,'['))
	{
		cp = argv[--argc];
		if(!c_eq(cp, ']'))
		{
			errormsg(SH_DICT,ERROR_exit(2),e_missing,"']'");
			UNREACHABLE();
		}
	}
	/* POSIX requires the test builtin to return 1 if expression is missing */
	if(argc <= 1)
		return 1;
	cp = argv[1];
	/* Compat kludge: if there are up to 5 args and the first and last are parentheses, remove the parentheses... */
	if(c_eq(cp,'(') && argc<=6 && c_eq(argv[argc-1],')'))
	{
		/* ...except if the middle arg is a binary operator, as in test '(' = ')', which must return false in POSIX */
		if(!(argc==4 && sh_lookup(argv[2],shtab_testops)))
		{
			cp =  (++argv)[1];
			++tdata.av;
			argc -= 2;
		}
	}
	not = c_eq(cp,'!');
	/* POSIX portion for test */
	switch(argc)
	{
		case 5:
			if(!not)
				break;
			if(posix_andor(argv[3]))
			{	/*
				 * In POSIX mode, enforce a violation of basic logic that sadly made it to every other shell:
				 * "test ! foo -o bar" must return 1, i.e., is same as "test ! \( -n foo -o -n bar \)"
				 * "test ! foo -a ''"  must return 0, i.e., is same as "test ! \( -n foo -a -n '' \)"
				 * This only applies if argc==5; for example, "test ! -n foo -o -n bar" correctly returns 0.
				 */
				tdata.av++;		/* skip the '!' */
				tdata.ac = argc - 1;
				return expr(&tdata,0);	/* invert the exit status result by NOT inverting the logical result */
			}
			argv++;
			/* FALLTHROUGH */
		case 4:
		{
			int op = sh_lookup(cp=argv[2],shtab_testops);
			if(op&TEST_ANDOR)
			{
				if(sh_isoption(SH_POSIX))
					return !(op==TEST_AND ? *argv[1] && *argv[3] : *argv[1] || *argv[3]);
				break;
			}
			if(!op)
			{
				if(argc==5)
					break;
				if(not && cp[0]=='-' && cp[2]==0)
					return test_unop(cp[1],argv[3]) != 0;
				else if(argv[1][0]=='-' && argv[1][2]==0)
					return !test_unop(argv[1][1],cp);
				else if(not && c_eq(argv[2],'!'))
					return *argv[3] == 0;
				errormsg(SH_DICT,ERROR_exit(2),e_badop,cp);
				UNREACHABLE();
			}
			return test_binop(op,argv[1],argv[3]) ^ (argc != 5);
		}
		case 3:
			if(not)
				return *argv[2] != 0;
			if(cp[0] != '-' || cp[2] || cp[1]=='?')
			{	/*
				 * The following ugly hack supports 'test --man --' and '[ --man -- ]' and related
				 * getopts documentation options (which all overload the error message mechanism).
				 * This is the only way to make the 'test' command self-documenting; supporting the
				 * getopts doc options without the extra '--' argument would break the test/[ syntax.
				 */
				if(cp[0]=='-' && (cp[1]=='-' || cp[1]=='?') &&
					strcmp(argv[2],"--")==0)
				{
					char *av[3];
					av[0] = argv[0];
					av[1] = argv[1];
					av[2] = 0;
					if (optget(av,sh_opttest) == '?')
					{
						/* self-doc: write to standard output */
						error(ERROR_USAGE|ERROR_OUTPUT, STDOUT_FILENO, "%s", opt_info.arg);
						return 0;
					}
					else
					{
						errormsg(SH_DICT, ERROR_exit(2), "%s", opt_info.arg);
						UNREACHABLE();
					}
				}
				break;
			}
			return !test_unop(cp[1],argv[2]);
		case 2:
			return *cp == 0;
	}
	tdata.ac = argc;
	return !expr(&tdata,0);
}

/*
 * evaluate a test expression.
 * flag is 0 on outer level
 * flag & FLAG_PARENS when in parentheses
 * flag & FLAG_AND when evaluating -a (TEST_AND)
 */
#define FLAG_PARENS	0x01
#define FLAG_AND	0x02
static int expr(struct test *tp,int flag)
{
	int r;
	char *p;
	r = e3(tp,flag&FLAG_PARENS);
	while(tp->ap < tp->ac)
	{
		p = nxtarg(tp,0);
		/* check for -o and -a */
		if(flag && c_eq(p,')'))
		{
			tp->ap--;
			break;
		}
		if(*p=='-' && *(p+2)==0)
		{
			if(*++p == 'o')
			{
				if(flag&FLAG_AND)
				{
					tp->ap--;
					break;
				}
				r |= expr(tp,flag&FLAG_PARENS);
				continue;
			}
			else if(*p == 'a')
			{
				r &= expr(tp,flag&FLAG_PARENS|FLAG_AND);
				continue;
			}
		}
		errormsg(SH_DICT,ERROR_exit(2),e_badsyntax);
		UNREACHABLE();
	}
	return r;
}

static char *nxtarg(struct test *tp,int mt)
{
	if(tp->ap >= tp->ac)
	{
		if(mt)
		{
			tp->ap++;
			return NULL;
		}
		errormsg(SH_DICT,ERROR_exit(2),e_argument);
		UNREACHABLE();
	}
	return tp->av[tp->ap++];
}


static int e3(struct test *tp,int inparens)
{
	char *arg, *cp;
	int op;
	char *binop;
	arg=nxtarg(tp,0);
	/*
	 * In POSIX mode, do not process ! or ( as operators if followed directly by -a or -o;
	 * this disables the nonstandard unary -a and -o operators in these cases. Examples:
	 * "test ! -a /dev/null" returns 0 (is equivalent to "test -n ! -a -n /dev/null")
	 * "test \( -o \)" is not an error and returns 0 (is equivalent to "test \( -n -o \)")
	 */
	if(c_eq(arg, '!') && tp->ap < tp->ac && !(inparens && tp->ap == tp->ac - 1) && !posix_andor(tp->av[tp->ap]))
		return !e3(tp,inparens);
	if(c_eq(arg, '(') && !posix_andor(tp->av[tp->ap]))
	{
		op = expr(tp,FLAG_PARENS);
		cp = nxtarg(tp,0);
		if(!cp || !c_eq(cp, ')'))
		{
			errormsg(SH_DICT,ERROR_exit(2),e_missing,"')'");
			UNREACHABLE();
		}
		return op;
	}
	cp = nxtarg(tp,1);
	if(cp!=0 && (c_eq(cp,'=') || c2_eq(cp,'!','=')))
		goto skip;
	if(!sh_isoption(SH_POSIX) && c2_eq(arg,'-','t'))
	{	/*
		 * Ancient compatibility hack supporting test -t with no arguments == test -t 1.
		 * This is only reached when doing a compound expression like: test 1 -eq 1 -a -t
		 * (for simple 'test -t' this is handled in the parser, see qscan() in sh/parse.c).
		 */
		if(cp)
		{
			op = strtol(cp,&binop, 10);
			return *binop ? 0 : tty_check(op);
		}
		else
		{
			tp->ap--;
			return tty_check(1);
		}
	}
	if(*arg=='-' && arg[2]==0)
	{
		op = arg[1];
		if(!cp)					/* no further argument: */
			return 1;			/* treat as nonempty string instead of unary op, so return true */
		if(strchr(test_opchars,op))
			return test_unop(op,cp);
	}
	if(!cp)
	{
		tp->ap--;
		return *arg!=0;
	}
skip:
	if(!(op = sh_lookup(cp,shtab_testops)))
	{
		if(inparens && c_eq(cp,')'))
		{
			tp->ap--;
			return *arg!=0;
		}
		errormsg(SH_DICT,ERROR_exit(2),e_badop,cp);
		UNREACHABLE();
	}
	if(op&TEST_ANDOR)
		tp->ap--;
	else
		cp = nxtarg(tp,0);
	return test_binop(op,arg,cp);
}

int test_unop(int op,const char *arg)
{
	struct stat statb;
	int f;
	switch(op)
	{
	    case 'r':
		return permission(arg, R_OK);
	    case 'w':
		return permission(arg, W_OK);
	    case 'x':
		return permission(arg, X_OK);
	    case 'd':
		return test_stat(arg,&statb)>=0 && S_ISDIR(statb.st_mode);
	    case 'c':
		return test_stat(arg,&statb)>=0 && S_ISCHR(statb.st_mode);
	    case 'b':
		return test_stat(arg,&statb)>=0 && S_ISBLK(statb.st_mode);
	    case 'f':
		return test_stat(arg,&statb)>=0 && S_ISREG(statb.st_mode);
	    case 'u':
		return test_mode(arg) & S_ISUID;
	    case 'g':
		return test_mode(arg) & S_ISGID;
	    case 'k':
#ifdef S_ISVTX
		return test_mode(arg) & S_ISVTX;
#else
		return 0;
#endif /* S_ISVTX */
#if SHOPT_TEST_L
	    case 'l':
#endif
	    case 'L':
	    case 'h':
		if(*arg==0 || arg[strlen(arg)-1]=='/' || lstat(arg,&statb)<0)
			return 0;
		return S_ISLNK(statb.st_mode);

	    case 'C':
#ifdef S_ISCTG
		return test_stat(arg,&statb)>=0 && S_ISCTG(statb.st_mode);
#else
		return 0;
#endif	/* S_ISCTG */
	    case 'H':
#ifdef S_ISCDF
	    {
		if(test_stat(arg,&statb)>=0 && S_ISCDF(statb.st_mode))
			return 1;
		sfputr(sh.strbuf,arg,'+');
		return test_stat(sfstruse(sh.strbuf),&statb)>=0 && S_ISCDF(statb.st_mode);
	    }
#else
		return 0;
#endif	/* S_ISCDF */

	    case 'S':
		return isasock(arg,&statb);
	    case 'N':
		return test_stat(arg,&statb)>=0 && tmxgetmtime(&statb) > tmxgetatime(&statb);
	    case 'p':
		return isapipe(arg,&statb);
	    case 'n':
		return *arg != 0;
	    case 'z':
		return *arg == 0;
	    case 's':
		sfsync(sfstdout);
		/* FALLTHROUGH */
	    case 'O':
	    case 'G':
		if(*arg==0 || test_stat(arg,&statb)<0)
			return 0;
		if(op=='s')
			return statb.st_size>0;
		else if(op=='O')
			return statb.st_uid==sh.userid;
		return statb.st_gid==sh.groupid;
	    case 'a':
	    case 'e':
		return permission(arg, F_OK);
	    case 'o':
		f=1;
		if(*arg=='?')
			return sh_lookopt(arg+1,&f)>0;
		op = sh_lookopt(arg,&f);
		return op>0 && (f==(sh_isoption(op)!=0));
	    case 't':
	    {
		char *last;
		op = strtol(arg,&last, 10);
		return *last ? 0 : tty_check(op);
	    }
	    case 'v':
	    case 'R':
	    {
		Namval_t *np;
		Namarr_t *ap;
		int isref;
		if(!(np = nv_open(arg,sh.var_tree,NV_VARNAME|NV_NOFAIL|NV_NOADD|NV_NOREF)))
			return 0;
		isref = nv_isref(np);
		if(op=='R')
			return isref;
		if(isref)
		{
			if(np->nvalue)
				np = nv_refnode(np);
			else
				return 0;
		}
		if(ap = nv_arrayptr(np))
			return nv_arrayisset(np,ap);
		return !nv_isnull(np);
	    }
	    default:
	    {
		static char a[3] = "-?";
		a[1]= op;
		errormsg(SH_DICT,ERROR_exit(2),e_badop,a);
		UNREACHABLE();
	    }
	}
}

/*
 * This function handles binary operators for both the
 * test/[ built-in and the [[ ... ]] compound command
 */
int test_binop(int op,const char *left,const char *right)
{
	if(op&TEST_ARITH)
	{
		Sfdouble_t lnum, rnum;
		if(sh.bltinfun==b_test && sh_isoption(SH_POSIX))
		{
			/* for test/[ in POSIX, only accept simple decimal numbers */
			char *l = (char*)left, *r = (char*)right;
			while(*l=='0')
				l++;
			while(*r=='0')
				r++;
			lnum = strtold(l,&l);
			rnum = strtold(r,&r);
			if(*l || *r)
			{
				errormsg(SH_DICT, ERROR_exit(2), e_number, *l ? left : right);
				UNREACHABLE();
			}
		}
		else
		{
			/* numeric operands are arithmetic expressions */
			lnum = sh_arith(left);
			rnum = sh_arith(right);
		}
		switch(op)
		{
			case TEST_EQ:
				return lnum==rnum;
			case TEST_NE:
				return lnum!=rnum;
			case TEST_GT:
				return lnum>rnum;
			case TEST_LT:
				return lnum<rnum;
			case TEST_GE:
				return lnum>=rnum;
			case TEST_LE:
				return lnum<=rnum;
		}
		/* all arithmetic binary operators should be covered above */
		UNREACHABLE();
	}
	switch(op)
	{
		case TEST_AND:
		case TEST_OR:
			return *left != 0;
		case TEST_PEQ:
			return test_strmatch(left, right);
		case TEST_PNE:
			return !test_strmatch(left, right);
		case TEST_SGT:
			return strcoll(left, right) > 0;
		case TEST_SLT:
			return strcoll(left, right) < 0;
		case TEST_SEQ:
			return strcmp(left, right) == 0;
		case TEST_SNE:
			return strcmp(left, right) != 0;
		case TEST_REP:
			sfprintf(sh.strbuf, "~(E)%s", right);
			return test_strmatch(left, sfstruse(sh.strbuf)) > 0;
		case TEST_EF:
			return test_inode(left,right);
		case TEST_NT:
			return test_time(left,right) > 0;
		case TEST_OT:
			return test_time(left,right) < 0;
	}
	/* all non-arithmetic binary operators should be covered above */
	UNREACHABLE();
}

/*
 * returns 1 if file1 was modified more recently than file2, or if file1 exists and file2 does not
 * returns -1 if file2 was modified more recently than file1, or if file2 exists and file1 does not
 * returns 0 if file1 was modified at the same time as file2, or if neither file1 nor file2 exist
 */
static int test_time(const char *file1,const char *file2)
{
	Time_t t1, t2;
	struct stat statb1,statb2;
	int r=test_stat(file2,&statb2);
	if(test_stat(file1,&statb1)<0)
		return r<0?0:-1;
	if(r<0)
		return 1;
	t1 = tmxgetmtime(&statb1);
	t2 = tmxgetmtime(&statb2);
	if (t1 > t2)
		return 1;
	if (t1 < t2)
		return -1;
	return 0;
}

/*
 * return true if inode of two files are the same
 */
int test_inode(const char *file1,const char *file2)
{
	struct stat stat1,stat2;
	if(test_stat(file1,&stat1)>=0  && test_stat(file2,&stat2)>=0)
		if(stat1.st_dev == stat2.st_dev && stat1.st_ino == stat2.st_ino)
			return 1;
	return 0;
}


/*
 * This version of access checks against the effective UID/GID
 */
int sh_access(const char *name, int mode)
{
	struct stat statb;
	if(*name==0)
		return -1;
	if(!sh_isoption(SH_POSIX) && sh_isdevfd(name))
		return sh_ioaccess((int)strtol(name+8, NULL, 10),mode);
	/* can't use access function for execute permission with root */
	if(mode==X_OK && sh.euserid==0)
		goto skip;
	if(sh.userid==sh.euserid && sh.groupid==sh.egroupid)
		return access(name,mode);
#if _lib_setreuid
	/* swap the real UID to effective, check access then restore */
	/* first swap real and effective GID, if different */
	if(sh.groupid==sh.euserid || setregid(sh.egroupid,sh.groupid)==0)
	{
		/* next swap real and effective UID, if needed */
		if(sh.userid==sh.euserid || setreuid(sh.euserid,sh.userid)==0)
		{
			mode = access(name,mode);
			/* restore IDs */
			if(sh.userid!=sh.euserid)
				setreuid(sh.userid,sh.euserid);
			if(sh.groupid!=sh.egroupid)
				setregid(sh.groupid,sh.egroupid);
			return mode;
		}
		else if(sh.groupid!=sh.egroupid)
			setregid(sh.groupid,sh.egroupid);
	}
#endif /* _lib_setreuid */
skip:
	if(test_stat(name, &statb) == 0)
	{
		if(mode == F_OK)
			return mode;
		else if(sh.euserid == 0)
		{
			if(!S_ISREG(statb.st_mode) || mode!=X_OK)
				return 0;
		    	/* root needs execute permission for someone */
			mode = (S_IXUSR|S_IXGRP|S_IXOTH);
		}
		else if(sh.euserid == statb.st_uid)
			mode <<= 6;
		else if(sh.egroupid == statb.st_gid)
			mode <<= 3;
#if _lib_getgroups
		/* on some systems you can be in several groups */
		else
		{
			static int maxgroups;
			gid_t *groups;
			int n;
			if(maxgroups==0)
			{
				/* first time */
				if((maxgroups=getgroups(0,NULL)) <= 0)
				{
					/* pre-POSIX system */
					maxgroups = (int)astconf_long(CONF_NGROUPS_MAX);
				}
			}
			groups = stkalloc(sh.stk,(maxgroups+1)*sizeof(gid_t));
			n = getgroups(maxgroups,groups);
			while(--n >= 0)
			{
				if(groups[n] == statb.st_gid)
				{
					mode <<= 3;
					break;
				}
			}
		}
#endif /* _lib_getgroups */
		if(statb.st_mode & mode)
			return 0;
	}
	return -1;
}

/*
 * Return the mode bits of file <file>
 * If <file> is null, then the previous stat buffer is used.
 * The mode bits are zero if the file doesn't exist.
 */
static int test_mode(const char *file)
{
	struct stat statb;
	statb.st_mode = 0;
	if(file && (*file==0 || test_stat(file,&statb)<0))
		return 0;
	return statb.st_mode;
}

/*
 * do an fstat() for /dev/fd/n, otherwise stat()
 */
static int test_stat(const char *name,struct stat *buff)
{
	if(*name==0)
	{
		errno = ENOENT;
		return -1;
	}
	if(sh_isdevfd(name))
		return fstat((int)strtol(name+8, NULL, 10),buff);
	else
		return stat(name,buff);
}

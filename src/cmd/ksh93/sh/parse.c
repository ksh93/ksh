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
 *  This is the parser for a shell language
 */

#include	"shopt.h"
#include	"defs.h"
#include	<fcin.h>
#include	<error.h>
#include	"shlex.h"
#include	"history.h"
#include	"builtins.h"
#include	"path.h"
#include	"test.h"
#include	"history.h"
#include	"version.h"

#define HERE_MEM	SFIO_BUFSIZE	/* size of here-docs kept in memory */

/* These routines are local to this module */

static Shnode_t	*makeparent(Lex_t*, int, Shnode_t*);
static Shnode_t	*makelist(Lex_t*, int, Shnode_t*, Shnode_t*);
static struct argnod	*qscan(struct comnod*, int);
static struct ionod	*inout(Lex_t*,struct ionod*, int);
static Shnode_t	*sh_cmd(Lex_t*,int,int);
static Shnode_t	*term(Lex_t*,int);
static Shnode_t	*list(Lex_t*,int);
static struct regnod	*syncase(Lex_t*,int);
static Shnode_t	*item(Lex_t*,int);
static Shnode_t	*simple(Lex_t*,int, struct ionod*);
static int	skipnl(Lex_t*,int);
static Shnode_t	*test_expr(Lex_t*,int);
static Shnode_t	*test_and(Lex_t*);
static Shnode_t	*test_or(Lex_t*);
static Shnode_t	*test_primary(Lex_t*);

static void	dcl_hacktivate(void), dcl_dehacktivate(void), (*orig_exit)(int), dcl_exit(int);
static Dt_t	*dcl_tree;
static unsigned	dcl_recursion;

#define sh_getlineno(lp)	(lp->lastline)

#define CNTL(x)		((x)&037)

static int		opt_get;

#define getnode(type)	stkalloc(sh.stk,sizeof(struct type))

#if SHOPT_KIA
/*
 * write out entities for each item in the list
 * type=='V' for variable assignment lists
 * Otherwise type is determined by the command */
static unsigned long writedefs(Lex_t *lexp,struct argnod *arglist, int line, int type, struct argnod *cmd)
{
	struct argnod *argp = arglist;
	char *cp;
	int n,eline;
	int width=0;
	unsigned long r=0;
	static char atbuff[20];
	int  justify=0;
	char *attribute = atbuff;
	unsigned long parent=kia.script;
	if(type==0)
	{
		parent = kia.current;
		type = 'v';
		switch(*argp->argval)
		{
		    case 'a':
			type='p';
			justify = 'a';
			break;
		    case 'e':
			*attribute++ =  'x';
			break;
		    case 'r':
			*attribute++ = 'r';
			break;
		    case 'l':
			break;
		}
		while(argp = argp->argnxt.ap)
		{
			if((n= *(cp=argp->argval))!='-' && n!='+')
				break;
			if(cp[1]==n)
				break;
			while((n= *++cp))
			{
				if(isdigit(n))
					width = 10*width + n-'0';
				else if(n=='L' || n=='R' || n =='Z')
					justify=n;
				else
					*attribute++ = n;
			}
		}
	}
	else if(cmd)
		parent=kiaentity(lexp,sh_argstr(cmd),-1,'p',-1,-1,kia.unknown,'b',0,"");
	*attribute = 0;
	while(argp)
	{
		if((cp=strchr(argp->argval,'='))||(cp=strchr(argp->argval,'?')))
			n = cp-argp->argval;
		else
			n = strlen(argp->argval);
		eline = sh.inlineno-(lexp->token==NL);
		r=kiaentity(lexp,argp->argval,n,type,line,eline,parent,justify,width,atbuff);
		sfprintf(kia.tmp,"p;%..64d;v;%..64d;%d;%d;s;\n",kia.current,r,line,eline);
		argp = argp->argnxt.ap;
	}
	return r;
}
#endif /* SHOPT_KIA */

static void typeset_order(const char *str,int line)
{
	int			c,n=0;
	unsigned const char	*cp=(unsigned char*)str;
	static unsigned char	*table;
	if(*cp!='+' && *cp!='-')
		return;
	if(!table)
	{
		table = sh_calloc(1,256);
		for(cp=(unsigned char*)"bflmnprstuxACHS";c = *cp; cp++)
			table[c] = 1;
		for(cp=(unsigned char*)"aiEFLRXhTZ";c = *cp; cp++)
			table[c] = 2;
		for(c='0'; c <='9'; c++)
			table[c] = 3;
	}
	for(cp=(unsigned char*)str; c= *cp++; n=table[c])
	{
		if(table[c] < n)
			errormsg(SH_DICT,ERROR_warn(0),e_lextypeset,line,str);
	}
}

static noreturn int b_dummy(int argc, char *argv[], Shbltin_t *context)
{
	NOT_USED(argc);
	NOT_USED(argv[0]);
	NOT_USED(context);
	errormsg(SH_DICT,ERROR_PANIC,e_internal);
	UNREACHABLE();
}

/*
 * This function handles linting for 'typeset' options via typeset_order().
 *
 * Also, upon parsing typeset -T or enum, it pre-adds the type declaration built-ins that these would create to
 * an internal tree to avoid syntax errors upon pre-execution parsing of assignment-arguments with parentheses.
 *
 * intypeset==1 for typeset & friends; intypeset==2 for enum
 */
static void check_typedef(struct comnod *tp, char intypeset)
{
	char	*cp=0;		/* name of built-in to pre-add */
	if(tp->comtyp&COMSCAN)
	{
		struct argnod *ap = tp->comarg.ap;
		while(ap = ap->argnxt.ap)
		{
			if(!(ap->argflag&ARG_RAW) || strncmp(ap->argval,"--",2))
				break;
			if(sh_isoption(SH_NOEXEC))
				typeset_order(ap->argval,tp->comline);
			if(strncmp(ap->argval,"-T",2)==0)
			{
				if(ap->argval[2])
					cp = ap->argval+2;
				else if((ap->argnxt.ap)->argflag&ARG_RAW)
					cp = (ap->argnxt.ap)->argval;
				if(cp)
					break;
			}
		}
	}
	else
	{
		struct dolnod *dp = tp->comarg.dp;
		char **argv = dp->dolval + ARG_SPARE;
		if(intypeset==2)
		{
			/* Skip over possible 'command' prefix(es) */
			while(*argv && strcmp(*argv, SYSENUM->nvname))
				argv++;
			/* Skip over 'enum' options */
			opt_info.index = 0;
			while(optget(argv, sh_optenum))
				;
			if(error_info.errors)
				return;
			cp = argv[opt_info.index];
		}
		else while((cp = *argv++) && strncmp(cp,"--",2))
		{
			if(sh_isoption(SH_NOEXEC))
				typeset_order(cp,tp->comline);
			if(strncmp(cp,"-T",2)==0)
			{
				if(cp[2])
					cp = cp+2;
				else
					cp = *argv;
				break;
			}
		}
	}
	if(cp)
	{
		Dt_t *tp = sh.bltin_tree;
		if(!dcl_tree)
		{
			dcl_tree = dtopen(&_Nvdisc, Dtoset);
			dtview(sh.bltin_tree, dcl_tree);
		}
		sh.bltin_tree = dcl_tree;
		nv_onattr(sh_addbuiltin(cp, b_dummy, NULL), NV_BLTIN|BLT_DCL);
		sh.bltin_tree = tp;
	}
}
/*
 * Hack to avoid an inconsistent state if a 'typeset -T' or 'enum' declaration is parsed but not executed:
 *
 * The parser needs to know about to-be-created type built-ins before their declarations are executed,
 * otherwise assignment-arguments with parentheses -- e.g., Type_t foo=(bar baz) -- are a syntax error if
 * parsed within the same pass as the 'typeset -T Type_t=(...)' declaration. This would be especially bad
 * in dot scripts, which are completely parsed in one single sh_parse() call before execution.
 *
 * Previously, to cope with this, ksh simply added preliminary dummy versions of the built-ins to
 * sh.bltin_tree at parse time, avoiding the syntax error. The idea is that these dummies get overwritten
 * by nv_addtype() in nvtype.c at execution time. But if the 'typeset -T' declaration was parsed but not
 * executed (due to 'if', etc.), only the dummy built-in was left, which crashed the shell when run.
 *
 * To fix this, we now (de)activate an internal declaration built-ins tree, dcl_tree, into which
 * check_typedef() can pre-add those dummies. A viewpath from the main built-ins tree to this internal tree
 * is added, unifying the two for search purposes. When parsing is done, we close that viewpath and (unless
 * compiling with shcomp) clear out the dummy nodes, leaving no trace after parsing and before executing.
 * The real type built-in can then safely be either added or not added at execution time.
 */
static void dcl_hacktivate(void)
{
	if(dcl_recursion++)
		return;
	if(dcl_tree)
		dtview(sh.bltin_tree, dcl_tree);
	orig_exit = error_info.exit;
	error_info.exit = dcl_exit;
}
static void dcl_dehacktivate(void)
{
	if(!dcl_recursion || --dcl_recursion)
		return;
	error_info.exit = orig_exit;
	if(dcl_tree)
	{
		dtview(sh.bltin_tree, NULL);
		if(!sh.shcomp)
			dtclear(dcl_tree);
	}
}
static noreturn void dcl_exit(int e)
{
	dcl_recursion = 1;
	dcl_dehacktivate();
	(*error_info.exit)(e);
	UNREACHABLE();
}

/*
 * Make a parent node for fork() or io-redirection
 */
static Shnode_t	*makeparent(Lex_t *lp, int flag, Shnode_t *child)
{
	Shnode_t	*par = getnode(forknod);
	par->fork.forktyp = flag;
	par->fork.forktre = child;
	par->fork.forkio = 0;
	par->fork.forkline = sh_getlineno(lp)-1;
	return par;
}

static const char *paramsub(const char *str)
{
	int c,sub=0,lit=0;
	while(c= *str++)
	{
		if(c=='$' && !lit)
		{
			if(*str=='(')
				return NULL;
			if(sub)
				continue;
			if(*str=='{')
				str++;
			if(!isdigit(*str) && strchr("?#@*!$ ",*str)==0)
			{
				if(str[-1]=='{')
					str--;  /* variable in the form of ${var} */
				return str;
			}
		}
		else if(c=='`')
			return NULL;
		else if(c=='[' && !lit)
			sub++;
		else if(c==']' && !lit)
			sub--;
		else if(c=='\'')
			lit = !lit;
	}
	return NULL;
}

static Shnode_t *getanode(Lex_t *lp, struct argnod *ap)
{
	Shnode_t *t = getnode(arithnod);
	t->ar.artyp = TARITH;
	t->ar.arline = sh_getlineno(lp);
	t->ar.arexpr = ap;
	if(ap->argflag&ARG_RAW)
		t->ar.arcomp = sh_arithcomp(ap->argval);
	else
	{
		const char *p, *q;
		if(sh_isoption(SH_NOEXEC) && (ap->argflag&ARG_MAC) &&
			((p = paramsub(ap->argval)) != NULL))
		{
			for(q = p; !isspace(*q) && *q != '\0'; q++)
				;
			errormsg(SH_DICT, ERROR_warn(0), e_lexwarnvar,
				sh.inlineno, ap->argval, q - p, p);
		}
		t->ar.arcomp = 0;
	}
	return t;
}

/*
 *  Make a node corresponding to a command list
 */
static Shnode_t	*makelist(Lex_t *lexp, int type, Shnode_t *l, Shnode_t *r)
{
	Shnode_t	*t = NULL;
	if(!l || !r)
		sh_syntax(lexp,0);
	else
	{
		if((type&COMMSK) == TTST)
			t = getnode(tstnod);
		else
			t = getnode(lstnod);
		t->lst.lsttyp = type;
		t->lst.lstlef = l;
		t->lst.lstrit = r;
	}
	return t;
}

/*
 * entry to shell parser
 * Flag can be the union of SH_EOF|SH_NL
 */
void	*sh_parse(Sfio_t *iop, int flag)
{
	Shnode_t	*t;
	Lex_t		*lexp = (Lex_t*)sh.lex_context;
	Fcin_t		sav_input;
	struct argnod	*sav_arg = lexp->arg;
	int		sav_prompt = sh.nextprompt;
	if(sh.binscript && (sffileno(iop)==sh.infd || (flag&SH_FUNEVAL)))
		return sh_trestore(iop);
	fcsave(&sav_input);
	sh.st.staklist = 0;
	lexp->assignlevel = 0;
	lexp->noreserv = 0;
	lexp->heredoc = 0;
	lexp->inlineno = sh.inlineno;
	lexp->firstline = sh.st.firstline;
	sh.nextprompt = 1;
	if(sh_isoption(SH_VERBOSE))
		sh_onstate(SH_VERBOSE);
	sh_lexopen(lexp,0);
	if(fcfopen(iop) < 0)
		return NULL;
	if(fcfile())
	{
		char *cp = fcfirst();
		if( cp[0]==CNTL('k') &&  cp[1]==CNTL('s') && cp[2]==CNTL('h') && cp[3]==0)
		{
			int version;
			fcseek(4);
			version = fcgetc();
			fcclose();
			fcrestore(&sav_input);
			lexp->arg = sav_arg;
			if(version > SHCOMP_HDR_VERSION)
			{
				errormsg(SH_DICT,ERROR_exit(1),e_lexversion);
				UNREACHABLE();
			}
			if(sffileno(iop)==sh.infd || (flag&SH_FUNEVAL))
				sh.binscript = 1;
			sfgetc(iop);
			t = sh_trestore(iop);
			if(flag&SH_NL)
			{
				Shnode_t *tt;
				while(1)
				{
					if(!(tt = sh_trestore(iop)))
						break;
					t =makelist(lexp,TLST, t, tt);
				}
			}
			return t;
		}
	}
	flag &= ~SH_FUNEVAL;
	if((flag&SH_NL) && (sh.inlineno=error_info.line+sh.st.firstline)==0)
		sh.inlineno=1;
	sh.nextprompt = 2;
	t = sh_cmd(lexp,(flag&SH_EOF)?EOFSYM:'\n',SH_SEMI|SH_EMPTY|(flag&SH_NL));
	fcclose();
	fcrestore(&sav_input);
	lexp->arg = sav_arg;
	/* unstack any completed alias expansions */
	if((sfset(iop,0,0)&SFIO_STRING) && !sfreserve(iop,0,-1))
	{
		Sfio_t *sp = sfstack(iop,NULL);
		if(sp)
			sfclose(sp);
	}
	sh.nextprompt = sav_prompt;
	if(flag&SH_NL)
	{
		sh.st.firstline = lexp->firstline;
		sh.inlineno = lexp->inlineno;
	}
	stkseek(sh.stk,0);
	return t;
}

/*
 * This routine parses up the matching right parenthesis and returns
 * the parse tree
 */
Shnode_t *sh_dolparen(Lex_t* lp)
{
	Shnode_t *t=0;
	Sfio_t *sp = fcfile();
	int line = sh.inlineno;
	sh.inlineno = error_info.line+sh.st.firstline;
	sh_lexopen(lp,1);
	lp->comsub = 1;
	switch(sh_lex(lp))
	{
	    /* ((...)) arithmetic expression */
	    case EXPRSYM:
		t = getanode(lp,lp->arg);
		break;
	    case LPAREN:
		t = sh_cmd(lp,RPAREN,SH_NL|SH_EMPTY);
		break;
	    case LBRACE:
		t = sh_cmd(lp,RBRACE,SH_NL|SH_EMPTY);
		break;
	}
	lp->comsub = 0;
	if(!sp && (sp=fcfile()))
	{
		/*
		 * This code handles the case where string has been converted
		 * to a file by an alias setup
		 */
		char *cp;
		if(fcgetc() > 0)
			fcseek(-1);
		cp = fcseek(0);
		fcclose();
		fcsopen(cp);
		sfclose(sp);
	}
	sh.inlineno = line;
	return t;
}

/*
 * remove temporary files and stacks
 */
void	sh_freeup(void)
{
	if(sh.st.staklist)
		sh_funstaks(sh.st.staklist,-1);
	sh.st.staklist = 0;
}

/*
 * increase reference count for each stack in function list when flag>0
 * decrease reference count for each stack in function list when flag<=0
 * stack is freed when reference count is zero
 */
void sh_funstaks(struct slnod *slp,int flag)
{
	struct slnod *slpold;
	while(slpold=slp)
	{
		if(slp->slchild)
			sh_funstaks(slp->slchild,flag);
		slp = slp->slnext;
		if(slpold->slptr)
		{
			if(flag<=0)
			{
				/*
				 * Since we're dealing with a linked list of stacks, slpold may be inside the allocated region
				 * pointed to by slpold->slptr, meaning the stkclose() call may invalidate slpold as well as
				 * slpold->slptr. So if we do 'stkclose(slpold->slptr); slpold->slptr = NULL' as may
				 * seem obvious, the assignment may be a use-after-free of slpold. Therefore, save the pointer
				 * value and reset the pointer before closing/freeing the stack.
				 */
				Stk_t *sp = slpold->slptr;
				slpold->slptr = NULL;
				stkclose(sp);
			}
			else
				stklink(slpold->slptr);
		}
	}
}
/*
 * cmd
 *	empty
 *	list
 *	list & [ cmd ]
 *	list [ ; cmd ]
 */
static Shnode_t	*sh_cmd(Lex_t *lexp, int sym, int flag)
{
	Shnode_t	*left, *right;
	int		type = FINT|FAMP;
	dcl_hacktivate();
	if(sym==NL)
		lexp->lasttok = 0;
	left = list(lexp,flag);
	if(lexp->token==NL)
	{
		if(flag&SH_NL)
			lexp->token=';';
	}
	else if(!left && !(flag&SH_EMPTY))
		sh_syntax(lexp,0);
	switch(lexp->token)
	{
	    case COOPSYM:		/* set up a cooperating process */
		type |= (FPIN|FPOU|FPCL|FCOOP);
		/* FALLTHROUGH */
	    case '&':
		if(left)
		{
			/* (...)& -> {...;} & */
			if(left->tre.tretyp==TPAR)
				left = left->par.partre;
			left = makeparent(lexp,TFORK|type, left);
		}
		/* FALLTHROUGH */
	    case ';':
		if(!left)
			sh_syntax(lexp,0);
		if(right=sh_cmd(lexp,sym,flag|SH_EMPTY))
			left=makelist(lexp,TLST, left, right);
		break;
	    case EOFSYM:
		if(sym==NL)
			break;
		/* FALLTHROUGH */
	    default:
		if(sym && sym!=lexp->token)
		{
			if(sym!=ELSESYM || (lexp->token!=ELIFSYM && lexp->token!=FISYM))
				sh_syntax(lexp,0);
		}
	}
	dcl_dehacktivate();
	return left;
}

/*
 * list
 *	term
 *	list && term
 *	list || term
 *      unfortunately, these are equal precedence
 */
static Shnode_t	*list(Lex_t *lexp, int flag)
{
	Shnode_t	*t = term(lexp,flag);
	int 		token;
	while(t && ((token=lexp->token)==ANDFSYM || token==ORFSYM))
		t = makelist(lexp,(token==ANDFSYM?TAND:TORF), t, term(lexp,SH_NL|SH_SEMI));
	return t;
}

/*
 * term
 *	item
 *	item | term
 */
static Shnode_t	*term(Lex_t *lexp,int flag)
{
	Shnode_t	*t;
	int		token;
	if(flag&SH_NL)
		token = skipnl(lexp,flag);
	else
		token = sh_lex(lexp);
	/* check to see if pipeline is to be timed */
	if(token==TIMESYM || token==NOTSYM)
	{
		t = getnode(parnod);
		t->par.partyp=TTIME;
		if(lexp->token==NOTSYM)
			t->par.partyp |= COMSCAN;
		t->par.partre = term(lexp,0);
	}
	else if((t=item(lexp,SH_NL|SH_EMPTY|(flag&SH_SEMI))) && lexp->token=='|')
	{
		Shnode_t	*tt;
		int showme = t->tre.tretyp&FSHOWME;
		t = makeparent(lexp,TFORK|FPOU,t);
		if(tt=term(lexp,SH_NL))
		{
			switch(tt->tre.tretyp&COMMSK)
			{
			    case TFORK:
				tt->tre.tretyp |= FPIN|FPCL;
				break;
			    case TFIL:
				tt->lst.lstlef->tre.tretyp |= FPIN|FPCL;
				break;
			    default:
				tt= makeparent(lexp,TSETIO|FPIN|FPCL,tt);
			}
			t=makelist(lexp,TFIL,t,tt);
			t->tre.tretyp |= showme;
		}
		else if(lexp->token)
			sh_syntax(lexp,0);
	}
	return t;
}

/*
 * case statement
 */
static struct regnod*	syncase(Lex_t *lexp,int esym)
{
	int tok = skipnl(lexp,0);
	struct regnod	*r;
	if(tok==esym)
		return NULL;
	r = stkalloc(sh.stk,sizeof(struct regnod));
	r->regptr=0;
	r->regflag=0;
	if(tok==LPAREN)
		skipnl(lexp,0);
	while(1)
	{
		if(!lexp->arg)
			sh_syntax(lexp,0);
		lexp->arg->argnxt.ap=r->regptr;
		r->regptr = lexp->arg;
		if((tok=sh_lex(lexp))==RPAREN)
			break;
		else if(tok=='|')
			sh_lex(lexp);
		else
			sh_syntax(lexp,0);
	}
	r->regcom=sh_cmd(lexp,0,SH_NL|SH_EMPTY|SH_SEMI);
	switch (tok = lexp->token)
	{
	case FALLMATCHSYM:	/* ;;& */
		r->regflag++;
		/* FALLTHROUGH */
	case FALLTHRUSYM:	/* ;& */
		r->regflag++;
		/* FALLTHROUGH */
	case BREAKCASESYM:	/* ;; */
		r->regnxt=syncase(lexp,esym);
		break;
	default:
		if(tok!=esym && tok!=EOFSYM)
			sh_syntax(lexp,0);
		r->regnxt=0;
	}
	if(lexp->token==EOFSYM)
		return NULL;
	return r;
}

/*
 * This routine creates the parse tree for the arithmetic for
 * When called, shlex.arg contains the string inside ((...))
 * When the first argument is missing, a while node is returned
 * Otherwise a list containing an arithmetic command and a while
 * is returned.
 */
static Shnode_t	*arithfor(Lex_t *lexp,Shnode_t *tf)
{
	Shnode_t	*t, *tw = tf;
	int		offset;
	struct argnod	*argp;
	int		n;
	int		argflag = lexp->arg->argflag;
	Fcin_t		sav_input;
	/* save current input */
	fcsave(&sav_input);
	fcsopen(lexp->arg->argval);
	/* split ((...)) into three expressions */
	for(n=0; ; n++)
	{
		int c;
		argp = stkseek(sh.stk,ARGVAL);
		argp->argnxt.ap = 0;
		argp->argchn.cp = 0;
		argp->argflag = argflag;
		if(n==2)
			break;
		/* copy up to ; onto the stack */
		sh_lexskip(lexp,';',1,ST_NESTED);
		offset = stktell(sh.stk)-1;
		if(!lexp->token)
			break;
		/* remove trailing white space */
		while(offset>ARGVAL && ((c= *stkptr(sh.stk,offset-1)),isspace(c)))
			offset--;
		/* check for empty initialization expression */
		if(offset==ARGVAL && n==0)
			continue;
		stkseek(sh.stk,offset);
		/* check for empty condition and treat as while((1)) */
		if(offset==ARGVAL)
			sfputc(sh.stk,'1');
		argp = stkfreeze(sh.stk,1);
		t = getanode(lexp,argp);
		if(n==0)
			tf = makelist(lexp,TLST,t,tw);
		else
			tw->wh.whtre = t;
	}
	while((offset=fcpeek(0)) && isspace(offset))
		fcseek(1);
	sfputr(sh.stk,fcseek(0),-1);
	argp = stkfreeze(sh.stk,1);
	fcrestore(&sav_input);
	if(n<2)
	{
		lexp->token = RPAREN|SYMREP;
		sh_syntax(lexp,0);
	}
	/* check whether the increment is present */
	if(*argp->argval)
	{
		t = getanode(lexp,argp);
		tw->wh.whinc = (struct arithnod*)t;
	}
	else
		tw->wh.whinc = 0;
	sh_lexopen(lexp, 1);
	if((n=sh_lex(lexp))==NL)
		n = skipnl(lexp,0);
	else if(n==';')
		n = sh_lex(lexp);
	if(n!=DOSYM && n!=LBRACE)
		sh_syntax(lexp,0);
	tw->wh.dotre = sh_cmd(lexp,n==DOSYM?DONESYM:RBRACE,SH_NL|SH_SEMI);
	tw->wh.whtyp = TWH;
	return tf;
}

static Shnode_t *funct(Lex_t *lexp)
{
	Shnode_t *t;
	int flag;
	struct slnod *volatile slp=0;
	Stk_t *volatile savstak=0;
	struct functnod *volatile fp;
	Sfio_t *iop;
#if SHOPT_KIA
	unsigned long current = kia.current;
#endif /* SHOPT_KIA */
	int nargs=0,size=0,jmpval;
	struct  checkpt buff;
	int save_optget = opt_get;
	void	*in_mktype = sh.mktype;
	sh.mktype = 0;
	opt_get = 0;
	t = getnode(functnod);
	t->funct.functline = sh.inlineno;
	t->funct.functtyp=TFUN;
	t->funct.functargs = 0;
	if(!(flag = (lexp->token==FUNCTSYM)))
		t->funct.functtyp |= FPOSIX;
	else if(sh_lex(lexp))
		sh_syntax(lexp,0);
	if(!(iop=fcfile()))
	{
		iop = sfopen(NULL,fcseek(0),"s");
		fcclose();
		fcfopen(iop);
	}
	t->funct.functnam= (char*)lexp->arg->argval;
#if SHOPT_KIA
	if(kia.file)
		kia.current = kiaentity(lexp,t->funct.functnam,-1,'p',-1,-1,kia.script,'p',0,"");
#endif /* SHOPT_KIA */
	if(flag)
		lexp->token = sh_lex(lexp);
	if(t->funct.functtyp&FPOSIX)
		skipnl(lexp,0);
	else
	{
		if(lexp->token==0)
		{
			struct comnod	*ac;
			char		*cp, **argv, **argv0;
			int		c=-1;
			t->funct.functargs = ac = (struct comnod*)simple(lexp,SH_NOIO|SH_FUNDEF,NULL);
			if(ac->comset || (ac->comtyp&COMSCAN))
				sh_syntax(lexp,2);
			argv0 = argv = ac->comarg.dp->dolval + ARG_SPARE;
			while(cp= *argv++)
			{
				size += strlen(cp)+1;
				if((c = mbchar(cp)) && isaletter(c))
		                        while(c=mbchar(cp), isaname(c));
			}
			if(c)
				sh_syntax(lexp,2);
			nargs = argv-argv0;
			size += sizeof(struct dolnod)+(nargs+ARG_SPARE)*sizeof(char*);
			if(sh.shcomp && strncmp(".sh.math.",t->funct.functnam,9)==0)
			{
				struct Ufunction *rp;
				Namval_t *np= nv_open(t->funct.functnam,sh.fun_tree,NV_ADD|NV_VARNAME);
				rp = np->nvalue = new_of(struct Ufunction,sh.funload?sizeof(Dtlink_t):0);
				memset(rp, 0, sizeof(struct Ufunction));
				rp->argc = ac->comarg.dp->dolnum;
			}
		}
		while(lexp->token==NL)
			lexp->token = sh_lex(lexp);
	}
	if((flag && lexp->token!=LBRACE) || lexp->token==EOFSYM)
		sh_syntax(lexp,0);
	sh_pushcontext(&buff,1);
	jmpval = sigsetjmp(buff.buff,0);
	if(jmpval == 0)
	{
		/* create a new stack to compile the command */
		savstak = sh.stk;
		sh.stk = stkopen(STK_SMALL);
		slp = stkalloc(sh.stk,sizeof(struct slnod)+sizeof(struct functnod));
		slp->slchild = 0;
		slp->slnext = sh.st.staklist;
		sh.st.staklist = 0;
		t->funct.functstak = (struct slnod*)slp;
		/*
		 * store the pathname of function definition file on stack
		 * in name field of fake for node
		 */
		fp = (struct functnod*)(slp+1);
		fp->functtyp = TFUN|FAMP;
		fp->functnam = 0;
		fp->functargs = 0;
		fp->functline = t->funct.functline;
		if(sh.st.filename)
			fp->functnam = stkcopy(sh.stk,sh.st.filename);
		if(size)
		{
			struct dolnod *dp = stkalloc(sh.stk,size);
			char *cp, *sp, **argv, **old = t->funct.functargs->comarg.dp->dolval + 1;
			argv = ((char**)(dp->dolval))+1;
			dp->dolnum = t->funct.functargs->comarg.dp->dolnum;
			t->funct.functargs->comarg.dp = dp;
			for(cp=(char*)&argv[nargs]; sp= *old++; cp++)
			{
				*argv++ = cp;
				cp = strcopy(cp,sp);
			}
			*argv = 0;
		}
		if(!flag && lexp->token==0)
		{
			/* functname() simple_command: copy current word token to current stack frame */
			size_t sz = ARGVAL + strlen(lexp->arg->argval) + 1;  /* include terminating 0 */
			struct argnod *ap = stkalloc(sh.stk,sz);
			memcpy(ap,lexp->arg,sz);
			lexp->arg = ap;
		}
		lexp->assignok = 1;
		t->funct.functtre = item(lexp,SH_NOIO);
	}
	else if(sh.shcomp)
		exit(1);
	sh_popcontext(&buff);
	/* restore the old stack */
	if(slp)
	{
		slp->slptr = sh.stk;
		sh.stk = savstak;
		slp->slchild = sh.st.staklist;
	}
#if SHOPT_KIA
	kia.current = current;
#endif /* SHOPT_KIA */
	if(jmpval)
	{
		if(slp && slp->slptr)
		{
			Stk_t *slptr_save = slp->slptr;
			sh.st.staklist = slp->slnext;
			slp->slptr = NULL;
			stkclose(slptr_save);
		}
		siglongjmp(*sh.jmplist,jmpval);
	}
	sh.st.staklist = (struct slnod*)slp;
	fp->functtre = t;
	sh.mktype = in_mktype;
#if 	SHOPT_KIA
	if(kia.file)
		kiaentity(lexp,t->funct.functnam,-1,'p',t->funct.functline,sh.inlineno-1,kia.current,'p',0,"");
#endif /* SHOPT_KIA */
	t->funct.functtyp |= opt_get;
	opt_get = save_optget;
	return t;
}

static int check_array(Lex_t *lexp)
{
	int c;
	if(lexp->token==0 && strcmp(lexp->arg->argval, SYSTYPESET->nvname)==0)
	{
		while((c = fcgetc())==' ' || c=='\t');
		if(c=='-')
		{
			if(fcgetc()=='a')
			{
				lexp->assignok = SH_ASSIGN;
				lexp->noreserv = 1;
				sh_lex(lexp);
				return 1;
			}
			else
				fcseek(-2);
		}
		else
			fcseek(-1);
	}
	return 0;
}

/*
 * Compound assignment
 */
static struct argnod *assign(Lex_t *lexp, struct argnod *ap, int type)
{
	int n;
	Shnode_t *t, **tp;
	struct comnod *ac = NULL;
	int array=0, index=0;
	Namval_t *np;
	lexp->assignlevel++;
	n = strlen(ap->argval)-1;
	if(ap->argval[n]!='=')
		sh_syntax(lexp,0);
	if(ap->argval[n-1]=='+')
	{
		ap->argval[n--]=0;
		array = ARG_APPEND;
		type |= NV_APPEND;
	}
	/* shift right */
	while(n > 0)
	{
		ap->argval[n] = ap->argval[n-1];
		n--;
	}
	*ap->argval=0;
	t = getnode(fornod);
	t->for_.fornam = (char*)(ap->argval+1);
	t->for_.fortyp = sh_getlineno(lexp);
	tp = &t->for_.fortre;
	ap->argchn.ap = (struct argnod*)t;
	ap->argflag &= ARG_QUOTED;
	ap->argflag |= array;
	lexp->assignok = SH_ASSIGN;
	if(type&NV_ARRAY)
	{
		lexp->noreserv = 1;
		lexp->assignok = 0;
	}
	else
		lexp->aliasok = 2;
	array= (type&NV_ARRAY)?SH_ARRAY:0;
	if((n=skipnl(lexp,0))==RPAREN || n==LPAREN)
	{
		struct argnod *ar,*aq,**settail;
		ac = getnode(comnod);
		memset(ac,0,sizeof(*ac));
	comarray:
		settail= &ac->comset;
		ac->comline = sh_getlineno(lexp);
		while(n==LPAREN)
		{
			ar = stkseek(sh.stk,ARGVAL);
			ar->argflag= ARG_ASSIGN;
			sfprintf(sh.stk,"[%d]=",index++);
			if(aq = ac->comarg.ap)
			{
				ac->comarg.ap = aq->argnxt.ap;
				sfprintf(sh.stk,"%s",aq->argval);
				ar->argflag |= aq->argflag;
			}
			ar = stkfreeze(sh.stk,1);
			ar->argnxt.ap = 0;
			if(!aq)
			{
				ar = assign(lexp,ar,0);
				/* since noreserv is reset to 0 below, we have set it again after a recursive call */
				lexp->noreserv = 1;
			}
			ar->argflag |= ARG_MESSAGE;
			*settail = ar;
			settail = &(ar->argnxt.ap);
			if(aq)
				continue;
			while((n = skipnl(lexp,0))==0)
			{
				ar = stkseek(sh.stk,ARGVAL);
				ar->argflag= ARG_ASSIGN;
				sfprintf(sh.stk,"[%d]=",index++);
				sfputr(sh.stk,lexp->arg->argval,-1);
				ar = stkfreeze(sh.stk,1);
				ar->argnxt.ap = 0;
				ar->argflag = lexp->arg->argflag;
				*settail = ar;
				settail = &(ar->argnxt.ap);
			}
		}
	}
	else if(n && n!=FUNCTSYM)
		sh_syntax(lexp,0);
	else if(type!=NV_ARRAY &&
		n!=FUNCTSYM &&
		!(lexp->arg->argflag&ARG_ASSIGN) &&
		!((np=nv_search(lexp->arg->argval,sh.fun_tree,0)) &&
		(nv_isattr(np,BLT_DCL) || np==SYSDOT || np==SYSSOURCE)))
	{
		array=SH_ARRAY;
		if((n = fcgetc())==LPAREN)
		{
			if(fcgetc()==RPAREN)
			{
				lexp->token =  SYMRES;
				array = 0;
			}
			else
				fcseek(-2);
		}
		else if(n>0)
			fcseek(-1);
		if(array && type==NV_TYPE)
		{
			struct argnod *arg = lexp->arg;
			unsigned save_recursion = dcl_recursion;
			int p;
			/*
			 * Forcibly deactivate the dummy declaration built-ins tree as path_search()
			 * does an FPATH search, which may execute arbitrary code at parse time.
			 */
			n = lexp->token;
			dcl_recursion = 1;
			dcl_dehacktivate();
			p = path_search(lexp->arg->argval,NULL,1);
			dcl_hacktivate();
			dcl_recursion = save_recursion;
			if(p && (np=nv_search(lexp->arg->argval,sh.fun_tree,0)) && nv_isattr(np,BLT_DCL))
			{
				lexp->token = n;
				lexp->arg = arg;
				array = 0;
			}
			else
				sh_syntax(lexp,0);
		}
	}
	lexp->noreserv = 0;
	while(1)
	{
		if((n=lexp->token)==RPAREN)
			break;
		if(n==FUNCTSYM || n==SYMRES)
			ac = (struct comnod*)funct(lexp);
		else
			ac = (struct comnod*)simple(lexp,SH_NOIO|SH_ASSIGN|type|array,NULL);
		if((n=lexp->token)==RPAREN)
			break;
		if(n!=NL && n!=';')
		{
			if(array && n==LPAREN)
				goto comarray;
			sh_syntax(lexp,0);
		}
		lexp->assignok = SH_ASSIGN;
		if((n=skipnl(lexp,0)) || array)
		{
			if(n==RPAREN)
				break;
			if(array ||  n!=FUNCTSYM)
				sh_syntax(lexp,0);
		}
		if((n!=FUNCTSYM) &&
			!(lexp->arg->argflag&ARG_ASSIGN) &&
			!((np=nv_search(lexp->arg->argval,sh.fun_tree,0)) &&
			(nv_isattr(np,BLT_DCL) || np==SYSDOT || np==SYSSOURCE)))
		{
			struct argnod *arg = lexp->arg;
			if(n!=0)
				sh_syntax(lexp,0);
			/* check for SysV style function */
			if(sh_lex(lexp)!=LPAREN || sh_lex(lexp)!=RPAREN)
			{
				lexp->arg = arg;
				lexp->token = 0;
				sh_syntax(lexp,0);
			}
			lexp->arg = arg;
			lexp->token = SYMRES;
		}
		t = makelist(lexp,TLST,(Shnode_t*)ac,t);
		*tp = t;
		tp = &t->lst.lstrit;
	}
	*tp = (Shnode_t*)ac;
	lexp->assignok = 0;
	lexp->assignlevel--;
	return ap;
}

/*
 * item
 *
 *	( cmd ) [ < in ] [ > out ]
 *	word word* [ < in ] [ > out ]
 *	if ... then ... else ... fi
 *	for ... while ... do ... done
 *	case ... in ... esac
 *	begin ... end
 */
static Shnode_t	*item(Lex_t *lexp,int flag)
{
	Shnode_t *t;
	struct ionod *io;
	int tok = (lexp->token&0xff);
	int savwdval = lexp->lasttok;
	int savline = lexp->lastline;
	int showme=0, comsub;
	if(!(flag&SH_NOIO) && (tok=='<' || tok=='>' || lexp->token==IOVNAME))
		io=inout(lexp,NULL,1);
	else
		io=0;
	if((tok=lexp->token) && tok!=EOFSYM && tok!=FUNCTSYM)
	{
		lexp->lastline =  sh_getlineno(lexp);
		lexp->lasttok = lexp->token;
	}
	switch(tok)
	{
	    /* [[ ... ]] test expression */
	    case BTESTSYM:
		t = test_expr(lexp,ETESTSYM);
		t->tre.tretyp &= ~TTEST;
		break;
	    /* ((...)) arithmetic expression */
	    case EXPRSYM:
		t = getanode(lexp,lexp->arg);
		sh_lex(lexp);
		goto done;

	    /* case statement */
	    case CASESYM:
	    {
		int savetok = lexp->lasttok;
		int saveline = lexp->lastline;
		t = getnode(swnod);
		if(sh_lex(lexp))
			sh_syntax(lexp,0);
		t->sw.swarg=lexp->arg;
		t->sw.swtyp=TSW;
		t->sw.swio = 0;
		t->sw.swtyp |= FLINENO;
		t->sw.swline =  sh.inlineno;
		if((tok=skipnl(lexp,0))!=INSYM && tok!=LBRACE)
			sh_syntax(lexp,0);
		if(!(t->sw.swlst=syncase(lexp,tok==INSYM?ESACSYM:RBRACE)) && lexp->token==EOFSYM)
		{
			lexp->lasttok = savetok;
			lexp->lastline = saveline;
			sh_syntax(lexp,0);
		}
		break;
	    }

	    /* if statement */
	    case IFSYM:
	    {
		Shnode_t	*tt;
		t = getnode(ifnod);
		t->if_.iftyp=TIF;
		t->if_.iftre=sh_cmd(lexp,THENSYM,SH_NL);
		t->if_.thtre=sh_cmd(lexp,ELSESYM,SH_NL|SH_SEMI);
		tok = lexp->token;
		t->if_.eltre=(tok==ELSESYM?sh_cmd(lexp,FISYM,SH_NL|SH_SEMI):
			(tok==ELIFSYM?(lexp->token=IFSYM, tt=item(lexp,SH_NOIO)):0));
		if(tok==ELIFSYM)
		{
			if(!tt || tt->tre.tretyp!=TSETIO)
				goto done;
			t->if_.eltre = tt->fork.forktre;
			tt->fork.forktre = t;
			t = tt;
			goto done;
		}
		break;
	    }

	    /* for and select statement */
	    case FORSYM:
	    case SELECTSYM:
	    {
		t = getnode(fornod);
		t->for_.fortyp=(lexp->token==FORSYM?TFOR:TSELECT);
		t->for_.forlst=0;
		t->for_.forline =  sh.inlineno;
		if(sh_lex(lexp))
		{
			if(lexp->token!=EXPRSYM || t->for_.fortyp!=TFOR)
				sh_syntax(lexp,0);
			/* arithmetic for */
			t = arithfor(lexp,t);
			break;
		}
		t->for_.fornam=(char*) lexp->arg->argval;
		t->for_.fortyp |= FLINENO;
#if SHOPT_KIA
		if(kia.file)
			writedefs(lexp,lexp->arg,sh.inlineno,'v',NULL);
#endif /* SHOPT_KIA */
		while((tok=sh_lex(lexp))==NL);
		if(tok==INSYM)
		{
			if(sh_lex(lexp))
			{
				if(lexp->token != NL && lexp->token !=';')
					sh_syntax(lexp,0);
				/* some Linux scripts assume this */
				if(sh_isoption(SH_NOEXEC))
					errormsg(SH_DICT,ERROR_warn(0),e_lexemptyfor,sh.inlineno-(lexp->token=='\n'));
				t->for_.forlst = memset(getnode(comnod),0,sizeof(struct comnod));
			}
			else
				t->for_.forlst=(struct comnod*)simple(lexp,SH_NOIO,NULL);
			if(lexp->token != NL && lexp->token !=';')
				sh_syntax(lexp,0);
			tok = skipnl(lexp,0);
		}
		/* 'for i;do cmd' is valid syntax */
		else if(tok==';')
			while((tok=sh_lex(lexp))==NL);
		if(tok!=DOSYM && tok!=LBRACE)
			sh_syntax(lexp,0);
		t->for_.fortre=sh_cmd(lexp,tok==DOSYM?DONESYM:RBRACE,SH_NL|SH_SEMI);
		break;
	    }

	    /* This is the code for parsing function definitions */
	    case FUNCTSYM:
		return funct(lexp);

#if SHOPT_NAMESPACE
	    case NSPACESYM:
		t = getnode(functnod);
		t->funct.functtyp=TNSPACE;
		t->funct.functargs = 0;
		if(sh_lex(lexp))
			sh_syntax(lexp,0);
		t->funct.functnam=(char*) lexp->arg->argval;
		while((tok=sh_lex(lexp))==NL);
		if(tok!=LBRACE)
			sh_syntax(lexp,0);
		t->funct.functtre = sh_cmd(lexp,RBRACE,SH_NL);
		break;
#endif /* SHOPT_NAMESPACE */

	    /* while and until */
	    case WHILESYM:
	    case UNTILSYM:
		t = getnode(whnod);
		t->wh.whtyp=(lexp->token==WHILESYM ? TWH : TUN);
		t->wh.whtre = sh_cmd(lexp,DOSYM,SH_NL);
		t->wh.dotre = sh_cmd(lexp,DONESYM,SH_NL|SH_SEMI);
		t->wh.whinc = 0;
		break;

	    /* command group with {...} */
	    case LBRACE:
		comsub = lexp->comsub;
		lexp->comsub = 0;
		t = sh_cmd(lexp,RBRACE,SH_NL|SH_SEMI);
		lexp->comsub = comsub;
		break;

	    case LPAREN:
		t = getnode(parnod);
		t->par.partre=sh_cmd(lexp,RPAREN,SH_NL|SH_SEMI);
		t->par.partyp=TPAR;
		break;

	    default:
		if(io==0)
			return NULL;
		/* FALLTHROUGH */
	    case ';':
		if(io==0)
		{
			if(!(flag&SH_SEMI))
				return NULL;
			if(sh_lex(lexp)==';')
				sh_syntax(lexp,0);
			showme =  FSHOWME;
		}
		/* FALLTHROUGH */
	    /* simple command */
	    case 0:
		t = (Shnode_t*)simple(lexp,flag,io);
		if(t->com.comarg.ap && lexp->intypeset)
			check_typedef(&t->com, lexp->intypeset);
		lexp->intypeset = 0;
		lexp->inexec = 0;
		t->tre.tretyp |= showme;
		return t;
	}
	sh_lex(lexp);
done:
	/* redirection(s) following a compound command or arithmetic expression */
	if(io=inout(lexp,io,0))
	{
		t=makeparent(lexp,TSETIO,t);
		t->tre.treio=io;
	}
	lexp->lasttok = savwdval;
	lexp->lastline = savline;
	return t;
}

static struct argnod *process_sub(Lex_t *lexp,int tok)
{
	struct argnod *argp;
	Shnode_t *t;
	int mode = (tok==OPROCSYM);
	t = sh_cmd(lexp,RPAREN,SH_NL);
	argp = stkalloc(sh.stk,sizeof(struct argnod));
	*argp->argval = 0;
	argp->argchn.ap = (struct argnod*)makeparent(lexp,mode?TFORK|FPIN|FAMP|FPCL:TFORK|FPOU,t);
	argp->argflag =  (ARG_EXP|mode);
	return argp;
}


/*
 * This is for a simple command, for list, or compound assignment
 */
static Shnode_t *simple(Lex_t *lexp,int flag, struct ionod *io)
{
	struct comnod	*t;
	struct argnod	*argp, *ap;
	int		tok;
	struct argnod	**argtail;
	struct argnod	**settail;
	int		cmdarg = 0;
	int		type = 0;
	int		was_assign = 0;
	int		argno = 0;
	int		assignment = 0;
	int		key_on = (!(flag&SH_NOIO) && sh_isoption(SH_KEYWORD));
	int		associative=0;
	if((argp=lexp->arg) && (argp->argflag&ARG_ASSIGN) && argp->argval[0]=='[')
	{
		flag |= SH_ARRAY;
		associative = 1;
	}
	t = memset(getnode(comnod),0,sizeof(struct comnod));
	t->comio=io; /* initial io chain */
	/* set command line number for error messages */
	t->comline = sh_getlineno(lexp);
	argtail = &(t->comarg.ap);
	settail = &(t->comset);
	if(lexp->assignlevel && (flag&SH_ARRAY) && check_array(lexp))
		type |= NV_ARRAY;
	while(lexp->token==0)
	{
		was_assign = 0;
		argp = lexp->arg;
		if(*argp->argval==LBRACE && (flag&SH_FUNDEF) && argp->argval[1]==0)
		{
			lexp->token = LBRACE;
			break;
		}
		if(associative && (!(argp->argflag&ARG_ASSIGN) || argp->argval[0]!='['))
			sh_syntax(lexp,0);
		/* check for assignment argument */
		if((argp->argflag&ARG_ASSIGN) && assignment!=2)
		{
			*settail = argp;
			settail = &(argp->argnxt.ap);
			lexp->assignok = (flag&SH_ASSIGN)?SH_ASSIGN:1;
			if(assignment)
			{
				ap=argp;
				if(assignment==1)
				{
					stkseek(sh.stk,ARGVAL);
					sfwrite(sh.stk,argp->argval,lexp->varnamelength);
					ap = stkfreeze(sh.stk,1);
					ap->argflag = ARG_RAW;
					ap->argchn.ap = 0;
				}
				*argtail = ap;
				argtail = &(ap->argnxt.ap);
				if(argno>=0)
					argno++;
			}
			else /* alias substitutions allowed */
				lexp->aliasok = 1;
		}
		else
		{
			if(!(argp->argflag&ARG_RAW))
				argno = -1;
			if(argno>=0 && argno++==cmdarg && !(flag&SH_ARRAY)
			&& !(sh_isoption(SH_RESTRICTED) && strchr(argp->argval,'/')))
			{
				/* check for builtin command (including path-bound builtins executed by full pathname) */
				Namval_t *np=nv_bfsearch(argp->argval,sh.fun_tree, (Namval_t**)&t->comnamq,NULL);
				if(np && is_abuiltin(np))
				{
					if(cmdarg==0)
						t->comnamp = np;
					if(nv_isattr(np,BLT_DCL))
					{
						assignment = 1;
						if(np >= SYSTYPESET && np <= SYSTYPESET_END)
							lexp->intypeset = 1;
						else if(np == SYSENUM)
							lexp->intypeset = 2;
						key_on = 1;
					}
					else if(np==SYSCOMMAND)	/* treat 'command typeset', etc. as declaration command */
						cmdarg++;
					else if(np==SYSEXEC || np==SYSREDIR)
						lexp->inexec = 1;
					else if(funptr(np)==b_getopts)
						opt_get |= FOPTGET;
				}
			}
			if((flag&NV_COMVAR) && !assignment)
				sh_syntax(lexp,0);
			*argtail = argp;
			argtail = &(argp->argnxt.ap);
			if(!(lexp->assignok=key_on)  && !(flag&SH_NOIO) && sh_isoption(SH_NOEXEC))
				lexp->assignok = SH_COMPASSIGN;
			lexp->aliasok = 0;
		}
	retry:
		tok = sh_lex(lexp);
		if(was_assign && check_array(lexp))
			type = NV_ARRAY;
		if(tok==IPROCSYM || tok==OPROCSYM)
		{
	procsub:
			argp = process_sub(lexp,tok);
			argno = -1;
			*argtail = argp;
			argtail = &(argp->argnxt.ap);
			goto retry;
		}
		if(tok==LPAREN)
		{
			if(argp->argflag&ARG_ASSIGN)
			{
				int intypeset = lexp->intypeset;
				lexp->intypeset = 0;
				if(t->comnamp == SYSCOMPOUND)
					type = NV_COMVAR;
				else if((Namval_t*)t->comnamp >= SYSTYPESET && (Namval_t*)t->comnamp <= SYSTYPESET_END)
				{
					for(ap = t->comarg.ap->argnxt.ap; ap; ap = ap->argnxt.ap)
					{
						if(*ap->argval!='-')
							break;
						if(strchr(ap->argval,'T'))
							type |= NV_TYPE;
						else if(strchr(ap->argval,'a'))
							type |= NV_ARRAY;
						else if(strchr(ap->argval,'C'))
							type |= NV_COMVAR;
						else
							continue;
					}
				}
				argp = assign(lexp,argp,type);
				if(type==NV_ARRAY)
					argp->argflag |= ARG_ARRAY;
				lexp->intypeset = intypeset;
				if(lexp->assignlevel)
					was_assign = 1;
				if(associative)
					lexp->assignok |= SH_ASSIGN;
				goto retry;
			}
			else if(argno==1 && !t->comset)
			{
				/* SVR2 style function */
				if(!(flag&SH_ARRAY) && sh_lex(lexp) == RPAREN)
				{
					lexp->arg = argp;
					return funct(lexp);
				}
				lexp->token = LPAREN;
			}
		}
		else if(flag&SH_ASSIGN)
		{
			if(tok==RPAREN)
				break;
			else if(tok==NL && (flag&SH_ARRAY))
			{
				lexp->comp_assign = 2;
				goto retry;
			}
		}
		if(!(flag&SH_NOIO))
		{
			if(io)
			{
				while(io->ionxt)
					io = io->ionxt;
				io->ionxt = inout(lexp,NULL,0);
			}
			else
				t->comio = io = inout(lexp,NULL,0);
			if((tok = lexp->token)==IPROCSYM || tok==OPROCSYM)
				goto procsub;
		}
	}
	*argtail = 0;
	t->comtyp = TCOM;
#if SHOPT_KIA
	if(kia.file && !(flag&SH_NOIO))
	{
		Namval_t *np=(Namval_t*)t->comnamp;
		unsigned long r=0;
		int line = t->comline;
		argp = t->comarg.ap;
		if(np)
			r = kiaentity(lexp,nv_name(np),-1,'p',-1,0,kia.unknown,'b',0,"");
		else if(argp)
			r = kiaentity(lexp,sh_argstr(argp),-1,'p',-1,0,kia.unknown,'c',0,"");
		if(r>0)
			sfprintf(kia.tmp,"p;%..64d;p;%..64d;%d;%d;c;\n",kia.current,r,line,line);
		if(t->comset && argno==0)
			writedefs(lexp,t->comset,line,'v',t->comarg.ap);
		else if(np && nv_isattr(np,BLT_DCL))
			writedefs(lexp,argp,line,0,NULL);
		else if(argp && strcmp(argp->argval,"read")==0)
			writedefs(lexp,argp,line,0,NULL);
		else if(argp && *argp->argval=='.' && argp->argval[1]==0 && (argp=argp->argnxt.ap))
		{
			r = kiaentity(lexp,sh_argstr(argp),-1,'p',0,0,kia.script,'d',0,"");
			sfprintf(kia.tmp,"p;%..64d;p;%..64d;%d;%d;d;\n",kia.current,r,line,line);
		}
	}
#endif /* SHOPT_KIA */
	/* noexec: warn about obsolescent options passed to set and alias */
	if(sh_isoption(SH_NOEXEC) && t->comnamp)
	{
		int lineno = sh.inlineno - (lexp->token == NL);
		if((Namval_t*)t->comnamp==SYSSET)
		{
			char *prev_argval = NULL;
			for(ap = t->comarg.ap->argnxt.ap; ap; ap = ap->argnxt.ap)
			{
				if(strmatch(ap->argval,"(*eyword|*og|*rackall|*iraw)|([+-][^-]*[hkt]*)|([+-][hkt]*)|([+-])"))
				{
					if(strmatch(prev_argval,"[+-]o"))
						errormsg(SH_DICT,ERROR_warn(0),e_lexobsolete7,lineno,prev_argval,ap->argval);
					else
						errormsg(SH_DICT,ERROR_warn(0),e_lexobsolete5,lineno,ap->argval);
				}
				prev_argval = ap->argval;
			}
		}
		else if((Namval_t*)t->comnamp==SYSALIAS)
			for(ap = t->comarg.ap->argnxt.ap; ap; ap = ap->argnxt.ap)
				if(strmatch(ap->argval, "-*x*"))
					errormsg(SH_DICT,ERROR_warn(0),e_lexobsolete8,lineno,ap->argval);
	}
	/* expand argument list if possible */
	if(argno>0 && !(flag&(SH_ARRAY|NV_APPEND)))
		t->comarg.ap = qscan(t,argno);
	else if(t->comarg.ap)
		t->comtyp |= COMSCAN;
	lexp->aliasok = 0;
	return (Shnode_t*)t;
}

/*
 * skip past newlines but issue prompt if interactive
 */
static int	skipnl(Lex_t *lexp,int flag)
{
	int token;
	while((token=sh_lex(lexp))==NL);
	if(token==';' && !(flag&SH_SEMI))
		sh_syntax(lexp,0);
	return token;
}

/*
 * check for and process I/O redirections
 * if flag>0 then an alias can be in the next word
 * if flag<0 only one redirection will be processed
 */
static struct ionod	*inout(Lex_t *lexp,struct ionod *lastio,int flag)
{
	int 		iof = lexp->digits, token=lexp->token;
	struct ionod	*iop;
	char		*iovname=0;
	int		errout=0;
	/* return if a process substitution is found without a redirection */
	if(token==IPROCSYM || token==OPROCSYM)
		return lastio;
	if(token==IOVNAME)
	{
		iovname=lexp->arg->argval+1;
		token= sh_lex(lexp);
		iof = 0;
	}
	switch(token&0xff)
	{
	    case '<':
		if(token==IODOCSYM)
			iof |= (IODOC|IORAW);
		else if(token==IOMOV0SYM)
			iof |= IOMOV;
		else if(token==IORDWRSYMT)
			iof |= IORDW|IOREWRITE;
		else if(token==IORDWRSYM)
			iof |= IORDW;
		else if((token&SYMSHARP) == SYMSHARP)
		{
			int n;
			iof |= IOLSEEK;
			if((n = fcgetc())=='#')
				iof |= IOCOPY;
			else if(n>0)
				fcseek(-1);
		}
		else if((token&UCHAR_MAX)!=token)  /* unhandled SYM* bits */
			sh_syntax(lexp,0);
		break;

	    case '>':
		if(iof<0)
		{
			errout = 1;
			iof = 1;
		}
		iof |= IOPUT;
		if(token==IOAPPSYM)
			iof |= IOAPP;
		else if(token==IOMOV1SYM)
			iof |= IOMOV;
		else if(token==IOCLOBSYM)
			iof |= IOCLOB;
		else if((token&SYMSHARP) == SYMSHARP)
			iof |= IOLSEEK;
		else if((token&SYMSEMI) == SYMSEMI)
			iof |= IOREWRITE;
		else if((token&UCHAR_MAX)!=token)  /* unhandled SYM* bits */
			sh_syntax(lexp,0);
		break;

	    default:
		return lastio;
	}
	lexp->digits=0;
	iop = stkalloc(sh.stk,sizeof(struct ionod));
	iop->iodelim = 0;
	if(token=sh_lex(lexp))
	{
		if(token==RPAREN && (iof&IOLSEEK) && lexp->comsub)
		{
			lexp->arg = stkalloc(sh.stk,sizeof(struct argnod)+3);
			strcpy(lexp->arg->argval,"CUR");
			lexp->arg->argflag = ARG_RAW;
			iof |= IOARITH;
			fcseek(-1);
		}
		else if(token==EXPRSYM && (iof&IOLSEEK))
			iof |= IOARITH;
		else if(((token==IPROCSYM && !(iof&IOPUT)) || (token==OPROCSYM && (iof&IOPUT))) && !(iof&(IOLSEEK|IOREWRITE|IOMOV|IODOC)))
		{
			lexp->arg = process_sub(lexp,token);
			iof |= IOPROCSUB;
		}
		else
			sh_syntax(lexp,0);
	}
	if( (iof&IOPROCSUB) && !(iof&IOLSEEK))
		iop->ioname= (char*)lexp->arg->argchn.ap;
	else
		iop->ioname=lexp->arg->argval;
	iop->iovname = iovname;
	if(iof&IODOC)
	{
		if(lexp->digits==2)
		{
			iof |= IOSTRG;
			if(!(lexp->arg->argflag&ARG_RAW))
				iof &= ~IORAW;
		}
		else
		{
			if(!sh.heredocs)
				sh.heredocs = sftmp(HERE_MEM);
			iop->iolst=lexp->heredoc;
			lexp->heredoc=iop;
			if(lexp->arg->argflag&ARG_QUOTED)
				iof |= IOQUOTE;
			if(lexp->digits==3)
				iof |= IOLSEEK;
			if(lexp->digits)
				iof |= IOSTRIP;
		}
	}
	else
	{
		iop->iolst = 0;
		if(lexp->arg->argflag&ARG_RAW)
			iof |= IORAW;
	}
	iop->iofile=iof;
	if(flag>0)
		/* allow alias substitutions and parameter assignments */
		lexp->aliasok = lexp->assignok = 1;
#if SHOPT_KIA
	if(kia.file)
	{
		int n = sh.inlineno-(lexp->token=='\n');
		if(!(iof&IOMOV))
		{
			unsigned long r=kiaentity(lexp,(iof&IORAW)?sh_fmtq(iop->ioname):iop->ioname,-1,'f',0,0,kia.script,'f',0,"");
			sfprintf(kia.tmp,"p;%..64d;f;%..64d;%d;%d;%c;%d\n",kia.current,r,n,n,(iof&IOPUT)?((iof&IOAPP)?'a':'w'):((iof&IODOC)?'h':'r'),iof&IOUFD);
		}
	}
#endif /* SHOPT_KIA */
	if(flag>=0)
	{
		struct ionod *ioq=iop;
		sh_lex(lexp);
		if(errout)
		{
			/* redirect standard output to standard error */
			ioq = stkalloc(sh.stk,sizeof(struct ionod));
			memset(ioq,0,sizeof(*ioq));
			ioq->ioname = "1";
			ioq->iolst = 0;
			ioq->iodelim = 0;
			ioq->iofile = IORAW|IOPUT|IOMOV|2;
			iop->ionxt=ioq;
		}
		ioq->ionxt=inout(lexp,lastio,flag);
	}
	else
		iop->ionxt=0;
	return iop;
}

/*
 * convert argument chain to argument list when no special arguments
 */
static struct argnod *qscan(struct comnod *ac,int argn)
{
	char **cp;
	struct argnod *ap;
	struct dolnod* dp;
	int special=0;
	/*
	 * The 'special' variable flags a parser hack for ancient 'test -t' compatibility.
	 * As this is done at parse time, it only affects literal '-t', not 'foo=-t; test "$foo"'.
	 * It only works for a simple '-t'; a compound expression ending in '-t' is hacked in bltins/test.c.
	 */
	if(!sh_isoption(SH_POSIX))
	{
		if((Namval_t*)ac->comnamp==SYSTEST)
			special = 2;	/* convert "test -t" to "test -t 1" */
		else if(*(ac->comarg.ap->argval)=='[' && ac->comarg.ap->argval[1]==0)
			special = 3;	/* convert "[ -t ]" to "[ -t 1 ]" */
	}
	if(special)
	{
		ap = ac->comarg.ap->argnxt.ap;
		if(argn==(special+1) && ap->argval[1]==0 && *ap->argval=='!')
			ap = ap->argnxt.ap;
		else if(argn!=special)
			special=0;
	}
	if(special)
	{
		const char *message;
		if(strcmp(ap->argval,"-t"))
		{
			message = "line %d: Invariant test";
			special=0;
		}
		else
		{
			message = "line %d: -t requires argument";
			argn++;
		}
		if(sh_isoption(SH_NOEXEC))
			errormsg(SH_DICT,ERROR_warn(0),message,ac->comline);
	}
	/* leave space for an extra argument at the front */
	dp = stkalloc(sh.stk,(unsigned)sizeof(struct dolnod) + ARG_SPARE*sizeof(char*) + argn*sizeof(char*));
	cp = dp->dolval+ARG_SPARE;
	dp->dolnum = argn;
	dp->dolbot = ARG_SPARE;
	ap = ac->comarg.ap;
	while(ap)
	{
		*cp++ = ap->argval;
		ap = ap->argnxt.ap;
	}
	if(special==3)
	{
		cp[0] = cp[-1];
		cp[-1] = "1";
		cp++;
	}
	else if(special)
		*cp++ = "1";
	*cp = 0;
	return (struct argnod*)dp;
}

static Shnode_t *test_expr(Lex_t *lp,int sym)
{
	Shnode_t *t = test_or(lp);
	if(lp->token!=sym)
		sh_syntax(lp,0);
	return t;
}

static Shnode_t *test_or(Lex_t *lp)
{
	Shnode_t *t = test_and(lp);
	while(lp->token==ORFSYM)
		t = makelist(lp,TORF|TTEST,t,test_and(lp));
	return t;
}

static Shnode_t *test_and(Lex_t *lp)
{
	Shnode_t *t = test_primary(lp);
	while(lp->token==ANDFSYM)
		t = makelist(lp,TAND|TTEST,t,test_primary(lp));
	return t;
}

/*
 * convert =~ into == ~(E)
 */
static void ere_match(void)
{
	Sfio_t *base, *iop = sfopen(NULL," ~(E)","s");
	int c;
	while(c = fcgetc(), c==' ' || c=='\t');
	if(c)
		fcseek(-1);
	if(!(base=fcfile()))
		base = sfopen(NULL,fcseek(0),"s");
	fcclose();
	sfstack(base,iop);
	fcfopen(base);
}

static Shnode_t *test_primary(Lex_t *lexp)
{
	struct argnod *arg;
	Shnode_t *t;
	int num,token;
	token = skipnl(lexp,0);
	num = lexp->digits;
	switch(token)
	{
	    case '(':
		t = test_expr(lexp,')');
		t = makelist(lexp,TTST|TTEST|TPAREN ,t, (Shnode_t*)pointerof(sh.inlineno));
		break;
	    case '!':
		if(!(t = test_primary(lexp)))
			sh_syntax(lexp,0);
		t->tre.tretyp ^= TNEGATE;  /* xor it, so that a '!' negates another '!' */
		return t;
	    case TESTUNOP:
		if(sh_lex(lexp))
			sh_syntax(lexp,0);
#if SHOPT_KIA
		if(kia.file && !strchr("sntzoOG",num))
		{
			int line = sh.inlineno- (lexp->token==NL);
			unsigned long r;
			r=kiaentity(lexp,sh_argstr(lexp->arg),-1,'f',0,0,kia.script,'t',0,"");
			sfprintf(kia.tmp,"p;%..64d;f;%..64d;%d;%d;t;\n",kia.current,r,line,line);
		}
#endif /* SHOPT_KIA */
		t = makelist(lexp,TTST|TTEST|TUNARY|(num<<TSHIFT),
			(Shnode_t*)lexp->arg,(Shnode_t*)lexp->arg);
		t->tst.tstline =  sh.inlineno;
		break;
	    /* binary test operators */
	    case 0:
		arg = lexp->arg;
		if((token=sh_lex(lexp))==TESTBINOP)
		{
			num = lexp->digits;
			if(num==TEST_REP)
			{
				ere_match();
				num = TEST_PEQ;
			}
		}
		else if(token=='<')
			num = TEST_SLT;
		else if(token=='>')
			num = TEST_SGT;
		else if(token==ANDFSYM||token==ORFSYM||token==ETESTSYM||token==RPAREN)
		{
			t = makelist(lexp,TTST|TTEST|TUNARY|('n'<<TSHIFT),
				(Shnode_t*)arg,(Shnode_t*)arg);
			t->tst.tstline =  sh.inlineno;
			return t;
		}
		else
			sh_syntax(lexp,0);
#if SHOPT_KIA
		if(kia.file && (num==TEST_EF||num==TEST_NT||num==TEST_OT))
		{
			int line = sh.inlineno- (lexp->token==NL);
			unsigned long r;
			r=kiaentity(lexp,sh_argstr(lexp->arg),-1,'f',0,0,kia.current,'t',0,"");
			sfprintf(kia.tmp,"p;%..64d;f;%..64d;%d;%d;t;\n",kia.current,r,line,line);
		}
#endif /* SHOPT_KIA */
		if(sh_lex(lexp))
			sh_syntax(lexp,0);
		if(num&TEST_STRCMP)
		{
			/* If the argument is unquoted, enable pattern matching */
			if(lexp->arg->argflag&(ARG_EXP|ARG_MAC))
				num &= ~TEST_STRCMP;
		}
		t = getnode(tstnod);
		t->lst.lsttyp = TTST|TTEST|TBINARY|(num<<TSHIFT);
		t->lst.lstlef = (Shnode_t*)arg;
		t->lst.lstrit = (Shnode_t*)lexp->arg;
		t->tst.tstline =  sh.inlineno;
#if SHOPT_KIA
		if(kia.file && (num==TEST_EF||num==TEST_NT||num==TEST_OT))
		{
			int line = sh.inlineno-(lexp->token==NL);
			unsigned long r;
			r=kiaentity(lexp,sh_argstr(lexp->arg),-1,'f',0,0,kia.current,'t',0,"");
			sfprintf(kia.tmp,"p;%..64d;f;%..64d;%d;%d;t;\n",kia.current,r,line,line);
		}
#endif /* SHOPT_KIA */
		break;
	    default:
		return NULL;
	}
	skipnl(lexp,0);
	return t;
}

#if SHOPT_KIA
/*
 * ksh is currently compiled with -D_API_ast=20100309, which sets CDT_VERSION to 20100309 in src/lib/libast/include/cdt.h
 * which enables a legacy Cdt API also used elsewhere in ksh. Do not remove this version check as it is not yet obsolete.
 */
#if CDT_VERSION < 20111111L
#define hash	nvlink.hl._hash
#else
#define hash	nvlink.lh.__hash
#endif
/*
 * return an entity checksum
 * The entity is created if it doesn't exist
 */
unsigned long kiaentity(Lex_t *lexp,const char *name,int len,int type,int first,int last,unsigned long parent, int pkind, int width, const char *attr)
{
	Namval_t *np;
	long offset = stktell(sh.stk);
	sfputc(sh.stk,type);
	if(len>0)
		sfwrite(sh.stk,name,len);
	else
	{
		if(type=='p')
			sfputr(sh.stk,path_basename(name),0);
		else
			sfputr(sh.stk,name,0);
	}
	sfputc(sh.stk,'\0');  /* terminate name while writing database output */
	np = nv_search(stkptr(sh.stk,offset),kia.entity_tree,NV_ADD);
	stkseek(sh.stk,offset);
	np->nvalue = sh_malloc(sizeof(int));
	*((int*)np->nvalue) = pkind;
	nv_setsize(np,width);
	if(!nv_isattr(np,NV_TAGGED) && first>=0)
	{
		nv_onattr(np,NV_TAGGED);
		if(!pkind)
			pkind = '0';
		if(len>0)
			sfprintf(kia.file,"%..64d;%c;%.*s;%d;%d;%..64d;%..64d;%c;%d;%s\n",np->hash,type,len,name,first,last,parent,kia.fscript,pkind,width,attr);
		else
			sfprintf(kia.file,"%..64d;%c;%s;%d;%d;%..64d;%..64d;%c;%d;%s\n",np->hash,type,name,first,last,parent,kia.fscript,pkind,width,attr);
	}
	return np->hash;
}
#undef hash

static void kia_add(Namval_t *np, void *data)
{
	char *name = nv_name(np);
	Lex_t	*lp = (Lex_t*)data;
	NOT_USED(data);
	kiaentity(lp,name+1,-1,*name,0,-1,(*name=='p'?kia.unknown:kia.script),*((int*)np->nvalue),nv_size(np),"");
}

int kiaclose(Lex_t *lexp)
{
	off_t off1,off2;
	int n;
	if(kia.file)
	{
		unsigned long r = kiaentity(lexp,kia.scriptname,-1,'p',-1,sh.inlineno-1,0,'s',0,"");
		kiaentity(lexp,kia.scriptname,-1,'p',1,sh.inlineno-1,r,'s',0,"");
		kiaentity(lexp,kia.scriptname,-1,'f',1,sh.inlineno-1,r,'s',0,"");
		nv_scan(kia.entity_tree,kia_add,lexp,NV_TAGGED,0);
		off1 = sfseek(kia.file,0,SEEK_END);
		sfseek(kia.tmp,0,SEEK_SET);
		sfmove(kia.tmp,kia.file,SFIO_UNBOUND,-1);
		off2 = sfseek(kia.file,0,SEEK_END);
		if(off2==off1)
			n= sfprintf(kia.file,"DIRECTORY\nENTITY;%jd;%zu\nDIRECTORY;",(Sflong_t)kia.begin,(size_t)(off1-kia.begin));
		else
			n= sfprintf(kia.file,"DIRECTORY\nENTITY;%jd;%zu\nRELATIONSHIP;%jd;%zu\nDIRECTORY;",(Sflong_t)kia.begin,(size_t)(off1-kia.begin),(Sflong_t)off1,(size_t)(off2-off1));
		if(off2 >= INT_MAX)
			off2 = -(n+12);
		sfprintf(kia.file,"%010.10jd;%010d\n",(Sflong_t)off2+10, n+12);
	}
	return sfclose(kia.file);
}
#endif /* SHOPT_KIA */

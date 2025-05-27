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
*         hyenias <58673227+hyenias@users.noreply.github.com>          *
*                   Marc Wilson <posguy99@gmail.com>                   *
*                      Phi <phi.debian@gmail.com>                      *
*               K. Eugene Carlson <kvngncrlsn@gmail.com>               *
*                                                                      *
***********************************************************************/
/*
 * Shell macro expander
 * expands ~
 * expands ${...}
 * expands $(...)
 * expands $((...))
 * expands `...`
 *
 *   David Korn
 *   AT&T Labs
 *
 */

#include	"shopt.h"
#include	"defs.h"
#include	<fcin.h>
#include	<pwd.h>
#include	<ctype.h>
#include	<regex.h>
#include	"name.h"
#include	"variables.h"
#include	"shlex.h"
#include	"io.h"
#include	"jobs.h"
#include	"shnodes.h"
#include	"path.h"
#include	"national.h"
#include	"streval.h"

#if _WINIX
    static int Skip;
#endif /* _WINIX */

static int	_c_;
typedef struct  _mac_
{
	Sfio_t		*sp;		/* stream pointer for here-document */
	struct argnod	**arghead;	/* address of head of argument list */
	char		*ifsp;		/* pointer to IFS value */
	int		fields;		/* number of fields */
	short		quoted;		/* set when word has quotes */
	short		atspecial;	/* quoted count at which special-casing for "$@" with no PPs has been applied */
	unsigned char	ifs;		/* first byte of IFS */
	char		atmode;		/* when processing $@ */
	char		quote;		/* set within double quoted contexts */
	char		lit;		/* set within single quotes */
	char		split;		/* set when word splitting is possible */
	char		pattern;	/* set when glob pattern expansion or matching follows */
	char		patfound;	/* set if pattern character found */
	char		noextpat;	/* set to disallow extended patterns */
	char		wasexpan;	/* set when word resulted from a $ or ` expansion */
	char		assign;		/* set for assignments */
	char		arith;		/* set for ((...)) */
	char		arrayok;	/* $x[] ok for arrays */
	char		subcopy;	/* set when copying subscript */
	char		macsub;		/* set to 1 when running mac_substitute */
	int		dotdot;		/* set for .. in subscript */
	void		*nvwalk;	/* for name space walking */
} Mac_t;

#undef ESCAPE
#define ESCAPE		'\\'
#define isescchar(s)	((s)>S_QUOTE)
#define isqescchar(s)	((s)>=S_QUOTE)
#define isbracechar(c)	((c)==RBRACE || (_c_=sh_lexstates[ST_BRACE][c])==S_MOD1 ||_c_==S_MOD2)
#define ltos(x)		fmtint(x,0)

/* type of macro expansions */
#define M_BRACE		1	/* ${var}	*/
#define M_TREE		2	/* ${var.}	*/
#define M_SIZE		3	/* ${#var}	*/
#define M_VNAME		4	/* ${!var}	*/
#define M_SUBNAME	5	/* ${!var[sub]}	*/
#define M_NAMESCAN	6	/* ${!var*}	*/
#define M_NAMECOUNT	7	/* ${#var*}	*/
#define M_TYPE		8	/* ${@var}	*/

static noreturn void	mac_error(void);
static int	substring(const char*, size_t, const char*, int[], int);
static void	copyto(Mac_t*, int, int);
static void	comsubst(Mac_t*, Shnode_t*, int);
static int	varsub(Mac_t*);
static void	mac_copy(Mac_t*,const char*, int);
static void	tilde_expand2(int);
static char 	*sh_tilde(const char*);
static char	*special(int);
static void	endfield(Mac_t*,int);
static char	*mac_getstring(char*);
static int	charlen(const char*,int);
#if SHOPT_MULTIBYTE
#   define lastchar(string,endstring)  (mbwide() ? _lastchar(string,endstring) : (endstring))
    static char	*_lastchar(const char*,const char*);
#else
#   define lastchar(string,endstring)  (endstring)
#endif /* SHOPT_MULTIBYTE */

void *sh_macopen(void)
{
	return sh_newof(0,Mac_t,1,0);
}

/*
 * perform only parameter substitution and catch failures
 * (also save lexer state to allow use while in here-docs)
 */
char *sh_mactry(char *string)
{
	if(string)
	{
		int		jmp_val;
		int		savexit = sh.savexit;
		struct checkpt	buff;
		Lex_t		*lexp = (Lex_t*)sh.lex_context, savelex = *lexp;
		sh_pushcontext(&buff,SH_JMPSUB);
		jmp_val = sigsetjmp(buff.buff,0);
		if(jmp_val == 0)
			string = sh_mactrim(string,0);
		sh_popcontext(&buff);
		*lexp = savelex;
		sh.savexit = savexit;
		return string;
	}
	return "";
}

/*
 * Read $IFS and initialize:
 * - ifsp: pointer to value of IFS (NULL if IFS is unset)
 * - ifs: first byte of value of IFS (0 if empty, space if IFS is unset)
 */
static void setup_ifs(Mac_t *mp)
{
	if(mp->ifsp = nv_getval(sh_scoped(IFSNOD)))
		mp->ifs = *mp->ifsp;
	else
		mp->ifs = ' ';
}

/*
 * Perform parameter expansion, command substitution, and arithmetic
 * expansion on <str>.
 * If <mode> greater than 1 file expansion is performed if the result
 * yields a single pathname.
 * If <mode> negative, then expansion rules for assignment are applied.
 */
char *sh_mactrim(char *str, int mode)
{
	Mac_t	*mp = (Mac_t*)sh.mac_context;
	Stk_t	*stkp = sh.stk;
	Mac_t	savemac = *mp;
	stkseek(stkp,0);
	mp->arith = (mode==3);
	nv_setoptimize(NULL);
	mp->pattern = (mode==1||mode==2);
	mp->patfound = 0;
	mp->assign = 0;
	if(mode<0)
		mp->assign = -mode;
	mp->quoted = mp->lit = mp->split = mp->quote = 0;
	mp->sp = 0;
	setup_ifs(mp);
	stkseek(stkp,0);
	fcsopen(str);
	copyto(mp,0,mp->arith);
	str = stkfreeze(stkp,1);
	if(mode==2)
	{
		/* expand only if unique */
		struct argnod *arglist=0;
		if((mode=path_expand(str,&arglist,0))==1)
			str = arglist->argval;
		else if(mode>1)
		{
			errormsg(SH_DICT,ERROR_exit(1),e_ambiguous,str);
			UNREACHABLE();
		}
		sh_trim(str);
	}
	*mp = savemac;
	return str;
}

/*
 * Perform all the expansions on the argument <argp>
 */
int sh_macexpand(struct argnod *argp, struct argnod **arghead,int flag)
{
	int	flags = argp->argflag;
	char	*str = argp->argval;
	Mac_t	*mp = (Mac_t*)sh.mac_context;
	char	**saveoptimize = nv_getoptimize();
	Mac_t	savemac = *mp;
	Stk_t	*stkp = sh.stk;
	mp->sp = 0;
	setup_ifs(mp);
	if((flag&ARG_OPTIMIZE) && !sh.indebug && !(flags&ARG_MESSAGE))
		nv_setoptimize((char**)&argp->argchn.ap);
	else
		nv_setoptimize(NULL);
	mp->arghead = arghead;
	mp->quoted = mp->lit = mp->quote = 0;
	mp->arith = ((flag&ARG_ARITH)!=0);
	mp->split = !(flag&ARG_ASSIGN);
	mp->assign = !mp->split;
	mp->pattern = mp->split && !(flag&ARG_NOGLOB) && (!sh_isoption(SH_NOGLOB) || sh_isstate(SH_COMPLETE) || sh_isstate(SH_FCOMPLETE));
	mp->arrayok = mp->arith || (flag&ARG_ARRAYOK);
	str = argp->argval;
	fcsopen(str);
	mp->fields = 0;
	mp->atmode = 0;
	if(!arghead)
	{
		mp->split = 0;
		mp->pattern = ((flag&ARG_EXP)!=0);
		stkseek(stkp,0);
	}
	else
	{
		stkseek(stkp,ARGVAL);
		*stkptr(stkp,ARGVAL-1) = 0;
	}
	mp->patfound = 0;
	if(mp->pattern)
		mp->arrayok = 0;
	copyto(mp,0,mp->arith);
	if(!arghead)
	{
		argp->argchn.cp = stkfreeze(stkp,1);
		if(nv_getoptimize())
			argp->argflag |= ARG_MAKE;
	}
	else
	{
		endfield(mp,mp->quoted|mp->atmode);
		flags = mp->fields;
		if(flags==1 && nv_getoptimize())
			argp->argchn.ap = *arghead;
	}
	nv_setoptimize(saveoptimize);
	*mp = savemac;
	return flags;
}

/*
 * Expand here document which is stored in <infile> or <string>
 * The result is written to <outfile>
 */
void sh_machere(Sfio_t *infile, Sfio_t *outfile, char *string)
{
	int		c,n;
	const char	*state = sh_lexstates[ST_QUOTE];
	char		*cp;
	Mac_t		*mp = (Mac_t*)sh.mac_context;
	Lex_t		*lp = (Lex_t*)sh.lex_context;
	Fcin_t		save;
	Mac_t		savemac = *mp;
	Stk_t		*stkp = sh.stk;
	stkseek(stkp,0);
	nv_setoptimize(NULL);
	mp->sp = outfile;
	mp->split = mp->assign = mp->pattern = mp->patfound = mp->lit = mp->arith = 0;
	mp->quote = 1;
	setup_ifs(mp);
	fcsave(&save);
	if(infile)
		fcfopen(infile);
	else
		fcsopen(string);
	fcnotify(0,lp);
	cp = fcseek(0);
	while(1)
	{
		if(mbwide())
		{
			do
			{
				ssize_t len;
				switch(len = mbsize(cp))
				{
				    case -1:	/* illegal multi-byte char */
				    case 0:
				    case 1:
					n=state[*(unsigned char*)cp++];
					break;
				    default:
					/* use state of alpha character */
					n=state['a'];
					cp += len;
				}
			}
			while(n == 0);
		}
		else
			while((n=state[*(unsigned char*)cp++])==0)
				;
		if(n==S_NL || n==S_QUOTE || n==S_RBRA)
			continue;
		if(c=(cp-1)-fcseek(0))
			sfwrite(outfile,fcseek(0),c);
		cp = fcseek(c+1);
		switch(n)
		{
		    case S_EOF:
			if((n=fcfill()) <=0)
			{
				/* ignore 0 byte when reading from file */
				if(n==0 && fcfile())
					continue;
				fcrestore(&save);
				*mp = savemac;
				return;
			}
			cp = fcseek(-1);
			continue;
		    case S_ESC:
			c = fcgetc();
			cp=fcseek(-1);
			if(c>0)
				cp++;
			if(!isescchar(state[c]))
				sfputc(outfile,ESCAPE);
			continue;
		    case S_GRAVE:
			comsubst(mp,NULL,0);
			break;
		    case S_DOL:
			c = fcget();
			if(c=='.')
				goto regular;
		    again:
			switch(n=sh_lexstates[ST_DOL][c])
			{
			    case S_ALP: case S_SPC1: case S_SPC2:
			    case S_DIG: case S_LBRA:
			    {
				Fcin_t	save2;
				int	offset = stktell(stkp);
				int	offset2;
				fcnotify(0,lp);
				sfputc(stkp,c);
				if(n==S_LBRA)
				{
					c = fcpeek(0);
					if(sh_lexstates[ST_NORM][c]==S_BREAK)
					{
						comsubst(mp,NULL,2);
						break;
					}
					sh_lexskip(lp,RBRACE,1,ST_BRACE);
				}
				else if(n==S_ALP)
				{
					while(c = fcgetc(), isaname(c))
						sfputc(stkp,c);
					fcseek(-1);
				}
				sfputc(stkp,0);
				offset2 = stktell(stkp);
				fcsave(&save2);
				fcsopen(stkptr(stkp,offset));
				varsub(mp);
				if(c=stktell(stkp)-offset2)
					sfwrite(outfile,(char*)stkptr(stkp,offset2),c);
				fcrestore(&save2);
				stkseek(stkp,offset);
				break;
			    }
			    case S_PAR:
				/* Execute a command substitution or arithmetic expression from a here-document */
				comsubst(mp,NULL,1);
				/* Because comsubs and here-documents both do lexical analysis at runtime, lp->lexd.first
				 * may become invalid if these are nested, which may crash lex_advance(), triggered by
				 * fcfill() below. To avoid that, reset it (lex.c handles this correctly). */
				lp->lexd.first = NULL;
				break;
			    case S_EOF:
				if((c=fcfill()) > 0)
					goto again;
				/* FALLTHROUGH */
			    default:
			    regular:
				sfputc(outfile,'$');
				fcseek(-1);
				break;
			}
		}
		cp = fcseek(0);
	}
}

/*
 * expand argument but do not trim pattern characters
 */
char *sh_macpat(struct argnod *arg, int flags)
{
	char *sp = arg->argval;
	if((arg->argflag&ARG_RAW))
		return sp;
	sh_stats(STAT_ARGEXPAND);
	if(flags&ARG_OPTIMIZE)
		arg->argchn.ap=0;
	if(!(sp=arg->argchn.cp))
	{
		sh_macexpand(arg,NULL,flags|ARG_ARRAYOK);
		sp = arg->argchn.cp;
		if(!(flags&ARG_OPTIMIZE) || !(arg->argflag&ARG_MAKE))
			arg->argchn.cp = 0;
		arg->argflag &= ~ARG_MAKE;
	}
	else
		sh_stats(STAT_ARGHITS);
	return sp;
}

/*
 * Process the characters up to <endch> or end of input string
 */
static void copyto(Mac_t *mp,int endch, int newquote)
{
	int		c,n;
	const char	*state = sh_lexstates[ST_MACRO];
	char		*cp,*first;
	Lex_t		*lp = (Lex_t*)sh.lex_context;
	int		tilde = -1;		/* offset for tilde expansion */
	int		dotdot = 0;		/* offset for '..' in subscript */
	int		paren = 0;		/* level of (parentheses) */
	int		brace = 0;		/* level of {braces} */
	char		oldquote = mp->quote;	/* save "double quoted" state */
	char		ansi_c = 0;		/* set when processing ANSI C escape codes */
	int32_t		ere = 0;		/* bitmask of pattern options indicating an extended regular expression */
	char		bracketexpr = 0; 	/* set when in [brackets] within a non-ERE glob pattern */
	Sfio_t		*sp = mp->sp;
	Stk_t		*stkp = sh.stk;
	char		*resume = 0;
	mp->sp = NULL;
	mp->quote = newquote;
	first = cp = fcseek(0);
	if(!mp->quote && *cp=='~' && cp[1]!=LPAREN && !sh_isstate(SH_NOTILDEXP))
		tilde = stktell(stkp);
	/* handle // operator specially */
	if(mp->pattern==2 && *cp=='/')
		cp++;
	while(1)
	{
		if(mbwide())
		{
			ssize_t len;
			do
			{
				switch(len = mbsize(cp))
				{
				    case -1:	/* illegal multi-byte char */
				    case 0:
					len = 1;
					/* FALLTHROUGH */
				    case 1:
					n = state[*(unsigned char*)cp++];
					break;
				    default:
					/* treat as if alpha */
					cp += len;
					n=state['a'];
				}
			}
			while(n == 0);
			c = (cp-len) - first;
		}
		else
		{
			while((n=state[*(unsigned char*)cp++])==0);
			c = (cp-1) - first;
		}
		switch(n)
		{
		    case S_ESC:
			if(ansi_c)
			{
				/* process ANSI C escape character */
				char *addr= --cp;
				if(c)
					sfwrite(stkp,first,c);
				c = chresc(cp,&addr);
				cp = addr;
				first = fcseek(cp-first);
				if(mbwide() && c > UCHAR_MAX)
				{
					int		i;
					unsigned char	mb[8];

					n = mbconv((char*)mb, c);
					for(i=0;i<n;i++)
						sfputc(stkp,mb[i]);
				}
				else
					sfputc(stkp,c);
				if(c==ESCAPE && mp->pattern)
					sfputc(stkp,ESCAPE);
				break;
			}
#if SHOPT_BRACEPAT
			else if(sh_isoption(SH_BRACEEXPAND) && mp->pattern==4 && (*cp==',' || *cp==LBRACE || *cp==RBRACE || *cp=='.'))
				break;
#endif
			else if(mp->split && endch && !mp->quote && !mp->lit)
			{
				if(c)
					mac_copy(mp,first,c);
				cp = fcseek(c+2);
				if(c= cp[-1])
				{
					sfputc(stkp,c);
					if(c==ESCAPE)
						sfputc(stkp,ESCAPE);
				}
				else
					cp--;
				first = cp;
				break;
			}
			n = state[*(unsigned char*)cp];
			if(n==S_ENDCH && *cp!=endch)
				n = S_PAT;
			if(mp->pattern)
			{
				/* preserve \ for escaping glob pattern bracket expression operators */
				if(bracketexpr && n==S_BRAOP)
					break;
				/* preserve \digit for pattern matching */
				/* also \alpha for extended patterns */
				if(!mp->lit && !mp->quote)
				{
					int nc = *(unsigned char*)cp;
					if(n==S_DIG || ((paren || ere) && sh_lexstates[ST_DOL][nc]==S_ALP || nc=='<' || nc=='>'))
						break;
					if(ere && mp->pattern==1 && strchr(".[()*+?{|^$&!",*cp))
						break;
				}
				/* followed by file expansion */
				if(!mp->lit && (n==S_ESC || (!mp->quote &&
					(n==S_PAT||n==S_ENDCH||n==S_SLASH||n==S_BRACT||*cp=='-'))))
				{
					cp += (n!=S_EOF);
					if(ere && n==S_ESC && *cp =='\\' && cp[1]=='$')
					{
						/* convert \\\$ into \$' */
						sfwrite(stkp,first,c+1);
						cp = first = fcseek(c+3);
					}
					break;
				}
				if(!(ere && *cp=='$') && (mp->lit || (mp->quote && !isqescchar(n) && n!=S_ENDCH)))
				{
					/* add \ for file expansion */
					sfwrite(stkp,first,c+1);
					first = fcseek(c);
					break;
				}
			}
			if(mp->lit)
				break;
			if(!mp->quote || isqescchar(n) || n==S_ENDCH)
			{
				/* eliminate \ */
				if(c)
					sfwrite(stkp,first,c);
				/* check new-line joining */
				first = fcseek(c+1);
			}
			cp += (n!=S_EOF);
			break;
		    case S_GRAVE: case S_DOL:
			if(mp->lit)
				break;
			if(c)
			{
				if(mp->split && !mp->quote && endch)
					mac_copy(mp,first,c);
				else
					sfwrite(stkp,first,c);
			}
			first = fcseek(c+1);
			c = mp->pattern;
			if(n==S_GRAVE)
				comsubst(mp,NULL,0);
			else if((n= *cp) == '"' && !mp->quote)
			{
				int off = stktell(stkp);
				char	*dp;
				cp = first = fcseek(1);
				mp->quote = 1;
				if(!ERROR_translating())
					break;
				while(n=c, c= *++cp)
				{
					if(c=='\\' && n==c)
						c = 0;
					else if(c=='"' && n!='\\')
						break;
				}
				n = cp-first;
				sfwrite(stkp,first,n);
				sfputc(stkp,0);
				cp = stkptr(stkp,off);
				dp = (char*)sh_translate(cp);
				stkseek(stkp,off);
				if(dp==cp)
				{
					cp = first;
					break;
				}
				resume = fcseek(n);
				fcclose();
				fcsopen(dp);
				cp = first = fcseek(0);
				break;
			}
			else if(n==0 || !varsub(mp))
			{
				if(n=='\'' && !mp->quote)
					ansi_c = 1;
				else if(mp->quote || n!='"')
					sfputc(stkp,'$');
			}
			cp = first = fcseek(0);
			if(mp->quote && cp)
				mp->pattern = c;
			break;
		    case S_ENDCH:
			if(bracketexpr && cp[-1]==RBRACT && !(mp->quote || mp->lit))
				bracketexpr--;
			if((mp->lit || cp[-1]!=endch || mp->quote!=newquote))
				goto pattern;
			if(endch==RBRACE && mp->pattern && brace)
			{
				brace--;
				if(*cp==LPAREN && mp->pattern!=2)
					goto pattern;
				continue;
			}
			/* FALLTHROUGH */
		    case S_EOF:
			if(c)
			{
				if(mp->split && !mp->quote && !mp->lit && endch)
					mac_copy(mp,first,c);
				else
					sfwrite(stkp,first,c);
			}
			if(n==S_EOF && resume)
			{
				fcclose();
				fcsopen(resume);
				resume = 0;
				cp = first = fcseek(0);
				continue;
			}
			c += (n!=S_EOF);
			first = fcseek(c);
			if(tilde>=0)
				tilde_expand2(tilde);
			goto done;
		    case S_QUOTE:
			if(mp->lit || mp->arith)
				break;
			/* FALLTHROUGH */
		    case S_LIT:
			if(mp->arith)
			{
				if((*cp=='`' || *cp=='[') && cp[1]=='\'')
					cp +=2;
				break;
			}
			if(n==S_LIT && mp->quote)
				break;
			if(c)
			{
				if(mp->split && endch && !mp->quote && !mp->lit)
					mac_copy(mp,first,c);
				else
					sfwrite(stkp,first,c);
			}
			first = fcseek(c+1);
			if(n==S_LIT)
			{
				if(mp->quote)
					continue;
				if(mp->lit)
					mp->lit = ansi_c = 0;
				else
					mp->lit = 1;
			}
			else
				mp->quote = !mp->quote;
			mp->quoted++;
			break;
		    case S_BRACT:
			if(mp->arith || (((mp->assign&1) || endch==RBRACT) &&
				!(mp->quote || mp->lit)))
			{
				int offset=0,oldpat = mp->pattern;
				int oldarith = mp->arith, oldsub=mp->subcopy;
				sfwrite(stkp,first,++c);
				if(mp->assign&1)
				{
					if(first[c-2]=='.')
						offset = stktell(stkp);
					if(isastchar(*cp) && cp[1]==']')
					{
						errormsg(SH_DICT,ERROR_exit(1),e_badsubscript,*cp);
						UNREACHABLE();
					}
				}
				first = fcseek(c);
				mp->pattern = 4;
				mp->arith = 0;
				mp->subcopy = 0;
				copyto(mp,RBRACT,0);
				mp->subcopy = oldsub;
				mp->arith = oldarith;
				mp->pattern = oldpat;
				sfputc(stkp,RBRACT);
				if(offset)
				{
					cp = stkptr(stkp,stktell(stkp));
					if(sh_checkid(stkptr(stkp,offset),cp)!=cp)
						stkseek(stkp,stktell(stkp)-2);
				}
				cp = first = fcseek(0);
				break;
			}
			if(mp->pattern==1 && !ere && !bracketexpr)
			{
				bracketexpr++;
				/* a ] following [, as in []abc], should not close the bracket expression */
				if(cp[0]==RBRACT && cp[1])
					bracketexpr++;
			}
			/* FALLTHROUGH */
		    case S_PAT:
			if(mp->pattern && !(mp->quote || mp->lit))
			{
				mp->patfound = mp->pattern;
				if((n=cp[-1])==LPAREN)
				{
					paren++;
					if((cp-first)>1 && cp[-2]=='~')
					{
						char *p = cp, minus = 0;
						/* Parse expression options to know if the expression is an ERE */
						while((c=mbchar(p)) && c!=RPAREN && c!=':') switch(c)
						{
							case 'A':  /* augmented regular expression (AST) */
							case 'E':  /* extended regular expression */
							case 'P':  /* Perl regular expression */
							case 'X':  /* augmented regular expression (AST) */
								if(minus)
									ere &= ~(1 << c - 'A');
								else
									ere |= 1 << c - 'A';
								break;
							case 'B':  /* basic regular expression */
							case 'F':  /* fixed pattern */
							case 'G':  /* basic regular expression */
							case 'K':  /* ksh glob pattern */
							case 'L':  /* fixed pattern */
							case 'S':  /* sh glob pattern */
							case 'V':  /* System V regular expression */
								if(!minus)
									ere = 0;
								break;
							case '-':  /* disable the following options */
								minus = 1;
								break;
							case '+':  /* enable the following options */
								minus = 0;
								break;
						}
					}
				}
				else if(n==RPAREN)
					--paren;
			}
			goto pattern;
		    case S_COM:
			if(mp->pattern==4 && (mp->quote || mp->lit))
			{
				if(c)
				{
					sfwrite(stkp,first,c);
					first = fcseek(c);
				}
				sfputc(stkp,ESCAPE);
			}
			break;
		    case S_BRACE:
			if(!(mp->quote || mp->lit))
			{
#if SHOPT_BRACEPAT
				if(mp->split && sh_isoption(SH_BRACEEXPAND))
					mp->patfound = 1;
#endif
				brace++;
			}
		    pattern:
			if(!mp->pattern || !(mp->quote || mp->lit))
			{
				/* mark beginning of {a,b} */
#if SHOPT_BRACEPAT
				if(n==S_BRACE && endch==0 && mp->pattern)
					mp->pattern=4;
#endif
				if(n==S_SLASH && mp->pattern==2)
					mp->pattern=3;
				break;
			}
			if(mp->pattern==3)
				break;
			if(c)
				sfwrite(stkp,first,c);
			first = fcseek(c);
			sfputc(stkp,ESCAPE);
			break;
		    case S_EQ:
			if(mp->assign==1)
			{
				if(*cp=='~' && !endch && !mp->quote && !mp->lit && !sh_isstate(SH_NOTILDEXP))
					tilde = stktell(stkp)+(c+1);
				mp->assign = 2;
			}
			break;
		    case S_SLASH:
		    case S_COLON:
			if(tilde >=0)
			{
				if(c)
					sfwrite(stkp,first,c);
				first = fcseek(c);
				tilde_expand2(tilde);
#if _WINIX
				if(Skip)
				{
					first = cp = fcseek(Skip);
					Skip = 0;
				}
#endif /* _WINIX */
				tilde = -1;
				c=0;
			}
			if(n==S_COLON && mp->assign==2 && *cp=='~' && endch==0 && !mp->quote && !mp->lit && !sh_isstate(SH_NOTILDEXP))
				tilde = stktell(stkp)+(c+1);
			else if(n==S_SLASH && mp->pattern==2)
			{
				if(mp->quote || mp->lit)
					goto pattern;
				sfwrite(stkp,first,c+1);
				first = fcseek(c+1);
				c = stktell(stkp);
				sh_lexskip(lp,RBRACE,0,ST_NESTED);
				stkseek(stkp,c);
				cp = fcseek(-1);
				sfwrite(stkp,first,cp-first);
				first=cp;
			}
			break;
		    case S_DOT:
			if(*cp=='.' && mp->subcopy==1)
			{
				sfwrite(stkp,first,c);
				sfputc(stkp,0);
				dotdot = stktell(stkp);
				cp = first = fcseek(c+2);
			}
			break;
		    case S_BRAOP:
			/* escape a quoted !^- within a bracket expression */
			if(!bracketexpr || !(mp->quote || mp->lit))
				continue;
			if(c)
				sfwrite(stkp,first,c);
			first = fcseek(c);
			sfputc(stkp,ESCAPE);
			break;
		}
	}
done:
	mp->sp = sp;
	mp->dotdot = dotdot;
	mp->quote = oldquote;
}

/*
 * copy <str> to stack performing sub-expression substitutions
 */
static void mac_substitute(Mac_t *mp, char *cp,char *str,int subexp[],int subsize)
{
	int	c,n;
	char	*first=fcseek(0);
	char	*ptr;
	Mac_t	savemac;
	n = stktell(sh.stk);
	savemac = *mp;
	mp->pattern = 3;
	mp->split = 0;
	mp->macsub++;
	fcsopen(cp);
	copyto(mp,0,0);
	sfputc(sh.stk,0);
	ptr = cp = sh_strdup(stkptr(sh.stk,n));
	stkseek(sh.stk,n);
	*mp = savemac;
	fcsopen(first);
	first = cp;
	while(1)
	{
		while((c= *cp++) && c!=ESCAPE);
		if(c==0)
			break;
		if((n= *cp++)=='\\' || n==RBRACE || (n>='0' && n<='9'))
		{
			c = cp-first-2;
			if(c)
				mac_copy(mp,first,c);
			first=cp;
			if(n=='\\' || n==RBRACE)
			{
				first--;
				continue;
			}
			n -= '0';
			if(n<subsize && (c=subexp[2*n])>=0)
			{
				if((n=subexp[2*n+1]-c)>0)
					mac_copy(mp,str+c,n);
			}
		}
		else if(n==0)
			break;
	}
	if(n=cp-first-1)
		mac_copy(mp,first,n);
	free(ptr);
}

#if  SHOPT_FILESCAN
#define MAX_OFFSETS	 (sizeof(sh.offsets)/sizeof(sh.offsets[0]))
#define MAX_ARGN	(32*1024)

/*
 * compute the arguments $1 ... $n and $# from the current line as needed
 * save line offsets in the offsets array.
 */
static char *getdolarg(int n, int *size)
{
	int c=S_DELIM, d=sh.ifstable['\\'];
	unsigned char *first,*last,*cp = (unsigned char*)sh.cur_line;
	int m=sh.offsets[0],delim=0;
	if(m==0)
		return NULL;
	if(m<0)
		m = 0;
	else if(n<=m)
		m = n-1;
	else
		m--;
	if(m >= MAX_OFFSETS-1)
		m =  MAX_OFFSETS-2;
	cp += sh.offsets[m+1];
	n -= m;
	sh.ifstable['\\'] = 0;
	sh.ifstable[0] = S_EOF;
	while(1)
	{
		if(c==S_DELIM)
			while(sh.ifstable[*cp++]==S_SPACE);
		first = --cp;
		if(++m < MAX_OFFSETS)
			sh.offsets[m] = (first-(unsigned char*)sh.cur_line);
		while((c=sh.ifstable[*cp++])==0);
		last = cp-1;
		if(c==S_SPACE)
			while((c=sh.ifstable[*cp++])==S_SPACE);
		if(--n==0 || c==S_EOF)
		{
			if(last==first && c==S_EOF && (!delim || (m>1)))
			{
				n++;
				m--;
			}
			break;
		}
		delim = (c==S_DELIM);
	}
	sh.ifstable['\\'] = d;
	if(m > sh.offsets[0])
		sh.offsets[0] = m;
	if(n)
		first = last = 0;
	if(size)
		*size = last-first;
	return (char*)first;
}
#endif /* SHOPT_FILESCAN */

/*
 * get the prefix after name reference resolution
 */
static char *prefix(char *id)
{
	Namval_t *np;
	char *sub=0, *cp = strchr(id,'.');
	if(cp)
	{
		*cp = 0;
		np = nv_search(id, sh.var_tree,0);
		*cp = '.';
		if(isastchar(cp[1]))
			cp[1] = 0;
		if(np && nv_isref(np))
		{
			int n;
			char *sp;
			nv_setoptimize(NULL);
			while(nv_isref(np) && np->nvalue)
			{
				sub = nv_refsub(np);
				np = nv_refnode(np);
				if(sub)
					nv_putsub(np,sub,0L);
			}
			id = (char*)sh_malloc(strlen(cp)+1+(n=strlen(sp=nv_name(np)))+ (sub?strlen(sub)+3:1));
			memcpy(id,sp,n);
			if(sub)
			{
				id[n++] = '[';
				strcpy(&id[n],sub);
				n+= strlen(sub)+1;
				id[n-1] = ']';
			}
			strcpy(&id[n],cp);
			return id;
		}
	}
	return sh_strdup(id);
}

/*
 * copy to ']' onto the stack and return offset to it
 */
static int subcopy(Mac_t *mp, int flag)
{
	int split = mp->split;
	int xpattern = mp->pattern;
	int loc = stktell(sh.stk);
	int xarith = mp->arith;
	int arrayok = mp->arrayok;
	mp->split = 0;
	mp->arith = 0;
	mp->pattern = flag?4:0;
	mp->arrayok=1;
	mp->subcopy++;
	mp->dotdot = 0;
	copyto(mp,RBRACT,0);
	mp->subcopy = 0;
	mp->pattern = xpattern;
	mp->split = split;
	mp->arith = xarith;
	mp->arrayok = arrayok;
	return loc;
}

/*
 * if name is a discipline function, run the function and put the results
 * on the stack so that ${x.foo} behaves like ${ x.foo;}
 */
int sh_macfun(const char *name, int offset)
{
	Namval_t	*np, *nq;
	np = nv_bfsearch(name,sh.fun_tree,&nq,NULL);
	if(np)
	{
		/* treat ${x.foo} as ${x.foo;} */
		union
		{
			struct comnod	com;
			Shnode_t	node;
		} t;
		union
		{
			struct argnod	arg;
			struct dolnod	dol;
			char buff[sizeof(struct dolnod)+sizeof(char*)];
		} d;
		memset(&t,0,sizeof(t));
		memset(&d,0,sizeof(d));
		t.node.com.comarg.ap = &d.arg;
		t.node.com.comline = sh.inlineno;
		d.dol.dolnum = 1;
		d.dol.dolval[0] = sh_strdup(name);
		stkseek(sh.stk,offset);
		comsubst((Mac_t*)sh.mac_context,&t.node,2);
		free(d.dol.dolval[0]);
		return 1;
	}
	return 0;
}

static int namecount(Mac_t *mp,const char *prefix)
{
	int count = 0;
	mp->nvwalk = nv_diropen(NULL,prefix);
	while(nv_dirnext(mp->nvwalk))
		count++;
	nv_dirclose(mp->nvwalk);
	return count;
}

static char *nextname(Mac_t *mp,const char *prefix, int len)
{
	char *cp;
	if(len==0)
	{
		mp->nvwalk = nv_diropen(NULL,prefix);
		return (char*)mp->nvwalk;
	}
	if(!(cp=nv_dirnext(mp->nvwalk)))
		nv_dirclose(mp->nvwalk);
	return cp;
}

/*
 * This routine handles $param, ${param}, and ${param op word}
 * The input stream is assumed to be a string
 */
static int varsub(Mac_t *mp)
{
	int		c;
	int		type=0; /* M_xxx */
	char		*v = NULL, *argp = NULL;
	Namval_t	*np = NULL;
	int 		dolg=0, mode=0;
	Lex_t		*lp = (Lex_t*)sh.lex_context;
	Namarr_t	*ap=0;
	int		dolmax=0, vsize= -1, offset= -1, nulflg, replen=0, bysub=0;
	char		idbuff[3], *id = idbuff, *pattern=0, *repstr=0, *arrmax=0;
	char		*idx = 0;
	int		var=1,addsub=0,oldpat=mp->pattern,idnum=0,flag=0,d;
	Stk_t		*stkp = sh.stk;
	mp->wasexpan = 1;
retry1:
	idbuff[0] = 0;
	idbuff[1] = 0;
	c = fcmbget(&LEN);
	switch(isascii(c)?sh_lexstates[ST_DOL][c]:S_ALP)
	{
	    case S_RBRA:
		if(type<M_SIZE)
			goto nosub;
		/* This code handles ${#} */
		c = mode;
		mode = type = 0;
		/* FALLTHROUGH */
	    case S_SPC1:
		if(type==M_BRACE)
		{
			if(isaletter(mode=fcpeek(0)) || mode=='.')
			{
				if(c=='#')
					type = M_SIZE;
				else if(c=='@')
				{
					type = M_TYPE;
					goto retry1;
				}
				else
					type = M_VNAME;
				mode = c;
				goto retry1;
			}
			else if(c=='#' && (isadigit(mode)||fcpeek(1)==RBRACE))
			{
				type = M_SIZE;
				mode = c;
				goto retry1;
			}
		}
		/* FALLTHROUGH */
	    case S_SPC2:
		var = 0;
		*id = c;
		v = special(c);
		if(isastchar(c))
		{
			mode = c;
#if  SHOPT_FILESCAN
			if(sh.cur_line)
			{
				dolmax = MAX_ARGN;
				v = getdolarg(1,&vsize);
				if(c=='*' || !mp->quoted)
				{
					dolmax = 1;
					vsize = -1;
				}
			}
			else
#endif  /* SHOPT_FILESCAN */
			dolmax = sh.st.dolc+1;
			mp->atmode = (v && mp->quoted && c=='@');
			dolg = (v!=0);
		}
		break;
	    case S_LBRA:
		if(type)
			goto nosub;
		type = M_BRACE;
		goto retry1;
	    case S_PAR:
		if(type)
			goto nosub;
		comsubst(mp,NULL,1);
		return 1;
	    case S_DIG:
		var = 0;
		c -= '0';
		nv_setoptimize(NULL);
		if(type)
		{
			int d;
			while((d=fcget()),isadigit(d))
				c = 10*c + (d-'0');
			fcseek(-1);
		}
		idnum = c;
		if(c==0)
			v = special(c);
#if  SHOPT_FILESCAN
		else if(sh.cur_line)
		{
			sh.used_pos = 1;
			v = getdolarg(c,&vsize);
		}
#endif  /* SHOPT_FILESCAN */
		else if(c <= sh.st.dolc)
		{
			sh.used_pos = 1;
			v = sh.st.dolv[c];
		}
		else
			v = 0;
		/* Handle 'set -u'/'set -o nounset' for positional parameters */
		if(!v && sh_isoption(SH_NOUNSET))
		{
			unsigned char d = fcpeek(0);
			if(!(d && strchr(":+-?=",d)))
			{
				errormsg(SH_DICT,ERROR_exit(1),e_notset,ltos(c));
				UNREACHABLE();
			}
		}
		break;
	    case S_ALP:
	    {
		Namval_t *np_orig;
		if(c=='.' && type==0)
			goto nosub;
		offset = stktell(stkp);
		do
		{
			int d;
			np = 0;
			do
			{
				if(LEN==1)
					sfputc(stkp,c);
				else
					sfwrite(stkp,fcseek(0)-LEN,LEN);
			}
			while((d=c,(c=fcmbget(&LEN)),isaname(c))||type && c=='.');
			while(c==LBRACT && (type||mp->arrayok))
			{
				nv_setoptimize(NULL);
				if((c=fcmbget(&LEN),isastchar(c)) && fcpeek(0)==RBRACT && d!='.')
				{
					if(type==M_VNAME)
						type = M_SUBNAME;
					idbuff[0] = mode = c;
					fcget();
					c = fcmbget(&LEN);
					if(c=='.' || c==LBRACT)
					{
						sfputc(stkp,LBRACT);
						sfputc(stkp,mode);
						sfputc(stkp,RBRACT);
					}
					else
						flag = NV_ARRAY;
					break;
				}
				else
				{
					fcseek(-LEN);
					c = stktell(stkp);
					if(d!='.')
						sfputc(stkp,LBRACT);
					v = stkptr(stkp,subcopy(mp,1));
					if(type && mp->dotdot)
					{
						mode = '@';
						v[-1] = 0;
						if(type==M_VNAME)
							type = M_SUBNAME;
						else if(type==M_SIZE)
							goto nosub;
					}
					else if(d!='.')
						sfputc(stkp,RBRACT);
					c = fcmbget(&LEN);
					if(c==0 && type==M_VNAME)
						type = M_SUBNAME;
				}
			}
		}
		while(type && c=='.');
		if(type!=M_VNAME && c==RBRACE && type &&  fcpeek(-2)=='.')
		{
			/* ${x.} or ${x..} */
			if(fcpeek(-3) == '.')
			{
				stkseek(stkp,stktell(stkp)-2);
				nv_local = 1;
			}
			else
			{
				stkseek(stkp,stktell(stkp)-1);
				type = M_TREE;
			}
		}
		sfputc(stkp,0);
		id=stkptr(stkp,offset);
		if(isastchar(c) && type)
		{
			if(type==M_VNAME || type==M_SIZE)
			{
				idbuff[0] = mode = c;
				if((d=fcpeek(0))==c)
					idbuff[1] = fcget();
				if(type==M_VNAME)
					type = M_NAMESCAN;
				else
					type = M_NAMECOUNT;
				break;
			}
			goto nosub;
		}
		flag |= NV_VARNAME|NV_NOADD;
		if(c=='=' || c=='?' || (c==':' && ((d=fcpeek(0))=='=' || d=='?')))
		{
			if(c=='=' || (c==':' && d=='='))
				flag |= NV_ASSIGN;
			flag &= ~NV_NOADD;
			sh.cond_expan = 1;	/* tell nv_putsub() not to change value from null to empty */
			np = nv_open(id,sh.var_tree,flag|NV_NOFAIL);
			sh.cond_expan = 0;
		}
#if  SHOPT_FILESCAN
		else if(sh.cur_line && strcmp(id,REPLYNOD->nvname)==0)
		{
			nv_setoptimize(NULL);
			np = REPLYNOD;
		}
#endif  /* SHOPT_FILESCAN */
		else
		{
			if(nv_getoptimize())
				flag &= ~NV_NOADD;
			np = nv_open(id,sh.var_tree,flag|NV_NOFAIL);
		}
		if(!np)
		{
			/* id points to stack, which will be overwritten; save it for error message */
			sfputr(sh.strbuf,id,-1);
			id = sfstruse(sh.strbuf);
		}
		if(isastchar(mode))
			var = 0;
		if((!np || nv_isnull(np)) && type==M_BRACE && c==RBRACE && !(flag&NV_ARRAY) && strchr(id,'.'))
		{
			if(sh_macfun(id,offset))
			{
				fcmbget(&LEN);
				return 1;
			}
		}
		if(np && (flag&NV_NOADD) && nv_isnull(np))
		{
			if(nv_isattr(np,NV_NOFREE))
				nv_offattr(np,NV_NOFREE);
#if  SHOPT_FILESCAN
			else if(np!=REPLYNOD  || !sh.cur_line)
#else
			else
#endif  /* SHOPT_FILESCAN */
				np = 0;
		}
		np_orig = np;
		ap = np?nv_arrayptr(np):0;
		if(type)
		{
			if(mp->dotdot)
			{
#if SHOPT_FIXEDARRAY
				if(ap && !ap->fixed)
#else
				if(ap)
#endif /* SHOPT_FIXEDARRAY */
				{
					Namval_t *nq;
					if(nq = nv_opensub(np))
						np = nq;
				}
				if(ap)
				{
					np = nv_putsub(np,v,ARRAY_SCAN);
					v = stkptr(stkp,mp->dotdot);
					dolmax =1;
					ap = nv_arrayptr(np_orig); /* update */
					if(array_assoc(ap))
						arrmax = sh_strdup(v);
					else if((dolmax = (int)sh_arith(v))<0)
						dolmax += array_maxindex(np);
					if(type==M_SUBNAME)
						bysub = 1;
				}
				else
				{
					if((int)sh_arith(v))
						np = 0;
				}
			}
			else if(ap && (isastchar(mode)||type==M_TREE)  && !(ap->nelem&ARRAY_SCAN) && type!=M_SIZE)
				nv_putsub(np,NULL,ARRAY_SCAN);
			if(!isbracechar(c))
				goto nosub;
			else
				fcseek(-LEN);
		}
		else
			fcseek(-1);
		if(type<=1 && np && nv_isvtree(np) && mp->pattern==1 && !mp->split)
		{
			int cc=fcmbget(&LEN),peek=LEN;
			if(type && cc=='}')
			{
				cc = fcmbget(&LEN);
				peek++;
			}
			if(mp->quote && cc=='"')
			{
				cc = fcmbget(&LEN);
				peek++;
			}
			fcseek(-peek);
			if(cc==0)
				mp->assign = 1;
		}
		if((type==M_VNAME||type==M_SUBNAME) && nv_getoptimize() && strcmp(nv_name(np),id))
			nv_setoptimize(NULL);
		c = (type>M_BRACE && isastchar(mode));
		/*
		 * Check if the parameter is set or unset.
		 */
		if(np && type==M_BRACE && nv_getoptimize())
			nv_optimize(np);  /* needed before calling nv_isnull() */
		if(np && (type==M_BRACE ? !nv_isnull(np) : (type==M_TREE || !c || !ap)))
		{
			/* Either the parameter is set, or it's a special type of expansion where 'unset' doesn't apply. */
			void *savptr;
			c = *((unsigned char*)stkptr(stkp,offset-1));
			savptr = stkfreeze(stkp,0);
			if(type==M_VNAME || (type==M_SUBNAME && ap))
			{
				type = M_BRACE;
				v = nv_name(np);
				if(ap && !mp->dotdot)
				{
					ap = nv_arrayptr(np_orig); /* update */
					if(!(ap->nelem&ARRAY_UNDEF))
						addsub = 1;
				}
			}
			else if(type==M_TYPE)
			{
				Namval_t *nq = nv_type(np);
				type = M_BRACE;
				if(nq)
					nv_typename(nq,sh.strbuf);
				else
					nv_attribute(np,sh.strbuf,"typeset",1);
				v = sfstruse(sh.strbuf);
			}
#if  SHOPT_FILESCAN
			else if(sh.cur_line && np==REPLYNOD)
				v = sh.cur_line;
#endif  /* SHOPT_FILESCAN */
			else if(type==M_TREE)
				v = nv_getvtree(np,NULL);
			else
			{
				if(type && fcpeek(0)=='+')
				{
					if(ap)
					{
						ap = nv_arrayptr(np_orig); /* update */
						v = nv_arrayisset(np,ap)?(char*)"x":0;
					}
					else
						v = nv_isnull(np)?0:(char*)"x";
				}
				else
					v = nv_getval(np);
				mp->atmode = (v && mp->quoted && mode=='@');
			}
			if(savptr==stkptr(sh.stk,0))
				stkseek(stkp,offset);
			else
				stkset(stkp,savptr,offset);
		}
		else
		{
			/* The parameter is unset. */
			if(sh_isoption(SH_NOUNSET) && !isastchar(mode) && (type==M_VNAME || type==M_SIZE))
			{
				errormsg(SH_DICT,ERROR_exit(1),e_notset,id);
				UNREACHABLE();
			}
			v = 0;
			if(type==M_VNAME)
			{
				v = id;
				type = M_BRACE;
			}
			else if(type==M_TYPE)
				type = M_BRACE;
		}
		stkseek(stkp,offset);
		if(ap)
		{
			ap = nv_arrayptr(np_orig); /* update */
			if(nv_getoptimize())
				nv_optimize(np);
			if(isastchar(mode) && array_elem(ap)> !c)
				dolg = -1;
			else
			{
				ap->nelem &= ~ARRAY_SCAN;
				dolg = 0;
			}
		}
		break;
	    }
	    case S_EOF:
		fcseek(-1);
	    default:
		goto nosub;
	}
	if(type)
		c = fcmbget(&LEN);
	else
		c = fcget();
	if(type>M_TREE)
	{
		if(c!=RBRACE)
			mac_error();
		if(type==M_NAMESCAN || type==M_NAMECOUNT)
		{
			sh.last_root = sh.var_tree;
			id = idx = prefix(id);
			stkseek(stkp,offset);
			if(type==M_NAMECOUNT)
			{
				c = namecount(mp,id);
				v = ltos(c);
			}
			else
			{
				/* M_NAMESCAN: ${!prefix@} or ${!prefix*}. These work like $@, $*. */
				dolmax = strlen(id);
				dolg = -1;
				nextname(mp,id,0);
				/* Check if the prefix (id) itself exists. If so, start with that. */
				if(nv_open(id,sh.var_tree,NV_NOREF|NV_NOADD|NV_VARNAME|NV_NOFAIL))
					v = id;
				else
					v = nextname(mp,id,dolmax);
			}
		}
		else if(type==M_SUBNAME)
		{
			if(dolg<0)
			{
				v = nv_getsub(np);
				bysub=1;
			}
			else if(v)
			{
				if(!ap || isastchar(mode))
					v = "0";
				else
					v = nv_getsub(np);
			}
		}
		else
		{
			/* type==M_SIZE: ${#var} */
			if(!isastchar(mode))
				c = charlen(v,vsize);
			else if(dolg>0)
			{
#if  SHOPT_FILESCAN
				if(sh.cur_line)
				{
					getdolarg(MAX_ARGN,NULL);
					c = sh.offsets[0];
				}
				else
#endif  /* SHOPT_FILESCAN */
				c = sh.st.dolc;
			}
			else if(dolg<0)
			{
				ap = nv_arrayptr(np); /* update */
				c = array_elem(ap);
			}
			else
				c = (v!=0);
			dolg = dolmax = 0;
			v = ltos(c);
		}
		c = RBRACE;
	}
	nulflg = 0;
	if(type && c==':')
	{
		c = fcmbget(&LEN);
		if(isascii(c) &&sh_lexstates[ST_BRACE][c]==S_MOD1 && c!='*' && c!= ':')
			nulflg=1;
		else if(c!='%' && c!='#')
		{
			fcseek(-LEN);
			c = ':';
		}
	}
	if(type)
	{
		if(!isbracechar(c))
		{
			if(!nulflg)
				mac_error();
			fcseek(-LEN);
			c = ':';
		}
		if(c!=RBRACE)
		{
			int newops = sh_lexstates[ST_BRACE][c]==S_MOD2;
			offset = stktell(stkp);
			if(newops && sh_isoption(SH_NOUNSET) && *id && id!=idbuff  && (!np || nv_isnull(np)))
			{
				errormsg(SH_DICT,ERROR_exit(1),e_notset,id);
				UNREACHABLE();
			}
			if(c=='/' || c==':' || ((!v || (nulflg && *v==0)) ^ (c=='+' || newops)))
			{
				int newquote = mp->quote;
				int split = mp->split;
				int quoted = mp->quoted;
				int arith = mp->arith;
				int assign = mp->assign;
				if(newops)
				{
					type = fcget();
					if(type=='%' || type=='#')
					{
						int d = fcmbget(&LEN);
						fcseek(-LEN);
						if(d=='(')
							type = 0;
					}
					fcseek(-1);
					mp->pattern = 1+(c=='/');
					mp->split = 0;
					mp->quoted = 0;
					mp->assign &= ~1;
					mp->arith = 0;
					newquote = 0;
				}
				else if(c=='?' || c=='=')
					mp->split = mp->pattern = 0;
				copyto(mp,RBRACE,newquote);
				if(!oldpat)
					mp->patfound = 0;
				mp->pattern = oldpat;
				mp->split = split;
				mp->quoted = quoted;
				mp->arith = arith;
				mp->assign = assign;
				/* add null byte */
				sfputc(stkp,0);
				if(c!='=')
					stkseek(stkp,stktell(stkp)-1);
			}
			else
			{
				int state;
				if(sh_lexstates[ST_BRACE][c]==S_MOD1)
					state = mp->quote ? ST_QUOTE : ST_MOD1;
				else
					state = ST_NESTED;
				sh_lexskip(lp, RBRACE, 0, state);
				stkseek(stkp,offset);
			}
			argp=stkptr(stkp,offset);
		}
	}
	else
	{
		fcseek(-1);
		c=0;
	}
	/* check for slice operator: ${parameter:offset[:length]} */
	if(c==':')
	{
		char *lastchar;
		int sliceoffset;
		sh_trim(argp);  /* remove internal backslash escapes */
		sliceoffset = (int)sh_strnum(argp,&lastchar,1);
		if(isastchar(mode))
		{
			if(id==idbuff)  /* ${@} or ${*} */
			{
				if(sliceoffset < 0 && (sliceoffset += dolmax) < 0)
					sliceoffset = 0;
				if(sliceoffset==0)
					v = special(dolg=0);
#if  SHOPT_FILESCAN
				else if(sh.cur_line)
				{
					v = getdolarg(dolg = sliceoffset, &vsize);
					if(!v)
						dolmax = sliceoffset;
				}
#endif  /* SHOPT_FILESCAN */
				else if(sliceoffset < dolmax)
					v = sh.st.dolv[dolg = sliceoffset];
				else
					v =  0;
			}
			else if(ap)
			{
				/* Array slicing */
				ap = nv_arrayptr(np); /* update */
				if(sliceoffset < 0)
				{
					if(array_assoc(ap))
						sliceoffset = -sliceoffset;
					else
						sliceoffset += array_maxindex(np);
				}
				if(array_assoc(ap))
				{
					nv_putsub(np, NULL, ap->nelem&ARRAY_SCAN ? 0 : ARRAY_SCAN);
					while(sliceoffset-- > 0 && (v=0,nv_nextsub(np)))
						v = nv_getval(np);
				}
				else if(sliceoffset >= 0)
				{
					if(nv_putsub(np, NULL, sliceoffset|ARRAY_SCAN))
						v = nv_getval(np);
					else
						v = 0;
				}
			}
			else if(sliceoffset > 0)
				v = 0;
			if(!v)
				mp->atmode = 0;
		}
		else if(v)
		{
			vsize = charlen(v,vsize);
			if(sliceoffset < 0 && (sliceoffset += vsize) < 0)
				sliceoffset = 0;
			if(vsize < sliceoffset)
				v = 0;
			else if(mbwide())
			{
				for(c = sliceoffset; c; c--)
					mbchar(v);
				c = ':';
			}
			else
				v += sliceoffset;
			vsize = v?strlen(v):0;
		}
		if(*lastchar==':')
		{
			int slicelength = (int)sh_strnum(lastchar+1,&lastchar,1);
			if(slicelength <= 0)
			{
				v = 0;
				mp->atmode = 0;
			}
			else if(isastchar(mode))
			{
				if(dolg>=0)
				{
					if(dolg + slicelength < dolmax)
						dolmax = dolg + slicelength;
				}
				else
					dolmax = slicelength;
			}
			else if(slicelength < vsize)
			{
				if(mbwide())
				{
					char *vp = v;
					while(slicelength-- > 0)
					{
						if((c=mbsize(vp))<1)
							c = 1;
						vp += c;
					}
					slicelength = vp - v;
					c = ':';
				}
				vsize = slicelength;
			}
			else
				vsize = v?strlen(v):0;
		}
		if(*lastchar)
			mac_error();
		stkseek(stkp,offset);
		argp = 0;
	}
	/* check for substring operations */
	else if(sh_lexstates[ST_BRACE][c]==S_MOD2)
	{
		if(c=='/')
		{
			if(type=='/' || type=='#' || type=='%')
			{
				c = type;
				type = '/';
				argp++;
			}
			else
				type = 0;
		}
		else
		{
			if(type==c) /* ## or %% */
				argp++;
			else
				type = 0;
		}
		pattern = sh_strdup(argp);
		if((type=='/' || c=='/') && (repstr = mac_getstring(pattern)))
			replen = strlen(repstr);
		if(v || c=='/' && offset>=0)
			stkseek(stkp,offset);
	}
	/* discount the quotes around $@ if there are zero PPs, so that empty "$@" generates zero fields */
	if(mode=='@' && mp->quote && !v && c!='-' && mp->atspecial != mp->quoted)
		mp->quoted -= 2, mp->atspecial = mp->quoted;
retry2:
	if(v && (!nulflg || *v ) && c!='+')
	{
		int ofs_size = 0;
		int match[2*(MATCH_MAX+1)],index;
		int nmatch, nmatch_prev, vsize_last = 0, tsize;
		char *vlast = NULL, *oldv;
		while(1)
		{
			if(!v)
				v= "";
			if(c=='/' || c=='#' || c== '%')
			{
				flag = (type || c=='/')?(STR_GROUP|STR_MAXIMAL):STR_GROUP;
				if(c!='/')
					flag |= STR_LEFT;
				index = nmatch = 0;
				tsize = (int)strlen(v);
				while(1)
				{
					vsize = tsize;
					oldv = v;
					nmatch_prev = nmatch;
					if(c=='%')
						nmatch = substring(v, tsize,
							*pattern ? pattern : "~(E)$",
							match,
							flag & STR_MAXIMAL);
					else
						nmatch = strngrpmatch(v, vsize,
							*pattern ? pattern : (c=='#' ? "~(E)^" : pattern),
							(ssize_t*)match,
							elementsof(match) / 2,
							flag | STR_INT);
					if(nmatch && repstr && !mp->macsub)
						sh_setmatch(v,vsize,nmatch,match,index++);
					if(nmatch)
					{
						vlast = v;
						vsize_last = vsize;
						vsize = match[0];
					}
					else if(c=='#')
						vsize = 0;
					if(vsize)
						mac_copy(mp,v,vsize);
					if(nmatch && replen>0 && (match[1] || !nmatch_prev))
						mac_substitute(mp,repstr,v,match,nmatch);
					if(nmatch==0)
						v += vsize;
					else
						v += match[1];
					if(*v &&  c=='/' && type)
					{
						/* avoid infinite loop */
						if(nmatch && match[1]==0)
						{
							int	sz;
							nmatch = 0;
							/* copy, and advance v by, one character */
							if ((sz = mbsize(v)) < 1)
								sz = 1;
							mac_copy(mp, v, sz);
							v += sz;
						}
						tsize -= v-oldv;
						continue;
					}
					vsize = -1;
					break;
				}
				if(!mp->macsub && (!repstr || (nmatch==0 && index==0)))
					sh_setmatch(vlast,vsize_last,nmatch,match,index++);
				if(!mp->macsub && index>0 && c=='/' && type)
					sh_setmatch(0,0,nmatch,0,-1);
			}
			if (c == '^' || c == ',')
			{
				/* case modification: ${var^pat} ${var^^pat} ${var,pat} ${var,,pat} */
				vsize = strlen(v);
				while (vsize > 0)
				{
					flag = STR_GROUP | STR_MAXIMAL | (type ? 0 : STR_LEFT);
					nmatch = strngrpmatch(v, vsize,
						*pattern ? pattern : "?",
						(ssize_t *)match, elementsof(match) / 2,
						flag | STR_INT);
					if (nmatch == 0)
						break;
					if (match[1] == 0 && type)  /* avoid infinite loop */
					{
						int	sz;
						/* copy, and advance v by, one character */
						if ((sz = mbsize(v)) < 1)
							sz = 1;
						mac_copy(mp, v, sz);
						v += sz;
						vsize -= sz;
						continue;
					}
					if (match[0])
						mac_copy(mp, v, match[0]);
					if (mbwide())  /* locale uses multibyte characters? */
					{
						int	wc, nwc;
						char	*mbuf = 0;
						char	*cp = v + match[0], *ocp;
						while (cp < v + match[1])
						{
							ocp = cp;
							wc = mbchar(cp);  /* this advances cp by one multibyte character */
							if (wc < 0)
								nwc = '?';
							else if (c == '^')
								nwc = towupper(wc);
							else
								nwc = towlower(wc);
							if (nwc == wc)  /* performance: avoid converting it back */
								mac_copy(mp, ocp, cp - ocp);
							else  /* convert new wide character to multibyte representation */
							{
								if (!mbuf)
									mbuf = fmtbuf(mbmax());
								mac_copy(mp, mbuf, mbconv(mbuf, nwc));
							}
						}						
					}
					else /* no multibyte */
					{
						char	*cp, *cq, *buf;
						buf = sh_malloc(match[1] - match[0]);
						for (cp = v + match[0], cq = buf; cp < v + match[1]; cp++, cq++)
							*cq = c == '^' ? toupper(*cp) : tolower(*cp);
						mac_copy(mp, buf, match[1] - match[0]);
						free(buf);
					}
					v += match[1];
					vsize -= match[1];
					if (!type)
						break;
				}
			}
			if(vsize)
				mac_copy(mp,v,vsize>0?vsize:strlen(v));
			if(addsub)
			{
				sh.instance++;
				sfprintf(sh.strbuf,"[%s]",nv_getsub(np));
				sh.instance--;
				v = sfstruse(sh.strbuf);
				mac_copy(mp, v, strlen(v));
			}
			if(dolg==0 && dolmax==0)
				 break;
			if(mp->dotdot)
			{
				if(nv_nextsub(np) == 0)
					break;
				if(bysub)
					v = nv_getsub(np);
				else
					v = nv_getval(np);
				ap = nv_arrayptr(np); /* update */
				if(array_assoc(ap))
				{
					if(strcmp(bysub?v:nv_getsub(np),arrmax)>0)
						break;
				}
				else
				{
					if(nv_aindex(np) > dolmax)
						break;
				}
			}
			else if(dolg>=0)
			{
				if(++dolg >= dolmax)
					break;
#if  SHOPT_FILESCAN
				if(sh.cur_line)
				{
					if(!(v=getdolarg(dolg,&vsize)))
					{
						dolmax = dolg;
						break;
					}
				}
				else
#endif  /* SHOPT_FILESCAN */
				v = sh.st.dolv[dolg];
			}
			else if(!np)
			{
				if(!(v = nextname(mp,id,dolmax)))
					break;
			}
			else
			{
				if(dolmax &&  --dolmax <=0)
				{
					nv_putsub(np,NULL,ARRAY_UNDEF);
					break;
				}
				if(ap)
				{
					ap = nv_arrayptr(np); /* update */
					ap->nelem |= ARRAY_SCAN;
				}
				if(nv_nextsub(np) == 0)
					break;
				if(bysub)
					v = nv_getsub(np);
				else
					v = nv_getval(np);
			}
			if(mp->split && (!mp->quote || mode=='@'))
			{
				if(!np)
					mp->pattern = 0;
				endfield(mp,mp->quoted);
				mp->atmode = mode=='@';
				mp->pattern = oldpat;
			}
			else
			{
				Sfio_t *sfio_ptr = (mp->sp) ? mp->sp : stkp;
				/*
				 * We're joining fields into one; write the output field separator, which may be multi-byte.
				 * For "$@" it's a space, for "$*" it's the 1st char of IFS (space if unset, none if empty).
				 */
				if(mp->pattern && mp->quote && mp->ifs)	/* avoid BUG_IFSGLOBS */
					sfputc(sfio_ptr, '\\');
				if(mode == '@' || !mp->ifsp)		/* if expanding $@ or if IFS is unset... */
					sfputc(sfio_ptr, ' ');
				else if(mp->ifs)			/* else if IFS is non-empty... */
				{
					if(!mbwide() || mp->ifs < 128)	/* if single-byte char... */
						sfputc(sfio_ptr, mp->ifs);
					else
					{
						if(!ofs_size)		/* only calculate this once per expansion */
						{
							ofs_size = mbsize(mp->ifsp);
							if(ofs_size<0)	/* invalid mb char: fall back to using first byte */
								ofs_size = 1;
						}
						sfwrite(sfio_ptr, mp->ifsp, ofs_size);
					}
				}
			}
		}
		if(arrmax)
			free(arrmax);
	}
	else if(argp)
	{
		if(c=='/' && replen>0 && pattern && strmatch("",pattern))
			mac_substitute(mp,repstr,v,0,0);
		if(c=='?')
		{
			if(np)
				id = nv_name(np);
			else if(idnum)
				id = ltos(idnum);
			if(*argp)
			{
				sfputc(stkp,0);
				errormsg(SH_DICT,ERROR_exit(1),"%s: %s",id,argp);
			}
			else if(v)
				errormsg(SH_DICT,ERROR_exit(1),e_nullset,id);
			else
				errormsg(SH_DICT,ERROR_exit(1),e_notset,id);
			UNREACHABLE();
		}
		else if(c=='=')
		{
			if(np)
			{
				if(sh.subshell)
					sh_assignok(np,1);
				nv_putval(np,argp,0);
				v = nv_getval(np);
				nulflg = 0;
				stkseek(stkp,offset);
				goto retry2;
			}
		else
			mac_error();
		}
	}
	else if(var && sh_isoption(SH_NOUNSET) && type<=M_TREE && (!np  || nv_isnull(np) || (nv_isarray(np) && !np->nvalue)))
	{
		if(np)
		{
			if(nv_isarray(np))
			{
				sfprintf(sh.strbuf,"%s[%s]\0",nv_name(np),nv_getsub(np));
				id = sfstruse(sh.strbuf);
			}
			else
				id = nv_name(np);
		}
		errormsg(SH_DICT,ERROR_exit(1),e_notset,id);
		UNREACHABLE();
	}
	if(pattern)
		free(pattern);
	if(idx)
		free(idx);
	return 1;
nosub:
	if(type==M_BRACE && sh_lexstates[ST_NORM][c]==S_BREAK)
	{
		fcseek(-1);
		comsubst(mp,NULL,2);
		return 1;
	}
	if(type)
		mac_error();
	fcseek(-1);
	mp->wasexpan = 0;
	return 0;
}

/*
 * This routine handles command substitution
 * and arithmetic expansion.
 * <type> is 0 for older `...` version
 * 1 for $(...) or 2 for ${ subshare; }
 */
static void comsubst(Mac_t *mp,Shnode_t* t, int type)
{
	Sfdouble_t		num;
	int			c;
	char			*str;
	Sfio_t			*sp;
	Stk_t			*stkp = sh.stk;
	Fcin_t			save;
	struct slnod            *saveslp = sh.st.staklist;
	Mac_t			savemac = *mp;
	int			savtop = stktell(stkp);
	char			lastc = '\0';
	void			*savptr = stkfreeze(stkp,0);
	int			was_history = sh_isstate(SH_HISTORY);
	int			was_verbose = sh_isstate(SH_VERBOSE);
	int			was_interactive = sh_isstate(SH_INTERACTIVE);
	int			newlines,bufsize,nextnewlines;
	Sfoff_t			foff;
	Namval_t		*np;
	savemac.wasexpan = 1;
	nv_setoptimize(NULL);
	sh.st.staklist=0;
	if(type)
	{
		sp = 0;
		fcseek(-1);
		if(!t)
			t = sh_dolparen((Lex_t*)sh.lex_context);
		if(t && t->tre.tretyp==TARITH)
		{
			fcsave(&save);
			if(t->ar.arcomp)
				num = arith_exec(t->ar.arcomp);
			else if((t->ar.arexpr->argflag&ARG_RAW))
				num = sh_arith(t->ar.arexpr->argval);
			else
				num = sh_arith(sh_mactrim(t->ar.arexpr->argval,3));
		out_offset:
			stkset(stkp,savptr,savtop);
			*mp = savemac;
			if((Sflong_t)num!=num)
				sfprintf(sh.strbuf,"%.*Lg",LDBL_DIG,num);
			else if(num)
				sfprintf(sh.strbuf,"%jd",(Sflong_t)num);
			else
				sfprintf(sh.strbuf,"%Lg",num);
			str = sfstruse(sh.strbuf);
			mac_copy(mp,str,strlen(str));
			sh.st.staklist = saveslp;
			fcrestore(&save);
			return;
		}
	}
	else
	{
		while((c = fcgetc()) && c != '`')
		{
			if(c==ESCAPE)
			{
				c = fcgetc();
				if(!(isescchar(sh_lexstates[ST_QUOTE][c]) ||
				  (c=='"' && mp->quote)))
					sfputc(stkp,ESCAPE);
			}
			sfputc(stkp,c);
		}
		sfputc(stkp,'\n');  /* a=`some_alias` leaked memory, a=`some_alias<LF>` did not! TODO: non-hack fix */
		c = stktell(stkp);
		str=stkfreeze(stkp,1);
		/* disable verbose and don't save in history file */
		sh_offstate(SH_HISTORY);
		sh_offstate(SH_VERBOSE);
		if(mp->sp)
			sfsync(mp->sp);	/* flush before executing command */
		sp = sfnew(NULL,str,c,-1,SFIO_STRING|SFIO_READ);
		c = sh.inlineno;
		sh.inlineno = error_info.line+sh.st.firstline;
		t = (Shnode_t*)sh_parse(sp,SH_EOF|SH_NL);
		sh.inlineno = c;
		type = 1;
	}
	if(t)
	{
		fcsave(&save);
		sfclose(sp);
		if(t->tre.tretyp==0 && !t->com.comarg.dp && !t->com.comset)
		{
			/* special case $(<file) and $(<#file) */
			int fd;
			int r=0;
			struct checkpt buff;
			struct ionod *ip=0;
			sh_pushcontext(&buff,SH_JMPIO);
			if((ip=t->tre.treio) &&
				((ip->iofile&IOLSEEK) || !(ip->iofile&IOUFD)) &&
				(r=sigsetjmp(buff.buff,0))==0)
				fd = sh_redirect(ip,3);
			else
				fd = sh_chkopen(e_devnull);
			sh_popcontext(&buff);
			if(r==0 && ip && (ip->iofile&IOLSEEK))
			{
				if(sp=sh.sftable[fd])
					num = sftell(sp);
				else
					num = lseek(fd, 0, SEEK_CUR);
				goto out_offset;
			}
			if(!(sp=sh.sftable[fd]))
			{
				char *cp = (char*)sh_malloc(IOBSIZE+1);
				sp = sfnew(NULL,cp,IOBSIZE,fd,SFIO_READ|SFIO_MALLOC);
			}
		}
		else
		{
			if(type==2 && sh.subshell && !sh.subshare)
				sh_subfork();	/* subshares within virtual subshells are broken, so fork first */
			sp = sh_subshell(t,sh_isstate(SH_ERREXIT),type);
		}
		fcrestore(&save);
	}
	else
		sp = sfopen(NULL,"","sr");
	sh_freeup();
	sh.st.staklist = saveslp;
	if(was_history)
		sh_onstate(SH_HISTORY);
	if(was_verbose)
		sh_onstate(SH_VERBOSE);
	*mp = savemac;
	np = sh_scoped(IFSNOD);
	nv_putval(np,mp->ifsp,NV_RDONLY);
	mp->ifsp = nv_getval(np);
	stkset(stkp,savptr,savtop);
	newlines = 0;
	sfsetbuf(sp,sp,0);
	bufsize = sfvalue(sp);
	/* read command substitution output and put on stack or here-doc */
	sfpool(sp, NULL, SFIO_WRITE);
	sh_offstate(SH_INTERACTIVE);
	if((foff = sfseek(sp,0,SEEK_END)) > 0)
	{
		size_t soff = stktell(stkp);
		sfseek(sp,0,SEEK_SET);
		stkseek(stkp,soff+foff+64);
		stkseek(stkp,soff);
	}
	while((str=(char*)sfreserve(sp,SFIO_UNBOUND,0)) && (c=bufsize=sfvalue(sp))>0)
	{
#if SHOPT_CRNL
		/* eliminate <cr> */
		char *dp;
		char *buff = str;
		while(c>1 && (*str !='\r'|| str[1]!='\n'))
		{
			c--;
			str++;
		}
		dp = str;
		while(c>1)
		{
			str++;
			c--;
			while(c>1 && (*str!='\r' || str[1]!='\n'))
			{
				c--;
				*dp++ = *str++;
			}
		}
		if(c)
			*dp++ = *str++;
		str = buff;
		c = dp-str;
#endif /* SHOPT_CRNL */
		/* delay appending trailing new-lines */
		for(nextnewlines=0; c-->0 && str[c]=='\n'; nextnewlines++);
		if(c < 0)
		{
			newlines += nextnewlines;
			continue;
		}
		if(newlines >0)
		{
			if(mp->sp)
				sfnputc(mp->sp,'\n',newlines);
			else if(!mp->quote && mp->split && sh.ifstable['\n'])
				endfield(mp,0);
			else
				sfnputc(stkp,'\n',newlines);
		}
		else if(lastc)
		{
			mac_copy(mp,&lastc,1);
			lastc = 0;
		}
		newlines = nextnewlines;
		if(++c < bufsize)
			str[c] = 0;
		else
		{
			/* can't write past buffer so save last character */
			c -= 1;
			lastc = str[c];
			str[c] = 0;
		}
		mac_copy(mp,str,c);
	}
	if(was_interactive)
		sh_onstate(SH_INTERACTIVE);
	if(--newlines>0 && sh.ifstable['\n']==S_DELIM)
	{
		if(mp->sp)
			sfnputc(mp->sp,'\n',newlines);
		else if(!mp->quote && mp->split)
			while(newlines--)
				endfield(mp,1);
		else
			sfnputc(stkp,'\n',newlines);
	}
	if(lastc)
	{
		mac_copy(mp,&lastc,1);
		lastc = 0;
	}
	sfclose(sp);
	return;
}

/*
 * copy <str> onto the stack
 */
static void mac_copy(Mac_t *mp,const char *str, int size)
{
	const char	*cp=str;
	int		c,n,nopat,len;
	Stk_t		*stkp=sh.stk;
	int		oldpat = mp->pattern;
	nopat = (mp->quote||(mp->assign==1)||mp->arith);
	if(mp->sp)
		sfwrite(mp->sp,str,size);
	else if(mp->pattern>=2 || (mp->pattern && nopat) || mp->assign==3)
	{
		const char *macro_state = sh_lexstates[ST_MACRO];
		/* insert \ before file expansion characters */
		while(size-->0)
		{
			if(mbwide() && (len=mbsize(cp))>1)
			{
				cp += len;
				size -= (len-1);
				continue;
			}
			c = macro_state[n = *(unsigned char*)cp++];
			if(mp->assign==3 && mp->pattern!=4)
			{
				if(c==S_BRACT)
				{
					nopat = 0;
					mp->pattern = 4;
				}
				continue;
			}
			if(nopat&&(c==S_PAT||c==S_ESC||c==S_BRACT||c==S_ENDCH) && mp->pattern!=3)
				c=1;
			else if(mp->pattern==4 && (c==S_ESC||c==S_BRACT||c==S_ENDCH || isastchar(n)))
			{
				if(c==S_ENDCH && oldpat!=4)
				{
					if(*cp==0 || *cp=='.' || *cp=='[')
					{
						mp->pattern = oldpat;
						c=0;
					}
					else
						c=1;
				}
				else
					c=1;
			}
			else if(mp->pattern==2 && c==S_SLASH)
				c=1;
			else if(mp->pattern==3 && c==S_ESC && (macro_state[*(unsigned char*)cp]==S_DIG || (*cp==ESCAPE)))
			{
				if(!(c=mp->quote))
					cp++;
			}
			else
				c=0;
			if(c)
			{
				if(c = (cp-1) - str)
					sfwrite(stkp,str,c);
				sfputc(stkp,ESCAPE);
				str = cp-1;
			}
		}
		if(c = cp-str)
			sfwrite(stkp,str,c);
	}
	else if(!mp->quote && mp->split && (mp->ifs||mp->pattern))
	{
		/* split words at ifs characters */
		char *ifs_state = sh.ifstable;
		if(mp->pattern)
		{
			char *sp = "&|()";
			while(c = *sp++)
			{
				if(ifs_state[c]==0)
					ifs_state[c] = S_EPAT;
			}
			sp = "*?[{";
			while(c = *sp++)
			{
				if(ifs_state[c]==0)
					ifs_state[c] = S_PAT;
			}
			if(ifs_state[ESCAPE]==0)
				ifs_state[ESCAPE] = S_ESC;
		}
		while(size-->0)
		{
			n = ifs_state[c = *(unsigned char*)cp++];
			if(mbwide() && n!=S_MBYTE && (len=mbsize(cp-1))>1)
			{
				sfwrite(stkp,cp-1, len);
				cp += --len;
				size -= len;
				continue;
			}
			if(n==S_ESC)
				sfputc(stkp,ESCAPE);
			else if(n==S_EPAT)
			{
				/* don't allow extended patterns in this case */
				mp->patfound = mp->pattern;
				sfputc(stkp,ESCAPE);
				if(!sh_isoption(SH_GLOBEX))
					mp->noextpat = 1;
			}
			else if(n==S_PAT)
				mp->patfound = mp->pattern;
			else if(n && mp->ifs)
			{
				if(mbwide() && n==S_MBYTE)
				{
					if(sh_strchr(mp->ifsp,cp-1)<0)
					{
						/*
						 * The multi-byte character that was found has the same initial
						 * byte as the IFS delimiter, but it's a different character. Put
						 * the first byte onto the stack and continue; multi-byte characters
						 * otherwise lose their initial byte.
						 */
						sfputc(stkp,c);
						continue;
					}
					n = mbsize(cp-1) - 1;
					if(n==-2)
						n = 0;
					cp += n;
					size -= n;
					n= S_DELIM;
				}
				if(n==S_SPACE || n==S_NL)
				{
					while(size>0 && ((n = ifs_state[c = *(unsigned char*)cp++])==S_SPACE || n==S_NL))
						size--;
					if(mbwide() && n==S_MBYTE && sh_strchr(mp->ifsp,cp-1)>=0)
					{
						n = mbsize(cp-1) - 1;
						if(n==-2)
							n = 0;
						cp += n;
						size -= n;
						n=S_DELIM;
					}
					else if(n==S_DELIM)
						size--;
				}
				endfield(mp,n==S_DELIM||mp->quoted);
				mp->patfound = 0;
				if(n==S_DELIM)
					while(size>0 && ((n = ifs_state[c = *(unsigned char*)cp++])==S_SPACE || n==S_NL))
						size--;
				if(size<=0)
					break;
				cp--;
				continue;

			}
			sfputc(stkp,c);
		}
		if(mp->pattern)
		{
			cp = "&|()";
			while(c = *cp++)
			{
				if(ifs_state[c]==S_EPAT)
					ifs_state[c] = 0;
			}
			cp = "*?[{";
			while(c = *cp++)
			{
				if(ifs_state[c]==S_PAT)
					ifs_state[c] = 0;
			}
			if(sh.ifstable[ESCAPE]==S_ESC)
				sh.ifstable[ESCAPE] = 0;
		}
	}
	else
		sfwrite(stkp,str,size);
}

/*
 * Terminate field.
 * If field is null count field if <split> is non-zero
 * Do filename expansion if required
 */
static void endfield(Mac_t *mp,int split)
{
	struct argnod	*argp;
	int		count=0;
	Stk_t		*stkp = sh.stk;
	if(stktell(stkp) > ARGVAL || split)
	{
		argp = stkfreeze(stkp,1);
		argp->argnxt.cp = 0;
		argp->argflag = 0;
		mp->atmode = 0;
		if(mp->patfound)
		{
			int musttrim = mp->wasexpan && !mp->quoted && !mp->noextpat && strchr(argp->argval,'\\');
			nv_setoptimize(NULL);
#if SHOPT_BRACEPAT
			/* in POSIX mode, disallow brace expansion for unquoted expansions */
			if(sh_isoption(SH_BRACEEXPAND) && !(sh_isoption(SH_POSIX) && mp->pattern==1))
				count = path_generate(argp,mp->arghead,musttrim);
			else
#endif /* SHOPT_BRACEPAT */
				count = path_expand(argp->argval,mp->arghead,musttrim);
			if(count)
				mp->fields += count;
			else if(split)	/* pattern is null string */
				*argp->argval = 0;
			else	/* pattern expands to nothing */
				count = -1;
		}
		if(count==0)
		{
			argp->argchn.ap = *mp->arghead;
			*mp->arghead = argp;
			mp->fields++;
		}
		if(count>=0)
		{
			(*mp->arghead)->argflag |= ARG_MAKE;
			if(mp->assign || sh_isoption(SH_NOGLOB))
				argp->argflag |= ARG_RAW|ARG_EXP;
		}
		stkseek(stkp,ARGVAL);
	}
	mp->quoted = mp->quote;
}

/*
 * Finds the right substring of STRING using the expression PAT
 * the longest substring is found when FLAG is set.
 */
static int substring(const char *string,size_t len,const char *pat,int match[], int flag)
{
	const char *sp=string;
	int size,nmatch,n;
	int smatch[2*(MATCH_MAX+1)];
	if(flag)
	{
		if(n=strngrpmatch(sp,len,pat,(ssize_t*)smatch,elementsof(smatch)/2,STR_RIGHT|STR_MAXIMAL|STR_INT))
		{
			memcpy(match,smatch,n*2*sizeof(smatch[0]));
			return n;
		}
		return 0;
	}
	size = (int)len;
	sp += size;
	while(sp>=string)
	{
		if(mbwide())
			sp = lastchar(string,sp);
		if(n=strgrpmatch(sp,pat,(ssize_t*)smatch,elementsof(smatch)/2,STR_RIGHT|STR_LEFT|STR_MAXIMAL|STR_INT))
		{
			nmatch = n;
			memcpy(match,smatch,n*2*sizeof(smatch[0]));
			size = sp-string;
			break;
		}
		sp--;
	}
	if(size==len)
		return 0;
	if(nmatch)
	{
		nmatch *=2;
		while(--nmatch>=0)
			match[nmatch] += size;
	}
	return n;
}

#if SHOPT_MULTIBYTE
	static char	*_lastchar(const char *string, const char *endstring)
	{
		char *str = (char*)string;
		int c;
		while(*str)
		{
			if((c=mbsize(str))<0)
				c = 1;
			if(str+c > endstring)
				break;
			str += c;
		}
		return str;
	}
#endif /* SHOPT_MULTIBYTE */
static int	charlen(const char *string,int len)
{
	if(!string)
		return 0;
	if(mbwide())
	{
		const char *str = string, *strmax=string+len;
		int n=0;
		if(len>0)
		{
			while(str<strmax && mbchar(str))
				n++;
		}
		else while(mbchar(str))
			n++;
		return n;
	}
	else
	{
		if(len<0)
			return strlen(string);
		return len;
	}
}

/*
 * <offset> is byte offset for beginning of tilde string
 */
static void tilde_expand2(int offset)
{
	char		*cp = NULL;			/* character pointer for tilde expansion result */
	char		*tp = stkptr(sh.stk,offset);	/* pointer to tilde string */
	int		curoff = stktell(sh.stk);	/* current offset of current stack object */
	sfputc(sh.stk,0);				/* terminate current stack object to avoid data corruption */
	/*
	 * Allow overriding tilde expansion with a .sh.tilde.set or .get discipline function.
	 */
	if(!sh.tilde_block && SH_TILDENOD->nvfun && SH_TILDENOD->nvfun->disc)
	{
		sh.tilde_block = 1;
		nv_putval(SH_TILDENOD, tp, 0);
		cp = nv_getval(SH_TILDENOD);
		sh.tilde_block = 0;
		if(cp[0]=='\0' || cp[0]=='~')
			cp = NULL;			/* do not use empty or unexpanded result */
	}
	/*
	 * Perform default tilde expansion unless overridden.
	 * Write the result to the stack, if any.
	 */
	if(!cp)
		cp = sh_tilde(tp);
	if(cp)
	{
		stkseek(sh.stk,offset);
		if(!(cp[0]=='/' && !cp[1] && fcpeek(0)=='/'))
			sfputr(sh.stk,cp,-1);		/* for ~ == /, avoid ~/foo -> //foo */
	}
	else
		stkseek(sh.stk,curoff);
}

/*
 * This routine is used to resolve ~ expansion.
 * A ~ by itself is replaced with the users login directory.
 * A ~- is replaced by the previous working directory in shell.
 * A ~+ is replaced by the present working directory in shell.
 * If ~name  is replaced with login directory of name.
 * If string doesn't start with ~ or ~... not found then 0 returned.
 */
static char *sh_tilde(const char *string)
{
	char		*cp;
	int		c;
	struct passwd	*pw = NULL;
	Namval_t	*np=0;
	unsigned int	save;
	static Dt_t	*logins_tree;
	if(*string++!='~')
		return NULL;
	if((c = *string)==0)
	{
		static char	*username;
		if(cp = nv_getval(sh_scoped(HOME)))
			return cp;
		/* Fallback for unset HOME: get username and treat ~ like ~username */
		if(!username && !((pw = getpwuid(sh.userid)) && (username = sh_strdup(pw->pw_name))))
			return NULL;
		string = username;
	}
	if((c=='-' || c=='+') && string[1]==0)
	{
		if(c=='+')
			cp = nv_getval(sh_scoped(PWDNOD));
		else
			cp = nv_getval(sh_scoped(OLDPWDNOD));
		return cp;
	}
#if _WINIX
	if((c = fcgetc())=='/')
	{
		char	*str;
		int	n=0,offset=stktell(sh.stk);
		sfputr(sh.stk,string,-1);
		do
		{
			sfputc(sh.stk,c);
			n++;
		}
		while ((c = fcgetc()) && c!='/');
		sfputc(sh.stk,0);
		if(c)
			fcseek(-1);
		str = stkseek(sh.stk,offset);
		Skip = n;
		if(logins_tree && (np=nv_search(str,logins_tree,0)))
			return nv_getval(np);
		if(pw = getpwnam(str))
		{
			string = str;
			goto skip;
		}
		Skip = 0;
	}
#endif /* _WINIX */
	if(logins_tree && (np=nv_search(string,logins_tree,0)))
		return nv_getval(np);
	if(!pw && !(pw = getpwnam(string)))
		return NULL;
#if _WINIX
skip:
#endif /* _WINIX */
	if(!logins_tree)
		logins_tree = dtopen(&_Nvdisc,Dtbag);
	if(np=nv_search(string,logins_tree,NV_ADD))
	{
		save = sh.subshell;
		sh.subshell = 0;
		nv_putval(np, pw->pw_dir,0);
		sh.subshell = save;
	}
	return pw->pw_dir;
}

/*
 * return values for special macros
 */
static char *special(int c)
{
	if(c!='$')
		nv_setoptimize(NULL);
	switch(c)
	{
	    case '@':
	    case '*':
		return sh.st.dolc>0?sh.st.dolv[1]:NULL;
	    case '#':
#if  SHOPT_FILESCAN
		if(sh.cur_line)
		{
			getdolarg(MAX_ARGN,NULL);
			return ltos(sh.offsets[0]);
		}
#endif  /* SHOPT_FILESCAN */
		return ltos(sh.st.dolc);
	    case '!':
		if(sh.bckpid)
			return ltos(sh.bckpid);
		break;
	    case '$':
		return ltos(sh.pid);
	    case '-':
		return sh_argdolminus(sh.arg_context);
	    case '?':
		return ltos(sh.savexit);
	    case 0:
		if(sh_isstate(SH_PROFILE) || sh.fn_depth==0 || !sh.st.cmdname)
			return sh.shname;
		else
			return sh.st.cmdname;
	}
	/* Handle 'set -u'/'set -o nounset' for special parameters */
	if(sh_isoption(SH_NOUNSET))
	{
		unsigned char d = fcpeek(0);
		if(!(d && strchr(":+-?=",d)))
		{
			char c_str[2];
			c_str[0]=(char)c;
			c_str[1]='\0';
			errormsg(SH_DICT,ERROR_exit(1),e_notset,c_str);
			UNREACHABLE();
		}
	}
	return NULL;
}

/*
 * Handle macro expansion errors
 */
static noreturn void mac_error(void)
{
	errormsg(SH_DICT,ERROR_exit(1),e_subst,fcfirst());
	UNREACHABLE();
}

/*
 * Given pattern/string, replace / with 0 and return pointer to string
 * \ characters are stripped from string.  The \ are stripped in the
 * replacement string unless followed by a digit or \.
 */
static char *mac_getstring(char *pattern)
{
	char	*cp = pattern, *rep = NULL, *dp = NULL;
	int	c;
	while(c = *cp++)
	{
		if(c==ESCAPE && (!rep || (*cp && strchr("&|()[]*?",*cp))))
		{
			c = *cp++;
		}
		else if(!rep && c=='/')
		{
			cp[-1] = 0;
			rep = dp = cp;
			continue;
		}
		if(rep)
			*dp++ = c;
	}
	if(rep)
		*dp = 0;
	return rep;
}

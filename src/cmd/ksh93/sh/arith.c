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
*         hyenias <58673227+hyenias@users.noreply.github.com>          *
*             dnewhall <dnewhall@users.noreply.github.com>             *
*                                                                      *
***********************************************************************/
/*
 * Shell arithmetic - uses streval library
 *   David Korn
 *   AT&T Labs
 */

#include	"shopt.h"
#include	"defs.h"
#include	"lexstates.h"
#include	"name.h"
#include	"streval.h"
#include	"variables.h"
#include	"builtins.h"
#include	<ast_release.h>

#ifndef LLONG_MAX
#define LLONG_MAX	LONG_MAX
#endif

typedef Sfdouble_t (*Math_f)(Sfdouble_t, ...);

extern const Namdisc_t	ENUM_disc;
static Sfdouble_t	NaN = NAN, Inf = INFINITY, Fun;
static Namval_t Infnod =
{
	{ 0 },
	"Inf",
};

static Namval_t NaNnod =
{
	{ 0 },
	"NaN",
};

static Namval_t FunNode =
{
	{ 0 },
	"?",
};

static Namval_t *scope(Namval_t *np,struct lval *lvalue,int assign)
{
	int	flag = lvalue->flag;
	char	*sub=0, *cp=(char*)np;
	Namval_t *mp;
	int	c=0,nosub = lvalue->nosub;
	Dt_t	*sdict = (sh.st.real_fun? sh.st.real_fun->sdict:0);
	Dt_t	*nsdict = (sh.namespace?nv_dict(sh.namespace):0);
	Dt_t	*root = sh.var_tree;
	assign = assign?NV_ASSIGN:0;
	lvalue->nosub = 0;
	if(nosub<0 && lvalue->ovalue)
		return (Namval_t*)lvalue->ovalue;
	lvalue->ovalue = 0;
	if(cp>=lvalue->expr &&  cp < lvalue->expr+lvalue->elen)
	{
		int offset;
		/* do binding to node now */
		int c = cp[flag];
		cp[flag] = 0;
		if((!(np = nv_open(cp,sh.var_tree,assign|NV_VARNAME|NV_NOADD|NV_NOFAIL)) || nv_isnull(np))
		&& sh_macfun(cp, offset = stktell(sh.stk)))
		{
			Fun = sh_arith(sub=stkptr(sh.stk,offset));
			FunNode.nvalue = &Fun;
			nv_onattr(&FunNode,NV_NOFREE|NV_LDOUBLE|NV_RDONLY);
			cp[flag] = c;
			return &FunNode;
		}
		if(!np && assign)
			np = nv_open(cp,sh.var_tree,assign|NV_VARNAME);
		cp[flag] = c;
		if(!np)
			return NULL;
		root = sh.last_root;
		if(cp[flag+1]=='[')
			flag++;
		else
			flag = 0;
		cp = (char*)np;
	}
	if((lvalue->emode & ARITH_COMP) && dtvnext(root))
	{
		if(mp = nv_search(cp, sdict ? sdict : root, NV_NOSCOPE|NV_REF))
			np = mp;
		else if(nsdict && (mp = nv_search(cp, nsdict, NV_REF)))
			np = mp;
	}
	while(nv_isref(np))
	{
#if SHOPT_FIXEDARRAY
		int n,dim;
		dim = nv_refdimen(np);
		n = nv_refindex(np);
#endif /* SHOPT_FIXEDARRAY */
		sub = nv_refsub(np);
		np = nv_refnode(np);
#if SHOPT_FIXEDARRAY
		if(n)
		{
			Namarr_t *ap = nv_arrayptr(np);
			ap->nelem = dim;
			nv_putsub(np,NULL,n);
		}
		else
#endif /* SHOPT_FIXEDARRAY */
		if(sub)
			nv_putsub(np,sub,assign==NV_ASSIGN?ARRAY_ADD:0);
	}
	if(!nosub && flag)
	{
		int		hasdot = 0;
		cp = (char*)&lvalue->expr[flag];
		if(sub)
		{
			goto skip;
		}
		sub = cp;
		while(1)
		{
			Namarr_t	*ap;
			Namval_t	*nq;
			cp = nv_endsubscript(np,cp,0);
			if(c || *cp=='.')
			{
				c = '.';
				while(*cp=='.')
				{
					hasdot=1;
					cp++;
					while(c=mbchar(cp),isaname(c));
				}
				if(c=='[')
				{
					cp--;
					continue;
				}
			}
			flag = *cp;
			*cp = 0;
			if(c || hasdot)
			{
				sfputr(sh.strbuf,nv_name(np),-1);
				sfputr(sh.strbuf,sub,-1);
				sub = sfstruse(sh.strbuf);
			}
			*cp = flag;
			if(c || hasdot)
			{
				np = nv_open(sub,sh.var_tree,NV_VARNAME|assign);
				return np;
			}
#if SHOPT_FIXEDARRAY
			ap = nv_arrayptr(np);
			cp = nv_endsubscript(np,sub,NV_ADD|(ap&&ap->fixed?NV_FARRAY:0));
#else
			cp = nv_endsubscript(np,sub,NV_ADD);
#endif /* SHOPT_FIXEDARRAY */
			if(*cp!='[')
				break;
		skip:
			if(nq = nv_opensub(np))
				np = nq;
			else
			{
				ap = nv_arrayptr(np);
				if(ap && !ap->table)
					ap->table = dtopen(&_Nvdisc,Dtoset);
				if(ap && ap->table && (nq=nv_search(nv_getsub(np),ap->table,NV_ADD)))
					nq->nvmeta = np;
				if(nq && nv_isnull(nq))
					np = nv_arraychild(np,nq,0);
			}
			sub = cp;
		}
	}
	else if(nosub>0)
		nv_putsub(np,NULL,nosub-1);
	return np;
}

/* look up a function in the standard math function table */
static Math_f sh_mathstdfun(const char *fname, size_t fsize, short * nargs)
{
	const struct mathtab *tp;
	char c = fname[0];
	/* first byte of tp->fname is num. args and return type, unless empty */
	for(tp=shtab_math; *tp->fname; tp++)
	{
		if(tp->fname[1]==c && strncmp(&tp->fname[1],fname,fsize)==0 && tp->fname[fsize+1]==0)
		{
			if(nargs)
				*nargs = *tp->fname;
			return tp->fnptr;
		}
	}
	return NULL;
}

int	sh_mathstd(const char *name)
{
	return sh_mathstdfun(name,strlen(name),NULL)!=0;
}

static Sfdouble_t arith(const char **ptr, struct lval *lvalue, int type, Sfdouble_t n)
{
	Sfdouble_t r= 0;
	char *str = (char*)*ptr;
	char *cp;
	switch(type)
	{
	    case ASSIGN:
	    {
		Namval_t *np;
		unsigned short attr;
		if (lvalue->sub && lvalue->nosub > 0) /* indexed array ARITH_ASSIGNOP */
		{
			np = (Namval_t*)lvalue->sub; /* use saved subscript reference instead of last worked value */
			nv_putsub(np, NULL, lvalue->nosub-1);
			/* reset nosub and sub for next assignment in a compound arithmetic expression */
			lvalue->nosub = 0;
			lvalue->sub = 0;
		}
		else
		{
			np = (Namval_t*)lvalue->value;
			np = scope(np, lvalue, 1);
		}
		if(nv_isattr(np,NV_UINT16)==NV_UINT16)
		{
			Namfun_t *fp = nv_hasdisc(np, &ENUM_disc);
			if(fp && (n < 0.0 || n > (Sfdouble_t)(b_enum_nelem(fp) - 1)))
			{
				errormsg(SH_DICT, ERROR_exit(1), "%s: value %ld out of enum range", nv_name(np), (long)n);
				UNREACHABLE();
			}
		}
		nv_putval(np, (char*)&n, NV_LDOUBLE);
		if(lvalue->isenum)
			lvalue->enum_p = nv_hasdisc(np,&ENUM_disc);
		lvalue->isenum = 0;
		lvalue->value = (char*)np;
		/*
		 * The result (r) of an assignment is its value (n), cast to the type of the variable
		 * assigned to. We cannot simply reobtain the value with nv_getnum() to effectuate the
		 * typecast, because that may trigger a getnum discipline function with side effects.
		 * The order of checks below is essential due to how the bit masks work (see nval.h).
		 */
		attr = nv_isnum(np);
		if(!attr || (attr & NV_LDOUBLE)==NV_LDOUBLE)	/* long float */
			r = n;
		else if((attr & NV_FLOAT)==NV_FLOAT)		/* short float */
			r = (float)n;
		else if((attr & NV_DOUBLE)==NV_DOUBLE)		/* normal float */
			r = (double)n;
		/* Avoid typecasting a negative float (Sfdouble_t) to an
		 * unsigned integer (uint*_t), which is undefined behaviour */
		else if((attr & NV_UINT64)==NV_UINT64)		/* long unsigned integer */
			r = n < 0 ? -((uintmax_t)(-n)) : (uintmax_t)n;
		else if((attr & NV_UINT16)==NV_UINT16)		/* short unsigned integer */
			r = n < 0 ? -((uint16_t)(-n)) : (uint16_t)n;
		else if((attr & NV_UINT32)==NV_UINT32)		/* normal unsigned integer */
			r = n < 0 ? -((uint32_t)(-n)) : (uint32_t)n;
		else if((attr & NV_INT64)==NV_INT64)		/* long signed integer */
			r = (intmax_t)n;
		else if((attr & NV_INT16)==NV_INT16)		/* short signed integer */
			r = (int16_t)((intmax_t)n);
		else if((attr & NV_INT32)==NV_INT32)		/* normal signed integer */
			r = (int32_t)((intmax_t)n);
#if _AST_release
		else	r = n;					/* should never happen */
#else
		else	abort();
#endif
		break;
	    }
	    case LOOKUP:
	    {
		int c = *str;
		char *xp=str;
		lvalue->value = NULL;
		if(c=='.')
			str++;
		c = mbchar(str);
		if(isaletter(c))
		{
			Namval_t *np=0;
			int dot=0;
			while(1)
			{
				while(xp=str, c=mbchar(str), isaname(c));
				str = xp;
				while(c=='[' && dot==NV_NOADD)
				{
					str = nv_endsubscript(NULL,str,0);
					c = *str;
				}
				if(c!='.')
					break;
				dot=NV_NOADD;
				if((c = *++str) !='[')
					continue;
				str = nv_endsubscript(NULL,cp=str,0)-1;
				if(sh_checkid(cp+1,NULL))
					str -=2;
			}
			if(c=='(')
			{
				int off=stktell(sh.stk);
				int fsize = str- (char*)(*ptr);
				const struct mathtab *tp;
				c = **ptr;
				lvalue->fun = 0;
				sfprintf(sh.stk,".sh.math.%.*s%c",fsize,*ptr,0);
				stkseek(sh.stk,off);
				if(np=nv_search(stkptr(sh.stk,off),sh.fun_tree,0))
				{
					struct Ufunction *rp = np->nvalue;
					lvalue->nargs = -rp->argc;
					lvalue->fun = (Math_f)np;
					break;
				}
				if(fsize<=(sizeof(tp->fname)-2))
					lvalue->fun = (Math_f)sh_mathstdfun(*ptr,fsize,&lvalue->nargs);
				if(lvalue->fun)
					break;
				if(lvalue->emode&ARITH_COMP)
					lvalue->value = (char*)e_function;
				else
					lvalue->value = (char*)ERROR_dictionary(e_function);
				return r;
			}
			if((lvalue->emode&ARITH_COMP) && dot)
			{
				lvalue->value = (char*)*ptr;
				lvalue->flag =  str-lvalue->value;
				break;
			}
			*str = 0;
			if(sh_isoption(SH_NOEXEC))
				np = L_ARGNOD;
			else
			{
				int offset = stktell(sh.stk);
				char *saveptr = stkfreeze(sh.stk,0);
				Dt_t  *root = (lvalue->emode&ARITH_COMP)?sh.var_base:sh.var_tree;
				*str = c;
				cp = str;
				while(c=='[' || c=='.')
				{
					if(c=='[')
					{
						str = nv_endsubscript(np,str,0);
						if((c= *str)!='[' &&  c!='.')
						{
							str = cp;
							c = '[';
							break;
						}
					}
					else
					{
						dot = NV_NOADD|NV_NOFAIL;
						str++;
						while(xp=str, c=mbchar(str), isaname(c));
						str = xp;
					}
				}
				*str = 0;
				cp = (char*)*ptr;
				if(!sh_isoption(SH_POSIX) && (cp[0] == 'i' || cp[0] == 'I') && (cp[1] == 'n' || cp[1] == 'N') && (cp[2] == 'f' || cp[2] == 'F') && cp[3] == 0)
				{
					if (!Infnod.nvalue)
					{
						Infnod.nvalue = &Inf;
						nv_onattr(&Infnod,NV_NOFREE|NV_LDOUBLE|NV_RDONLY);
					}
					np = &Infnod;
				}
				else if(!sh_isoption(SH_POSIX) && (cp[0] == 'n' || cp[0] == 'N') && (cp[1] == 'a' || cp[1] == 'A') && (cp[2] == 'n' || cp[2] == 'N') && cp[3] == 0)
				{
					if (!NaNnod.nvalue)
					{
						NaNnod.nvalue = &NaN;
						nv_onattr(&NaNnod,NV_NOFREE|NV_LDOUBLE|NV_RDONLY);
					}
					np = &NaNnod;
				}
				else if(!(np = nv_open(*ptr,root,NV_NOREF|NV_VARNAME|dot)))
				{
					lvalue->value = (char*)*ptr;
					lvalue->flag =  str-lvalue->value;
				}
				if(saveptr != stkptr(sh.stk,0))
					stkset(sh.stk,saveptr,offset);
				else
					stkseek(sh.stk,offset);
			}
			*str = c;
			if(!np && lvalue->value)
				break;
			lvalue->value = (char*)np;
			/* bind subscript later */
			if(nv_isattr(np,NV_DOUBLE)==NV_DOUBLE)
				lvalue->isfloat=1;
			lvalue->flag = 0;
			if(c=='[')
			{
				lvalue->flag = (str-lvalue->expr);
				do
				{
					while(c=='.')
					{
						str++;
						while(xp=str, c=mbchar(str), isaname(c));
						c = *(str = xp);
					}
					if(c=='[')
						str = nv_endsubscript(np,str,0);
				}
				while((c= *str)=='[' || c=='.');
				break;
			}
		}
		else
		{
			char	lastbase=0, *val = xp, oerrno = errno;
			lvalue->isenum = 0;
			errno = 0;
			r = strtonll(val,&str, &lastbase,-1);
			if(lastbase==8 && *val=='0' && !sh_isoption(sh.bltinfun==b_let ? SH_LETOCTAL : SH_POSIX))
			{
				/* disable leading-0 octal by reparsing as decimal */
				lastbase=10;
				errno = 0;
				r = strtonll(val,&str, &lastbase,-1);
			}
			if(lastbase<=1)
				lastbase=10;
			if(*val=='0')
			{
				while(*val=='0')
					val++;
				if(*val==0 || *val=='.' || *val=='x' || *val=='X')
					val--;
			}
			if(r==LLONG_MAX && errno)
				c='e';
			else
				c = *str;
			if(c=='.' && sh.radixpoint!='.')
			{
				sfprintf(sh.strbuf, "%s: radix point '.' requires LC_NUMERIC=C", val);
				lvalue->value = sfstruse(sh.strbuf);
				return r;
			}
			if(c==sh.radixpoint || c=='e' || c == 'E' || lastbase == 16 && (c == 'p' || c == 'P'))
			{
				lvalue->isfloat=1;
				r = strtold(val,&str);
			}
			else if(lastbase==10 && val[1])
			{
				if(val[2]=='#')
					val += 3;
				if((str-val)>2*sizeof(Sflong_t))
				{
					Sfdouble_t rr;
					rr = strtold(val,&str);
					if(rr!=r)
					{
						r = rr;
						lvalue->isfloat=1;
					}
				}
			}
			errno = oerrno;
		}
		break;
	    }
	    case VALUE:
	    {
		Namval_t *np = (Namval_t*)(lvalue->value);
		if(sh_isoption(SH_NOEXEC))
			return 0;
		np = scope(np,lvalue,0);
		if(!np)
		{
			if(sh_isoption(SH_NOUNSET))
			{
				*ptr = lvalue->value;
				goto skip;
			}
			return 0;
		}
		lvalue->ovalue = (char*)np;
		if(lvalue->isenum)
		{
			lvalue->enum_p = nv_hasdisc(np,&ENUM_disc);
			lvalue->isenum = 0;
		}
		else if(lvalue->enum_p && !nv_hasdisc(np,&ENUM_disc) && !nv_isattr(np,NV_INTEGER))
		{
			/* convert the enum rvalue of the lvalue's enum type to its number */
			Namval_t *mp,node;
			mp = ((Namfun_t*)lvalue->enum_p)->type;
			memset(&node,0,sizeof(node));
			nv_clone(mp,&node,0);
			nv_offattr(&node,NV_RDONLY|NV_NOFREE);
			nv_putval(&node,np->nvname,0);
			r = nv_getnum(&node);
			_nv_unset(&node,0);
			return r;
		}
		if(((lvalue->emode&2) || lvalue->level>1 || sh_isoption(SH_NOUNSET)) && nv_isnull(np) && !nv_isattr(np,NV_INTEGER))
		{
			*ptr = nv_name(np);
		skip:
			lvalue->value = (char*)ERROR_dictionary(e_notset);
			lvalue->emode |= 010;
			return 0;
		}
		r = nv_getnum(np);
		if(nv_isattr(np,NV_INTEGER|NV_BINARY)==(NV_INTEGER|NV_BINARY))
			lvalue->isfloat= (r!=(Sflong_t)r);
		else if(nv_isattr(np,NV_DOUBLE)==NV_DOUBLE)
			lvalue->isfloat=1;
		if((lvalue->emode&ARITH_ASSIGNOP) && nv_isarray(np))
		{
			lvalue->nosub = nv_aindex(np)+1; /* subscript number of array */
			lvalue->sub = (char*)np; /* store subscript reference for upcoming iteration of ASSIGN */
		}
		return r;
	    }
	    case MESSAGE:
		sfsync(NULL);
		if(lvalue->emode&ARITH_COMP)
			return -1;
		errormsg(SH_DICT,ERROR_exit((lvalue->emode&3)!=0),lvalue->value,*ptr);
	}
	*ptr = str;
	return r;
}

/*
 * convert number defined by string to a Sfdouble_t
 * ptr is set to the last character processed
 * if mode>0, an error will be fatal with value <mode>
 */
Sfdouble_t sh_strnum(const char *str, char** ptr, int mode)
{
	Sfdouble_t d;
	char base = (sh_isoption(sh.bltinfun==b_let ? SH_LETOCTAL : SH_POSIX) ? 0 : 10), *last;
	if(*str==0)
	{
		d = 0.0;
		last = (char*)str;
	}
	else
	{
		errno = 0;
		d = strtonll(str,&last,&base,-1);
		if(*last && sh_isstate(SH_INIT))
		{
			/* Handle floating point or "base#value" literals if we're importing untrusted env vars. */
			errno = 0;
			if(*last==sh.radixpoint)
				d = strtold(str,&last);
			else
				d = strtonll(str,&last,NULL,-1);
		}
		if(*last || errno)
		{
			if(sh_isstate(SH_INIT))
				/*
				 * Initializing means importing untrusted env vars. The string does not appear to be
				 * a recognized numeric literal, so give up. We can't safely call arith_strval(), because
				 * that allows arbitrary expressions, which could be a security vulnerability.
				 */
				d = 0.0;
			else
			{
				if(!last || *last!=sh.radixpoint || last[1]!=sh.radixpoint)
					d = arith_strval(str,&last,arith,mode);
				if(!ptr && *last && mode>0)
				{
					errormsg(SH_DICT,ERROR_exit(1),e_lexbadchar,*last,str);
					UNREACHABLE();
				}
			}
		}
		else if(!d && *str=='-')
			d = -0.0;
	}
	if(ptr)
		*ptr = last;
	return d;
}

Sfdouble_t sh_arith(const char *str)
{
	return sh_strnum(str, NULL, 1);
}

void	*sh_arithcomp(char *str)
{
	const char *ptr = str;
	Arith_t *ep;
	ep = arith_compile(str,(char**)&ptr,arith,ARITH_COMP|1);
	if(*ptr)
	{
		errormsg(SH_DICT,ERROR_exit(1),e_lexbadchar,*ptr,str);
		UNREACHABLE();
	}
	return ep;
}

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
 * string processing routines for Korn shell
 *
 */

#include	"shopt.h"
#include	<ast.h>
#include	<ast_wchar.h>
#include	"defs.h"
#include	"shtable.h"
#include	"lexstates.h"
#include	"national.h"

#if _hdr_wctype
#   include <wctype.h>
#endif

#if !_lib_iswprint && !defined(iswprint)
#   define iswprint(c)		(((c)&~0377) || isprint(c))
#endif

/*
 *  Table lookup routine
 *  <table> is searched for string <sp> and corresponding value is returned
 *  This is only used for small tables and is used to save non-shareable memory
 *  NOTE: assumes tables are sorted by sh_name in ASCII order!
 */
const Shtable_t *sh_locate(const char *sp,const Shtable_t *table,int size)
{
	int			first;
	const Shtable_t		*tp;
	int			c;
	static const Shtable_t	empty = {0,0};
	if(sp==0 || (first= *sp)==0)
		return &empty;
	tp=table;
	while((c = *tp->sh_name) && c <= first)
	{
		if(first == c && strcmp(sp,tp->sh_name)==0)
			return tp;
		tp = (Shtable_t*)((char*)tp+size);
	}
	return &empty;
}

/*
 *  shtab_options lookup routine
 *
 *  Long-form option names are case-sensitive but insensitive to '-' and '_', and may be abbreviated to a
 *  non-arbitrary string. A no- prefix is skipped and inverts the meaning (special handling for 'notify').
 *  The table must be sorted in ASCII order after skipping the no- prefix.
 *
 *  Returns 0 if not found, -1 if multiple found (ambiguous), or the number of the option found.
 */

#define sep(c)		((c)=='-'||(c)=='_')

int sh_lookopt(const char *sp, int *invert)
{
	int			first;
	const Shtable_t		*tp;
	int			c;
	const char		*s, *t, *sw, *tw;
	int			amb;
	int			hit;
	int			inv;
	int			no;
	if(sp==0)
		return 0;
	if(*sp=='n' && *(sp+1)=='o' && (*(sp+2)!='t' || *(sp+3)!='i'))
	{
		sp+=2;
		if(sep(*sp))
			sp++;
		*invert = !*invert;
	}
	if((first= *sp)==0)
		return 0;
	tp=shtab_options;
	amb=hit=0;
	for(;;)
	{
		t=tp->sh_name;
		if(no = *t=='n' && *(t+1)=='o' && *(t+2)!='t')
			t+=2;
		if(!(c= *t))
			break;
		if(first == c)
		{
			if(strcmp(sp,t)==0)
			{
				*invert ^= no;
				return tp->sh_number;
			}
			s=sw=sp;
			tw=t;
			for(;;)
			{
				if(!*s || *s=='=')
				{
					if (*s == '=' && !strtol(s+1, NULL, 0))
						no = !no;
					if (!*t)
					{
						*invert ^= no;
						return tp->sh_number;
					}
					if (hit || amb)
					{
						hit = 0;
						amb = 1;
					}
					else
					{
						hit = tp->sh_number;
						inv = no;
					}
					break;
				}
				else if(!*t)
					break;
				else if(sep(*s))
					sw = ++s;
				else if(sep(*t))
					tw = ++t;
				else if(*s==*t)
				{
					s++;
					t++;
				}
				else if(s==sw && t==tw)
					break;
				else
				{
					if(t!=tw)
					{
						while(*t && !sep(*t))
							t++;
						if(!*t)
							break;
						tw = ++t;
					}
					while (s>sw && *s!=*t)
						s--;
				}
			}
		}
		tp = (Shtable_t*)((char*)tp+sizeof(*shtab_options));
	}
	if(hit)
		*invert ^= inv;
	return amb ? -1 : hit;
}

/*
 * look for the substring <oldsp> in <string> and replace with <newsp>
 * The new string is put on top of the stack
 */
char *sh_substitute(const char *string,const char *oldsp,char *newsp)
/*@
	assume string!=NULL && oldsp!=NULL && newsp!=NULL;
	return x satisfying x==NULL ||
		strlen(x)==(strlen(in string)+strlen(in newsp)-strlen(in oldsp));
@*/
{
	const char *sp = string;
	const char *cp;
	const char *savesp = 0;
	stkseek(sh.stk,0);
	if(*sp==0)
		return NULL;
	if(*(cp=oldsp) == 0)
		goto found;
	do
	{
	/* skip to first character which matches start of oldsp */
		while(*sp && (savesp==sp || *sp != *cp))
		{
#if SHOPT_MULTIBYTE
			/* skip a whole character at a time */
			int c = mbsize(sp);
			if(c < 0)
				sp++;
			while(c-- > 0)
#endif /* SHOPT_MULTIBYTE */
			sfputc(sh.stk,*sp++);
		}
		if(*sp == 0)
			return NULL;
		savesp = sp;
	        for(;*cp;cp++)
		{
			if(*cp != *sp++)
				break;
		}
		if(*cp==0)
		/* match found */
			goto found;
		sp = savesp;
		cp = oldsp;
	}
	while(*sp);
	return NULL;

found:
	/* copy new */
	sfputr(sh.stk,newsp,-1);
	/* copy rest of string */
	sfputr(sh.stk,sp,-1);
	return stkfreeze(sh.stk,1);
}

/*
 * TRIM(sp)
 * Remove escape characters from characters in <sp> and eliminate quoted nulls.
 */
void	sh_trim(char *sp)
/*@
	assume sp!=NULL;
	promise strlen(in sp) <= in strlen(sp);
@*/
{
	char *dp;
	int c;
	if(sp)
	{
		dp = sp;
		while(c= *sp)
		{
			int len;
			if(mbwide() && (len=mbsize(sp))>1)
			{
				memmove(dp, sp, len);
				dp += len;
				sp += len;
				continue;
			}
			sp++;
			if(c == '\\')
				c = *sp++;
			if(c)
				*dp++ = c;
		}
		*dp = 0;
	}
}

/*
 * format string as a csv field
 */
static char	*sh_fmtcsv(const char *string)
{
	const char *cp = string;
	int c;
	int offset;
	if(!cp)
		return NULL;
	offset = stktell(sh.stk);
	while((c=mbchar(cp)),isaname(c));
	if(c==0)
		return (char*)string;
	sfputc(sh.stk,'"');
	sfwrite(sh.stk,string,cp-string);
	if(c=='"')
		sfputc(sh.stk,'"');
	string = cp;
	while(c=mbchar(cp))
	{
		if(c=='"')
		{
			sfwrite(sh.stk,string,cp-string);
			string = cp;
			sfputc(sh.stk,'"');
		}
	}
	if(--cp>string)
		sfwrite(sh.stk,string,cp-string);
	sfputc(sh.stk,'"');
	sfputc(sh.stk,0);
	return stkptr(sh.stk,offset);
}

/*
 * Returns false if c is an invisible Unicode character, excluding ASCII space.
 * Use iswgraph(3) if possible. In the ksh-specific C.UTF-8 locale, this is
 * generally not possible as the OS-provided iswgraph(3) doesn't support that
 * locale. So do a quick test and do our best with a fallback if necessary.
 */
static int	sh_isprint(int c)
{
	if(!mbwide() || c<=127)				/* not in multibyte locale, or multibyte but c is ASCII? */
		return isprint(c);			/* use plain isprint(3) */
	else if(!(ast.locale.set & AST_LC_utf8))	/* not in UTF-8 locale? */
		return iswgraph(c);			/* the test below would not be valid */
	else if(iswgraph(0x5E38) && !iswgraph(0xFEFF))	/* can we use iswgraph(3)? */
		return iswgraph(c);			/* use iswgraph(3) */
	else						/* fallback: */
		return !(c <= 0x001F ||			/* control characters */
			c >= 0x007F && c <= 0x009F ||	/* control characters */
			c == 0x00A0 ||			/* non-breaking space */
			c == 0x061C ||			/* arabic letter mark */
			c == 0x1680 ||			/* ogham space mark */
			c == 0x180E ||			/* mongolian vowel separator */
			c >= 0x2000 && c <= 0x200F ||	/* spaces and format characters */
			c >= 0x2028 && c <= 0x202F ||	/* separators and format characters */
			c >= 0x205F && c <= 0x206F ||	/* various format characters */
			c == 0x3000 ||			/* ideographic space */
			c == 0xFEFF);			/* zero-width non-breaking space */
}

/*
 * print <str> quoting chars so that it can be read by the shell
 * puts null-terminated result on stack, but doesn't freeze it
 */
char	*sh_fmtq(const char *string)
{
	const char *cp = string, *op;
	int c, state;
	int offset;
	if(!cp)
		return NULL;
	offset = stktell(sh.stk);
	state = ((c= mbchar(cp))==0);
	if(isaletter(c) || c=='.')
	{
		while((c=mbchar(cp)), isaname(c) || c=='.')
			;
		if(c==0)
			return (char*)string;
		if(c=='=' || c=='+' && *cp=='=')
		{
			if(*cp==0)
				return (char*)string;
			if(*cp=='=')
				cp++;
			c = cp - string;
			sfwrite(sh.stk,string,c);
			string = cp;
			c = mbchar(cp);
		}
	}
	if(c==0 || c=='#' || c=='~')
		state = 1;
	for(;c;c= mbchar(cp))
	{
		if(c=='\'' || !sh_isprint(c))
			state = 2;
		else if(c==']' || c=='=' || (c!=':' && c<=0x7f && (c=sh_lexstates[ST_NORM][c]) && c!=S_EPAT))
			state |=1;
	}
	if(state<2)
	{
		if(state==1)
			sfputc(sh.stk,'\'');
		if(c = --cp - string)
			sfwrite(sh.stk,string,c);
		if(state==1)
			sfputc(sh.stk,'\'');
	}
	else
	{
		sfwrite(sh.stk,"$'",2);
		cp = string;
		while(op = cp, c= mbchar(cp))
		{
			state=1;
			switch(c)
			{
			    case '\033':	/* ASCII ESC */
				c = 'E';
				break;
			    case '\n':
				c = 'n';
				break;
			    case '\r':
				c = 'r';
				break;
			    case '\t':
				c = 't';
				break;
			    case '\f':
				c = 'f';
				break;
			    case '\b':
				c = 'b';
				break;
			    case '\a':
				c = 'a';
				break;
			    case '\\':	case '\'':
				break;
			    default:
				if(mbwide())
				{
					/* We're in a multibyte locale */
					if(c<0 || c<128 && !isprint(c))
					{
						/* Invalid multibyte char, or unprintable ASCII char: quote as hex byte */
						c = *((unsigned char *)op);
						cp = op+1;
						goto quote_one_byte;
					}
					if(!sh_isprint(c))
					{
						/* Unicode hex code */
						sfprintf(sh.stk,"\\u[%x]",c);
						continue;
					}
				}
				else if(!isprint(c))
				{
				quote_one_byte:
					sfprintf(sh.stk, isxdigit(*cp) ? "\\x[%.2x]" : "\\x%.2x", c);
					continue;
				}
				state=0;
				break;
			}
			if(state)
			{
				sfputc(sh.stk,'\\');
				sfputc(sh.stk,c);
			}
			else
				sfwrite(sh.stk,op, cp-op);
		}
		sfputc(sh.stk,'\'');
	}
	sfputc(sh.stk,0);
	return stkptr(sh.stk,offset);
}

/*
 * print <str> quoting chars so that it can be read by the shell
 * puts null-terminated result on stack, but doesn't freeze it
 * single!=0 limits quoting to '...'
 * fold>0 prints raw newlines and inserts appropriately
 * escaped newlines every (fold-x) chars
 */
char	*sh_fmtqf(const char *string, int single, int fold)
{
	const char *cp = string;
	const char *bp;
	const char *vp;
	int c;
	int n;
	int q;
	int a;
	int offset;

	if (--fold < 8)
		fold = 0;
	if(single)
		return sh_fmtcsv(cp);
	if (!cp || !*cp || !fold || fold && strlen(string) < fold)
		return sh_fmtq(cp);
	offset = stktell(sh.stk);
	single = single ? 1 : 3;
	c = mbchar(string);
	a = isaletter(c) ? '=' : 0;
	vp = cp + 1;
	do
	{
		q = 0;
		n = fold;
		bp = cp;
		while ((!n || n-- > 0) && (c = mbchar(cp)))
		{
			if (a && !isaname(c))
				a = 0;
#if SHOPT_MULTIBYTE
			if (c >= 0x200)
				continue;
			if (c == '\'' || !iswprint(c))
#else
			if (c == '\'' || !isprint(c))
#endif /* SHOPT_MULTIBYTE */
			{
				q = single;
				break;
			}
			if (c == '\n')
				q = 1;
			else if (c == a)
			{
				sfwrite(sh.stk,bp, cp - bp);
				bp = cp;
				vp = cp + 1;
				a = 0;
			}
			else if ((c == '#' || c == '~') && cp == vp || c == ']' || c != ':' && (c = sh_lexstates[ST_NORM][c]) && c != S_EPAT)
				q = 1;
		}
		if (q & 2)
		{
			sfputc(sh.stk,'$');
			sfputc(sh.stk,'\'');
			cp = bp;
			n = fold - 3;
			q = 1;
			while (c = mbchar(cp))
			{
				switch (c)
				{
		    		case '\033':		/* ASCII ESC */
					c = 'E';
					break;
		    		case '\n':
					q = 0;
					n = fold - 1;
					break;
		    		case '\r':
					c = 'r';
					break;
		    		case '\t':
					c = 't';
					break;
		    		case '\f':
					c = 'f';
					break;
		    		case '\b':
					c = 'b';
					break;
		    		case '\a':
					c = 'a';
					break;
		    		case '\\':
					if (*cp == 'n')
					{
						c = '\n';
						q = 0;
						n = fold - 1;
						break;
					}
				case '\'':
					break;
		    		default:
#if SHOPT_MULTIBYTE
					if(!iswprint(c))
#else
					if(!isprint(c))
#endif
					{
						if ((n -= 4) <= 0)
						{
							sfwrite(sh.stk,"'\\\n$'", 5);
							n = fold - 7;
						}
						sfprintf(sh.stk, "\\%03o", c);
						continue;
					}
					q = 0;
					break;
				}
				if ((n -= q + 1) <= 0)
				{
					if (!q)
					{
						sfputc(sh.stk,'\'');
						cp = bp;
						break;
					}
					sfwrite(sh.stk,"'\\\n$'", 5);
					n = fold - 5;
				}
				if (q)
					sfputc(sh.stk,'\\');
				else
					q = 1;
				sfputc(sh.stk,c);
				bp = cp;
			}
			if (!c)
				sfputc(sh.stk,'\'');
		}
		else if (q & 1)
		{
			sfputc(sh.stk,'\'');
			cp = bp;
			n = fold ? (fold - 2) : 0;
			while (c = mbchar(cp))
			{
				if (c == '\n')
					n = fold - 1;
				else if (n && --n <= 0)
				{
					n = fold - 2;
					sfwrite(sh.stk,bp, --cp - bp);
					bp = cp;
					sfwrite(sh.stk,"'\\\n'", 4);
				}
				else if (n == 1 && *cp == '\'')
				{
					n = fold - 5;
					sfwrite(sh.stk,bp, --cp - bp);
					bp = cp;
					sfwrite(sh.stk,"'\\\n\\''", 6);
				}
				else if (c == '\'')
				{
					sfwrite(sh.stk,bp, cp - bp - 1);
					bp = cp;
					if (n && (n -= 4) <= 0)
					{
						n = fold - 5;
						sfwrite(sh.stk,"'\\\n\\''", 6);
					}
					else
						sfwrite(sh.stk,"'\\''", 4);
				}
			}
			sfwrite(sh.stk,bp, cp - bp - 1);
			sfputc(sh.stk,'\'');
		}
		else if (n = fold)
		{
			cp = bp;
			while (c = mbchar(cp))
			{
				if (--n <= 0)
				{
					n = fold;
					sfwrite(sh.stk,bp, --cp - bp);
					bp = cp;
					sfwrite(sh.stk,"\\\n", 2);
				}
			}
			sfwrite(sh.stk,bp, cp - bp - 1);
		}
		else
			sfwrite(sh.stk,bp, cp - bp);
		if (c)
		{
			sfputc(sh.stk,'\\');
			sfputc(sh.stk,'\n');
		}
	} while (c);
	sfputc(sh.stk,0);
	return stkptr(sh.stk,offset);
}

/*
 * Find a multi-byte character in a string.
 * NOTE: Unlike strchr(3), the return value is an integer offset or -1 if not found.
 */
int sh_strchr(const char *string, const char *dp)
{
	const char *cp;
	if(mbwide())
	{
		wchar_t c, d;
		cp = string;
		d = mbchar(dp);
		while(c = mbchar(cp))
		{
			if(c==d)
				return cp-string;
		}
		if(d==0)
			return cp-string;
		return -1;
	}
	cp = strchr(string,*dp);
	return cp ? cp-string : -1;
}

/*
 * Return the number of terminal columns occupied by a string when printed.
 */
int sh_strwidth(const char *s)
{
	int w = 0, c, n;
	while (c = mbchar(s))
	{
		if (c < 0)				/* invalid multibyte character: count as 1 position */
			w++;
		else if ((n = mbwidth(c)) > 0)		/* don't count control characters (which yield -1) */
			w += n;
	}
	return w;
}

const char *_sh_translate(const char *message)
{
	return ERROR_translate(0,0,e_dict,message);
}

/*
 * change '['identifier']' to identifier
 * character before <str> must be a '['
 * returns pointer to last character
 */
char *sh_checkid(char *str, char *last)
{
	unsigned char *cp = (unsigned char*)str;
	unsigned char *v = cp;
	int c;
	if(c=mbchar(cp),isaletter(c))
		while(c=mbchar(cp),isaname(c));
	if(c==']' && (!last || ((char*)cp==last)))
	{
		/* eliminate [ and ] */
		while(v < cp)
		{
			v[-1] = *v;
			v++;
		}
		if(last)
			last -=2;
		else
		{
			while(*v)
			{
				v[-2] = *v;
				v++;
			}
			v[-2] = 0;
			last = (char*)v;
		}
	}
	return last;
}

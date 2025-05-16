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
*               K. Eugene Carlson <kvngncrlsn@gmail.com>               *
*                                                                      *
***********************************************************************/
/*
 *  completion.c - command and file completion for shell editors
 *
 */

#include	"shopt.h"
#include	"defs.h"

#if !SHOPT_SCRIPTONLY

#include	<ast_wchar.h>
#include	"lexstates.h"
#include	"path.h"
#include	"io.h"
#include	"edit.h"
#include	"history.h"

static char *fmtx(const char *string)
{
	const char	*cp = string;
	int	 	n = 0, c;
	int		pos = 0;
	unsigned char 	*state = (unsigned char*)sh_lexstates[ST_NORM];
	int		offset = stktell(sh.stk);
	char		hc[3];
#if SHOPT_HISTEXPAND
	const char	hexp = sh_isoption(SH_HISTEXPAND)!=0;
	if(hexp)
		hist_setchars(hc);
#else
	const char	hexp = 0;
#endif /* SHOPT_HISTEXPAND */
	if((!hexp || (*cp!=hc[0] && *cp!=hc[2])) && (*cp=='#' || *cp=='~'))
		sfputc(sh.stk,'\\');
	while((c=mbchar(cp)),((c>UCHAR_MAX)||(n=state[c])==0 || n==S_EPAT)
	&& (!hexp || ((c!=hc[0]) && (c!=hc[2] || string[0]!=hc[2]))) && c!='~')
		;
	if(n==S_EOF && *string!='#')
		return (char*)string;
	sfwrite(sh.stk,string,--cp-string);
	for(string=cp;c=mbchar(cp);string=cp)
	{
		if((n=cp-string)==1)
		{
			if(((n=state[c]) && n!=S_EPAT) || (hexp && ((c==hc[0]) || (c==hc[2] && !pos))))
				sfputc(sh.stk,'\\');
			sfputc(sh.stk,c);
		}
		else
			sfwrite(sh.stk,string,n);
		pos++;
	}
	sfputc(sh.stk,0);
	return stkptr(sh.stk,offset);
}

#if !SHOPT_GLOBCASEDET
#define charcmp(a,b,dummy) (a==b)
#else
static int charcmp(int a, int b, int nocase)
{
	if(nocase)
	{
#if _lib_towlower
		if(mbwide())
		{
			a = (int)towlower((wint_t)a);
			b = (int)towlower((wint_t)b);
		}
		else
#endif
		{
			a = tolower(a);
			b = tolower(b);
		}
	}
	return a==b;
}
#endif /* !SHOPT_GLOBCASEDET */

/*
 *  overwrites <str> to common prefix of <str> and <newstr>
 *  if <str> is equal to <newstr> returns  <str>+strlen(<str>)+1
 *  otherwise returns <str>+strlen(<str>)
 */
static char *overlaid(char *str,const char *newstr,int nocase)
{
	int c,d;
	char *strnext;
	while((strnext = str, c = mbchar(strnext)) && (d = mbchar(newstr), charcmp(c,d,nocase)))
		str = strnext;
	if(*str)
		*str = 0;
	else if(*newstr==0)
		str++;
	return str;
}


/*
 * returns pointer to beginning of expansion and sets type of expansion
 *
 * Detects variable expansions, command substitutions, and three quoting styles:
 * 1. '...'	inquote=='\'', dollarquote==0; no special characters
 * 2. $'...'	inquote=='\'', dollarquote==1; skips \.
 * 3. "..."	inquote=='"',  dollarquote==0; skips \., $..., ${...}, $(...), `...`
 */
static char *find_begin(char outbuff[], char *last, int endchar, int *type)
{
	char	*cp=outbuff, *bp, *xp;
	char	inquote = 0, dollarquote = 0, inassign = 0;
	int	mode=*type, c;
	bp = outbuff;
	*type = 0;
	while(cp < last)
	{
		xp = cp;
		switch(c= mbchar(cp))
		{
		    case '\'': case '"':
			if(!inquote)
			{
				inquote = c;
				bp = xp;
				break;
			}
			if(inquote==c)
				inquote = dollarquote = 0;
			break;
		    case '\\':
			if(inquote != '\'' || dollarquote)
				mbchar(cp);
			break;
		    case '$':
			if(inquote == '\'')
			{
				*type = '\'';
				bp = xp;
				break;
			}
			c = *(unsigned char*)cp;
			if(mode!='*' && (isaletter(c) || c=='{'))
			{
				int dot = '.';
				if(c=='{')
				{
					xp = cp;
					mbchar(cp);
					c = *(unsigned char*)cp;
					if(c!='.' && !isaletter(c))
						break;
				}
				else
					dot = 'a';
				while(cp < last)
				{
					if((c= mbchar(cp)) , c!=dot && !isaname(c))
						break;
				}
				if(cp>=last)
				{
					if(c==dot || isaname(c))
					{
						*type='$';
						return ++xp;
					}
					if(c!='}')
						bp = cp;
				}
			}
			else if(c=='(')
			{
				*type = mode;
				xp = find_begin(cp,last,')',type);
				if(*(cp=xp)!=')')
					bp = xp;
			}
			else if(c=='\'' && !inquote)
				dollarquote = 1;
			break;
		    case '`':
			if(inquote=='\'')
			{
				*type = '\'';
				bp = xp;
			}
			else
				bp = cp;
			break;
		    case '=':
			if(!inquote)
			{
				bp = cp;
				inassign = 1;
			}
			break;
		    case ':':
			if(!inquote && inassign)
				bp = cp;
			break;
		    case '~':
			if(*cp=='(')
				break;
			/* FALLTHROUGH */
		    default:
			if(c && c==endchar)
				return xp;
			if(!inquote && ismeta(c))
			{
				bp = cp;
				inassign = 0;
			}
			break;
		}
	}
	if(inquote && *bp==inquote)
	{
		/* set special type -1 for $'...' */
		*type = dollarquote ? -1 : inquote;
		bp++;
	}
	return bp;
}

/*
 * file name generation for edit modes
 * non-zero exit for error, <0 ring bell
 * don't search back past beginning of the buffer
 * mode is '*' for inline expansion,
 * mode is '\' for filename completion
 * mode is '=' cause files to be listed in select format
 */

int ed_expand(Edit_t *ep, char outbuff[],int *cur,int *eol,int mode, int count)
{
	struct comnod	*comptr;
	struct argnod	*ap;
	char		*out;
	char 		*av[2], *begin , *dir=0;
	int		addstar=0, rval=0, var=0, strip=1;
	int 		nomarkdirs = !sh_isoption(SH_MARKDIRS);
	sh_onstate(SH_FCOMPLETE);
	if(ep->e_nlist)
	{
		if(mode=='=' && count>0)
		{
			if(count> ep->e_nlist)
				return -1;
			mode = '?';
			av[0] = ep->e_clist[count-1];
			av[1] = 0;
		}
		else
		{
			stkset(sh.stk,ep->e_stkptr,ep->e_stkoff);
			ep->e_nlist = 0;
		}
	}
	comptr = stkalloc(sh.stk,sizeof(struct comnod));
	ap = stkseek(sh.stk,ARGVAL);
#if SHOPT_MULTIBYTE
	{
		int c = *cur;
		genchar *cp;
		/* adjust cur */
		cp = (genchar *)outbuff + *cur;
		c = *cp;
		*cp = 0;
		*cur = ed_external((genchar*)outbuff,(char*)stkptr(sh.stk,0));
		*cp = c;
		*eol = ed_external((genchar*)outbuff,outbuff);
	}
#endif /* SHOPT_MULTIBYTE */
#if SHOPT_VSH
	out = outbuff + *cur + (sh_isoption(SH_VI)!=0);
#if SHOPT_MULTIBYTE
	if(sh_isoption(SH_VI) && ep->e_savedwidth > 0)
		out += (ep->e_savedwidth - 1);
#endif /* SHOPT_MULTIBYTE */
#else
	out = outbuff + *cur;
#endif /* SHOPT_VSH */
	if(out[-1]=='"' || out[-1]=='\'')
	{
#if SHOPT_VSH
		rval = -(sh_isoption(SH_VI)!=0);
#else
		rval = 0;
#endif
		goto done;
	}
	comptr->comtyp = COMSCAN;
	comptr->comarg.ap = ap;
	ap->argflag = (ARG_MAC|ARG_EXP);
	ap->argnxt.ap = 0;
	ap->argchn.cp = 0;
	{
		int c;
		char *last = out;
		c =  *(unsigned char*)out;
		var = mode;
		begin = out = find_begin(outbuff,last,0,&var);
		if(var=='\'' && (*begin=='$' || *begin=='`'))
		{
			/* avoid spurious expansion or comsub execution within '...' */
			rval = -1;
			goto done;
		}
		else if(var=='$')
		{
			/* expand ${!varname@} to complete variable name(s) */
			sfputr(sh.stk,"${!",-1);
			sfwrite(sh.stk,out,last-out);
			sfputr(sh.stk,"@}",-1);
			out = last;
		}
		else
		{
			/* addstar set to zero if * should not be added */
			addstar = '*';
			while(out < last)
			{
				c = *(unsigned char*)out;
				if(isexp(c))
					addstar = 0;
				if (c == '/')
				{
					if(addstar == 0)
						strip = 0;
					dir = out+1;
				}
				sfputc(sh.stk,c);
				out++;
			}
		}
		if(mode=='?')
			mode = '*';
		if(var!='$' && mode=='\\' && out[-1]!='*')
			addstar = '*';
		if(*begin=='~')
		{
			/* do not perform tilde expansion while completing a quoted string */
			if(begin>outbuff && (begin[-1]=='"' || begin[-1]=='\''))
				sh_onstate(SH_NOTILDEXP);
			else if(!strchr(begin,'/'))
				addstar = 0;
		}
		if(addstar)
			sfputc(sh.stk,addstar);
		ap = stkfreeze(sh.stk,1);
	}
	if(mode!='*')
		sh_onoption(SH_MARKDIRS);
	{
		char	**com;
		char	*cp=begin, *left=0, *saveout=(char*)e_dot;
		int	nocase=0, narg, cmd_completion=0;
		int	size='x';
		while(cp>outbuff && ((size=cp[-1])==' ' || size=='\t'))
			cp--;
		if(!var && !strchr(ap->argval,'/') && (((cp==outbuff&&sh.nextprompt==1) || (strchr(";&|(",size)) && (cp==outbuff+1||size=='('||cp[-2]!='>') && *begin!='~' )))
		{
			cmd_completion=1;
			sh_onstate(SH_COMPLETE);
			sh_offstate(SH_DEFPATH);
		}
		if(ep->e_nlist)
		{
			narg = 1;
			com = av;
			if(dir)
				begin += (dir-begin);
		}
		else
		{
			com = sh_argbuild(&narg,comptr,0);
			/* special handling for leading quotes */
			if(begin>outbuff && (begin[-1]=='"' || begin[-1]=='\''))
			{
				begin--;
				if(var == -1)		/* $'...' */
					begin--;	/* also remove initial dollar */
			}
		}
		sh_offstate(SH_COMPLETE);
		/* allow a search to be aborted */
		if(sh.trapnote&SH_SIGSET)
		{
			rval = -1;
			goto done;
		}
		/* match? */
		if (*com==0 || (narg <= 1 && (strcmp(ap->argval,*com)==0) || (addstar && com[0][strlen(*com)-1]=='*')))
		{
			rval = -1;
			goto done;
		}
		if(mode=='\\' && out[-1]=='/'  && narg>1)
			mode = '=';
		else if(mode=='=' && narg<2)
			mode = '\\';  /* no filename menu if there is only one choice */
		if(mode=='=')
		{
			if (strip && !cmd_completion)
			{
				char **ptrcom;
				for(ptrcom=com;*ptrcom;ptrcom++)
					/* trim directory prefix */
					*ptrcom = path_basename(*ptrcom);
			}
			sfputc(sfstderr,'\n');
			sh_menu(sfstderr,narg,com);
			sfsync(sfstderr);
			ep->e_nlist = narg;
			ep->e_clist = com;
			goto done;
		}
		/* see if there is enough room */
		size = *eol - (out-begin);
		if(mode=='\\')
		{
			int c;
			if(dir)
			{
				c = *dir;
				*dir = 0;
				saveout = begin;
			}
#if SHOPT_GLOBCASEDET
			if(sh_isoption(SH_GLOBCASEDET))
				nocase = (pathicase(saveout) > 0);
#endif
			if(dir)
				*dir = c;
			/* just expand until name is unique */
			size += strlen(*com);
		}
		else
		{
			size += narg;
			{
				char **savcom = com;
				while (*com)
					size += strlen(cp=fmtx(*com++));
				com = savcom;
			}
		}
		/* see if room for expansion */
		if(outbuff+size >= &outbuff[MAXLINE])
		{
			com[0] = ap->argval;
			com[1] = 0;
		}
		/* save remainder of the buffer */
		if(*out)
			left=stkcopy(sh.stk,out);
		if(cmd_completion && mode=='\\')
			out = strcopy(begin,path_basename(cp= *com++));
		else if(mode=='*')
		{
			if(ep->e_nlist && dir && var)
			{
				if(*cp==var)
					cp++;
				else
					*begin++ = var;
				out = strcopy(begin,cp);
				var = 0;
			}
			else
				out = strcopy(begin,fmtx(*com));
			com++;
		}
		else
			out = strcopy(begin,*com++);
		if(mode=='\\')
		{
			saveout= ++out;
			while (*com && *begin)
			{
				if(cmd_completion)
					out = overlaid(begin,path_basename(*com++),nocase);
				else
					out = overlaid(begin,*com++,nocase);
			}
			mode = (out==saveout);
			if(out>outbuff && out[-1]==0)
				out--;
			if(mode && (out==outbuff || out>outbuff && out[-1]!='/'))
			{
				if(cmd_completion)
				{
					/* add as tracked alias */
					Pathcomp_t *pp;
					if(*cp=='/' && (pp=path_dirfind(sh.pathlist,cp,'/')))
						path_settrackedalias(begin,pp);
					out = strcopy(begin,cp);
				}
				/* add quotes if necessary */
				if((cp=fmtx(begin))!=begin)
					out = strcopy(begin,cp);
				if(var=='$' && begin[-1]=='{')
					*out = '}';
				else
					*out = ' ';
				*++out = 0;
			}
			else if((cp=fmtx(begin))!=begin)
			{
				out = strcopy(begin,cp);
				if(out[-1] =='"' || out[-1]=='\'')
					  *--out = 0;
			}
			if(*begin==0)
				ed_ringbell();
		}
		else
		{
			while (*com)
			{
				*out++  = ' ';
				out = strcopy(out,fmtx(*com++));
			}
		}
		if(ep->e_nlist)
		{
			cp = com[-1];
			if(cp[strlen(cp)-1]!='/')
			{
				if(var=='$' && begin[-1]=='{')
					*out = '}';
				else
					*out = ' ';
				out++;
			}
			else if(out[-1] =='"' || out[-1]=='\'')
				out--;
			*out = 0;
		}
		*cur = (out-outbuff);
		/* restore rest of buffer */
		if(left)
			out = strcopy(out,left);
		*eol = (out-outbuff);
	}
 done:
	sh_offstate(SH_FCOMPLETE);
	sh_offstate(SH_NOTILDEXP);
	if(!ep->e_nlist)
		stkset(sh.stk,ep->e_stkptr,ep->e_stkoff);
	if(nomarkdirs)
		sh_offoption(SH_MARKDIRS);
#if SHOPT_MULTIBYTE
	{
		int c,n=0;
		/* first re-adjust cur */
		c = outbuff[*cur];
		outbuff[*cur] = 0;
		for(out=outbuff; *out;n++)
			mbchar(out);
		outbuff[*cur] = c;
		*cur = n;
		outbuff[*eol+1] = 0;
		*eol = ed_internal(outbuff,(genchar*)outbuff);
	}
#endif /* SHOPT_MULTIBYTE */
	return rval;
}

#if SHOPT_ESH || SHOPT_VSH
/*
 * look for edit macro named _i
 * if found, puts the macro definition into lookahead buffer and returns 1
 */
int ed_macro(Edit_t *ep, int i)
{
	char *out;
	Namval_t *np;
	genchar buff[LOOKAHEAD+1];
	if(i != '@')
		ep->e_macro[1] = i;
	/* macros of the form <ESC>[c evoke alias __c */
	if(i=='_')
		ep->e_macro[2] = i = ed_getchar(ep,1);
	else
		ep->e_macro[2] = 0;
	if (isalnum(i)&&(np=nv_search(ep->e_macro,sh.alias_tree,0))&&(out=nv_getval(np)))
	{
#if SHOPT_MULTIBYTE
		/* copy to buff in internal representation */
		int c = 0;
		if( strlen(out) > LOOKAHEAD )
		{
			c = out[LOOKAHEAD];
			out[LOOKAHEAD] = 0;
		}
		i = ed_internal(out,buff);
		if(c)
			out[LOOKAHEAD] = c;
#else
		strncpy((char*)buff,out,LOOKAHEAD);
		buff[LOOKAHEAD] = 0;
		i = strlen((char*)buff);
#endif /* SHOPT_MULTIBYTE */
		while(i-- > 0)
			ed_ungetchar(ep,buff[i]);
		return 1;
	}
	return 0;
}
#endif /* SHOPT_ESH || SHOPT_VSH */

/*
 * Enter the fc command on the current history line
 */
int ed_fulledit(Edit_t *ep)
{
	char *cp;
	if(!sh.hist_ptr)
		return -1;
	/* use EDITOR on current command */
	if(ep->e_hline == ep->e_hismax)
	{
		if(ep->e_eol<0)
			return -1;
#if SHOPT_MULTIBYTE
		ep->e_inbuf[ep->e_eol+1] = 0;
		ed_external(ep->e_inbuf, (char *)ep->e_inbuf);
#endif /* SHOPT_MULTIBYTE */
		sfwrite(sh.hist_ptr->histfp,(char*)ep->e_inbuf,ep->e_eol+1);
		sh_onstate(SH_HISTORY);
		hist_flush(sh.hist_ptr);
	}
	cp = strcopy((char*)ep->e_inbuf,e_runvi);
	cp = strcopy(cp, fmtint(ep->e_hline,1));
#if SHOPT_VSH
	ep->e_eol = ((unsigned char*)cp - (unsigned char*)ep->e_inbuf)-(sh_isoption(SH_VI)!=0);
#else
	ep->e_eol = ((unsigned char*)cp - (unsigned char*)ep->e_inbuf);
#endif
	return 0;
}

#else
NoN(completion)
#endif /* !SHOPT_SCRIPTONLY */

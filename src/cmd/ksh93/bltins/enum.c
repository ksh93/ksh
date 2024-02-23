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
#include	"shopt.h"
#include	"defs.h"

#define ENUM_ID "enum (ksh 93u+m) 2022-03-05"

const char sh_optenum[] =
"[-?\n@(#)$Id: " ENUM_ID " $\n]"
"[--catalog?" SH_DICT "]"
"[+NAME?enum - create an enumeration type]"
"[+DESCRIPTION?\benum\b is a declaration command that creates one or more "
	"enumeration type declaration commands named \atypename\a. Variables "
	"of the created type can only store any one of the \avalue\as given. "
	"For example, \benum bool=(false true)\b creates a Boolean variable "
	"type of which variables may be declared like \bbool x=true y=false\b.]"
"[+?If the list of \avalue\as is omitted, then \atypename\a must name an "
    "indexed array variable with at least two elements.]" 
"[+?For more information, create a type, then use \atypename\a \b--man\b.]"
"[+USE IN ARITHMETIC EXPRESSIONS?When an enumeration variable is used in an "
	"arithmetic expression, its value is the index into the array that "
	"defined it, starting from 0. Taking the \bbool\b type from the "
	"example above, if a variable of this type is used in an arithmetic "
	"expression, \bfalse\b translates to 0 and \btrue\b to 1.]"
"[+?Enumeration values may also be used directly in an arithmetic expression "
	"that refers to a variable of an enumeration type. "
	"To continue our example, for a \bbool\b variable \bv\b, "
	"\b((v==true))\b is the same as \b((v==1))\b and "
	"if a variable named \btrue\b exists, it is ignored.]"
"[i:ignorecase?The values are case insensitive.]"
"\n"
"\n\atypename\a[\b=(\b \avalue\a ... \b)\b] ...\n"
"\n"
"[+EXIT STATUS]"
    "{"
        "[+0?Successful completion.]"
        "[+>0?An error occurred.]"
    "}"
"[+SEE ALSO?\bksh\b(1), \btypeset\b(1).]"
;

static const char enum_type[] =
"[-?\n@(#)$Id: " ENUM_ID " $\n]"
"[--catalog?" SH_DICT "]"
"[+NAME?\f?\f - create an instance of type \b\f?\f\b]"
"[+DESCRIPTION?The \b\f?\f\b declaration command creates a variable for "
    "each \aname\a with enumeration type \b\f?\f\b, a type that has been "
    "created with the \benum\b(1) command.]"
"[+?The variable can have one of the following values: \fvalues\f. "
	"The values are \fcase\fcase sensitive. "
	"If \b=\b\avalue\a is omitted, the default is \fdefault\f.]"
"[+?Within arithmetic expressions, these values translate to index numbers "
	"from \b0\b (for \fdefault\f) to \flastn\f (for \flastv\f). "
	"It is an error for an arithmetic expression to assign a value "
	"outside of that range. Decimal fractions are ignored.]"
"[+?If no \aname\as are specified then the names and values of all "
        "variables of this type are written to standard output.]"
"[+?\b\f?\f\b is built in to the shell as a declaration command so that "
        "field splitting and pathname expansion are not performed on "
        "the arguments. Tilde expansion occurs on \avalue\a.]"
"[r?Enables readonly. Once enabled, the value cannot be changed or unset.]"
"[a?Indexed array. Each \aname\a is converted to an indexed "
        "array of type \b\f?\f\b. If a variable already exists, the current "
        "value will become index \b0\b.]"
"[A?Associative array. Each \aname\a is converted to an associative "
        "array of type \b\f?\f\b. If a variable already exists, the current "
        "value will become subscript \b0\b.]"
"[h]:[string?Used within a type definition to provide a help string  "
        "for variable \aname\a. Otherwise, it is ignored.]"
"[S?Used with a type definition to indicate that the variable is shared by "
        "each instance of the type. When used inside a function defined "
        "with the \bfunction\b reserved word, the specified variables "
        "will have function static scope. Otherwise, the variable is "
        "unset prior to processing the assignment list.]"
"\n"
"\n[name[=value]...]\n"
"\n"
"[+EXIT STATUS?]{"
        "[+0?Successful completion.]"
        "[+>0?An error occurred.]"
"}"

"[+SEE ALSO?\benum\b(1), \btypeset\b(1)]"
;

extern const char is_spcbuiltin[];

struct Enum
{
	Namfun_t	hdr;
	short		nelem;
	short		iflag;
	const char	*values[1];
};

/*
 * For range checking in arith.c
 */
short b_enum_nelem(Namfun_t *fp)
{
	return ((struct Enum *)fp)->nelem;
}

static int enuminfo(Opt_t* op, Sfio_t *out, const char *str, Optdisc_t *fp)
{
	Namval_t	*np;
	struct Enum	*ep;
	int		n=0;
	const char	*v;
	np = *(Namval_t**)(fp+1);
	ep = (struct Enum*)np->nvfun;
	if(!ep)
		return 0;
	if(strcmp(str,"default")==0)
		sfprintf(out,"\b%s\b",ep->values[0]);
	else if(strncmp(str,"last",4)==0)
	{
		while(ep->values[++n])
			;
		n--;
		if(str[4]=='v')
			sfprintf(out,"\b%s\b",ep->values[n]);
		else
			sfprintf(out,"\b%d\b",n);
	}
	else if(strcmp(str,"case")==0)
	{
		if(ep->iflag)
			sfprintf(out,"not ");
	}
	else while(v=ep->values[n])
		sfprintf(out, n++ ? ", \b%s\b" : "\b%s\b", v);
	return 0;
}

static Namfun_t *clone_enum(Namval_t* np, Namval_t *mp, int flags, Namfun_t *fp)
{
	struct Enum	*ep, *pp=(struct Enum*)fp;
	ep = sh_newof(0,struct Enum,1,pp->nelem*sizeof(char*));
	memcpy(ep,pp,sizeof(struct Enum)+pp->nelem*sizeof(char*));
	return &ep->hdr;
}

static void put_enum(Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	struct Enum 		*ep = (struct Enum*)fp;
	const char		*v;
	unsigned short		i=0;
	int			n;
	if(!val)
	{
		nv_putv(np, val, flags,fp);
		nv_disc(np,&ep->hdr,NV_POP);
		if(!ep->hdr.nofree)
			free(ep);
		return;
	}
	if(flags&NV_INTEGER)
	{
		nv_putv(np,val,flags,fp);
		return;
	}
	while(v=ep->values[i])
	{
		if(ep->iflag)
			n = strcasecmp(v,val);
		else
			n = strcmp(v,val);
		if(n==0)
		{
			nv_putv(np, (char*)&i, NV_UINT16, fp);
			return;
		}
		i++;
	}
	error(ERROR_exit(1), "%s: invalid value %s",nv_name(np),val);
	UNREACHABLE();
}

static char* get_enum(Namval_t* np, Namfun_t *fp)
{
	static char buff[6];
	struct Enum *ep = (struct Enum*)fp;
	long n = nv_getn(np,fp);
	if(n < ep->nelem)
		return (char*)ep->values[n];
	sfsprintf(buff,sizeof(buff),"%u%c",n,0);
	return buff;
}

const Namdisc_t ENUM_disc = { 0, put_enum, get_enum, nv_getn, 0, 0, clone_enum };

int b_enum(int argc, char** argv, Shbltin_t *context)
{
	int			sz,i,n,iflag = 0;
	Namval_t		*np, *tp;
	Namarr_t		*ap;
	char			*cp,*sp;
	struct Enum		*ep;
	struct {
	    Optdisc_t	opt;
	    Namval_t	*np;
	}			optdisc;

	cmdinit(argc, argv, context, SH_DICT, ERROR_NOTIFY);
	for (;;)
	{
		switch (optget(argv, sh_optenum))
		{
		case 'i':
			iflag = 'i';
			continue;
		case '?':
			error(ERROR_usage(2), "%s", opt_info.arg);
			UNREACHABLE();
		case ':':
			error(2, "%s", opt_info.arg);
			break;
		}
		break;
	}
	argv += opt_info.index;
	if (error_info.errors || !*argv)
	{
		error(ERROR_USAGE|2, "%s", optusage(NULL));
		return 1;
	}
	if(sh.subshell && !sh.subshare)
		sh_subfork();
	while(cp = *argv++)
	{
		/* Do not allow 'enum' to override special built-ins -- however, exclude
		 * previously created type commands from this search as that is handled elsewhere. */
		if((tp=nv_search(cp,sh.bltin_tree,0)) && nv_isattr(tp,BLT_SPC) && !nv_search(cp,sh.typedict,0))
		{
			errormsg(SH_DICT,ERROR_exit(1),"%s:%s",cp,is_spcbuiltin);
			UNREACHABLE();
		}
		if(!(np = nv_open(cp, NULL, NV_VARNAME|NV_NOADD))  || !(ap=nv_arrayptr(np)) || ap->fun || (sz=ap->nelem&(((1L<<ARRAY_BITS)-1))) < 2)
		{
			error(ERROR_exit(1), "%s must name an array containing at least two elements",cp);
			UNREACHABLE();
		}
		sfprintf(sh.strbuf,"%s.%s",NV_CLASS,np->nvname);
		tp = nv_open(sfstruse(sh.strbuf), sh.var_tree, NV_VARNAME);
		n = sz;
		i = 0;
		nv_onattr(tp, NV_UINT16);
		nv_putval(tp, (char*)&i, NV_INTEGER);
		nv_putsub(np, NULL, ARRAY_SCAN);
		do
		{
			sz += strlen(nv_getval(np));
		}
		while(nv_nextsub(np));
		sz += n*sizeof(char*);
		ep = sh_newof(0,struct Enum,1,sz);
		ep->iflag = iflag;
		ep->nelem = n;
		cp = (char*)&ep->values[n+1];
		nv_putsub(np, NULL, ARRAY_SCAN);
		ep->values[n] = 0;
		i = 0;
		do
		{
			ep->values[i++] = cp;
			sp =  nv_getval(np);
			n = strlen(sp);
			memcpy(cp,sp,n+1);
			cp += n+1;
		}
		while(nv_nextsub(np));
		ep->hdr.dsize = sizeof(struct Enum)+sz;
		ep->hdr.disc = &ENUM_disc;
		ep->hdr.type = tp;
		nv_onattr(tp, NV_RDONLY);
		nv_disc(tp, &ep->hdr,NV_FIRST);
		memset(&optdisc,0,sizeof(optdisc));
		optdisc.opt.infof = enuminfo;
		optdisc.np = tp;
		nv_addtype(tp, enum_type, &optdisc.opt, sizeof(optdisc)); 
	}
	return error_info.errors != 0;
}

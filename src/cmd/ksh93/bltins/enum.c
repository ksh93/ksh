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
#include	"defs.h"

static const char enum_usage[] =
"[-?@(#)$Id: enum (AT&T Research) 2008-01-08 $\n]"
"[--catalog?" ERROR_CATALOG "]"
"[+NAME?enum - create an enumeration type]"
"[+DESCRIPTION?\benum\b is a declaration command that creates an enumeration "
    "type \atypename\a that can only store any one of the values in the indexed "
    "array variable \atypename\a.]"
"[+?If the list of \avalue\as is omitted, then \atypename\a must name an "
    "indexed array variable with at least two elements.]" 
"[i:ignorecase?The values are case insensitive.]"
"\n"
"\n\atypename\a[\b=(\b \avalue\a ... \b)\b]\n"
"\n"
"[+EXIT STATUS]"
    "{"
        "[+0?Successful completion.]"
        "[+>0?An error occurred.]"
    "}"
"[+SEE ALSO?\bksh\b(1), \btypeset\b(1).]"
;

static const char enum_type[] =
"[-1c?\n@(#)$Id: type (AT&T Labs Research) 2008-01-08 $\n]"
"[--catalog?" ERROR_CATALOG "]"
"[+NAME?\f?\f - create an instance of type \b\f?\f\b]"
"[+DESCRIPTION?The \b\f?\f\b declaration command creates a variable for "
    "each \aname\a with enumeration type \b\f?\f\b, a type that has been "
    "created with the \benum\b(1) command.]"
"[+?The variable can have one of the following values: \fvalues\f. "
    "The values are \fcase\fcase sensitive.]"
"[+?If \b=\b\avalue\a is omitted, the default is \fdefault\f.]"
"[+?If no \aname\as are specified then the names and values of all "
        "variables of this type are written to standard output.]"
"[+?\b\f?\f\b is built in to the shell as a declaration command so that "
        "field splitting and pathname expansion are not performed on "
        "the arguments.  Tilde expansion occurs on \avalue\a.]"
"[r?Enables readonly.  Once enabled, the value cannot be changed or unset.]"
"[a?Indexed array. Each \aname\a is converted to an indexed "
        "array of type \b\f?\f\b.  If a variable already exists, the current "
        "value will become index \b0\b.]"
"[A?Associative array. Each \aname\a is converted to an associative "
        "array of type \b\f?\f\b.  If a variable already exists, the current "
        "value will become subscript \b0\b.]"
"[h]:[string?Used within a type definition to provide a help string  "
        "for variable \aname\a.  Otherwise, it is ignored.]"
"[S?Used with a type definition to indicate that the variable is shared by "
        "each instance of the type.  When used inside a function defined "
        "with the \bfunction\b reserved word, the specified variables "
        "will have function static scope.  Otherwise, the variable is "
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

struct Enum
{
	Namfun_t	hdr;
	short		nelem;
	short		iflag;
	const char	*values[1];
};

static int enuminfo(Opt_t* op, Sfio_t *out, const char *str, Optdisc_t *fp)
{
	Namval_t	*np;
	struct Enum	*ep;
	int		n=0;
	const char	*v;
	np = *(Namval_t**)(fp+1);
	ep = (struct Enum*)np->nvfun;
	if(strcmp(str,"default")==0)
		sfprintf(out,"\b%s\b",ep->values[0]);
	else if(strcmp(str,"case")==0)
	{
		if(ep->iflag)
			sfprintf(out,"not ");
	}
	else while(v=ep->values[n])
		sfprintf(out, n++ ? ", \b%s\b" : "\b%s\b", v);
	return(0);
}

static Namfun_t *clone_enum(Namval_t* np, Namval_t *mp, int flags, Namfun_t *fp)
{
	struct Enum	*ep, *pp=(struct Enum*)fp;
	ep = sh_newof(0,struct Enum,1,pp->nelem*sizeof(char*));
	memcpy((void*)ep,(void*)pp,sizeof(struct Enum)+pp->nelem*sizeof(char*));
	return(&ep->hdr);
}

static void put_enum(Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	struct Enum 		*ep = (struct Enum*)fp;
	register const char	*v;
	unsigned short		i=0;
	int			n;
	if(!val)
	{
		nv_putv(np, val, flags,fp);
		nv_disc(np,&ep->hdr,NV_POP);
		if(!ep->hdr.nofree)
			free((void*)ep);
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

static char* get_enum(register Namval_t* np, Namfun_t *fp)
{
	static char buff[6];
	struct Enum *ep = (struct Enum*)fp;
	long n = nv_getn(np,fp);
	if(n < ep->nelem)
		return((char*)ep->values[n]);
	sfsprintf(buff,sizeof(buff),"%u%c",n,0);
	return(buff);
}

static Sfdouble_t get_nenum(register Namval_t* np, Namfun_t *fp)
{
	return(nv_getn(np,fp));
}

const Namdisc_t ENUM_disc        = {  0, put_enum, get_enum, get_nenum, 0,0,clone_enum };

#ifdef STANDALONE
static int enum_create(int argc, char** argv, Shbltin_t *context)
#else
int b_enum(int argc, char** argv, Shbltin_t *context)
#endif
{
	int			sz,i,n,iflag = 0;
	Namval_t		*np, *tp;
	Namarr_t		*ap;
	char			*cp,*sp;
	struct Enum		*ep;
	Shell_t			*shp = context->shp;
	struct {
	    Optdisc_t	opt;
	    Namval_t	*np;
	}			optdisc;

	cmdinit(argc, argv, context, ERROR_CATALOG, ERROR_NOTIFY);
	for (;;)
	{
		switch (optget(argv, enum_usage))
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
	if (error_info.errors || !*argv || *(argv + 1))
	{
		error(ERROR_USAGE|2, "%s", optusage(NiL));
		return 1;
	}
	while(cp = *argv++)
	{
		if(!(np = nv_open(cp, (void*)0, NV_VARNAME|NV_NOADD))  || !(ap=nv_arrayptr(np)) || ap->fun || (sz=ap->nelem&(((1L<<ARRAY_BITS)-1))) < 2)
		{
			error(ERROR_exit(1), "%s must name an array containing at least two elements",cp);
			UNREACHABLE();
		}
		n = staktell();
		sfprintf(stkstd,"%s.%s%c",NV_CLASS,np->nvname,0);
		tp = nv_open(stakptr(n), shp->var_tree, NV_VARNAME);
		stakseek(n);
		n = sz;
		i = 0;
		nv_onattr(tp, NV_UINT16);
		nv_putval(tp, (char*)&i, NV_INTEGER);
		nv_putsub(np, (char*)0, ARRAY_SCAN);
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
		nv_putsub(np, (char*)0, ARRAY_SCAN);
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

#ifdef STANDALONE
void lib_init(int flag, void* context)
{
	Shell_t		*shp = ((Shbltin_t*)context)->shp;
	Namval_t	*mp,*bp;
	if(flag)
		return;
	bp = sh_addbuiltin("Enum", enum_create, (void*)0); 
	mp = nv_search("typeset",shp->bltin_tree,0);
	nv_onattr(bp,nv_isattr(mp,NV_PUBLIC));
}
#endif

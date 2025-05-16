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
*                                                                      *
***********************************************************************/
/*
 * UNIX shell parse tree executer
 *
 *   David Korn
 *   AT&T Labs
 *
 */

#include	"shopt.h"
#include	"defs.h"
#include	<fcin.h>
#include	"variables.h"
#include	"path.h"
#include	"name.h"
#include	"io.h"
#include	"shnodes.h"
#include	"jobs.h"
#include	"test.h"
#include	"builtins.h"
#include	"FEATURE/time"
#include	"FEATURE/externs"
#include	"FEATURE/locale"
#include	"streval.h"

#if _lib_getrusage && !defined(RUSAGE_SELF)
#   include <sys/resource.h>
#endif

#undef _use_ntfork_tcpgrp
#if SHOPT_SPAWN && _lib_posix_spawn > 1 && _lib_posix_spawn_file_actions_addtcsetpgrp_np
#define _use_ntfork_tcpgrp 1
#endif

#if SHOPT_SPAWN
    static pid_t sh_ntfork(const Shnode_t*,char*[],int*,int);
#endif /* SHOPT_SPAWN */

static void	sh_funct(Namval_t*, int, char*[], struct argnod*,int);
static void	coproc_init(int pipes[]);

static void	*timeout;
static char	nlock;
static char	pipejob;

struct funenv
{
	Namval_t	*node;
	struct argnod	*env;
	Namval_t	**nref;
};

/* ========	command execution	======== */

#if !SHOPT_DEVFD
    static pid_t fifo_save_ppid;

    static void fifo_check(void *handle)
    {
	NOT_USED(handle);
	if(getppid() != fifo_save_ppid)
	{
		unlink(sh.fifo);
		sh_done(0);
	}
    }

    /* Remove any remaining FIFOs to stop unused process substitutions blocking on trying to open the FIFO */
    static void fifo_cleanup(void)
    {
	if(sh.fifo_tree)
	{
		Namval_t	*fifo = dtfirst(sh.fifo_tree);
		if(fifo)
		{
			do
				unlink(fifo->nvname);
			while(fifo = dtnext(sh.fifo_tree,fifo));
			dtclose(sh.fifo_tree);
			sh.fifo_tree = NULL;
		}
	}
    }
#endif /* !SHOPT_DEVFD */

#if _lib_getrusage
/* getrusage tends to have higher precision */
static void get_cpu_times(struct timeval *tv_usr, struct timeval *tv_sys)
{
	struct rusage usage_self, usage_child;

	getrusage(RUSAGE_SELF, &usage_self);
	getrusage(RUSAGE_CHILDREN, &usage_child);
	timeradd(&usage_self.ru_utime, &usage_child.ru_utime, tv_usr);
	timeradd(&usage_self.ru_stime, &usage_child.ru_stime, tv_sys);
}
#else
static void get_cpu_times(struct timeval *tv_usr, struct timeval *tv_sys)
{
	struct tms cpu_times;
	struct timeval tv1, tv2;
	Sfdouble_t dtime;

	if(times(&cpu_times) == (clock_t)-1)
	{
		errormsg(SH_DICT, ERROR_exit(1), "times(3) failed: %s", strerror(errno));
		UNREACHABLE();
	}

	dtime = (Sfdouble_t)cpu_times.tms_utime / sh.lim.clk_tck;
	tv1.tv_sec = dtime / 60;
	tv1.tv_usec = 1000000 * (dtime - tv1.tv_sec);
	dtime = (Sfdouble_t)cpu_times.tms_cutime / sh.lim.clk_tck;
	tv2.tv_sec = dtime / 60;
	tv2.tv_usec = 1000000 * (dtime - tv2.tv_sec);
	timeradd(&tv1, &tv2, tv_usr);

	dtime = (Sfdouble_t)cpu_times.tms_stime / sh.lim.clk_tck;
	tv1.tv_sec = dtime / 60;
	tv1.tv_usec = 1000000 * (dtime - tv1.tv_sec);
	dtime = (Sfdouble_t)cpu_times.tms_cstime / sh.lim.clk_tck;
	tv2.tv_sec = dtime / 60;
	tv2.tv_usec = 1000000 * (dtime - tv2.tv_sec);
	timeradd(&tv1, &tv2, tv_sys);
}
#endif /* _lib_getrusage */

static inline Sfdouble_t timeval_to_double(struct timeval tv)
{
	return (Sfdouble_t)tv.tv_sec + ((Sfdouble_t)tv.tv_usec / 1000000.0);
}

/*
 * print time <t> in h:m:s format with precision <p>
 */
static void l_time(Sfio_t *outfile, struct timeval *tv, int precision)
{
	Sfulong_t hr = tv->tv_sec / (60 * 60);
	Sfulong_t min = (tv->tv_sec / 60) % 60;
	Sfulong_t sec = tv->tv_sec % 60;
	Sfulong_t frac = tv->tv_usec;

	/* scale fraction from micro to milli, centi, or deci second according to precision */
	int n;
	for(n = 3 + (3 - precision); n > 0; --n)
		frac /= 10;
	if(hr)
		sfprintf(outfile,"%juh",hr);
	if(precision)
		sfprintf(outfile, sh_isoption(SH_POSIX) ? "%jum%ju%c%0*jus" : "%jum%02ju%c%0*jus", min, sec, sh.radixpoint, precision, frac);
	else
		sfprintf(outfile, sh_isoption(SH_POSIX) ? "%jum%jus" : "%jum%02jus", min, sec);
}

#define TM_REAL_IDX 0
#define TM_USR_IDX 1
#define TM_SYS_IDX 2

static void p_time(Sfio_t *out, const char *format, struct timeval tm[3])
{
	int		c,n,offset = stktell(sh.stk);
	const char	*first;
	struct timeval	tv_cpu_sum;
	struct timeval	*tvp;
	for(first=format; *format; format++)
	{
		unsigned char l_modifier = 0;
		int precision = 3;
		c = *format;
		if(c!='%')
			continue;
		sfwrite(sh.stk, first, format-first);
		c = *++format;
		if(c=='\0')
		{
			/* If a lone percent is the last character of the format pretend
			   the user had written '%%' for a literal percent */
			sfwrite(sh.stk, "%", 1);
			first = format + 1;
			break;
		}
		else if(c=='%')
		{
			first = format;
			continue;
		}
		if(c>='0' && c <='9')
		{
			precision = (c>'6')?6:(c-'0');
			c = *++format;
		}
		if(c=='P')
		{
			struct timeval tv_real = tm[TM_REAL_IDX];
			struct timeval tv_cpu;
			Sfdouble_t d;
			timeradd(&tm[TM_USR_IDX], &tm[TM_SYS_IDX], &tv_cpu);
			d = timeval_to_double(tv_real);
			if(d)
				d = 100.0 * timeval_to_double(tv_cpu) / d;
			sfprintf(sh.stk, "%.*f", precision, d);
			first = format + 1;
			continue;
		}
		if(c=='l')
		{
			l_modifier = 1;
			c = *++format;
		}
		if(c=='R')
			tvp = &tm[TM_REAL_IDX];
		else if(c=='U')
			tvp = &tm[TM_USR_IDX];
		else if(c=='S')
			tvp = &tm[TM_SYS_IDX];
		else if(c=='C')
		{
			/* sum of U + S */
			timeradd(&tm[TM_USR_IDX], &tm[TM_SYS_IDX], &tv_cpu_sum);
			tvp = &tv_cpu_sum;
		}
		else
		{
			stkseek(sh.stk,offset);
			errormsg(SH_DICT,ERROR_exit(0),e_badtformat,c);
			return;
		}
		if(l_modifier)
			l_time(sh.stk, tvp, precision);
		else
		{
			/* scale fraction from micro to milli, centi, or deci second according to precision */
			Sfulong_t sec = tvp->tv_sec;
			Sfulong_t n, frac = tvp->tv_usec;
			for(n = 3 + (3 - precision); n > 0; --n)
				frac /= 10;
			if(precision)
				sfprintf(sh.stk, "%ju%c%0*ju", sec, sh.radixpoint, precision, frac);
			else
				sfprintf(sh.stk, "%ju", sec);
		}
		first = format+1;
	}
	if(format>first)
		sfwrite(sh.stk,first, format-first);
	sfputc(sh.stk,'\n');
	n = stktell(sh.stk)-offset;
	sfwrite(out,stkptr(sh.stk,offset),n);
	stkseek(sh.stk,offset);
}

#if SHOPT_OPTIMIZE
/*
 * clear argument pointers that point into the stack
 */
static int p_arg(struct argnod*,int);
static int p_switch(struct regnod*);
static int p_comarg(struct comnod *com)
{
	Namval_t *np=com->comnamp;
	int n = p_arg(com->comset,ARG_ASSIGN);
	if((com->comtyp&COMSCAN) && com->comarg.ap)
		n += p_arg(com->comarg.ap,0);
	if(com->comstate && np)
	{
		/* call builtin to cleanup state */
		Shbltin_t *bp = &sh.bltindata;
		void  *save_ptr = bp->ptr;
		void  *save_data = bp->data;
		bp->bnode = np;
		bp->vnode = com->comnamq;
		bp->ptr = nv_context(np);
		bp->data = com->comstate;
		bp->flags = SH_END_OPTIM;
		(funptr(np))(0, NULL, bp);
		bp->ptr = save_ptr;
		bp->data = save_data;
	}
	com->comstate = 0;
	if(com->comarg.ap && !np)
		n++;
	return n;
}

extern void sh_optclear(void*);

static int sh_tclear(Shnode_t *t)
{
	int n=0;
	if(!t)
		return 0;
	switch(t->tre.tretyp&COMMSK)
	{
		case TTIME:
		case TPAR:
			return sh_tclear(t->par.partre);
		case TCOM:
			return p_comarg((struct comnod*)t);
		case TSETIO:
		case TFORK:
			return sh_tclear(t->fork.forktre);
		case TIF:
			n=sh_tclear(t->if_.iftre);
			n+=sh_tclear(t->if_.thtre);
			n+=sh_tclear(t->if_.eltre);
			return n;
		case TWH:
			if(t->wh.whinc)
				n=sh_tclear((Shnode_t*)(t->wh.whinc));
			n+=sh_tclear(t->wh.whtre);
			n+=sh_tclear(t->wh.dotre);
			return n;
		case TLST:
		case TAND:
		case TORF:
		case TFIL:
			n=sh_tclear(t->lst.lstlef);
			return n + sh_tclear(t->lst.lstrit);
		case TARITH:
			return p_arg(t->ar.arexpr,ARG_ARITH);
		case TFOR:
			n=sh_tclear(t->for_.fortre);
			return n + sh_tclear((Shnode_t*)t->for_.forlst);
		case TSW:
			n=p_arg(t->sw.swarg,0);
			return n + p_switch(t->sw.swlst);
		case TFUN:
			n=sh_tclear(t->funct.functtre);
			return n + sh_tclear((Shnode_t*)t->funct.functargs);
		case TTST:
			if((t->tre.tretyp&TPAREN)==TPAREN)
				return sh_tclear(t->lst.lstlef);
			else
			{
				n=p_arg(&(t->lst.lstlef->arg),0);
				if(t->tre.tretyp&TBINARY)
					n+=p_arg(&(t->lst.lstrit->arg),0);
			}
	}
	return n;
}

static int p_arg(struct argnod *arg,int flag)
{
	while(arg)
	{
		if(strlen(arg->argval) || (arg->argflag==ARG_RAW))
			arg->argchn.ap = 0;
		else if(flag==0)
			sh_tclear((Shnode_t*)arg->argchn.ap);
		else
			sh_tclear(((struct fornod*)arg->argchn.ap)->fortre);
		arg = arg->argnxt.ap;
	}
	return 0;
}

static int p_switch(struct regnod *reg)
{
	int n=0;
	while(reg)
	{
		n+=p_arg(reg->regptr,0);
		n+=sh_tclear(reg->regcom);
		reg = reg->regnxt;
	}
	return n;
}
#endif /* SHOPT_OPTIMIZE */

static void out_pattern(Sfio_t *iop, const char *cp, int n)
{
	int c;
	do
	{
		switch(c= *cp)
		{
		    case 0:
			if(n<0)
				return;
			c = n;
			break;
		    case '\n':
			sfputr(iop,"$'\\n",'\'');
			continue;
		    case '\\':
			if (!(c = *++cp))
				c = '\\';
			/* FALLTHROUGH */
		    case ' ':
		    case '<': case '>': case ';':
		    case '$': case '`': case '\t':
			sfputc(iop,'\\');
			break;
		}
		sfputc(iop,c);
	}
	while(*cp++);
}

static void out_string(Sfio_t *iop, const char *cp, int c, int quoted)
{
	if(quoted)
	{
		int n = stktell(sh.stk);
		cp = sh_fmtq(cp);
		if(iop==sh.stk && cp==stkptr(sh.stk,n))
		{
			*stkptr(sh.stk,stktell(sh.stk)-1) = c;
			return;
		}
	}
	sfputr(iop,cp,c);
}

/*
 * If a script changes .sh.level inside a DEBUG trap, it will switch the
 * scope as if it were executing the trap at that function call depth.
 */
static void put_level(Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	Shscope_t	*sp;
	int16_t		level, oldlevel = sh.level;
	if(!val)
		return;
	nv_putv(np,val,flags,fp);
	level = sh.level;
	if(level < 0 || level > sh.fn_depth + sh.dot_depth)
	{
		sh.level = oldlevel;
		errormsg(SH_DICT,ERROR_exit(1),"%d: level out of range",level);
		UNREACHABLE();
	}
	if(level==oldlevel)
		return;
	if(sp = sh_getscope(level,SEEK_SET))
	{
		/* switch scopes, except for loop flow control state */
		int breakcnt = sh.st.breakcnt, loopcnt = sh.st.loopcnt;
		sh_setscope(sp);
		sh.st.breakcnt = breakcnt, sh.st.loopcnt = loopcnt;
	}
}

static const Namdisc_t level_disc = { sizeof(Namfun_t), put_level };
static Namfun_t level_disc_fun = { &level_disc, 1 };

/*
 * Execute the DEBUG trap:
 * write the current command on the stack and make it available as .sh.command
 */
int sh_debug(const char *trap, const char *name, const char *subscript, char *const argv[], int flags)
{
	Namval_t		*np = SH_COMMANDNOD;
	int			n=4, offset=stktell(sh.stk);
	void			*sav = stkfreeze(sh.stk,0);
	struct sh_scoped	*savst = stkalloc(sh.stk,sizeof(struct sh_scoped));
	const char		*cp = "+=( ";
	if(sh.indebug)
		return 0;
	sh.indebug = 1;
	if(name)
	{
		sfputr(sh.stk,name,-1);
		if(subscript)
		{
			sfputc(sh.stk,'[');
			out_string(sh.stk,subscript,']',1);
		}
		if(!(flags&ARG_APPEND))
			cp+=1, n-=1;
		if(!(flags&ARG_ASSIGN))
			n -= 2;
		sfwrite(sh.stk,cp,n);
	}
	if(*argv && !(flags&ARG_RAW))
		out_string(sh.stk, *argv++,' ', 0);
	n = (flags&ARG_ARITH);
	while(cp = *argv++)
	{
		if((flags&ARG_EXP) && argv[1]==0)
			out_pattern(sh.stk, cp,' ');
		else
			out_string(sh.stk, cp,' ',n?0: (flags&(ARG_RAW|ARG_NOGLOB))||*argv);
	}
	if(flags&ARG_ASSIGN)
		sfputc(sh.stk,')');
	else
		*stkptr(sh.stk,stktell(sh.stk)-1) = 0;
	np->nvalue = stkfreeze(sh.stk,1);
	sh.st.lineno = error_info.line;
	*savst = sh.st;
	sh.st.trap[SH_DEBUGTRAP] = 0;
	/* set up .sh.level variable */
	if(!SH_LEVELNOD->nvfun || !SH_LEVELNOD->nvfun->disc)
		nv_disc(SH_LEVELNOD,&level_disc_fun,NV_FIRST);
	nv_offattr(SH_LEVELNOD,NV_RDONLY);
	/* run the trap */
	n = sh_trap(trap,0);
	nv_onattr(SH_LEVELNOD,NV_RDONLY);
	np->nvalue = NULL;
	sh.indebug = 0;
	nv_onattr(SH_PATHNAMENOD,NV_NOFREE);
	nv_onattr(SH_FUNNAMENOD,NV_NOFREE);
	/* restore scope */
	update_sh_level();
	sh.st = *savst;
	if(sav != stkptr(sh.stk,0))
		stkset(sh.stk,sav,offset);
	else
		stkseek(sh.stk,offset);
	return n;
}

/*
 * Given stream <iop> compile and execute
 */
int sh_eval(Sfio_t *iop, int mode)
{
	Shnode_t *t;
	struct slnod *saveslp = sh.st.staklist;
	int jmpval;
	struct checkpt *pp = (struct checkpt*)sh.jmplist;
	struct checkpt *buffp = stkalloc(sh.stk,sizeof(struct checkpt));
	static Sfio_t *io_save;
	volatile int traceon=0, lineno=0;
	int binscript=sh.binscript;
	char comsub = sh.comsub;
	io_save = iop; /* preserve correct value across longjmp */
	sh.binscript = 0;
	sh.comsub = 0;
	sh_pushcontext(buffp,SH_JMPEVAL);
	buffp->olist = pp->olist;
	jmpval = sigsetjmp(buffp->buff,0);
	while(jmpval==0)
	{
		if(mode&SH_READEVAL)
		{
			lineno = sh.inlineno;
			if(traceon=sh_isoption(SH_XTRACE))
				sh_offoption(SH_XTRACE);
		}
		/* Read and parse the entire script into one node before executing */
		t = (Shnode_t*)sh_parse(iop,(mode&(SH_READEVAL|SH_FUNEVAL))?mode&SH_FUNEVAL:SH_NL);
		if(errno && sferror(iop))
		{
			/* Error reading, presumably from dot script file */
			errormsg(SH_DICT,ERROR_system(1),e_readscript);
			UNREACHABLE();
		}
		if(!(mode&SH_FUNEVAL) || !sfreserve(iop,0,0))
		{
			if(!(mode&SH_READEVAL))
				sfclose(iop);
			io_save = 0;
			mode &= ~SH_FUNEVAL;
		}
		mode &= ~SH_READEVAL;
		if(!sh_isoption(SH_VERBOSE))
			sh_offstate(SH_VERBOSE);
		if((mode&~SH_FUNEVAL) && sh.hist_ptr)
		{
			hist_flush(sh.hist_ptr);
			mode = sh_state(SH_INTERACTIVE);
		}
		sh_exec(t,sh_isstate(SH_ERREXIT)|sh_isstate(SH_NOFORK)|(mode&~SH_FUNEVAL));
		if(!(mode&SH_FUNEVAL))
			break;
	}
	sh_popcontext(buffp);
	sh.binscript = binscript;
	sh.comsub = comsub;
	if(traceon)
		sh_onoption(SH_XTRACE);
	if(lineno)
		sh.inlineno = lineno;
	if(io_save)
	{
		sfclose(io_save);
		io_save = 0;
	}

	sh_freeup();
	sh.st.staklist = saveslp;
	if(jmpval>SH_JMPEVAL)
		siglongjmp(*sh.jmplist,jmpval);
	return sh.exitval;
}

/*
 * returns 1 when option -<c> is specified
 */
static int checkopt(char *argv[], int c)
{
	char *cp;
	while(cp = *++argv)
	{
		if(*cp=='+')
			continue;
		if(*cp!='-' || cp[1]=='-')
			break;
		if(strchr(++cp,c))
			return 1;
		if(*cp=='h' && cp[1]==0 && *++argv==0)
			break;
	}
	return 0;
}

static void free_list(struct openlist *olist)
{
	struct openlist *item,*next;
	for(item=olist;item;item=next)
	{
		next = item->next;
		free(item);
	}
}

/*
 * set ${.sh.name} and ${.sh.subscript}
 * set _ to reference for ${.sh.name}[${.sh.subscript}]
 */
static long set_instance(Namval_t *nq, Namval_t *node, struct Namref *nr)
{
	char		*sp=0,*cp;
	Namarr_t	*ap;
	Namval_t	*np;
	if(!nv_isattr(nq,NV_MINIMAL|NV_EXPORT|NV_ARRAY) && (np = nq->nvmeta) && nv_isarray(np))
		nq = np;
	else if(nv_isattr(nq,NV_MINIMAL)==NV_MINIMAL && !nv_type(nq) && (np = nv_typeparent(nq)))
		nq = np;
	cp = nv_name(nq);
	memset(nr,0,sizeof(*nr));
	nr->np = nq;
	nr->root = sh.var_tree;
	nr->table = sh.last_table;
#if SHOPT_NAMESPACE
	if(!nr->table && sh.namespace)
		nr->table = sh.namespace;
#endif /* SHOPT_NAMESPACE */
	sh.instance = 1;
	if((ap=nv_arrayptr(nq)) && (sp = nv_getsub(nq)))
		sp = sh_strdup(sp);
	sh.instance = 0;
	if(sh.var_tree!=sh.var_base && !nv_search((char*)nq,nr->root,NV_REF|NV_NOSCOPE))
	{
#if SHOPT_NAMESPACE
		nr->root = sh.namespace?nv_dict(sh.namespace):sh.var_base;
#else
		nr->root = sh.var_base;
#endif /* SHOPT_NAMESPACE */
	}
	nv_putval(SH_NAMENOD, cp, NV_NOFREE);
	memcpy(node,L_ARGNOD,sizeof(*node));
	L_ARGNOD->nvalue = nr;
	L_ARGNOD->nvflag = NV_REF|NV_NOFREE;
	L_ARGNOD->nvfun = NULL;
	L_ARGNOD->nvmeta = NULL;
	if(sp)
	{
		nv_putval(SH_SUBSCRNOD,nr->sub=sp,NV_NOFREE);
		return ap->nelem&ARRAY_SCAN;
	}
	return 0;
}

static void unset_instance(Namval_t *node, struct Namref *nr, long mode)
{
	L_ARGNOD->nvalue = node->nvalue;
	L_ARGNOD->nvflag = node->nvflag;
	L_ARGNOD->nvfun = node->nvfun;
	if(nr->sub)
	{
		nv_putsub(nr->np, nr->sub, mode);
		free(nr->sub);
	}
	nv_unset(SH_NAMENOD,0);
	nv_unset(SH_SUBSCRNOD,0);
}

#if SHOPT_FILESCAN
    static Sfio_t *openstream(struct ionod *iop, int *save)
    {
	int err = errno, savein, fd = sh_redirect(iop,3);
	Sfio_t	*sp;
	savein = dup(0);
	if(fd==0)
		fd = savein;
	sp = sfnew(NULL,NULL,SFIO_UNBOUND,fd,SFIO_READ);
	while(close(0)<0 && errno==EINTR)
		errno = err;
	open(e_devnull,O_RDONLY);
	sh.offsets[0] = -1;
	sh.offsets[1] = 0;
	*save = savein;
	return sp;
    }
#endif /* SHOPT_FILESCAN */

#if SHOPT_NAMESPACE
static Namval_t *enter_namespace(Namval_t *nsp)
{
	Namval_t	*path=nsp, *fpath=nsp, *onsp=sh.namespace;
	Dt_t		*root=0,*oroot=0;
	char		*val;
	if(nsp)
	{
		if(!nv_istable(nsp))
			nsp = 0;
		else if(nv_dict(nsp)->view!=sh.var_base)
			return onsp;
	}
	if(!nsp && !onsp)
		return 0;
	if(onsp == nsp)
		return nsp;
	if(onsp)
	{
		oroot = nv_dict(onsp);
		if(!nsp)
		{
			path = nv_search(PATHNOD->nvname,oroot,NV_NOSCOPE);
			fpath = nv_search(FPATHNOD->nvname,oroot,NV_NOSCOPE);
		}
		if(sh.var_tree==oroot)
		{
			sh.var_tree = sh.var_tree->view;
			oroot = sh.var_base;
		}
	}
	if(nsp)
	{
		if(sh.var_tree==sh.var_base)
			sh.var_tree = nv_dict(nsp);
		else
		{
			for(root=sh.var_tree; root->view!=oroot;root=root->view);
			dtview(root,nv_dict(nsp));
		}
	}
	sh.namespace = nsp;
	if(path && (path = nv_search(PATHNOD->nvname,sh.var_tree,NV_NOSCOPE)) && (val=nv_getval(path)))
		nv_putval(path,val,NV_RDONLY);
	if(fpath && (fpath = nv_search(FPATHNOD->nvname,sh.var_tree,NV_NOSCOPE)) && (val=nv_getval(fpath)))
		nv_putval(fpath,val,NV_RDONLY);
	return onsp;
}
#endif /* SHOPT_NAMESPACE */

/*
 * Check whether to execve(2) the final command or make its redirections permanent.
 */
static int check_exec_optimization(int type, int execflg, int execflg2, struct ionod *iop)
{
	if(type&(FAMP|FPOU)
	|| !(execflg && sh.fn_depth==0 || execflg2)
	|| sh.st.trapdontexec
	|| sh.subshell
	|| ((struct checkpt*)sh.jmplist)->mode==SH_JMPEVAL
	|| sh_isstate(SH_XARG)
	|| (pipejob && (sh_isstate(SH_MONITOR) || sh_isoption(SH_PIPEFAIL) || sh_isstate(SH_TIMING))))
	{
		return 0;
	}
	/* '<>;' (IOREWRITE) redirections are incompatible with exec */
	while(iop && !(iop->iofile & IOREWRITE))
		iop = iop->ionxt;
	if(iop)
		return 0;
	return 1;
}

/*
 * Main execution function: execute any type of command.
 */
int sh_exec(const Shnode_t *t, int flags)
{
	int		type;
	int 		mainloop;
	sh_sigcheck();
	/* Bail out on no command, break/continue, or noexec */
	if(!t || sh.st.breakcnt || sh_isoption(SH_NOEXEC))
		return sh.exitval;
	/* Set up state */
	sh.exitval = 0;
	sh.lastsig = 0;
	sh.chldexitsig = 0;
	type = t->tre.tretyp;
	mainloop = (flags&sh_state(SH_INTERACTIVE));
	if(mainloop)
	{
		if(pipejob==2)
			job_unlock();
		nlock = 0;
		pipejob = 0;
		job.curpgid = 0;
		job.curjobid = 0;
		flags &= ~sh_state(SH_INTERACTIVE);
	}
	/* Optimize a simple literal ':', 'true', 'false', 'break', 'continue' if the conditions are right */
	else if(type==TCOM && !sh.dont_optimize_builtins)		/* no COMSCAN bit in 'type' => no expansions */
	{
		Namval_t	*np;
		Shbltin_f	fp;
		if((np = (Namval_t*)t->com.comnamp) && (fp = funptr(np))
		&& (fp==b_true || (fp==b_false && !sh_isoption(SH_ERREXIT) && !sh.st.trap[SH_ERRTRAP])
		   || fp==b_break && t->com.comarg.dp->dolnum==1)	/* for break/continue: 1 arg (command name) */
		&& !t->com.comset					/* no variable assignments list */
		&& !t->com.comio					/* no I/O redirections */
		&& !sh_isoption(SH_XTRACE)
		&& !sh.st.trap[SH_DEBUGTRAP])
		{
			/* Execute optimized basic versions of the builtins */
			if(fp==b_false)
				++sh.exitval;
			else if(fp==b_break && sh.st.loopcnt)
				sh.st.breakcnt = np==SYSCONT ? -1 : 1;
			/* Set $?, possibly execute traps, and we're out of here */
			exitset();
			if(sh.trapnote)
				sh_chktrap();
			return sh.exitval;
		}
	}
	/* Normal command execution */
	{
		char		*com0 = 0;
		int 		errorflg = (flags&sh_state(SH_ERREXIT))|(flags & ARG_OPTIMIZE);
		int 		execflg = (flags&sh_state(SH_NOFORK));
		int 		execflg2 = (flags&sh_state(SH_FORKED));
		int		topfd = sh.topfd;
		char 		*sav=stkfreeze(sh.stk,0);
		char		*cp=0, **com=0, *comn;
		int		argn;
		int 		skipexitset = 0;
		volatile int	was_interactive = 0;
		volatile int	was_errexit = sh_isstate(SH_ERREXIT);
		volatile int	was_monitor = sh_isstate(SH_MONITOR);
		volatile int	echeck = 0;
		sh_offstate(SH_DEFPATH);
		if(!(flags & sh_state(SH_ERREXIT)))
			sh_offstate(SH_ERREXIT);
		switch(type&COMMSK)
		{
		    /*
		     * Simple command
		     */
		    case TCOM:
		    {
			struct argnod	*argp;
			char		*trap;
			Namval_t	*np, *nq, *last_table;
			struct ionod	*io;
			int		command=0, flgs=NV_ASSIGN, jmpval=0;
			sh.bltindata.invariant = type>>(COMBITS+2);
			type &= (COMMSK|COMSCAN);
			sh_stats(STAT_SCMDS);
			error_info.line = t->com.comline-sh.st.firstline;
			com = sh_argbuild(&argn,&(t->com),flags & ARG_OPTIMIZE);
			echeck = 1;
			if(t->tre.tretyp&COMSCAN)
			{
				argp = t->com.comarg.ap;
				if(argp && *com && !(argp->argflag&ARG_RAW))
					sh_sigcheck();
			}
			np = (Namval_t*)(t->com.comnamp);
			nq = (Namval_t*)(t->com.comnamq);
#if SHOPT_NAMESPACE
			if(np && sh.namespace && nq!=sh.namespace && nv_isattr(np,NV_BLTIN|NV_INTEGER|BLT_SPC)!=(NV_BLTIN|BLT_SPC))
			{
				Namval_t *mp;
				if(mp = sh_fsearch(com[0],0))
				{
					nq = sh.namespace;
					np = mp;
				}
			}
#endif /* SHOPT_NAMESPACE */
			com0 = com[0];
			sh_offstate(SH_XARG);
			while(np==SYSCOMMAND || !np && com0 && nv_search(com0,sh.fun_tree,0)==SYSCOMMAND)
			{
				int n = b_command(0,com,&sh.bltindata);
				if(n==0)
					break;
				command += n;
				np = 0;
				if(!(com0= *(com+=n)))
					break;
				np = nv_bfsearch(com0, sh.bltin_tree, &nq, &cp);
			}
			if(sh_isstate(SH_XARG))
			{
				sh.xargmin -= command;
				sh.xargmax -= command;
				sh.xargexit = 0;
			}
			argn -= command;
			if(np && is_abuiltin(np))
			{
				if(!command)
				{
#if SHOPT_NAMESPACE
					Namval_t *mp;
					if(sh.namespace && (mp=sh_fsearch(np->nvname,0)))
						np = mp;
					else
#endif /* SHOPT_NAMESPACE */
					np = dtsearch(sh.fun_tree,np);
				}
			}
			if(com0)
			{
				if((!np || !np->nvflag) && !sh_isstate(SH_EXEC) && !strchr(com0,'/'))
				{
					Dt_t *root = command?sh.bltin_tree:sh.fun_tree;
					np = nv_bfsearch(com0, root, &nq, &cp);
#if SHOPT_NAMESPACE
					if(sh.namespace && !nq && !cp)
						np = sh_fsearch(com0,0);
#endif /* SHOPT_NAMESPACE */
				}
				comn = com[argn-1];
			}
			io = t->tre.treio;
			if(sh.envlist = argp = t->com.comset)
			{
				if(argn==0 || (np && (nv_isattr(np,BLT_DCL) || (!command && nv_isattr(np,BLT_SPC)))))
				{
					Namval_t *tp=0;
					if(argn)
					{
						if(checkopt(com,'A'))
							flgs |= NV_ARRAY;
						else if(checkopt(com,'a'))
							flgs |= NV_IARRAY;
					}
					if(np && funptr(np)==b_typeset)
					{
						/* command calls b_typeset(); treat as a typeset variant */
						flgs |= NV_UNATTR;  /* unset previous attributes before assigning */
						if(np < SYSTYPESET || np > SYSTYPESET_END)
						{
							sh.typeinit = np;
							tp = nv_type(np);
						}
						if(np==SYSCOMPOUND || checkopt(com,'C'))
							flgs |= NV_COMVAR;
						if(checkopt(com,'S'))
							flgs |= NV_STATIC;
						if(checkopt(com,'m'))
							flgs |= NV_MOVE;
						if(checkopt(com,'g'))
							flgs |= NV_GLOBAL;
						if(np==SYSNAMEREF || checkopt(com,'n'))
							flgs |= NV_NOREF;
						else if(argn>=3 && checkopt(com,'T'))
						{
							if(sh.subshell && !sh.subshare)
								sh_subfork();
#if SHOPT_NAMESPACE
							if(sh.namespace)
							{
								sfputr(sh.strbuf,NV_CLASS,-1);
								sfputr(sh.strbuf,nv_name(sh.namespace),-1);
								sh.prefix = sh_strdup(sfstruse(sh.strbuf));
								nv_open(sh.prefix,sh.var_base,NV_VARNAME);
							}
							else
#endif /* SHOPT_NAMESPACE */
							sh.prefix = NV_CLASS;
							flgs |= NV_TYPE;
						}
						if(sh.fn_depth && !sh.prefix)
							flgs |= NV_NOSCOPE;
					}
					else if(np==SYSEXPORT)
						flgs |= NV_EXPORT;
					if(flgs&(NV_EXPORT|NV_NOREF))
						flgs |= NV_IDENT;
					else
						flgs |= NV_VARNAME;
					/* execute the list of assignments */
					if((!np || nv_isattr(np,BLT_SPC)) && !command || sh.mktype)
					{
						/* (bare assignment(s) or special builtin) and no 'command' prefix,
						 * or we're inside a type definition: exit on error */
						nv_setlist(argp,flgs,tp);
					}
					else
					{
						/* avoid exit on error from nv_setlist, e.g. read-only variable */
						struct checkpt *chkp = stkalloc(sh.stk,sizeof(struct checkpt));
						sh_pushcontext(chkp,SH_JMPCMD);
						jmpval = sigsetjmp(chkp->buff,0);
						if(!jmpval)
							nv_setlist(argp,flgs,tp);
						sh_popcontext(chkp);
						if(jmpval)	/* error occurred */
						{
							if(jmpval>SH_JMPCMD)
								siglongjmp(*sh.jmplist,jmpval);
							goto setexit;
						}
					}
					if(np==sh.typeinit)
						sh.typeinit = 0;
					sh.envlist = argp;
					argp = NULL;
				}
			}
			last_table = sh.last_table;
			sh.last_table = 0;
			if(io || argn)
			{
				static char *argv[2];
				int tflags = 1;
				if(np && nv_isattr(np,BLT_DCL))
					tflags |= 2;
				if(execflg && !check_exec_optimization(type,execflg,execflg2,io))
					execflg = 0;
				if(argn==0)
				{
					/* fake 'true' built-in */
					np = SYSTRUE;
					*argv = nv_name(np);
					com = argv;
				}
				/* set +x doesn't echo */
				else if((t->tre.tretyp&FSHOWME) && sh_isoption(SH_SHOWME))
				{
					int ison = sh_isoption(SH_XTRACE);
					if(!ison)
						sh_onoption(SH_XTRACE);
					sh_trace(com-command,tflags);
					if(io)
						sh_redirect(io,SH_SHOWME);
					if(!ison)
						sh_offoption(SH_XTRACE);
					break;
				}
				else if((np!=SYSSET) && sh_isoption(SH_XTRACE))
					sh_trace(com-command,tflags);
				if(trap=sh.st.trap[SH_DEBUGTRAP])
				{
					int n = sh_debug(trap,NULL,NULL,com,ARG_RAW);
					if(n==255 && sh.fn_depth+sh.dot_depth)
					{
						np = SYSRETURN;
						argn = 1;
						com[0] = np->nvname;
						com[1] = 0;
						io = 0;
						argp = 0;
					}
					else if(n==2)
					{
						/* Do not execute next command; keep exit status from trap handler */
						sh.exitval = n;
						break;
					}
				}
				if(io)
					sfsync(sh.outpool);
				if(!np && !sh_isstate(SH_EXEC))
				{
					if(!sh_isoption(SH_RESTRICTED) || !strchr(com0,'/'))
					{
						/* Search for a built-in again (including, unless restricted, a path-bound
						 * builtin referenced by canonical path) in case no node pointer was found
						 * above or at parse time */
						np = nv_search(com0, sh.bltin_tree, 0);
					}
					if(np || strchr(com0,'/'))
					{
						/* Do nothing */
					}
					else if(path_search(com0,NULL,1))
					{
						error_info.line = t->com.comline-sh.st.firstline;
#if SHOPT_NAMESPACE
						if(!sh.namespace || !(np=sh_fsearch(com0,0)))
#endif /* SHOPT_NAMESPACE */
							np=nv_search(com0,sh.fun_tree,0);
						if(!np || !np->nvalue)
						{
							Namval_t *mp=nv_search(com0,sh.bltin_tree,0);
							if(mp)
								np = mp;
						}
					}
					else if(np = path_gettrackedalias(com0))
						np = nv_search(nv_getval(np),sh.bltin_tree,0);
				}
				if(np && pipejob==2)
				{
					job_unlock();
					nlock--;
					pipejob = 1;
				}
				/* check for builtins */
				if(np && is_abuiltin(np))
				{
					volatile char	scope, share, was_mktype, was_nofork;
					volatile void	*save_ptr;
					volatile void	*save_data;
					int		save_prompt;
					struct checkpt	*buffp;
					Shbltin_t	*bp = &sh.bltindata;
					/* Fallback optimization for ':'/'true' and 'false' */
					if(!io && !argp && (funptr(np)==b_true || funptr(np)==b_false && ++sh.exitval))
						goto setexit;
					scope = 0, share = 0;
					was_mktype = sh.mktype!=NULL;
					was_nofork = execflg && sh_isstate(SH_NOFORK);
					save_ptr = bp->ptr;
					save_data = bp->data;
					if(execflg)
						sh_onstate(SH_NOFORK);
					buffp = stkalloc(sh.stk,sizeof(struct checkpt));
					sh_pushcontext(buffp,SH_JMPCMD);
					jmpval = sigsetjmp(buffp->buff,0);
					if(jmpval == 0)
					{
						if(!(nv_isattr(np,BLT_ENV)))
							error_info.flags |= ERROR_SILENT;
						errorpush(&buffp->err,0);
						if(io)
						{
							struct openlist *item;
							/* Below, 'type' is the type flag for sh_redirect; see there for info */
							if(np == SYSEXEC)		/* 'exec' */
							{
								/* Interactive shells no longer force an exit on failure to exec,
								 * so if there is a program to 'exec', save the file descriptor
								 * state (type==0) on interactive shells in case path_exec fails */
								if(com[1])
									type = sh_isstate(SH_INTERACTIVE) ? 0 : 1;
								else
									type = 2;	/* no operand: mark FD > 2 close-on-exec */
							}
							else if(np == SYSREDIR)		/* 'redirect' */
							{
								if(com[1])		/* do not allow operands */
								{
									errormsg(SH_DICT, ERROR_exit(2), "%s: %s: %s",
										 SYSREDIR->nvname, e_badsyntax, com[1]);
									UNREACHABLE();
								}
								type = 2;
							}
							else if(execflg)
								type = 1;		/* don't bother to save redirection state */
							else
								type = 0;		/* normal non-persistent redirection */
							sh.redir0 = 1;
							sh_redirect(io,type);
							for(item=buffp->olist;item;item=item->next)
								item->strm=0;
						}
						if(!(nv_isattr(np,BLT_ENV)))
						{
							sfsync(NULL);
							share = sfset(sfstdin,SFIO_SHARE,0);
							sh_onstate(SH_STOPOK);
							sfpool(sfstderr,NULL,SFIO_WRITE);
							sfset(sfstderr,SFIO_LINE,1);
							save_prompt = sh.nextprompt;
							sh.nextprompt = 0;
						}
						if(argp)
						{
							scope++;
							sh.invoc_local++;
							sh_scope(argp,0);
						}
						opt_info.index = opt_info.offset = 0;
						opt_info.disc = 0;
						error_info.id = *com;
						if(argn)
							sh.exitval = 0;
						sh.bltinfun = funptr(np);
						bp->bnode = np;
						bp->vnode = nq;
						bp->ptr = nv_context(np);
						bp->data = t->com.comstate;
						bp->sigset = 0;
						bp->notify = 0;
						bp->flags = ((flags & ARG_OPTIMIZE)!=0);
						if(sh.subshell && nv_isattr(np,BLT_NOSFIO))
							sh_subtmpfile();
						if(argn)
							sh.exitval = (*sh.bltinfun)(argn,com,bp);
						if(error_info.flags&ERROR_INTERACTIVE)
							tty_check(ERRIO);
						((Shnode_t*)t)->com.comstate = sh.bltindata.data;
						bp->data = (void*)save_data;
						if(sh.exitval && errno==EINTR && sh.lastsig)
							sh.exitval = SH_EXITSIG|sh.lastsig;
						else if(!nv_isattr(np,BLT_EXIT))
							sh.exitval &= SH_EXITMASK;
					}
					else
					{
						struct openlist *item;
						for(item=buffp->olist;item;item=item->next)
						{
							if(item->strm)
							{
								sfclrlock(item->strm);
								if(sh.hist_ptr && item->strm == sh.hist_ptr->histfp)
									hist_close(sh.hist_ptr);
								else
									sfclose(item->strm);
							}
						}
						if(sh.bltinfun && (error_info.flags&ERROR_NOTIFY))
							(*sh.bltinfun)(-2,com,bp);
						/* failure on special built-ins fatal */
						if(jmpval<=SH_JMPCMD && (!nv_isattr(np,BLT_SPC) || command) && !was_mktype)
							jmpval=0;
#if !SHOPT_DEVFD
						fifo_cleanup();
#endif
					}
					bp->bnode = 0;
					if(bp->ptr != nv_context(np))
						np->nvfun = bp->ptr;
					if(execflg && !was_nofork)
						sh_offstate(SH_NOFORK);
					if(!(nv_isattr(np,BLT_ENV)))
					{
						sh_offstate(SH_STOPOK);
						if(share&SFIO_SHARE)
							sfset(sfstdin,SFIO_PUBLIC|SFIO_SHARE,1);
						sfset(sfstderr,SFIO_LINE,0);
						sfpool(sfstderr,sh.outpool,SFIO_WRITE);
						sfpool(sfstdin,NULL,SFIO_WRITE);
						sh.nextprompt = save_prompt;
					}
					sh_popcontext(buffp);
					errorpop(&buffp->err);
					error_info.flags &= ~(ERROR_SILENT|ERROR_NOTIFY);
					sh.bltinfun = 0;
					if(buffp->olist)
						free_list(buffp->olist);
					if(scope)
					{
						sh_unscope();
						sh.invoc_local--;
					}
					bp->ptr = (void*)save_ptr;
					bp->data = (void*)save_data;
					sh.redir0 = 0;
					if(jmpval)
						siglongjmp(*sh.jmplist,jmpval);
					goto setexit;
				}
				/* check for functions */
				if(!command && np && nv_isattr(np,NV_FUNCTION))
				{
					volatile int indx;
					volatile char scope = 0;
					struct checkpt *buffp = stkalloc(sh.stk,sizeof(struct checkpt));
#if SHOPT_NAMESPACE
					Namval_t *namespace=0;
#endif /* SHOPT_NAMESPACE */
					Namval_t	*nodep;
					struct Namref	*nrp;
					long		mode = 0;
					struct slnod *slp;
					if(!np->nvalue)
					{
						indx = path_search(com0,NULL,0);
						if(indx==1)
						{
#if SHOPT_NAMESPACE
							if(sh.namespace)
								np = sh_fsearch(com0,0);
							else
#endif /* SHOPT_NAMESPACE */
							np = nv_search(com0,sh.fun_tree,NV_NOSCOPE);
						}
						if(!np->nvalue)
						{
							if(indx==1)
							{
								errormsg(SH_DICT,ERROR_exit(0),e_defined,com0);
								sh.exitval = ERROR_NOEXEC;
							}
							else
							{
								errormsg(SH_DICT,ERROR_exit(0),e_found,"function");
								sh.exitval = ERROR_NOENT;
							}
							goto setexit;
						}
					}
					/* increase refcnt for unset */
					slp = np->nvmeta;
					sh_funstaks(slp->slchild,1);
					if(slp->slptr)
						stklink(slp->slptr);
					if(nq)
					{
						Namval_t *mp=0;
						if(nv_isattr(np,NV_STATICF) && (mp=nv_type(nq)))
							nq = mp;
						sh.last_table = last_table;
						nodep = stkalloc(sh.stk,sizeof(Namval_t));
						nrp = stkalloc(sh.stk,sizeof(struct Namref));
						mode = set_instance(nq,nodep,nrp);
					}
					if(io)
					{
						indx = sh.topfd;
						sh_pushcontext(buffp,SH_JMPIO);
						jmpval = sigsetjmp(buffp->buff,0);
					}
					if(jmpval == 0)
					{
						if(argp)
						{
							sh.invoc_local++;
							scope++;
						}
						if(io)
							indx = sh_redirect(io,execflg);
#if SHOPT_NAMESPACE
						if(*np->nvname=='.')
						{
							char *cp = strchr(np->nvname+1,'.');
							if(cp)
							{
								*cp = 0;
								namespace = nv_search(np->nvname,sh.var_base,NV_NOSCOPE);
								*cp = '.';
							}
						}
						namespace = enter_namespace(namespace);
#endif /* SHOPT_NAMESPACE */
						sh_funct(np,argn,com,t->com.comset,(flags&~ARG_OPTIMIZE));
					}
#if SHOPT_NAMESPACE
					enter_namespace(namespace);
#endif /* SHOPT_NAMESPACE */
					if(io)
					{
						if(buffp->olist)
							free_list(buffp->olist);
						sh_popcontext(buffp);
						sh_iorestore(indx,jmpval);
					}
					if(scope)
						sh.invoc_local--;
					if(nq)
						unset_instance(nodep,nrp,mode);
					sh_funstaks(slp->slchild,-1);
					if(slp->slptr)
					{
						Stk_t *sp = slp->slptr;
						slp->slptr = NULL;
						stkclose(sp);
					}
					if(jmpval > SH_JMPFUN || (io && jmpval > SH_JMPIO))
						siglongjmp(*sh.jmplist,jmpval);
					goto setexit;
				}
				/* not a built-in or function: external command, fall through to TFORK */
			}
			else if(!io)
			{
			setexit:
#if !SHOPT_DEVFD
				fifo_cleanup();
#endif
				if(sh.topfd > topfd && !(sh.subshell && (np==SYSEXEC || np==SYSREDIR)))
					sh_iorestore(topfd,jmpval);  /* avoid leaking unused file descriptors */
				exitset();
				break;
			}
		    }
		    /* FALLTHROUGH */

		    /*
		     * Any command that needs the shell to fork (e.g. background or external)
		     */
		    case TFORK:
		    {
			pid_t parent;
			int no_fork,jobid;
			int pipes[3];
			if(sh.subshell)
				sh_subtmpfile();
			if(no_fork = check_exec_optimization(type,execflg,execflg2,t->fork.forkio))
				parent = 0;
			else
			{
#if SHOPT_BGX
				int maxjob;
				if(((type&(FAMP|FINT)) == (FAMP|FINT)) && (maxjob=nv_getnum(JOBMAXNOD))>0)
				{
					while(job.numbjob >= maxjob)
					{
						job_lock();
						job_reap(0);
						job_unlock();
					}
				}
#endif /* SHOPT_BGX */
				if(type&FCOOP)
				{
					pipes[2] = 0;
					coproc_init(pipes);
				}
#if !SHOPT_DEVFD
				if(sh.fifo)
					fifo_save_ppid = sh.current_pid;
#endif
#if SHOPT_SPAWN
#if _use_ntfork_tcpgrp
				if(com)
#else
				if(com && !job.jobcontrol)
#endif /* _use_ntfork_tcpgrp */
				{
					parent = sh_ntfork(t,com,&jobid,topfd);
					if(parent<0)
						break;
				}
				else
#endif /* SHOPT_SPAWN */
					parent = sh_fork(type,&jobid);
			}
			if(job.parent=parent)
			/* This is the parent branch of fork
			 * It may or may not wait for the child
			 */
			{
				if(pipejob==2)
				{
					pipejob = 1;
					nlock--;
					job_unlock();
				}
				if(sh.subshell)
					sh.spid = parent;
				if(type&FPCL)
					sh_close(sh.inpipe[0]);
				if(type&(FCOOP|FAMP))
					sh.bckpid = parent;
				else if(!(type&(FAMP|FPOU)))
				{
					if(!sh_isstate(SH_MONITOR))
					{
						if(!(sh.sigflag[SIGINT]&(SH_SIGFAULT|SH_SIGOFF)))
							sh_sigtrap(SIGINT);
						sigblock(SIGINT);
					}
					if(sh.pipepid)
						sh.pipepid = parent;
					else
					{
						job_wait(parent);
						if(parent==sh.spid)
							sh.spid = 0;
					}
					if(sh.topfd > topfd)
						sh_iorestore(topfd,0);
					if(!sh_isstate(SH_MONITOR))
						sigrelease(SIGINT);
				}
				/* print job number */
				if(type&FAMP && (sh_isstate(SH_INTERACTIVE) || sh_isstate(SH_PROFILE)) && !sh_isstate(SH_PROCSUB) && !sh.realsubshell)
					sfprintf(sfstderr,"[%d]\t%d\n",jobid,parent);
				break;
			}
			else
			/*
			 * this is the FORKED branch (child) of execute
			 */
			{
				volatile int jmpval;
				struct checkpt *buffp = stkalloc(sh.stk,sizeof(struct checkpt));
				struct ionod *iop;
				int	rewrite=0;
#if !SHOPT_DEVFD
				char	*save_sh_fifo = sh.fifo;
				if(sh.fifo_tree)
				{
					/* do not clean up process substitution FIFOs in child; parent handles this */
					dtclose(sh.fifo_tree);
					sh.fifo_tree = NULL;
				}
#endif
				sh_invalidate_rand_seed();
				if(no_fork)
					sh_sigreset(2);
				sh_pushcontext(buffp,SH_JMPEXIT);
				jmpval = sigsetjmp(buffp->buff,0);
				if(jmpval)
					goto done;
				if((type&FINT) && !sh_isstate(SH_MONITOR))
				{
					/* default std input for & */
					signal(SIGINT,SIG_IGN);
					signal(SIGQUIT,SIG_IGN);
					if(!sh.st.ioset)
					{
						if(sh_close(0)>=0)
							sh_chkopen(e_devnull);
					}
				}
				sh_offstate(SH_INTERACTIVE);
				/* pipe in or out */
				if((type&FAMP) && sh_isoption(SH_BGNICE))
					nice(4);
#if !SHOPT_DEVFD
				if(sh.fifo && (type&(FPIN|FPOU)))
				{
					int	fn, fd, save_errno;
					void	*fifo_timer = sh_timeradd(50,1,fifo_check,NULL);
					fd = (type&FPIN) ? 0 : 1;
					fn = sh_open(sh.fifo,fd?O_WRONLY:O_RDONLY);
					save_errno = errno;
					sh_timerdel(fifo_timer);
					sh.fifo = 0;
					if(fn<0)
					{
						if((errno = save_errno) != ENOENT)
						{
							errormsg(SH_DICT, ERROR_SYSTEM|ERROR_PANIC,
								 "process substitution: FIFO open failed");
							UNREACHABLE();
						}
						sh_done(0);
					}
					sh_iorenumber(fn,fd);
					sh_close(fn);
					type &= ~(FPIN|FPOU);
				}
#endif /* !SHOPT_DEVFD */
				if(type&FPIN)
				{
					sh_iorenumber(sh.inpipe[0],0);
					if(!(type&FPOU) || (type&FCOOP))
						sh_close(sh.inpipe[1]);
				}
				if(type&FPOU)
				{
					sh_iorenumber(sh.outpipe[1],1);
					sh_pclose(sh.outpipe);
				}
				if((type&COMMSK)!=TCOM)
					error_info.line = t->fork.forkline-sh.st.firstline;
				if(sh.topfd)
					sh_iounsave();
				topfd = sh.topfd;
				if(com0 && (iop=t->tre.treio))
				{
					for(;iop;iop=iop->ionxt)
					{
						if(iop->iofile&IOREWRITE)
							rewrite = 1;
					}
				}
				sh_redirect(t->tre.treio,1);
				if(rewrite)
				{
					job_lock();
					while((parent = fork()) < 0)
						_sh_fork(parent, 0, NULL);
					if(parent)
					{
						job.toclear = 0;
						job_post(parent,0);
						job_wait(parent);
						sh_iorestore(topfd,SH_JMPCMD);
						sh_done((sh.exitval&SH_EXITSIG)?(sh.exitval&SH_EXITMASK):0);
					}
					job_unlock();
				}
				if((type&COMMSK)!=TCOM)
				{
					/* don't clear job table for out
					   pipes so that jobs command can
					   be used in a pipeline
					 */
					if(!no_fork && !(type&FPOU))
						job_clear();
					sh_exec(t->fork.forktre,flags|sh_state(SH_NOFORK)|sh_state(SH_FORKED));
				}
				else if(com0)
				{
					sh_offoption(SH_ERREXIT);
					sh_freeup();
					path_exec(com0,com,t->com.comset);
				}
			done:
#if !SHOPT_DEVFD
				if(save_sh_fifo)
				{
					unlink(save_sh_fifo);
					free(save_sh_fifo);
				}
#endif
				sh_popcontext(buffp);
				if(jmpval>SH_JMPEXIT)
					siglongjmp(*sh.jmplist,jmpval);
				sh_done(0);
			}
		    }

		    /*
		     * Redirection:
		     * don't create a new process, just
		     * save and restore io-streams
		     */
		    case TSETIO:
		    {
			pid_t	pid = 0;
			int 	jmpval, waitall = 0;
			int 	simple = (t->fork.forktre->tre.tretyp&COMMSK)==TCOM;
			struct checkpt *buffp = stkalloc(sh.stk,sizeof(struct checkpt));
			if(sh.subshell && !sh.subshare)
			{
				/* Subshell forking workaround for:
				 * https://github.com/ksh93/ksh/issues/161 (check each redirection for >&- or <&-)
				 * https://github.com/ksh93/ksh/issues/784 (check for stdout in a command substitution)
				 * TODO: find the elusive real fix */
				struct ionod *i;
				for (i = t->fork.forkio; i; i = i->ionxt)
				{
					unsigned f = i->iofile;
					if ((f & ~(IOUFD|IOPUT))==(IOMOV|IORAW) && !strcmp(i->ioname,"-") || (f & IOUFD)==1 && sh.comsub)
					{
						sh_subfork();
						break;
					}
				}
			}
			sh_pushcontext(buffp,SH_JMPIO);
			if(type&FPIN)
			{
				was_interactive = sh_isstate(SH_INTERACTIVE);
				sh_offstate(SH_INTERACTIVE);
				sh_iosave(0,sh.topfd,NULL);
				sh.pipepid = simple;
				sh_iorenumber(sh.inpipe[0],0);
				/*
				 * if read end of pipe is a simple command
				 * treat as non-shareable to improve performance
				 */
				if(simple)
					sfset(sfstdin,SFIO_PUBLIC|SFIO_SHARE,0);
				waitall = job.waitall;
				job.waitall = 0;
				pid = job.parent;
			}
			else
				error_info.line = t->fork.forkline-sh.st.firstline;
			jmpval = sigsetjmp(buffp->buff,0);
			if(jmpval==0)
			{
				if(execflg && !check_exec_optimization(type,execflg,execflg2,t->fork.forkio))
				{
					execflg = 0;
					flags &= ~sh_state(SH_NOFORK);
				}
				sh_redirect(t->fork.forkio,execflg);
				(t->fork.forktre)->tre.tretyp |= t->tre.tretyp&FSHOWME;
				sh_exec(t->fork.forktre,flags&~simple);
			}
			else
				sfsync(sh.outpool);
			sh_popcontext(buffp);
			sh_iorestore(buffp->topfd,jmpval);
			if(buffp->olist)
				free_list(buffp->olist);
			if(type&FPIN)
			{
				int e = sh.exitval, c = sh.chldexitsig;
				job.waitall = waitall;
				if(!(e & SH_EXITSIG))
				{
					/* wait for remainder of pipeline */
					if(sh.pipepid>1)
					{
						job_wait(sh.pipepid);
						e = sh.exitval, c = sh.chldexitsig;
					}
					else
						job_wait(waitall?pid:0);
					if(e || !sh_isoption(SH_PIPEFAIL))
						sh.exitval = e, sh.chldexitsig = c;
				}
				sh.pipepid = 0;
				sh.st.ioset = 0;
			}
			if(jmpval>SH_JMPIO)
				siglongjmp(*sh.jmplist,jmpval);
			break;
		    }

		    /*
		     * Parentheses subshell block
		     */
		    case TPAR:
			echeck = 1;
			flags &= ~ARG_OPTIMIZE;
			if(!sh.subshell && !sh.st.trapdontexec && (flags&sh_state(SH_NOFORK)))
			{
				/* This is the last command, so avoid creating a subshell */
				char *savsig;
				int nsig,jmpval;
				struct checkpt *buffp = stkalloc(sh.stk,sizeof(struct checkpt));
				sh.st.otrapcom = 0;
				if((nsig=sh.st.trapmax*sizeof(char*))>0 || sh.st.trapcom[0])
				{
					nsig += sizeof(char*);
					savsig = sh_malloc(nsig);
					memcpy(savsig,(char*)&sh.st.trapcom[0],nsig);
					sh.st.otrapcom = (char**)savsig;
				}
				/* Still act like a subshell: reseed $RANDOM and increment ${.sh.subshell} */
				sh_invalidate_rand_seed();
				sh.realsubshell++;
				sh_sigreset(0);
				sh_pushcontext(buffp,SH_JMPEXIT);
				jmpval = sigsetjmp(buffp->buff,0);
				if(jmpval==0)
					sh_exec(t->par.partre,flags);
				sh_popcontext(buffp);
				if(jmpval > SH_JMPEXIT)
					siglongjmp(*sh.jmplist,jmpval);
				sh_done(0);
			}
			else
				sh_subshell(t->par.partre,flags,0);
			break;

		    /*
		     * Pipe: command | command
		     * All elements of the pipe are started by the parent.
		     * The last element is executed in the current environment.
		     */
		    case TFIL:
		    {
			int	pvo[3];	/* old pipe for multi-stage */
			int	pvn[3];	/* current set up pipe */
			int	savepipe = pipejob;
			int	savelock = nlock;
			int	showme = t->tre.tretyp&FSHOWME;
			int	e, c;
			int	waitall, savewaitall = job.waitall;
			int	savejobid = job.curjobid;
			int	*exitval=0,*saveexitval = job.exitval;
			pid_t	savepgid = job.curpgid;
			echeck = 1;
			job.exitval = 0;
			job.curjobid = 0;
			if(sh.subshell)
				sh_subtmpfile();
			sh.inpipe = pvo;
			sh.outpipe = pvn;
			pvo[1] = -1;
			/*
			 * If the pipefail or monitor options are on or if the time keyword is in use, then wait
			 * for all commands in the pipeline to complete; otherwise, wait for the last one only
			 */
			if(sh_isoption(SH_PIPEFAIL))
			{
				const Shnode_t* tn=t;
				job.waitall = 2;
				job.curpgid = 0;
				while((tn=tn->lst.lstrit) && tn->tre.tretyp==TFIL)
					job.waitall++;
				exitval = job.exitval = stkalloc(sh.stk,job.waitall*sizeof(int));
				memset(exitval,0,job.waitall*sizeof(int));
			}
			else
				job.waitall = !pipejob && (sh_isstate(SH_MONITOR) || sh_isstate(SH_TIMING));
			job_lock();
			nlock++;
			do
			{
				/* create the pipe */
				sh_pipe(pvn);
				/* execute out part of pipe no wait */
				(t->lst.lstlef)->tre.tretyp |= showme;
				type = sh_exec(t->lst.lstlef, errorflg);
				/* close out-part of pipe */
				sh_close(pvn[1]);
				pipejob=1;
				/* save the pipe stream-ids */
				pvo[0] = pvn[0];
				/* pipeline all in one process group */
				t = t->lst.lstrit;
			}
			/* repeat until end of pipeline */
			while(!type && t->tre.tretyp==TFIL);
			sh.inpipe = pvn;
			sh.outpipe = 0;
			pipejob = 2;
			waitall = job.waitall;
			job.waitall = 0;
			if(type == 0)
			{
				/*
				 * execute last element of pipeline
				 * in the current process
				 */
				((Shnode_t*)t)->tre.tretyp |= showme;
				sh_exec(t,flags);
			}
			else
				/* execution failure, close pipe */
				sh_pclose(pvn);
			if(pipejob==2)
				job_unlock();
			if((pipejob = savepipe) && nlock<savelock)
				pipejob = 1;
			e = sh.exitval, c = sh.chldexitsig;
			if(job.waitall = waitall)
			{
				if(sh_isstate(SH_MONITOR))
					job_wait(0);
				else
				{
					sh.intrap++;
					job_wait(0);
					sh.intrap--;
				}
			}
			if(e==0 && exitval)
			{
				while(exitval <= --job.exitval)
				{
					if(*job.exitval)
					{
						e = *job.exitval;
						c = 0;
						break;
					}
				}
			}
			sh.exitval = e, sh.chldexitsig = c;
			if(!pipejob && sh_isstate(SH_MONITOR) && job.jobcontrol)
				tcsetpgrp(JOBTTY,sh.pid);
			job.curpgid = savepgid;
			job.exitval = saveexitval;
			job.waitall = savewaitall;
			job.curjobid = savejobid;
			break;
		    }

		    /*
		     * List of semicolon-separated commands
		     */
		    case TLST:
		    {
			do
			{
				sh_exec(t->lst.lstlef,errorflg|(flags & ARG_OPTIMIZE));
				t = t->lst.lstrit;
			}
			while(t->tre.tretyp == TLST);
			sh_exec(t,flags);
			break;
		    }

		    /*
		     * Logical and: command && command
		     */
		    case TAND:
			if(type&TTEST)
				skipexitset++;
			if(sh_exec(t->lst.lstlef, flags & ARG_OPTIMIZE)==0)
				sh_exec(t->lst.lstrit,flags);
			break;

		    /*
		     * Logical or: command || command
		     */
		    case TORF:
			if(type&TTEST)
				skipexitset++;
			if(sh_exec(t->lst.lstlef, flags & ARG_OPTIMIZE)!=0)
				sh_exec(t->lst.lstrit,flags);
			break;

		    /*
		     * Loop: iterative 'for' or 'select'
		     */
		    case TFOR:
		    {
			char **args;
			int nargs;
			Namval_t *np;
			int flag = errorflg|ARG_OPTIMIZE;
			struct dolnod	*argsav = NULL;
			struct comnod	*tp;
			char *cp, *trap, *null_pointer = NULL;
			int nameref, refresh=1;
			char *av[5];
#if SHOPT_OPTIMIZE
			int  jmpval = ((struct checkpt*)sh.jmplist)->mode;
			struct checkpt *buffp = stkalloc(sh.stk,sizeof(struct checkpt));
			void *optlist = sh.optlist;
			sh.optlist = 0;
			sh_tclear(t->for_.fortre);
			sh_pushcontext(buffp,jmpval);
			jmpval = sigsetjmp(buffp->buff,0);
			if(jmpval)
				goto endfor;
#endif /* SHOPT_OPTIMIZE */
			error_info.line = t->for_.forline-sh.st.firstline;
			if(!(tp=t->for_.forlst))
			{
				args=sh.st.dolv+1;
				nargs = sh.st.dolc;
				argsav=sh_arguse();
			}
			else
			{
				args=sh_argbuild(&argn,tp,0);
				nargs = argn;
			}
			np = nv_open(t->for_.fornam, sh.var_tree,NV_NOARRAY|NV_VARNAME|NV_NOREF);
			nameref = nv_isref(np)!=0;
			sh.st.loopcnt++;
			cp = *args;
			while(cp && sh.st.breakcnt==0)
			{
				if(t->tre.tretyp&COMSCAN)
				{
					char *val;
					int save_prompt;
					if(refresh)
					{
						sh_menu(sfstderr,nargs,args);
						refresh = 0;
					}
					save_prompt = sh.nextprompt;
					sh.nextprompt = 3;
					sh.timeout = 0;
					sh.exitval=sh_readline(&null_pointer,0,1,0,1000*(Sflong_t)sh.st.tmout);
					sh.nextprompt = save_prompt;
					if(sh.exitval||sfeof(sfstdin)||sferror(sfstdin))
					{
						sh.exitval = 1;
						break;
					}
					if(!(val=nv_getval(sh_scoped(REPLYNOD))))
						continue;
					else
					{
						int c;  /* user's menu choice */
						if(*(cp=val) == 0)
						{
							refresh++;
							goto check;
						}
						while(c = *cp++)
							if(c < '0' && c > '9')
								break;
						if(c!=0)
							c = nargs;
						else
							c = (int)strtol(val, NULL, 10)-1;
						if(c<0 || c >= nargs)
							cp = Empty;
						else
							cp = args[c];
					}
				}
				if(nameref)
					nv_offattr(np,NV_REF|NV_NOOPTIMIZE);
				else if(nv_isattr(np, NV_ARRAY))
					nv_putsub(np,NULL,0L);
				nv_putval(np,cp,0);
				if(nameref)
				{
					nv_setref(np,NULL,NV_VARNAME);
					nv_onattr(np,NV_NOOPTIMIZE);
				}
				if(trap=sh.st.trap[SH_DEBUGTRAP])
				{
					av[0] = (t->tre.tretyp&COMSCAN)?"select":"for";
					av[1] = t->for_.fornam;
					av[2] = "in";
					av[3] = cp;
					av[4] = 0;
					sh_debug(trap,NULL,NULL,av,0);
				}
				sh_exec(t->for_.fortre,flag);
				flag &= ~ARG_OPTIMIZE;
				if(t->tre.tretyp&COMSCAN)
				{
					if((cp=nv_getval(sh_scoped(REPLYNOD))) && *cp==0)
						refresh++;
				}
				else
					cp = *++args;
			check:
				/* decrease 'continue' level */
				if(sh.st.breakcnt<0)
					sh.st.breakcnt++;
			}
			if(nameref)
				nv_offattr(np,NV_NOOPTIMIZE);
#if SHOPT_OPTIMIZE
		endfor:
			sh_popcontext(buffp);
			sh_tclear(t->for_.fortre);
			sh_optclear(optlist);
			if(jmpval)
				siglongjmp(*sh.jmplist,jmpval);
#endif /* SHOPT_OPTIMIZE */
			/* decrease 'break' level */
			if(sh.st.breakcnt>0)
				sh.st.breakcnt--;
			sh.st.loopcnt--;
			sh_argfree(argsav,0);
			break;
		    }

		    /*
		     * Loop: 'while', 'until', or arithmetic 'for'
		     */
		    case TWH:
		    {
			volatile int 	r=0;
			int first = ARG_OPTIMIZE;
			Shnode_t *tt = t->wh.whtre;
			char always_true;
			Namval_t *np;
			Shbltin_f fp;
#if SHOPT_FILESCAN
			Sfio_t *iop=0;
			int savein=-1;
#endif /* SHOPT_FILESCAN */
#if SHOPT_OPTIMIZE
			int  jmpval = ((struct checkpt*)sh.jmplist)->mode;
			struct checkpt *buffp = stkalloc(sh.stk,sizeof(struct checkpt));
			void *optlist = sh.optlist;
			sh.optlist = 0;
			sh_tclear(t->wh.whtre);
			sh_tclear(t->wh.dotre);
			sh_pushcontext(buffp,jmpval);
			jmpval = sigsetjmp(buffp->buff,0);
			if(jmpval)
				goto endwhile;
#endif /* SHOPT_OPTIMIZE */
#if SHOPT_FILESCAN
			/* Recognize filescan loop for a lone input redirection following 'while' */
			if(type==TWH					/* 'while' (not 'until') */
			&& tt->tre.tretyp==TCOM 			/* one simple command follows 'while'... */
			&& !tt->com.comarg.dp				/* ...with no command name or arguments... */
			&& !tt->com.comset				/* ...and no variable assignments list... */
			&& tt->com.comio				/* ...and one I/O redirection... */
			&& !tt->com.comio->ionxt			/* ...but not more than one... */
			&& !(tt->com.comio->iofile & (IOPUT|IOAPP))	/* ...and not > or >> */
			&& !sh_isoption(SH_POSIX))			/* not in POSIX compliance mode */
			{
				iop = openstream(tt->com.comio,&savein);
			}
#endif /* SHOPT_FILESCAN */
			/* Optimization: don't call sh_exec() for simple 'while :', 'while true' or 'until false' */
			always_true = (tt->tre.tretyp==TCOM		/* one simple command (no COMSCAN = no expansions) */
				&& !sh.dont_optimize_builtins
				&& (np = (Namval_t*)tt->com.comnamp) && (fp = funptr(np))
				&& (type==TWH && fp==b_true || type==TUN && fp==b_false)
				&& !tt->com.comset			/* no variable assignments list */
				&& !tt->com.comio			/* no I/O redirections */
				&& !sh_isoption(SH_XTRACE)
				&& !sh.st.trap[SH_DEBUGTRAP]);
			sh.st.loopcnt++;
			while(sh.st.breakcnt==0)
			{
#if SHOPT_FILESCAN
				if(iop)
				{
					if(!(sh.cur_line=sfgetr(iop,'\n',SFIO_STRING)))
						break;
				}
				else
#endif /* SHOPT_FILESCAN */
				if(!always_true && (sh_exec(tt,first)==0)!=(type==TWH))
					break;
				r = sh_exec(t->wh.dotre,first|errorflg);
				/* decrease 'continue' level */
				if(sh.st.breakcnt<0)
					sh.st.breakcnt++;
				/* This is for the arithmetic for */
				if(sh.st.breakcnt==0 && t->wh.whinc)
					sh_exec((Shnode_t*)t->wh.whinc,first);
				first = 0;
				errorflg &= ~ARG_OPTIMIZE;
#if SHOPT_FILESCAN
				sh.offsets[0] = -1;
				sh.offsets[1] = 0;
#endif /* SHOPT_FILESCAN */
			}
#if SHOPT_OPTIMIZE
		endwhile:
			sh_popcontext(buffp);
			sh_tclear(t->wh.whtre);
			sh_tclear(t->wh.dotre);
			sh_optclear(optlist);
			if(jmpval)
				siglongjmp(*sh.jmplist,jmpval);
#endif /* SHOPT_OPTIMIZE */
			/* decrease 'break' level */
			if(sh.st.breakcnt>0)
				sh.st.breakcnt--;
			sh.st.loopcnt--;
			sh.exitval= r;
#if SHOPT_FILESCAN
			if(iop)
			{
				int err=errno;
				sfclose(iop);
				while(close(0)<0 && errno==EINTR)
					errno = err;
				dup(savein);
				sh.cur_line = 0;
			}
#endif /* SHOPT_FILESCAN */
			break;
		    }

		    /*
		     * Arithmetic command: ((expression))
		     */
		    case TARITH:
		    {
			char *trap;
			char *arg[4];
			error_info.line = t->ar.arline-sh.st.firstline;
			arg[0] = "((";
			if(!(t->ar.arexpr->argflag&ARG_RAW))
				arg[1] = sh_macpat(t->ar.arexpr,(flags & ARG_OPTIMIZE)|ARG_ARITH);
			else
				arg[1] = t->ar.arexpr->argval;
			arg[2] = "))";
			arg[3] = 0;
			if(trap=sh.st.trap[SH_DEBUGTRAP])
				sh_debug(trap,NULL,NULL,arg,ARG_ARITH);
			if(sh_isoption(SH_XTRACE))
			{
				sh_trace(NULL,0);
				sfprintf(sfstderr,"((%s))\n",arg[1]);
			}
			if(t->ar.arcomp)
				sh.exitval  = !arith_exec((Arith_t*)t->ar.arcomp);
			else
				sh.exitval = !sh_arith(arg[1]);
			break;
		    }

		    /*
		     * Conditional block: if ... fi
		     */
		    case TIF:
			if(sh_exec(t->if_.iftre, flags & ARG_OPTIMIZE)==0)
				sh_exec(t->if_.thtre,flags);
			else if(t->if_.eltre)
				sh_exec(t->if_.eltre, flags);
			else
				sh.exitval=0; /* force zero exit for if-then-fi */
			break;

		    /*
		     * Switch block: case ... esac
		     */
		    case TSW:
		    {
			const int eflag = flags & sh_state(SH_ERREXIT);
			char *r = sh_macpat(t->sw.swarg, flags & ARG_OPTIMIZE);
			error_info.line = t->sw.swline - sh.st.firstline;
			if(sh.st.trap[SH_DEBUGTRAP])
			{
				char *av[4];
				av[0] = "case";
				av[1] = r;
				av[2] = "in";
				av[3] = 0;
				sh_debug(sh.st.trap[SH_DEBUGTRAP], NULL, NULL, av, 0);
			}
			t = (Shnode_t*)t->sw.swlst;
			while(t)
			{
				struct argnod *rex = t->reg.regptr;
				while(rex)
				{
					const unsigned char raw = rex->argflag & ARG_RAW;
					char *s;
					if(rex->argflag&ARG_MAC)
						s = sh_macpat(rex,(flags & ARG_OPTIMIZE)|ARG_EXP);
					else
						s = rex->argval;
					if(raw && strcmp(r,s)==0 || !raw && strmatch(r,s))
					{
						do				/* execute; keep going while ;& */
							sh_exec(t->reg.regcom, t->reg.regflag ? eflag : flags);
						while(t->reg.regflag==1 && (t = (Shnode_t*)t->reg.regnxt));
						if(t && t->reg.regflag==0)	/* if not end or ;;& */
							t = 0;			/* break outer loop */
						break;
					}
					else
						rex=rex->argnxt.ap;
				}
				if(t)
					t=(Shnode_t*)t->reg.regnxt;
			}
			break;
		    }

		    /*
		     * The 'time' keyword: time a pipeline
		     */
		    case TTIME:
		    {
			const char *format = e_timeformat;
			struct timeval ta, tb;
			struct timeval before_usr, before_sys, after_usr, after_sys, tm[3];
			if(type!=TTIME)
			{
				sh_exec(t->par.partre, flags & ARG_OPTIMIZE);
				sh.exitval = !sh.exitval;
				break;
			}
			if(t->par.partre)
			{
				int timer_on = sh_isstate(SH_TIMING);
				/* must be run after forking a subshell */
				timeofday(&tb);
				get_cpu_times(&before_usr, &before_sys);
				sh_onstate(SH_TIMING);
				sh_exec(t->par.partre,sh_isstate(SH_ERREXIT)|(flags & ARG_OPTIMIZE));
				if(!timer_on)
					sh_offstate(SH_TIMING);
			}
			else
			{
				before_usr.tv_sec = before_usr.tv_usec = 0;
				before_sys.tv_sec = before_sys.tv_usec = 0;
			}
			get_cpu_times(&after_usr, &after_sys);
			timeofday(&ta);
			timersub(&ta, &tb, &tm[TM_REAL_IDX]); /* calculate elapsed real-time */
			timersub(&after_usr, &before_usr, &tm[TM_USR_IDX]);
			timersub(&after_sys, &before_sys, &tm[TM_SYS_IDX]);
			if(t->par.partre)
			{
				Namval_t *np;
				if(np = nv_open("TIMEFORMAT",sh.var_tree,NV_NOADD))
					format = nv_getval(np);
			}
			else
				format = strchr(format+1,'\n')+1;
			if(format && *format)
				p_time(sfstderr,sh_translate(format),tm);
			break;
		    }

		    /*
		     * Function definition
		     */
		    case TFUN:
		    {
			Namval_t *np=0;
			struct slnod *slp;
			char *fname = ((struct functnod*)t)->functnam;
			char *cp = strrchr(fname,'.');
			Namval_t *npv=0,*mp;
#if SHOPT_NAMESPACE
			/* Namespace definition: a modified function definition */
			if(t->tre.tretyp==TNSPACE)
			{
				Dt_t *root;
				Namval_t *oldnspace = sh.namespace;
				int offset = stktell(sh.stk);
				int	flags=NV_NOARRAY|NV_VARNAME;
				struct checkpt *chkp = stkalloc(sh.stk,sizeof(struct checkpt));
				int jmpval;
				if(cp)
				{
					errormsg(SH_DICT,ERROR_exit(1),e_ident,fname);
					UNREACHABLE();
				}
				if(sh.st.real_fun)
				{
					error(ERROR_exit(3),"namespaces cannot be defined in a ksh function scope");
					UNREACHABLE();
				}
				sfputc(sh.stk,'.');
				sfputr(sh.stk,fname,0);
				np = nv_open(stkptr(sh.stk,offset),sh.var_tree,flags);
				offset = stktell(sh.stk);
				if(nv_istable(np))
					root = nv_dict(np);
				else
				{
					root = dtopen(&_Nvdisc,Dtoset);
					nv_mount(np, NULL, root);
					np->nvalue = Empty;
					dtview(root,sh.var_base);
				}
				oldnspace = enter_namespace(np);
				/* make sure to restore oldnspace if a special builtin throws an error */
				sh_pushcontext(chkp,SH_JMPCMD);
				jmpval = sigsetjmp(chkp->buff,0);
				if(!jmpval)
					sh_exec(t->for_.fortre,flags|sh_state(SH_ERREXIT));
				sh_popcontext(chkp);
				enter_namespace(oldnspace);
				if(jmpval)	/* error occurred */
					siglongjmp(*sh.jmplist,jmpval);
				break;
			}
#endif /* SHOPT_NAMESPACE */
			/* look for discipline functions */
			error_info.line = t->funct.functline-sh.st.firstline;
			if(cp || sh.prefix)
			{
				int offset = stktell(sh.stk);
				if(sh.prefix)
				{
					cp = sh.prefix;
					sh.prefix = 0;
					npv = nv_open(cp,sh.var_tree,NV_NOARRAY|NV_VARNAME);
					sh.prefix = cp;
					cp = fname;
				}
				else
				{
					sfwrite(sh.stk,fname,cp++-fname);
					sfputc(sh.stk,0);
					npv = nv_open(stkptr(sh.stk,offset),sh.var_tree,NV_NOARRAY|NV_VARNAME);
				}
				offset = stktell(sh.stk);
				sfprintf(sh.stk,"%s.%s%c",nv_name(npv),cp,0);
				fname = stkptr(sh.stk,offset);
			}
			else if((mp=nv_search(fname,sh.bltin_tree,0)))
			{
				if(nv_isattr(mp,BLT_SPC))
				{	/* Function names cannot be special builtin */
					errormsg(SH_DICT,ERROR_exit(1),e_badfun,fname);
					UNREACHABLE();
				}
				if(funptr(mp)==b_true || funptr(mp)==b_false || funptr(mp)==b_break)
					sh.dont_optimize_builtins = 1;
			}
#if SHOPT_NAMESPACE
			if(sh.namespace && !sh.prefix && *fname!='.')
				np = sh_fsearch(fname,NV_ADD|NV_NOSCOPE);
			if(!np)
#endif /* SHOPT_NAMESPACE */
			np = nv_open(fname,sh_subfuntree(1),NV_NOARRAY|NV_VARNAME|NV_NOSCOPE);
			if(npv)
			{
				if(!sh.mktype)
				{	/*
					 * Set the discipline function. If this is done in a subshell, the variable
					 * must be scoped to the subshell before nvfun is set to the discipline.
					 */
					if(sh.subshell && !sh.subshare)
						sh_assignok(npv, 1);
					cp = nv_setdisc(npv,cp,np,(Namfun_t*)npv);
				}
				if(!cp)
				{
					errormsg(SH_DICT,ERROR_exit(1),e_baddisc,fname);
					UNREACHABLE();
				}
			}
			if(np->nvalue)
			{
				struct Ufunction *rp = np->nvalue;
				slp = np->nvmeta;
				sh_funstaks(slp->slchild,-1);
				if(slp->slptr)
				{
					Stk_t *sp = slp->slptr;
					slp->slptr = NULL;
					stkclose(sp);
				}
				if(rp->sdict)
				{
					Namval_t *mp, *nq;
					sh.last_root = rp->sdict;
					for(mp=(Namval_t*)dtfirst(rp->sdict);mp;mp=nq)
					{
						nv_unset(mp,NV_RDONLY);
						nq = dtnext(rp->sdict,mp);
						nv_delete(mp,rp->sdict,0);
					}
					dtclose(rp->sdict);
					rp->sdict = 0;
				}
				if(sh.funload)
				{
					if(!sh.fpathdict)
						free(np->nvalue);
					np->nvalue = NULL;
				}
			}
			if(!np->nvalue)
			{
				np->nvalue = new_of(struct Ufunction,sh.funload?sizeof(Dtlink_t):0);
				memset(np->nvalue,0,sizeof(struct Ufunction));
			}
			if(t->funct.functstak)
			{
				struct Ufunction *rp = np->nvalue;
				static Dtdisc_t		_Rpdisc =
				{
				        offsetof(struct Ufunction,fname), -1, sizeof(struct Ufunction)
				};
				struct functnod *fp;
				struct comnod *ac = t->funct.functargs;
				slp = t->funct.functstak;
				sh_funstaks(slp->slchild,1);
				if(slp->slptr)
					stklink(slp->slptr);
				np->nvmeta = slp;
				nv_funtree(np) = (int*)(t->funct.functtre);
				rp->lineno = t->funct.functline;
				rp->nspace = sh.namespace;
				rp->fname = 0;
				rp->argv = ac ? ac->comarg.dp->dolval + 1 : NULL;
				rp->argc = ac ? ac->comarg.dp->dolnum : 0;
				rp->fdict = sh.fun_tree;
				fp = (struct functnod*)(slp+1);
				if(fp->functtyp==(TFUN|FAMP))
					rp->fname = fp->functnam;
				nv_offattr(np,NV_FPOSIX);
				if(sh.funload)
				{
					rp->np = np;
					if(!sh.fpathdict)
						sh.fpathdict = dtopen(&_Rpdisc,Dtobag);
					if(sh.fpathdict)
						dtinsert(sh.fpathdict,rp);
				}
			}
			else
				nv_unset(np,0);
			if(type&FPOSIX)
				nv_onattr(np,NV_FUNCTION|NV_FPOSIX);
			else
				nv_onattr(np,NV_FUNCTION);
			if(type&FOPTGET)
				nv_onattr(np,NV_OPTGET);
			break;
		    }

		    /*
		     * The [[ keyword: new test compound command
		     */
		    case TTST:
		    {
			int n;
			char *left;
			int negate = (type&TNEGATE)!=0;
			if(type&TTEST)
				skipexitset++;
			error_info.line = t->tst.tstline-sh.st.firstline;
			echeck = 1;
			if((type&TPAREN)==TPAREN)
			{
				sh_exec(t->lst.lstlef, flags & ARG_OPTIMIZE);
				n = !sh.exitval;
			}
			else
			{
				int traceon=0;
				char *right = 0;
				char *trap;
				char *argv[6];
				n = type>>TSHIFT;
				left = sh_macpat(&(t->lst.lstlef->arg), flags & ARG_OPTIMIZE);
				if(type&TBINARY)
					right = sh_macpat(&(t->lst.lstrit->arg),((n==TEST_PEQ||n==TEST_PNE)?ARG_EXP:0)|(flags & ARG_OPTIMIZE));
				if(trap=sh.st.trap[SH_DEBUGTRAP])
					argv[0] = (type&TNEGATE)?((char*)e_tstbegin):"[[";
				if(sh_isoption(SH_XTRACE))
				{
					traceon = sh_trace(NULL,0);
					sfwrite(sfstderr,e_tstbegin,(type&TNEGATE?5:3));
				}
				if(type&TUNARY)
				{
					if(traceon)
						sfprintf(sfstderr,"-%c %s",n,sh_fmtq(left));
					if(trap)
					{
						char unop[3];
						unop[0] = '-';
						unop[1] = n;
						unop[2] = 0;
						argv[1] = unop;
						argv[2] = left;
						argv[3] = "]]";
						argv[4] = 0;
						sh_debug(trap,NULL,NULL,argv,0);
					}
					n = test_unop(n,left);
				}
				else if(type&TBINARY)
				{
					char *op = 0;
					int pattern = 0;
					if(trap || traceon)
						op = (char*)(shtab_testops+(n&037)-1)->sh_name;
					type >>= TSHIFT;
					if(type==TEST_PEQ || type==TEST_PNE)
						pattern=ARG_EXP;
					if(trap)
					{
						argv[1] = left;
						argv[2] = op;
						argv[3] = right;
						argv[4] = "]]";
						argv[5] = 0;
						sh_debug(trap,NULL,NULL,argv,pattern);
					}
					n = test_binop(n,left,right);
					if(traceon)
					{
						sfprintf(sfstderr,"%s %s ",sh_fmtq(left),op);
						if(pattern)
							out_pattern(sfstderr,right,-1);
						else
							sfputr(sfstderr,sh_fmtq(right),-1);
					}
				}
				if(traceon)
					sfwrite(sfstderr,e_tstend,4);
			}
			sh.exitval = ((!n)^negate);
			if(!skipexitset)
				exitset();
			break;
		    }
		}
		if(sh.trapnote || (sh.exitval && sh_isstate(SH_ERREXIT)) && t && echeck)
			sh_chktrap();
		/* set $_ */
		if(mainloop && com0)
		{
			/* store last argument here if it fits */
			static char	lastarg[32];
			if(sh_isstate(SH_FORKED))
				sh_done(0);
			if(sh.lastarg!= lastarg && sh.lastarg)
				free(sh.lastarg);
			if(strlen(comn) < sizeof(lastarg))
			{
				nv_onattr(L_ARGNOD,NV_NOFREE);
				sh.lastarg = strcpy(lastarg,comn);
			}
			else
			{
				nv_offattr(L_ARGNOD,NV_NOFREE);
				sh.lastarg = sh_strdup(comn);
			}
		}
		if(!skipexitset)
			exitset();
		if(!(flags & ARG_OPTIMIZE))
		{
			if(sav != stkptr(sh.stk,0))
				stkset(sh.stk,sav,0);
			else if(stktell(sh.stk))
				stkseek(sh.stk,0);
		}
		if(sh.trapnote&SH_SIGSET)
			sh_exit(SH_EXITSIG|sh.lastsig);
		if(was_interactive)
			sh_onstate(SH_INTERACTIVE);
		if(was_monitor && sh_isoption(SH_MONITOR))
			sh_onstate(SH_MONITOR);
		if(was_errexit)
			sh_onstate(SH_ERREXIT);
	}
	return sh.exitval;
}

/*
 * Public API function: run the command given by by the argument list argv,
 * containing argn elements. If argv[0] does not contain a /, check for a
 * built-in or function before performing a path search.
 */
int sh_run(int argn, char *argv[])
{
	struct dolnod	*dp;
	struct comnod	*t = stkalloc(sh.stk,sizeof(struct comnod));
	int		savtop = stktell(sh.stk);
	void		*savptr = stkfreeze(sh.stk,0);
	Opt_t		*op, *np = optctx(0, 0);
	Shbltin_t	bltindata;
	bltindata = sh.bltindata;
	op = optctx(np, 0);
	memset(t, 0, sizeof(struct comnod));
	dp = stkalloc(sh.stk, (unsigned)sizeof(struct dolnod) + ARG_SPARE*sizeof(char*) + argn*sizeof(char*));
	dp->dolnum = argn;
	dp->dolbot = ARG_SPARE;
	memcpy(dp->dolval+ARG_SPARE, argv, (argn+1)*sizeof(char*));
	t->comarg.dp = dp;
	if(!strchr(argv[0],'/'))
		t->comnamp = nv_bfsearch(argv[0],sh.fun_tree,(Namval_t**)&t->comnamq,NULL);
	argn=sh_exec((Shnode_t*)t,sh_isstate(SH_ERREXIT));
	optctx(op,np);
	sh.bltindata = bltindata;
	if(savptr!=stkptr(sh.stk,0))
		stkset(sh.stk,savptr,savtop);
	else
		stkseek(sh.stk,savtop);
	return argn;
}

/*
 * print out the command line if set -x is on
 */
int sh_trace(char *argv[], int nl)
{
	if(sh_isoption(SH_XTRACE))
	{
		char *cp;
		int bracket = 0;
		int decl = (nl&2);
		nl &= ~2;
		/* make this trace atomic */
		sfset(sfstderr,SFIO_SHARE|SFIO_PUBLIC,0);
		if(!(cp=nv_getval(sh_scoped(PS4NOD))))
			cp = "+ ";
		else
		{
			sh.intrace = 1;
			sh_offoption(SH_XTRACE);
			cp = sh_mactry(cp);
			sh_onoption(SH_XTRACE);
			sh.intrace = 0;
		}
		if(*cp)
			sfputr(sfstderr,cp,-1);
		if(argv)
		{
			char *argv0 = *argv;
			nl = (nl?'\n':-1);
			/* don't quote [ and [[ */
			if(*(cp=argv[0])=='[' && (!cp[1] || !cp[2]&&cp[1]=='['))
			{
				sfputr(sfstderr,cp,*++argv?' ':nl);
				bracket = 1;
			}
			while(cp = *argv++)
			{
				if(bracket==0 || *argv || *cp!=']')
					cp = sh_fmtq(cp);
				if(decl && sh.prefix && cp!=argv0 && *cp!='-')
				{
					if(*cp=='.' && cp[1]==0)
						cp = sh.prefix;
					else
						sfputr(sfstderr,sh.prefix,'.');
				}
				sfputr(sfstderr,cp,*argv?' ':nl);
			}
		}
		sfset(sfstderr,SFIO_SHARE|SFIO_PUBLIC,1);
		return 1;
	}
	return 0;
}

static void timed_out(void *handle)
{
	NOT_USED(handle);
	timeout = 0;
}

/*
 * called by parent and child after fork by sh_fork()
 */
pid_t _sh_fork(pid_t parent,int flags,int *jobid)
{
	static long forkcnt = 1000L;
	pid_t	curpgid = job.curpgid;
	pid_t	postid = (flags&FAMP)?0:curpgid;
	int	sig,nochild;
	if(parent<0)
	{
		sh_sigcheck();
		if((forkcnt *= 2) > 1000L*SH_FORKLIM)
		{
			forkcnt=1000L;
			errormsg(SH_DICT,ERROR_system(ERROR_NOEXEC),e_nofork);
			UNREACHABLE();
		}
		timeout = sh_timeradd(forkcnt, 0, timed_out, NULL);
		nochild = job_wait((pid_t)1);
		if(timeout)
		{
			if(nochild)
				pause();
			else if(forkcnt>1000L)
				forkcnt /= 2;
			sh_timerdel(timeout);
			timeout = 0;
		}
		return -1;
	}
	forkcnt = 1000L;
	if(parent)
	{
		int myjob,waitall=job.waitall;
		if(job.toclear)
			job_clear();
		job.waitall = waitall;
		/* first process defines process group */
		if(sh_isstate(SH_MONITOR))
		{
			/*
			 * errno == EPERM means that an earlier process has
			 * finished running. Make the parent the job group ID.
			 */
			if(postid==0)
				job.curpgid = parent;
			if(job.jobcontrol || (flags&FAMP))
			{
				if(setpgid(parent,job.curpgid)<0 && errno==EPERM)
					setpgid(parent,parent);
			}
		}
		if(!sh_isstate(SH_MONITOR) && job.waitall && postid==0)
			job.curpgid = parent;
		if(flags&FCOOP)
			sh.cpid = parent;
		if(!postid && job.curjobid && (flags&FPOU))
			postid = job.curpgid;
		myjob = job_post(parent, (!postid && (flags&(FAMP|FINT))==(FAMP|FINT)) ? 1 : postid);
		if(job.waitall && (flags&FPOU))
		{
			if(!job.curjobid)
				job.curjobid = myjob;
			if(job.exitval)
				job.exitval++;
		}
		if(flags&FAMP)
			job.curpgid = curpgid;
		if(jobid)
			*jobid = myjob;
		return parent;
	}
	/* This is the child process */
	sh.current_ppid = sh.current_pid;
	sh.current_pid = getpid();  /* ${.sh.pid} */
	sh.outpipepid = ((flags&FPOU)?sh.current_pid:0);
	if(sh.trapnote&SH_SIGTERM)
		sh_exit(SH_EXITSIG|SIGTERM);
	sh_timerdel(NULL);
	if(sh_isstate(SH_MONITOR))
	{
		if(postid==0)
			job.curpgid = sh.current_pid;
		while(setpgid(0,job.curpgid)<0 && job.curpgid!=sh.current_pid)
			job.curpgid = sh.current_pid;
		if(job.jobcontrol && job.curpgid==sh.current_pid && !(flags&FAMP))
			tcsetpgrp(job.fd,job.curpgid);
	}
	if(job.jobcontrol)
	{
		signal(SIGTTIN,SIG_DFL);
		signal(SIGTTOU,SIG_DFL);
		signal(SIGTSTP,SIG_DFL);
	}
	job.jobcontrol = 0;
	job.toclear = 1;
	sh_offoption(SH_LOGIN_SHELL);
	sh_onstate(SH_FORKED);
#if SHOPT_ACCT
	sh_accsusp();
#endif	/* SHOPT_ACCT */
	/* Reset remaining signals to parent */
	/* except for those `lost' by trap   */
	if(!(flags&FSHOWME))
		sh_sigreset(2);
	sh.realsubshell++;		/* increase ${.sh.subshell} */
	sh.subshell = 0;		/* zero virtual subshells */
	sh.comsub = 0;
	sh.spid = 0;
	if((flags&FAMP) && sh.coutpipe>1)
		sh_close(sh.coutpipe);
	sig = sh.savesig;
	sh.savesig = 0;
	if(sig>0)
		kill(sh.current_pid,sig);
	sh_sigcheck();
	return 0;
}

/*
 * This routine creates a subshell by calling fork(2).
 * If fork fails, the shell sleeps for exponentially longer periods
 *   and tries again until a limit is reached.
 * SH_FORKLIM is the max period between forks - power of 2 usually.
 * Currently, the shell tries after 2, 4, 8, 16 and 32 seconds, and then quits.
 * Failures cause the routine to error exit.
 * Parent links to here-documents are removed by the child.
 * Traps are reset by the child.
 * The process-id of the child is returned to the parent, 0 to the child.
 */
pid_t sh_fork(int flags, int *jobid)
{
	pid_t parent;
	int sig;
	if(!sh.pathlist)
		path_get(Empty);
	sfsync(NULL);
	sh.trapnote &= ~SH_SIGTERM;
	job_fork(-1);
	sh.savesig = -1;
	while(_sh_fork(parent=fork(),flags,jobid) < 0);
	sh_stats(STAT_FORKS);
	sig = sh.savesig;
	sh.savesig = 0;
	if(sig>0)
		kill(sh.current_pid,sig);
	job_fork(parent);
	return parent;
}

/*
 * add exports from previous scope to the new scope
 */
static void  local_exports(Namval_t *np, void *data)
{
	Namval_t	*mp;
	NOT_USED(data);
	if(!nv_isnull(np) && (mp = nv_search(nv_name(np), sh.var_tree, NV_ADD|NV_NOSCOPE)) && nv_isnull(mp))
		nv_clone(np,mp,0);
}

/*
 * This routine executes .sh.math functions from within ((...))
 */
Sfdouble_t sh_mathfun(void *fp, int nargs, Sfdouble_t *arg)
{
	Sfdouble_t	d;
	Namval_t	node,*mp,*np, *nref[9], **nr=nref;
	char		*argv[2];
	struct funenv	funenv;
	int		i;
	np = (Namval_t*)fp;
	funenv.node = np;
	funenv.nref = nref;
	funenv.env = NULL;
	memcpy(&node,SH_VALNOD,sizeof(node));
	SH_VALNOD->nvfun = NULL;
	SH_VALNOD->nvmeta = NULL;
	SH_VALNOD->nvflag = NV_LDOUBLE|NV_NOFREE;
	SH_VALNOD->nvalue = NULL;
	for(i=0; i < nargs; i++)
	{
		*nr++ = mp = nv_namptr(sh.mathnodes,i);
		mp->nvalue = arg++;
	}
	*nr = 0;
	SH_VALNOD->nvalue = &d;
	argv[0] = np->nvname;
	argv[1] = NULL;
	sh_funscope(1,argv,0,&funenv,0);
	while(mp= *nr++)
		mp->nvalue = NULL;
	SH_VALNOD->nvfun = node.nvfun;
	SH_VALNOD->nvflag = node.nvflag;
	SH_VALNOD->nvmeta = node.nvmeta;
	SH_VALNOD->nvalue = node.nvalue;
	return d;
}

/*
 * This routine is used to execute the given function <fun> in a new scope
 * If <fun> is NULL, then arg points to a structure containing a pointer
 * to a function that will be executed in the current environment.
 */
int sh_funscope(int argn, char *argv[],int(*fun)(void*),void *arg,int execflg)
{
	char			*trap;
	int			nsig;
	struct dolnod		*argsav=0,*saveargfor;
	struct sh_scoped	*savst = stkalloc(sh.stk,sizeof(struct sh_scoped));
	struct sh_scoped	*prevscope = sh.st.self;
	struct argnod		*envlist=0;
	int			isig,jmpval;
	volatile int		r = 0;
	int			n;
	char			save_invoc_local;
	char 			**savsig, *save_debugtrap = 0;
	struct funenv		*fp = 0;
	struct checkpt		*buffp = stkalloc(sh.stk,sizeof(struct checkpt));
	Namval_t		*nspace = sh.namespace;
	Dt_t			*last_root = sh.last_root;
	Shopt_t			options;
	options = sh.options;
	NOT_USED(argn);
	if(sh.fn_depth==0)
		sh.glob_options =  sh.options;
	else
		sh.options = sh.glob_options;
	*prevscope = sh.st;
	sh_offoption(SH_ERREXIT);
	sh.st.prevst = prevscope;
	sh.st.self = savst;
	sh.topscope = (Shscope_t*)sh.st.self;
	sh.st.opterror = sh.st.optchar = 0;
	sh.st.optindex = 1;
	sh.st.loopcnt = 0;
	if(!fun)
	{
		fp = (struct funenv*)arg;
		sh.st.real_fun = fp->node->nvalue;
		envlist = fp->env;
	}
	prevscope->save_tree = sh.var_tree;
	n = dtvnext(prevscope->save_tree)!= (sh.namespace?sh.var_base:0);
	sh_scope(envlist,1);
	if(n)
	{
		/* eliminate parent scope */
		nv_scan(prevscope->save_tree, local_exports, NULL, NV_EXPORT, NV_EXPORT|NV_NOSCOPE);
	}
	sh.st.save_tree = sh.var_tree;
	if(!fun)
	{
		if(nv_isattr(fp->node,NV_TAGGED))
			sh_onoption(SH_XTRACE);
		else if(!sh_isoption(SH_FUNCTRACE))
			sh_offoption(SH_XTRACE);
	}
	sh.st.cmdname = argv[0];
	/* save trap table */
	if((nsig=sh.st.trapmax)>0 || sh.st.trapcom[0])
	{
		savsig = sh_malloc(nsig * sizeof(char*));
		/*
		 * the data is, usually, modified in code like:
		 *	tmp = buf[i]; buf[i] = sh_strdup(tmp); free(tmp);
		 * so sh.st.trapcom needs a "deep copy" to properly save/restore pointers.
		 */
		for (isig = 0; isig < nsig; ++isig)
		{
			if(sh.st.trapcom[isig] == Empty)
				savsig[isig] = Empty;
			else if(sh.st.trapcom[isig])
				savsig[isig] = sh_strdup(sh.st.trapcom[isig]);
			else
				savsig[isig] = NULL;
		}
	}
	if(!fun && sh_isoption(SH_FUNCTRACE) && sh.st.trap[SH_DEBUGTRAP] && *sh.st.trap[SH_DEBUGTRAP])
		save_debugtrap = sh_strdup(sh.st.trap[SH_DEBUGTRAP]);
	sh_sigreset(-1);
	if(save_debugtrap)
		sh.st.trap[SH_DEBUGTRAP] = save_debugtrap;
	argsav = sh_argnew(argv,&saveargfor);
	sh_pushcontext(buffp,SH_JMPFUN);
	errorpush(&buffp->err,0);
	error_info.id = argv[0];
	if(!fun)
	{
		struct Ufunction *rp = fp->node->nvalue;
		if(rp)
			sh.st.filename = rp->fname;
		sh.st.funname = nv_name(fp->node);
		sh.last_root = nv_dict(DOTSHNOD);
		nv_putval(SH_PATHNAMENOD,sh.st.filename,NV_NOFREE);
		nv_putval(SH_FUNNAMENOD,sh.st.funname,NV_NOFREE);
	}
	save_invoc_local = sh.invoc_local;
	sh.invoc_local = 0;
	jmpval = sigsetjmp(buffp->buff,0);
	if(jmpval == 0)
	{
		if(sh.fn_depth >= MAXDEPTH)
		{
			sh.toomany = 1;
			siglongjmp(*sh.jmplist,SH_JMPERRFN);
		}
		sh.fn_depth++;
		update_sh_level();
		if(fun)
			r= (*fun)(arg);
		else
		{
			char		**arg = sh.st.real_fun->argv;
			Namval_t	*np, *nq, **nref;
			if(nref=fp->nref)
			{
				sh.last_root = 0;
				for(r=0; arg[r]; r++)
				{
					np = nv_search(arg[r],sh.var_tree,NV_NOSCOPE|NV_ADD);
					if(np && (nq=*nref++))
					{
						np->nvalue = sh_newof(0,struct Namref,1,0);
						((struct Namref*)np->nvalue)->np = nq;
						nv_onattr(np,NV_REF|NV_NOFREE);
					}
				}
			}
			sh_exec((Shnode_t*)(nv_funtree((fp->node))),execflg|SH_ERREXIT);
			r = sh.exitval;
		}
	}
	sh.invoc_local = save_invoc_local;
	sh.fn_depth--;
	update_sh_level();
	if(sh.fn_depth==1 && jmpval==SH_JMPERRFN)
	{
		errormsg(SH_DICT,ERROR_exit(1),e_toodeep,argv[0]);
		UNREACHABLE();
	}
	sh_popcontext(buffp);
	sh_unscope();
	sh.namespace = nspace;
	sh.var_tree = (Dt_t*)prevscope->save_tree;
	sh_argreset(argsav,saveargfor);
	trap = sh.st.trapcom[0];
	sh.st.trapcom[0] = 0;
	sh_sigreset(1);
	sh.st = *prevscope;
	sh.topscope = (Shscope_t*)prevscope;
	nv_getval(sh_scoped(IFSNOD));
	if(nsig)
	{
		for (isig = 0; isig < nsig; ++isig)
			if (sh.st.trapcom[isig] && sh.st.trapcom[isig]!=Empty)
				free(sh.st.trapcom[isig]);
		memcpy((char*)&sh.st.trapcom[0],savsig,nsig*sizeof(char*));
		free(savsig);
	}
	sh.trapnote=0;
	sh.options = options;
	sh.last_root = last_root;
	if(jmpval == SH_JMPSUB)
		siglongjmp(*sh.jmplist,jmpval);
	if(trap)
	{
		sh_trap(trap,0);
		free(trap);
	}
	if(jmpval)
		r=sh.exitval;
	if(jmpval==SH_JMPFUN && sh.lastsig)
		kill(sh.current_pid, sh.lastsig);  /* pass down unhandled signal that interrupted ksh function */
	if(jmpval > SH_JMPFUN)
	{
		sh_chktrap();
		siglongjmp(*sh.jmplist,jmpval);
	}
	return r;
}

static void sh_funct(Namval_t *np,int argn, char *argv[],struct argnod *envlist,int execflg)
{
	struct Ufunction *rp = np->nvalue;
	struct funenv	fun;
	char		*fname = nv_getval(SH_FUNNAMENOD);
	pid_t		pipepid = sh.pipepid;
#if !SHOPT_DEVFD
	Dt_t		*save_fifo_tree = sh.fifo_tree;
	sh.fifo_tree = NULL;
#endif
	sh.pipepid = 0;
	sh_stats(STAT_FUNCT);
	if((struct sh_scoped*)sh.topscope != sh.st.self)
		sh_setscope(sh.topscope);
	sh.st.lineno = error_info.line;
	rp->running += 2;
	if(nv_isattr(np,NV_FPOSIX))
	{
		char *save;
		int loopcnt = sh.st.loopcnt;
		sh.posix_fun = np;
		save = argv[-1];
		argv[-1] = 0;
		sh.st.funname = nv_name(np);
		sh.last_root = nv_dict(DOTSHNOD);
		nv_putval(SH_FUNNAMENOD, nv_name(np),NV_NOFREE);
		opt_info.index = opt_info.offset = 0;
		error_info.errors = 0;
		sh.st.loopcnt = 0;
		b_dot_cmd(argn+1,argv-1,&sh.bltindata);
		sh.st.loopcnt = loopcnt;
		argv[-1] = save;
	}
	else
	{
		fun.env = envlist;
		fun.node = np;
		fun.nref = 0;
		sh_funscope(argn,argv,0,&fun,execflg);
	}
	sh.last_root = nv_dict(DOTSHNOD);
	nv_putval(SH_FUNNAMENOD,fname,NV_NOFREE);
	nv_putval(SH_PATHNAMENOD,sh.st.filename,NV_NOFREE);
	sh.pipepid = pipepid;
	if(rp = np->nvalue)
	{
		rp->running -= 2;
		if(rp->running==1)
		{
			rp->running = 0;
			nv_unset(np, NV_RDONLY);
		}
	}
#if !SHOPT_DEVFD
	fifo_cleanup();
	sh.fifo_tree = save_fifo_tree;
#endif
}

/*
 * external interface to execute a function
 * <np> is the function node
 * If <nq> is not-null, then sh.name and sh.subscript will be set
 */
int sh_fun(Namval_t *np, Namval_t *nq, char *argv[])
{
	struct checkpt	*checkpoint;
	int		jmpval = 0;
	int		jmpthresh;
	int		offset = 0;
	char		*base;
	Namval_t	node;
	struct Namref	nr;
	long		mode = 0;
	char		*prefix = sh.prefix;
	int		n=0;
	char		*av[3];
	Fcin_t		save;
	fcsave(&save);
	if((offset=stktell(sh.stk))>0)
		base=stkfreeze(sh.stk,0);
	sh.prefix = 0;
	if(!argv)
	{
		argv = av+1;
		argv[1]=0;
	}
	argv[0] = nv_name(np);
	while(argv[n])
		n++;
	if(nq)
		mode = set_instance(nq,&node, &nr);
	jmpthresh = is_abuiltin(np) ? SH_JMPCMD : SH_JMPFUN;
	checkpoint = stkalloc(sh.stk,sizeof(struct checkpt));
	sh_pushcontext(checkpoint, jmpthresh);
	jmpval = sigsetjmp(checkpoint->buff,1);
	if(jmpval == 0)
	{
		if(is_abuiltin(np))
		{
			Shbltin_t *bp = &sh.bltindata;
			bp->bnode = np;
			bp->ptr = nv_context(np);
			errorpush(&checkpoint->err,0);
			error_info.id = argv[0];
			opt_info.index = opt_info.offset = 0;
			opt_info.disc = 0;
			sh.exitval = 0;
			sh.exitval = (funptr(np))(n,argv,bp);
		}
		else
			sh_funct(np,n,argv,NULL,sh_isstate(SH_ERREXIT));
	}
	sh_popcontext(checkpoint);
	if(nq)
		unset_instance(&node, &nr, mode);
	fcrestore(&save);
	if(offset>0)
		stkset(sh.stk,base,offset);
	sh.prefix = prefix;
	if(jmpval >= jmpthresh)
		siglongjmp(*sh.jmplist,jmpval);
	return sh.exitval;
}

/*
 * set up pipe for cooperating process
 */
static void coproc_init(int pipes[])
{
	int outfd;
	if(sh.coutpipe>=0 && sh.cpid)
	{
		errormsg(SH_DICT,ERROR_exit(1),e_copexists);
		UNREACHABLE();
	}
	sh.cpid = 0;
	if(sh.cpipe[0]<=0 || sh.cpipe[1]<=0)
	{
		/* first co-process */
		sh_pclose(sh.cpipe);
		sh_pipe(sh.cpipe);
		if((outfd=sh.cpipe[1]) < 10)
		{
		        int fd=sh_fcntl(sh.cpipe[1],F_DUPFD,10);
			if(fd>=10)
			{
			        sh.fdstatus[fd] = (sh.fdstatus[outfd]&~IOCLEX);
				close(outfd);
			        sh.fdstatus[outfd] = IOCLOSE;
				sh.cpipe[1] = fd;
			}
		}
		if(fcntl(*sh.cpipe,F_SETFD,FD_CLOEXEC)>=0)
			sh.fdstatus[sh.cpipe[0]] |= IOCLEX;
		sh.fdptrs[sh.cpipe[0]] = sh.cpipe;
		if(fcntl(sh.cpipe[1],F_SETFD,FD_CLOEXEC) >=0)
			sh.fdstatus[sh.cpipe[1]] |= IOCLEX;
	}
	sh.outpipe = sh.cpipe;
	sh_pipe(sh.inpipe=pipes);
	sh.coutpipe = sh.inpipe[1];
	sh.fdptrs[sh.coutpipe] = &sh.coutpipe;
	if(fcntl(sh.outpipe[0],F_SETFD,FD_CLOEXEC)>=0)
		sh.fdstatus[sh.outpipe[0]] |= IOCLEX;
}

#if SHOPT_SPAWN

static void sigreset(int mode)
{
	char   *trap;
	int	sig;
	for (sig = 1; sig < sh.st.trapmax; sig++)
	{
		if(sig==SIGCHLD)
			continue;
		if((trap=sh.st.trapcom[sig]) && *trap==0)
			signal(sig,mode?sh_fault:SIG_IGN);
	}
}

/*
 * A combined fork/exec for systems with slow fork().
 * Incompatible with job control on interactive shells (job.jobcontrol) if
 * the system does not support posix_spawn_file_actions_addtcsetpgrp_np().
 */
static pid_t sh_ntfork(const Shnode_t *t,char *argv[],int *jobid,int topfd)
{
	static pid_t	spawnpid;
	struct checkpt	*buffp = stkalloc(sh.stk,sizeof(struct checkpt));
	int		jmpval,jobfork=0;
	volatile int	scope=0, sigwasset=0;
	char		**arge, *path;
	volatile pid_t	grp = 0;
	Pathcomp_t	*pp;
#if _use_ntfork_tcpgrp
	volatile int	jobwasset=0;
#endif /* _use_ntfork_tcpgrp */
	sh_pushcontext(buffp,SH_JMPCMD);
	errorpush(&buffp->err,ERROR_SILENT);
	job_lock();		/* errormsg will unlock */
	jmpval = sigsetjmp(buffp->buff,0);
	if(jmpval == 0)
	{
		spawnpid = -1;
		if(t->com.comio)
			sh_redirect(t->com.comio,0);
		error_info.id = *argv;
		if(t->com.comset)
		{
			scope++;
			sh.invoc_local++;
			sh_scope(t->com.comset,0);
		}
		if(!strchr(path=argv[0],'/'))
		{
			Namval_t *np;
			if(np = path_gettrackedalias(path))
				path = nv_getval(np);
			else if(path_absolute(path,NULL,0))
			{
				path = stkptr(sh.stk,PATH_OFFSET);
				stkfreeze(sh.stk,0);
			}
			else
			{
				pp=path_get(path);
				while(pp)
				{
					if(pp->len==1 && *pp->name=='.')
						break;
					pp = pp->next;
				}
				if(!pp)
					path = 0;
			}
		}
		else if(sh_isoption(SH_RESTRICTED))
		{
			errormsg(SH_DICT,ERROR_exit(1),e_restricted,path);
			UNREACHABLE();
		}
		if(!path)
		{
			spawnpid = -1;
			goto fail;
		}
		arge = sh_envgen();
		sh.exitval = 0;
#if _use_ntfork_tcpgrp
		if(job.jobcontrol)
		{
			signal(SIGTTIN,SIG_DFL);
			signal(SIGTTOU,SIG_DFL);
			signal(SIGTSTP,SIG_DFL);
			jobwasset++;
		}
		if(sh_isstate(SH_MONITOR) && job.jobcontrol)
		{
			if(job.curpgid==0)
				grp = 1;
			else
				grp = job.curpgid;
		}
#endif /* _use_ntfork_tcpgrp */

		sfsync(NULL);
		sigreset(0);	/* set signals to ignore */
		sigwasset++;
	        /* find first path that has a library component */
		for(pp=path_get(argv[0]); pp && !pp->lib ; pp=pp->next);
		job_fork(-1);
		jobfork = 1;
		spawnpid = path_spawn(path,argv,arge,pp,(grp<<1)|1);
		if(spawnpid < 0 && errno==ENOEXEC)
		{
			char *devfd;
			int fd = open(path,O_RDONLY);
			argv[-1] = argv[0];
			argv[0] = path;
			if(fd>=0)
			{
				struct stat statb;
				sfprintf(sh.strbuf,"/dev/fd/%d",fd);
				if(stat(devfd=sfstruse(sh.strbuf),&statb)>=0)
					argv[0] =  devfd;
			}
			if(!sh.shpath)
				sh.shpath = pathshell();
			spawnpid = path_spawn(sh.shpath,&argv[-1],arge,pp,(grp<<1)|1);
			if(fd>=0)
				close(fd);
			argv[0] = argv[-1];
		}
	fail:
		if(jobfork && spawnpid<0)
			job_fork(-2);
		if(spawnpid == -1)
		{
#if _use_ntfork_tcpgrp
			if(jobwasset)
			{
				signal(SIGTTIN,SIG_IGN);
				signal(SIGTTOU,SIG_IGN);
				if(sh_isstate(SH_INTERACTIVE))
					signal(SIGTSTP,SIG_IGN);
				else
					signal(SIGTSTP,SIG_DFL);
			}
			if(job.jobcontrol)
				tcsetpgrp(job.fd,sh.pid);
#endif /* _use_ntfork_tcpgrp */
			switch(errno=sh.path_err)
			{
			    case ENOENT:
				errormsg(SH_DICT,ERROR_exit(ERROR_NOENT),e_found+4);
				UNREACHABLE();
#ifdef ENAMETOOLONG
			    case ENAMETOOLONG:
				errormsg(SH_DICT,ERROR_exit(ERROR_NOENT),e_toolong+4);
				UNREACHABLE();
#endif
			    default:
				errormsg(SH_DICT,ERROR_system(ERROR_NOEXEC),e_exec+4);
				UNREACHABLE();
			}
		}
		job_unlock();
	}
	else
		exitset();
	sh_popcontext(buffp);
	if(buffp->olist)
		free_list(buffp->olist);
#if _use_ntfork_tcpgrp
	if(jobwasset)
	{
		signal(SIGTTIN,SIG_IGN);
		signal(SIGTTOU,SIG_IGN);
		if(sh_isstate(SH_INTERACTIVE))
			signal(SIGTSTP,SIG_IGN);
		else
			signal(SIGTSTP,SIG_DFL);
	}
#endif /* _use_ntfork_tcpgrp */
	if(sigwasset)
		sigreset(1);	/* restore ignored signals */
	if(scope)
	{
		sh_unscope();
		sh.invoc_local--;
		if(jmpval==SH_JMPSCRIPT)
			nv_setlist(t->com.comset,NV_EXPORT|NV_IDENT|NV_ASSIGN,0);
	}
	if(t->com.comio && (jmpval || spawnpid<=0) && sh.topfd > topfd)
		sh_iorestore(topfd,jmpval);
	if(jmpval>SH_JMPCMD)
		siglongjmp(*sh.jmplist,jmpval);
	if(spawnpid>0)
	{
		_sh_fork(spawnpid,0,jobid);
		job_fork(spawnpid);
		if(grp==1)
			job.curpgid = spawnpid;
	}
	return spawnpid;
}

#endif /* SHOPT_SPAWN */

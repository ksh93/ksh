/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2026 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  David Korn <dgk@research.att.com>                   *
*                   Phong Vo <kpv@research.att.com>                    *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
/*
 *   Routines to implement a stack-like storage library
 *
 *   A stack consists of a link list of variable size frames
 *   The beginning of each frame is initialized with a frame structure
 *   that contains a pointer to the previous frame and a pointer to the
 *   end of the current frame.
 *
 *   This is a rewrite of the stk library that uses sfio
 *
 *   David Korn
 *   AT&T Research
 *   dgk@research.att.com
 *
 */

#include	<sfio_t.h>
#include	<ast.h>
#include	<align.h>
#include	<stk.h>

/*
 *  A stack is a header and a linked list of frames
 *  The first frame has structure
 *	Sfio_t
 *	Sfdisc_t
 *	struct stk
 * Frames have structure
 *	struct frame
 *	data
 */

#define STK_ALIGN	((size_t)ALIGN_BOUND)
#define STK_FSIZE	(1024*sizeof(char*))
#define STK_HDRSIZE	(sizeof(Sfio_t)+sizeof(Sfdisc_t))

typedef void (*_stk_overflow_)(size_t);
typedef char* (*_old_stk_overflow_)(size_t);	/* for stkinstall (deprecated) */

static int stkexcept(Sfio_t*,int,void*,Sfdisc_t*);
static Sfdisc_t stkdisc = { 0, 0, 0, stkexcept };

Sfio_t	_Stak_data = SFNEW(NULL,0,-1,SFIO_STATIC|SFIO_WRITE|SFIO_STRING,&stkdisc);

struct frame
{
	char	*prev;		/* address of previous frame */
	char	*end;		/* address of end this frame */
	char	**aliases;	/* address aliases */
	ssize_t	nalias;		/* number of aliases */
};

struct stk
{
	_stk_overflow_	stkoverflow;	/* called when malloc fails */
	unsigned int	stkref;		/* reference count */
	int		stkflags;	/* stack attributes */
	char		*stkbase;	/* beginning of current stack frame */
	char		*stkend;	/* end of current stack frame */
};

static size_t		init;		/* 1 when initialized */
static struct stk	*stkcur;	/* pointer to current stk */
static char		*stkgrow(Sfio_t*, size_t);

#define stream2stk(stream)	((stream)==stkstd? stkcur:\
				 ((struct stk*)(((char*)(stream))+STK_HDRSIZE)))
#define stk2stream(sp)		((Sfio_t*)(((char*)(sp))-STK_HDRSIZE))
#define stkleft(stream)		((stream)->_endb-(stream)->_data)

static const char Omsg[] = "out of memory while growing stack\n";

/*
 * default overflow exception
 */
static noreturn void overflow(size_t n)
{
	NoP(n);
	write(2,Omsg, sizeof(Omsg)-1);
	exit(128);
	UNREACHABLE();
}

/*
 * initialize stkstd, sfio operations may have already occurred
 */
static void stkinit(size_t size)
{
	Sfio_t *sp;
	init = size;
	sp = stkopen(0);
	init = 1;
	stkinstall(sp,(_old_stk_overflow_)overflow);
}

static int stkexcept(Sfio_t *stream, int type, void* val, Sfdisc_t* dp)
{
	NoP(dp);
	NoP(val);
	switch(type)
	{
	    case SFIO_CLOSING:
		{
			struct stk *sp = stream2stk(stream);
			if(--sp->stkref == 0)
			{
				if(stream==stkstd)
					stkset(stream,NULL,0);
				else
				{
					void *bp = sp->stkbase, *fp;
					while(fp = bp)
					{
						bp = ((struct frame*)bp)->prev;
						free(fp);
					}
				}
			}
			stream->_data = stream->_next = 0;
		}
		return 0;
	    case SFIO_FINAL:
		free(stream);
		return 1;
	    case SFIO_DPOP:
		return -1;
	    case SFIO_WRITE:
	    case SFIO_SEEK:
		{
			size_t size = (size_t)sfvalue(stream);
			if(init)
			{
				if(!stkgrow(stream, size - (size_t)(stream->_endb - stream->_data)))
					return -1;
			}
			else
				stkinit(size);
		}
		return 1;
	    case SFIO_NEW:
		return -1;
	}
	return 0;
}

/*
 * create a stack
 */
Sfio_t *stkopen(int flags)
{
	size_t bsize;
	Sfio_t *stream;
	struct stk *sp;
	struct frame *fp;
	Sfdisc_t *dp;
	char *cp;
	if(!(stream = calloc(1, sizeof(Sfio_t) + sizeof(Sfdisc_t) + sizeof(struct stk))))
		return NULL;
	dp = (Sfdisc_t*)(stream+1);
	dp->exceptf = stkexcept;
	sp = (struct stk*)(dp+1);
	sp->stkref = 1;
	sp->stkflags = flags;
	sp->stkoverflow = flags&STK_NULL ? 0 : stkcur ? stkcur->stkoverflow : overflow;
	bsize = init+sizeof(struct frame);
	if(flags&STK_SMALL)
		bsize = roundof(bsize,STK_FSIZE/16);
	else
		bsize = roundof(bsize,STK_FSIZE);
	bsize -= sizeof(struct frame);
	if(!(fp = malloc(sizeof(struct frame) + bsize)))
	{
		free(stream);
		return NULL;
	}
	cp = (char*)(fp+1);
	sp->stkbase = (char*)fp;
	fp->prev = 0;
	fp->nalias = 0;
	fp->aliases = 0;
	fp->end = sp->stkend = cp+bsize;
	if(!sfnew(stream,cp,bsize,-1,SFIO_STRING|SFIO_WRITE|SFIO_STATIC|SFIO_EOF))
		return NULL;
	sfdisc(stream,dp);
	return stream;
}

/*
 * return a pointer to the current stack
 * if <stream> is not null, it becomes the new current stack
 * <oflow> becomes the new overflow function
 */
Sfio_t *stkinstall(Sfio_t *stream, _old_stk_overflow_ oflow)
{
	Sfio_t *old;
	struct stk *sp;
	if(!init)
	{
		stkinit(1);
		if(oflow)
			stkcur->stkoverflow = (_stk_overflow_)oflow;
		return NULL;
	}
	old = stkcur?stk2stream(stkcur):0;
	if(stream)
	{
		sp = stream2stk(stream);
		while(sfstack(stkstd, SFIO_POPSTACK));
		if(stream!=stkstd)
			sfstack(stkstd,stream);
		stkcur = sp;
	}
	else
		sp = stkcur;
	if(oflow)
		sp->stkoverflow = (_stk_overflow_)oflow;
	return old;
}

/*
 * set or unset the overflow function
 */
void stkoverflow(Sfio_t *stream, _stk_overflow_ oflow)
{
	struct stk *sp;
	if(!init)
		stkinit(1);
	sp = stream2stk(stream);
	sp->stkoverflow = oflow ? oflow : (sp->stkflags & STK_NULL ? NULL : overflow);
}

/*
 * increase the reference count on the given <stack>
 */
unsigned int stklink(Sfio_t* stream)
{
	struct stk *sp = stream2stk(stream);
	return sp->stkref++;
}

/*
 * terminate a stack and free up the space
 * >0 returned if reference decremented but still > 0
 *  0 returned on last close
 * <0 returned on error
 */
int stkclose(Sfio_t* stream)
{
	struct stk *sp = stream2stk(stream);
	if(sp->stkref>1)
	{
		sp->stkref--;
		return 1;
	}
	return sfclose(stream);
}

/*
 * reset the bottom of the current stack back to <address>
 * if <address> is null, then the stack is reset to the beginning
 * if <address> is not in this stack, the program dumps core
 * otherwise, the top of the stack is set to stkbot+<offset>
 */
void *stkset(Sfio_t *stream, void *address, ptrdiff_t offset)
{
	struct stk *sp = stream2stk(stream);
	char *cp, *loc = (char*)address;
	struct frame *fp;
	unsigned int frames = 0;
	ssize_t n;
	if(!init)
		stkinit((size_t)(offset+1));
	while(1)
	{
		fp = (struct frame*)sp->stkbase;
		cp = sp->stkbase + roundof(sizeof(struct frame), STK_ALIGN);
		n = fp->nalias;
		while(n-->0)
		{
			if(loc==fp->aliases[n])
			{
				loc = cp;
				break;
			}
		}
		/* see whether <loc> is in current stack frame */
		if(loc>=cp && loc<=sp->stkend)
		{
			if(frames)
				sfsetbuf(stream,cp,(size_t)(sp->stkend-cp));
			stream->_data = (unsigned char*)(cp + roundof((size_t)(loc-cp),STK_ALIGN));
			stream->_next = (unsigned char*)loc+offset;
			goto found;
		}
		if(fp->prev)
		{
			sp->stkbase = fp->prev;
			sp->stkend = ((struct frame*)(fp->prev))->end;
			free(fp);
		}
		else
			break;
		frames++;
	}
	/* not found: produce a useful stack trace now instead of a useless one later */
	if(loc)
		abort();
	/* set stack back to the beginning */
	cp = (char*)(fp+1);
	if(frames)
		sfsetbuf(stream,cp,(size_t)(sp->stkend-cp));
	else
		stream->_data = stream->_next = (unsigned char*)cp;
found:
	return stream->_data;
}

/*
 * allocate <n> bytes on the current stack
 *
 * Since 2026-07-29, initialize them to zero.
 */
void *stkalloc(Sfio_t *stream, size_t n)
{
	unsigned char *old;
	if(!init)
		stkinit(n);
	n = roundof(n,STK_ALIGN);
	if(stkleft(stream) <= (ssize_t)n && !stkgrow(stream,n))
		return NULL;
	old = stream->_data;
	stream->_data = stream->_next = old+n;
	return memset(old, 0, n);
}

/*
 * begin a new stack object of at least <n> bytes
 */
void *_stkseek(Sfio_t *stream, ptrdiff_t n)
{
	if(!init)
		stkinit((size_t)n);
	if(stkleft(stream) <= n && !stkgrow(stream,(size_t)n))
		return NULL;
	stream->_next = stream->_data+n;
	return stream->_data;
}

/*
 * advance the stack to the current top
 * if extra is non-zero, first add extra bytes and zero the first
 */
void	*stkfreeze(Sfio_t *stream, size_t extra)
{
	unsigned char *old, *top;
	if(!init)
		stkinit(extra);
	old = stream->_data;
	top = stream->_next;
	if(extra)
	{
		if((ssize_t)extra > (stream->_endb-stream->_next))
		{
			if (!(top = (unsigned char*)stkgrow(stream,extra)))
				return NULL;
			old = stream->_data;
		}
		*top = 0;
		top += extra;
	}
	stream->_next = stream->_data += roundof((size_t)(top-old),STK_ALIGN);
	return (char*)old;
}

/*
 * copy string <str> onto the stack as a new object
 */
char	*stkcopy(Sfio_t *stream, const char* str)
{
	char *cp, *tp;
	size_t n;
	void *base = stkptr(stream,0);
	size_t off = (size_t)stktell(stream);
	/* save unterminated object */
	if(off)
	{
		if (!(tp = malloc(off)))
			return NULL;
		memcpy(tp, stream->_data, off);
	}
	/* allocate space for string on stack */
	n = strlen(str) + 1;
	if (!(cp = stkalloc(stream, n)))
		goto error_out;
	/* restore unterminated object */
	if (off)
	{
		if (!_stkseek(stream,(ptrdiff_t)off))
			goto error_out;
		memcpy(stream->_data, tp, off);
		free(tp);
	}
	/* copy string to stack */
	return memcpy(cp, str, n);

error_out:
	if(off)
		free(tp);
	stkset(stream, base, (ptrdiff_t)off);
	return NULL;
}

/*
 * add a new stack frame of size >= <n> to the current stack.
 * if <n> > 0, copy the bytes from stkbot to stktop to the new stack
 * if <n> is zero, then copy the remainder of the stack frame from stkbot
 * to the end is copied into the new stack frame
 */

static char *stkgrow(Sfio_t *stream, size_t size)
{
	size_t n = size;
	struct stk *sp = stream2stk(stream);
	struct frame *fp= (struct frame*)sp->stkbase;
	char *cp, *dp=0;
	size_t m = (size_t)stktell(stream);
	ptrdiff_t endoff;
	char *end=0, *oldbase=0;
	ssize_t nn=0,add=1;
	n += (m + sizeof(struct frame)+1);
	if(sp->stkflags&STK_SMALL)
		n = roundof(n,STK_FSIZE/16);
	else
		n = roundof(n,STK_FSIZE);
	/* see whether current frame can be extended */
	if(stkptr(stream,0)==sp->stkbase+sizeof(struct frame))
	{
		nn = fp->nalias+1;
		dp=sp->stkbase;
		sp->stkbase = ((struct frame*)dp)->prev;
		end = fp->end;
		oldbase = dp;
		endoff = end - dp;
	}
	cp = realloc(dp, n + (size_t)nn * sizeof(char*));
	if(!cp)
		return NULL;
	if(dp==cp)
	{
		nn--;
		add = 0;
	}
	else if(dp)
	{
		dp = cp;
		end = dp + endoff;
	}
	fp = (struct frame*)cp;
	fp->prev = sp->stkbase;
	sp->stkbase = cp;
	sp->stkend = fp->end = cp+n;
	cp = (char*)(fp+1);
	cp = sp->stkbase + roundof((size_t)(cp-sp->stkbase),STK_ALIGN);
	if(fp->nalias=nn)
	{
		fp->aliases = (char**)fp->end;
		if(end && nn>add)
			memmove(fp->aliases,end,(size_t)(nn-add)*sizeof(char*));
		if(add)
			fp->aliases[nn-1] = oldbase + roundof(sizeof(struct frame),STK_ALIGN);
	}
	if(m && !dp)
		memcpy(cp,(char*)stream->_data,m);
	sfsetbuf(stream,cp,(size_t)(sp->stkend-cp));
	return (char*)(stream->_next = stream->_data+m);
}

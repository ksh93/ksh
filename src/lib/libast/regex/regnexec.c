/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2013 AT&T Intellectual Property          *
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
 * POSIX regex executor
 * single sized-record interface
 */

#include "reglib.h"

#define BEG_ALT		1	/* beginning of an alt			*/
#define BEG_ONE		2	/* beginning of one iteration of a rep	*/
#define BEG_REP		3	/* beginning of a repetition		*/
#define BEG_SUB		4	/* beginning of a subexpression		*/
#define END_ANY		5	/* end of any of above			*/

/*
 * returns from parse()
 */

#define NONE		0	/* no parse found			*/
#define GOOD		1	/* some parse was found			*/
#define CUT		2	/* no match and no backtrack		*/
#define BEST		3	/* an unbeatable parse was found	*/
#define BAD		4	/* error occurred			*/
#define SUSPEND		5	/* parse asked for another iteration	*/

/*
 * REG_SHELL_DOT test
 */

#define LEADING(e,r,s)	(*(s)==(e)->leading&&((s)==(e)->beg||*((s)-1)==(r)->explicit))

/*
 * Pos_t is for comparing parses. An entry is made in the
 * array at the beginning and at the end of each Group_t,
 * each iteration in a Group_t, and each Binary_t.
 */

typedef struct
{
	unsigned char*	p;		/* where in string		*/
	size_t		length;		/* length in string		*/
	int		serial;		/* preorder subpattern number	*/
	short		be;		/* which end of pair		*/
} Pos_t;

/* ===== begin library support ===== */

#define vector(t,v,i)	(((i)<(v)->max)?(t*)((v)->vec+(size_t)(i)*(v)->siz):(t*)vecseek(&(v),i))

static Vector_t*
vecopen(ssize_t inc, size_t siz)
{
	Vector_t*	v;
	Stk_t*		sp;

	if (inc <= 0)
		inc = 16;
	if (!(sp = stkopen(STK_SMALL|STK_NULL)))
		return NULL;
	if (!(v = stkseek(sp, (ptrdiff_t)(sizeof(Vector_t) + (size_t)inc * siz))))
	{
		stkclose(sp);
		return NULL;
	}
	v->stk = sp;
	v->vec = (char*)v + sizeof(Vector_t);
	v->max = v->inc = inc;
	v->siz = siz;
	v->cur = 0;
	return v;
}

static void*
vecseek(Vector_t** p, ssize_t index)
{
	Vector_t*	v = *p;

	if (index >= v->max)
	{
		while ((v->max += v->inc) <= index);
		if (!(v = stkseek(v->stk, (ptrdiff_t)(sizeof(Vector_t) + (size_t)v->max * v->siz))))
			return NULL;
		*p = v;
		v->vec = (char*)v + sizeof(Vector_t);
	}
	return v->vec + (size_t)index * v->siz;
}

static void
vecclose(Vector_t* v)
{
	if (v)
		stkclose(v->stk);
}

typedef struct
{
	Stk_pos_t	pos;
	char		data[1];
} Stk_frame_t;

#define stknew(s,p)	((p)->offset=stktell(s),(p)->base=stkfreeze(s,0))
#define stkold(s,p)	stkset(s,(p)->base,(p)->offset)

#define stkframe(s)	(*((Stk_frame_t**)stktop(s)-1))
#define stkdata(s,t)	((t*)stkframe(s)->data)
#define stkpop(s)	stkold(s,&(stkframe(s)->pos))

static void*
stkpush(Stk_t* sp, size_t size)
{
	Stk_frame_t*	f;
	Stk_pos_t	p;

	stknew(sp, &p);
	size = sizeof(Stk_frame_t) + sizeof(size_t) + size - 1;
	if (!(f = stkalloc(sp, sizeof(Stk_frame_t) + sizeof(Stk_frame_t*) + size - 1)))
		return NULL;
	f->pos = p;
	stkframe(sp) = f;
	return f->data;
}

/* ===== end library support ===== */

/*
 * Match_frame_t is for saving and restoring match records
 * around alternate attempts, so that fossils will not be
 * left in the match array.  These are the only entries in
 * the match array that are not otherwise guaranteed to
 * have current data in them when they get used.
 */

typedef struct
{
	size_t			size;
	regmatch_t*		match;
	regmatch_t		save[1];
} Match_frame_t;

#define matchpush(e,x)	((x)->re.group.number?_matchpush(e,x):0)
#define matchcopy(e,x)	do if ((x)->re.group.number) { Match_frame_t* fp = (void*)stkframe(e->mst)->data; memcpy(fp->match, fp->save, fp->size); } while (0)
#define matchpop(e,x)	do if ((x)->re.group.number) { Match_frame_t* fp = (void*)stkframe(e->mst)->data; memcpy(fp->match, fp->save, fp->size); stkpop(e->mst); } while (0)

#define pospop(e)	(--(e)->pos->cur)

/*
 * allocate a frame and push a match onto the stack
 */

static int
_matchpush(Env_t* env, Rex_t* rex)
{
	Match_frame_t*	f;
	regmatch_t*	m;
	regmatch_t*	e;
	regmatch_t*	s;
	ssize_t		num;

	if (rex->re.group.number <= 0 || (num = rex->re.group.last - rex->re.group.number + 1) <= 0)
		num = 0;
	if (!(f = stkpush(env->mst, sizeof(Match_frame_t) + (size_t)(num > 0 ? num - 1 : 0) * sizeof(regmatch_t))))
	{
		env->error = REG_ESPACE;
		return 1;
	}
	f->size = (size_t)num * sizeof(regmatch_t);
	f->match = m = env->match + rex->re.group.number;
	e = m + num;
	s = f->save;
	while (m < e)
	{
		*s++ = *m;
		*m++ = state.nomatch;
	}
	return 0;
}

/*
 * allocate a frame and push a pos onto the stack
 */

static int
pospush(Env_t* env, Rex_t* rex, unsigned char* p, short be)
{
	Pos_t*	pos;

	if (!(pos = vector(Pos_t, env->pos, env->pos->cur)))
	{
		env->error = REG_ESPACE;
		return 1;
	}
	pos->serial = rex->serial;
	pos->p = p;
	pos->be = be;
	env->pos->cur++;
	return 0;
}

/*
 * two matches are known to have the same length
 * os is start of old pos array, ns is start of new,
 * oend and nend are end+1 pointers to ends of arrays.
 * oe and ne are ends (not end+1) of subarrays.
 * returns 1 if new is better, -1 if old, else 0.
 */

static int
better(Env_t* env, Pos_t* os, Pos_t* ns, Pos_t* oend, Pos_t* nend, int level)
{
	Pos_t*	oe;
	Pos_t*	ne;
	int	k;
	int	n;

	if (env->error)
		return -1;
	for (;;)
	{
		if (ns >= nend)
			return 0;
		if (os >= oend)
			return 1;
		n = os->serial;
		if (ns->serial > n)
			return -1;
		if (n > ns->serial)
		{
			env->error = REG_PANIC;
			return -1;
		}
		if (ns->p > os->p)
			return 1;
		if (os->p > ns->p)
			return -1;
		oe = os;
		k = 0;
		for (;;)
			if ((++oe)->serial == n)
			{
				if (oe->be != END_ANY)
					k++;
				else if (k-- <= 0)
					break;
			}
		ne = ns;
		k = 0;
		for (;;)
			if ((++ne)->serial == n)
			{
				if (ne->be != END_ANY)
					k++;
				else if (k-- <= 0)
					break;
			}
		if (ne->p > oe->p)
			return 1;
		if (oe->p > ne->p)
			return -1;
		if (k = better(env, os + 1, ns + 1, oe, ne, level + 1))
			return k;
		os = oe + 1;
		ns = ne + 1;
	}
}

#define follow(e,r,c,s)	((r)->next?parse(e,(r)->next,c,s):(c)?parse(e,c,NULL,s):BEST)

static int		parse(Env_t*, Rex_t*, Rex_t*, unsigned char*);

/*
 * Matching a repetition means parsing its body over and over again, handing
 * control to the continuation after each iteration. Doing that by calling
 * parserep() recursively grows the C stack per iteration, which overflows if
 * there are too many matching elements: crash. Instead, the recursion is now
 * emulated with a stack of Rep_frame_t on the heap, which has no such limit.
 *
 * When a nested iteration is requested from deep inside parse(), the C stack
 * unwinds back to parserep() with SUSPEND, which then runs the requested
 * iteration itself and hands the result back via the requesting rep catcher.
 *
 * Each frame saves the mutable parse state (position vector, match stack,
 * match array and end pointer) just before it parses anything, and restores
 * it whenever the attempt is resumed, so that a resumed attempt sees the
 * same state that the recursive version would have seen.
 */

#define REP_START	0	/* fresh iteration			*/
#define REP_DOWN	1	/* body parse suspended; replay it	*/
#define REP_FOLLOW1	2	/* MINIMAL early exit follow suspended	*/
#define REP_FOLLOW3	3	/* trailing follow suspended		*/

/*
 * One iteration that has already been run for a repetition, and the result it
 * produced. A repetition with a body that offers a choice can reach the same
 * rep catcher more than once while parsing its body, each time at a different
 * string position and each time needing its own iteration, so one result per
 * catcher is not enough: results are kept per position instead.
 */

typedef struct Rep_req_s
{
	unsigned char*	s;		/* string position the iteration started at	*/
	int		res;		/* what it returned				*/
} Rep_req_t;

typedef struct Rep_frame_s
{
	Rex_t		catcher;	/* REX_REP_CATCH continuation; must be first,
					   because catcher addresses identify frames	*/
	Rex_t*		rex;		/* the repetition being matched			*/
	Rex_t*		cont;		/* its continuation				*/
	size_t		parent;		/* frame that asked for this one, or ~0		*/
	size_t		reqidx;		/* entry in the parent's req[] to fill in	*/
	unsigned char*	s;		/* string position of this iteration		*/
	int		n;		/* iteration number				*/
	int		stage;		/* how far this iteration has got		*/
	int		r;		/* saved result of the body parse		*/
	Rep_req_t*	req;		/* iterations already run for this catcher	*/
	size_t		nreq;		/* number of entries in req[]			*/
	size_t		reqmax;		/* allocated size of req[]			*/
	regmatch_t*	match;		/* saved copy of env->match[0..nsub]		*/
	ssize_t		poscur;		/* saved env->pos->cur				*/
	Stk_pos_t	mstpos;		/* saved env->mst position			*/
	unsigned char*	end;		/* saved env->end				*/
} Rep_frame_t;

/*
 * save parse state
 */

static void
rep_save(Env_t* env, Rep_frame_t* f)
{
	if (env->stack)
	{
		f->poscur = env->pos->cur;
		stknew(env->mst, &f->mstpos);
		f->end = env->end;
		memcpy(f->match, env->match, (env->nsub + 1) * sizeof(regmatch_t));
	}
}

/*
 * restore parse state: undo everything that the suspended attempt pushed onto it
 */

static void
rep_restore(Env_t* env, Rep_frame_t* f)
{
	if (env->stack)
	{
		env->pos->cur = f->poscur;
		stkold(env->mst, &f->mstpos);
		env->end = f->end;
		memcpy(env->match, f->match, (env->nsub + 1) * sizeof(regmatch_t));
	}
}

/*
 * run one iteration of one repetition, returning SUSPEND if the parse asked
 * for an iteration that has to be run by the caller (see parserep() below)
 */

static int
rep_iterate(Env_t* env, Rep_frame_t* f)
{
	Rex_t*		rex = f->rex;
	Rex_t*		cont = f->cont;
	Rex_t*		catcher = &f->catcher;
	unsigned char*	s = f->s;
	int		n = f->n;
	int		i;
	int		r = NONE;

	/* a resumed attempt must first undo what the suspended attempt pushed */
	if (f->stage != REP_START)
		rep_restore(env, f);
	if (f->stage == REP_FOLLOW3)
	{
		r = f->r;
		goto final;
	}
	if (f->stage == REP_DOWN)
		goto descend;

	if ((rex->flags & REG_MINIMAL) && n >= rex->lo && n < rex->hi)
	{
		/* minimal repetition: try the continuation before repeating again */
		rep_save(env, f);
		if (env->stack && pospush(env, rex, s, END_ANY))
			return BAD;
		i = follow(env, rex, cont, s);
		if (i == SUSPEND)
		{
			f->stage = REP_FOLLOW1;
			return SUSPEND;
		}
		rep_restore(env, f);
		switch (i)
		{
		case BAD:
			return BAD;
		case CUT:
			return CUT;
		case BEST:
		case GOOD:
			return BEST;
		}
	}
	r = NONE;
	if (n < rex->hi)
	{
		catcher->type = REX_REP_CATCH;
		catcher->serial = rex->serial;
		catcher->re.rep_catch.ref = rex;
		catcher->re.rep_catch.cont = cont;
		catcher->re.rep_catch.beg = s;
		catcher->re.rep_catch.s = s;
		catcher->re.rep_catch.n = n + 1;
		catcher->next = rex->next;
		if (n == 0)
			rex->re.rep_catch.beg = s;
		if (env->stack)
		{
			if (matchpush(env, rex))
				return BAD;
			if (pospush(env, rex, s, BEG_ONE))
				return BAD;
		}
descend:
		rep_save(env, f);
		f->stage = REP_DOWN;
		r = parse(env, rex->re.group.expr.rex, catcher, s);
		if (r == SUSPEND)
			return SUSPEND;
		rep_restore(env, f);
		if (env->stack)
		{
			pospop(env);
			matchpop(env, rex);
		}
		switch (r)
		{
		case BAD:
			return BAD;
		case BEST:
			return BEST;
		case CUT:
			r = NONE;
			break;
		case GOOD:
			if (rex->flags & REG_MINIMAL)
				return BEST;
			r = GOOD;
			break;
		}
	}
	if (n < rex->lo)
		return r;
	f->r = r;
final:
	if (!(rex->flags & REG_MINIMAL) || n >= rex->hi)
	{
		/* try to match the continuation */
		rep_save(env, f);
		if (env->stack && pospush(env, rex, s, END_ANY))
			return BAD;
		i = follow(env, rex, cont, s);
		if (i == SUSPEND)
		{
			f->stage = REP_FOLLOW3;
			return SUSPEND;
		}
		rep_restore(env, f);
		switch (i)
		{
		case BAD:
			r = BAD;
			break;
		case CUT:
			r = CUT;
			break;
		case BEST:
			r = BEST;
			break;
		case GOOD:
			r = (rex->flags & REG_MINIMAL) ? BEST : GOOD;
			break;
		}
	}
	return r;
}

static int
parserep(Env_t* env, Rex_t* rex, Rex_t* cont, unsigned char* s, int n)
{
	Rep_frame_t**	stack = NULL;
	Rep_frame_t**	newstack;
	Rep_frame_t*	f;
	Rep_frame_t*	fr;
	Rep_catch_t*	rc;
	size_t		requester = ~(size_t)0;
	size_t		reqidx = 0;
	ssize_t		max = 0;
	ssize_t		top = 0;
	ssize_t		i;
	int		r = BAD;
	size_t		pidx;
	size_t		ridx;

	for (;;)
	{
		if (top >= max)
		{
			ssize_t	oldmax = max;
			/* grow the stack of pointers to repetition frames, exponentially at first, but limit that */
			max = !max ? 8 : max < 2048 ? 2 * max : max + 2048;
			newstack = realloc(stack, (size_t)max * sizeof(Rep_frame_t*));
			if (!newstack)
			{
				env->error = REG_ESPACE;
				goto done;
			}
			stack = newstack;
			/* init added pointers to NULL */
			memset(stack + oldmax, 0, (size_t)(max - oldmax) * sizeof(Rep_frame_t*));
		}
		if (!stack[top])
		{
			f = calloc(1, sizeof(Rep_frame_t));
			if (!f)
			{
				env->error = REG_ESPACE;
				goto done;
			}
			f->rex = rex;
			f->cont = cont;
			f->s = s;
			f->n = n;
			f->parent = requester;
			stack[top] = f;
			if (env->stack && !(f->match = malloc((env->nsub + 1) * sizeof(regmatch_t))))
			{
				free(f);
				stack[top] = NULL;
				env->error = REG_ESPACE;
				goto done;
			}
			f->reqidx = reqidx;
		}
		env->rep_suspended = NULL;
		r = rep_iterate(env, stack[top]);
		if (env->rep_suspended)
		{
			/*
			 * Run the iteration that the parse asked for. The catcher that asked
			 * for it belongs to a frame that is still suspended below us, and
			 * that frame is the one to resume once it is done. It is usually the
			 * frame we just ran, but not always: a repetition can ask for another
			 * iteration from within its trailing continuation, in which case the
			 * catcher belongs to a frame further down. The catcher is the first
			 * member of its frame, so its address identifies the frame.
			 */
			for (i = top; i >= 0; i--)
				if (stack[i] == (Rep_frame_t*)env->rep_suspended)
					break;
			if (i < 0)
			{
				/*
				 * The catcher belongs to an enclosing repetition, reached
				 * from within the body of the one we are parsing. Only the
				 * enclosing parserep() can run that iteration, so drop our
				 * frames and let SUSPEND travel up to it and restore state.
				 */
				r = SUSPEND;
				goto done;
			}
			fr = (Rep_frame_t*)env->rep_suspended;
			rc = &fr->catcher.re.rep_catch;
			/*
			 * Record the request, so that a later visit to this catcher at
			 * the same position can be answered from it.
			 */
			if (fr->nreq >= fr->reqmax)
			{
				size_t	oldmax = fr->reqmax;
				Rep_req_t*	newreq;

				if (!(newreq = realloc(fr->req, (fr->reqmax = oldmax ? 2 * oldmax : 4) * sizeof(Rep_req_t))))
				{
					env->error = REG_ESPACE;
					r = BAD;
					goto done;
				}
				fr->req = newreq;
			}
			fr->req[fr->nreq].s = s = rc->s;
			fr->req[fr->nreq].res = NONE;
			reqidx = fr->nreq++;
			rex = rc->ref;
			cont = rc->cont;
			n = rc->n;
			requester = (size_t)i;
			top++;
			continue;
		}

		pidx = stack[top]->parent;
		ridx = stack[top]->reqidx;

		if (pidx == ~(size_t)0)
			goto done;
		/*
		 * This iteration is finished; return its result to the catcher that asked for
		 * it, then discard every frame above the one to resume. The frame to resume
		 * is identified by the frame that just finished, not by whatever was pushed
		 * most recently, which may have been a different branch of the search.
		 */
		i = (ssize_t)pidx;
		while (top > i)
		{
			free(stack[top]->match);
			free(stack[top]->req);
			free(stack[top]);
			stack[top] = NULL;
			top--;
		}
		stack[top]->req[ridx].res = r;
	}
 done:
	for (i = 0; stack && i < max; i++)
	{
		if (stack[i])
		{
			free(stack[i]->match);
			free(stack[i]->req);
			free(stack[i]);
		}
	}
	free(stack);
	return r;
}

static int
parsetrie(Env_t* env, Trie_node_t* x, Rex_t* rex, Rex_t* cont, unsigned char* s)
{
	unsigned char*	p;
	int		r;

	if (p = rex->map)
	{
		for (;;)
		{
			if (s >= env->end)
				return NONE;
			while (x->c != p[*s])
				if (!(x = x->sib))
					return NONE;
			if (x->end)
				break;
			x = x->son;
			s++;
		}
	}
	else
	{
		for (;;)
		{
			if (s >= env->end)
				return NONE;
			while (x->c != *s)
				if (!(x = x->sib))
					return NONE;
			if (x->end)
				break;
			x = x->son;
			s++;
		}
	}
	s++;
	if (rex->flags & REG_MINIMAL)
		switch (follow(env, rex, cont, s))
		{
		case BAD:
			return BAD;
		case CUT:
			return CUT;
		case BEST:
		case GOOD:
			return BEST;
		case SUSPEND:
			return SUSPEND;
		}
	if (x->son)
		switch (parsetrie(env, x->son, rex, cont, s))
		{
		case BAD:
			return BAD;
		case CUT:
			return CUT;
		case BEST:
			return BEST;
		case GOOD:
			if (rex->flags & REG_MINIMAL)
				return BEST;
			r = GOOD;
			break;
		case SUSPEND:
			return SUSPEND;
		default:
			r = NONE;
			break;
		}
	else
		r = NONE;
	if (!(rex->flags & REG_MINIMAL))
		switch (follow(env, rex, cont, s))
		{
		case BAD:
			return BAD;
		case CUT:
			return CUT;
		case BEST:
			return BEST;
		case GOOD:
			return GOOD;
		case SUSPEND:
			return SUSPEND;
	}
	return r;
}

static int
collelt(Celt_t* ce, char* key, int c, ptrdiff_t x)
{
	Ckey_t	elt;

	assert(ast.locale.transform != NULL);
	ast.locale.transform((char*)elt, key, COLL_KEY_MAX);
	for (;; ce++)
	{
		switch (ce->typ)
		{
		case COLL_call:
			if (!x && (*ce->fun)(c))
				return 1;
			continue;
		case COLL_char:
			if (!strcmp((char*)ce->beg, (char*)elt))
				return 1;
			continue;
		case COLL_range:
			if (strcmp((char*)ce->beg, (char*)elt) <= ce->min && strcmp((char*)elt, (char*)ce->end) <= ce->max)
				return 1;
			continue;
		case COLL_range_lc:
			if (strcmp((char*)ce->beg, (char*)elt) <= ce->min && strcmp((char*)elt, (char*)ce->end) <= ce->max && (iswlower((wint_t)c) || !iswupper((wint_t)c)))
				return 1;
			continue;
		case COLL_range_uc:
			if (strcmp((char*)ce->beg, (char*)elt) <= ce->min && strcmp((char*)elt, (char*)ce->end) <= ce->max && (iswupper((wint_t)c) || !iswlower((wint_t)c)))
				return 1;
			continue;
		}
		break;
	}
	return 0;
}

static int
collic(Celt_t* ce, char* key, char* nxt, int c, ptrdiff_t x)
{
	if (!x)
	{
		if (collelt(ce, key, c, x))
			return 1;
		if (iswlower((wint_t)c))
			c = (int)towupper((wint_t)c);
		else if (iswupper((wint_t)c))
			c = (int)towlower((wint_t)c);
		else
			return 0;
		x = mbconv(key, c);
		key[x] = 0;
		return collelt(ce, key, c, 0);
	}
	while (*nxt)
	{
		if (collic(ce, key, nxt + 1, c, x))
			return 1;
		if (islower(*nxt))
			*nxt = (char)toupper(*nxt);
		else if (isupper(*nxt))
			*nxt = (char)tolower(*nxt);
		else
			return 0;
		nxt++;
	}
	return collelt(ce, key, c, x);
}

static int
collmatch(Rex_t* rex, unsigned char* s, unsigned char* e, unsigned char** p)
{
	unsigned char*		t;
	wchar_t			c;
	int			r;
	ptrdiff_t		w;
	ptrdiff_t		x;
	int			ic;
	Ckey_t			key;
	Ckey_t			elt;

	assert(ast.locale.transform != NULL);
	ic = (rex->flags & REG_ICASE);
	if ((w = MBSIZE(s)) > 1)
	{
		memcpy((char*)key, (char*)s, (size_t)w);
		key[w] = 0;
		t = s;
		c = mbchar(t);
#if !_lib_wctype
		c &= 0xff;
#endif
		x = 0;
	}
	else
	{
		c = s[0];
		if (ic && isupper(c))
			c = tolower(c);
		key[0] = (unsigned char)c;
		key[1] = 0;
		if (isalpha(c))
		{
			x = e - s;
			if (x > COLL_KEY_MAX)
				x = COLL_KEY_MAX;
			while (w < x)
			{
				size_t	z;
				c = s[w];
				if (!isalpha(c))
					break;
				z = ast.locale.transform((char*)elt, (const char*)key, COLL_KEY_MAX);
				if (ic && isupper(c))
					c = tolower(c);
				key[w] = (unsigned char)c;
				key[w + 1] = 0;
				if (ast.locale.transform((char*)elt, (const char*)key, COLL_KEY_MAX) != z)
					break;
				w++;
			}
		}
		key[w] = 0;
		c = key[0];
		x = w - 1;
	}
	r = 1;
	for (;;)
	{
		if (ic ? collic(rex->re.collate.elements, (char*)key, (char*)key, c, x) : collelt(rex->re.collate.elements, (char*)key, c, x))
			break;
		if (!x)
		{
			r = 0;
			break;
		}
		w = x--;
		key[w] = 0;
	}
	*p = s + w;
	return rex->re.collate.invert ? !r : r;
}

static unsigned char*
nestmatch(unsigned char* s, unsigned char* e, const unsigned short* type, int co)
{
	int		c;
	int		cc;
	unsigned int	n;
	int		oc;

	if (type[co] & (REX_NEST_literal|REX_NEST_quote))
	{
		n = (type[co] & REX_NEST_literal) ? REX_NEST_terminator : (REX_NEST_escape|REX_NEST_terminator);
		while (s < e)
		{
			c = *s++;
			if (c == co)
				return s;
			else if (type[c] & n)
			{
				if (s >= e || (type[c] & REX_NEST_terminator))
					break;
				s++;
			}
		}
	}
	else
	{
		cc = type[co] >> REX_NEST_SHIFT;
		oc = type[co] & (REX_NEST_open|REX_NEST_close);
		n = 1;
		while (s < e)
		{
			c = *s++;
			switch (type[c] & (REX_NEST_escape|REX_NEST_open|REX_NEST_close|REX_NEST_delimiter|REX_NEST_separator|REX_NEST_terminator))
			{
			case REX_NEST_delimiter:
			case REX_NEST_terminator:
				return oc ? NULL : s;
			case REX_NEST_separator:
				if (!oc)
					return s;
				break;
			case REX_NEST_escape:
				if (s >= e)
					return NULL;
				s++;
				break;
			case REX_NEST_open|REX_NEST_close:
				if (c == cc)
				{
					if (!--n)
						return s;
				}
				/* FALLTHROUGH */
			case REX_NEST_open:
				if (c == co)
				{
					if (!++n)
						return NULL;
				}
				else if (!(s = nestmatch(s, e, type, c)))
					return NULL;
				break;
			case REX_NEST_close:
				if (c != cc)
					return NULL;
				if (!--n)
					return s;
				break;
			}
		}
		return (oc || !(type[UCHAR_MAX+1] & REX_NEST_terminator)) ? NULL : s;
	}
	return NULL;
}

static int
parse(Env_t* env, Rex_t* rex, Rex_t* cont, unsigned char* s)
{
	int		c;
	int		d;
	int		r;
	ptrdiff_t	n;
	ptrdiff_t	m;
	unsigned char*	p;
	unsigned char*	t;
	unsigned char*	b;
	unsigned char*	e;
	char*		u;
	regmatch_t*	o;
	Trie_node_t*	x;
	Rex_t*		q;
	Rex_t		catcher;
	Rex_t		next;
	ssize_t		cur_save;
	regoff_t	rf;
	ptrdiff_t	j;

	for (;;)
	{
		switch (rex->type)
		{
		case REX_ALT:
			if (env->stack)
			{
				if (matchpush(env, rex))
					return BAD;
				if (pospush(env, rex, s, BEG_ALT))
					return BAD;
				catcher.type = REX_ALT_CATCH;
				catcher.serial = rex->serial;
				catcher.re.alt_catch.cont = cont;
				catcher.next = rex->next;
				r = parse(env, rex->re.group.expr.binary.left, &catcher, s);
				if (r == SUSPEND)
					return SUSPEND;
				if (r < BEST || (rex->flags & REG_MINIMAL))
				{
					matchcopy(env, rex);
					((Pos_t*)env->pos->vec + env->pos->cur - 1)->serial = catcher.serial = rex->re.group.expr.binary.serial;
					n = parse(env, rex->re.group.expr.binary.right, &catcher, s);
					if (n == SUSPEND)
						return SUSPEND;
					if (n != NONE)
						r = (int)n;
				}
				pospop(env);
				matchpop(env, rex);
			}
			else
			{
				r = parse(env, rex->re.group.expr.binary.left, cont, s);
				if (r == SUSPEND)
					return SUSPEND;
				if (r == NONE)
				{
					r = parse(env, rex->re.group.expr.binary.right, cont, s);
					if (r == SUSPEND)
						return SUSPEND;
				}
				if (r == GOOD)
					r = BEST;
			}
			return r;
		case REX_ALT_CATCH:
			if (pospush(env, rex, s, END_ANY))
				return BAD;
			r = follow(env, rex, rex->re.alt_catch.cont, s);
			if (r == SUSPEND)
				return SUSPEND;
			pospop(env);
			return r;
		case REX_BACK:
			o = &env->match[rex->lo];
			if (o->rm_so < 0)
				return NONE;
			rf = o->rm_eo - o->rm_so;
			e = s + rf;
			if (e > env->end)
				return NONE;
			t = env->beg + o->rm_so;
			if (!(p = rex->map))
			{
				while (s < e)
					if (*s++ != *t++)
						return NONE;
			}
			else if (!mbwide())
			{
				while (s < e)
					if (p[*s++] != p[*t++])
						return NONE;
			}
			else
			{
				while (s < e)
				{
					c = mbchar(s);
					d = mbchar(t);
					if (towupper((wint_t)c) != towupper((wint_t)d))
						return NONE;
				}
			}
			break;
		case REX_BEG:
			if ((!(rex->flags & REG_NEWLINE) || s <= env->beg || *(s - 1) != '\n') && ((env->flags & REG_NOTBOL) || s != env->beg))
				return NONE;
			break;
		case REX_CLASS:
			if (LEADING(env, rex, s))
				return NONE;
			n = rex->hi;
			if (n > env->end - s)
				n = env->end - s;
			m = rex->lo;
			if (m > n)
				return NONE;
			r = NONE;
			if (!(rex->flags & REG_MINIMAL))
			{
				intmax_t i;
				for (i = 0; i < n; i++)
					if (!settst(rex->re.charclass, s[i]))
					{
						n = (ptrdiff_t)i;
						break;
					}
				for (s += n; n-- >= m; s--)
					switch (follow(env, rex, cont, s))
					{
					case BAD:
						return BAD;
					case CUT:
						return CUT;
					case BEST:
						return BEST;
					case GOOD:
						r = GOOD;
						break;
					case SUSPEND:
						return SUSPEND;
					}
			}
			else
			{
				for (e = s + m; s < e; s++)
					if (!settst(rex->re.charclass, *s))
						return r;
				e += n - m;
				for (;;)
				{
					switch (follow(env, rex, cont, s))
					{
					case BAD:
						return BAD;
					case CUT:
						return CUT;
					case BEST:
					case GOOD:
						return BEST;
					case SUSPEND:
						return SUSPEND;
					}
					if (s >= e || !settst(rex->re.charclass, *s))
						break;
					s++;
				}
			}
			return r;
		case REX_COLL_CLASS:
			if (LEADING(env, rex, s))
				return NONE;
			n = rex->hi;
			if (n > env->end - s)
				n = env->end - s;
			m = rex->lo;
			if (m > n)
				return NONE;
			r = NONE;
			e = env->end;
			if (!(rex->flags & REG_MINIMAL))
			{
				intmax_t i;
				if (!(b = stkpush(env->mst, (size_t)n)))
				{
					env->error = REG_ESPACE;
					return BAD;
				}
				for (i = 0; s < e && i < n && collmatch(rex, s, e, &t); i++)
				{
					b[i] = (unsigned char)(t - s);
					s = t;
				}
				for (; i-- >= rex->lo; s -= b[i])
					switch (follow(env, rex, cont, s))
					{
					case BAD:
						stkpop(env->mst);
						return BAD;
					case CUT:
						stkpop(env->mst);
						return CUT;
					case BEST:
						stkpop(env->mst);
						return BEST;
					case GOOD:
						r = GOOD;
						break;
					case SUSPEND:
						return SUSPEND;
					}
				stkpop(env->mst);
			}
			else
			{
				intmax_t i;
				for (i = 0; i < m && s < e; i++, s = t)
					if (!collmatch(rex, s, e, &t))
						return r;
				while (i++ <= n)
				{
					switch (follow(env, rex, cont, s))
					{
					case BAD:
						return BAD;
					case CUT:
						return CUT;
					case BEST:
					case GOOD:
						return BEST;
					case SUSPEND:
						return SUSPEND;
					}
					if (s >= e || !collmatch(rex, s, e, &s))
						break;
				}
			}
			return r;
		case REX_CONJ:
			next.type = REX_CONJ_RIGHT;
			next.re.conj_right.cont = cont;
			next.next = rex->next;
			catcher.type = REX_CONJ_LEFT;
			catcher.re.conj_left.right = rex->re.group.expr.binary.right;
			catcher.re.conj_left.cont = &next;
			catcher.re.conj_left.beg = s;
			catcher.next = NULL;
			return parse(env, rex->re.group.expr.binary.left, &catcher, s);
		case REX_CONJ_LEFT:
			rex->re.conj_left.cont->re.conj_right.end = s;
			cont = rex->re.conj_left.cont;
			s = rex->re.conj_left.beg;
			rex = rex->re.conj_left.right;
			continue;
		case REX_CONJ_RIGHT:
			if (rex->re.conj_right.end != s)
				return NONE;
			cont = rex->re.conj_right.cont;
			break;
		case REX_DONE:
		{
			Pos_t*	pos;
			if (!env->stack)
				return BEST;
			n = s - env->beg;
			r = (int)env->nsub;
			if ((rf = env->best[0].rm_eo) >= 0)
			{
				if (rex->flags & REG_MINIMAL)
				{
					if (n > rf)
						return GOOD;
				}
				else
				{
					if (n < rf)
						return GOOD;
				}
				if (n == rf && better(env,
						     (Pos_t*)env->bestpos->vec,
						     (Pos_t*)env->pos->vec,
						     (Pos_t*)env->bestpos->vec+env->bestpos->cur,
						     (Pos_t*)env->pos->vec+env->pos->cur,
						     0) <= 0)
					return GOOD;
			}
			env->best[0].rm_eo = (regoff_t)n;
			memcpy(&env->best[1], &env->match[1], (size_t)r * sizeof(regmatch_t));
			cur_save = env->pos->cur;
			pos = vector(Pos_t, env->bestpos, cur_save);
			if (!pos)
			{
				env->error = REG_ESPACE;
				return BAD;
			}
			env->bestpos->cur = cur_save;
			memcpy(env->bestpos->vec, env->pos->vec, (size_t)cur_save * sizeof(Pos_t));
			return GOOD;
		}
		case REX_DOT:
			if (LEADING(env, rex, s))
				return NONE;
			n = rex->hi;
			if (n > env->end - s)
				n = env->end - s;
			m = rex->lo;
			if (m > n)
				return NONE;
			if ((c = rex->explicit) >= 0 && !mbwide())
			{
				intmax_t i;
				for (i = 0; i < n; i++)
					if (s[i] == c)
					{
						n = (ptrdiff_t)i;
						break;
					}
			}
			r = NONE;
			if (!(rex->flags & REG_MINIMAL))
			{
				if (!mbwide())
				{
					for (s += n; n-- >= m; s--)
						switch (follow(env, rex, cont, s))
						{
						case BAD:
							return BAD;
						case CUT:
							return CUT;
						case BEST:
							return BEST;
						case GOOD:
							r = GOOD;
							break;
						case SUSPEND:
							return SUSPEND;
						}
				}
				else
				{
					intmax_t i;
					if (!(b = stkpush(env->mst, (size_t)n)))
					{
						env->error = REG_ESPACE;
						return BAD;
					}
					e = env->end;
					for (i = 0; s < e && i < n && *s != c; i++)
						s += b[i] = (unsigned char)MBSIZE(s);
					for (; i-- >= m; s -= b[i])
						switch (follow(env, rex, cont, s))
						{
						case BAD:
							stkpop(env->mst);
							return BAD;
						case CUT:
							stkpop(env->mst);
							return CUT;
						case BEST:
							stkpop(env->mst);
							return BEST;
						case GOOD:
							r = GOOD;
							break;
						case SUSPEND:
							return SUSPEND;
						}
					stkpop(env->mst);
				}
			}
			else
			{
				if (!mbwide())
				{
					e = s + n;
					for (s += m; s <= e; s++)
						switch (follow(env, rex, cont, s))
						{
						case BAD:
							return BAD;
						case CUT:
							return CUT;
						case BEST:
						case GOOD:
							return BEST;
						case SUSPEND:
							return SUSPEND;
						}
				}
				else
				{
					intmax_t i;
					e = env->end;
					for (i = 0; s < e && i < m && *s != c; i++)
						s += MBSIZE(s);
					if (i >= m)
						for (; s <= e && i <= n; s += MBSIZE(s), i++)
							switch (follow(env, rex, cont, s))
							{
							case BAD:
								return BAD;
							case CUT:
								return CUT;
							case BEST:
							case GOOD:
								return BEST;
							case SUSPEND:
								return SUSPEND;
							}
				}
			}
			return r;
		case REX_END:
			if ((!(rex->flags & REG_NEWLINE) || *s != '\n') && ((env->flags & REG_NOTEOL) || s < env->end))
				return NONE;
			break;
		case REX_GROUP:
			if (env->stack)
			{
				if (rex->re.group.number)
					env->match[rex->re.group.number].rm_so = s - env->beg;
				if (pospush(env, rex, s, BEG_SUB))
					return BAD;
				catcher.re.group_catch.eo = rex->re.group.number ? &env->match[rex->re.group.number].rm_eo : NULL;
			}
			catcher.type = REX_GROUP_CATCH;
			catcher.serial = rex->serial;
			catcher.re.group_catch.cont = cont;
			catcher.next = rex->next;
			r = parse(env, rex->re.group.expr.rex, &catcher, s);
			if (r == SUSPEND)
				return SUSPEND;
			if (env->stack)
			{
				pospop(env);
				if (rex->re.group.number)
					env->match[rex->re.group.number].rm_so = -1;
			}
			return r;
		case REX_GROUP_CATCH:
			if (env->stack)
			{
				if (rex->re.group_catch.eo)
					*rex->re.group_catch.eo = s - env->beg;
				if (pospush(env, rex, s, END_ANY))
					return BAD;
			}
			r = follow(env, rex, rex->re.group_catch.cont, s);
			if (r == SUSPEND)
				return SUSPEND;
			if (env->stack)
			{
				pospop(env);
				if (rex->re.group_catch.eo)
					*rex->re.group_catch.eo = -1;
			}
			return r;
		case REX_GROUP_AHEAD:
			catcher.type = REX_GROUP_AHEAD_CATCH;
			catcher.flags = rex->flags;
			catcher.serial = rex->serial;
			catcher.re.rep_catch.beg = s;
			catcher.re.rep_catch.cont = cont;
			catcher.next = rex->next;
			return parse(env, rex->re.group.expr.rex, &catcher, s);
		case REX_GROUP_AHEAD_CATCH:
			return follow(env, rex, rex->re.rep_catch.cont, rex->re.rep_catch.beg);
		case REX_GROUP_AHEAD_NOT:
			r = parse(env, rex->re.group.expr.rex, NULL, s);
			if (r == SUSPEND)
				return SUSPEND;
			if (r == NONE)
			{
				r = follow(env, rex, cont, s);
				if (r == SUSPEND)
					return SUSPEND;
			}
			else if (r != BAD)
				r = NONE;
			return r;
		case REX_GROUP_BEHIND:
			if ((s - env->beg) < rex->re.group.size)
				return NONE;
			catcher.type = REX_GROUP_BEHIND_CATCH;
			catcher.flags = rex->flags;
			catcher.serial = rex->serial;
			catcher.re.behind_catch.beg = s;
			catcher.re.behind_catch.end = e = env->end;
			catcher.re.behind_catch.cont = cont;
			catcher.next = rex->next;
			for (t = s - rex->re.group.size; t >= env->beg; t--)
			{
				env->end = s;
				r = parse(env, rex->re.group.expr.rex, &catcher, t);
				if (r == SUSPEND)
					return SUSPEND;
				env->end = e;
				if (r != NONE)
					return r;
			}
			return NONE;
		case REX_GROUP_BEHIND_CATCH:
			if (s != rex->re.behind_catch.beg)
				return NONE;
			env->end = rex->re.behind_catch.end;
			return follow(env, rex, rex->re.behind_catch.cont, rex->re.behind_catch.beg);
		case REX_GROUP_BEHIND_NOT:
			if ((s - env->beg) < rex->re.group.size)
				r = NONE;
			else
			{
				catcher.type = REX_GROUP_BEHIND_NOT_CATCH;
				catcher.re.neg_catch.beg = s;
				catcher.next = NULL;
				e = env->end;
				env->end = s;
				for (t = s - rex->re.group.size; t >= env->beg; t--)
				{
					r = parse(env, rex->re.group.expr.rex, &catcher, t);
					if (r == SUSPEND)
						return SUSPEND;
					if (r != NONE)
						break;
				}
				env->end = e;
			}
			if (r == NONE)
			{
				r = follow(env, rex, cont, s);
				if (r == SUSPEND)
					return SUSPEND;
			}
			else if (r != BAD)
				r = NONE;
			return r;
		case REX_GROUP_BEHIND_NOT_CATCH:
			return s == rex->re.neg_catch.beg ? GOOD : NONE;
		case REX_GROUP_COND:
			if (q = rex->re.group.expr.binary.right)
			{
				catcher.re.cond_catch.next[0] = q->re.group.expr.binary.right;
				catcher.re.cond_catch.next[1] = q->re.group.expr.binary.left;
			}
			else
				catcher.re.cond_catch.next[0] = catcher.re.cond_catch.next[1] = NULL;
			if (q = rex->re.group.expr.binary.left)
			{
				catcher.type = REX_GROUP_COND_CATCH;
				catcher.flags = rex->flags;
				catcher.serial = rex->serial;
				catcher.re.cond_catch.yes = 0;
				catcher.re.cond_catch.beg = s;
				catcher.re.cond_catch.cont = cont;
				catcher.next = rex->next;
				r = parse(env, q, &catcher, s);
				if (r == SUSPEND)
					return SUSPEND;
				if (r == BAD || catcher.re.cond_catch.yes)
					return r;
			}
			else if (!rex->re.group.size || rex->re.group.size > 0 && env->match[rex->re.group.size].rm_so >= 0)
				r = GOOD;
			else
				r = NONE;
			if (q = catcher.re.cond_catch.next[r != NONE])
			{
				catcher.type = REX_CAT;
				catcher.flags = q->flags;
				catcher.serial = q->serial;
				catcher.re.group_catch.cont = cont;
				catcher.next = rex->next;
				return parse(env, q, &catcher, s);
			}
			return follow(env, rex, cont, s);
		case REX_GROUP_COND_CATCH:
			rex->re.cond_catch.yes = 1;
			catcher.type = REX_CAT;
			catcher.flags = rex->flags;
			catcher.serial = rex->serial;
			catcher.re.group_catch.cont = rex->re.cond_catch.cont;
			catcher.next = rex->next;
			return parse(env, rex->re.cond_catch.next[1], &catcher, rex->re.cond_catch.beg);
		case REX_CAT:
			return follow(env, rex, rex->re.group_catch.cont, s);
		case REX_GROUP_CUT:
			catcher.type = REX_GROUP_CUT_CATCH;
			catcher.flags = rex->flags;
			catcher.serial = rex->serial;
			catcher.re.group_catch.cont = cont;
			catcher.next = rex->next;
			return parse(env, rex->re.group.expr.rex, &catcher, s);
		case REX_GROUP_CUT_CATCH:
			switch (r = follow(env, rex, rex->re.group_catch.cont, s))
			{
			case GOOD:
				r = BEST;
				break;
			case NONE:
				r = CUT;
				break;
			case SUSPEND:
				return SUSPEND;
			}
			return r;
		case REX_NEG:
			if (LEADING(env, rex, s))
				return NONE;
			j = env->end - s;
			n = ((j + 7) >> 3) + 1;
			catcher.type = REX_NEG_CATCH;
			catcher.re.neg_catch.beg = s;
			if (!(p = stkpush(env->mst, (size_t)n)))
				return BAD;
			memset(catcher.re.neg_catch.index = p, 0, (size_t)n);
			catcher.next = rex->next;
			r = parse(env, rex->re.group.expr.rex, &catcher, s);
			if (r == SUSPEND)
				return SUSPEND;
			if (r != BAD)
			{
				r = NONE;
				for (; j >= 0; j--)
					if (!bittst(p, j))
					{
						switch (follow(env, rex, cont, s + j))
						{
						case BAD:
							r = BAD;
							break;
						case BEST:
							r = BEST;
							break;
						case CUT:
							r = CUT;
							break;
						case GOOD:
							r = GOOD;
							/* FALLTHROUGH */
						case SUSPEND:
							return SUSPEND;
						default:
							continue;
						}
						break;
					}
			}
			stkpop(env->mst);
			return r;
		case REX_NEG_CATCH:
			bitset(rex->re.neg_catch.index, s - rex->re.neg_catch.beg);
			return NONE;
		case REX_NEST:
			if (s >= env->end)
				return NONE;
			do
			{
				if ((c = *s++) == rex->re.nest.primary)
				{
					if (s >= env->end || !(s = nestmatch(s, env->end, rex->re.nest.type, c)))
						return NONE;
					break;
				}
				if (rex->re.nest.primary >= 0)
					return NONE;
				if (rex->re.nest.type[c] & (REX_NEST_delimiter|REX_NEST_separator|REX_NEST_terminator))
					break;
				if (!(s = nestmatch(s, env->end, rex->re.nest.type, c)))
					return NONE;
			} while (s < env->end && !(rex->re.nest.type[*(s-1)] & (REX_NEST_delimiter|REX_NEST_separator|REX_NEST_terminator)));
			break;
		case REX_NULL:
			break;
		case REX_ONECHAR:
			n = rex->hi;
			if (n > env->end - s)
				n = env->end - s;
			m = rex->lo;
			if (m > n)
				return NONE;
			r = NONE;
			c = rex->re.onechar;
			if (!(rex->flags & REG_MINIMAL))
			{
				intmax_t i;
				if (!mbwide())
				{
					if (p = rex->map)
					{
						for (i = 0; i < n; i++, s++)
							if (p[*s] != c)
								break;
					}
					else
					{
						for (i = 0; i < n; i++, s++)
							if (*s != c)
								break;
					}
					for (; i-- >= m; s--)
						switch (follow(env, rex, cont, s))
						{
						case BAD:
							return BAD;
						case BEST:
							return BEST;
						case CUT:
							return CUT;
						case GOOD:
							r = GOOD;
							break;
						case SUSPEND:
							return SUSPEND;
						}
				}
				else
				{
					if (!(b = stkpush(env->mst, (size_t)n)))
					{
						env->error = REG_ESPACE;
						return BAD;
					}
					e = env->end;
					if (!(rex->flags & REG_ICASE))
					{
						for (i = 0; s < e && i < n; i++, s = t)
						{
							t = s;
							if (mbchar(t) != c)
								break;
							b[i] = (unsigned char)(t - s);
						}
					}
					else
					{
						for (i = 0; s < e && i < n; i++, s = t)
						{
							t = s;
							if (towupper((wint_t)mbchar(t)) != (wint_t)c)
								break;
							b[i] = (unsigned char)(t - s);
						}
					}
					for (; i-- >= m; s -= b[i])
						switch (follow(env, rex, cont, s))
						{
						case BAD:
							stkpop(env->mst);
							return BAD;
						case BEST:
							stkpop(env->mst);
							return BEST;
						case CUT:
							stkpop(env->mst);
							return CUT;
						case GOOD:
							r = GOOD;
							break;
						case SUSPEND:
							return SUSPEND;
						}
					stkpop(env->mst);
				}
			}
			else
			{
				if (!mbwide())
				{
					e = s + m;
					if (p = rex->map)
					{
						for (; s < e; s++)
							if (p[*s] != c)
								return r;
						e += n - m;
						for (;;)
						{
							switch (follow(env, rex, cont, s))
							{
							case BAD:
								return BAD;
							case CUT:
								return CUT;
							case BEST:
							case GOOD:
								return BEST;
							case SUSPEND:
								return SUSPEND;
							}
							if (s >= e || p[*s++] != c)
								break;
						}
					}
					else
					{
						for (; s < e; s++)
							if (*s != c)
								return r;
						e += n - m;
						for (;;)
						{
							switch (follow(env, rex, cont, s))
							{
							case BAD:
								return BAD;
							case CUT:
								return CUT;
							case BEST:
							case GOOD:
								return BEST;
							case SUSPEND:
								return SUSPEND;
							}
							if (s >= e || *s++ != c)
								break;
						}
					}
				}
				else
				{
					intmax_t i;
					e = env->end;
					if (!(rex->flags & REG_ICASE))
					{
						for (i = 0; i < m && s < e; i++, s = t)
						{
							t = s;
							if (mbchar(t) != c)
								return r;
						}
						while (i++ <= n)
						{
							switch (follow(env, rex, cont, s))
							{
							case BAD:
								return BAD;
							case CUT:
								return CUT;
							case BEST:
							case GOOD:
								return BEST;
							case SUSPEND:
								return SUSPEND;
							}
							if (s >= e)
								break;
							if (mbchar(s) != c)
								break;
						}
					}
					else
					{
						for (i = 0; i < m && s < e; i++, s = t)
						{
							t = s;
							if (towupper((wint_t)mbchar(t)) != (wint_t)c)
								return r;
						}
						while (i++ <= n)
						{
							switch (follow(env, rex, cont, s))
							{
							case BAD:
								return BAD;
							case CUT:
								return CUT;
							case BEST:
							case GOOD:
								return BEST;
							case SUSPEND:
								return SUSPEND;
							}
							if (s >= e)
								break;
							if (towupper((wint_t)mbchar(s)) != (wint_t)c)
								break;
						}
					}
				}
			}
			return r;
		case REX_REP:
			if (env->stack && pospush(env, rex, s, BEG_REP))
				return BAD;
			r = parserep(env, rex, cont, s, 0);
			if (r == SUSPEND)
				return SUSPEND;
			if (env->stack)
				pospop(env);
			return r;
		case REX_REP_CATCH:
			if (env->stack && pospush(env, rex, s, END_ANY))
				return BAD;
			if (s == rex->re.rep_catch.beg && rex->re.rep_catch.n > rex->re.rep_catch.ref->lo)
			{
				/*
				 * optional empty iteration
				 */

				if (!env->stack || s != rex->re.rep_catch.ref->re.rep_catch.beg && !rex->re.rep_catch.ref->re.group.expr.rex->re.group.back)
					r = NONE;
				else if (pospush(env, rex, s, END_ANY))
					r = BAD;
				else
				{
					r = follow(env, rex, rex->re.rep_catch.cont, s);
					if (r == SUSPEND)
						return SUSPEND;
					pospop(env);
				}
			}
			else
			{
				/*
				 * If this catcher has already been run for this position,
				 * return that result. Otherwise, ask parserep() to run the
				 * iteration; it will come back here with the result recorded.
				 */
				Rep_frame_t*	fr = (Rep_frame_t*)rex;
				size_t		k;

				for (k = 0; k < fr->nreq; k++)
					if (fr->req[k].s == s)
						break;
				if (k < fr->nreq)
					r = fr->req[k].res;
				else
				{
					rex->re.rep_catch.s = s;
					env->rep_suspended = rex;
					return SUSPEND;
				}
			}
			if (env->stack)
				pospop(env);
			return r;
		case REX_STRING:
			if (rex->re.string.size > (size_t)(env->end - s))
				return NONE;
			t = rex->re.string.base;
			e = t + rex->re.string.size;
			if (!(p = rex->map))
			{
				while (t < e)
					if (*s++ != *t++)
						return NONE;
			}
			else if (!mbwide())
			{
				while (t < e)
					if (p[*s++] != *t++)
						return NONE;
			}
			else
			{
				while (t < e)
				{
					c = mbchar(s);
					d = mbchar(t);
					if (towupper((wint_t)c) != (wint_t)d)
						return NONE;
				}
			}
			break;
		case REX_TRIE:
			if (((s + rex->re.trie.min) > env->end) || !(x = rex->re.trie.root[rex->map ? rex->map[*s] : *s]))
				return NONE;
			return parsetrie(env, x, rex, cont, s);
		case REX_EXEC:
			u = NULL;
			r = (*env->disc->re_execf)(env->regex, rex->re.exec.data, rex->re.exec.text, rex->re.exec.size, (const char*)s, (size_t)(env->end - s), &u, env->disc);
			e = (unsigned char*)u;
			if (e >= s && e <= env->end)
				s = e;
			switch (r)
			{
			case 0:
				break;
			case REG_NOMATCH:
				return NONE;
			default:
				env->error = r;
				return BAD;
			}
			break;
		case REX_WBEG:
			if (!isword(*s) || s > env->beg && isword(*(s - 1)))
				return NONE;
			break;
		case REX_WEND:
			if (isword(*s) || s > env->beg && !isword(*(s - 1)))
				return NONE;
			break;
		case REX_WORD:
			if (s > env->beg && isword(*(s - 1)) == isword(*s))
				return NONE;
			break;
		case REX_WORD_NOT:
			if (s == env->beg || isword(*(s - 1)) != isword(*s))
				return NONE;
			break;
		case REX_BEG_STR:
			if (s != env->beg)
				return NONE;
			break;
		case REX_END_STR:
			for (t = s; t < env->end && *t == '\n'; t++);
			if (t < env->end)
				return NONE;
			break;
		case REX_FIN_STR:
			if (s < env->end)
				return NONE;
			break;
		}
		if (!(rex = rex->next))
		{
			if (!(rex = cont))
				break;
			cont = NULL;
		}
	}
	return GOOD;
}

/*
 * returning REG_BADPAT or REG_ESPACE is not explicitly
 * countenanced by the standard
 */

int
regnexec_20120528(const regex_t* p, const char* s, size_t len, size_t nmatch, regmatch_t* match, regflags_t flags)
{
	ssize_t		n = 0;
	int		i;
	size_t		j;
	int		k;
	size_t		m;
	int		advance;
	Env_t*		env;

	if (!p || !(env = p->env))
		return REG_BADPAT;
	if (!s)
		return fatal(env->disc, REG_BADPAT, NULL);
	if (len < env->min)
		return REG_NOMATCH;
	env->regex = p;
	env->beg = (unsigned char*)s;
	env->end = env->beg + len;
	env->flags &= ~REG_EXEC;
	env->flags |= (flags & REG_EXEC);
	advance = 0;
	stknew(env->mst, &env->stk);
	if (env->stack = env->hard || !(env->flags & REG_NOSUB) && nmatch)
	{
		n = (ssize_t)env->nsub;
		if (!(env->match = stkpush(env->mst, 2 * (size_t)(n + 1) * sizeof(regmatch_t))) ||
		    !env->pos && !(env->pos = vecopen(16, sizeof(Pos_t))) ||
		    !env->bestpos && !(env->bestpos = vecopen(16, sizeof(Pos_t))))
		{
			k = REG_ESPACE;
			goto done;
		}
		env->pos->cur = env->bestpos->cur = 0;
		env->best = &env->match[n + 1];
		env->best[0].rm_so = 0;
		env->best[0].rm_eo = -1;
		for (i = 0; i <= n; i++)
			env->match[i] = state.nomatch;
		if (flags & REG_ADVANCE)
			advance = 1;
	}
	k = REG_NOMATCH;
	j = env->once || (flags & REG_LEFT);
	while ((i = parse(env, env->rex, &env->done, (unsigned char*)s)) == NONE || advance && !env->best[0].rm_eo && !(advance = 0))
	{
		if (j)
			goto done;
		i = MBSIZE(s);
		s += i;
		if ((unsigned char*)s > env->end - env->min)
			goto done;
		if (env->stack)
			env->best[0].rm_so += i;
	}
	if ((flags & REG_LEFT) && env->stack && env->best[0].rm_so)
		goto done;
	if (k = env->error)
		goto done;
	if (i == CUT)
	{
		k = env->error = REG_NOMATCH;
		goto done;
	}
	if (!(env->flags & REG_NOSUB))
	{
		k = (env->flags & (REG_SHELL|REG_AUGMENTED)) == (REG_SHELL|REG_AUGMENTED);
		for (i = j = m = 0; j < nmatch; i++)
			if (!i || !k || (i & 1))
			{
				if (i > n)
					match[j] = state.nomatch;
				else
					match[m = j] = env->best[i];
				j++;
			}
		if (k)
		{
			while (m > 0 && match[m].rm_so == -1 && match[m].rm_eo == -1)
				m--;
			((regex_t*)p)->re_nsub = m;
		}
	}
	k = 0;
 done:
	stkold(env->mst, &env->stk);
	env->stk.base = NULL;
	if (k > REG_NOMATCH)
		fatal(p->env->disc, k, NULL);
	return k;
}

void
regfree(regex_t* p)
{
	Env_t*	env;

	if (p && (env = p->env))
	{
#if _REG_subcomp
		if (env->sub)
		{
			regsubfree(p);
			p->re_sub = NULL;
		}
#endif
		p->env = NULL;
		if (!(env->disc->re_flags & REG_NOFREE))
		{
			drop(env->disc, env->rex);
			if (env->pos)
				vecclose(env->pos);
			if (env->bestpos)
				vecclose(env->bestpos);
			if (env->mst)
				stkclose(env->mst);
			alloc(env->disc, env, 0);
		}
	}
}

/*
 * 20120528: regoff_t changed from int to ssize_t
 */

#undef	regnexec
#define regnexec	_ast_regnexec

extern int
regnexec(const regex_t* p, const char* s, size_t len, size_t nmatch, oldregmatch_t* oldmatch, regflags_t flags)
{
	if (oldmatch)
	{
		regmatch_t*	match;
		size_t		i;
		int		r;

		if (!(match = oldof(0, regmatch_t, nmatch, 0)))
			return -1;
		if (!(r = regnexec_20120528(p, s, len, nmatch, match, flags)))
			for (i = 0; i < nmatch; i++)
			{
				oldmatch[i].rm_so = (int)match[i].rm_so;
				oldmatch[i].rm_eo = (int)match[i].rm_eo;
			}
		free(match);
		return r;
	}
	return regnexec_20120528(p, s, len, 0, NULL, flags);
}

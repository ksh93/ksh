/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1990-2011 AT&T Intellectual Property          *
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
*                 Glenn Fowler <gsf@research.att.com>                  *
*                                                                      *
***********************************************************************/
#pragma prototyped
#pragma clang diagnostic ignored "-Wdeprecated-register"
#pragma clang diagnostic ignored "-Wparentheses"

/*
 * mamake -- MAM make
 *
 * coded for portability
 */

#define RELEASE_DATE "2021-01-21"
static char id[] = "\n@(#)$Id: mamake (ksh 93u+m) " RELEASE_DATE " $\0\n";

#if _PACKAGE_ast

#include <ast.h>
#include <error.h>

static const char usage[] =
"[-?\n@(#)$Id: mamake (ksh 93u+m) " RELEASE_DATE " $\n]"
"[-author?Glenn Fowler <gsf@research.att.com>]"
"[-copyright?(c) 1994-2012 AT&T Intellectual Property]"
"[-copyright?(c) 2020-2021 Contributors to https://github.com/ksh93/ksh]"
"[-license?http://www.eclipse.org/org/documents/epl-v10.html]"
"[+NAME?mamake - make abstract machine make]"
"[+DESCRIPTION?\bmamake\b reads \amake abstract machine\a target and"
"	prerequisite file descriptions from a mamfile (see \b-f\b) and executes"
"	actions to update targets that are older than their prerequisites."
"	Mamfiles are portable to environments that only have"
"	\bsh\b(1) and \bcc\b(1).]"
"[+?Mamfiles are used rather than"
"	old-\bmake\b makefiles because some features are not reliably supported"
"	across all \bmake\b variants:]{"
"		[+action execution?Multi-line actions are executed as a"
"			unit by \b$SHELL\b. There are some shell constructs"
"			that cannot be expressed in an old-\bmake\b makefile.]"
"		[+viewpathing?\bVPATH\b is properly interpreted. This allows"
"			source to be separate from generated files.]"
"		[+recursion?Ordered subdirectory recursion over unrelated"
"			makefiles.]"
"	}"
"[+?\bmamprobe\b(1) is called to probe and generate system specific variable"
"	definitions. The probe information is regenerated when it is older"
"	than the \bmamprobe\b command.]"
"[+?For compatibility with \bnmake\b(1) the \b-K\b option and the"
"	\brecurse\b and \bcc-*\b command line targets are ignored.]"
"[e:?Explain reason for triggering action. Ignored if -F is on.]"
"[f:?Read \afile\a instead of the default.]:[file:=Mamfile]"
"[i:?Ignore action errors.]"
"[k:?Continue after error with sibling prerequisites.]"
"[n:?Print actions but do not execute. Recursion actions (see \b-r\b) are still"
"	executed. Use \b-N\b to disable recursion actions too.]"
"[r:?Recursively make leaf directories matching \apattern\a. Only leaf"
"	directories containing a makefile named \bNmakefile\b, \bnmakefile\b,"
"	\bMakefile\b or \bmakefile\b are considered. The first makefile"
"	found in each leaf directory is scanned for leaf directory"
"	prerequisites; the recursion order is determined by a topological sort"
"	of these prerequisites.]:[pattern]"
"[C:?Do all work in \adirectory\a. All messages will mention"
"	\adirectory\a.]:[directory]"
"[D:?Set the debug trace level to \alevel\a. Higher levels produce more"
"	output.]#[level]"
"[F:?Force all targets to be out of date.]"
"[K:?Ignored.]"
"[N:?Like \b-n\b but recursion actions (see \b-r\b) are also disabled.]"
"[V:?Print the program version and exit.]"
"[G:debug-symbols?Compile and link with debugging symbol options enabled.]"
"[S:strip-symbols?Strip link-time static symbols from executables.]"

"\n"
"\n[ target ... ] [ name=value ... ]\n"
"\n"

"[+SEE ALSO?\bgmake\b(1), \bmake\b(1), \bmamprobe\b(1),"
"	\bnmake\b(1), \bsh\b(1)]"
;

#else

#define elementsof(x)	(sizeof(x)/sizeof(x[0]))
#define newof(p,t,n,x)	((p)?(t*)realloc((char*)(p),sizeof(t)*(n)+(x)):(t*)calloc(1,sizeof(t)*(n)+(x)))

#define NiL		((char*)0)

#endif

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#if !_PACKAGE_ast && defined(__STDC__)
#include <stdlib.h>
#include <string.h>
#endif

#define delimiter(c)	((c)==' '||(c)=='\t'||(c)=='\n'||(c)==';'||(c)=='('||(c)==')'||(c)=='`'||(c)=='|'||(c)=='&'||(c)=='=')

#define add(b,c)	(((b)->nxt >= (b)->end) ? append(b, "") : NiL, *(b)->nxt++ = (c))
#define get(b)		((b)->nxt-(b)->buf)
#define set(b,o)	((b)->nxt=(b)->buf+(o))
#define use(b)		(*(b)->nxt=0,(b)->nxt=(b)->buf)

#define CHUNK		4096
#define KEY(a,b,c,d)	((((unsigned long)(a))<<15)|(((unsigned long)(b))<<10)|(((unsigned long)(c))<<5)|(((unsigned long)(d))))
#define NOW		((unsigned long)time((time_t*)0))
#define ROTATE(p,l,r,t)	((t)=(p)->l,(p)->l=(t)->r,(t)->r=(p),(p)=(t))

#define RULE_active	0x0001		/* active target		*/
#define RULE_dontcare	0x0002		/* ok if not found		*/
#define RULE_error	0x0004		/* not found or not generated	*/
#define RULE_exists	0x0008		/* target file exists		*/
#define RULE_generated	0x0010		/* generated target		*/
#define RULE_ignore	0x0020		/* ignore time			*/
#define RULE_implicit	0x0040		/* implicit prerequisite	*/
#define RULE_made	0x0080		/* already made			*/
#define RULE_virtual	0x0100		/* not a file			*/

#define STREAM_KEEP	0x0001		/* don't fclose() on pop()	*/
#define STREAM_MUST	0x0002		/* push() file must exist	*/
#define STREAM_PIPE	0x0004		/* pclose() on pop()		*/

#ifndef S_IXUSR
#define S_IXUSR		0100		/* owner execute permission	*/
#endif
#ifndef S_IXGRP
#define S_IXGRP		0010		/* group execute permission	*/
#endif
#ifndef S_IXOTH
#define S_IXOTH		0001		/* other execute permission	*/
#endif

struct Rule_s;

typedef struct stat Stat_t;
typedef FILE Stdio_t;

typedef struct Buf_s			/* buffer stream		*/
{
	struct Buf_s*	old;		/* next dropped buffer		*/
	char*		end;		/* 1 past end of buffer		*/
	char*		nxt;		/* next char to add		*/
	char*		buf;		/* buffer space			*/
} Buf_t;

typedef struct Dict_item_s		/* dictionary item		*/
{
	struct Dict_item_s*	left;	/* left child			*/
	struct Dict_item_s*	right;	/* right child			*/
	void*			value;	/* user defined value		*/
	char			name[1];/* 0 terminated name		*/
} Dict_item_t;

typedef struct Dict_s			/* dictionary handle		*/
{
	Dict_item_t*	root;		/* root item			*/
} Dict_t;

typedef struct List_s			/* Rule_t list			*/
{
	struct List_s*	next;		/* next in list			*/
	struct Rule_s*	rule;		/* list item			*/
} List_t;

typedef struct Rule_s			/* rule item			*/
{
	char*		name;		/* unbound name			*/
	char*		path;		/* bound path			*/
	List_t*		prereqs;	/* prerequisites		*/
	struct Rule_s*	leaf;		/* recursion leaf alias		*/
	int		flags;		/* RULE_* flags			*/
	int		making;		/* currently make()ing		*/
	unsigned long	time;		/* modification time		*/
} Rule_t;

typedef struct Stream_s			/* input file stream stack	*/
{
	Stdio_t*	fp;		/* read stream			*/
	char*		file;		/* stream path			*/
	unsigned long	line;		/* stream line			*/
	int		flags;		/* stream flags			*/
} Stream_t;

typedef struct View_s			/* viewpath level		*/
{
	struct View_s*	next;		/* next level in viewpath	*/
	int		node;		/* viewpath node path length	*/
	char		dir[1];		/* viewpath level dir prefix	*/
} View_t;

static struct				/* program state		*/
{
	Buf_t*		buf;		/* work buffer			*/
	Buf_t*		old;		/* dropped buffers		*/
	Buf_t*		opt;		/* option buffer		*/

	Dict_t*		leaf;		/* recursion leaf dictionary	*/
	Dict_t*		libs;		/* library dictionary		*/
	Dict_t*		rules;		/* rule dictionary		*/
	Dict_t*		vars;		/* variable dictionary		*/

	View_t*		view;		/* viewpath levels		*/

	char*		directory;	/* work in this directory	*/
	char*		id;		/* command name			*/
	char*		file;		/* first input file		*/
	char*		pwd;		/* current directory		*/
	char*		recurse;	/* recursion pattern		*/
	char*		shell;		/* ${SHELL}			*/

	int		active;		/* targets currently active	*/
	int		debug;		/* negative of debug level	*/
	int		errors;		/* some error(s) occurred	*/
	int		exec;		/* execute actions		*/
	int		explain;	/* explain actions		*/
	int		force;		/* all targets out of date	*/
	int		ignore;		/* ignore command errors	*/
	int		indent;		/* debug indent			*/
	int		keepgoing;	/* do siblings on error		*/
	int		never;		/* never execute		*/
	int		peek;		/* next line already in input	*/
	int		probed;		/* probe already done		*/
	int		verified;	/* don't bother with verify()	*/

	Stream_t	streams[4];	/* input file stream stack	*/
	Stream_t*	sp;		/* input stream stack pointer	*/

	char		input[8*CHUNK];	/* input buffer			*/
} state;

static unsigned long	make(Rule_t*);

static char		mamfile[] = "Mamfile";
static char		sh[] = "/bin/sh";

extern char**		environ;

#if !_PACKAGE_ast

#if defined(NeXT) || defined(__NeXT)
#define getcwd(a,b)	getwd(a)
#endif

/*
 * emit usage message and exit
 */

static void
usage()
{
	fprintf(stderr, "Usage: %s [-iknFKNV] [-f mamfile] [-r pattern] [-C directory] [-D level] [target ...] [name=value ...]\n", state.id);
	exit(2);
}

#endif

/*
 * output error message identification
 */

static void
identify(Stdio_t* sp)
{
	if (state.directory)
		fprintf(sp, "%s [%s]: ", state.id, state.directory);
	else
		fprintf(sp, "%s: ", state.id);
}

/*
 * emit error message
 * level:
 *	<0	debug
 *	 0	info
 *	 1	warning
 *	 2	error
 *	>2	exit(level-2)
 */

static void
report(int level, char* text, char* item, unsigned long stamp)
{
	int	i;

	if (level >= state.debug)
	{
		if (level)
			identify(stderr);
		if (level < 0)
		{
			fprintf(stderr, "debug%d: ", level);
			for (i = 1; i < state.indent; i++)
				fprintf(stderr, "  ");
		}
		else
		{
			if (state.sp && state.sp->line)
			{
				if (state.sp->file)
					fprintf(stderr, "%s: ", state.sp->file);
				fprintf(stderr, "%ld: ", state.sp->line);
			}
			if (level == 1)
				fprintf(stderr, "warning: ");
			else if (level > 1)
				state.errors = 1;
		}
		if (item)
			fprintf(stderr, "%s: ", item);
		fprintf(stderr, "%s", text);
		if (stamp && state.debug <= -2)
			fprintf(stderr, " %10lu", stamp);
		fprintf(stderr, "\n");
		if (level > 2)
			exit(level - 2);
	}
}

/*
 * don't know how to make or exit code making
 */

static void
dont(Rule_t* r, int code, int keepgoing)
{
	identify(stderr);
	if (!code)
		fprintf(stderr, "don't know how to make %s\n", r->name);
	else
	{
		fprintf(stderr, "*** exit code %d making %s%s\n", code, r->name, state.ignore ? " ignored" : "");
		unlink(r->name);
		if (state.ignore)
			return;
	}
	if (!keepgoing)
		exit(1);
	state.errors++;
	r->flags |= RULE_error;
}

/*
 * local strrchr()
 */

static char*
last(register char* s, register int c)
{
	register char*	r = 0;

	for (r = 0; *s; s++)
		if (*s == c)
			r = s;
	return r;
}

/*
 * open a buffer stream
 */

static Buf_t*
buffer(void)
{
	register Buf_t*	buf;

	if (buf = state.old)
		state.old = state.old->old;
	else if (!(buf = newof(0, Buf_t, 1, 0)) || !(buf->buf = newof(0, char, CHUNK, 0)))
		report(3, "out of memory [buffer]", NiL, (unsigned long)0);
	buf->end = buf->buf + CHUNK;
	buf->nxt = buf->buf;
	return buf;
}

/*
 * close a buffer stream
 */

static void
drop(Buf_t* buf)
{
	buf->old = state.old;
	state.old = buf;
}

/*
 * append str length n to buffer and return the buffer base
 */

static char*
appendn(Buf_t* buf, char* str, int n)
{
	int	m;
	int	i;

	if ((n + 1) >= (buf->end - buf->nxt))
	{
		i = buf->nxt - buf->buf;
		m = (((buf->end - buf->buf) + n + CHUNK + 1) / CHUNK) * CHUNK;
		if (!(buf->buf = newof(buf->buf, char, m, 0)))
			report(3, "out of memory [buffer resize]", NiL, (unsigned long)0);
		buf->end = buf->buf + m;
		buf->nxt = buf->buf + i;
	}
	memcpy(buf->nxt, str, n + 1);
	buf->nxt += n;
	return buf->buf;
}

/*
 * append str to buffer and return the buffer base
 * if str==0 then next pointer reset to base
 */

static char*
append(Buf_t* buf, char* str)
{
	if (str)
		return appendn(buf, str, strlen(str));
	buf->nxt = buf->buf;
	return buf->buf;
}

/*
 * allocate space for s and return the copy
 */

static char*
duplicate(char* s)
{
	char*	t;
	int	n;

	n = strlen(s);
	if (!(t = newof(0, char, n, 1)))
		report(3, "out of memory [duplicate]", s, (unsigned long)0);
	strcpy(t, s);
	return t;
}

/*
 * open a new dictionary
 */

static Dict_t*
dictionary(void)
{
	Dict_t*	dict;

	if (!(dict = newof(0, Dict_t, 1, 0)))
		report(3, "out of memory [dictionary]", NiL, (unsigned long)0);
	return dict;
}

/*
 * return the value for item name in dictionary dict
 * if value!=0 then name entry value is created if necessary and set
 * uses top-down splaying (ala Tarjan and Sleator)
 */

static void*
search(register Dict_t* dict, char* name, void* value)
{
	register int		cmp;
	register Dict_item_t*	root;
	register Dict_item_t*	t;
	register Dict_item_t*	left;
	register Dict_item_t*	right;
	register Dict_item_t*	lroot;
	register Dict_item_t*	rroot;

	root = dict->root;
	left = right = lroot = rroot = 0;
	while (root)
	{
		if (!(cmp = strcmp(name, root->name)))
			break;
		else if (cmp < 0)
		{	
			if (root->left && (cmp = strcmp(name, root->left->name)) <= 0)
			{
				ROTATE(root, left, right, t);
				if (!cmp)
					break;
			}
			if (right)
				right->left = root;
			else
				rroot = root;
			right = root;
			root = root->left;
			right->left = 0;
		}
		else
		{	
			if (root->right && (cmp = strcmp(name, root->right->name)) >= 0)
			{
				ROTATE(root, right, left, t);
				if (!cmp)
					break;
			}
			if (left)
				left->right = root;
			else
				lroot = root;
			left = root;
			root = root->right;
			left->right = 0;
		}
	}
	if (root)
	{
		if (right)
			right->left = root->right;
		else
			rroot = root->right;
		if (left)
			left->right = root->left;
		else
			lroot = root->left;
	}
	else if (value)
	{
		if (!(root = newof(0, Dict_item_t, 1, strlen(name))))
			report(3, "out of memory [dictionary]", name, (unsigned long)0);
		strcpy(root->name, name);
	}
	if (root)
	{
		if (value)
			root->value = value;
		root->left = lroot;
		root->right = rroot;
		dict->root = root;
		return value ? (void*)root->name : root->value;
	}
	if (left)
	{
		left->right = rroot;
		dict->root = lroot;
	}
	else if (right)
	{
		right->left = lroot;
		dict->root = rroot;
	}
	return 0;
}

/*
 * low level for walk()
 */

static int
apply(Dict_t* dict, Dict_item_t* item, int (*func)(Dict_item_t*, void*), void* handle)
{
	register Dict_item_t*	right;

	do
	{
		right = item->right;
		if (item->left && apply(dict, item->left, func, handle))
			return -1;
		if ((*func)(item, handle))
			return -1;
	} while (item = right);
	return 0;
}

/*
 * apply func to each dictionary item
 */

static int
walk(Dict_t* dict, int (*func)(Dict_item_t*, void*), void* handle)
{
	return dict->root ? apply(dict, dict->root, func, handle) : 0;
}

/*
 * return a rule pointer for name
 */

static Rule_t*
rule(char* name)
{
	Rule_t*	r;

	if (!(r = (Rule_t*)search(state.rules, name, NiL)))
	{
		if (!(r = newof(0, Rule_t, 1, 0)))
			report(3, "out of memory [rule]", name, (unsigned long)0);
		r->name = (char*)search(state.rules, name, (void*)r);
	}
	return r;
}

/*
 * prepend p onto rule r prereqs
 */

static void
cons(Rule_t* r, Rule_t* p)
{
	register List_t*	x;

	for (x = r->prereqs; x && x->rule != p; x = x->next);
	if (!x)
	{
		if (!(x = newof(0, List_t, 1, 0)))
			report(3, "out of memory [list]", r->name, (unsigned long)0);
		x->rule = p;
		x->next = r->prereqs;
		r->prereqs = x;
	}
}

/*
 * initialize the viewpath
 */

static void
view(void)
{
	register char*		s;
	register char*		t;
	register char*		p;
	register View_t*	vp;

	View_t*			zp;
	int			c;
	int			n;

	Stat_t			st;
	Stat_t			ts;

	char			buf[CHUNK];

	if (stat(".", &st))
		report(3, "cannot stat", ".", (unsigned long)0);
	if ((s = (char*)search(state.vars, "PWD", NiL)) && !stat(s, &ts) &&
	    ts.st_dev == st.st_dev && ts.st_ino == st.st_ino)
		state.pwd = s;
	if (!state.pwd)
	{
		if (!getcwd(buf, sizeof(buf) - 1))
			report(3, "cannot determine PWD", NiL, (unsigned long)0);
		state.pwd = duplicate(buf);
		search(state.vars, "PWD", state.pwd);
	}
	if ((s = (char*)search(state.vars, "VPATH", NiL)) && *s)
	{
		zp = 0;
		for (;;)
		{
			for (t = s; *t && *t != ':'; t++);
			if (c = *t)
				*t = 0;
			if (!state.view)
			{
				/*
				 * determine the viewpath offset
				 */

				if (stat(s, &st))
					report(3, "cannot stat top view", s, (unsigned long)0);
				if (stat(state.pwd, &ts))
					report(3, "cannot stat", state.pwd, (unsigned long)0);
				if (ts.st_dev == st.st_dev && ts.st_ino == st.st_ino)
					p = ".";
				else
				{
					p = state.pwd + strlen(state.pwd);
					while (p > state.pwd)
						if (*--p == '/')
						{
							if (p == state.pwd)
								report(3, ". not under VPATH", s, (unsigned long)0);
							*p = 0;
							if (stat(state.pwd, &ts))
								report(3, "cannot stat", state.pwd, (unsigned long)0);
							*p = '/';
							if (ts.st_dev == st.st_dev && ts.st_ino == st.st_ino)
							{
								p++;
								break;
							}
						}
					if (p <= state.pwd)
						report(3, "cannot determine viewpath offset", s, (unsigned long)0);
				}
			}
			n = strlen(s);
			if (!(vp = newof(0, View_t, 1, strlen(p) + n + 1)))
				report(3, "out of memory [view]", s, (unsigned long)0);
			vp->node = n + 1;
			strcpy(vp->dir, s);
			*(vp->dir + n) = '/';
			strcpy(vp->dir + n + 1, p);
			report(-4, vp->dir, "view", (unsigned long)0);
			if (!state.view)
				state.view = zp = vp;
			else
				zp = zp->next = vp;
			if (!c)
				break;
			*t++ = c;
			s = t;
		}
	}
}

/*
 * return next '?' or '}' in nested '}'
 */

static char*
cond(register char* s)
{
	register int	n;

	if (*s == '?')
		s++;
	n = 0;
	for (;;)
	{
		switch (*s++)
		{
		case 0:
			break;
		case '{':
			n++;
			continue;
		case '}':
			if (!n--)
				break;
			continue;
		case '?':
			if (!n)
				break;
			continue;
		default:
			continue;
		}
		break;
	}
	return s - 1;
}

/*
 * expand var refs from s into buf
 */

static void
substitute(Buf_t* buf, register char* s)
{
	register char*	t;
	register char*	v;
	register char*	q;
	register char*	b;
	register int	c;
	register int	n;
	int		a = 0;
	int		i;

	while (c = *s++)
	{
		if (c == '$' && *s == '{')
		{
			b = s - 1;
			i = 1;
			for (n = *(t = ++s) == '-' ? 0 : '-'; (c = *s) && c != '?' && c != '+' && c != n && c != ':' && c != '=' && c != '[' && c != '}'; s++)
				if (!isalnum(c) && c != '_')
					i = 0;
			*s = 0;
			if (c == '[')
			{
				append(buf, b);
				*s = c;
				continue;
			}
			v = (char*)search(state.vars, t, NiL);
			if ((c == ':' || c == '=') && (!v || c == ':' && !*v))
			{
				append(buf, b);
				*s = c;
				continue;
			}
			if (t[0] == 'A' && t[1] == 'R' && t[2] == 0)
				a = 1;
			*s = c;
			if (c && c != '}')
			{
				n = 1;
				for (t = ++s; *s; s++)
					if (*s == '{')
						n++;
					else if (*s == '}' && !--n)
						break;
			}
			switch (c)
			{
			case '?':
				q = cond(t - 1);
				if (v)
				{
					if (((q - t) != 1 || *t != '*') && strncmp(v, t, q - t))
						v = 0;
				}
				else if (q == t)
					v = s;
				t = cond(q);
				if (v)
				{
					if (t > q)
					{
						c = *t;
						*t = 0;
						substitute(buf, q + 1);
						*t = c;
					}
				}
				else
				{
					q = cond(t);
					if (q > t)
					{
						c = *q;
						*q = 0;
						substitute(buf, t + 1);
						*q = c;
					}
				}
				break;
			case '+':
			case '-':
				if ((v == 0 || *v == 0) == (c == '-'))
				{
					c = *s;
					*s = 0;
					substitute(buf, t);
					*s = c;
					break;
				}
				if (c != '-')
					break;
				/* FALLTHROUGH */
			case 0:
			case '=':
			case '}':
				if (v)
				{
					if (a && t[0] == 'm' && t[1] == 'a' && t[2] == 'm' && t[3] == '_' && t[4] == 'l' && t[5] == 'i' && t[6] == 'b')
					{
						for (t = v; *t == ' '; t++);
						for (; *t && *t != ' '; t++);
						if (*t)
							*t = 0;
						else
							t = 0;
						substitute(buf, v);
						if (t)
							*t = ' ';
					}
					else
						substitute(buf, v);
				}
				else if (i)
				{
					c = *s;
					*s = 0;
					append(buf, b);
					*s = c;
					continue;
				}
				break;
			}
			if (*s)
				s++;
		}
		else
			add(buf, c);
	}
}

/*
 * expand var refs from s into buf and return buf base
 */

static char*
expand(Buf_t* buf, char* s)
{
	substitute(buf, s);
	return use(buf);
}

/*
 * stat() with .exe check
 */

static char*
status(Buf_t* buf, int off, char* path, struct stat* st)
{
	int		r;
	char*		s;
	Buf_t*		tmp;

	if (!stat(path, st))
		return path;
	if (!(tmp = buf))
	{
		tmp = buffer();
		off = 0;
	}
	if (off)
		set(tmp, off);
	else
		append(tmp, path);
	append(tmp, ".exe");
	s = use(tmp);
	r = stat(s, st);
	if (!buf)
	{
		drop(tmp);
		s = path;
	}
	if (r)
	{
		if (off)
			s[off] = 0;
		s = 0;
	}
	return s;
}

/*
 * return path to file
 */

static char*
find(Buf_t* buf, char* file, struct stat* st)
{
	char*		s;
	View_t*		vp;
	int		node;
	int		c;
	int		o;

	if (s = status(buf, 0, file, st))
	{
		report(-3, s, "find", (unsigned long)0);
		return s;
	}
	if (vp = state.view)
	{
		node = 0;
		if (*file == '/')
		{
			do
			{
				if (!strncmp(file, vp->dir, vp->node))
				{
					file += vp->node;
					node = 2;
					break;
				}
			} while (vp = vp->next);
		}
		else
			vp = vp->next;
		if (vp)
			do
			{
				if (node)
				{
					c = vp->dir[vp->node];
					vp->dir[vp->node] = 0;
					append(buf, vp->dir);
					vp->dir[vp->node] = c;
				}
				else
				{
					append(buf, vp->dir);
					append(buf, "/");
				}
				append(buf, file);
				o = get(buf);
				s = use(buf);
				if (s = status(buf, o, s, st))
				{
					report(-3, s, "find", (unsigned long)0);
					return s;
				}
			} while (vp = vp->next);
	}
	return 0;
}

/*
 * bind r to a file and return the modify time
 */

static unsigned long
bind(Rule_t* r)
{
	char*		s;
	Buf_t*		buf;
	struct stat	st;

	buf = buffer();
	if (s = find(buf, r->name, &st))
	{
		if (s != r->name)
			r->path = duplicate(s);
		r->time = st.st_mtime;
		r->flags |= RULE_exists;
	}
	drop(buf);
	return r->time;
}

/*
 * pop the current input file
 */

static int
pop(void)
{
	int	r;

	if (!state.sp)
		report(3, "input stack underflow", NiL, (unsigned long)0);
	if (!state.sp->fp || (state.sp->flags & STREAM_KEEP))
		r = 0;
	else if (state.sp->flags & STREAM_PIPE)
		r = pclose(state.sp->fp);
	else
		r = fclose(state.sp->fp);
	if (state.sp == state.streams)
		state.sp = 0;
	else
		state.sp--;
	return r;
}

/*
 * push file onto the input stack
 */

static int
push(char* file, Stdio_t* fp, int flags)
{
	char*		path;
	Buf_t*		buf;
	struct stat	st;

	if (!state.sp)
		state.sp = state.streams;
	else if (++state.sp >= &state.streams[elementsof(state.streams)])
		report(3, "input stream stack overflow", NiL, (unsigned long)0);
	if (state.sp->fp = fp)
	{
		if(state.sp->file)
			free(state.sp->file);
		state.sp->file = strdup("pipeline");
		if(!state.sp->file)
			report(3, "out of memory [push]", NiL, (unsigned long)0);
	}
	else if (flags & STREAM_PIPE)
		report(3, "pipe error", file, (unsigned long)0);
	else if (!file || !strcmp(file, "-") || !strcmp(file, "/dev/stdin"))
	{
		flags |= STREAM_KEEP;
		if(state.sp->file)
			free(state.sp->file);
		state.sp->file = strdup("/dev/stdin");
		if(!state.sp->file)
			report(3, "out of memory [push]", NiL, (unsigned long)0);
		state.sp->fp = stdin;
	}
	else
	{
		buf = buffer();
		if (path = find(buf, file, &st))
		{
			if (!(state.sp->fp = fopen(path, "r")))
				report(3, "cannot read", path, (unsigned long)0);
			if(state.sp->file)
				free(state.sp->file);
			state.sp->file = duplicate(path);
			drop(buf);
		}
		else
		{
			drop(buf);
			pop();
			if (flags & STREAM_MUST)
				report(3, "not found", file, (unsigned long)0);
			return 0;
		}
	}
	state.sp->flags = flags;
	state.sp->line = 0;
	return 1;
}

/*
 * return the next input line
 */

static char*
input(void)
{
	char*	e;

	if (!state.sp)
		report(3, "no input file stream", NiL, (unsigned long)0);
	if (state.peek)
		state.peek = 0;
	else if (!fgets(state.input, sizeof(state.input), state.sp->fp))
		return 0;
	else if (*state.input && *(e = state.input + strlen(state.input) - 1) == '\n')
		*e = 0;
	state.sp->line++;
	e = state.input;
	while (isspace(*e))
		e++;	/* allow indentation */
	return e;
}

/*
 * pass shell action s to ${SHELL:-/bin/sh}
 * the -c wrapper ensures that scripts are run in the selected shell
 * even on systems that otherwise demand #! magic (can you say Cygwin)
 */

static int
execute(register char* s)
{
	register int	c;
	Buf_t*		buf;

	if (!state.shell && (!(state.shell = (char*)search(state.vars, "SHELL", NiL)) || !strcmp(state.shell, sh)))
		state.shell = sh;
	buf = buffer();
	append(buf, state.shell);
	append(buf, " -c '");
	while (c = *s++)
	{
		if (c == '\'')
		{
			add(buf, c);
			for (s--; *s == c; s++)
			{
				add(buf, '\\');
				add(buf, c);
			} 
		}
		add(buf, c);
	}
	add(buf, '\'');
	s = use(buf);
	report(-5, s, "exec", (unsigned long)0);
	if ((c = system(s)) > 255)
		c >>= 8;
	drop(buf);
	return c;
}

/*
 * run action s to update r
 */

static unsigned long
run(Rule_t* r, register char* s)
{
	register Rule_t*	q;
	register char*		t;
	register int		c;
	register View_t*	v;
	int			i;
	int			j;
	int			x;
	Stat_t			st;
	Buf_t*			buf;

	if (r->flags & RULE_error)
		return r->time;
	buf = buffer();
	if (!strncmp(s, "mamake -r ", 10))
	{
		state.verified = 1;
		x = !state.never;
	}
	else
		x = state.exec;
	if (x)
		append(buf, "trap - 1 2 3 15\nPATH=.:$PATH\nset -x\n");
	if (state.view)
	{
		do
		{
			for (; delimiter(*s); s++)
				add(buf, *s);
			for (t = s; *s && !delimiter(*s); s++);
			c = *s;
			*s = 0;
			if (c == '=')
			{
				append(buf, t);
				continue;
			}
			if ((q = (Rule_t*)search(state.rules, t, NiL)) && q->path && !(q->flags & RULE_generated))
				append(buf, q->path);
			else
			{
				append(buf, t);
				if (*t == '-' && *(t + 1) == 'I' && (*(t + 2) || c))
				{
					if (*(t + 2))
						i = 2;
					else
					{
						for (i = 3; *(t + i) == ' ' || *(t + i) == '\t'; i++);
						*s = c;
						for (s = t + i; *s && *s != ' ' && *s != '\t' && *s != '\n'; s++);
						c = *s;
						*s = 0;
						append(buf, t + 2);
					}
					if (*(t + i) && *(t + i) != '/')
					{
						v = state.view;
						while (v = v->next)
						{
							add(buf, ' ');
							for (j = 0; j < i; j++)
								add(buf, *(t + j));
							append(buf, v->dir);
							if (*(t + i) != '.' || *(t + i + 1))
							{
								add(buf, '/');
								append(buf, t + i);
							}
						}
					}
				}
			}
		} while (*s = c);
		s = use(buf);
	}
	else if (x)
	{
		append(buf, s);
		s = use(buf);
	}
	if (x)
	{
		if (c = execute(s))
			dont(r, c, state.keepgoing);
		if (status((Buf_t*)0, 0, r->name, &st))
		{
			r->time = st.st_mtime;
			r->flags |= RULE_exists;
		}
		else
			r->time = NOW;
	}
	else
	{
		fprintf(stdout, "%s\n", s);
		if (state.debug)
			fflush(stdout);
		r->time = NOW;
		r->flags |= RULE_exists;
	}
	drop(buf);
	return r->time;
}

/*
 * return the full path for s using buf workspace
 */

static char*
path(Buf_t* buf, char* s, int must)
{
	register char*	p;
	register char*	d;
	register char*	x;
	char*		e;
	register int	c;
	int		t;
	int		o;
	Stat_t		st;

	for (e = s; *e && *e != ' ' && *e != '\t'; e++);
	t = *e;
	if ((x = status(buf, 0, s, &st)) && (st.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)))
		return x;
	if (!(p = (char*)search(state.vars, "PATH", NiL)))
		report(3, "variable not defined", "PATH", (unsigned long)0);
	do
	{
		for (d = p; *p && *p != ':'; p++);
		c = *p;
		*p = 0;
		if (*d && (*d != '.' || *(d + 1)))
		{
			append(buf, d);
			add(buf, '/');
		}
		*p = c;
		if (t)
			*e = 0;
		append(buf, s);
		if (t)
			*e = t;
		o = get(buf);
		x = use(buf);
		if ((x = status(buf, o, x, &st)) && (st.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)))
			return x;
	} while (*p++);
	if (must)
		report(3, "command not found", s, (unsigned long)0);
	return 0;
}

/*
 * generate (if necessary) and read the MAM probe information
 * done on the first `setv CC ...'
 */

static void
probe(void)
{
	register char*	cc;
	register char*	s;
	unsigned long	h;
	unsigned long	q;
	Buf_t*		buf;
	Buf_t*		pro;
	Buf_t*		tmp;
	struct stat	st;

	static char	let[] = "ABCDEFGHIJKLMNOP";
	static char	cmd[] = "mamprobe";

	if (!(cc = (char*)search(state.vars, "CC", NiL)))
		cc = "cc";
	buf = buffer();
	s = path(buf, cmd, 1);
	q = stat(s, &st) ? (unsigned long)0 : (unsigned long)st.st_mtime;
	pro = buffer();
	s = cc = path(pro, cc, 1);
	for (h = 0; *s; s++)
		h = h * 0x63c63cd9L + *s + 0x9c39c33dL;
	if (!(s = (char*)search(state.vars, "INSTALLROOT", NiL)))
		report(3, "variable must be defined", "INSTALLROOT", (unsigned long)0);
	append(buf, s);
	append(buf, "/lib/probe/C/mam/");
	for (h &= 0xffffffffL; h; h >>= 4)
		add(buf, let[h & 0xf]);
	s = use(buf);
	h = stat(s, &st) ? (unsigned long)0 : (unsigned long)st.st_mtime;
	if (h < q || !push(s, (Stdio_t*)0, 0))
	{
		tmp = buffer();
		append(tmp, cmd);
		add(tmp, ' ');
		append(tmp, s);
		add(tmp, ' ');
		append(tmp, cc);
		if (execute(use(tmp)))
			report(3, "cannot generate probe info", s, (unsigned long)0);
		drop(tmp);
		if (!push(s, (Stdio_t*)0, 0))
			report(3, "cannot read probe info", s, (unsigned long)0);
	}
	drop(pro);
	drop(buf);
	make(rule(""));
	pop();
}

/*
 * add attributes in s to r
 */

static void
attributes(register Rule_t* r, register char* s)
{
	register char*	t;
	register int	n;

	for (;;)
	{
		for (; *s == ' '; s++);
		for (t = s; *s && *s != ' '; s++);
		if (!(n = s - t))
			break;
		switch (*t)
		{
		case 'd':
			if (n == 8 && !strncmp(t, "dontcare", n))
				r->flags |= RULE_dontcare;
			break;
		case 'g':
			if (n == 9 && !strncmp(t, "generated", n))
				r->flags |= RULE_generated;
			break;
		case 'i':
			if (n == 6 && !strncmp(t, "ignore", n))
				r->flags |= RULE_ignore;
			else if (n == 8 && !strncmp(t, "implicit", n))
				r->flags |= RULE_implicit;
			break;
		case 'v':
			if (n == 7 && !strncmp(t, "virtual", n))
				r->flags |= RULE_virtual;
			break;
		}
	}
}

/*
 * define ${mam_libX} for library reference lib
 */

static char*
require(char* lib, int dontcare)
{
	register int	c;
	char*		s;
	char*		r;
	FILE*		f;
	Buf_t*		buf;
	Buf_t*		tmp;
	struct stat	st;

	int		tofree = 0;
	static int	dynamic = -1;

	if (dynamic < 0)
		dynamic = (s = search(state.vars, "mam_cc_L", NiL)) ? atoi(s) : 0;
	if (!(r = search(state.vars, lib, NiL)))
	{
		buf = buffer();
		tmp = buffer();
		s = 0;
		for (;;)
		{
			if (s)
				append(buf, s);
			if (r = search(state.vars, "mam_cc_PREFIX_ARCHIVE", NiL))
				append(buf, r);
			append(buf, lib + 2);
			if (r = search(state.vars, "mam_cc_SUFFIX_ARCHIVE", NiL))
				append(buf, r);
			r = expand(tmp, use(buf));
			if (!stat(r, &st))
				break;
			if (s)
			{
				r = lib;
				break;
			}
			s = "${INSTALLROOT}/lib/";
			if (dynamic)
			{
				append(buf, s);
				if (r = search(state.vars, "mam_cc_PREFIX_SHARED", NiL))
					append(buf, r);
				append(buf, lib + 2);
				if (r = search(state.vars, "mam_cc_SUFFIX_SHARED", NiL))
					append(buf, r);
				r = expand(tmp, use(buf));
				if (!stat(r, &st))
				{
					r = lib;
					break;
				}
			}
		}
		if (r != lib)
		{
			tofree = 1;
			r = duplicate(r);
		}
		search(state.vars, lib, r);
		append(tmp, lib + 2);
		append(tmp, ".req");
		if (!(f = fopen(use(tmp), "r")))
		{
			append(tmp, "${INSTALLROOT}/lib/lib/");
			append(tmp, lib + 2);
			f = fopen(expand(buf, use(tmp)), "r");
		}
		if (f)
		{
			for (;;)
			{
				while ((c = fgetc(f)) == ' ' || c == '\t' || c == '\n');
				if (c == EOF)
					break;
				do
				{
					add(tmp, c);
				} while ((c = fgetc(f)) != EOF && c != ' ' && c != '\t' && c != '\n');
				s = use(tmp);
				if (s[0] && (s[0] != '-' || s[1]))
				{
					add(buf, ' ');
					append(buf, require(s, 0));
				}
			}
			fclose(f);
			if(tofree)
				free(r);
			r = use(buf);
		}
		else if (dontcare)
		{
			append(tmp, "set -\n");
			append(tmp, "cd \"${TMPDIR:-/tmp}\"\n");
			append(tmp, "echo 'int main(){return 0;}' > x.${!-$$}.c\n");
			append(tmp, "${CC} ${CCFLAGS} -o x.${!-$$}.x x.${!-$$}.c ");
			append(tmp, r);
			append(tmp, " >/dev/null 2>&1\n");
			append(tmp, "c=$?\n");
			append(tmp, "rm -f x.${!-$$}.[cox]\n");
			append(tmp, "exit $c\n");
			if (execute(expand(buf, use(tmp))))
			{
				if(tofree)
					free(r);
				r = "";
			}
		}
		r = duplicate(r);
		search(state.vars, lib, r);
		append(tmp, "mam_lib");
		append(tmp, lib + 2);
		search(state.vars, use(tmp), r);
		drop(tmp);
		drop(buf);
	}
	return r;
}

/*
 * input() until `done r'
 */

static unsigned long
make(Rule_t* r)
{
	register char*		s;
	register char*		t;
	register char*		u;
	register char*		v;
	register Rule_t*	q;
	unsigned long		z;
	unsigned long		x;
	Buf_t*			buf;
	Buf_t*			cmd;

	r->making++;
	if (r->flags & RULE_active)
		state.active++;
	if (*r->name)
	{
		z = bind(r);
		state.indent++;
		report(-1, r->name, "make", r->time);
	}
	else
		z = 0;
	buf = buffer();
	cmd = 0;
	while (s = input())
	{
		for (; *s == ' '; s++);
		for (; isdigit(*s); s++);
		for (; *s == ' '; s++);
		for (u = s; *s && *s != ' '; s++);
		if (*s)
		{
			for (*s++ = 0; *s == ' '; s++);
			for (t = s; *s && *s != ' '; s++);
			if (*s)
				for (*s++ = 0; *s == ' '; s++);
			v = s;
		}
		else
			t = v = s;
		switch (KEY(u[0], u[1], u[2], u[3]))
		{
		case KEY('b','i','n','d'):
			if ((t[0] == '-' || t[0] == '+') && t[1] == 'l' && (s = require(t, !strcmp(v, "dontcare"))) && strncmp(r->name, "FEATURE/", 8) && strcmp(r->name, "configure.h"))
				for (;;)
				{
					for (t = s; *s && *s != ' '; s++);
					if (*s)
						*s = 0;
					else
						s = 0;
					if (*t)
					{
						q = rule(expand(buf, t));
						attributes(q, v);
						x = bind(q);
						if (z < x)
							z = x;
						if (q->flags & RULE_error)
							r->flags |= RULE_error;
					}
					if (!s)
						break;
					for (*s++ = ' '; *s == ' '; s++);
				}
			continue;
		case KEY('d','o','n','e'):
			q = rule(expand(buf, t));
			if (q != r)
				report(2, "improper done statement", t, (unsigned long)0);
			attributes(r, v);
			if (cmd && state.active && (state.force || r->time < z || !r->time && !z))
			{
				if (state.explain && !state.force)
				{
					if (!r->time)
						fprintf(stderr, "%s [not found]\n", r->name);
					else
						fprintf(stderr, "%s [%lu] older than prerequisites [%lu]\n", r->name, r->time, z);
				}
				substitute(buf, use(cmd));
				x = run(r, use(buf));
				if (z < x)
					z = x;
			}
			r->flags |= RULE_made;
			if (!(r->flags & (RULE_dontcare|RULE_error|RULE_exists|RULE_generated|RULE_implicit|RULE_virtual)))
				dont(r, 0, state.keepgoing);
			break;
		case KEY('e','x','e','c'):
			r->flags |= RULE_generated;
			if (r->path)
			{
				free(r->path);
				r->path = 0;
				r->time = 0;
			}
			if (state.active)
			{
				if (cmd)
					add(cmd, '\n');
				else
					cmd = buffer();
				append(cmd, v);
			}
			continue;
		case KEY('m','a','k','e'):
			q = rule(expand(buf, t));
			if (!q->making)
			{
				attributes(q, v);
				x = make(q);
				if (!(q->flags & RULE_ignore) && z < x)
					z = x;
				if (q->flags & RULE_error)
					r->flags |= RULE_error;
			}
			continue;
		case KEY('p','r','e','v'):
			q = rule(expand(buf, t));
			if (!q->making)
			{
				if (!(q->flags & RULE_ignore) && z < q->time)
					z = q->time;
				if (q->flags & RULE_error)
					r->flags |= RULE_error;
				state.indent++;
				report(-2, q->name, "prev", q->time);
				state.indent--;
			}
			continue;
		case KEY('s','e','t','v'):
			if (!search(state.vars, t, NiL))
			{
				if (*v == '"')
				{
					s = v + strlen(v) - 1;
					if (*s == '"')
					{
						*s = 0;
						v++;
					}
				}
				search(state.vars, t, duplicate(expand(buf, v)));
			}
			if (!state.probed && t[0] == 'C' && t[1] == 'C' && !t[2])
			{
				state.probed = 1;
				probe();
			}
			continue;
		default:
			continue;
		}
		break;
	}
	drop(buf);
	if (cmd)
		drop(cmd);
	if (*r->name)
	{
		report(-1, r->name, "done", z);
		state.indent--;
	}
	if (r->flags & RULE_active)
		state.active--;
	r->making--;
	return r->time = z;
}

/*
 * verify that active targets were made
 */

static int
verify(Dict_item_t* item, void* handle)
{
	Rule_t*	r = (Rule_t*)item->value;

	if ((r->flags & (RULE_active|RULE_error|RULE_made)) == RULE_active)
		dont(r, 0, 1);
	return 0;
}

/*
 * return 1 if name is an initializer
 */

static int
initializer(char* name)
{
	register char*	s;

	if (s = last(name, '/'))
		s++;
	else
		s = name;
	return s[0] == 'I' && s[1] == 'N' && s[2] == 'I' && s[3] == 'T';
}

/*
 * update recursion leaf r and its prerequisites
 */

static int
update(register Rule_t* r)
{
	register List_t*	x;
	Buf_t*			buf;

	static char		cmd[] = "${MAMAKE} -C ";
	static char		arg[] = " ${MAMAKEARGS}";

	r->flags |= RULE_made;
	if (r->leaf)
		r->leaf->flags |= RULE_made;
	for (x = r->prereqs; x; x = x->next)
		if (x->rule->leaf && !(x->rule->flags & RULE_made))
			update(x->rule);
	buf = buffer();
	substitute(buf, cmd);
	append(buf, r->name);
	substitute(buf, arg);
	run(r, use(buf));
	drop(buf);
	return 0;
}

/*
 * scan makefile prereqs
 */

static int
scan(Dict_item_t* item, void* handle)
{
	register Rule_t*	r = (Rule_t*)item->value;
	register char*		s;
	register char*		t;
	register char*		u;
	register char*		w;
	Rule_t*			q;
	int			i;
	int			j;
	int			k;
	int			p;
	Buf_t*			buf;

	static char*		files[] =
				{
					"Mamfile"
				/* ksh 93u+m no longer uses these:
				 *	"Nmakefile",
				 *	"nmakefile",
				 *	"Makefile",
				 *	"makefile"
				 */
				};

	/*
	 * drop non-leaf rules
	 */

	if (!r->leaf)
		return 0;

	/*
	 * always make initializers
	 */

	if (initializer(r->name))
	{
		if (!(r->flags & RULE_made))
			update(r);
		return 0;
	}
	buf = buffer();
	for (i = 0; i < elementsof(files); i++)
	{
		append(buf, r->name);
		add(buf, '/');
		append(buf, files[i]);
		if (push(use(buf), (Stdio_t*)0, 0))
		{
			while (s = input())
			{
				j = p = 0;
				while (*s)
				{
					for (k = 1; (i = *s) == ' ' || i == '\t' || i == '"' || i == '\''; s++);
					for (t = s; (i = *s) && i != ' ' && i != '\t' && i != '"' && i != '\'' && i != '\\' && i != ':'; s++)
						if (i == '/')
							t = s + 1;
						else if (i == '.' && *(s + 1) != 'c' && *(s + 1) != 'C' && *(s + 1) != 'h' && *(s + 1) != 'H' && t[0] == 'l' && t[1] == 'i' && t[2] == 'b')
							*s = 0;
					if (*s)
						*s++ = 0;
					if (!t[0])
						k = 0;
					else if ((t[0] == '-' || t[0] == '+') && t[1] == 'l' && t[2])
					{
						append(buf, "lib");
						append(buf, t + 2);
						t = use(buf);
					}
					else if (p)
					{
						if (t[0] == '+' && !t[1])
							p = 2;
						else if (p == 1)
						{
							if (i != ':' || strncmp(s, "command", 7))
							{
								append(buf, "lib");
								append(buf, t);
								t = use(buf);
							}
							if (i == ':')
								while (*s && (*s == ' ' || *s == '\t'))
									s++;
						}
					}
					else if (i == ':')
					{
						if (j != ':' || !isupper(*t))
							k = 0;
						else if (!strcmp(t, "PACKAGE"))
						{
							p = 1;
							k = 0;
						}
						else
							for (u = t; *u; u++)
								if (isupper(*u))
									*u = tolower(*u);
								else if (!isalnum(*u))
								{
									k = 0;
									break;
								}
					}
					else if (t[0] != 'l' || t[1] != 'i' || t[2] != 'b')
						k = 0;
					else
						for (u = t + 3; *u; u++)
							if (!isalnum(*u))
							{
								k = 0;
								break;
							}
					if (k && ((q = (Rule_t*)search(state.leaf, t, NiL)) && q != r || *t++ == 'l' && *t++ == 'i' && *t++ == 'b' && *t && (q = (Rule_t*)search(state.leaf, t, NiL)) && q != r))
					{
						for (t = w = r->name; *w; w++)
							if (*w == '/')
								t = w + 1;
						if (t[0] == 'l' && t[1] == 'i' && t[2] == 'b')
							t += 3;
						for (u = w = q->name; *w; w++)
							if (*w == '/')
								u = w + 1;
						if (strcmp(t, u))
							cons(r, q);
					}
					j = i;
				}
			}
			pop();
			for (s = 0, w = r->name; *w; w++)
				if (*w == '/')
					s = w;
			if (s)
			{
				if ((s - r->name) > 3 && *(s - 1) == 'b' && *(s - 2) == 'i' && *(s - 3) == 'l' && *(s - 4) != '/')
				{
					/*
					 * foolib : foo : libfoo
					 */

					*(s - 3) = 0;
					q = (Rule_t*)search(state.leaf, r->name, NiL);
					if (q && q != r)
						cons(r, q);
					for (t = w = r->name; *w; w++)
						if (*w == '/')
							t = w + 1;
					append(buf, "lib");
					append(buf, t);
					q = (Rule_t*)search(state.leaf, use(buf), NiL);
					if (q && q != r)
						cons(r, q);
					*(s - 3) = 'l';
				}
				else if (((s - r->name) != 3 || *(s - 1) != 'b' || *(s - 2) != 'i' || *(s - 3) != 'l') && (*(s + 1) != 'l' || *(s + 2) != 'i' || *(s + 3) != 'b'))
				{
					/*
					 * huh/foobar : lib/libfoo
					 */

					s++;
					t = s + strlen(s);
					while (--t > s)
					{
						append(buf, "lib/lib");
						appendn(buf, s, t - s);
						q = (Rule_t*)search(state.leaf, use(buf), NiL);
						if (q && q != r)
							cons(r, q);
					}
				}
			}
			break;
		}
	}
	drop(buf);
	return 0;
}

/*
 * descend into op and its prereqs
 */

static int
descend(Dict_item_t* item, void* handle)
{
	Rule_t*	r = (Rule_t*)item->value;

	if (!state.active && (!(r->flags & RULE_active) || !(r = (Rule_t*)search(state.leaf, r->name, NiL))))
		return 0;
	return r->leaf && !(r->flags & RULE_made) ? update(r) : 0;
}

/*
 * append the non-leaf active targets to state.opt
 */

static int
active(Dict_item_t* item, void* handle)
{
	Rule_t*	r = (Rule_t*)item->value;

	if (r->flags & RULE_active)
	{
		if (r->leaf || search(state.leaf, r->name, NiL))
			state.active = 0;
		else
		{
			add(state.opt, ' ');
			append(state.opt, r->name);
		}
	}
	return 0;
}

/*
 * recurse on mamfiles in subdirs matching pattern
 */

static int
recurse(char* pattern)
{
	register char*	s;
	register char*	t;
	Rule_t*		r;
	Buf_t*		buf;
	Buf_t*		tmp;
	struct stat	st;

	/*
	 * first determine the MAM subdirs
	 */

	tmp = buffer();
	buf = buffer();
	state.exec = !state.never;
	state.leaf = dictionary();
	append(buf, "ls -d ");
	append(buf, pattern);
	s = use(buf);
	push("recurse", popen(s, "r"), STREAM_PIPE);
	while (s = input())
	{
		append(buf, s);
		add(buf, '/');
		append(buf, mamfile);
		if (find(tmp, use(buf), &st))
		{
			r = rule(s);
			if (t = last(r->name, '/'))
				t++;
			else
				t = r->name;
			r->leaf = rule(t);
			search(state.leaf, t, r);
		}
	}
	pop();
	drop(buf);
	drop(tmp);

	/*
	 * grab the non-leaf active targets
	 */

	if (!state.active)
	{
		state.active = 1;
		walk(state.rules, active, NiL);
	}
	search(state.vars, "MAMAKEARGS", duplicate(use(state.opt) + 1));

	/*
	 * scan the makefile and descend
	 */

	walk(state.rules, scan, NiL);
	state.view = 0;
	walk(state.rules, descend, NiL);
	return 0;
}

int
main(int argc, char** argv)
{
	register char**		e;
	register char*		s;
	register char*		t;
	register char*		v;
	Buf_t*			tmp;
	int			c;

	/*
	 * initialize the state
	 */

	state.id = "mamake";
	state.active = 1;
	state.exec = 1;
	state.file = mamfile;
	state.opt = buffer();
	state.rules = dictionary();
	state.vars = dictionary();
	search(state.vars, "MAMAKE", *argv);

	/*
	 * parse the options
	 */

#if _PACKAGE_ast
	error_info.id = state.id;
	for (;;)
	{
		switch (optget(argv, usage))
		{
		case 'e':
			append(state.opt, " -e");
			state.explain = 1;
			continue;
		case 'i':
			append(state.opt, " -i");
			state.ignore = 1;
			continue;
		case 'k':
			append(state.opt, " -k");
			state.keepgoing = 1;
			continue;
		case 'N':
			state.never = 1;
			/* FALLTHROUGH */
		case 'n':
			append(state.opt, " -n");
			state.exec = 0;
			continue;
		case 'F':
			append(state.opt, " -F");
			state.force = 1;
			continue;
		case 'K':
			continue;
		case 'V':
			fprintf(stdout, "%s\n", id + 10);
			exit(0);
		case 'f':
			append(state.opt, " -f ");
			append(state.opt, opt_info.arg);
			state.file = opt_info.arg;
			continue;
		case 'r':
			state.recurse = opt_info.arg;
			continue;
		case 'C':
			state.directory = opt_info.arg;
			continue;
		case 'D':
			append(state.opt, " -D");
			append(state.opt, opt_info.arg);
			state.debug = -opt_info.num;
			continue;
		case 'G':
			append(state.opt, " -G");
			search(state.vars, "-debug-symbols", "1");
			continue;
		case 'S':
			append(state.opt, " -S");
			search(state.vars, "-strip-symbols", "1");
			continue;
		case '?':
			error(ERROR_usage(2), "%s", opt_info.arg);
			UNREACHABLE();
		case ':':
			error(2, "%s", opt_info.arg);
			continue;
		}
		break;
	}
	if (error_info.errors)
	{
		error(ERROR_usage(2), "%s", optusage(NiL));
		UNREACHABLE();
	}
	argv += opt_info.index;
#else
	while ((s = *++argv) && *s == '-')
	{
		if (*(s + 1) == '-')
		{
			if (!*(s + 2))
			{
				append(state.opt, " --");
				argv++;
				break;
			}
			for (t = s += 2; *t && *t != '='; t++);
			if (!strncmp(s, "debug-symbols", t - s) && append(state.opt, " -G") || !strncmp(s, "strip-symbols", t - s) && append(state.opt, " -S"))
			{
				if (*t)
				{
					v = t + 1;
					if (t > s && *(t - 1) == '+')
						t--;
					c = *t;
					*t = 0;
				}
				else
				{
					c = 0;
					v = "1";
				}
				search(state.vars, s - 1, v);
				if (c)
					*t = c;
				continue;
			}
			usage();
			break;
		}
		for (;;)
		{
			switch (*++s)
			{
			case 0:
				break;
			case 'e':
				append(state.opt, " -e");
				state.explain = 1;
				continue;
			case 'i':
				append(state.opt, " -i");
				state.ignore = 1;
				continue;
			case 'k':
				append(state.opt, " -k");
				state.keepgoing = 1;
				continue;
			case 'N':
				state.never = 1;
				/* FALLTHROUGH */
			case 'n':
				append(state.opt, " -n");
				state.exec = 0;
				continue;
			case 'F':
				append(state.opt, " -F");
				state.force = 1;
				continue;
			case 'G':
				append(state.opt, " -G");
				search(state.vars, "-debug-symbols", "1");
				continue;
			case 'K':
				continue;
			case 'S':
				append(state.opt, " -S");
				search(state.vars, "-strip-symbols", "1");
				continue;
			case 'V':
				fprintf(stdout, "%s\n", id + 10);
				exit(0);
			case 'f':
			case 'r':
			case 'C':
			case 'D':
				t = s;
				if (!*++s && !(s = *++argv))
				{
					report(2, "option value expected", t, (unsigned long)0);
					usage();
				}
				else
					switch (*t)
					{
					case 'f':
						append(state.opt, " -f ");
						append(state.opt, s);
						state.file = s;
						break;
					case 'r':
						state.recurse = s;
						break;
					case 'C':
						state.directory = s;
						break;
					case 'D':
						append(state.opt, " -D");
						append(state.opt, s);
						state.debug = -atoi(s);
						break;
					}
				break;
			default:
				report(2, "unknown option", s, (unsigned long)0);
				/* FALLTHROUGH */
			case '?':
				usage();
				break;
			}
			break;
		}
	}
#endif

	/*
	 * load the environment
	 */

	for (e = environ; s = *e; e++)
		for (t = s; *t; t++)
			if (*t == '=')
			{
				*t = 0;
				search(state.vars, s, t + 1);
				*t = '=';
				break;
			}

	/*
	 * grab the command line targets and variable definitions
	 */

	while (s = *argv++)
	{
		for (t = s; *t; t++)
			if (*t == '=')
			{
				v = t + 1;
				if (t > s && *(t - 1) == '+')
					t--;
				c = *t;
				*t = 0;
				search(state.vars, s, v);
				tmp = buffer();
				append(tmp, s);
				append(tmp, ".FORCE");
				search(state.vars, use(tmp), v);
				drop(tmp);
				*t = c;
				break;
			}
		if (!*t)
		{
			/*
			 * handle a few targets for nmake compatibility
			 */

			if (*s == 'e' && !strncmp(s, "error 0 $(MAKEVERSION:", 22))
				exit(1);
			if (*s == 'r' && !strcmp(s, "recurse") || *s == 'c' && !strncmp(s, "cc-", 3))
				continue;
			rule(s)->flags |= RULE_active;
			state.active = 0;
			if (state.recurse)
				continue;
		}
		add(state.opt, ' ');
		add(state.opt, '\'');
		append(state.opt, s);
		add(state.opt, '\'');
	}

	/*
	 * initialize the views
	 */

	if (state.directory && chdir(state.directory))
		report(3, "cannot change working directory", NiL, (unsigned long)0);
	view();

	/*
	 * recursion drops out here
	 */

	if (state.recurse)
		return recurse(state.recurse);

	/*
	 * read the mamfile(s) and bring the targets up to date
	 */

	search(state.vars, "MAMAKEARGS", duplicate(use(state.opt) + 1));
	push(state.file, (Stdio_t*)0, STREAM_MUST);
	make(rule(""));
	pop();

	/*
	 * verify that active targets were made
	 */

	if (!state.active && !state.verified)
		walk(state.rules, verify, NiL);

	/*
	 * done
	 */

	return state.errors != 0;
}

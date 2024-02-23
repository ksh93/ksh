/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1990-2013 AT&T Intellectual Property          *
*          Copyright (c) 2020-2024 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
#if __clang__
#pragma clang diagnostic ignored "-Wparentheses"
#elif __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic ignored "-Wparentheses"
#endif

/*
 * mamake -- MAM make
 *
 * coded for portability
 */

#define RELEASE_DATE "2024-02-17"
static char id[] = "\n@(#)$Id: mamake (ksh 93u+m) " RELEASE_DATE " $\0\n";

#if _PACKAGE_ast

#include <ast.h>
#include <error.h>

static const char usage[] =
"[-?\n@(#)$Id: mamake (ksh 93u+m) " RELEASE_DATE " $\n]"
"[-author?Glenn Fowler <gsf@research.att.com>]"
"[-author?Martijn Dekker <martijn@inlv.org>]"
"[-author?Contributors to https://github.com/ksh93/ksh]"
"[-copyright?(c) 1994-2013 AT&T Intellectual Property]"
"[-copyright?(c) 2020-2024 Contributors to ksh 93u+m]"
"[-license?https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html]"
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
"	directories containing a file named \bMamfile\b"
"	are considered. The \abind\a commands in the Mamfile"
"	found in each leaf directory are scanned for leaf directory"
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

/*
 * For compatibility with compiler flags such as -std=c99, feature macros
 * must be enabled to prevent the build from failing at the very start.
 * These are normally handled in src/lib/libast/features/standards; further
 * information can be found there. Mamake shouldn't need all that much enabled,
 * so not much is enabled here.
 */

/* macOS */
#if (defined(__APPLE__) && defined(__MACH__) && defined(NeXTBSD)) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE 1

/* Solaris and illumos */
#elif defined(__sun)
#define _XPG7
#define _XPG6
#define _XPG5
#define _XPG4_2
#define _XPG4
#define _XPG3
#define __EXTENSIONS__	1
#define _XOPEN_SOURCE	9900
#undef _POSIX_C_SOURCE
/*
 * Though POSIX says it must be allowed, Solaris Studio cc dislikes NULL, a.k.a.
 * (void*)0, being used for function pointers. It warns, or in some cases it even
 * throws an error. Just use 0 for NULL, as that is always acceptable in C.
 */
#if __SUNPRO_C
#undef  NULL
#define NULL    0
#endif /* __SUNPRO_C */

/* Linux and Cygwin */
#elif (defined(__linux__) || __CYGWIN__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE 1

/* QNX */
#elif defined(__QNX__) && !defined(_QNX_SOURCE)
#define _QNX_SOURCE 1

/* Everything else (minus BSD, as the defaults there are acceptable) */
#elif !(BSD && !__APPLE__ && !__MACH__ && !NeXTBSD) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 21000101L
#endif

#endif /* _PACKAGE_ast */

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#if !_PACKAGE_ast
#include <stdlib.h>
#include <string.h>
#endif

#define delimiter(c)	(isspace(c)||(c)==';'||(c)=='('||(c)==')'||(c)=='`'||(c)=='|'||(c)=='&'||(c)=='=')

#define add(b,c)	(((b)->nxt >= (b)->end) ? append(b, "") : NULL, *(b)->nxt++ = (c))
#define get(b)		((b)->nxt-(b)->buf)
#define set(b,o)	((b)->nxt=(b)->buf+(o))
#define use(b)		(*(b)->nxt=0,(b)->nxt=(b)->buf)

#define CHUNK		4096
#define KEY(a,b,c,d)	((((unsigned long)(a))<<24)|(((unsigned long)(b))<<16)|(((unsigned long)(c))<<8)|(((unsigned long)(d))))
#define NOW		((unsigned long)time(NULL))
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
#define RULE_notrace	0x0200		/* do not xtrace shell action	*/
#define RULE_updated	0x0400		/* rule was outdated and remade */

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
#if __STDC_VERSION__ >= 199901L
	char			name[];	/* 0 terminated name		*/
#else
	char			name[1];/* 0 terminated name		*/
#endif
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
	unsigned long	line;		/* starting line in Mamfile	*/
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
#if __STDC_VERSION__ >= 199901L
	char		dir[];		/* viewpath level dir prefix	*/
#else
	char		dir[1];		/* viewpath level dir prefix	*/
#endif
} View_t;

static struct				/* program state		*/
{
	int		strict;		/* strict mode activated if set */

	Buf_t*		old;		/* dropped buffers		*/
	Buf_t*		opt;		/* option buffer		*/

	Dict_t*		leaf;		/* recursion leaf dictionary	*/
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
	int		probed;		/* probe already done		*/
	int		verified;	/* don't bother with verify()	*/

	Stream_t	streams[4];	/* input file stream stack	*/
	Stream_t*	sp;		/* input stream stack pointer	*/

	char*		shim;		/* finished shim		*/
	Buf_t*		shim_buf;	/* shim being built up		*/
} state;

static unsigned long	make(Rule_t*, int, unsigned long, Buf_t**);

static char		mamfile[] = "Mamfile";
static char		sh[] = "/bin/sh";
static char		empty[] = "";

static Dict_item_t*	auto_making;	/* ${@} - name of rule being made */
static Dict_item_t*	auto_prev;	/* ${<} - name of last prerequisite */
static Dict_item_t*	auto_allprev;	/* ${^} - space-separated names of all prerequisites */
static Dict_item_t*	auto_updprev;	/* ${?} - space-separated names of updated prerequisites */

extern char**		environ;

#if !_PACKAGE_ast

#if defined(NeXT) || defined(__NeXT)
#define getcwd(a,b)	getwd(a)
#endif

/*
 * emit usage message and exit
 */

static void
usage(void)
{
	fprintf(stderr, "Usage: %s"
		" [-iknFKNV]"
		" [-f Mamfile]"
		" [-r pattern]"
		" [-C directory]"
		" [-D level]"
		" [target ...]"
		" [name=value ...]"
		"\n", state.id);
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
		report(keepgoing ? 1 : 3, "missing prerequisite", r->name, 0);
	else
	{
		fprintf(stderr, "*** exit code %d making %s%s\n", code, r->name, state.ignore ? " ignored" : "");
		unlink(r->name);
		if (state.ignore)
			return;
		if (!keepgoing)
			exit(1);
		state.errors = 1;
	}
	r->flags |= RULE_error;
}

/*
 * open a buffer stream
 */

static Buf_t*
buffer(void)
{
	Buf_t*	buf;

	if (buf = state.old)
		state.old = state.old->old;
	else if (!(buf = newof(0, Buf_t, 1, 0)) || !(buf->buf = newof(0, char, CHUNK, 0)))
		report(3, "out of memory [buffer]", NULL, 0);
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
			report(3, "out of memory [buffer resize]", NULL, 0);
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
 * realloc for strings (with support for non-allocated empty string):
 * allocate space for s and return the copy
 */

static char*
reduplicate(char* orig, char* s)
{
	char*	t;
	int	n;

	n = strlen(s);
	if (n == 0)
	{
		if (orig && orig != empty)
			free(orig);
		return empty;
	}
	if (!(t = realloc(orig == empty ? NULL : orig, n + 1)))
		report(3, "out of memory [duplicate]", s, 0);
	strcpy(t, s);
	return t;
}

static char*
duplicate(char* s)
{
	return reduplicate(NULL, s);
}

/*
 * open a new dictionary
 */

static Dict_t*
dictionary(void)
{
	Dict_t*	dict;

	if (!(dict = newof(0, Dict_t, 1, 0)))
		report(3, "out of memory [dictionary]", NULL, 0);
	return dict;
}

/*
 * return the value for item name in dictionary dict
 * if value!=0 then name entry value is created if necessary and set
 * uses top-down splaying (ala Tarjan and Sleator)
 */

static void*
search(Dict_t* dict, char* name, void* value)
{
	int		cmp;
	Dict_item_t*	root;
	Dict_item_t*	t;
	Dict_item_t*	left;
	Dict_item_t*	right;
	Dict_item_t*	lroot;
	Dict_item_t*	rroot;

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
		if (!(root = newof(0, Dict_item_t, 1, strlen(name) + 1)))
		{
			report(3, "out of memory [dictionary]", name, 0);
			abort();
		}
		strcpy(root->name, name);
	}
	if (root)
	{
		if (value)
			root->value = value;
		root->left = lroot;
		root->right = rroot;
		dict->root = root;
		return value ? root : root->value;
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
	return NULL;
}

/*
 * low level for walk()
 */

static int
apply(Dict_t* dict, Dict_item_t* item, int (*func)(Dict_item_t*, void*), void* handle)
{
	Dict_item_t*	right;

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

	if (!(r = (Rule_t*)search(state.rules, name, NULL)))
	{
		int n;
		if (!(r = newof(0, Rule_t, 1, 0)))
			report(3, "out of memory [rule]", name, 0);
		r->name = ((Dict_item_t*)search(state.rules, name, r))->name;
		r->line = state.sp ? state.sp->line : 0;
		/*
		 * Since ksh 93u+m removed proto(1) including the *.lic license
		 * atrocities, make those prerequisites optional. This allows
		 * testing code with old Mamfiles using the current build system.
		 */
		if (!state.strict && *name == '/' && (n = strlen(name)) > 4 && strcmp(name + n - 4, ".lic") == 0)
			r->flags |= RULE_dontcare;
	}
	return r;
}

/*
 * prepend p onto rule r prereqs
 */

static void
cons(Rule_t* r, Rule_t* p)
{
	List_t*	x;

	for (x = r->prereqs; x && x->rule != p; x = x->next);
	if (!x)
	{
		if (!(x = newof(0, List_t, 1, 0)))
			report(3, "out of memory [list]", r->name, 0);
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
	char*		s;
	char*		t;
	char*		p;
	View_t*		vp;

	View_t*		zp;
	int		c;
	int		n;

	Stat_t		st;
	Stat_t		ts;

	char		buf[CHUNK];

	if (stat(".", &st))
		report(3, "cannot stat", ".", 0);
	if ((s = (char*)search(state.vars, "PWD", NULL)) && !stat(s, &ts) &&
	    ts.st_dev == st.st_dev && ts.st_ino == st.st_ino)
		state.pwd = s;
	if (!state.pwd)
	{
		if (!getcwd(buf, sizeof(buf) - 1))
			report(3, "cannot determine PWD", NULL, 0);
		state.pwd = duplicate(buf);
		search(state.vars, "PWD", state.pwd);
	}
	if ((s = (char*)search(state.vars, "VPATH", NULL)) && *s)
	{
		p = NULL;
		zp = NULL;
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
					report(3, "cannot stat top view", s, 0);
				if (stat(state.pwd, &ts))
					report(3, "cannot stat", state.pwd, 0);
				if (ts.st_dev == st.st_dev && ts.st_ino == st.st_ino)
					p = ".";
				else
				{
					p = state.pwd + strlen(state.pwd);
					while (p > state.pwd)
					{
						if (*--p == '/')
						{
							if (p == state.pwd)
								report(3, ". not under VPATH", s, 0);
							*p = 0;
							if (stat(state.pwd, &ts))
								report(3, "cannot stat", state.pwd, 0);
							*p = '/';
							if (ts.st_dev == st.st_dev && ts.st_ino == st.st_ino)
							{
								p++;
								break;
							}
						}
					}
					if (p <= state.pwd)
						report(3, "cannot determine viewpath offset", s, 0);
				}
			}
			n = strlen(s);
			if (!p)
				abort();
			if (!(vp = newof(0, View_t, 1, strlen(p) + n + 1)))
				report(3, "out of memory [view]", s, 0);
			vp->node = n + 1;
			strcpy(vp->dir, s);
			*(vp->dir + n) = '/';
			strcpy(vp->dir + n + 1, p);
			report(-4, vp->dir, "view", 0);
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
cond(char* s)
{
	int	n;

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
substitute(Buf_t* buf, char* s)
{
	char	*t, *q;
	char	*v;		/* variable's value */
	char	*b;		/* beginning: the literal expansion starting at $ */
	int	c, n;
	int	found_AR = 0;	/* 1 if ${AR} encountered */
	int	valid_sh_name;	/* if set, the variable name is valid in sh(1) */

	while (c = *s++)
	{
		if (c == '$' && *s == '{')
		{
			b = s - 1;
			t = ++s;
			n = *t == '-' ? 0 : '-';
			valid_sh_name = 1;
			while ( (c = *s) &&
				(c != '?' || s == t) &&
				c != '+' &&
				c != n &&
				c != ':' &&
				c != '=' &&
				c != '[' &&
				c != '}' )
			{
				s++;
				if (!isalnum(c) && c != '_')
					valid_sh_name = 0;
			}

			/* Zero-terminate the variable name */

			*s = 0;

			/* Keep unexpanded if it looks like a ksh array expansion ${var[subscript]} */

			if (c == '[')
			{
				append(buf, b);
				*s = c;
				continue;
			}

			/* Obtain value */

			v = (char*)search(state.vars, t, NULL);

			/*
			 * In strict >= 2, always keep ${foo:-bar}, ${foo:+bar}, ${foo:=bar}, ${foo=bar}
			 * unexpanded for passing on to shell scripts in 'exec'.
			 *
			 * Before strict==2, this depends on whether the MAM variable value is nonexistent
			 * or (if ':') empty, which does not make a lot of sense, as that is not valid MAM
			 * expansion syntax and the feature is not actually implemented properly at all.
			 */

			if ((c == ':' || c == '=') && (state.strict >= 2 || !v || c == ':' && !*v))
			{
				append(buf, b);
				*s = c;
				continue;
			}

			/* A really absurd hack, see check for found_AR further below */

			if (t[0] == 'A' && t[1] == 'R' && t[2] == 0)
				found_AR = 1;

			/* Un-terminate the variable name */

			*s = c;

			/* Find the ending '}', dealing with nesting */

			if (c && c != '}')
			{
				n = 1;
				for (t = ++s; *s; s++)
					if (*s == '{')
						n++;
					else if (*s == '}' && !--n)
						break;
			}

			/* Special expansion syntax */

			switch (c)
			{
			case '?':
				/* ${variable?c?x?y?} */
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
				/* ${variable+x}, ${variable-x} */
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
				{	/*
					 * Perform the expansion: append the value of the variable to the buffer.
					 */
					if (found_AR && strncmp(t, "mam_lib", 7) == 0 && state.strict < 2)
					{	/*
						 * Absurd AT&T hack from 2007. The relevant src/cmd/INIT/RELEASE entry:
						 *	07-02-26 mamake.c: expand first of ${mam_lib*} for ${AR}
						 * i.e.: after ${AR}, expand any ${mam_lib*} to only first word of its value.
						 *
						 * In the entire AST repository's history, this has only been used once, in
						 * libcmd/Mamfile, to extract sumlib.o from libsum.a (to add it to libcmd.a):
						 *
						 *	exec - ${AR} x ${mam_libsum} sumlib.o
						 *
						 * Taking the first word from ${mam_libsum} can very easily be handled
						 * by the shell script instead, so this is disabled at strict level 2+.
						 */
						for (t = v; isspace(*t); t++);
						for (; *t && !isspace(*t); t++);
						if (*t)
							*t = 0;
						else
							t = 0;
						substitute(buf, v);
						if (t)
							*t = ' ';
					}
					else if (state.strict < 2)
					{
						/* Recursively expand variable references in values (no loop detection)! */
						substitute(buf, v);
					}
					else
					{
						/* Sanity at long last. Variables expand to their literal values. */
						append(buf, v);
					}
				}
				else if (valid_sh_name || state.strict >= 2)
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
		report(-3, s, "find", 0);
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
		{
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
					report(-3, s, "find", 0);
					return s;
				}
			} while (vp = vp->next);
		}
	}
	return NULL;
}

/*
 * bind r to a file and return the modify time
 */

static unsigned long
bindfile(Rule_t* r)
{
	char*		s;
	Buf_t*		buf;
	struct stat	st;

	buf = buffer();
	if (s = find(buf, r->name, &st))
	{
		if (s != r->name)
			r->path = reduplicate(r->path, s);
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
		report(3, "input stack underflow", NULL, 0);
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
		report(3, "input stream stack overflow", NULL, 0);
	if (state.sp->fp = fp)
	{
		if (state.sp->file)
			free(state.sp->file);
		state.sp->file = strdup("pipeline");
		if (!state.sp->file)
			report(3, "out of memory [push]", NULL, 0);
	}
	else if (flags & STREAM_PIPE)
		report(3, "pipe error", file, 0);
	else if (!file || !strcmp(file, "-") || !strcmp(file, "/dev/stdin"))
	{
		flags |= STREAM_KEEP;
		if (state.sp->file)
			free(state.sp->file);
		state.sp->file = strdup("/dev/stdin");
		if (!state.sp->file)
			report(3, "out of memory [push]", NULL, 0);
		state.sp->fp = stdin;
	}
	else
	{
		buf = buffer();
		if (path = find(buf, file, &st))
		{
			if (!(state.sp->fp = fopen(path, "r")))
				report(3, "cannot read", path, 0);
			state.sp->file = reduplicate(state.sp->file, path);
			drop(buf);
		}
		else
		{
			drop(buf);
			pop();
			if (flags & STREAM_MUST)
				report(3, "not found", file, 0);
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
	static char	input[8*CHUNK];  /* input buffer */
	char		*e;

	if (!state.sp)
		report(3, "no input file stream", NULL, 0);
	if (!fgets(input, sizeof(input), state.sp->fp))
	{
		if (ferror(state.sp->fp))
			report(3, "read error", NULL, 0);
		return NULL;
	}
	if (*input && *(e = input + strlen(input) - 1) == '\n')
		*e = 0;
	state.sp->line++;
	return input;
}

/*
 * pass shell action s to ${SHELL:-/bin/sh}
 * the -c wrapper ensures that scripts are run in the selected shell
 * even on systems that otherwise demand #! magic (can you say Cygwin)
 */

static int
execute(char* s)
{
	int		c;
	Buf_t*		buf;

	if (!state.shell && (!(state.shell = (char*)search(state.vars, "SHELL", NULL)) || !strcmp(state.shell, sh)))
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
	report(-5, s, "exec", 0);
	if ((c = system(s)) > 255)
		c >>= 8;
	drop(buf);
	return c;
}

/*
 * run action s to update r
 */

static unsigned long
run(Rule_t* r, char* s)
{
	Rule_t*		q;
	char*		t;
	int		c;
	int		i;
	int		j;
	int		x;
	Stat_t		st;
	Buf_t*		buf;

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
	{
		/* stubs for backward compat */
		if (!state.strict)
			append(buf,
				"alias silent=\n"
				"ignore() { env \"$@\" || :; }\n"
			);
		/* find commands in the current working directory first */
		append(buf,
			"case $PATH in\n"
			".:*)	;;\n"
			"*)	PATH=.:$PATH;;\n"
			"esac\n"
		);
		/* disable global pathname expansion for safer field splitting */
		if (state.strict >= 2)
			append(buf,"set -f\n");
		/* show trace for the shell action commands */
		if (!(r->flags & RULE_notrace))
			append(buf,"set -x\n");
		/* add the shim if one was set */
		if (state.shim)
			append(buf, state.shim);
	}

	/*
	 * Do viewpathing
	 */

	if (state.view)
	{
		char	*tofree = NULL;
		if (x && state.shim)
		{
			/* Also subject the user-set shim to viewpathing
			 * (plus other code preprended above, but it should not contain anything viewpathable) */
			char	*pre = use(buf);
			long	n = strlen(pre);
			if (!(tofree = malloc(n + strlen(s) + 1)))
				report(3, "out of memory [run]", NULL, 0);
			strcpy(tofree, pre);
			strcpy(tofree + n, s);
			s = tofree;
		}
		/* Find words to apply viewpathing to */
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
			/*
			 * If the word matches the name of a non-generated prerequisite,
			 * replace it with its canonical path within the source directory.
			 */
			if ((q = (Rule_t*)search(state.rules, t, NULL)) && q->path && !(q->flags & RULE_generated))
				append(buf, q->path);
			else
			{
				append(buf, t);
				/*
				 * Viewpathing for -I cc flags (include path directories):
				 * duplicate every '-Ipath' and '-I path', where 'path' is a relative
				 * pathname (i.e. not starting with '/'). The duplicate points to the
				 * corresponding canonical path in the source directory.
				 */
				if (*t == '-' && *(t + 1) == 'I' && (*(t + 2) || c))
				{
					if (*(t + 2))
						i = 2;
					else
					{
						for (i = 3; isspace(*(t + i)); i++);
						*s = c;
						for (s = t + i; *s && !isspace(*s); s++);
						c = *s;
						*s = 0;
						append(buf, t + 2);
					}
					if (*(t + i) && *(t + i) != '/')
					{
						View_t *v = state.view;
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
		if (tofree)
			free(tofree);
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
		if (status(NULL, 0, r->name, &st))
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
	char*		p;
	char*		d;
	char*		x;
	char*		e;
	int		c;
	int		t;
	int		o;
	Stat_t		st;

	for (e = s; *e && !isspace(*e); e++);
	t = *e;
	if ((x = status(buf, 0, s, &st)) && (st.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)))
		return x;
	if (!(p = (char*)search(state.vars, "PATH", NULL)))
		report(3, "variable not defined", "PATH", 0);
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
		report(3, "command not found", s, 0);
	return NULL;
}

/*
 * generate (if necessary) and read the MAM probe information
 * done on the first `setv CC ...'
 */

static void
probe(void)
{
	char*		cc;
	char*		s;
	unsigned long	h;
	unsigned long	q;
	Buf_t*		buf;
	Buf_t*		pro;
	Buf_t*		tmp;
	struct stat	st;

	static char	let[] = "ABCDEFGHIJKLMNOP";
	static char	cmd[] = "mamprobe";

	if (!(cc = (char*)search(state.vars, "CC", NULL)))
		cc = "cc";
	buf = buffer();
	s = path(buf, cmd, 1);
	q = stat(s, &st) ? 0 : (unsigned long)st.st_mtime;
	pro = buffer();
	s = cc = path(pro, cc, 1);
	for (h = 0; *s; s++)
		h = h * 0x63c63cd9L + *s + 0x9c39c33dL;
	if (!(s = (char*)search(state.vars, "INSTALLROOT", NULL)))
		report(3, "variable must be defined", "INSTALLROOT", 0);
	append(buf, s);
	append(buf, "/lib/probe/C/mam/");
	for (h &= 0xffffffffL; h; h >>= 4)
		add(buf, let[h & 0xf]);
	s = use(buf);
	h = stat(s, &st) ? 0 : (unsigned long)st.st_mtime;
	if (h < q || !push(s, NULL, 0))
	{
		tmp = buffer();
		append(tmp, cmd);
		add(tmp, ' ');
		append(tmp, s);
		add(tmp, ' ');
		append(tmp, cc);
		if (execute(use(tmp)))
			report(3, "cannot generate probe info", s, 0);
		drop(tmp);
		if (!push(s, NULL, 0))
			report(3, "cannot read probe info", s, 0);
	}
	drop(pro);
	drop(buf);
	make(rule(""), 0, 0, NULL);
	pop();
}

/*
 * add attributes in s to r
 */

static void
attributes(Rule_t* r, char* s)
{
	char*	t;
	int	n;

	for (;;)
	{
		int	flag = 0;
		for (; isspace(*s); s++);
		for (t = s; *s && !isspace(*s); s++);
		if (!(n = s - t))
			break;
		switch (*t)
		{
		case 'd':
			if (n == 8 && !strncmp(t, "dontcare", n))
				flag = RULE_dontcare;
			break;
		case 'g':
			/* 'exec' assigns this attribute; ignore explicit assignment at strict level 1 */
			if (n == 9 && !strncmp(t, "generated", n))
				flag = state.strict ? -1 : RULE_generated;
			break;
		case 'i':
			if (n == 6 && !strncmp(t, "ignore", n))
				flag = RULE_ignore;
			else if (n == 8 && !strncmp(t, "implicit", n))
				flag = RULE_implicit;
			break;
		case 'v':
			if (n == 7 && !strncmp(t, "virtual", n))
				flag = RULE_virtual;
			break;
		case 'a':
			if (n == 7 && !strncmp(t, "archive", n))
				flag = -1;	/* not implemented */
			break;
		case 'j':
			if (n == 5 && !strncmp(t, "joint", n))
				flag = -1;	/* not implemented */
			break;
		case 'n':
			if (n == 7 && !strncmp(t, "notrace", n))
				flag = RULE_notrace;
			break;
		}
		if (flag > 0)
			r->flags |= flag;
		else if (flag == 0 || state.strict >= 2)
		{
			t[n] = '\0';
			report(3, "unknown attribute", t, 0);
		}
		else if (state.strict)
			report(1, "deprecated", t, 0);
	}
}

/*
 * define ${mam_libX} for library reference lib
 *
 * lib is expected to be in the format "-lX"
 */

#define LIB_VARPREFIX "mam_lib"

static char*
require(char* lib, int dontcare)
{
	static int	dynamic = -1;
	char		*s, *r, varname[64];

	if (dynamic < 0)
		dynamic = (s = search(state.vars, "mam_cc_L", NULL)) ? atoi(s) : 0;

	if (strlen(lib + 2) > sizeof(varname) - sizeof(LIB_VARPREFIX))
		report(3, "-lname too long", lib, 0);
	sprintf(varname, LIB_VARPREFIX "%s", lib + 2);

	if (!(r = search(state.vars, varname, NULL)))
	{
		Buf_t		*buf = buffer(), *tmp = buffer();
		int		c, tofree = 0;
		FILE		*f;
		struct stat	st;

		s = 0;
		for (;;)
		{
			if (s)
				append(buf, s);
			if (r = search(state.vars, "mam_cc_PREFIX_ARCHIVE", NULL))
				append(buf, r);
			append(buf, lib + 2);
			if (r = search(state.vars, "mam_cc_SUFFIX_ARCHIVE", NULL))
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
				if (r = search(state.vars, "mam_cc_PREFIX_SHARED", NULL))
					append(buf, r);
				append(buf, lib + 2);
				if (r = search(state.vars, "mam_cc_SUFFIX_SHARED", NULL))
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
		search(state.vars, varname, r);
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
				while (isspace(c = fgetc(f)));
				if (c == EOF)
					break;
				do
				{
					add(tmp, c);
				} while ((c = fgetc(f)) != EOF && !isspace(c));
				s = use(tmp);
				if (s[0] && (s[0] != '-' || s[1]))
				{
					add(buf, ' ');
					append(buf, require(s, 0));
				}
			}
			fclose(f);
			if (tofree)
				free(r);
			r = use(buf);
		}
		else if (dontcare)
		{
			append(tmp, "echo 'int main(void){return 0;}' > libtest.$$.c\n"
				"${CC} ${CCFLAGS} -o libtest.$$.x libtest.$$.c ");
			append(tmp, r);
			append(tmp, " >/dev/null 2>&1\n"
				"c=$?\n"
				"exec rm -rf libtest.$$.* &\n"  /* also remove artefacts like *.dSYM dir (macOS) */
				"exit $c\n");
			if (execute(expand(buf, use(tmp))))
			{
				if (tofree)
					free(r);
				r = "";
			}
		}
		r = duplicate(r);
		search(state.vars, varname, r);
		drop(tmp);
		drop(buf);
	}
	return r;
}

/*
 * update ${<}, ${^} and ${?}
 */

static void
update_allprev(Rule_t *r, char *all, char *upd)
{
	char		*name = r->name;
	unsigned long	n = strlen(name), nn;
	/* set ${<} */
	auto_prev->value = reduplicate(auto_prev->value, name);
	/* restore ${^}, append to it */
	if (nn = strlen(all))
		(all = realloc(all, nn + n + 2)) && (all[nn++] = ' ');
	else
		all = malloc(n + 1);
	if (!all)
		report(3, "out of memory [upd_allprev]", NULL, 0);
	strcpy(all + nn, name);
	auto_allprev->value = all;
	/* restore ${?}, append to it if rule was updated */
	if (r->flags & RULE_updated)
	{
		if (nn = strlen(upd))
			(upd = realloc(upd, nn + n + 2)) && (upd[nn++] = ' ');
		else
			upd = malloc(n + 1);
		if (!upd)
			report(3, "out of memory [upd_allprev]", NULL, 0);
		strcpy(upd + nn, name);
	}
	auto_updprev->value = upd;
}

/*
 * input() until `done r'
 *
 * This function is called recursively for both 'make' and 'loop'. The inloop
 * parameter is nonzero while processing a loop, in which case modtime and
 * cmd are passed on from the caller and updated in the caller upon return.
 * If inloop==0, modtime must be initialised to zero and parentcmd is ignored.
 */

static unsigned long
make(Rule_t *r, int inloop, unsigned long modtime, Buf_t **parentcmd)
{
	char		*s;
	char		*u;	/* command name */
	char		*t;	/* argument word */
	char		*v;	/* operand string */
	Rule_t		*q;	/* new rule */
	unsigned long	x;	/* new modtime */
	Buf_t		*buf;	/* scratch buffer */
	Buf_t		*cmd;	/* shell action */

	if (inloop)
		cmd = *parentcmd;
	else
	{
		cmd = NULL;
		r->making++;
		if (r->flags & RULE_active)
			state.active++;
		if (*r->name)
		{
			if (!(r->flags & RULE_virtual))
				modtime = bindfile(r);
			state.indent++;
			report(-1, r->name, "make", r->time);
		}
	}
	buf = buffer();
	/*
	 * Parse lines
	 */
	while (s = input())
	{
		/* skip initial whitespace and empty line */
		for (; isspace(*s); s++);
		if (!*s)
			continue;
		/* isolate command name (u), argument word (t), and the operand string (v) */
		for (u = s; *s && !isspace(*s); s++);
		if (*s)
		{
			for (*s++ = 0; isspace(*s); s++);
			for (t = s; *s && !isspace(*s); s++);
			if (*s)
				for (*s++ = 0; isspace(*s); s++);
			v = s;
		}
		else
			t = v = s;
		/* enforce 4-letter lowercase command name */
		if (u[0]<'a' || u[0]>'z' || u[1]<'a' || u[1]>'z' || u[2]<'a' || u[2]>'z' || u[3]<'a' || u[3]>'z' || u[4] && !isspace(u[4]))
			report(3, "not a command name", u, 0);
		switch (KEY(u[0], u[1], u[2], u[3]))
		{
		case KEY('b','i','n','d'):
			if (t[0] == '-' && t[1] == 'l' && (s = require(t, !strcmp(v, "dontcare"))) && strncmp(r->name, "FEATURE/", 8) && strcmp(r->name, "configure.h"))
			{
				/* bind to library file */
				for (;;)
				{
					for (t = s; *s && !isspace(*s); s++);
					if (*s)
						*s = 0;
					else
						s = 0;
					/* only bother if t is a path to a *.a we built (i.e. not -l...) */
					if (*t == '/')
					{
						q = rule(expand(buf, t));
						attributes(q, v);
						x = bindfile(q);
						if (modtime < x)
							modtime = x;
						if (q->flags & RULE_error)
							r->flags |= RULE_error;
					}
					if (!s)
						break;
					for (*s++ = ' '; isspace(*s); s++);
				}
			}
			continue;

		case KEY('d','o','n','e'):
			if (inloop)
			{
				if (*t)
					report(3, "superflous arguments", u, 0);
				break;
			}
			if (*t)
			{	/* target is optional; use it for sanity check if present */
				q = rule(expand(buf, t));
				if (q != r && (t[0] != '$' || state.strict))
				{
					append(buf,q->name);
					append(buf," != ");
					append(buf,r->name);
					report(3, "mismatched done statement", use(buf), 0);
				}
				if (*v)
				{
					if (state.strict >= 2)
						report(3, "superfluous arguments", u, 0);
					else if (state.strict)
						report(1, "attributes deprecated, move to 'make'", u, 0);
					attributes(r, v);
				}
			}
			if (cmd && state.active && (state.force || r->time < modtime || !r->time && !modtime))
			{
				char	*fname = state.sp->file, *rname = r->name, *rnamepre = "", *val;
				int	len;

				/* show a nice trace header */
				/* ...mamfile path: make relative to ${PACKAGEROOT} */
				if (*fname == '/'
				&& (val = search(state.vars, "PACKAGEROOT", NULL)) && (len = strlen(val))
				&& strncmp(fname, val, len) == 0 && fname[len] == '/' && fname[++len])
					fname += len;
				/* ...rule name: change install root path prefix back to '${INSTALLROOT}' for brevity */
				if (*rname == '/'
				&& (val = search(state.vars, "INSTALLROOT", NULL)) && (len = strlen(val))
				&& strncmp(rname, val, len) == 0 && rname[len] == '/' && rname[len + 1])
					rname += len, rnamepre = "${INSTALLROOT}";
				fprintf(stderr, "\n# %s: %lu-%lu: make %s%s\n",
					fname, r->line, state.sp->line, rnamepre, rname);

				/* -e option */
				if (state.explain)
				{
					fprintf(stderr, "# reason: ");
					if (!r->time)
						fprintf(stderr, "target %s\n",
							(r->flags & RULE_virtual) ? "is virtual" : "not found");
					else
						fprintf(stderr, "target [%lu] older than prerequisites [%lu]\n", r->time, modtime);
				}

				/* run the shell action */
				x = run(r, use(cmd));
				if (modtime < x)
					modtime = x;
				r->flags |= RULE_updated;
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
				/* expand MAM vars now for each line, and not for the entire script at 'done',
				 * to avoid confusing behaviour of automatic variables such as ${<} */
				append(cmd, expand(buf, v));
			}
			/* if a shim is buffered, get it ready and reset the buffer */
			if (get(state.shim_buf))
			{
				state.shim = use(state.shim_buf);
				/* a single 'shim -' deactivates the shim */
				if (*state.shim == '\n' && !state.shim[1])
					state.shim = NULL;
			}
			continue;

		case KEY('l','o','o','p'):
		{
			long		saveoff;
			char		*vname, *words, *w, *nextw, *cp, *save_value;
			Dict_item_t	*vnode;
			unsigned long	saveline = state.sp->line;

			if (!*v)
				report(3, "syntax error", u, 0);
			/* remember current offset for repeated reading */
			if ((saveoff = ftell(state.sp->fp)) < 0)
				report(3, "unseekable input", u, 0);
			/* iterate through one or more whitespace-separated words */
			vname = duplicate(expand(buf, t));
			w = words = duplicate(expand(buf, v));
			save_value = search(state.vars, vname, NULL);
			vnode = search(state.vars, vname, empty);
			for (w = words; w; w = nextw)
			{
				/* zero-terminate current word and find next word */
				nextw = NULL;
				for (cp = w; *cp && !isspace(*cp); cp++);
				if (*cp)
				{
					*cp++ = '\0';
					for (; *cp && isspace(*cp); cp++);
					if (*cp)
						nextw = cp;
				}
				/* set iteration variable to current word */
				vnode->value = w;
				/* reposition input to the start of this loop block */
				if (w != words)
				{
					if (fseek(state.sp->fp, saveoff, SEEK_SET) < 0)
						report(3, "fseek failed", u, 0);
					state.sp->line = saveline;
				}
				/* (re)read the loop block until 'done', in the context of the current rule */
				modtime = make(r, 1, modtime, &cmd);
			}
			vnode->value = save_value;
			free(words);
			free(vname);
			continue;
		}

		case KEY('m','a','k','e'):
			q = rule(expand(buf, t));
			if (!q->making)
			{
				char *save_making = auto_making->value;
				char *save_allprev = auto_allprev->value;
				char *save_updprev = auto_updprev->value;

				/* set ${@}; empty ${?}, ${^} and ${<} */
				auto_making->value = q->name;
				auto_updprev->value = empty;
				auto_allprev->value = empty;
				auto_prev->value = reduplicate(auto_prev->value, empty);

				/* make the target */
				attributes(q, v);
				x = make(q, 0, 0, NULL);
				if (!(q->flags & RULE_ignore) && modtime < x)
					modtime = x;
				if (q->flags & RULE_error)
					r->flags |= RULE_error;

				/* update ${<}, restore/update ${^} and ${?} */
				if (auto_allprev->value != empty)
					free(auto_allprev->value);
				if (auto_updprev->value != empty)
					free(auto_updprev->value);
				update_allprev(q, save_allprev, save_updprev);
				/* restore ${@} */
				auto_making->value = save_making;
			}
			continue;

		case KEY('p','r','e','v'):
		{
			char *name = expand(buf, t);
			if (!state.strict)
				q = rule(name); /* for backward compat */
			else if (!(q = (Rule_t*)search(state.rules, name, NULL)))
			{	/*
				 * 'prev' on a nonexistent rule, i.e., without a preceding 'make'...'done':
				 * special-case this as a way to declare a simple source file prerequisite
				 */
				attributes(q = rule(name), v);
				if (!(q->flags & RULE_virtual))
				{
					bindfile(q);
					if (!(q->flags & (RULE_dontcare | RULE_exists)))
						dont(q, 0, state.keepgoing);
				}
			}
			else if (*v)
				report(3, v, "prev: superfluous attributes", 0);
			if (!q->making)
			{
				if (!(q->flags & RULE_ignore) && modtime < q->time)
					modtime = q->time;
				if (q->flags & RULE_error)
					r->flags |= RULE_error;
				state.indent++;
				report(-2, q->name, "prev", q->time);
				state.indent--;
			}
			/* update ${<}, ${^} and ${?} */
			update_allprev(q, auto_allprev->value, auto_updprev->value);
			continue;
		}

		case KEY('s','e','t','v'):
			if (!search(state.vars, t, NULL))
			{
				if (*v == '"' && state.strict < 2)
				{
					s = v + strlen(v) - 1;
					if (*s == '"')
					{
						*s = 0;
						v++;
					}
				}
				v = duplicate(expand(buf, v));
				search(state.vars, t, v);
				if (strcmp(t, "MAMAKE_STRICT") == 0)
					state.strict = *v ? strtol(v, NULL, 10) : 1;
			}
			if (!state.probed && t[0] == 'C' && t[1] == 'C' && !t[2])
			{
				state.probed = 1;
				probe();
			}
			continue;

		case KEY('s','h','i','m'):
			state.shim = NULL;
			/* add line of code to shim buffer */
			if (*v)
				append(state.shim_buf, expand(buf, v));
			add(state.shim_buf, '\n');
			continue;

		case KEY('n','o','t','e'):
			/* comment command */
			continue;

		case KEY('i','n','f','o'):
		case KEY('m','e','t','a'):
			if (!state.strict)
				continue;
			/* FALLTHROUGH */

		default:
			report(3, "unknown command", u, 0);
		}
		break;
	}
	drop(buf);
	if (inloop)
	{
		*parentcmd = cmd;
		return modtime;
	}
	if (cmd)
		drop(cmd);
	if (*r->name)
	{
		report(-1, r->name, "done", modtime);
		state.indent--;
	}
	if (r->flags & RULE_active)
		state.active--;
	r->making--;
	return r->time = modtime;
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
	char*	s;

	if (s = strrchr(name, '/'))
		s++;
	else
		s = name;
	return s[0] == 'I' && s[1] == 'N' && s[2] == 'I' && s[3] == 'T';
}

/*
 * update recursion leaf r and its prerequisites
 */

static int
update(Rule_t* r)
{
	List_t*			x;
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
	fprintf(stderr, "\n# ... making %s ...\n", r->name);
	if (state.explain)
		fprintf(stderr, "# reason: recursion\n");
	run(r, use(buf));
	drop(buf);
	return 0;
}

/*
 * scan Mamfile prereqs
 */

static int
scan(Dict_item_t* item, void* handle)
{
	Rule_t*			r = (Rule_t*)item->value;
	char*			s;
	char*			t;
	Rule_t*			q;
	Buf_t*			buf;

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
	append(buf, r->name);
	add(buf, '/');
	append(buf, mamfile);
	if (push(use(buf), NULL, 0))
	{
		while (s = input())
		{
			for (; isspace(*s); s++);
			/* examine only commands of the form bind -lfoo */
			if (s[0] != 'b' || s[1] != 'i' || s[2] != 'n' || s[3] != 'd' || !isspace(s[4]))
				continue;
			for (s += 5; isspace(*s); s++);
			if (s[0] != '-' || s[1] != 'l' || !s[2])
				continue;
			/* construct potential leaf directory name */
			append(buf, "lib");
			append(buf, s + 2);
			t = use(buf);
			for (s = t; *s && !isspace(*s); s++);
			*s = '\0';
			/* add a rule and prepend it onto the prerequisites */
			if ((q = (Rule_t*)search(state.leaf, t, NULL)) && q != r)
				cons(r, q);
		}
		pop();
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

	if (!state.active && (!(r->flags & RULE_active) || !(r = (Rule_t*)search(state.leaf, r->name, NULL))))
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
		if (r->leaf || search(state.leaf, r->name, NULL))
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
	char*		s;
	char*		t;
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
			if (t = strrchr(r->name, '/'))
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
		walk(state.rules, active, NULL);
	}
	search(state.vars, "MAMAKEARGS", duplicate(use(state.opt) + 1));

	/*
	 * scan the Mamfile and descend
	 */

	walk(state.rules, scan, NULL);
	while (state.view)
	{
		View_t *prev = state.view;
		state.view = state.view->next;
		free(prev);
	}
	walk(state.rules, descend, NULL);
	return 0;
}

int
main(int argc, char** argv)
{
	char**		e;
	char*		s;
	char*		t;
	char*		v;
	Buf_t*		tmp;
	int		c;

	/*
	 * initialize the state
	 */

	state.id = "mamake";
	state.active = 1;
	state.exec = 1;
	state.file = mamfile;
	state.opt = buffer();
	state.shim_buf = buffer();
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
		error(ERROR_usage(2), "%s", optusage(NULL));
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
					report(2, "option value expected", t, 0);
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
				report(2, "unknown option", s, 0);
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
	 * option incompatibility
	 */

	if (state.force)
		state.explain = 0;

	/*
	 * load the environment
	 */

	for (e = environ; s = *e; e++)
	{
		for (t = s; *t; t++)
		{
			if (*t == '=')
			{
				*t = 0;
				search(state.vars, s, t + 1);
				*t = '=';
				break;
			}
		}
	}

	/*
	 * initialize the automatic variables
	 */

	auto_making = search(state.vars, "@", empty);
	auto_prev = search(state.vars, "<", empty);
	auto_allprev = search(state.vars, "^", empty);
	auto_updprev = search(state.vars, "?", empty);

	/*
	 * grab the command line targets and variable definitions
	 */

	while (s = *argv++)
	{
		for (t = s; *t; t++)
		{
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
		report(3, "cannot change working directory", NULL, 0);
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
	push(state.file, NULL, STREAM_MUST);
	make(rule(""), 0, 0, NULL);
	pop();

	/*
	 * verify that active targets were made
	 */

	if (!state.active && !state.verified)
		walk(state.rules, verify, NULL);

	/*
	 * done
	 */

	return state.errors != 0;
}

/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1990-2013 AT&T Intellectual Property          *
*          Copyright (c) 2020-2026 Contributors to ksh 93u+m           *
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
#elif __GNUC__
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wparentheses"
#endif

/*
 * mamake -- MAM make
 *
 * coded for portability
 */

#define RELEASE_DATE "2026-02-15"
static char id[] = "\n@(#)$Id: mamake (ksh 93u+m) " RELEASE_DATE " $\0\n";

#if _PACKAGE_ast

#include <ast.h>
#include <error.h>
#include <sig.h>

static const char usage[] =
"[-?\n@(#)$Id: mamake (ksh 93u+m) " RELEASE_DATE " $\n]"
"[-author?Glenn Fowler <gsf@research.att.com>]"
"[-author?Martijn Dekker <martijn@inlv.org>]"
"[-author?Contributors to https://github.com/ksh93/ksh]"
"[-copyright?(c) 1994-2013 AT&T Intellectual Property]"
"[-copyright?(c) 2020-2026 Contributors to ksh 93u+m]"
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
"[c:?Chaotic simultaneous output when executing shell actions in parallel."
"	See \b-j\b below.]"
"[e:?Explain reason for triggering action. Ignored if -F is on.]"
"[f:?Read \afile\a instead of the default.]:[file:=Mamfile]"
"[i:?Ignore action errors.]"
"[j:?Execute up to \amaxjobs\a shell actions in parallel."
"	Unless \b-c\b is given, each shell action's output is saved up and"
"	then logged in one go when it finishes, avoiding chaotic logs."
"	Rules with the \bvirtual\b attribute are never run in parallel."
"	Ignored if \bMAMAKE_STRICT\b is unset or less than 5.]#[maxjobs]"
"[k:?If an error occurs, keep going and build as much as possible."
"	Rules that depend on rules that had errors"
"	will not have their actions executed.]"
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
"[N:?Like \b-n\b but recursion actions (see \b-r\b) are also disabled.]"
"[V:?Print the program version and exit.]"
"[G:debug-symbols?Compile and link with debugging symbol options enabled.]"
"[S:strip-symbols?Strip link-time static symbols from executables.]"

"\n"
"\n[ target ... ] [ name=value ... ]\n"
"\n"

"[+SEE ALSO?\bmamprobe\b(1), \bsh\b(1)]"
;

#else

#define elementsof(x)	(sizeof(x)/sizeof(x[0]))
#define NOT_USED(x)	do (void)(x); while(0)

/*
 * For compatibility with compiler flags such as -std=c99, feature macros
 * must be enabled to prevent the build from failing at the very start.
 */

#define _DARWIN_C_SOURCE 1		/* macOS */
#define _GNU_SOURCE 1			/* GNU/Linux, Cygwin */
#define _FILE_OFFSET_BITS 64		/* 32-bit Linux 5.6+ (glibc 2.32+) */
#define _TIME_BITS 64			/* 32-bit Linux 5.6+ (glibc 2.32+) */
#define _QNX_SOURCE 1			/* QNX */
#define __EXTENSIONS__	1		/* Solaris/illumos */

#endif /* _PACKAGE_ast */

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>

#if !_PACKAGE_ast
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#endif

/* assert.h does not play well with libast < 20240811 */
#if !_PACKAGE_ast || AST_VERSION >= 20240811
#include <assert.h>
#else
#define assert(c)	((void)0)
#endif

/* backward compat for file offsets */
#if _POSIX_VERSION < 200112L && !_PACKAGE_ast
#define off_t		long
#define fseeko		fseek
#define ftello		ftell
#endif

#define delimiter(c)	(isspace(c)||(c)==';'||(c)=='('||(c)==')'||(c)=='`'||(c)=='|'||(c)=='&'||(c)=='=')

#define add(b,c)	(((b)->nxt >= (b)->end) ? append(b, "") : NULL, *(b)->nxt++ = (c))
#define unadd(b)	(--(b)->nxt)
#define getsize(b)	((b)->nxt-(b)->buf)
#define setsize(b,o)	((b)->nxt=(b)->buf+(o))
#define use(b)		(*(b)->nxt=0,(b)->nxt=(b)->buf)

#define CHUNK		4096
#define KEY(a,b,c,d)	((((unsigned long)(a))<<24)|(((unsigned long)(b))<<16)|(((unsigned long)(c))<<8)|(((unsigned long)(d))))

#ifdef SA_RESTART
#define PARALLEL(r)	(state.maxjobs > 1 && state.strict >= 5 && !((r)->flags & RULE_virtual))
#else
/* disable parallel build on systems that can't auto-restart interrupted system calls */
#define PARALLEL(r)	0
#define SA_RESTART	0
#endif

#define RULE_active	0x0001		/* active target		*/
#define RULE_dontcare	0x0002		/* ok if not found		*/
#define RULE_error	0x0004		/* not found or not generated	*/
#define RULE_exists	0x0008		/* target file exists		*/
#define RULE_generated	0x0010		/* generated target		*/
#define RULE_ignore	0x0020		/* ignore time			*/
#define RULE_made	0x0040		/* already made			*/
#define RULE_virtual	0x0080		/* not a file			*/
#define RULE_notrace	0x0100		/* do not xtrace shell action	*/
#define RULE_updated	0x0200		/* rule was outdated and remade */
#define RULE_preexisted	0x0400		/* the rule's target preexisted	*/

#define STREAM_KEEP	0x0001		/* don't fclose() on pop()	*/
#define STREAM_MUST	0x0002		/* push() file must exist	*/
#define STREAM_PIPE	0x0004		/* pclose() on pop()		*/

#define LIB_VARPREFIX	"mam_lib"	/* prefix for dependencies vars	*/

struct Rule_s;

typedef struct Buf_s			/* buffer stream		*/
{
	struct Buf_s	*old;		/* next dropped buffer		*/
	char		*end;		/* 1 past end of buffer		*/
	char		*nxt;		/* next char to add		*/
	char		*buf;		/* buffer space			*/
} Buf_t;

typedef struct Dict_item_s		/* dictionary item		*/
{
	struct Dict_item_s	*left;	/* left child			*/
	struct Dict_item_s	*right;	/* right child			*/
	void			*value;	/* user defined value		*/
#if __STDC_VERSION__ >= 199901L
	char			name[];	/* 0 terminated name		*/
#else
	char			name[1];/* 0 terminated name		*/
#endif
} Dict_item_t;

typedef struct Dict_s			/* dictionary handle		*/
{
	Dict_item_t	*root;		/* root item			*/
} Dict_t;

typedef struct List_s			/* Rule_t list			*/
{
	struct List_s	*next;		/* next in list			*/
	struct Rule_s	*rule;		/* list item			*/
} List_t;

typedef struct Rule_s			/* rule item			*/
{
	char		*name;		/* unbound name			*/
	char		*path;		/* bound path			*/
	List_t		*prereqs;	/* prerequisites		*/
	struct Rule_s	*leaf;		/* recursion leaf alias		*/
	int		flags;		/* RULE_* flags			*/
	int		making;		/* currently make()ing		*/
	time_t		time;		/* modification time		*/
	time_t		parenttime;	/* parent's modification time	*/
	time_t		origtime;	/* pre-existing modific. time	*/
	unsigned int	line;		/* starting line in Mamfile	*/
	unsigned int	endline;	/* ending line in Mamfile	*/
	pid_t		pid;		/* PID of parallel bg job	*/
	char		*logtmp;	/* temp file for log output	*/
} Rule_t;

typedef struct Stream_s			/* input file stream stack	*/
{
	FILE		*fp;		/* read stream			*/
	char		*file;		/* stream path			*/
	unsigned int	line;		/* stream line			*/
	int		flags;		/* stream flags			*/
} Stream_t;

typedef struct View_s			/* viewpath level		*/
{
	struct View_s	*next;		/* next level in viewpath	*/
	size_t		node;		/* viewpath node path length	*/
#if __STDC_VERSION__ >= 199901L
	char		dir[];		/* viewpath level dir prefix	*/
#else
	char		dir[1];		/* viewpath level dir prefix	*/
#endif
} View_t;

typedef struct Makestate_s		/* make() shareable state	*/
{
	time_t		modtime;	/* preliminary last-mod time	*/
	Buf_t		*cmd;		/* shell action			*/
	List_t		*bg;		/* list of rules with bg jobs	*/
} Makestate_t;

static struct				/* program state		*/
{
	int		strict;		/* strict mode activated if set */

	Buf_t		*old;		/* dropped buffers		*/
	Buf_t		*opt;		/* option buffer		*/

	Dict_t		*leaf;		/* recursion leaf dictionary	*/
	Dict_t		*rules;		/* rule dictionary		*/
	Dict_t		*vars;		/* variable dictionary		*/

	View_t		*view;		/* viewpath levels		*/

	char		*directory;	/* work in this directory	*/
	char		*id;		/* command name			*/
	char		*file;		/* first input file		*/
	char		*pwd;		/* current directory		*/
	char		*recurse;	/* recursion pattern		*/
	char		*shell;		/* %{SHELL}			*/
	char		*installroot;	/* %{INSTALLROOT}		*/
	char		*packageroot;	/* %{PACKAGEROOT}		*/

	int		active;		/* targets currently active	*/
	int		chaos;		/* don't save up parallel logs	*/
	int		debug;		/* negative of debug level	*/
	int		exitstatus;	/* > 0 if error(s) occurred	*/
	int		exec;		/* execute actions		*/
	int		explain;	/* explain actions		*/
	int		force;		/* all targets out of date	*/
	int		ignore;		/* ignore command errors	*/
	int		indent;		/* debug indent			*/
	int		installrootlen;	/* strlen of %{INSTALLROOT}	*/
	int		packagerootlen;	/* strlen of %{PACKAGEROOT}	*/
	int		keepgoing;	/* do siblings on error		*/
	int		never;		/* never execute		*/
	int		probed;		/* probe already done		*/
	int		verified;	/* don't bother with verify()	*/
	int		jobs, maxjobs;	/* for parallel sh execution	*/

	Stream_t	streams[4];	/* input file stream stack	*/
	Stream_t	*sp;		/* input stream stack pointer	*/

	char		*shim;		/* finished shim		*/
	Buf_t		*shim_buf;	/* shim being built up		*/
} state;

static void		make(Rule_t*, Makestate_t*);
static void		exit_wait(int);
static int		execute_v(Rule_t*, char**);
static int		execute(Rule_t*, char*);

static char		mamfile[] = "Mamfile";
static char		sh[] = "/bin/sh";
static char		empty[] = "";

static Dict_item_t	*auto_making;	/* %{@} - name of rule being made */
static Dict_item_t	*auto_prev;	/* %{<} - name of last prerequisite */
static Dict_item_t	*auto_allprev;	/* %{^} - space-separated names of all prerequisites */
static Dict_item_t	*auto_updprev;	/* %{?} - space-separated names of updated prerequisites */

extern char		**environ;

#if !_PACKAGE_ast

/*
 * emit usage message and exit
 */

static void usage(void)
{
	fprintf(stderr, "Usage: %s"
		" [-ceiknFNVGS]"
		" [-f file]"
		" [-j maxjobs]"
		" [-r pattern]"
		" [-C directory]"
		" [-D level]"
		" [ target ... ]"
		" [ name=value ... ]"
		"\n", state.id);
	exit(2);
}

#endif

/*
 * output error message identification
 */

static void identify(FILE *sp)
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

static void report(int level, char *text, char *item, Rule_t *r)
{
	int	i;

	if (level >= state.debug)
	{
		if (level)
			identify(stderr);
		if (level < 0)
		{
			fprintf(stderr, "debug%d: ", level);
			for (i = 0; i < state.indent; i++)
				fprintf(stderr, "  ");
		}
		else
		{
			if (state.sp && state.sp->line)
			{
				if (state.sp->file)
					fprintf(stderr, "%s: ", state.sp->file);
				/* do not use the current line when reporting an error from a background job */
				fprintf(stderr, "%u: ", r && r->pid ? r->endline : state.sp->line);
			}
			if (level == 1)
				fprintf(stderr, "warning: ");
			else if (level > 1 && !state.exitstatus)
				state.exitstatus = 1;
		}
		if (item)
			fprintf(stderr, "%s: ", item);
		fprintf(stderr, "%s", text);
		if (r && r->time && state.debug <= -2)
#if __STDC_VERSION__ >= 199901L
			fprintf(stderr, " %llu", (unsigned long long)r->time);
#else
			fprintf(stderr, " %lu", (unsigned long)r->time);
#endif
		fprintf(stderr, "\n");
		if (level > 2)
			exit_wait(level - 2);
	}
}

/*
 * throw error and exit with status 1
 */

static void error_out(char *text, char *item)
{
	report(3, text, item, NULL);
}

/*
 * out of memory error with stack trace
 */

static void out_of_memory(void)
{
	report(2, "out of memory", NULL, NULL);
	abort();
}

/*
 * don't know how to make or exit code making
 */

static void error_making(Rule_t *r, int code)
{
	if (code <= 0)
	{
		report(state.ignore && code ? 1 : state.keepgoing ? 2 : 3,
			code == -2 ? "target not updated" : code ? "target not generated" : "missing prerequisite",
			r->name, r);
		if (state.ignore && code)
			return;
		code = 1;
	}
	else
	{
		identify(stderr);
		fprintf(stderr, "*** exit code %d making %s%s\n", code, r->name, state.ignore ? " ignored" : "");
		unlink(r->name);
		if (state.ignore)
			return;
		if (!state.keepgoing)
			exit_wait(code);
	}
	if (code > state.exitstatus)
		state.exitstatus = code;
	r->flags |= RULE_error;
}

/*
 * open a buffer stream
 */

static Buf_t *buffer(void)
{
	Buf_t	*buf;

	if (buf = state.old)
		state.old = state.old->old;
	else
	{
		if (!(buf = calloc(1, sizeof(Buf_t))) || !(buf->buf = malloc(CHUNK)))
			out_of_memory();
		buf->end = buf->buf + CHUNK;
	}
	buf->nxt = buf->buf;
	return buf;
}

/*
 * close a buffer stream
 */

static void drop(Buf_t *buf)
{
	buf->old = state.old;
	state.old = buf;
}

/*
 * append str length n to buffer and return the buffer base
 */

static char *appendn(Buf_t *buf, char *str, size_t n)
{
	size_t	m, i;

	if ((n + 1) >= (size_t)(buf->end - buf->nxt))
	{
		i = buf->nxt - buf->buf;
		m = (((buf->end - buf->buf) + n + CHUNK + 1) / CHUNK) * CHUNK;
		if (!(buf->buf = realloc(buf->buf, m)))
			out_of_memory();
		buf->end = buf->buf + m;
		buf->nxt = buf->buf + i;
	}
	memcpy(buf->nxt, str, n + 1);
	buf->nxt += n;
	return buf->buf;
}

/*
 * append str to buffer and return the buffer base
 */

static char *append(Buf_t *buf, char *str)
{
	return appendn(buf, str, strlen(str));
}

/*
 * realloc for strings (with support for non-allocated empty string):
 * allocate space for s and return the copy
 */

static char *reduplicate(char *orig, char *s)
{
	char	*t;
	size_t	n;

	n = strlen(s);
	if (n == 0)
	{
		if (orig && orig != empty)
			free(orig);
		return empty;
	}
	if (!(t = realloc(orig == empty ? NULL : orig, ++n)))
		out_of_memory();
	return memcpy(t, s, n);
}

static char *duplicate(char *s)
{
	return reduplicate(NULL, s);
}

/*
 * open a new dictionary
 */

static Dict_t *dictionary(void)
{
	Dict_t	*dict;

	if (!(dict = calloc(1, sizeof(Dict_t))))
		out_of_memory();
	return dict;
}

/*
 * Return a pointer to item 'name' in dictionary 'dict'.
 * If create!=0, then the item is created if necessary.
 * Uses top-down splaying (ala Tarjan and Sleator).
 */

static Dict_item_t *search(Dict_t *dict, char *name, int create)
{
	int		cmp;
	Dict_item_t	*root = dict->root, *left = NULL, *right = NULL, *lroot = NULL, *rroot = NULL;

	while (root)
	{
		if (!(cmp = strcmp(name, root->name)))
			break;
		else if (cmp < 0)
		{
			if (root->left && (cmp = strcmp(name, root->left->name)) <= 0)
			{
				/* rotate(left, right) */
				Dict_item_t *t = root->left;
				root->left = t->right;
				t->right = root;
				root = t;
				if (!cmp)
					break;
			}
			if (right)
				right->left = root;
			else
				rroot = root;
			right = root;
			root = root->left;
			right->left = NULL;
		}
		else
		{
			if (root->right && (cmp = strcmp(name, root->right->name)) >= 0)
			{
				/* rotate(right, left) */
				Dict_item_t *t = root->right;
				root->right = t->left;
				t->left = root;
				root = t;
				if (!cmp)
					break;
			}
			if (left)
				left->right = root;
			else
				lroot = root;
			left = root;
			root = root->right;
			left->right = NULL;
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
	else if (create)
	{
		size_t n = strlen(name) + 1;
		if (!(root = malloc(sizeof(Dict_item_t) + n)))
			out_of_memory();
		root->value = NULL;
		memcpy(root->name, name, n);
	}
	if (root)
	{
		root->left = lroot;
		root->right = rroot;
		dict->root = root;
		return root;
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

static void setval(Dict_t *dict, char *name, void *value)
{
	Dict_item_t *node = search(dict, name, 1);
	node->value = value;
}

static void *getval(Dict_t *dict, char *name)
{
	Dict_item_t *node = search(dict, name, 0);
	return node ? node->value : NULL;
}

/*
 * low level for walk()
 */

static int apply(Dict_t *dict, Dict_item_t *item, int (*func)(Dict_item_t *))
{
	Dict_item_t	*right;

	do
	{
		right = item->right;
		if (item->left && apply(dict, item->left, func))
			return -1;
		if ((*func)(item))
			return -1;
	} while (item = right);
	return 0;
}

/*
 * apply func to each dictionary item
 */

static int walk(Dict_t *dict, int (*func)(Dict_item_t *))
{
	return dict->root ? apply(dict, dict->root, func) : 0;
}

/*
 * return a rule pointer for name
 */

static Rule_t *rule(char *name)
{
	Rule_t	*r;

	if (!(r = getval(state.rules, name)))
	{
		Dict_item_t *rnode;
		size_t n;
		if (!(r = calloc(1, sizeof(Rule_t))))
			out_of_memory();
		rnode = search(state.rules, name, 1);
		rnode->value = r;
		r->name = rnode->name;
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

static void cons(Rule_t *r, Rule_t *p)
{
	List_t	*x;

	for (x = r->prereqs; x && x->rule != p; x = x->next);
	if (!x)
	{
		if (!(x = calloc(1, sizeof(List_t))))
			out_of_memory();
		x->rule = p;
		x->next = r->prereqs;
		r->prereqs = x;
	}
}

/*
 * initialize the viewpath
 */

static void view(void)
{
	char		*s, *t, *p;
	View_t		*vp, *zp;
	int		c;
	size_t		slen, plen;
	struct stat	st, ts;
	char		buf[CHUNK];
	Dict_item_t	*vnode;

	if (stat(".", &st))
		error_out("cannot stat", ".");
	vnode = search(state.vars, "PWD", 1);
	if ((s = vnode->value) && !stat(s, &ts) &&
	    ts.st_dev == st.st_dev && ts.st_ino == st.st_ino)
		state.pwd = s;
	if (!state.pwd)
	{
		if (!getcwd(buf, sizeof buf - 1))
			error_out("cannot determine PWD", NULL);
		state.pwd = duplicate(buf);
		vnode->value = state.pwd;
	}
	if ((s = getval(state.vars, "VPATH")) && *s)
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
					error_out("cannot stat top view", s);
				if (stat(state.pwd, &ts))
					error_out("cannot stat", state.pwd);
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
								error_out(". not under VPATH", s);
							*p = 0;
							if (stat(state.pwd, &ts))
								error_out("cannot stat", state.pwd);
							*p = '/';
							if (ts.st_dev == st.st_dev && ts.st_ino == st.st_ino)
							{
								p++;
								break;
							}
						}
					}
					if (p <= state.pwd)
						error_out("cannot determine viewpath offset", s);
				}
			}
			slen = strlen(s);
			assert(p);
			plen = strlen(p);
			if (!(vp = malloc(sizeof(View_t) + slen + plen + 2)))
				out_of_memory();
			vp->next = NULL;
			vp->node = slen + 1;
			memcpy(vp->dir, s, slen);
			*(vp->dir + slen) = '/';
			memcpy(vp->dir + slen + 1, p, plen + 1);
			report(-4, vp->dir, "view", NULL);
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

static char *cond(char *s)
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

static void substitute(Buf_t *buf, char *s)
{
	char	*t, *q;
	char	*v;		/* variable's value */
	char	*b;		/* beginning: the literal expansion starting at % (or $) */
	int	c, n;
	int	found_AR = 0;	/* 1 if ${AR} encountered */
	int	valid_sh_name;	/* if set, the variable name is valid in sh(1) */
	int	newexp;		/* if set, %{...}, otherwise ${...} */
	char	*vnterm;	/* pointer to byte following variable name */
	char	**argv;		/* argument list for %{variable@sh script} */

	while (c = *s++)
	{
		if ((c == '%' || (c == '$' && state.strict < 4)) && *s == '{')
		{
			newexp = (c == '%');
			b = s - 1;
			t = ++s;
			n = *t == '-' ? 0 : '-';  /* disable %{var-x} if var starts with - */
			valid_sh_name = !newexp && (isalpha(*t) || *t == '_');
			s++;
			while ( (c = *s) &&
				c != '?' &&
				c != '+' &&
				c != n &&
				(c != ':' && c != '=' && c != '[' || newexp) &&
				(c != '@' && c != '|' || !newexp) &&
				c != '}' )
			{
				s++;
				if (valid_sh_name && !isalnum(c) && c != '_')
					valid_sh_name = 0;
			}
			vnterm = s;

			/* Zero-terminate the variable name */

			*vnterm = 0;

			/* Keep unexpanded if it looks like a ksh array expansion ${var[subscript]} */

			if (c == '[')
			{
				append(buf, b);
				*vnterm = c;
				continue;
			}

			/* Obtain value */

			v = getval(state.vars, t);

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
				*vnterm = c;
				continue;
			}

			/* A really absurd hack, see check for found_AR further below */

			if (!newexp && strcmp(t, "AR") == 0)
				found_AR = 1;

			/* Un-terminate the variable name */

			*vnterm = c;

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

			argv = NULL;
			switch (c)
			{
			case '?':
				/* %{variable?c?x?y?} */
				q = cond(t - 1);
				if (v)
				{
					if (((q - t) != 1 || *t != '*') && strncmp(v, t, q - t))
						v = 0;
				}
				else if (q == t)
					v = s;
				t = cond(q);
				if (v && *v)
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
			case '@':
				/* %{variable@sh script}: whitespace-separated fields become positional parameters in sh */
				if (q = v)
				{
					n = 4;  /* first four args will be "sh" "-c" script state.id */
					while (*q)
					{
						char	*a;
						while (isspace(*q))
							q++;
						if (!*q)
							break;
						a = q;
						while (*q && !isspace(*q))
							q++;
						n++;
						c = *q, *q = 0;  /* terminate for duplicate() */
						if (!(argv = realloc(argv, (n+1)*sizeof(char*))) || !(argv[n-1] = duplicate(a)))
							out_of_memory();
						*q = c;
					}
					if (!argv)
						break;
					argv[n] = NULL;
				}
				/* FALLTHROUGH */
			case '|':
				/* if (!argv): %{variable|sh script}: whitespace-separated fields are piped into sh as lines */
				if (!v)
					goto undefined;
				if (*v)
				{
					static char	in[] = ".mamake.in";
					static char	out[] = ".mamake.out";
					FILE		*f;
					int		final_newline = 0;
					Buf_t		*scr;
					if (!argv)
					{	/* write value to temp file, converting whitespace to newlines */
						if (!(f = fopen(in, "w")))
							error_out(strerror(errno), "could not open pipe command input for writing");
						errno = 0;
						do
						{
							while (isspace(*v))
								v++;
							while ((n = *v++) && !isspace(n))
								if (putc(n, f) == EOF) break;
							if (putc('\n', f) == EOF) break;
						} while (n);
						fclose(f);
						if (errno)
							error_out(strerror(errno), "could not write pipe command input");
					}
					/* prepend redirections to script and execute using our chosen shell */
					scr = buffer();
					append(scr, "exec");
					if (!argv)
						add(scr, '<'), append(scr, in);
					add(scr, '>'), append(scr, out), add(scr, '\n');
					n = *s, *s = 0;  /* change final '}' to string terminator */
					substitute(scr, t);
					q = use(scr);
					if ((argv ? (argv[2] = q, execute_v(NULL, argv)) : execute(NULL, q)) != 0)
						error_out("expansion script error", t);
					*s = n;
					drop(scr);
					/* read output back, converting each newline but the last to a space */
					if (!(f = fopen(out, "r")))
						error_out(strerror(errno), "could not open pipe command output for reading");
					while ((c = getc(f)) != EOF)
						add(buf, (final_newline = (c == '\n')) ? ' ' : c);
					if (final_newline)
						unadd(buf);
					if (ferror(f) || fclose(f) == EOF)
						error_out(strerror(errno), "could not read pipe command output");
					unlink(out);
					if (!argv)
						unlink(in);
				}
				if (argv)
				{
					for (n = 4; argv[n]; n++)
						free(argv[n]);
					free(argv);
				}
				break;
			case '+':
			case '-':
				/* %{variable+x}, %{variable-x} */
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
					if (state.strict < 2 && found_AR &&
						strncmp(t, LIB_VARPREFIX, sizeof LIB_VARPREFIX - 1) == 0)
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
				else if (newexp)
				{
				undefined:
					*vnterm = 0;
					error_out("variable not defined", b + 2);
				}
				else if (valid_sh_name || state.strict >= 2)
				{
					/* Leave sh-like ${var} unexpanded for undefined var */
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

static char *expand(Buf_t *buf, char *s)
{
	substitute(buf, s);
	return use(buf);
}

/*
 * stat() with .exe check
 */

static char *status(Buf_t *buf, size_t off, char *path, struct stat *st)
{
	int	r;
	char	*s;
	Buf_t	*tmp;

	if (!stat(path, st))
		return path;
	if (!(tmp = buf))
	{
		tmp = buffer();
		off = 0;
	}
	if (off)
		setsize(tmp, off);
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

static char *find(Buf_t *buf, char *file, struct stat *st)
{
	char	*s;
	View_t	*vp;
	int	node, c;
	size_t	o;

	if (s = status(buf, 0, file, st))
	{
		report(-3, s, "find", NULL);
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
				o = getsize(buf);
				s = use(buf);
				if (s = status(buf, o, s, st))
				{
					report(-3, s, "find", NULL);
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

static void bindfile(Rule_t *r)
{
	char		*s;
	Buf_t		*buf;
	struct stat	st;

	buf = buffer();
	if (s = find(buf, r->name, &st))
	{
		if (s != r->name)
			r->path = reduplicate(r->path, s);
		r->time = r->origtime = st.st_mtime;
		r->flags |= RULE_exists;
	}
	drop(buf);
}

/*
 * pop the current input file
 */

static int pop(void)
{
	int	r;

	assert(state.sp);
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

static int push(char *file, FILE *fp, int flags)
{
	char		*path;
	Buf_t		*buf;
	struct stat	st;

	if (!state.sp)
		state.sp = state.streams;
	else if (++state.sp >= &state.streams[elementsof(state.streams)])
		error_out("input stream stack overflow", NULL);
	if (state.sp->fp = fp)
		state.sp->file = reduplicate(state.sp->file, "pipeline");
	else if (flags & STREAM_PIPE)
		error_out("pipe error", file);
	else if (!file || !strcmp(file, "-") || !strcmp(file, "/dev/stdin"))
	{
		flags |= STREAM_KEEP;
		state.sp->file = reduplicate(state.sp->file, "/dev/stdin");
		state.sp->fp = stdin;
	}
	else
	{
		buf = buffer();
		if (path = find(buf, file, &st))
		{
			if (!(state.sp->fp = fopen(path, "r")))
				error_out("cannot read", path);
			state.sp->file = reduplicate(state.sp->file, path);
			drop(buf);
		}
		else
		{
			drop(buf);
			pop();
			if (flags & STREAM_MUST)
				error_out("not found", file);
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

static char *input(void)
{
	static char	input[CHUNK];  /* input buffer */
	char		*e;

	assert(state.sp);
	if (!fgets(input, sizeof input, state.sp->fp) || !*input)
	{
		if (ferror(state.sp->fp))
			error_out("read error", NULL);
		return NULL;
	}
	state.sp->line++;
	if (*(e = input + strlen(input) - 1) == '\n')
		*e = 0;
	else if (e == input + sizeof input - 2)
		error_out("line too long", NULL);
	return input;
}

/*
 * nice trace header for shell actions, with optional explanation
 */

static void print_nice_hdr(Rule_t *r)
{
	char	*fname, *rname, *rnamepre;
	size_t	len;
	if (!r)
		return;
	fname = state.sp->file;
	rname = r->name;
	rnamepre = "";
	/* mamfile path: make relative to %{PACKAGEROOT} */
	if (*fname == '/' && (len = state.packagerootlen)
	&& strncmp(fname, state.packageroot, len) == 0 && fname[len] == '/' && fname[++len])
		fname += len;
	/* rule name: change install root path prefix back to '%{INSTALLROOT}' for brevity */
	if (*rname == '/' && (len = state.installrootlen)
	&& strncmp(rname, state.installroot, len) == 0 && rname[len] == '/' && rname[len + 1])
		rname += len, rnamepre = "%{INSTALLROOT}";
	fprintf(stderr, "\n# %s: %u-%u: make %s%s\n",
		fname, r->line, r->endline, rnamepre, rname);
	/* -e option */
	if (state.explain)
		fprintf(stderr, "# reason: target %s\n",
			r->flags & RULE_preexisted ? "older than prerequisites" :
				r->flags & RULE_virtual ? "is virtual" : "not found");
}

/*
 * check and propagate results of shell action
 */

static void check_shellaction(Rule_t *r, int e)
{
	struct stat	fstat;
	if (e)						/* action failed? */
		error_making(r, e);
	else if (r->flags & RULE_virtual)		/* rule does not generate a file? */
		;
	else if (status(NULL, 0, r->name, &fstat))	/* target file exists? */
	{
		if (fstat.st_mtime <= r->origtime && (r->flags & (RULE_exists | RULE_dontcare)) == RULE_exists && state.strict >= 5)
			error_making(r, -2);		/* "target not updated" */
		r->time = fstat.st_mtime;
		r->flags |= RULE_exists;
	}
	else if (!(r->flags & RULE_dontcare))
		error_making(r, -1);			/* "target not generated" */
}

/*
 * convert waitpid(3) termination info to exit status
 */

static int p_exitstatus(int pstat)
{
	if (WIFSIGNALED(pstat))
		return WTERMSIG(pstat) & 0x80;		/* signal number with 8th bit set (a.k.a. +128) */
	else if (WIFEXITED(pstat))
		return WEXITSTATUS(pstat);
	return 0x80;					/* should not happen */
}

/*
 * flag==0: wait for and reap a rule's background process, if one is running
 * flag==WNOHANG: do nothing if the process is still running
 */

static void reap(Rule_t *r, int flag)
{
	int		pstat;

	if (!r || !r->pid || waitpid(r->pid, &pstat, flag) < 1)
		return;
	if (state.debug <= -4)
	{
		char b[32];
		snprintf(b, sizeof b, "reaping PID %lu", (unsigned long)r->pid);
		report(-4, r->name, b, r);
	}
	/* dump saved-up log output */
	if (r->logtmp)
	{
		FILE	*logf;
		char	b[CHUNK];
		size_t	s;
		print_nice_hdr(r);
		if (!(logf = fopen(r->logtmp, "r")))
			report(3, r->logtmp, "log gone", r);
		while ((s = fread(b, 1, CHUNK, logf)) > 0)
			fwrite(b, 1, s, stdout);
		fclose(logf);
		fflush(stdout);
		unlink(r->logtmp);
		free(r->logtmp);
		r->logtmp = NULL;
	}
	r->pid = 0;
	assert(state.jobs > 0);
	state.jobs--;
	check_shellaction(r, p_exitstatus(pstat));
}

/*
 * reap one rule's bg job (if any) via walk()
 */

static int wreap(Dict_item_t *item)
{
	reap(item->value, 0);
	return 0;
}

/*
 * reap one rule's bg job (if any, and if finished) via walk()
 */

static int wreap_nowait(Dict_item_t *item)
{
	reap(item->value, WNOHANG);
	return 0;
}

/*
 * SIGCHLD handling (initialised in main())
 * just a dummy to make it not ignored
 */

static sigset_t empty_sigmask;

static void sigchld_dummy(int sig)
{
	assert(sig == SIGCHLD);
}

/*
 * pass shell action argv[2] to ${SHELL:-/bin/sh}
 * argv[4, 5, ...] become $1, $2, ... in the shell
 * the -c wrapper ensures that scripts are run in the selected shell
 * even on systems that otherwise demand #! magic (can you say Cygwin)
 */

static int execute_v(Rule_t *r, char **argv)
{
	pid_t	pid;
	int	pstat;

	if (!state.shell && (!(state.shell = getval(state.vars, "SHELL")) || !strcmp(state.shell, sh)))
		state.shell = sh;
	pid = fork();
	if (pid < 0)
		error_out(strerror(errno), "fork() failed");
	if (pid == 0)
	{	/* child */
		argv[0] = "sh", argv[1] = "-c", argv[3] = state.id;
		report(-5, argv[2], "exec", NULL);
		execv(state.shell, argv);
		if (errno == ENOENT)
			_exit(127);
		_exit(126);
	}
	/* parent */
	/* run job in background if wanted & possible */
	if (r && PARALLEL(r))
	{
		/* if we're at maxjobs, wait for some to finish */
		assert(state.jobs <= state.maxjobs);
		while (state.jobs == state.maxjobs)
		{
			sigsuspend(&empty_sigmask);
			walk(state.rules, wreap_nowait);
		}
		/* let it run in parallel */
		state.jobs++;
		r->pid = pid;
		return -1;
	}
	/* good old-fashioned sequential execution */
	waitpid(pid, &pstat, 0);
	return p_exitstatus(pstat);
}

static int execute(Rule_t *r, char *s)
{
	char	*argv[5];
	argv[2] = s, argv[4] = NULL;
	return execute_v(r, argv);
}

/*
 * run action s to update r
 */

static void run(Rule_t *r, char *s)
{
	Rule_t	*q;
	char	*t;
	int	c, i, j, x;
	Buf_t	*buf;

	if (r->flags & RULE_error)
		return;
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
		/* have the shell redirect parallel job output to a temp file */
		if (PARALLEL(r) && !state.chaos)
		{
			static unsigned	serial;
			char		logtmp[32];
			snprintf(logtmp, sizeof logtmp, ".mamake.%u.out", serial++);
			append(buf, "exec >");
			append(buf, logtmp);
			append(buf, " 2>&1\n");
			assert(r->logtmp == NULL);
			r->logtmp = duplicate(logtmp);
		}
		/* stubs for backward compat */
		if (!state.strict)
			append(buf,
				"alias silent=\n"
				"ignore() { env \"$@\" || :; }\n"
			);
		/* find commands in the current working directory first */
		append(buf,
			"case $PATH in\n"
			".:* | :*)	;;\n"
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
			 * (plus other code prepended above, but it should not contain anything viewpathable) */
			char	*pre = use(buf);
			size_t	prelen = strlen(pre), slen_endzero = strlen(s) + 1;
			if (!(tofree = malloc(prelen + slen_endzero)))
				out_of_memory();
			memcpy(tofree, pre, prelen);
			memcpy(tofree + prelen, s, slen_endzero);
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
			if ((q = getval(state.rules, t)) && q->path && !(q->flags & RULE_generated))
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
		int e = execute(r, s);
		if (e >= 0)
			check_shellaction(r, e);
	}
	else
	{
		fprintf(stdout, "%s\n", s);
		if (state.debug)
			fflush(stdout);
		r->time = time(NULL); /* now */
		r->flags |= RULE_exists;
	}
	drop(buf);
}

/*
 * include file in rule r
 */

static int include(Rule_t *r, Makestate_t *stp, char *file, int pushflags)
{
	int	rv;
	if (rv = push(file, NULL, pushflags))
	{
		report(-1, file, "include", NULL);
		state.indent++;
		make(r, stp);
		state.indent--;
		report(-1, file, "end of", NULL);
		pop();
	}
	return rv;
}

/*
 * generate (if necessary) and read the MAM probe information
 * done on the first `setv CC ...'
 */

static void probe(Rule_t *r, Makestate_t *stp)
{
	char		*cc, *s, *cmd;
	unsigned long	h;
	time_t		cmd_time, output_time;
	Buf_t		*buf;
	struct stat	st;

	if (!(cc = getval(state.vars, "CC")))
		cc = "cc";
	buf = buffer();
	append(buf, state.installroot), append(buf, "/bin/mamprobe");
	cmd = duplicate(use(buf));
	reap(getval(state.rules, cmd), 0);  /* we may need to wait for mamprobe to be generated */
	cmd_time = stat(cmd, &st) ? 0 : st.st_mtime;
	if (stat(cmd, &st) < 0)
		error_out("not found", cmd);
	cmd_time = st.st_mtime;
	/* make a hash from $CC's absolute path */
	if (*cc != '/')
	{
		append(buf, "command -v "), append(buf, cc);
		push("", popen(use(buf), "r"), STREAM_PIPE);
		if (!(cc = input()) || *cc != '/')
			error_out("cc not found", cc);
		pop();
	}
	for (h = 0, s = cc; *s; s++)
		h = h * 0x63c63cd9L + *s + 0x9c39c33dL;
	/* use the hash as the file name */
	append(buf, state.installroot);
	append(buf, "/lib/probe/C/mam/");
	for (h &= 0xffffffffL; h; h >>= 4)
		add(buf, "0123456789ABCDEF"[h & 0xf]);
	s = use(buf);
	/* generate probe info if it is nonexistent or older than mamprobe */
	output_time = stat(s, &st) ? 0 : st.st_mtime;
	if (output_time < cmd_time || !include(r, stp, s, 0))
	{
		Buf_t	*tmp = buffer();
		append(tmp, cmd);
		add(tmp, ' ');
		append(tmp, s);
		add(tmp, ' ');
		append(tmp, cc);
		if (execute(NULL, use(tmp)))
			error_out("cannot generate probe info", s);
		drop(tmp);
		include(r, stp, s, STREAM_MUST);
	}
	free(cmd);
	drop(buf);
}

/*
 * add attributes in s to r
 */

static void attributes(Rule_t *r, char *s)
{
	char	*t;
	size_t	n;

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
			else if (state.strict < 4 && n == 8 && !strncmp(t, "implicit", n))
				flag = RULE_dontcare;
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
			error_out("unknown attribute", t);
		}
		else if (state.strict)
			report(1, "deprecated", t, NULL);
	}
}

/*
 * append libNAME.a to the given buffer
 */

void append_ar_name(Buf_t *buf, char *name)
{
	append(buf, "lib");
	append(buf, name);
	append(buf, ".a");
}

/*
 * define %{mam_libX} for library reference lib
 *
 * lib is expected to be in the format "-lX"
 */

static char *require(char *lib, int dontcare)
{
	char		*s, *r, varname[64];

	if (strlen(lib + 2) > sizeof varname - sizeof LIB_VARPREFIX)
		error_out("-lname too long", lib);
	snprintf(varname, sizeof varname, LIB_VARPREFIX "%s", lib + 2);

	if (!(r = getval(state.vars, varname)))
	{
		Buf_t		*buf = buffer(), *tmp = buffer();
		int		c, tofree = 0;
		FILE		*f;
		struct stat	st;

		for (c = 0; ; c++)
		{
			append_ar_name(buf, lib + 2);
			r = use(buf);
			if (!stat(r, &st))
				break;
			if (c)
			{
				r = lib;
				break;
			}
			append(buf, state.installroot);
			append(buf, "/lib/");
		}
		if (r != lib)
		{
			tofree = 1;
			r = duplicate(r);
		}
		setval(state.vars, varname, r);
		append(tmp, lib + 2);
		append(tmp, ".req");
		if (!(f = fopen(use(tmp), "r")))
		{
			append(tmp, state.installroot);
			append(tmp, "/lib/lib/");
			append(tmp, lib + 2);
			f = fopen(use(tmp), "r");
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
			append(tmp, "echo 'int main(void){return 0;}' > libtest.$$.c\n");
			if (!(s = getval(state.vars, "CC")))
				error_out("variable not defined", "CC");
			append(tmp, s), add(tmp, ' ');
			if (s = getval(state.vars, "CCFLAGS"))
				append(tmp, s), add(tmp, ' ');
			append(tmp, "-o libtest.$$.x libtest.$$.c ");
			append(tmp, r);
			append(tmp, " >/dev/null 2>&1\n"
				"c=$?\n"
				"exec rm -rf libtest.$$.* &\n"  /* also remove artefacts like *.dSYM dir (macOS) */
				"exit $c\n");
			if (execute(NULL, use(tmp)))
			{
				if (tofree)
					free(r);
				r = "";
			}
		}
		r = duplicate(r);
		setval(state.vars, varname, r);
		report(-4, r, varname, NULL);
		drop(tmp);
		drop(buf);
	}
	return r;
}

/*
 * update %{<}, %{^} and %{?}
 */

static void update_allprev(Rule_t *r, char *all, char *upd)
{
	char		*name = r->name;
	size_t		n = strlen(name) + 1, nn;
	/* set %{<} */
	auto_prev->value = reduplicate(auto_prev->value, name);
	/* restore %{^}, append to it */
	if (nn = strlen(all))
		(all = realloc(all, nn + n + 1)) && (all[nn++] = ' ');
	else
		all = malloc(n);
	if (!all)
		out_of_memory();
	memcpy(all + nn, name, n);
	auto_allprev->value = all;
	/* restore %{?}, append to it if rule was updated */
	if (r->flags & RULE_updated)
	{
		if (nn = strlen(upd))
			(upd = realloc(upd, nn + n + 1)) && (upd[nn++] = ' ');
		else
			upd = malloc(n);
		if (!upd)
			out_of_memory();
		memcpy(upd + nn, name, n);
	}
	auto_updprev->value = upd;
}

/*
 * propagate last-modified timestamp and error flag from child rule to current rule
 */

static void propagate(Rule_t *q, Rule_t *r, time_t *modtime)
{
	report(-5, q->name, "propagate", q);
	if (!(q->flags & RULE_ignore) && *modtime < q->time)
		*modtime = q->time;
	if (r && (q->flags & RULE_error))
		r->flags |= RULE_error;
}

/*
 * wait for all background jobs, then exit with their highest status or e
 */

static void exit_wait(int e)
{
	state.keepgoing = 1;
	walk(state.rules, wreap);
	if (state.exitstatus > e)
		e = state.exitstatus;
	exit(e);
}


/*
 * input() until `done r'
 *
 * This function is called recursively for both 'make' and 'loop'. The parentstate
 * parameter is nonzero while processing a loop, in which case parentstate
 * variables are passed on from the caller and updated in the caller on return.
 */

static void make(Rule_t *r, Makestate_t *parentstate)
{
	Makestate_t	st;	/* shareable state */
	char		*s;
	char		*u;	/* command name */
	char		*t;	/* argument word */
	char		*v;	/* operand string */
	Rule_t		*q;	/* new rule */
	Buf_t		*buf;	/* scratch buffer */
	Buf_t		*line;	/* expanded input line */

	if (parentstate)
		st = *parentstate;
	else
	{
		memset(&st, 0, sizeof st);
		r->making++;
		if (r->flags & RULE_active)
			state.active++;
		if (*r->name)
		{
			if (!(r->flags & RULE_virtual))
			{
				bindfile(r);
				st.modtime = r->time;
			}
			report(-1, r->name, "make", r);
			state.indent++;
		}
	}
	buf = buffer();
	line = buffer();
	/*
	 * Parse lines
	 */
	while (s = input())
	{
		/* skip initial whitespace and empty line */
		for (; isspace(*s); s++);
		if (!*s)
			continue;
		/* expand all variables, except in comments */
		if (!(s[0] == 'n' && s[1] == 'o' && s[2] == 't' && s[3] == 'e'))
			s = expand(line, s);
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
			error_out("not a command name", u);
		switch (KEY(u[0], u[1], u[2], u[3]))
		{
		case KEY('b','i','n','d'):
			if (!(t[0] == '-' && t[1] == 'l'))
				error_out("bad -lname", t);
			/* make sure it's finished linking before calling require() */
			append_ar_name(buf, t + 2);
			if (q = getval(state.rules, use(buf)))
			{
				reap(q, 0);
				propagate(q, r, &st.modtime);
			}
			if (s = require(t, !strcmp(v, "dontcare")))
			{
				char *s1, *s2, *s3;
				char *libname = t + 2;
				/*
				 * bind to the *.a files that require() just derived from $INSTALLROOT/lib/lib/NAME
				 */
				for (;;)
				{
					Rule_t	*rp;
					for (t = s; *s && !isspace(*s); s++);
					if (*s)
						*s = 0;
					else
						s = 0;
					/* only bother if t is a path to a *.a we built (i.e. not -l...) */
					if (t[0] && (t[0] != '-' || t[1] != 'l'))
					{
						rp = rule(t);
						attributes(rp, v);
						bindfile(rp);
						propagate(rp, r, &st.modtime);
						report(-1, rp->name, "bind: file", rp);
					}
					if (!s)
						break;
					for (*s++ = ' '; isspace(*s); s++);
				}
				/*
				 * read library header dependency rules from $INSTALLROOT/lib/mam/NAME
				 *
				 * ...but not for a library that was just made in the same Mamfile; its header
				 * dependencies will already have been made as part of building that library
				 */
				if (q && (q->flags & RULE_made))
					continue;
				/*
				 * The _hdrdeps_libNAME_ rule is generated by mkdeps; if its
				 * name is changed below, mkdeps.sh must be changed to match!
				 * If it has already been made...
				 */
				append(buf, "_hdrdeps_lib");
				append(buf, libname);
				add(buf, '_');
				if ((q = getval(state.rules, use(buf))) && (q->flags & RULE_made))
				{
					/* ...then do a 'prev _hdrdeps_libNAME_' */
					report(-2, q->name, "bind: prev", q);
					propagate(q, r, &st.modtime);
					continue;
				}
				/* otherwise, include the rules file if it exists */
				/* save automatic variables */
				s1 = auto_prev->value, s2 = auto_allprev->value, s3 = auto_updprev->value;
				auto_prev->value = auto_allprev->value = auto_updprev->value = empty;
				/* read the file */
				append(buf, state.installroot);
				append(buf, "/lib/mam/");
				append(buf, libname);
				include(r, &st, use(buf), 0);
				/* restore automatic variables */
				if (auto_prev->value != empty)
					free(auto_prev->value);
				if (auto_allprev->value != empty)
					free(auto_allprev->value);
				if (auto_updprev->value != empty)
					free(auto_updprev->value);
				auto_prev->value = s1, auto_allprev->value = s2, auto_updprev->value = s3;
			}
			continue;

		case KEY('d','o','n','e'):
			r->endline = state.sp->line;
			if (parentstate)
			{
				/* loop block done */
				if (*t)
					error_out("syntax error", u);
				break;
			}
			/* make block done */
			if (*t)
			{	/* target is optional; use it for sanity check if present */
				q = rule(t);
				if (q != r && (t[0] != '$' || state.strict))
				{
					append(buf,q->name);
					append(buf," != ");
					append(buf,r->name);
					error_out("mismatched done statement", use(buf));
				}
				if (*v)
				{
					if (state.strict)
						report(state.strict >= 2 ? 3 : 1, "attributes obsolete, move to 'make'", u, NULL);
					attributes(r, v);
				}
			}
			/* wait for all the child rules' bg jobs to finish, then propagate their timestamps and error flags */
			while (st.bg)
			{
				List_t	*prev = st.bg;
				reap(st.bg->rule, 0);
				propagate(st.bg->rule, r, &st.modtime);
				st.bg = st.bg->next;
				free(prev);
			}
			if (st.cmd && state.active && (state.force || r->time < st.modtime || !r->time && !st.modtime))
			{
				/* flag for -e */
				if (r->time)
					r->flags |= RULE_preexisted;
				/* show a nice trace header */
				if ((!PARALLEL(r) || state.chaos) && !(r->flags & RULE_error))
					print_nice_hdr(r);
				/* run the shell action */
				run(r, use(st.cmd));
				if (!r->pid)
					propagate(r, NULL, &st.modtime);
				r->flags |= RULE_updated;
			}
			else if (st.modtime > r->parenttime && r->flags & RULE_generated)
			{
				/* if we didn't generate the target in this run, but it's newer than the parent
				 * target, then the generation of the parent target was probably interrupted
				 * and then resumed in this run, so include this target in %{?} for consistency */
				r->flags |= RULE_updated;
			}
			r->flags |= RULE_made;
			if (!(r->flags & (RULE_dontcare|RULE_error|RULE_exists|RULE_generated|RULE_virtual)))
				error_making(r, 0);
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
				if (st.cmd)
					add(st.cmd, '\n');
				else
					st.cmd = buffer();
				append(st.cmd, v);
			}
			/* if a shim is buffered, get it ready and reset the buffer */
			if (getsize(state.shim_buf))
			{
				state.shim = use(state.shim_buf);
				/* a single 'shim -' deactivates the shim */
				if (*state.shim == '\n' && !state.shim[1])
					state.shim = NULL;
			}
			continue;

		case KEY('i','n','c','l'):
			if (!*t || *v)
				error_out("syntax error", u);
			if (!strchr(t,'/'))
			{
				if (include(r, &st, t, 0))
					continue;
				append(buf, state.packageroot);
				append(buf, "/src/cmd/INIT/include/");
				append(buf, t);
				t = use(buf);
			}
			include(r, &st, t, STREAM_MUST);
			continue;

		case KEY('l','o','o','p'):
		{
			off_t		saveoff;
			char		*w, *nextw, *cp, *save_value;
			Dict_item_t	*vnode;
			unsigned int	saveline = state.sp->line;

			if (!*v)
				error_out("syntax error", u);
			/* remember current offset for repeated reading */
			if ((saveoff = ftello(state.sp->fp)) < 0)
				error_out("unseekable input", u);
			/* iterate through one or more whitespace-separated words */
			vnode = search(state.vars, t, 1);
			save_value = vnode->value;
			vnode->value = empty;
			for (w = v; w; w = nextw)
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
				if (w != v)
				{
					if (fseeko(state.sp->fp, saveoff, SEEK_SET) < 0)
						error_out("fseek failed", u);
					state.sp->line = saveline;
				}
				/* (re)read the loop block until 'done', in the context of the current rule */
				make(r, &st);
			}
			vnode->value = save_value;
			continue;
		}

		case KEY('m','a','k','e'):
		{
			char *save_making = auto_making->value;
			char *save_allprev = auto_allprev->value;
			char *save_updprev = auto_updprev->value;
			if ((q = getval(state.rules, t)) && (q->flags & RULE_made))
				report(state.strict < 3 ? 1 : 3, "rule already made", t, NULL);
			if (!q)
				q = rule(t);
			/* set %{@}; empty %{?}, %{^} and %{<} */
			auto_making->value = q->name;
			auto_updprev->value = empty;
			auto_allprev->value = empty;
			auto_prev->value = reduplicate(auto_prev->value, empty);
			if (q->making)
				report(state.strict < 3 ? 1 : 3, "rule already being made", t, NULL);
			else
			{
				/* make the target */
				attributes(q, v);
				q->parenttime = r->time;
				make(q, NULL);
				if (q->pid)
				{
					/* add the rule to the bg list */
					List_t	*j = malloc(sizeof(List_t));
					if (!j)
						out_of_memory();
					j->rule = q;
					j->next = st.bg;
					st.bg = j;
				}
				else
					propagate(q, r, &st.modtime);
			}
			/* update %{<}, restore/update %{^} and %{?} */
			if (auto_allprev->value != empty)
				free(auto_allprev->value);
			if (auto_updprev->value != empty)
				free(auto_updprev->value);
			update_allprev(q, save_allprev, save_updprev);
			/* restore %{@} */
			auto_making->value = save_making;
			continue;
		}

		case KEY('m','a','k','p'):
		case KEY('p','r','e','v'):
		{
			const int makp = (u[0] == 'm');
			q = getval(state.rules, t);
			if (!q && !makp && !state.strict)
				q = rule(t); /* for backward compat */
			else if (!q && (makp || state.strict < 4))
			{
				/* declare a simple source file prerequisite */
				attributes(q = rule(t), v);
				if (!(q->flags & RULE_virtual))
				{
					bindfile(q);
					if (!(q->flags & (RULE_dontcare | RULE_exists)))
						error_making(q, 0);
					propagate(q, r, &st.modtime);
				}
				q->flags |= RULE_made;
				report(-2, q->name, "makp", q);
			}
			else if (makp)
				error_out(t, q->flags & RULE_made ? "rule already made" : "rule already being made");
			else if (!q)
				error_out(t, "prev: rule not made");
			else if (*v && state.strict)
				error_out(v, "prev: attributes not allowed");
			else if (q->making)
				report(state.strict < 3 ? 1 : 3, "rule already being made", t, NULL);
			else
			{	/* we may need to wait for it to finish processing */
				reap(q, 0);
				report(-2, q->name, "prev", q);
				propagate(q, r, &st.modtime);
			}
			/* update %{<}, %{^} and %{?} */
			update_allprev(q, auto_allprev->value, auto_updprev->value);
			continue;
		}

		case KEY('s','e','t','v'):
			if (!getval(state.vars, t))
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
				v = duplicate(v);
				setval(state.vars, t, v);
				if (strcmp(t, "MAMAKE_STRICT") == 0)
					state.strict = *v ? atoi(v) : 1;
			}
			if (!state.probed && strcmp(t, "CC") == 0)
			{
				state.probed = 1;
				probe(r, &st);
			}
			continue;

		case KEY('s','h','i','m'):
			state.shim = NULL;
			/* add line of code to shim buffer */
			if (*v)
				append(state.shim_buf, v);
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
			error_out("unknown command", u);
		}
		break;
	}
	drop(buf);
	drop(line);
	if (parentstate)
	{
		*parentstate = st;
		return;
	}
	if (st.cmd)
		drop(st.cmd);
	r->time = st.modtime;
	if (*r->name)
	{
		state.indent--;
		report(-1, r->name, "done", r);
	}
	if (r->flags & RULE_active)
		state.active--;
	r->making--;
}

/*
 * verify that active targets were made
 */

static int verify(Dict_item_t *item)
{
	Rule_t	*r = item->value;

	if ((r->flags & (RULE_active|RULE_error|RULE_made)) == RULE_active)
		error_making(r, 0);
	return 0;
}

/*
 * return 1 if name is an initializer
 */

static int initializer(char *name)
{
	char	*s;

	if (s = strrchr(name, '/'))
		s++;
	else
		s = name;
	return !strncmp(s, "INIT", 4);
}

/*
 * update recursion leaf r and its prerequisites
 */

static int update(Rule_t *r)
{
	List_t	*x;
	Buf_t	*buf;
	char	*args = getval(state.vars, "MAMAKEARGS");
	int	testing = !strcmp(args, "test");

	/* topological sort */
	r->flags |= RULE_made;
	if (r->leaf)
		r->leaf->flags |= RULE_made;
	for (x = r->prereqs; x; x = x->next)
		if (x->rule->leaf && !(x->rule->flags & RULE_made))
			update(x->rule);

	buf = buffer();

	/* announce */
	{
		char    *p;
		append(buf, state.pwd);
		add(buf, '/');
		append(buf, r->name);
		/* show path relative to ${INSTALLROOT} */
		if (strncmp(p = use(buf), state.installroot, state.installrootlen) == 0)
			p += state.installrootlen + 1;
		fprintf(stderr, "\n# ... %sing %s ...\n", testing ? "test" : "mak", p);
		if (state.explain)
			fprintf(stderr, "# reason: recursion\n");
	}

	/* do */
	append(buf, "$MAMAKE_DEBUG_PREFIX ");
	append(buf, getval(state.vars, "MAMAKE"));
	append(buf, " -C ");
	append(buf, r->name);
	add(buf, ' ');
	append(buf, args);
	run(r, use(buf));
	drop(buf);
	return 0;
}

/*
 * scan Mamfile prereqs
 */

static int scan(Dict_item_t *item)
{
	Rule_t	*r = item->value;
	char	*s, *t;
	Rule_t	*q;
	Buf_t	*buf;

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
			if ((q = getval(state.leaf, t)) && q != r)
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

static int descend(Dict_item_t *item)
{
	Rule_t	*r = item->value;

	if (!state.active && (!(r->flags & RULE_active) || !(r = getval(state.leaf, r->name))))
		return 0;
	return r->leaf && !(r->flags & RULE_made) ? update(r) : 0;
}

/*
 * append the non-leaf active targets to state.opt
 */

static int active(Dict_item_t *item)
{
	Rule_t *r = item->value;

	if (r->flags & RULE_active)
	{
		if (r->leaf || getval(state.leaf, r->name))
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

static void recurse(void)
{
	char		*s, *t;
	Rule_t		*r;
	Buf_t		*buf, *tmp;
	struct stat	st;

	/*
	 * first determine the MAM subdirs
	 */

	tmp = buffer();
	buf = buffer();
	state.exec = !state.never;
	state.leaf = dictionary();
	append(buf, "ls -d ");
	append(buf, state.recurse);
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
			setval(state.leaf, t, r);
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
		walk(state.rules, active);
	}
	setval(state.vars, "MAMAKEARGS", duplicate(use(state.opt) + 1));

	/*
	 * scan the Mamfile and descend
	 */

	walk(state.rules, scan);
	while (state.view)
	{
		View_t *prev = state.view;
		state.view = state.view->next;
		free(prev);
	}
	walk(state.rules, descend);
}

/*
 * -------- MAIN --------
 */

int main(int argc, char **argv)
{
	char		**e, *s, *t, *v;
	Buf_t		*tmp;
	int		c;

	NOT_USED(argc);

	/*
	 * initialize the state
	 */

	state.id = argv[0];
	state.active = 1;
	state.exec = 1;
	state.file = mamfile;
	state.opt = buffer();
	state.shim_buf = buffer();
	state.rules = dictionary();
	state.vars = dictionary();
	setval(state.vars, "MAMAKE", *argv);

	/*
	 * parse the options
	 */

#if _PACKAGE_ast
	error_info.id = state.id;
	for (;;)
	{
		switch (optget(argv, usage))
		{
		case 'c':
			append(state.opt, " -c");
			state.chaos = 1;
			continue;
		case 'e':
			append(state.opt, " -e");
			state.explain = 1;
			continue;
		case 'i':
			append(state.opt, " -i");
			state.ignore = 1;
			continue;
		case 'j':
			append(state.opt, " -j");
			append(state.opt, opt_info.arg);
			state.maxjobs = opt_info.num;
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
		case 'V':
			return !(write(1, id + 10, strlen(id) - 12) > 0 && putchar('\n') == '\n');
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
			if (state.debug > 0)
				state.debug = 0;
			continue;
		case 'G':
			append(state.opt, " -G");
			setval(state.vars, "-debug-symbols", "1");
			continue;
		case 'S':
			append(state.opt, " -S");
			setval(state.vars, "-strip-symbols", "1");
			continue;
		case '?':
			/* self-doc: write to standard output */
			error(ERROR_USAGE|ERROR_OUTPUT, STDOUT_FILENO, "%s", opt_info.arg);
			return 0;
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
				setval(state.vars, s - 1, v);
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
			case 'c':
				append(state.opt, " -c");
				state.chaos = 1;
				continue;
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
				setval(state.vars, "-debug-symbols", "1");
				continue;
			case 'S':
				append(state.opt, " -S");
				setval(state.vars, "-strip-symbols", "1");
				continue;
			case 'V':
				return !(write(1, id + 10, strlen(id) - 12) > 0 && putchar('\n') == '\n');
			case 'f':
			case 'j':
			case 'r':
			case 'C':
			case 'D':
				t = s;
				if (!*++s && !(s = *++argv))
				{
					report(2, "option value expected", t, NULL);
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
					case 'j':
						append(state.opt, " -j");
						append(state.opt, s);
						state.maxjobs = atoi(s);
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
						if (state.debug > 0)
							state.debug = 0;
						break;
					}
				break;
			default:
				report(2, "unknown option", s, NULL);
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
	if (state.recurse)
		state.maxjobs = 0;

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
				setval(state.vars, s, t + 1);
				*t = '=';
				break;
			}
		}
	}
	if (!(state.installroot = getval(state.vars, "INSTALLROOT")) || *state.installroot != '/')
		error_out("variable not defined", "INSTALLROOT");
	state.installrootlen = strlen(state.installroot);
	if (!(state.packageroot = getval(state.vars, "PACKAGEROOT")) || *state.packageroot != '/')
		error_out("variable not defined", "PACKAGEROOT");
	state.packagerootlen = strlen(state.packageroot);

	/*
	 * initialize the automatic variables
	 */

	auto_making = search(state.vars, "@", 1);
	auto_prev = search(state.vars, "<", 1);
	auto_allprev = search(state.vars, "^", 1);
	auto_updprev = search(state.vars, "?", 1);
	auto_making->value = auto_prev->value = auto_allprev->value = auto_updprev->value = empty;

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
				setval(state.vars, s, v);
				tmp = buffer();
				append(tmp, s);
				append(tmp, ".FORCE");
				setval(state.vars, use(tmp), v);
				drop(tmp);
				*t = c;
				break;
			}
		}
		if (!*t)
		{
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
		error_out("cannot change working directory", NULL);
	view();

	/*
	 * recursion drops out here
	 */

	if (state.recurse)
	{
		recurse();
		return state.exitstatus;
	}

	/*
	 * set up SIGCHLD handling for parallel processing
	 */

	if (SA_RESTART && state.maxjobs > 1)
	{
		struct sigaction act;
		sigemptyset(&empty_sigmask);
		act.sa_handler = sigchld_dummy;
		sigemptyset(&act.sa_mask);
		sigaddset(&act.sa_mask, SIGCHLD);
		act.sa_flags = SA_NOCLDSTOP | SA_RESTART;
		sigaction(SIGCHLD, &act, NULL);
	}

	/*
	 * read the mamfile(s) and bring the targets up to date
	 */

	setval(state.vars, "MAMAKEARGS", duplicate(use(state.opt) + 1));
	push(state.file, NULL, STREAM_MUST);
	make(rule(""), NULL);
	pop();

	/*
	 * verify that active targets were made
	 */

	if (!state.active && !state.verified)
	{
		state.keepgoing = 1;
		walk(state.rules, verify);
	}

	/*
	 * done
	 */

	return state.exitstatus;
}

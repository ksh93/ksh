/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1992-2014 AT&T Intellectual Property          *
*             Copyright (c) 2025 Contributors to ksh 93u+m             *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  David Korn <dgk@research.att.com>                   *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/

static const char usage[] =
"[-1c?\n@(#)$Id: grep (ksh 93u+m) 2025-05-05 $\n]"
#if STANDALONE
"[-author?Glenn Fowler <gsf@research.att.com>]"
"[-author?Doug McIlroy <doug@research.bell-labs.com>]"
"[-copyright?(c) 1992-2014 AT&T Intellectual Property]"
"[-copyright?(c) 2025 Contributors to ksh 93u+m]"
"[-license?https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html]"
#ifndef ERROR_CATALOG
#define ERROR_CATALOG "libcmd"
#endif
#else
"[--plugin?ksh]"
#endif /* STANDALONE */
"[--catalog?" ERROR_CATALOG "]"
"[+NAME?grep - search lines in files for matching patterns]"
"[+DESCRIPTION?\bgrep\b searches the named input files for lines that "
    "contain a match for the given \apattern\a. Matching lines are "
    "printed by default. The standard input is searched if neither the "
    "\b-r\b option nor any input files are given, or when the file "
    "\b-\b is specified.]"
"[+?\bgrep\b supports eight kinds of \apattern\a, specified by the options "
    "below. Two may also be specified by command name (deprecated):]"
    "{"
        "[+egrep?Equivalent to \bgrep -E\b.]"
        "[+fgrep?Equivalent to \bgrep -F\b.]"
    "}"
"[G:basic-regexp?Use POSIX basic regular expression \apattern\as (default).]"
"[E:extended-regexp?Use POSIX extended regular expression \apattern\as.]"
"[X:augmented-regexp?Use AST augmented regular expression \apattern\as.]"
"[P:perl-regexp?Use \bperl\b(1) regular expression \apattern\as.]"
"[01:sysv-regexp?Use old-style UNIX System V regular expression "
    "\apattern\as. Implies \b-O\b.]"
"[S:sh-regexp?Use POSIX \bsh\b(1) file match \apattern\as. Implies \b-x\b.]"
"[K:ksh-regexp?Use \bksh\b(1) extended file match \apattern\as. Implies \b-x\b.]"
"[F:fixed-string?Use fixed string \apattern\as.]"
"[A:after-context?Equivalent to \b--context=,\b\alines\a.]:?[lines:=2]"
"[B:before-context?Equivalent to \b--context=\b\alines\a,.]:?[lines:=2]"
"[C:context?Set the matched line context \abefore\a and \aafter\a count. "
    "If ,\aafter\a is omitted then it is set to \abefore\a. By default only "
    "matched lines are printed.]:?[before[,after]]:=2,2]"
"[c:count?Only print a matching line count for each file.]"
"[e:expression|pattern|regexp?Specify a matching \apattern\a. More than "
    "one \apattern\a implies alternation. If this option is specified then "
    "the command line \apattern\a must be omitted.]: [pattern]"
"[f:file?Each line in \apattern-file\a is a \apattern\a, placed into a "
    "single alternating expression.]: [pattern-file]"
"[H:filename|with-filename?Prefix each matched line with the containing "
    "file name.]"
"[h:no-filename?Suppress containing file name prefix for each matched "
    "line.]"
"[i:ignore-case?Ignore case when matching.]"
"[l:files-with-matches?Only print file names with at least one match.]"
"[L:files-without-matches?Only print file names with no matches.]"
"[v:invert-match|revert-match?Invert the \apattern\a match sense.]"
"[m:label?All patterns must be of the form \alabel\a:\apattern\a. Match "
    "and count output will be prefixed by the corresponding \alabel\a:. At "
    "most one label is output for each line; if more than one label matches "
    "a line then it is undefined what label is output.]"
"[O:lenient?Enable lenient \apattern\a interpretation. Disables \b-p\b. "
    "This is the default, unless the \bgetconf(1)\b configuration variable "
    "\bCONFORMANCE\b is set to \bstandard\b.]"
"[x:line-match|line-regexp?Force \apattern\as to match complete lines.]"
"[n:number|line-number?Prefix each matched line with its line number.]"
"[N:name?Set the standard input file name prefix to "
    "\aname\a.]:[name:=empty]"
"[o:only-matching?Print only the non-empty matching parts of matching lines, "
    "each part on a separate line.]"
"[p:strict|pedantic?Enable strict \apattern\a interpretation with "
    "diagnostics. Disables \b-O\b. "
    "Automatically enabled if the \bgetconf(1)\b configuration variable "
    "\bCONFORMANCE\b is set to \bstandard\b.]"
"[q:quiet|silent?Do not print matching lines.]"
"[r|R:recursive?Recursively process all files in each named directory. "
/* TODO: uncomment this if/when we backport tw
    "Use \btw -e\b \aexpression\a \bgrep ...\b to control the directory "
    "traversal."
*/
    "]"
"[s:suppress|no-messages?Suppress error and warning messages.]"
"[t:total?Only print a single matching line count for all files.]"
"[w:word-match|word-regexp?Force \apattern\as to match complete words.]"
"[a?Ignored for GNU compatibility.]"
"[02:highlight|color|colour?Highlight matches "
    "using the ANSI terminal bold sequence. "
    "If \awhen\a is \bauto\b, highlight "
    "if the standard output is on a terminal. "
    "If \awhen\a is \balways\b, always highlight. "
    "If \awhen\a is \bnever\b, never highlight.]"
    ":?[when:=auto]"
"\n"
"\n[ pattern ] [ file ... ]\n"
"\n"
"[+DIAGNOSTICS?Exit status 0 if matches were found, 1 if no matches were "
    "found, where \b-v\b inverts the exit status. Exit status 2 for other "
    "errors that are accompanied by a message on the standard error.]"
"[+SEE ALSO?\bed\b(1), \bsed\b(1), \bperl\b(1), "
    /* "\btw\b(1), "  //TODO: uncomment this if/when we backport tw */
    "\bregex\b(3)]"
;

#include "cmd.h"
#include <ctype.h>
#include <ccode.h>
#include <error.h>
#include <fts.h>
#include <regex.h>
#include <vmalloc.h>
#include "context.h"

/*
 * snarfed from Doug McIlroy's C++ version
 *
 * this grep is based on the Posix re package.
 * unfortunately it has to have a nonstandard interface.
 * 1. fgrep does not have usual operators. REG_LITERAL
 * caters for this.
 * 2. grep allows null expressions, hence REG_NULL.
 * 3. it may be possible to combine the multiple 
 * patterns of grep into single patterns.
 * 4. anchoring by -x has to be done separately from
 * compilation (remember that fgrep has no ^ or $ operator),
 * hence REG_LEFT|REG_RIGHT.  (An honest, but slow alternative:
 * run regexec with REG_NOSUB off and nmatch=1 and check
 * whether the match is full length)
 */

struct State_s;
typedef struct State_s State_t;

typedef struct Item_s			/* list item			*/
{
	struct Item_s*	next;		/* next in list			*/
	uintmax_t	hits;		/* labeled pattern matches	*/
	uintmax_t	total;		/* total hits			*/
	char		string[1];	/* string value			*/
} Item_t;

typedef struct List_s			/* generic list			*/
{
	Item_t*		head;		/* list head			*/
	Item_t*		tail;		/* list tail			*/
} List_t;

struct State_s				/* program state		*/
{
	regdisc_t	redisc;		/* regex discipline		*/
	regex_t		re;		/* main compiled re		*/

	Vmalloc_t*	vm;		/* allocation region		*/

	Item_t*		hit;		/* label for most recent match	*/

	Sfio_t*		tmp;		/* tmp re compile string	*/

	List_t		files;		/* pattern file list		*/
	List_t		patterns;	/* pattern list			*/
	List_t		labels;		/* labelled re list		*/

	regmatch_t	posvec[1];	/* match position vector	*/
	regmatch_t*	pos;		/* match position pointer	*/
	int		posnum;		/* number of match positions	*/

	int		after;		/* # lines to list after match	*/
	int		before;		/* # lines to list before match	*/
	int		list;		/* list files with hits		*/
	regflags_t	options;	/* regex options		*/

	unsigned char	any;		/* if any pattern hit		*/
	unsigned char	notfound;	/* some input file not found	*/

	unsigned char	count;		/* count number of hits		*/
	unsigned char	label;		/* all patterns labelled	*/
	unsigned char	match;		/* match sense			*/
	unsigned char	only;		/* only print matching parts	*/
	unsigned char	query;		/* return status but no output	*/
	unsigned char	number;		/* line numbers			*/
	unsigned char	prefix;		/* print file prefix		*/
	unsigned char	suppress;	/* no unopenable file messages	*/
	unsigned char	words;		/* word matches only		*/
};

static void*
labelcomp(const regex_t* re, const char* s, size_t len, regdisc_t* disc)
{
	const char*	e = s + len;
	uintmax_t	n;

	n = 0;
	while (s < e)
		n = (n << 3) + (*s++ - '0');
	return (void*)((uintptr_t)n);
}

static int
labelexec(const regex_t* re, void* data, const char* xstr, size_t xlen, const char* sstr, size_t slen, char** snxt, regdisc_t* disc)
{
	((State_t*)disc)->hit = (Item_t*)data;
	return 0;
}

static int
addre(State_t* state, char* s)
{
	int		c;
	int		r;
	char*		b;
	Item_t*		x;

	x = 0;
	r = -1;
	b = s;
	if (state->label)
	{
		if (!(s = strchr(s, ':')))
		{
			error(2, "%s: label:pattern expected", b);
			goto done;
		}
		c = s - b;
		s++;
		if (!(x = vmnewof(state->vm, 0, Item_t, 1, c)))
		{
			error(ERROR_SYSTEM|2, "out of memory (pattern `%s')", b);
			goto done;
		}
		if (c)
			memcpy(x->string, b, c);
		x->string[c] = 0;
	}
	if (sfstrtell(state->tmp))
		sfputc(state->tmp, '\n');
	if (state->words)
	{
		if (!(state->options & REG_AUGMENTED))
			sfputc(state->tmp, '\\');
		sfputc(state->tmp, '<');
	}
	sfputr(state->tmp, s, -1);
	if (state->words)
	{
		if (!(state->options & REG_AUGMENTED))
			sfputc(state->tmp, '\\');
		sfputc(state->tmp, '>');
	}
	if (x)
	{
		b = (state->options & (REG_AUGMENTED|REG_EXTENDED)) ? "" : "\\";
		sfprintf(state->tmp, "%s(?{%I*o})", b, sizeof(ptrdiff_t), (intptr_t)x);
		if (state->labels.tail)
			state->labels.tail = state->labels.tail->next = x;
		else
			state->labels.head = state->labels.tail = x;
	}
	state->any = 1;
	r = 0;
 done:
	if (r && x)
		vmfree(state->vm, x);
	return r;
}

static int
addstring(State_t* state, List_t* p, char* s)
{
	Item_t*	x;

	if (!(x = vmnewof(state->vm, 0, Item_t, 1, strlen(s))))
	{
		error(ERROR_SYSTEM|2, "out of memory (string `%s')", s);
		return -1;
	}
	strcpy(x->string, s);
	if (p->head)
		p->tail->next = x;
	else
		p->head = x;
	p->tail = x;
	return 0;
}

static int
compile(State_t* state)
{
	int	line = 0;
	int	c;
	int	r;
	size_t	n;
	char*	s;
	char*	t;
	char*	file = NULL;
	Item_t*	x;
	Sfio_t*	f = NULL;

	r = 1;
	if (!(state->tmp = sfstropen()))
	{
		error(ERROR_SYSTEM|2, "out of memory");
		goto done;
	}
	for (x = state->patterns.head; x; x = x->next)
		if (addre(state, x->string))
			return r;
	file = error_info.file;
	line = error_info.line;
	f = 0;
	for (x = state->files.head; x; x = x->next)
	{
		s = x->string;
		if (!(f = sfopen(NULL, s, "r")))
		{
			error(ERROR_SYSTEM|2, "%s: cannot open", s);
			r = 2;
			goto done;
		}
		error_info.file = s;
		error_info.line = 0;
		while (s = (char*)sfreserve(f, SFIO_UNBOUND, SFIO_LOCKR))
		{
			if (!(n = sfvalue(f)))
				break;
			if (s[n - 1] != '\n')
			{
				for (t = s + n; t > s && *--t != '\n'; t--);
				if (t == s)
				{
					sfread(f, s, 0);
					break;
				}
				n = t - s + 1;
			}
			s[n - 1] = 0;
			if (addre(state, s))
				goto done;
			s[n - 1] = '\n';
			sfread(f, s, n);
		}
		while ((s = sfgetr(f, '\n', 1)) || (s = sfgetr(f, '\n', -1)))
		{
			error_info.line++;
			if (addre(state, s))
				goto done;
		}
		error_info.file = file;
		error_info.line = line;
		sfclose(f);
		f = 0;
	}
	if (!state->any)
	{
		error(2, "no pattern");
		goto done;
	}
	state->any = 0;
	if (!(s = sfstruse(state->tmp)))
	{
		error(ERROR_SYSTEM|2, "out of memory");
		goto done;
	}
	error(-1, "RE ``%s''", s);
	state->re.re_disc = &state->redisc;
	if (state->label)
	{
		state->redisc.re_compf = labelcomp;
		state->redisc.re_execf = labelexec;
	}
	if (c = regcomp(&state->re, s, state->options))
	{
		regfatal(&state->re, 2, c);
		goto done;
	}
	if (!state->label)
	{
		if (!(state->hit = vmnewof(state->vm, 0, Item_t, 1, 0)))
		{
			error(ERROR_SYSTEM|2, "out of memory");
			goto done;
		}
		state->labels.head = state->labels.tail = state->hit;
	}
	r = 0;
 done:
	error_info.file = file;
	error_info.line = line;
	if (f)
		sfclose(f);
	if (state->tmp)
		sfstrclose(state->tmp);
	return r;
}

static int
hit(State_t* state, const char* prefix, int sep, int line, const char* s, size_t len)
{
	regmatch_t*		pos;

	static const char	bold[] =	{CC_esc,'[','1','m'};
	static const char	normal[] =	{CC_esc,'[','0','m'};

	state->hit->hits++;
	if (state->query || state->list)
		return -1;
	if (!state->count)
	{
	another:
		if ((pos = state->pos) && (state->before || state->after) && (regnexec(&state->re, s, len, state->posnum, state->pos, 0) == 0) != state->match)
		{
			if (state->only)
				return 0;
			pos = 0;
		}
		if (state->prefix)
			sfprintf(sfstdout, "%s%c", prefix, sep);
		if (state->number && line)
			sfprintf(sfstdout, "%d%c", line, sep);
		if (state->label)
			sfprintf(sfstdout, "%s%c", state->hit->string, sep);
		if (!pos)
			sfwrite(sfstdout, s, len + 1);
		else if (state->only)
		{
			sfwrite(sfstdout, s + state->pos[0].rm_so, state->pos[0].rm_eo - state->pos[0].rm_so);
			sfputc(sfstdout, '\n');
			s += state->pos[0].rm_eo;
			if ((len -= state->pos[0].rm_eo) && !regnexec(&state->re, s, len, state->posnum, state->pos, 0))
				goto another;
		}
		else
		{
			do
			{
				sfwrite(sfstdout, s, state->pos[0].rm_so);
				sfwrite(sfstdout, bold, sizeof(bold));
				sfwrite(sfstdout, s + state->pos[0].rm_so, state->pos[0].rm_eo - state->pos[0].rm_so);
				sfwrite(sfstdout, normal, sizeof(normal));
				s += state->pos[0].rm_eo;
				if (!(len -= state->pos[0].rm_eo))
					break;
			} while (!regnexec(&state->re, s, len, state->posnum, state->pos, 0));
			sfwrite(sfstdout, s, len + 1);
		}
	}
	return 0;
}

static int
list(Context_line_t* lp, int show, int group, void* handle)
{
	if (group)
		sfputr(sfstdout, "--", '\n');
	return hit((State_t*)handle, error_info.file, show ? ':' : '-', lp->line, lp->data, lp->size - 1);
}

static int
execute(State_t* state, Sfio_t* input, char* name, Shbltin_t* context)
{
	char*		s;
	char*		file;
	Item_t*		x;
	size_t		len;
	int		result;
	int		line;

	int		r = 1;
	
	if (!name)
		name = "(standard input)"; /* posix! (ast prefers /dev/stdin) */
	file = error_info.file;
	error_info.file = name;
	line = error_info.line;
	error_info.line = 0;
	if (state->before || state->after)
	{
		Context_t*	cp;
		Context_line_t*	lp;

		if (!(cp = context_open(input, state->before, state->after, list, state)))
		{
			error(2, "context_open() failed");
			goto bad;
		}
		while (lp = context_line(cp))
		{
			if ((result = regnexec(&state->re, lp->data, lp->size - 1, state->posnum, state->pos, 0)) && result != REG_NOMATCH)
			{
				regfatal(&state->re, 2, result);
				goto bad;
			}
			if ((result == 0) == state->match)
				context_show(cp);
		}
		context_close(cp);
	}
	else
	{
		for (;;)
		{
			if (sh_checksig(context))
				goto bad;
			error_info.line++;
			if (s = sfgetr(input, '\n', 0))
				len = sfvalue(input) - 1;
			else if (s = sfgetr(input, '\n', -1))
			{
				len = sfvalue(input);
				s[len] = '\n';
			}
			else if (sferror(input) && errno != EISDIR)
			{
				error(ERROR_SYSTEM|2, "read error");
				goto bad;
			}
			else
				break;
			if ((result = regnexec(&state->re, s, len, state->posnum, state->pos, 0)) && result != REG_NOMATCH)
			{
				regfatal(&state->re, 2, result);
				goto bad;
			}
			if ((result == 0) == state->match && hit(state, name, ':', error_info.line, s, len) < 0)
				break;
		}
	}
	error_info.file = file;
	error_info.line = line;
	x = state->labels.head;
	do
	{
		if (x->hits && state->list >= 0)
		{
			state->any = 1;
			if (state->query)
				break;
		}
		if (!state->query)
		{
			if (!state->list)
			{
				if (state->count)
				{
					if (state->count & 2)
						x->total += x->hits;
					else
					{
						if (state->prefix)
							sfprintf(sfstdout, "%s:", name);
						if (*x->string)
							sfprintf(sfstdout, "%s:", x->string);
						sfprintf(sfstdout, "%I*u\n", sizeof(x->hits), x->hits);
					}
				}
			}
			else if ((x->hits != 0) == (state->list > 0))
			{
				if (state->list < 0)
					state->any = 1;
				if (*x->string)
					sfprintf(sfstdout, "%s:%s\n", name, x->string);
				else
					sfprintf(sfstdout, "%s\n", name);
			}
		}
		x->hits = 0;
	} while (x = x->next);
	r = 0;
 bad:
	error_info.file = file;
	error_info.line = line;
	return r;
}

static int
grep(char* id, int options, int argc, char** argv, Shbltin_t* context)
{
	int	c;
	char*	s;
	char*	h;
	Sfio_t*	f;
	int	flags;
	int	r = 1;
	FTS*	fts;
	FTSENT*	ent;
	State_t	state;

	cmdinit(argc, argv, context, ERROR_CATALOG, ERROR_NOTIFY);
	flags = fts_flags() | FTS_META | FTS_TOP | FTS_NOPOSTORDER | FTS_NOSEEDOTDIR;
	memset(&state, 0, sizeof(state));
	if (!(state.vm = vmopen()))
	{
		error(ERROR_SYSTEM|ERROR_exit(2), "out of memory");
		UNREACHABLE();
	}
	/* NOTE: as grep doesn't setjmp, do NOT use error() calls that longjmp after this point -- must free memory on error */
	state.vm->options = VM_INIT | VM_FREEONFAIL;
	state.redisc.re_version = REG_VERSION;
	state.redisc.re_flags = REG_NOFREE;
	state.redisc.re_resizef = (regresize_t)vmresize;
	state.redisc.re_resizehandle = (void*)state.vm;
	state.match = 1;
	state.options = REG_FIRST|REG_NOSUB|REG_NULL|REG_DISCIPLINE|REG_MULTIPLE|options;
	if (strcmp(astconf("CONFORMANCE", NULL, NULL), "standard"))
		state.options |= REG_LENIENT;
	error_info.id = id;
	h = 0;
	fts = 0;
	while (c = optget(argv, usage)) switch (c)
	{
	/* ... regex type options ... */
	case 'G':
		/* POSIX basic regular expression (BRE) */
		state.options &= ~(REG_AUGMENTED|REG_EXTENDED|REG_CLASS_ESCAPE|REG_LITERAL|REG_REGEXP|REG_SHELL|REG_LEFT|REG_RIGHT);
		state.options |= REG_NULL;
		break;
	case 'E':
		/* POSIX extended regular expression (ERE) */
		state.options &= ~(REG_AUGMENTED|REG_LITERAL|REG_CLASS_ESCAPE|REG_REGEXP|REG_SHELL|REG_LEFT|REG_RIGHT);
		state.options |= REG_NULL|REG_EXTENDED;
		break;
	case 'X':
		/* AST augmented regular expression (ARE) */
		state.options &= ~(REG_LITERAL|REG_CLASS_ESCAPE|REG_REGEXP|REG_SHELL|REG_LEFT|REG_RIGHT);
		state.options |= REG_NULL|REG_AUGMENTED|REG_EXTENDED;
		break;
	case 'P':
		/* perl(1) regular expression */
		state.options &= ~(REG_AUGMENTED|REG_LITERAL|REG_REGEXP|REG_SHELL|REG_LEFT|REG_RIGHT);
		state.options |= REG_NULL|REG_EXTENDED|REG_CLASS_ESCAPE;
		break;
	case -1:
		/* --sysv-regexp, old UNIX System V regex -- BRE plus leniency, minus [: :] [. .] [= =] within [ ] */
		state.options &= ~(REG_AUGMENTED|REG_EXTENDED|REG_CLASS_ESCAPE|REG_LITERAL|REG_SHELL|REG_LEFT|REG_RIGHT);
		state.options |= REG_NULL|REG_REGEXP;
		break;
	case 'S':
		/* POSIX sh glob pattern (SRE) */
		state.options &= ~(REG_NULL|REG_AUGMENTED|REG_EXTENDED|REG_CLASS_ESCAPE|REG_LITERAL|REG_REGEXP);
		state.options |= REG_SHELL|REG_LEFT|REG_RIGHT;
		break;
	case 'K':
		/* ksh glob pattern (KRE) */
		state.options &= ~(REG_NULL|REG_EXTENDED|REG_CLASS_ESCAPE|REG_LITERAL|REG_REGEXP);
		state.options |= REG_AUGMENTED|REG_SHELL|REG_LEFT|REG_RIGHT;
		break;
	case 'F':
		/* fixed string */
		state.options &= ~(REG_AUGMENTED|REG_EXTENDED|REG_CLASS_ESCAPE|REG_REGEXP|REG_SHELL|REG_LEFT|REG_RIGHT);
		state.options |= REG_NULL|REG_LITERAL;
		break;
	/* ... other options ... */
	case 'A':
		if (opt_info.arg)
		{
			state.after = (int)strtol(opt_info.arg, &s, 0);
			if (*s || state.after < 0)
			{
	badafter:
				error(2, "%s: invalid after-context line count", opt_info.arg);
				goto done;
			}
		}
		else
			state.after = 2;
		break;
	case 'B':
		if (opt_info.arg)
		{
			state.before = (int)strtol(opt_info.arg, &s, 0);
			if (*s || state.before < 0)
			{
	badbefore:
				error(2, "%s: invalid before-context line count", opt_info.arg);
				goto done;
			}
		}
		else
			state.before = 2;
		break;
	case 'C':
		if (opt_info.arg)
		{
			state.before = (int)strtol(opt_info.arg, &s, 0);
			if (state.before < 0 || (*s && *s != ','))
				goto badbefore;
			state.after = (*s == ',') ? (int)strtol(s + 1, &s, 0) : state.before;
			if (*s || state.after < 0)
				goto badafter;
		}
		else
			state.before = state.after = 2;
		break;
	case 'H':
		state.prefix = opt_info.num;
		break;
	case 'L':
		state.list = -opt_info.num;
		break;
	case 'N':
		h = opt_info.arg;
		break;
	case 'O':
		state.options |= REG_LENIENT;
		break;
	case 'a':
		break;
	case 'c':
		state.count |= 1;
		break;
	case 'e':
		if (addstring(&state, &state.patterns, opt_info.arg))
			goto done;
		break;
	case 'f':
		if (addstring(&state, &state.files, opt_info.arg))
			goto done;
		break;
	case 'h':
		state.prefix = 2;
		break;
	case 'i':
		state.options |= REG_ICASE;
		break;
	case 'l':
		state.list = opt_info.num;
		break;
	case 'm':
		state.label = 1;
		break;
	case 'n':
		state.number = 1;
		break;
	case 'o':
		state.only = 1;
		state.options &= ~(REG_FIRST|REG_NOSUB);
		break;
	case 'p':
		state.options &= ~REG_LENIENT;
		break;
	case 'q':
		state.query = 1;
		break;
	case 'r':
		if (opt_info.num)
			flags &= ~FTS_TOP;
		break;
	case 's':
		state.suppress = opt_info.num;
		break;
	case 't':
		state.count |= 2;
		break;
	case 'v':
		if (state.match = !opt_info.num)
			state.options &= ~REG_INVERT;
		else
			state.options |= REG_INVERT;
		break;
	case 'w':
		state.words = 1;
		break;
	case 'x':
		state.options |= REG_LEFT|REG_RIGHT;
		break;
	case -2:
		/* --highlight|color|colour */
		s = opt_info.arg;
		if (!s || strcasecmp(s, "auto") == 0)
			c = 0;
		else if (strcasecmp(s, "always") == 0)
			c = 1;
		else if (strcasecmp(s, "never") == 0)
			c = 2;
		else
		{
			error(2, "%s: bad highlight option", s);
			goto done;
		}
		if (c == 0 && isatty(STDOUT_FILENO) || c == 1)
			state.options &= ~(REG_FIRST|REG_NOSUB);
		else
			state.options |= REG_FIRST|REG_NOSUB;
		break;
	case '?':
		/* self-doc: write to standard output */
		error(ERROR_USAGE|ERROR_OUTPUT, STDOUT_FILENO, "%s", opt_info.arg);
		r = 0;
		goto done;
	case ':':
		error(2, "%s", opt_info.arg);
		break;
	default:
		error(2, "%s: not implemented", opt_info.name);
		goto done;
	}
	argv += opt_info.index;
	if ((state.options & REG_LITERAL) && (state.options & (REG_AUGMENTED|REG_EXTENDED)))
	{
		error(2, "-F and -A or -P or -X are incompatible");
		error_info.errors++;
	}
	if ((state.options & REG_LITERAL) && state.words)
	{
		error(ERROR_SYSTEM|2, "-F and -w are incompatible");
		error_info.errors++;
	}
	if (!state.files.head && !state.patterns.head)
	{
		if (!argv[0])
		{
			error(2, "no pattern");
			error_info.errors++;
		}
		else if (addstring(&state, &state.patterns, *argv++))
			goto done;
	}
	if (error_info.errors)
	{
		error(ERROR_USAGE|2, "%s", optusage(NULL));
		r = 2;
		goto done;
	}
	if (!(state.options & (REG_FIRST|REG_NOSUB)))
	{
		if (state.count || state.list || state.query || (state.options & REG_INVERT))
			state.options |= REG_FIRST|REG_NOSUB;
		else
		{
			state.pos = state.posvec;
			state.posnum = elementsof(state.posvec);
		}
	}
	if (r = compile(&state))
		goto done;
	sfset(sfstdout, SFIO_LINE, 1);
	/* read stdin if neither args nor -r */
	if (!argv[0] && (flags & FTS_TOP))
	{
		if (state.prefix != 1)
			state.prefix = h ? 1 : 0;
		if (r = execute(&state, sfstdin, h, context))
			goto done;
	}
	if (state.prefix > 1)
		state.prefix = 0;
	else if (!(flags & FTS_TOP) || argv[1])
		state.prefix = 1;
	if (!(fts = fts_open(argv, flags, NULL)))
	{
		error(ERROR_SYSTEM|2, "%s: not found", argv[0]);
		r = 1;
		goto done;
	}
	while (!sh_checksig(context) && (ent = fts_read(fts)))
	{
		switch (ent->fts_info)
		{
		case FTS_F:
			if (f = sfopen(NULL, ent->fts_accpath, "r"))
			{
				r = execute(&state, f, ent->fts_path, context);
				sfclose(f);
				if (r)
					goto done;
				if (state.query && state.any)
					goto quit;
				break;
			}
			/*FALLTHROUGH*/
		case FTS_NS:
		case FTS_SLNONE:
			state.notfound = 1;
			if (!state.suppress)
				error(ERROR_SYSTEM|2, "%s: cannot open", ent->fts_path);
			break;
		case FTS_DC:
			error(ERROR_WARNING|1, "%s: directory causes cycle", ent->fts_path);
			break;
		case FTS_DNR:
			error(ERROR_SYSTEM|2, "%s: cannot read directory", ent->fts_path);
			break;
		case FTS_DNX:
			error(ERROR_SYSTEM|2, "%s: cannot search directory", ent->fts_path);
			break;
		}
	}
 quit:
	if ((state.count & 2) && !state.query && !state.list)
	{
		Item_t*		x;

		x = state.labels.head;
		do
		{
			if (*x->string)
				sfprintf(sfstdout, "%s:", x->string);
			sfprintf(sfstdout, "%I*u\n", sizeof(x->total), x->total);
		} while (x = x->next);
	}
	r = (state.notfound && !state.query) ? 2 : !state.any;
 done:
	if (fts)
		fts_close(fts);
	vmclose(state.vm);
	sfset(sfstdout, SFIO_LINE, 0);
	if (sfsync(sfstdout))
		error(ERROR_SYSTEM|2, "write error");
	if (sh_checksig(context))
	{
		errno = EINTR;
		r = 2;
	}
	return r;
}

int
b_grep(int argc, char** argv, Shbltin_t* context)
{
	char*	s;
	int	options;

	NoP(argc);
	options = 0;
	if (s = strrchr(argv[0], '/'))
		s++;
	else
		s = argv[0];
	switch (*s)
	{
	case 'e':
	case 'E':
		s = "egrep";
		options = REG_EXTENDED;
		break;
	case 'f':
	case 'F':
		s = "fgrep";
		options = REG_LITERAL;
		break;
	default:
		s = "grep";
		break;
	}
	return grep(s, options, argc, argv, context);
}

#if STANDALONE

int
main(int argc, char** argv)
{
	return b_grep(argc, argv, 0);
}

#else

int
b_egrep(int argc, char** argv, Shbltin_t* context)
{
	NoP(argc);
	return grep("egrep", REG_EXTENDED, argc, argv, context);
}

int
b_fgrep(int argc, char** argv, Shbltin_t* context)
{
	NoP(argc);
	return grep("fgrep", REG_LITERAL, argc, argv, context);
}

#endif /* STANDALONE */

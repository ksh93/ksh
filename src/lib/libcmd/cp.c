/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1992-2012 AT&T Intellectual Property          *
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
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
/*
 * Glenn Fowler
 * AT&T Research
 *
 * cp/ln/mv -- copy/link/move files
 */

static const char usage_head[] =
"[-?\n@(#)$Id: cp (ksh 93u+m) 2026-07-31 $\n]"
"[--catalog?" ERROR_CATALOG "]"
;

static const char usage_cp[] =
"[+NAME?cp - copy files]"
"[+DESCRIPTION?If the last argument names an existing directory, \bcp\b "
    "copies each \afile\a into a file with the same name in that directory. "
    "Otherwise, if only two files are given, \bcp\b copies the first onto "
    "the second. It is an error if the last argument is not a directory and "
    "more than two files are given. By default directories are not copied.]"

"[a:archive?Preserve as much as possible of the structure and attributes "
    "of the original files in the copy. Equivalent to \b--physical\b "
    "\b--preserve\b \b--recursive\b.]"
"[A:attributes?Preserve selected file attributes:]:[eipt]"
    "{"
	"[+e?Everything permissible.]"
	"[+i?Owner UID and GID.]"
	"[+p?Permissions.]"
	"[+t?Access and modify times.]"
    "}"
"[p:preserve?Preserve file owner, group, permissions and timestamps.]"
"[r|R:recursive?Operate on the contents of directories recursively.]"
"[x|X:xdev|local|mount|one-file-system?Do not descend into directories "
    "in other file systems than their parents.]"
"[h:hierarchy|parents?Form the name of each destination file by "
    "appending to the target directory a slash and the specified source file "
    "name. The last argument must be an existing directory. Missing "
    "destination directories are created.]"
"[H:metaphysical?Follow command argument symbolic links, otherwise don't "
    "follow.]"
"[l:link?Make hard links to destination files instead of copies.]"
"[U:remove-destination?Remove existing destination files before copying.]"
"[L:logical|dereference?Follow symbolic links and copy the files they "
    "point to.]"
"[P|d:physical|nodereference|no-dereference?Don't follow symbolic links; copy symbolic "
    "links rather than the files they point to.]"
;

static const char usage_ln[] =
"[+NAME?ln - link files]"
"[+DESCRIPTION?If the last argument names an existing directory, \bln\b "
    "links each \afile\a into a file with the same name in that directory. "
    "Otherwise, if only two files are given, \bln\b links the first onto the "
    "second. It is an error if the last argument is not a directory and more "
    "than two files are given. By default directories are not linked.]"
;

static const char usage_mv[] =
"[+NAME?mv - move or rename files]"
"[+DESCRIPTION?If the last argument names an existing directory, \bmv\b "
    "moves each \afile\a into that directory. "
    "Otherwise, if only two files are given, \bmv\b renames the first onto "
    "the second. It is an error if the last argument is not a directory and "
    "more than two files are given. Each source file or directory that "
    "resides on another file system than the destination is recursively "
    "copied to the destination and deleted. However, \bmv\b will not cross "
    "file system boundaries while traversing source directory hierarchies.]"

"[U:remove-destination?Remove existing destination files before moving.]"
;

static const char usage_tail[] =
"[f:force?Replace existing destination files.]"
"[i:interactive|prompt?Prompt whether to replace existing destination "
    "files. An affirmative response (\by\b or \bY\b) replaces the file, a "
    "quit response (\bq\b or \bQ\b) exits immediately, and all other "
    "responses skip the file.]"
"[s:symlink|symbolic-link?Make symbolic links to destination files.]"
"[u:update?Replace a destination file only if its modification time is "
    "older than the corresponding source file modification time.]"
"[v:verbose?Report each successful operation.]"
"[F:fsync|sync?\bfsync\b(2) each file after it is copied.]"
"[B:backup?Make backups of files that are about to be replaced. "
    "\b--suffix\b sets the backup suffix. The backup type is determined in "
    "this order: this option, the \bVERSION_CONTROL\b environment variable, "
    "or the default value \bexisting\b. \atype\a may be one of:]:?[type]"
    "{"
	"[+numbered|t?Always make numbered backups. The numbered backup "
	    "suffix is \b.\aSNS\a, where \aS\a is the \bbackup-suffix\b and "
	    "\aN\a is the version number, starting at 1, incremented with "
	    "each version.]"
	"[+existing|nil?Make numbered backups of files that already have "
	    "them, otherwise simple backups.]"
	"[+simple|never?Always make simple backups.]"
	"[+none|off?Disable backups.]"
    "}"
"[S:suffix?A backup file is made by renaming the file to the same name "
    "with the backup suffix appended. The backup suffix is determined in "
    "this order: this option, the \bSIMPLE_BACKUP_SUFFIX\b, environment "
    "variable, or the default value \b~\b.]:[suffix]"
"[b?\b--backup\b using the type in the \bVERSION_CONTROL\b environment "
    "variable.]"

"\n"
"\nsource destination\n"
"file ... directory\n"
"\n"

"[+SEE ALSO?\bpax\b(1), \bfsync\b(2), \blink\b(2), \bsymlink\b(2), "
    "\brename\b(2), \bremove\b(2), \brmdir\b(2), \bunlink\b(2)]"
;

#include <ast_release.h>
#include <cmd.h>
#include <ls.h>
#include <times.h>
#include <fts.h>
#include <stk.h>
#include <tmx.h>
#include <libgen.h>

#define PATH_CHUNK	256U

#define CP		0x1		/* cp(1), copy files or dirs	*/
#define LN		0x2		/* ln(1), hard link or symlink	*/
#define MV		0x4		/* mv(1), move or rename	*/
#define MV_XDEV		(0x8 | CP)	/* mv(1), cross-device move	*/

#define PRESERVE_IDS	0x1		/* preserve UID and GID		*/
#define PRESERVE_PERM	0x2		/* preserve permissions		*/
#define PRESERVE_TIME	0x4		/* preserve times		*/

#define BAK_replace	0		/* no backup -- just replace	*/
#define BAK_existing	1		/* number if already else simple*/
#define BAK_number	2		/* append .suffix number suffix	*/
#define BAK_simple	3		/* append suffix		*/

typedef struct State_s			/* program state		*/
{
	Shbltin_t*	context;	/* builtin context		*/
	int		op;		/* {CP,LN,MV,MV_XDEV}		*/
	int		preserve;	/* preserve { ids perms times }	*/
	int		backup;		/* BAK_* type			*/
	int		wflags;		/* open() for write flags	*/
	mode_t		missmode;	/* default missing dir mode	*/
	mode_t		perm;		/* permissions to preserve	*/
	uid_t		uid;		/* caller UID			*/

	char		directory;	/* destination is directory	*/
	char		force;		/* force approval		*/
	char		hierarchy;	/* preserve hierarchy		*/
	char		interactive;	/* prompt for approval		*/
	char		recursive;	/* subtrees too			*/
	char		remove;		/* remove destination before op	*/
	char		sync;		/* fsync() each file after copy	*/
	char		update;		/* replace only if newer	*/
	char		verbose;	/* list each file before op	*/

	int		(*link)(const char*, const char*);	/* link	*/
	int		(*stat)(const char*, struct stat*);	/* stat	*/

#define INITSTATE	pathsiz		/* (re)init state before this	*/
	size_t		pathsiz;	/* state.path buffer size	*/
	size_t		postsiz;	/* state.path post index	*/
	ssize_t		presiz;		/* state.path pre index		*/
	size_t		suflen;		/* strlen(state.suffix)		*/

	char*		path;		/* to pathname buffer		*/
	char*		opname;		/* state.op message string	*/
	char*		suffix;		/* backup suffix		*/

	Sfio_t*		tmp;		/* tmp string stream		*/

	char		text[PATH_MAX];	/* link text buffer		*/
} State_t;

static const char	dot[2] = { '.' };

static void noreturn
outofmemory(State_t *state)
{
	if (state->path)
	{
		free(state->path);
		state->path = NULL;
	}
	state->pathsiz = 0;
	state->context->ptr = NULL;
	free(state);
	error(ERROR_SYSTEM|ERROR_PANIC, "out of memory");
	UNREACHABLE();
}

static void
grow_path(State_t *state, size_t len)
{
	char	*tmp;

	if (state->pathsiz >= state->postsiz + len)
		return;
	state->pathsiz = roundof(state->postsiz + 2, PATH_CHUNK);
	tmp = realloc(state->path, state->pathsiz);
	if (!tmp)
		outofmemory(state);
	state->path = tmp;
}

/*
 * preserve support
 */

static void
preserve(State_t* state, const char* path, struct stat* ns, struct stat* os)
{
	int	n;

	if ((state->preserve & PRESERVE_TIME) && tmxtouch(path, tmxgetatime(os), tmxgetmtime(os), TMX_NOTIME, 0))
		error(ERROR_SYSTEM|2, "%s: cannot reset access and modify times", path);
	if (state->preserve & PRESERVE_IDS)
	{
		n = ((ns->st_uid != os->st_uid) << 1) | (ns->st_gid != os->st_gid);
		if (n && chown(state->path, os->st_uid, os->st_gid))
		{
			switch (n)
			{
			case 01:
				error(ERROR_SYSTEM|2, "%s: cannot reset group to %s", path, fmtgid(os->st_gid));
				break;
			case 02:
				error(ERROR_SYSTEM|2, "%s: cannot reset owner to %s", path, fmtuid(os->st_uid));
				break;
			case 03:
				error(ERROR_SYSTEM|2, "%s: cannot reset owner to %s and group to %s", path, fmtuid(os->st_uid), fmtgid(os->st_gid));
				break;
			}
		}
	}
}

/*
 * visit a single file and state.op to the destination
 */

static int
visit(State_t* state, FTSENT* ent)
{
	char*		base;
	int		n;
	ssize_t		len;
	int		rm = state->remove || ent->fts_info == FTS_SL;
	int		m;
	int		v;
	size_t		length;
	char*		s;
	char*		e;
	char*		protection;
	Sfio_t*		ip;
	Sfio_t*		op;
	FTS*		fts;
	FTSENT*		sub;
	struct stat	st;

	if (ent->fts_info == FTS_DC)
	{
		error(2, "%s: directory causes cycle", ent->fts_path);
		fts_set(NULL, ent, FTS_SKIP);
		return 0;
	}
	if (ent->fts_level == 0)
	{
		base = ent->fts_name;
		len = (ssize_t)ent->fts_namelen;
		if (state->hierarchy)
			state->presiz = -1;
		else
		{
			state->presiz = (ssize_t)ent->fts_pathlen;
			while (*base == '.' && *(base + 1) == '/')
				for (base += 2; *base == '/'; base++);
			if (*base == '.' && !*(base + 1))
				state->presiz--;
			else if (*base)
				state->presiz -= base - ent->fts_name;
			base = ent->fts_name + len;
			while (base > ent->fts_name && *(base - 1) == '/')
				base--;
			while (base > ent->fts_name && *(base - 1) != '/')
				base--;
			len -= base - ent->fts_name;
			if (state->directory)
				state->presiz -= len + 1;
		}
	}
	else
	{
		base = ent->fts_path + state->presiz + 1;
		len = (ssize_t)ent->fts_pathlen - state->presiz - 1;
	}
	len++;
	if (state->directory)
	{
		grow_path(state, (size_t)len);
		if (state->hierarchy && ent->fts_level == 0 && strchr(base, '/'))
		{
			s = state->path + state->postsiz;
			memcpy(s, base, (size_t)len);
			while (e = strchr(s, '/'))
			{
				*e = 0;
				if (access(state->path, F_OK))
				{
					st.st_mode = state->missmode;
					if (s = strrchr(s, '/'))
					{
						*s = 0;
						stat(state->path, &st);
						*s = '/';
					}
					if (mkdir(state->path, st.st_mode & S_IPERM))
					{
						error(ERROR_SYSTEM|2, "%s: cannot create directory -- %s ignored", state->path, ent->fts_path);
						fts_set(NULL, ent, FTS_SKIP);
						return 0;
					}
				}
				*e++ = '/';
				s = e;
			}
		}
	}
	switch (ent->fts_info)
	{
	case FTS_DP:
		/*
		 * Post-order visit of directory -- a second
		 * visit after all operations on it are done.
		 */
		if (state->op == MV_XDEV)
		{
			/*
			 * Cross-device move: we have copied and unlinked the contents of
			 * the directory, so now attempt to remove the directory itself.
			 */
			if (rmdir(ent->fts_path))
				error(ERROR_SYSTEM|2,"%s: cannot remove", ent->fts_path);
		}
		else if (state->preserve && state->op != LN || ent->fts_level > 0 && (ent->fts_statp->st_mode & S_IRWXU) != S_IRWXU)
		{
			if (len && ent->fts_level > 0)
				memcpy(state->path + state->postsiz, base, (size_t)len);
			else
				state->path[state->postsiz] = 0;
			if (stat(state->path, &st))
				error(ERROR_SYSTEM|2, "%s: cannot stat", state->path);
			else
			{
				if ((ent->fts_statp->st_mode & S_IPERM) != (st.st_mode & S_IPERM) && chmod(state->path, ent->fts_statp->st_mode & S_IPERM))
					error(ERROR_SYSTEM|2, "%s: cannot reset directory mode to %s", state->path, fmtmode(st.st_mode & S_IPERM, 0) + 1);
				if (state->preserve & (PRESERVE_IDS|PRESERVE_TIME))
					preserve(state, state->path, &st, ent->fts_statp);
			}
		}
		return 0;
	case FTS_DNR:
	case FTS_DNX:
	case FTS_D:
		if (!state->recursive)
		{
			fts_set(NULL, ent, FTS_SKIP);
			if (state->op & CP)
			{
				error(2, "%s: skipping directory", ent->fts_path);
				return 0;
			}
			else if (state->link == link && !state->force)
			{
				error(2, "%s: cannot link directory", ent->fts_path);
				return 0;
			}
		}
		else switch (ent->fts_info)
		{
		case FTS_DNR:
			error(2, "%s: cannot read directory", ent->fts_path);
			return 0;
		case FTS_DNX:
			error(2, "%s: cannot search directory", ent->fts_path);
			fts_set(NULL, ent, FTS_SKIP);
			/* FALLTHROUGH */
		case FTS_D:
			if (state->directory)
				memcpy(state->path + state->postsiz, base, (size_t)len);
			m = -1;
			if (!(*state->stat)(state->path, &st))
			{
				if (state->op == MV_XDEV)
				{
					/* Refuse to write into an existing directory other than the destination. */
					error(2, "%s: cannot %s existing directory", state->path, state->opname);
					return -1;
				}
				if (!S_ISDIR(st.st_mode))
				{
					error(2, "%s: not a directory -- %s ignored", state->path, ent->fts_path);
					return 0;
				}
			}
			else if (m = mkdir(state->path, (ent->fts_statp->st_mode & S_IPERM)|(ent->fts_info == FTS_D ? S_IRWXU : 0)))
			{
				error(ERROR_SYSTEM|2, "%s: cannot create directory -- %s ignored", state->path, ent->fts_path);
				fts_set(NULL, ent, FTS_SKIP);
			}
			if (!state->directory)
			{
				state->directory = 1;
				state->path[state->postsiz++] = '/';
				state->presiz--;
			}
			/* mv: show --verbose message on successful mkdir */
			if (state->op == MV_XDEV && m == 0)
				goto success;
			return 0;
		}
		break;
	case FTS_ERR:
	case FTS_NS:
	case FTS_SLNONE:
		if (state->link != pathsetlink)
		{
			error(2, "%s: not found", ent->fts_path);
			return 0;
		}
		break;
	}
	if (state->directory)
		memcpy(state->path + state->postsiz, base, (size_t)len);
	if ((*state->stat)(state->path, &st))
		st.st_mode = 0;
	else if (state->update && !S_ISDIR(st.st_mode) && (unsigned long)ent->fts_statp->st_mtime < (unsigned long)st.st_mtime)
	{
		fts_set(NULL, ent, FTS_SKIP);
		return 0;
	}
	else
	{
		if (state->op != LN && st.st_dev == ent->fts_statp->st_dev && st.st_ino == ent->fts_statp->st_ino)
		{
			if (state->op == MV)
			{
				/*
				 * let rename() handle it
				 */

				goto operate;
			}
			error(2, "%s: identical to %s", state->path, ent->fts_path);
			return 0;
		}
		if (S_ISDIR(st.st_mode))
		{
			error(2, "%s: cannot %s existing directory", state->path, state->opname);
			return 0;
		}
		if (!rm || !state->force)
		{
			if (S_ISLNK(st.st_mode) && (n = -1) || (n = open(state->path, O_RDWR|O_BINARY|O_cloexec)) >= 0)
			{
				if (n >= 0)
					ast_close(n);
				if (state->force)
					/* ok */;
				else if (state->interactive)
				{
					if ((n = astquery(-1, "%s %s? ", state->opname, state->path)) < 0 || sh_checksig(state->context))
						return -1;
					if (n)
						return 0;
				}
				else if (state->op == LN)
				{
					error(2, "%s: cannot %s existing file", state->path, state->opname);
					return 0;
				}
			}
			else if (state->force)
				rm = 1;
			else
			{
				protection =
#ifdef ETXTBSY
				    errno == ETXTBSY ? "``running program''" :
#endif
				    st.st_uid != state->uid ? "``not owner''" :
				    fmtmode(st.st_mode & (S_IRWXU|S_IRWXG|S_IRWXO), 0) + 1;
				if (state->interactive)
				{
					if ((n = astquery(-1, "override protection %s for %s? ", protection, state->path)) < 0 || sh_checksig(state->context))
						return -1;
					if (n)
						return 0;
					rm = 1;
				}
				else if (!rm)
				{
					error(2, "%s: cannot %s %s protection", state->path, state->opname, protection);
					return 0;
				}
			}
		}
		switch (state->backup)
		{
		case BAK_existing:
		case BAK_number:
			v = 0;
			if (s = strrchr(state->path, '/'))
			{
				e = state->path;
				*s++ = 0;
			}
			else
			{
				e = (char*)dot;
				s = state->path;
			}
			length = strlen(s);
			if (fts = fts_open((char**)e, FTS_NOCHDIR|FTS_ONEPATH|FTS_PHYSICAL|FTS_NOPOSTORDER|FTS_NOSTAT|FTS_NOSEEDOTDIR, NULL))
			{
				while (sub = fts_read(fts))
				{
					if (strneq(s, sub->fts_name, length) && sub->fts_name[length] == '.' && strneq(sub->fts_name + length + 1, state->suffix, state->suflen) && (m = (int)strtol(sub->fts_name + length + state->suflen + 1, &e, 10)) && streq(e, state->suffix) && m > v)
						v = m;
					if (sub->fts_level)
						fts_set(NULL, sub, FTS_SKIP);
				}
				fts_close(fts);
			}
			if (s != state->path)
				*--s = '/';
			if (v || state->backup == BAK_number)
			{
				sfprintf(state->tmp, "%s.%s%d%s", state->path, state->suffix, v + 1, state->suffix);
				goto backup;
			}
			/* FALLTHROUGH */
		case BAK_simple:
			sfprintf(state->tmp, "%s%s", state->path, state->suffix);
		backup:
			if (!(s = sfstruse(state->tmp)))
				outofmemory(state);
			if (rename(state->path, s))
			{
				error(ERROR_SYSTEM|2, "%s: cannot backup to %s", state->path, s);
				return 0;
			}
			break;
		default:
			if (rm && remove(state->path))
			{
				error(ERROR_SYSTEM|2, "%s: cannot remove", state->path);
				return 0;
			}
			break;
		}
	}
 operate:
	switch (state->op)
	{
	case MV:
		if (!rename(ent->fts_path, state->path))
			goto success;
		if (errno != ENOENT && st.st_mode && !remove(state->path) && !rename(ent->fts_path, state->path))
			goto success;
		error(ERROR_SYSTEM|2, "%s: cannot rename to %s", ent->fts_path, state->path);
		return 0;
	case CP:
	case MV_XDEV:
		if (S_ISLNK(ent->fts_statp->st_mode))
		{
			ssize_t l;
			if ((l = pathgetlink(ent->fts_path, state->text, sizeof(state->text) - 1)) < 0)
			{
				error(ERROR_SYSTEM|2, "%s: cannot read symbolic link text", ent->fts_path);
				return 0;
			}
			state->text[l] = 0;
			if (pathsetlink(state->text, state->path))
			{
				error(ERROR_SYSTEM|2, "%s: cannot copy symbolic link to %s", ent->fts_path, state->path);
				return 0;
			}
		}
		else if (S_ISREG(ent->fts_statp->st_mode) || S_ISDIR(ent->fts_statp->st_mode))
		{
			int	rfd = -1;
			int	wfd = -1;
			if (ent->fts_statp->st_size > 0 && (rfd = open(ent->fts_path, O_RDONLY|O_BINARY|O_cloexec)) < 0)
			{
				error(ERROR_SYSTEM|2, "%s: cannot read", ent->fts_path);
				return 0;
			}
			else if ((wfd = open(state->path, (st.st_mode ? (state->wflags & ~O_EXCL) : state->wflags)|O_cloexec, ent->fts_statp->st_mode & state->perm)) < 0)
			{
				error(ERROR_SYSTEM|2, "%s: cannot write", state->path);
				if (ent->fts_statp->st_size > 0)
					ast_close(rfd);
				return 0;
			}
			else if (ent->fts_statp->st_size > 0)
			{
				if (!(ip = sfnew(NULL, NULL, (size_t)SFIO_UNBOUND, rfd, SFIO_READ)))
				{
					error(ERROR_SYSTEM|2, "%s: %s read stream error", ent->fts_path, state->path);
					ast_close(rfd);
					ast_close(wfd);
					return 0;
				}
				if (!(op = sfnew(NULL, NULL, (size_t)SFIO_UNBOUND, wfd, SFIO_WRITE)))
				{
					error(ERROR_SYSTEM|2, "%s: %s write stream error", ent->fts_path, state->path);
					ast_close(wfd);
					sfclose(ip);
					return 0;
				}
				n = 0;
				if (sfmove(ip, op, (Sfoff_t)SFIO_UNBOUND, -1) < 0)
					n |= 3;
				if (!sfeof(ip))
					n |= 1;
				if (sfsync(op) || state->sync && fsync(wfd) || sfclose(op))
					n |= 2;
				if (sfclose(ip))
					n |= 1;
				if (n)
				{
					error(ERROR_SYSTEM|2, "%s: %s %s error", ent->fts_path, state->path, n == 1 ? ERROR_translate(0, 0, 0, "read") : n == 2 ? ERROR_translate(0, 0, 0, "write") : ERROR_translate(0, 0, 0, "io"));
					return 0;
				}
			}
			else
				ast_close(wfd);
		}
		else if ((n = S_ISFIFO(ent->fts_statp->st_mode)) || S_ISBLK(ent->fts_statp->st_mode) || S_ISCHR(ent->fts_statp->st_mode))
		{
			/* Avoid passing dev != 0 for FIFOs */
			if (mknod(state->path, ent->fts_statp->st_mode, n ? 0 : idevice(ent->fts_statp)))
			{
				error(ERROR_SYSTEM|2, "%s: cannot copy special file to %s", ent->fts_path, state->path);
				return 0;
			}
		}
		else
		{
			error(2, "%s: cannot copy -- unknown file type 0%o", ent->fts_path, S_ITYPE(ent->fts_statp->st_mode));
			return 0;
		}
		if (state->preserve)
		{
			if (ent->fts_info != FTS_SL)
			{
				if (stat(state->path, &st))
					error(ERROR_SYSTEM|2, "%s: cannot stat", state->path);
				else
				{
					if ((state->preserve & PRESERVE_PERM) && (ent->fts_statp->st_mode & state->perm) != (st.st_mode & state->perm) && chmod(state->path, ent->fts_statp->st_mode & state->perm))
						error(ERROR_SYSTEM|2, "%s: cannot reset mode to %s", state->path, fmtmode(st.st_mode & state->perm, 0) + 1);
					if (state->preserve & (PRESERVE_IDS|PRESERVE_TIME))
						preserve(state, state->path, &st, ent->fts_statp);
				}
			}
			/* For a cross-device move, delete the file after copying. */
			if (state->op == MV_XDEV && unlink(ent->fts_path))
			{
				error(ERROR_SYSTEM|1, "%s: cannot remove", ent->fts_path);
				return 0;
			}
		}
	success:
		if (state->verbose)
			sfprintf(sfstdout, "%s -> %s\n", ent->fts_path, state->path);
		break;
	case LN:
		if ((*state->link)(ent->fts_path, state->path))
			error(ERROR_SYSTEM|2, "%s: cannot link to %s", ent->fts_path, state->path);
		else if (state->verbose)
			sfprintf(sfstdout, "%s %c> %s\n", state->path, state->link == link ? '=' : '-', ent->fts_path);
		break;
	}
	return 0;
}

int
b_cp(int argc, char** argv, Shbltin_t* context)
{
	char*		dest;
	char*		s;
	char**		v;
	char*		backup_type;
	FTS*		fts;
	FTSENT*		ent;
	const char*	usage;
	int		FTS_flags = 0;  /* capitals to avoid name conflict with libast's fts_flags() */
	int		path_resolve = 0;
	int		standard;
	int		dest_exists;
	struct stat	dest_stat;
	State_t*	state;
	Shbltin_t*	sh;
	Shbltin_t*	cleanup = context;

	cmdinit(argc, argv, context, ERROR_CATALOG, ERROR_NOTIFY);
	if (!(sh = CMD_CONTEXT(context)) || !(state = (State_t*)sh->ptr))
	{
		if (!(state = newof(0, State_t, 1, 0)))
			outofmemory(state);
		if (sh)
			sh->ptr = state;
	}
	else
		memset(state, 0, offsetof(State_t, INITSTATE));
	state->context = context;
	state->presiz = -1;
	backup_type = 0;
	FTS_flags = FTS_NOCHDIR|FTS_NOSEEDOTDIR;
	state->uid = geteuid();
	state->wflags = O_WRONLY|O_CREAT|O_TRUNC|O_BINARY;
	if (!state->tmp && !(state->tmp = sfstropen()))
		outofmemory(state);
	sfputr(state->tmp, usage_head, -1);
	standard = !!conformance(0, 0);
	switch (error_info.id[0])
	{
	case 'c':
	case 'C':
		sfputr(state->tmp, usage_cp, -1);
		state->op = CP;
		state->stat = stat;
		path_resolve = -1;
		break;
	case 'l':
	case 'L':
		sfputr(state->tmp, usage_ln, -1);
		state->op = LN;
		FTS_flags |= FTS_PHYSICAL;
		state->link = link;
		state->remove = 1;
		state->stat = lstat;
		path_resolve = 1;
		break;
	case 'm':
	case 'M':
		sfputr(state->tmp, usage_mv, -1);
		state->op = MV;
		FTS_flags |= FTS_PHYSICAL;
		state->preserve = PRESERVE_IDS|PRESERVE_PERM|PRESERVE_TIME;
		state->stat = lstat;
		path_resolve = 1;
		break;
	default:
		error(3, "not implemented");
		break;
	}
	sfputr(state->tmp, usage_tail, -1);
	if (!(usage = sfstruse(state->tmp)))
		outofmemory(state);
	state->opname = state->op == CP ? ERROR_translate(0, 0, 0, "overwrite") : ERROR_translate(0, 0, 0, "replace");
	for (;;)
	{
		switch (optget(argv, usage))
		{
		case 'a':
			FTS_flags |= FTS_PHYSICAL;
			state->preserve = PRESERVE_IDS|PRESERVE_PERM|PRESERVE_TIME;
			state->recursive = 1;
			path_resolve = 1;
			continue;
		case 'A':
			s = opt_info.arg;
			for (;;)
			{
				switch (*s++)
				{
				case 0:
					break;
				case 'e':
					state->preserve |= PRESERVE_IDS|PRESERVE_PERM|PRESERVE_TIME;
					continue;
				case 'i':
					state->preserve |= PRESERVE_IDS;
					continue;
				case 'p':
					state->preserve |= PRESERVE_PERM;
					continue;
				case 't':
					state->preserve |= PRESERVE_TIME;
					continue;
				default:
					error(1, "%s=%c: unknown attribute flag", opt_info.option, *(s - 1));
					continue;
				}
				break;
			}
			continue;
		case 'b':
			state->backup = 1;
			continue;
		case 'f':
			state->force = 1;
			if (state->op != CP || !standard)
				state->interactive = 0;
			continue;
		case 'h':
			state->hierarchy = 1;
			continue;
		case 'i':
			state->interactive = 1;
			if (state->op != CP || !standard)
				state->force = 0;
			continue;
		case 'l':
			state->op = LN;
			state->link = link;
			state->stat = lstat;
			continue;
		case 'p':
			state->preserve = PRESERVE_IDS|PRESERVE_PERM|PRESERVE_TIME;
			continue;
		case 'r':
			state->recursive = 1;
			if (path_resolve < 1)
			{
				FTS_flags &= ~FTS_META;
				FTS_flags |= FTS_PHYSICAL;
				path_resolve = 1;
			}
			continue;
		case 's':
			state->op = LN;
			state->link = pathsetlink;
			state->stat = lstat;
			continue;
		case 'u':
			state->update = 1;
			continue;
		case 'v':
			state->verbose = 1;
			continue;
		case 'x':
			FTS_flags |= FTS_XDEV;
			continue;
		case 'B':
			backup_type = opt_info.arg;
			state->backup = 1;
			continue;
		case 'F':
			state->sync = 1;
			continue;
		case 'H':
			FTS_flags |= FTS_META|FTS_PHYSICAL;
			path_resolve = 1;
			continue;
		case 'L':
			FTS_flags &= ~FTS_PHYSICAL;
			path_resolve = 1;
			continue;
		case 'P':
			FTS_flags &= ~FTS_META;
			FTS_flags |= FTS_PHYSICAL;
			path_resolve = 1;
			continue;
		case 'S':
			state->suffix = opt_info.arg;
			continue;
		case 'U':
			state->remove = 1;
			continue;
		case '?':
			return optselfdoc();
		case ':':
			error(2, "%s", opt_info.arg);
			continue;
		}
		break;
	}
	argc -= opt_info.index + 1;
	argv += opt_info.index;
	if (*argv && streq(*argv, "-") && !streq(*(argv - 1), "--"))
	{
		argc--;
		argv++;
	}

	/*
	 * At this point, argc==0 for 1 arg, argc==1 for 2 args, etc.
	 */

	if (!(v = stkalloc(stkstd, (size_t)(argc + 2) * sizeof(char*))))
		outofmemory(state);
	memcpy(v, argv, (size_t)(argc + 1) * sizeof(char*));
	argv = v;
	if (!standard)
	{
		state->wflags |= O_EXCL;
		if (!argc)
		{
			argc++;
			argv[1] = (char*)dot;
		}
	}
	if (state->backup)
	{
		char *type_optarg = backup_type;
		if (!type_optarg && !(backup_type = getenv("VERSION_CONTROL")))
			state->backup = 0;
		else
		{
			/* This array must be sorted */
			static const char *opts[] =
			{
				/*  0 */  "existing",
				/*  1 */  "never",
				/*  2 */  "nil",
				/*  3 */  "none",
				/*  4 */  "numbered",
				/*  5 */  "off",
				/*  6 */  "simple",
				/*  7 */  "t"
			};
			switch (strdisabbrev(backup_type, opts, elementsof(opts), DISABBREV_SORTED))
			{
			case 0:
			case 2:
				state->backup = BAK_existing;
				break;
			case 1:
			case 6:
				state->backup = BAK_simple;
				break;
			case 3:
			case 5:
				state->backup = 0;
				break;
			case 4:
			case 7:
				state->backup = BAK_number;
				break;
			case -2:
				if (type_optarg)
					error(2, "%s: ambiguous backup type", type_optarg);
				break;
			default:
				if (type_optarg)
					error(2, "%s: unknown backup type", type_optarg);
				break;
			}
		}
		if (!state->suffix && !(state->suffix = getenv("SIMPLE_BACKUP_SUFFIX")))
			state->suffix = "~";
		state->suflen = strlen(state->suffix);
	}
	if (argc <= 0 || error_info.errors)
	{
		error(ERROR_usage(2), "%s", optusage(NULL));
		UNREACHABLE();
	}
	if (!path_resolve)
		FTS_flags |= fts_flags() | FTS_META;
	/* save the destination argument */
	dest = argv[argc];
	/* let argv contain only the source argument(s) */
	argv[argc] = 0;
	if (s = strrchr(dest, '/'))
	{
		while (*s == '/')
			s++;
		if (!(!*s || *s == '.' && (!*++s || *s == '.' && !*++s)))
			s = 0;
	}
	if (dest != (char*)dot)
		pathcanon(dest, 0, 0);
	dest_exists = stat(dest, &dest_stat) == 0;
	state->directory = dest_exists && S_ISDIR(dest_stat.st_mode);
	if (!state->directory && (s || argc > 1))
	{
		error(3, "%s: not a directory", dest);
		return 1;
	}
	if (!dest_exists)
	{
		char *dup, *parent;
		/*
		 * If the destination argument does not exist, see if its
		 * parent directory does; if so, mv will use its st_dev
		 * ID to check for the need to move across file systems.
		 */
		if (!(dup = strdup(dest)))
			outofmemory(state);
		parent = dirname(dup);
		dest_exists = stat(parent, &dest_stat) == 0;
		free(dup);
		if (!dest_exists)
		{
			/* Any operation would fail now. */
			error(2|ERROR_SYSTEM, "%s: cannot access parent", dest);
			return 1;
		}
	}
	state->postsiz = strlen(dest);
	grow_path(state, 2);
	memcpy(state->path, dest, state->postsiz + 1);
	if (state->directory && state->path[state->postsiz - 1] != '/')
		state->path[state->postsiz++] = '/';
	if (state->hierarchy)
	{
		if (!state->directory)
			error(3, "%s: last argument must be a directory", dest);
		state->missmode = dest_stat.st_mode;
	}
	state->perm = state->uid ? S_IPERM : (S_IPERM & ~S_ISVTX);
	if (!state->recursive)
		FTS_flags |= FTS_TOP;

	/*
	 * Main operation starts here.
	 */

	if (state->op == MV)
	{
		struct stat src_stat;
		int i, save_flags, save_preserve;
		char save_recursive;
		char *av[2];
#if _AST_release
		const int force_xdev = 0;
#else
		/* for regression testing */
		const int force_xdev = getenv("_LIBCMD_MV_FORCE_XDEV") != NULL;
#endif
		/*
		 * For mv, each source file/directory may or may not need to
		 * be copied and removed due to being on another file system
		 * than the destination, so each source argument may require
		 * different fts(3) traversal flags. Therefore, we must open
		 * a separate fts stream for each one.
		 */
		av[1] = NULL;
		for (i = 0; argv[i]; i++)
		{
			av[0] = argv[i];
			if (stat(av[0], &src_stat) == -1)
			{
				if (errno == ENOENT)
					error(2, "%s: not found", av[0]);
				else
					error(ERROR_SYSTEM|2, "%s: cannot move", av[0]);
				continue;
			}
			if (src_stat.st_dev != dest_stat.st_dev || force_xdev)
			{
				/* we have a cross-device move */
				save_flags = FTS_flags;
				save_preserve = state->preserve;
				save_recursive = state->recursive;
				/* set flags for 'cp -ax' equivalence plus removal */
				state->op = MV_XDEV;
				FTS_flags = FTS_PHYSICAL | FTS_NOCHDIR | FTS_NOSEEDOTDIR | FTS_XDEV;
				state->preserve = PRESERVE_IDS | PRESERVE_PERM | PRESERVE_TIME;
				state->recursive = 1;
			}
			if (fts = fts_open(av, FTS_flags, NULL))
			{
				while (!sh_checksig(context) && (ent = fts_read(fts)) && !visit(state, ent));
				fts_close(fts);
			}
			else
				error(ERROR_SYSTEM|2, "%s: cannot move", av[0]);
			if (state->op == MV_XDEV)
			{
				state->op = MV;
				FTS_flags = save_flags;
				state->preserve = save_preserve;
				state->recursive = save_recursive;
			}
			if (sh_checksig(context))
				break;
		}
	}
	else if (fts = fts_open(argv, FTS_flags, NULL))
	{
		/*
		 * Main loop for cp and ln: all the source arguments are
		 * handled in a single fts(3) run.
		 */
		while (!sh_checksig(context) && (ent = fts_read(fts)) && !visit(state, ent));
		fts_close(fts);
	}
	else if (state->link != pathsetlink)
	{
		switch (state->op)
		{
		case CP:
			error(ERROR_SYSTEM|2, "%s: cannot copy", argv[0]);
			break;
		case LN:
			error(ERROR_SYSTEM|2, "%s: cannot link", argv[0]);
			break;
		}
	}
	else if ((*state->link)(*argv, state->path))
		error(ERROR_SYSTEM|2, "%s: cannot link to %s", *argv, state->path);
	if (cleanup && !sh)
	{
		if (state->path)
			free(state->path);
		free(state);
	}
	return error_info.errors != 0;
}

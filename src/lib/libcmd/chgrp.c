/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1992-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2024 Contributors to ksh 93u+m           *
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
 * David Korn
 * Glenn Fowler
 * AT&T Research
 *
 * chgrp+chown
 */

static const char usage_1[] =
"[-?\n@(#)$Id: chgrp (ksh 93u+m) 2022-08-30 $\n]"
"[--catalog?" ERROR_CATALOG "]"
;

static const char usage_grp_1[] =
"[+NAME?chgrp - change the group ownership of files]"
"[+DESCRIPTION?\bchgrp\b changes the group ownership of each file"
"	to \agroup\a, which can be either a group name or a numeric"
"	group ID. The user ownership of each file may also be changed to"
"	\auser\a by prepending \auser\a\b:\b to the group name.]"
;

static const char usage_own_1[] =
"[+NAME?chown - change the ownership of files]"
"[+DESCRIPTION?\bchown\b changes the ownership of each file"
"	to \auser\a, which can be either a user name or a numeric"
"	user ID. The group ownership of each file may also be changed to"
"	\auser\a by appending \b:\b\agroup\a to the user name.]"
;

static const char usage_2[] =
"[b:before?Only change files with \bctime\b before (less than) the "
    "\bmtime\b of \afile\a.]:[file]"
"[c:changes?Describe only files whose ownership actually changes.]"
"[f:quiet|silent?Do not report files whose ownership fails to change.]"
"[h|l:symlink|no-dereference?Change the ownership of symbolic links on systems that "
    "support \blchown\b(2). Implies \b--physical\b.]"
"[m:map?The first operand is interpreted as a file that contains a map "
    "of space separated \afrom_uid:from_gid to_uid:to_gid\a pairs. The "
    "\auid\a or \agid\a part of each pair may be omitted to mean any \auid\a "
    "or \agid\a. Ownership of files matching the \afrom\a part of any pair "
    "is changed to the corresponding \ato\a part of the pair. The matching "
    "for each file operand is in the order \auid\a:\agid\a, \auid\a:, "
    ":\agid\a. For a given file, once a \auid\a or \agid\a mapping is "
    "determined it is not overridden by any subsequent match. Unmatched "
    "files are silently ignored.]"
"[n:show?Show actions but don't execute.]"
"[N:numeric?By default numeric user and group ID operands are first "
    "interpreted as names; if no name exists then they are interpreted as "
    "explicit numeric IDs. \b--numeric\b interprets numeric ID operands as "
    "numeric IDs.]"
"[r:reference?Omit the explicit ownership operand and use the ownership "
    "of \afile\a instead.]:[file]"
"[u:unmapped?Print a diagnostic for each file for which either the "
    "\auid\a or \agid\a or both were not mapped.]"
"[v:verbose?Describe changed permissions of all files.]"
"[H:metaphysical?Follow symbolic links for command arguments; otherwise "
    "don't follow symbolic links when traversing directories.]"
"[L:logical|follow?Follow symbolic links when traversing directories.]"
"[P:physical|nofollow?Don't follow symbolic links when traversing "
    "directories.]"
"[R:recursive?Recursively change ownership of directories and their "
    "contents.]"
"[X:test?Canonicalize output for testing.]"

"\n"
"\n"
;

static const char usage_3[] =
" file ...\n"
"\n"
"[+EXIT STATUS?]{"
	"[+0?All files changed successfully.]"
	"[+>0?Unable to change ownership of one or more files.]"
"}"
"[+SEE ALSO?\bchmod\b(1), \bchown\b(2), \btw\b(1), \bgetconf\b(1), \bls\b(1)]"
;

#define lchown		______lchown

#include <cmd.h>
#include <cdt.h>
#include <ls.h>
#include <ctype.h>
#include <fts.h>

#include "FEATURE/symlink"

#undef	lchown

typedef struct Key_s			/* UID/GID key			*/
{
	int		uid;		/* UID				*/
	int		gid;		/* GID				*/
} Key_t;

typedef struct Map_s			/* UID/GID map			*/
{
	Dtlink_t	link;		/* dictionary link		*/
	Key_t		key;		/* key				*/
	Key_t		to;		/* map to these			*/
} Map_t;

#define OPT_CHOWN	0x0001		/* chown			*/
#define OPT_FORCE	0x0002		/* ignore errors		*/
#define OPT_GID		0x0004		/* have GID			*/
#define OPT_LCHOWN	0x0008		/* lchown			*/
#define OPT_NUMERIC	0x0010		/* favor numeric IDs		*/
#define OPT_SHOW	0x0020		/* show but don't do		*/
#define OPT_TEST	0x0040		/* canonicalize output		*/
#define OPT_UID		0x0080		/* have UID			*/
#define OPT_UNMAPPED	0x0100		/* unmapped file diagnostic	*/
#define OPT_VERBOSE	0x0200		/* have UID			*/

extern int	lchown(const char*, uid_t, gid_t);

/*
 * parse UID and GID from s
 */

static void
getids(char* s, char** e, Key_t* key, int options)
{
	char*	t;
	int	n;
	int	m;
	char*	z;
	char	buf[64];

	key->uid = key->gid = -1;
	while (isspace(*s))
		s++;
	for (t = s; (n = *t) && n != ':' && n != '.' && !isspace(n); t++);
	if (n)
	{
		options |= OPT_CHOWN;
		if ((n = t++ - s) >= sizeof(buf))
			n = sizeof(buf) - 1;
		*((s = (char*)memcpy(buf, s, n)) + n) = 0;
	}
	if (options & OPT_CHOWN)
	{
		if (*s)
		{
			n = (int)strtol(s, &z, 0);
			if (*z || !(options & OPT_NUMERIC))
			{
				if ((m = struid(s)) >= 0)
					n = m;
				else if (*z)
				{
					error(ERROR_exit(1), "%s: unknown user", s);
					UNREACHABLE();
				}
			}
			key->uid = n;
		}
		for (s = t; (n = *t) && !isspace(n); t++);
		if (n)
		{
			if ((n = t++ - s) >= sizeof(buf))
				n = sizeof(buf) - 1;
			*((s = (char*)memcpy(buf, s, n)) + n) = 0;
		}
	}
	if (*s)
	{
		n = (int)strtol(s, &z, 0);
		if (*z || !(options & OPT_NUMERIC))
		{
			if ((m = strgid(s)) >= 0)
				n = m;
			else if (*z)
			{
				error(ERROR_exit(1), "%s: unknown group", s);
				UNREACHABLE();
			}
		}
		key->gid = n;
	}
	if (e)
		*e = t;
}

/*
 * NOTE: we only use the native lchown() on symlinks just in case
 *	 the implementation is a feckless stub
 */

int
b_chgrp(int argc, char** argv, Shbltin_t* context)
{
	int		options = 0;
	char*		s;
	Map_t*		m;
	FTS*		fts;
	FTSENT*		ent;
	int		i;
	Dt_t*		map = 0;
	int		logical = 1;
	int		flags;
	int		uid = -1;
	int		gid = -1;
	char*		op;
	char*		usage;
	char*		t;
	Sfio_t*		sp;
	unsigned long	before;
	Dtdisc_t	mapdisc;
	Key_t		keys[3];
	Key_t		key;
	struct stat	st;
	int		(*chownf)(const char*, uid_t, gid_t);

	cmdinit(argc, argv, context, ERROR_CATALOG, ERROR_NOTIFY);
	flags = fts_flags() | FTS_META | FTS_TOP | FTS_NOPOSTORDER | FTS_NOSEEDOTDIR;
	before = ~0;
	if (!(sp = sfstropen()))
	{
		error(ERROR_SYSTEM|3, "out of space");
		UNREACHABLE();
	}
	sfputr(sp, usage_1, -1);
	if (error_info.id[2] == 'g')
		sfputr(sp, usage_grp_1, -1);
	else
	{
		sfputr(sp, usage_own_1, -1);
		options |= OPT_CHOWN;
	}
	sfputr(sp, usage_2, -1);
	if (options & OPT_CHOWN)
		sfputr(sp, ERROR_translate(0, 0, 0, "[owner[:group]]"), -1);
	else
		sfputr(sp, ERROR_translate(0, 0, 0, "[[owner:]group]"), -1);
	sfputr(sp, usage_3, -1);
	if (!(usage = sfstruse(sp)))
	{
		error(ERROR_SYSTEM|3, "out of space");
		UNREACHABLE();
	}
	for (;;)
	{
		switch (optget(argv, usage))
		{
		case 'b':
			if (stat(opt_info.arg, &st))
			{
				error(ERROR_exit(1), "%s: cannot stat", opt_info.arg);
				UNREACHABLE();
			}
			before = st.st_mtime;
			continue;
		case 'c':
		case 'v':
			options |= OPT_VERBOSE;
			continue;
		case 'f':
			options |= OPT_FORCE;
			continue;
		case 'h':
			options |= OPT_LCHOWN;
			continue;
		case 'm':
			memset(&mapdisc, 0, sizeof(mapdisc));
			mapdisc.key = offsetof(Map_t, key);
			mapdisc.size = sizeof(Key_t);
			if (!(map = dtopen(&mapdisc, Dtset)))
			{
				error(ERROR_SYSTEM|ERROR_PANIC, "out of memory [id map]");
				UNREACHABLE();
			}
			continue;
		case 'n':
			options |= OPT_SHOW;
			continue;
		case 'N':
			options |= OPT_NUMERIC;
			continue;
		case 'r':
			if (stat(opt_info.arg, &st))
			{
				error(ERROR_exit(1), "%s: cannot stat", opt_info.arg);
				UNREACHABLE();
			}
			uid = st.st_uid;
			gid = st.st_gid;
			options |= OPT_UID|OPT_GID;
			continue;
		case 'u':
			options |= OPT_UNMAPPED;
			continue;
		case 'H':
			flags |= FTS_META|FTS_PHYSICAL;
			logical = 0;
			continue;
		case 'L':
			flags &= ~(FTS_META|FTS_PHYSICAL);
			logical = 0;
			continue;
		case 'P':
			flags &= ~FTS_META;
			flags |= FTS_PHYSICAL;
			logical = 0;
			continue;
		case 'R':
			flags &= ~FTS_TOP;
			logical = 0;
			continue;
		case 'X':
			options |= OPT_TEST;
			continue;
		case ':':
			error(2, "%s", opt_info.arg);
			continue;
		case '?':
			error(ERROR_usage(2), "%s", opt_info.arg);
			UNREACHABLE();
		}
		break;
	}
	argv += opt_info.index;
	argc -= opt_info.index;
	if (error_info.errors || argc < 2)
	{
		error(ERROR_usage(2), "%s", optusage(NULL));
		UNREACHABLE();
	}
	s = *argv;
	if (options & OPT_LCHOWN)
	{
		flags &= ~FTS_META;
		flags |= FTS_PHYSICAL;
		logical = 0;
	}
	if (logical)
		flags &= ~(FTS_META|FTS_PHYSICAL);
	if (map)
	{
		if (streq(s, "-"))
			sp = sfstdin;
		else if (!(sp = sfopen(NULL, s, "r")))
		{
			error(ERROR_exit(1), "%s: cannot read", s);
			UNREACHABLE();
		}
		while (s = sfgetr(sp, '\n', 1))
		{
			getids(s, &t, &key, options);
			if (!(m = (Map_t*)dtmatch(map, &key)))
			{
				if (!(m = stkalloc(stkstd, sizeof(Map_t))))
				{
					error(ERROR_SYSTEM|ERROR_PANIC, "out of memory [id dictionary]");
					UNREACHABLE();
				}
				m->key = key;
				m->to.uid = m->to.gid = -1;
				dtinsert(map, m);
			}
			getids(t, NULL, &m->to, options);
		}
		if (sp != sfstdin)
			sfclose(sp);
		keys[1].gid = keys[2].uid = -1;
	}
	else if (!(options & (OPT_UID|OPT_GID)))
	{
		getids(s, NULL, &key, options);
		if ((uid = key.uid) >= 0)
			options |= OPT_UID;
		if ((gid = key.gid) >= 0)
			options |= OPT_GID;
	}
	switch (options & (OPT_UID|OPT_GID))
	{
	case OPT_UID:
		s = ERROR_translate(0, 0, 0, " owner");
		break;
	case OPT_GID:
		s = ERROR_translate(0, 0, 0, " group");
		break;
	case OPT_UID|OPT_GID:
		s = ERROR_translate(0, 0, 0, " owner and group");
		break;
	default:
		s = "";
		break;
	}
	if (!(fts = fts_open(argv + 1, flags, NULL)))
	{
		error(ERROR_system(1), "%s: not found", argv[1]);
		UNREACHABLE();
	}
	while (!sh_checksig(context) && (ent = fts_read(fts)))
		switch (ent->fts_info)
		{
		case FTS_SL:
		case FTS_SLNONE:
			if (options & OPT_LCHOWN)
			{
#if _lib_lchown
				chownf = lchown;
				op = "lchown";
				goto commit;
#else
				if (!(options & OPT_FORCE))
				{
					errno = ENOSYS;
					error(ERROR_system(0), "%s: cannot change symlink owner/group", ent->fts_path);
				}
#endif
			}
			break;
		case FTS_F:
		case FTS_D:
		anyway:
			chownf = chown;
			op = "chown";
		commit:
			if ((unsigned long)ent->fts_statp->st_ctime >= before)
				break;
			if (map)
			{
				options &= ~(OPT_UID|OPT_GID);
				uid = gid = -1;
				keys[0].uid = keys[1].uid = ent->fts_statp->st_uid;
				keys[0].gid = keys[2].gid = ent->fts_statp->st_gid;
				i = 0;
				do
				{
					if (m = (Map_t*)dtmatch(map, &keys[i]))
					{
						if (uid < 0 && m->to.uid >= 0)
						{
							uid = m->to.uid;
							options |= OPT_UID;
						}
						if (gid < 0 && m->to.gid >= 0)
						{
							gid = m->to.gid;
							options |= OPT_GID;
						}
					}
				} while (++i < elementsof(keys) && (uid < 0 || gid < 0));
			}
			else
			{
				if (!(options & OPT_UID))
					uid = ent->fts_statp->st_uid;
				if (!(options & OPT_GID))
					gid = ent->fts_statp->st_gid;
			}
			if ((options & OPT_UNMAPPED) && (uid < 0 || gid < 0))
			{
				if (uid < 0 && gid < 0)
					error(ERROR_warn(0), "%s: UID and GID not mapped", ent->fts_path);
				else if (uid < 0)
					error(ERROR_warn(0), "%s: UID not mapped", ent->fts_path);
				else
					error(ERROR_warn(0), "%s: GID not mapped", ent->fts_path);
			}
			if (uid != ent->fts_statp->st_uid && uid >= 0 || gid != ent->fts_statp->st_gid && gid >= 0)
			{
				if (options & (OPT_SHOW|OPT_VERBOSE))
				{
					if (options & OPT_TEST)
					{
						ent->fts_statp->st_uid = 0;
						ent->fts_statp->st_gid = 0;
					}
					sfprintf(sfstdout, "%s uid:%05d->%05d gid:%05d->%05d %s\n", op, ent->fts_statp->st_uid, uid, ent->fts_statp->st_gid, gid, ent->fts_path);
				}
				if (!(options & OPT_SHOW) && (*chownf)(ent->fts_accpath, uid, gid) && !(options & OPT_FORCE))
					error(ERROR_system(0), "%s: cannot change%s", ent->fts_path, s);
			}
			break;
		case FTS_DC:
			if (!(options & OPT_FORCE))
				error(ERROR_warn(0), "%s: directory causes cycle", ent->fts_path);
			break;
		case FTS_DNR:
			if (!(options & OPT_FORCE))
				error(ERROR_system(0), "%s: cannot read directory", ent->fts_path);
			goto anyway;
		case FTS_DNX:
			if (!(options & OPT_FORCE))
				error(ERROR_system(0), "%s: cannot search directory", ent->fts_path);
			goto anyway;
		case FTS_NS:
			if (!(options & OPT_FORCE))
				error(ERROR_system(0), "%s: not found", ent->fts_path);
			break;
		}
	fts_close(fts);
	if (map)
		dtclose(map);
	return error_info.errors != 0;
}

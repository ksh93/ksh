/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2026 Contributors to ksh 93u+m           *
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
*                                                                      *
***********************************************************************/

#include	"FEATURE/options"
#include	<ast.h>
#include	"ulimit.h"

/*
 * This is the list of resource limits controlled by ulimit
 * This command requires getrlimit(), vlimit(), or ulimit()
 */

#ifndef _no_ulimit

const char	e_unlimited[] = "unlimited";
const char*	e_units[] = { NULL, "block", "byte", "Kibyte", "second", "microsecond" };

const int	shtab_units[] = { 1, 512, 1, 1024, 1, 1 };

const Limit_t	shtab_limits[] =
{
"as",       "address space limit",           RLIMIT_AS,         NULL,           'M', LIM_KBYTE,
"core",     "core file size",                RLIMIT_CORE,       NULL,           'c', LIM_BLOCK,
"cpu",      "cpu time",                      RLIMIT_CPU,        NULL,           't', LIM_SECOND,
"data",     "data size",                     RLIMIT_DATA,       NULL,           'd', LIM_KBYTE,
"fsize",    "file size",                     RLIMIT_FSIZE,      NULL,           'f', LIM_BLOCK,
"kqueues",  "number of kqueues",             RLIMIT_KQUEUES,    NULL,           'k', LIM_COUNT,
"locks",    "number of file locks",          RLIMIT_LOCKS,      NULL,           'x', LIM_COUNT,
"memlock",  "locked address space",          RLIMIT_MEMLOCK,    NULL,           'l', LIM_KBYTE,
"msgqueue", "message queue size",            RLIMIT_MSGQUEUE,   NULL,           'q', LIM_KBYTE,
"nice",     "scheduling priority",           RLIMIT_NICE,       NULL,           'e', LIM_COUNT,
"nofile",   "number of open files",          RLIMIT_NOFILE,     "OPEN_MAX",     'n', LIM_COUNT,
"novmon",   "number of open vnode monitors", RLIMIT_NOVMON,     NULL,           'V', LIM_COUNT,
"nproc",    "number of processes",           RLIMIT_NPROC,      "CHILD_MAX",    'u', LIM_COUNT,
"npts",     "number of pseudo-terminals",    RLIMIT_NPTS,       NULL,           'P', LIM_COUNT,
"pipe",     "pipe buffer size",              RLIMIT_PIPE,       "PIPE_BUF",     'p', LIM_BYTE,
"rss",      "max memory size",               RLIMIT_RSS,        NULL,           'm', LIM_KBYTE,
"rtprio",   "max real-time priority",        RLIMIT_RTPRIO,     NULL,           'r', LIM_COUNT,
"rttime",   "max time before blocking",      RLIMIT_RTTIME,     NULL,           'R', LIM_MICROSECOND,
"sbsize",   "socket buffer size",            RLIMIT_SBSIZE,     "PIPE_BUF",     'b', LIM_BYTE,
"sigpend",  "signal queue size",             RLIMIT_SIGPENDING, "SIGQUEUE_MAX", 'i', LIM_COUNT,
"stack",    "stack size",                    RLIMIT_STACK,      NULL,           's', LIM_KBYTE,
"swap",     "swap size",                     RLIMIT_SWAP,       NULL,           'w', LIM_KBYTE,
"threads",  "number of threads",             RLIMIT_PTHREAD,    "THREADS_MAX",  'T', LIM_COUNT,
"vmem",     "process size",                  RLIMIT_VMEM,       NULL,           'v', LIM_KBYTE,
{ 0 }
};

#endif

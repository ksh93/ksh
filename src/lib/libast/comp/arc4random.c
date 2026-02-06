/***********************************************************************
*                                                                      *
*              This file is part of the ksh 93u+m package              *
*          Copyright (c) 2023-2026 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 OpenBSD License (reproduced below).                  *
*                                                                      *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/

/*
 * Copyright (c) 1996, David Mazieres <dm@uun.org>
 * Copyright (c) 2008, Damien Miller <djm@openbsd.org>
 * Copyright (c) 2013, Markus Friedl <markus@openbsd.org>
 * Copyright (c) 2014, Theo de Raadt <deraadt@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * ChaCha based random number generator for OpenBSD.
 */

/* OPENBSD ORIGINAL: lib/libc/crypt/arc4random.c */

#include <ast.h>
#include "FEATURE/random"

#if !_lib_arc4random || !_lib_arc4random_buf || !_lib_arc4random_uniform

#ifndef AST_RANDOM_DEV
#  if defined(_pth_urandom)
#    define AST_RANDOM_DEV _pth_urandom
#  elif defined(_pth_random)
#    define AST_RANDOM_DEV _pth_random
#  elif !_lib_getentropy && !_lib_getrandom
#    error no source of secure randomness found; define AST_RANDOM_DEV
#  endif
#endif /* AST_RANDOM_DEV */

#include <error.h>
#if _sys_random
#  include <sys/random.h>
#endif
#if _sys_mman
#  include <sys/mman.h>
#endif
#include "FEATURE/mmap"

#if !_lib_explicit_bzero
/*
 * Volatile pointer to memset: we use this to keep the compiler from
 * eliminating our call to memset.
 */
void *(*volatile memset_volatile)(void*,int,size_t) = memset;
#define explicit_bzero(x,y)	((*memset_volatile)((x),0,(y)))
#endif /* !_lib_explicit_bzero */

#if !_lib_arc4random

#define KEYSTREAM_ONLY
#include "chacha_private.h"

#define minimum(a, b) ((a) < (b) ? (a) : (b))

#define KEYSZ	32
#define IVSZ	8
#define BLOCKSZ	64
#define RSBUFSZ	(16*BLOCKSZ)

#define REKEY_BASE	(1024*1024) /* NB. should be a power of 2 */

/* Marked MAP_INHERIT_ZERO, so zero'd out in fork children. */
static struct _rs
{
	size_t		rs_have;	/* valid bytes at end of rs_buf */
	size_t		rs_count;	/* bytes till reseed */
} *rs;

/* Maybe be preserved in fork children, if _rs_allocate() decides. */
static struct _rsx
{
	chacha_ctx	rs_chacha;	/* chacha context for random keystream */
	unsigned char	rs_buf[RSBUFSZ];/* keystream blocks */
} *rsx;

/*
 * Stub functions for portability.  From LibreSSL with some adaptations.
 */

static void
_rs_forkdetect(void)
{
	static pid_t _rs_pid = 0;
	pid_t pid = getpid();

	if (_rs_pid == 0 || _rs_pid == 1 || _rs_pid != pid)
	{
		_rs_pid = pid;
		if (rs)
			explicit_bzero(rs, sizeof(*rs));
	}
}

static int
_rs_allocate(struct _rs **rsp, struct _rsx **rsxp)
{
#if _mmap_anon && defined(MAP_PRIVATE)
	if ((*rsp = mmap(NULL, sizeof(**rsp), PROT_READ|PROT_WRITE,
	    MAP_ANON|MAP_PRIVATE, -1, 0)) == MAP_FAILED)
		return -1;

	if ((*rsxp = mmap(NULL, sizeof(**rsxp), PROT_READ|PROT_WRITE,
	    MAP_ANON|MAP_PRIVATE, -1, 0)) == MAP_FAILED)
	{
		munmap(*rsp, sizeof(**rsp));
		*rsp = NULL;
		return -1;
	}
#else
	if ((*rsp = calloc(1, sizeof(**rsp))) == NULL)
		return -1;
	if ((*rsxp = calloc(1, sizeof(**rsxp))) == NULL)
	{
		free(*rsp);
		*rsp = NULL;
		return -1;
	}
#endif
	return 0;
}
/*
 * Always use the getentropy implementation provided below, which will
 * call a native getentropy if available then fall back as required.
 */
static int
_ast_getentropy(void *s, size_t len)
{
	int fd;
	ssize_t r;
	size_t o = 0;

	NOT_USED(fd); NOT_USED(r); NOT_USED(o);  /* potentially */

#if _lib_getentropy
	if (getentropy(s, len) == 0)
		return 0;
#endif /* _lib_getentropy */
#if _lib_getrandom
	if ((r = getrandom(s, len, 0)) > 0 && (size_t)r == len)
		return 0;
#endif /* _lib_getrandom */
#ifdef AST_RANDOM_DEV
	if ((fd = open(AST_RANDOM_DEV, O_RDONLY)) == -1)
		return -1;
	while (o < len)
	{
		r = read(fd, (unsigned char *)s + o, len - o);
		if (r < 0)
		{
			if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK)
				continue;
			ast_close(fd);
			return -1;
		}
		o += r;
	}
	ast_close(fd);
	return 0;
#else
	return -1;
#endif /* AST_RANDOM_DEV */
}

static void
_rs_init(unsigned char *buf, size_t n)
{
	if (n < KEYSZ + IVSZ)
		return;

	if (rs == NULL)
	{
		if (_rs_allocate(&rs, &rsx) == -1)
			_exit(1);
	}

	chacha_keysetup(&rsx->rs_chacha, buf, KEYSZ * 8);
	chacha_ivsetup(&rsx->rs_chacha, buf + KEYSZ);
}

static void
_rs_rekey(unsigned char *dat, size_t datlen)
{
#ifndef KEYSTREAM_ONLY
	explicit_bzero(rsx->rs_buf, sizeof(rsx->rs_buf));
#endif
	/* fill rs_buf with the keystream */
	chacha_encrypt_bytes(&rsx->rs_chacha, rsx->rs_buf,
	    rsx->rs_buf, sizeof(rsx->rs_buf));
	/* mix in optional user provided data */
	if (dat)
	{
		size_t i, m;

		m = minimum(datlen, KEYSZ + IVSZ);
		for (i = 0; i < m; i++)
			rsx->rs_buf[i] ^= dat[i];
	}
	/* immediately reinit for backtracking resistance */
	_rs_init(rsx->rs_buf, KEYSZ + IVSZ);
	explicit_bzero(rsx->rs_buf, KEYSZ + IVSZ);
	rs->rs_have = sizeof(rsx->rs_buf) - KEYSZ - IVSZ;
}

static void
_rs_stir(void)
{
	unsigned char rnd[KEYSZ + IVSZ];
	uint32_t rekey_fuzz = 0;

	if (_ast_getentropy(rnd, sizeof rnd) == -1)
	{
		error(ERROR_SYSTEM|ERROR_PANIC,"getentropy failed");
		UNREACHABLE();
	}

	if (!rs)
		_rs_init(rnd, sizeof(rnd));
	else
		_rs_rekey(rnd, sizeof(rnd));
	explicit_bzero(rnd, sizeof(rnd));	/* discard source seed */

	/* invalidate rs_buf */
	rs->rs_have = 0;
	explicit_bzero(rsx->rs_buf, sizeof(rsx->rs_buf));

	/* rekey interval should not be predictable */
	chacha_encrypt_bytes(&rsx->rs_chacha, (uint8_t *)&rekey_fuzz,
	    (uint8_t *)&rekey_fuzz, sizeof(rekey_fuzz));
	rs->rs_count = REKEY_BASE + (rekey_fuzz % REKEY_BASE);
}

static void
_rs_stir_if_needed(size_t len)
{
	_rs_forkdetect();
	if (!rs || rs->rs_count <= len)
		_rs_stir();
	if (rs->rs_count <= len)
		rs->rs_count = 0;
	else
		rs->rs_count -= len;
}

static void
_rs_random_u32(uint32_t *val)
{
	unsigned char *keystream;

	_rs_stir_if_needed(sizeof(*val));
	if (rs->rs_have < sizeof(*val))
		_rs_rekey(NULL, 0);
	keystream = rsx->rs_buf + sizeof(rsx->rs_buf) - rs->rs_have;
	memcpy(val, keystream, sizeof(*val));
	explicit_bzero(keystream, sizeof(*val));
	rs->rs_have -= sizeof(*val);
}

uint32_t
arc4random(void)
{
	uint32_t val;

	_rs_random_u32(&val);
	return val;
}

/*
 * If we are providing arc4random, then we can provide a more efficient
 * arc4random_buf().
 */
# if !_lib_arc4random_buf
void
arc4random_buf(void *_buf, size_t n)
{
	unsigned char *buf = (unsigned char *)_buf;
	unsigned char *keystream;
	size_t m;

	_rs_stir_if_needed(n);
	while (n > 0)
	{
		if (rs->rs_have > 0)
		{
			m = minimum(n, rs->rs_have);
			keystream = rsx->rs_buf + sizeof(rsx->rs_buf)
			    - rs->rs_have;
			memcpy(buf, keystream, m);
			explicit_bzero(keystream, m);
			buf += m;
			n -= m;
			rs->rs_have -= m;
		}
		if (rs->rs_have == 0)
			_rs_rekey(NULL, 0);
	}
}
# endif /* !_lib_arc4random_buf */
#endif /* !_lib_arc4random */

/* arc4random_buf() that uses platform arc4random() */
#if !_lib_arc4random_buf && _lib_arc4random
void
arc4random_buf(void *_buf, size_t n)
{
	size_t i;
	u_int32_t r = 0;
	char *buf = (char *)_buf;

	for (i = 0; i < n; i++)
	{
		if (i % 4 == 0)
			r = arc4random();
		buf[i] = r & 0xff;
		r >>= 8;
	}
	explicit_bzero(&r, sizeof(r));
}
#endif /* !_lib_arc4random_buf && _lib_arc4random */

#if !_lib_arc4random_uniform
/* OPENBSD ORIGINAL: lib/libc/crypto/arc4random_uniform.c */
/*
 * Calculate a uniformly distributed random number less than upper_bound
 * avoiding "modulo bias".
 *
 * Uniformity is achieved by generating new random numbers until the one
 * returned is outside the range [0, 2**32 % upper_bound).  This
 * guarantees the selected random number will be inside
 * [2**32 % upper_bound, 2**32) which maps back to [0, upper_bound)
 * after reduction modulo upper_bound.
 */
uint32_t
arc4random_uniform(uint32_t upper_bound)
{
	uint32_t r, min;

	if (upper_bound < 2)
		return 0;

	/* 2**32 % x == (2**32 - x) % x */
	min = -upper_bound % upper_bound;

	/*
	 * This could theoretically loop forever but each retry has
	 * p > 0.5 (worst case, usually far better) of selecting a
	 * number inside the range we need, so it should rarely need
	 * to re-roll.
	 */
	for (;;)
	{
		r = arc4random();
		if (r >= min)
			break;
	}

	return r % upper_bound;
}
#endif /* !_lib_arc4random_uniform */

#elif _lib_arc4random && _lib_arc4random_buf && _lib_arc4random_uniform
NoN(arc4random)
#endif /* !_lib_arc4random || !_lib_arc4random_buf || !_lib_arc4random_uniform */

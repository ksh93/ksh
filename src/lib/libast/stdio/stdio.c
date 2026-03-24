/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
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

#include "stdhdr.h"

int
_doprnt(const char* fmt, va_list args, Sfio_t* f)
{
	return (int)sfvprintf(f, fmt, args);
}

int
_doscan(Sfio_t* f, const char* fmt, va_list args)
{
	return sfvscanf(f, fmt, args);
}

extern int
_filbuf(Sfio_t* f)
{
	return (int)_sffilbuf(f, 0);
}

int
asprintf(char** s, const char* fmt, ...)
{
	va_list	args;
	int	v;

	va_start(args, fmt);
	v = vasprintf(s, fmt, args);
	va_end(args);
	return v;
}

int
clearerr(Sfio_t* f)
{
	sfclrerr(f);
	return sfclrlock(f);
}

int
fclose(Sfio_t* f)
{
	return sfclose(f);
}

Sfio_t*
fdopen(int fd, const char* mode)
{
	unsigned short	flags;

	if (fd < 0 || !(flags = _sftype(mode, NULL, NULL)))
		return NULL;
	return sfnew(NULL, NULL, (size_t)SFIO_UNBOUND, fd, flags);
}

int
fgetc(Sfio_t* f)
{
	return sfgetc(f);
}

int
fgetpos(Sfio_t* f, fpos_t* pos)
{
	return (pos->_sf_offset = sfseek(f, 0, SEEK_CUR)) >= 0 ? 0 : -1;
}

Sfio_t*
fopen(const char* path, const char* mode)
{
	return sfopen(NULL, path, mode);
}

Sfio_t*
fmemopen(void* buf, size_t size, const char* mode)
{
	return sfnew(NULL, buf, size, -1, SFIO_STRING|_sftype(mode, NULL, NULL));
}

int
fprintf(Sfio_t* f, const char* fmt, ...)
{
	va_list	args;
	int	v;

	va_start(args, fmt);

	v = (int)sfvprintf(f, fmt, args);
	va_end(args);
	return v;
}

int
fpurge(Sfio_t* f)
{
	return sfpurge(f);
}

int
fputs(const char* s, Sfio_t* f)
{
	return (int)sfputr(f, s, -1);
}

size_t
fread(void* p, size_t s, size_t n, Sfio_t* f)
{
	ssize_t	v;

	return ((v = sfread(f, p, s * n)) <= 0) ? 0 : ((size_t)v / s);
}

Sfio_t*
freopen(const char* path, const char* mode, Sfio_t* f)
{
	return sfopen(f, path, mode);
}

int
fscanf(Sfio_t* f, const char* fmt, ...)
{
	va_list		args;
	int		v;

	va_start(args, fmt);

	v = sfvscanf(f, fmt, args);
	va_end(args);
	return v;
}

int
fseek(Sfio_t* f, long off, int op)
{
	return sfseek(f, (Sfoff_t)off, op|SFIO_SHARE) >= 0 ? 0 : -1;
}

int
fseeko(Sfio_t* f, off_t off, int op)
{
	return sfseek(f, (Sfoff_t)off, op|SFIO_SHARE) >= 0 ? 0 : -1;
}

int
fsetpos(Sfio_t* f, const fpos_t* pos)
{
	return sfseek(f, (Sfoff_t)pos->_sf_offset, SFIO_PUBLIC) == (Sfoff_t)pos->_sf_offset ? 0 : -1;
}

long
ftell(Sfio_t* f)
{
	return (long)sfseek(f, 0, SEEK_CUR);
}

off_t
ftello(Sfio_t* f)
{
	return sfseek(f, 0, SEEK_CUR);
}

size_t
fwrite(const void* p, size_t s, size_t n, Sfio_t* f)
{
	ssize_t	v;

	return ((v = sfwrite(f, p, s * n)) <= 0) ? 0 : ((size_t)v / s);
}

int
getw(Sfio_t* f)
{
	int	v;

	return sfread(f, &v, sizeof(v)) == sizeof(v) ? v : -1;
}

int
pclose(Sfio_t* f)
{
	return sfclose(f);
}

Sfio_t*
popen(const char* cmd, const char* mode)
{
	return sfpopen((Sfio_t*)(-1), cmd, mode);
}

int
printf(const char* fmt, ...)
{
	va_list	args;
	int	v;

	va_start(args, fmt);
	v = (int)sfvprintf(sfstdout, fmt, args);
	va_end(args);
	return v;
}

int
puts(const char* s)
{
	return (int)sfputr(sfstdout, s, '\n');
}

int
putw(int v, Sfio_t* f)
{
	return sfwrite(f, &v, sizeof(v)) == sizeof(v) ? 0 : -1;
}


void
rewind(Sfio_t* f)
{
	sfseek(f, 0, SEEK_SET|SFIO_PUBLIC);
	sfclrlock(f);
}


int
scanf(const char* fmt, ...)
{
	va_list	args;
	int	v;

	va_start(args, fmt);
	v = sfvscanf(sfstdin, fmt, args);
	va_end(args);
	return v;
}

void
setbuf(Sfio_t* f, char* b)
{
	sfsetbuf(f, b, b ? BUFSIZ : 0);
}


int
setbuffer(Sfio_t* f, char* b, int n)
{
	return sfsetbuf(f, b, (size_t)n) ? 0 : -1;
}


int
setlinebuf(Sfio_t* f)
{
	sfset(f, SFIO_LINE, 1);
	return 0;
}

int
setvbuf(Sfio_t* f, char* buf, int type, size_t size)
{
	if (type == _IOLBF)
		sfset(f, SFIO_LINE, 1);
	else if (f->flags & SFIO_STRING)
		return -1;
	else if (type == _IONBF)
	{
		sfsync(f);
		sfsetbuf(f, NULL, 0);
	}
	else if (type == _IOFBF)
	{
		if (size == 0)
			size = SFIO_BUFSIZE;
		sfsync(f);
		sfsetbuf(f, buf, size);
	}
	return 0;
}

int
snprintf(char* s, size_t n, const char* fmt, ...)
{
	va_list	args;
	int	v;

	va_start(args, fmt);
	v = (int)sfvsprintf(s, n, fmt, args);
	va_end(args);
	return v;
}

int
sprintf(char* s, const char* fmt, ...)
{
	va_list	args;
	int	v;

	va_start(args, fmt);
	v = s ? (int)sfvsprintf(s, SIZE_MAX, fmt, args) : -1;
	va_end(args);
	return v;
}


int
sscanf(const char* s, const char* fmt, ...)
{
	va_list	args;
	int	v;

	va_start(args, fmt);
	v = sfvsscanf(s, fmt, args);
	va_end(args);
	return v;
}

Sfio_t*
tmpfile(void)
{
	return sftmp(0);
}

int
ungetc(int c, Sfio_t* f)
{
	return sfungetc(f, c);
}

int
vasprintf(char** s, const char* fmt, va_list args)
{
	Sfio_t*	f;
	int	v;

	if (f = sfstropen())
	{
		v = (int)sfvprintf(f, fmt, args);
		if (!(*s = strdup(sfstruse(f))))
			v = -1;
		sfstrclose(f);
	}
	else
	{
		*s = 0;
		v = -1;
	}
	return v;
}

int
vfprintf(Sfio_t* f, const char* fmt, va_list args)
{
	return (int)sfvprintf(f, fmt, args);
}

int
vfscanf(Sfio_t* f, const char* fmt, va_list args)
{
	return sfvscanf(f, fmt, args);
}

int
vprintf(const char* fmt, va_list args)
{
	return (int)sfvprintf(sfstdout, fmt, args);
}

int
vscanf(const char* fmt, va_list args)
{
	return sfvscanf(sfstdin, fmt, args);
}

int
vsnprintf(char* s, size_t n, const char* form, va_list args)
{
	Sfio_t*	f;
	int rv;

	/* make a temp stream */
	if(!(f = sfnew(NULL,NULL,(size_t)SFIO_UNBOUND,
			-1,SFIO_WRITE|SFIO_STRING)) )
		return -1;

	if((rv = (int)sfvprintf(f,form,args)) >= 0 )
	{	if(s && n > 0)
		{	if((rv+1) >= (int)n)
				n--;
			else
				n = (size_t)rv;
			memcpy(s, f->data, n);
			s[n] = 0;
		}
		_Sfi = rv;
	}

	sfclose(f);

	return rv;
}

int
vsprintf(char* s, const char* fmt, va_list args)
{
	return vsnprintf(s, 4 * SFIO_BUFSIZE, fmt, args);
}

int
vsscanf(const char* s, const char* fmt, va_list args)
{
	return sfvsscanf(s, fmt, args);
}

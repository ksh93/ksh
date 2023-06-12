/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2012 AT&T Intellectual Property          *
*          Copyright (c) 2020-2023 Contributors to ksh 93u+m           *
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

#include	"vmhdr.h"

/*	Method to help with debugging. This does rigorous checks on
**	addresses and arena integrity.
**
**	Written by Kiem-Phong Vo, kpv@research.att.com, 01/16/94.
*/

/* structure to keep track of file names */
typedef struct _dbfile_s	Dbfile_t;
struct _dbfile_s
{	Dbfile_t*	next;
	char		file[1];
};
static Dbfile_t*	Dbfile;
	
/* global watch list */
#define S_WATCH	32
static int	Dbnwatch;
static void*	Dbwatch[S_WATCH];

/* types of warnings reported by dbwarn() */
#define	DB_CHECK	0
#define DB_ALLOC	1
#define DB_FREE		2
#define DB_RESIZE	3
#define DB_WATCH	4
#define DB_RESIZED	5

static int Dbinit = 0;
#define DBINIT()	(Dbinit ? 0 : (dbinit(), Dbinit=1) )
static void dbinit(void)
{	int	fd;	
	if((fd = vmtrace(-1)) >= 0)
		vmtrace(fd);
}

static int	Dbfd = 2;	/* default warning file descriptor */
int vmdebug(int fd)
{
	int	old = Dbfd;
	Dbfd = fd;
	return old;
}


/* just an entry point to make it easy to set break point */
static void vmdbwarn(Vmalloc_t* vm, char* mesg, int n)
{
	Vmdata_t*	vd = vm->data;

	write(Dbfd,mesg,n);
	if(vd->mode&VM_DBABORT)
		abort();
}

/* issue a warning of some type */
static void dbwarn(Vmalloc_t*	vm,	/* region holding the block	*/
		   void*	data,	/* data block			*/
		   int		where,	/* byte that was corrupted	*/
		   char*	file,	/* file where call originates	*/
		   int		line,	/* line number of call		*/
		   void*	func,	/* function called from		*/
		   int		type)	/* operation being done		*/
{
	char	buf[1024], *bufp, *endbuf, *s;
#define SLOP	64	/* enough for a message and an int */

	DBINIT();

	bufp = buf;
	endbuf = buf + sizeof(buf);

	if(type == DB_ALLOC)
		bufp = (*_Vmstrcpy)(bufp, "alloc error", ':');
	else if(type == DB_FREE)
		bufp = (*_Vmstrcpy)(bufp, "free error", ':');
	else if(type == DB_RESIZE)
		bufp = (*_Vmstrcpy)(bufp, "resize error", ':');
	else if(type == DB_CHECK)
		bufp = (*_Vmstrcpy)(bufp, "corrupted data", ':');
	else if(type == DB_WATCH)
		bufp = (*_Vmstrcpy)(bufp, "alert", ':');

	/* region info */
	bufp = (*_Vmstrcpy)(bufp, "region", '=');
	bufp = (*_Vmstrcpy)(bufp, (*_Vmitoa)(VLONG(vm), 0), ':');

	if(data)
	{	bufp = (*_Vmstrcpy)(bufp,"block",'=');
		bufp = (*_Vmstrcpy)(bufp,(*_Vmitoa)(VLONG(data),0),':');
	}

	if(!data)
	{	if(where == DB_ALLOC)
			bufp = (*_Vmstrcpy)(bufp, "can't get memory", ':');
		else	bufp = (*_Vmstrcpy)(bufp, "region is locked", ':');
	}
	else if(type == DB_FREE || type == DB_RESIZE)
	{	if(where == 0)
			bufp = (*_Vmstrcpy)(bufp, "unallocated block", ':');
		else	bufp = (*_Vmstrcpy)(bufp, "already freed", ':');
	}
	else if(type == DB_WATCH)
	{	bufp = (*_Vmstrcpy)(bufp, "size", '=');
		bufp = (*_Vmstrcpy)(bufp, (*_Vmitoa)((Vmulong_t)DBSIZE(data),-1), ':');
		if(where == DB_ALLOC)
			bufp = (*_Vmstrcpy)(bufp,"just allocated", ':');
		else if(where == DB_FREE)
			bufp = (*_Vmstrcpy)(bufp,"being freed", ':');
		else if(where == DB_RESIZE)
			bufp = (*_Vmstrcpy)(bufp,"being resized", ':');
		else if(where == DB_RESIZED)
			bufp = (*_Vmstrcpy)(bufp,"just resized", ':');
	}
	else if(type == DB_CHECK)
	{	bufp = (*_Vmstrcpy)(bufp, "bad byte at", '=');
		bufp = (*_Vmstrcpy)(bufp, (*_Vmitoa)(VLONG(where),-1), ':');
		if((s = DBFILE(data)) && (bufp + strlen(s) + SLOP) < endbuf)
		{	bufp = (*_Vmstrcpy)(bufp,"allocated at", '=');
			bufp = (*_Vmstrcpy)(bufp, s, ',');
			bufp = (*_Vmstrcpy)(bufp,(*_Vmitoa)(VLONG(DBLINE(data)),-1),':');
		}
	}

	/* location where offending call originates from */
	if(file && file[0] && line > 0 && (bufp + strlen(file) + SLOP) < endbuf)
	{	bufp = (*_Vmstrcpy)(bufp, "detected at", '=');
		bufp = (*_Vmstrcpy)(bufp, file, ',');
		bufp = (*_Vmstrcpy)(bufp, (*_Vmitoa)(VLONG(line),-1), ',');
		bufp = (*_Vmstrcpy)(bufp, (*_Vmitoa)(VLONG(func),-1), ':');
	}

	*bufp++ = '\n';
	*bufp = '\0';

	vmdbwarn(vm,buf,(int)(bufp-buf));
}

/* check for watched address and issue warnings */
static void dbwatch(Vmalloc_t* vm, void* data,
		    char* file, int line, void* func, int type)
{
	int		n;

	for(n = Dbnwatch; n >= 0; --n)
	{	if(Dbwatch[n] == data)
		{	dbwarn(vm,data,type,file,line,func,DB_WATCH);
			return;
		}
	}
}

/* record information about the block */
static void dbsetinfo(Vmuchar_t*	data,	/* real address not the one from Vmbest	*/
		      size_t		size,	/* the actual requested size		*/
		      char*		file,	/* file where the request came from	*/
		      int		line)	/* and line number			*/
{
	Vmuchar_t	*begp, *endp;
	Dbfile_t	*last, *db;

	DBINIT();

	/* find the file structure */
	if(!file || !file[0])
		db = NULL;
	else
	{	for(last = NULL, db = Dbfile; db; last = db, db = db->next)
			if(strcmp(db->file,file) == 0)
				break;
		if(!db)
		{	db = (Dbfile_t*)vmalloc(Vmheap,sizeof(Dbfile_t)+strlen(file));
			if(db)
			{	(*_Vmstrcpy)(db->file,file,0);
				db->next = Dbfile;
				Dbfile = db;
			}
		}
		else if(last) /* move-to-front heuristic */
		{	last->next = db->next;
			db->next = Dbfile;
			Dbfile = db;
		}
	}

	DBSETFL(data,(db ? db->file : NULL),line);
	DBSIZE(data) = size;
	DBSEG(data)  = SEG(DBBLOCK(data));

	DBHEAD(data,begp,endp);
	while(begp < endp)
		*begp++ = DB_MAGIC;
	DBTAIL(data,begp,endp);
	while(begp < endp)
		*begp++ = DB_MAGIC;
}

/* Check to see if an address is in some data block of a region.
** This returns -(offset+1) if block is already freed, +(offset+1)
** if block is live, 0 if no match.
*/
static long dbaddr(Vmalloc_t* vm, void* addr, int local)
{
	Block_t	*b, *endb;
	Seg_t	*seg;
	Vmuchar_t	*data;
	long	offset = -1L;
	Vmdata_t	*vd = vm->data;

	SETLOCK(vm, local);

	b = endb = NULL;
	for(seg = vd->seg; seg; seg = seg->next)
	{	b = SEGBLOCK(seg);
		endb = (Block_t*)(seg->baddr - sizeof(Head_t));
		if((Vmuchar_t*)addr > (Vmuchar_t*)b &&
		   (Vmuchar_t*)addr < (Vmuchar_t*)endb)
			break;
	}
	if(!seg)
		goto done;

	if(local) /* must be vmfree or vmresize checking address */
	{	if(DBSEG(addr) == seg)
		{	b = DBBLOCK(addr);
			if(ISBUSY(SIZE(b)) && !ISJUNK(SIZE(b)) )
				offset = 0;
			else	offset = -2L;
		}
		goto done;
	}

	while(b < endb)
	{	data = (Vmuchar_t*)DATA(b);
		if((Vmuchar_t*)addr >= data && (Vmuchar_t*)addr < data+SIZE(b))
		{	if(ISBUSY(SIZE(b)) && !ISJUNK(SIZE(b)) )
			{	data = DB2DEBUG(data);
				if((Vmuchar_t*)addr >= data &&
				   (Vmuchar_t*)addr < data+DBSIZE(data))
					offset = (long)((Vmuchar_t*)addr - data);
			}
			goto done;
		}

		b = (Block_t*)((Vmuchar_t*)DATA(b) + (SIZE(b)&~BITS) );
	}

done:
	CLRLOCK(vm, local);
	return offset;
}


static long dbsize(Vmalloc_t* vm, void* addr, int local)
{
	Block_t		*b, *endb;
	Seg_t		*seg;
	long		size;
	Vmdata_t	*vd = vm->data;

	SETLOCK(vm, local);

	size = -1L;
	for(seg = vd->seg; seg; seg = seg->next)
	{	b = SEGBLOCK(seg);
		endb = (Block_t*)(seg->baddr - sizeof(Head_t));
		if((Vmuchar_t*)addr <= (Vmuchar_t*)b ||
		   (Vmuchar_t*)addr >= (Vmuchar_t*)endb)
			continue;
		while(b < endb)
		{	if(addr == DB2DEBUG(DATA(b)))
			{	if(ISBUSY(SIZE(b)) && !ISJUNK(SIZE(b)) )
					size = (long)DBSIZE(addr);
				goto done;
			}

			b = (Block_t*)((Vmuchar_t*)DATA(b) + (SIZE(b)&~BITS) );
		}
	}

done:
	CLRLOCK(vm, local);
	return size;
}

static void* dballoc(Vmalloc_t* vm, size_t size, int local)
{
	size_t		s;
	Vmuchar_t	*data;
	char		*file;
	int		line;
	void		*func;
	Vmdata_t	*vd = vm->data;
	VMFLF(vm,file,line,func);

	SETLOCK(vm, local);

	if(vd->mode&VM_DBCHECK)
		vmdbcheck(vm);

	s = ROUND(size,ALIGN) + DB_EXTRA;
	if(s < sizeof(Body_t))	/* no tiny blocks during Vmdebug */
		s = sizeof(Body_t);

	if(!(data = (Vmuchar_t*)KPVALLOC(vm,s,(*(Vmbest->allocf))) ) )
	{	dbwarn(vm,NULL,DB_ALLOC,file,line,func,DB_ALLOC);
		goto done;
	}

	data = DB2DEBUG(data);
	dbsetinfo(data,size,file,line);

	if((vd->mode&VM_TRACE) && _Vmtrace)
	{	vm->file = file; vm->line = line; vm->func = func;
		(*_Vmtrace)(vm,NULL,data,size,0);
	}

	if(Dbnwatch > 0 )
		dbwatch(vm,data,file,line,func,DB_ALLOC);

done:
	CLRLOCK(vm, local);

	return data;
}


static int dbfree(Vmalloc_t* vm, void* data, int local )
{
	char		*file;
	int		line;
	void		*func;
	long		offset;
	int		rv, *ip, *endip;
	Vmdata_t	*vd = vm->data;
	VMFLF(vm,file,line,func);

	if(!data)
		return 0;

	SETLOCK(vm, local);

	if(vd->mode&VM_DBCHECK)
		vmdbcheck(vm);

	if((offset = KPVADDR(vm,data,dbaddr)) != 0)
	{	dbwarn(vm,(Vmuchar_t*)data,offset == -1L ? 0 : 1,file,line,func,DB_FREE);
		rv = -1;
	}
	else
	{	if(Dbnwatch > 0)
			dbwatch(vm,data,file,line,func,DB_FREE);

		if((vd->mode&VM_TRACE) && _Vmtrace)
		{	vm->file = file; vm->line = line; vm->func = func;
			(*_Vmtrace)(vm,(Vmuchar_t*)data,NULL,DBSIZE(data),0);
		}

		/* clear free space */
		ip = (int*)data;
		endip = ip + (DBSIZE(data)+sizeof(int)-1)/sizeof(int);
		while(ip < endip)
			*ip++ = 0;

		rv = KPVFREE((vm), DB2BEST(data), (*Vmbest->freef));
	}

	CLRLOCK(vm, local);
	return rv;
}

/*	Resizing an existing block */
static void* dbresize(Vmalloc_t*	vm,	/* region allocating from	*/
		      void*		addr,	/* old block of data		*/
		      size_t	size,	/* new size			*/
		      int		type,	/* !=0 for movable, >0 for copy	*/
		      int		local)
{
	Vmuchar_t	*data;
	long		offset;
	size_t		s, oldsize;
	char		*file, *oldfile;
	int		line, oldline;
	void		*func;
	Vmdata_t	*vd = vm->data;
	VMFLF(vm,file,line,func);

	if(!addr)
	{	vm->file = file; vm->line = line;
		data = (Vmuchar_t*)dballoc(vm, size, local);
		if(data && (type&VM_RSZERO) )
			memset(data, 0, size);
		return data;
	}
	if(size == 0)
	{	vm->file = file; vm->line = line;
		(void)dbfree(vm, addr, local);
		return NULL;
	}

	SETLOCK(vm, local);

	if(vd->mode&VM_DBCHECK)
		vmdbcheck(vm);

	if((offset = KPVADDR(vm,addr,dbaddr)) != 0)
	{	dbwarn(vm,(Vmuchar_t*)addr,offset == -1L ? 0 : 1,file,line,func,DB_RESIZE);
		data = NULL;
	}
	else
	{	if(Dbnwatch > 0)
			dbwatch(vm,addr,file,line,func,DB_RESIZE);

		/* Vmbest data block */
		data = DB2BEST(addr);
		oldsize = DBSIZE(addr);
		oldfile = DBFILE(addr);
		oldline = DBLINE(addr);

		/* do the resize */
		s = ROUND(size,ALIGN) + DB_EXTRA;
		if(s < sizeof(Body_t))
			s = sizeof(Body_t);
		data = (Vmuchar_t*)KPVRESIZE(vm,data,s,
					 (type&~VM_RSZERO),(*(Vmbest->resizef)) );
		if(!data) /* failed, reset data for old block */
		{	dbwarn(vm,NULL,DB_ALLOC,file,line,func,DB_RESIZE);
			dbsetinfo((Vmuchar_t*)addr,oldsize,oldfile,oldline);
		}
		else
		{	data = DB2DEBUG(data);
			dbsetinfo(data,size,file,line);
	
			if((vd->mode&VM_TRACE) && _Vmtrace)
			{	vm->file = file; vm->line = line;
				(*_Vmtrace)(vm,(Vmuchar_t*)addr,data,size,0);
			}
			if(Dbnwatch > 0)
				dbwatch(vm,data,file,line,func,DB_RESIZED);
		}

		if(data && (type&VM_RSZERO) && size > oldsize)
		{	Vmuchar_t *d = data+oldsize, *ed = data+size;
			do { *d++ = 0; } while(d < ed);
		}
	}

	CLRLOCK(vm, local);

	return data;
}

/* compact any residual free space */
static int dbcompact(Vmalloc_t* vm, int local)
{
	return (*(Vmbest->compactf))(vm, local);
}

/* check for memory overwrites over all live blocks */
int vmdbcheck(Vmalloc_t* vm)
{
	Block_t	*b, *endb;
	Seg_t*	seg;
	int		rv;
	Vmdata_t*	vd = vm->data;

	/* check the meta-data of this region */
	if(vd->mode & (VM_MTDEBUG|VM_MTBEST|VM_MTPROFILE))
	{	if(_vmbestcheck(vd, NULL) < 0)
			return -1;
		if(!(vd->mode&VM_MTDEBUG) )
			return 0;
	}
	else	return -1;

	rv = 0;
	for(seg = vd->seg; seg; seg = seg->next)
	{	b = SEGBLOCK(seg);
		endb = (Block_t*)(seg->baddr - sizeof(Head_t));
		while(b < endb)
		{	Vmuchar_t	*data, *begp, *endp;

			if(ISJUNK(SIZE(b)) || !ISBUSY(SIZE(b)))
				goto next;

			data = DB2DEBUG(DATA(b));
			if(DBISBAD(data))	/* seen this before */
			{	rv += 1;
				goto next;
			}

			DBHEAD(data,begp,endp);
			for(; begp < endp; ++begp)
				if(*begp != DB_MAGIC)
					goto set_bad;

			DBTAIL(data,begp,endp);
			for(; begp < endp; ++begp)
			{	if(*begp == DB_MAGIC)
					continue;
			set_bad:
				dbwarn(vm,data,(long)(begp-data),vm->file,vm->line,0,DB_CHECK);
				DBSETBAD(data);
				rv += 1;
				goto next;
			}

		next:	b = (Block_t*)((Vmuchar_t*)DATA(b) + (SIZE(b)&~BITS));
		}
	}

	return rv;
}

/* set/delete an address to watch */
void* vmdbwatch(void* addr)
{
	int		n;
	void*	out;

	out = NULL;
	if(!addr)
		Dbnwatch = 0;
	else
	{	for(n = Dbnwatch - 1; n >= 0; --n)
			if(Dbwatch[n] == addr)
				break;
		if(n < 0)	/* insert */
		{	if(Dbnwatch == S_WATCH)	
			{	/* delete left-most */
				out = Dbwatch[0];
				Dbnwatch -= 1;
				for(n = 0; n < Dbnwatch; ++n)
					Dbwatch[n] = Dbwatch[n+1];
			}
			Dbwatch[Dbnwatch] = addr;
			Dbnwatch += 1;
		}
	}
	return out;
}

static void* dbalign(Vmalloc_t* vm, size_t size, size_t align, int local)
{
	Vmuchar_t	*data;
	size_t		s;
	char		*file;
	int		line;
	void		*func;
	Vmdata_t	*vd = vm->data;
	VMFLF(vm,file,line,func);

	if(size <= 0 || align <= 0)
		return NULL;

	SETLOCK(vm, local);

	if((s = ROUND(size,ALIGN) + DB_EXTRA) < sizeof(Body_t))
		s = sizeof(Body_t);

	if((data = (Vmuchar_t*)KPVALIGN(vm,s,align,(*(Vmbest->alignf)))) )
	{	data += DB_HEAD;
		dbsetinfo(data,size,file,line);

		if((vd->mode&VM_TRACE) && _Vmtrace)
		{	vm->file = file; vm->line = line; vm->func = func;
			(*_Vmtrace)(vm,NULL,data,size,align);
		}
	}

	CLRLOCK(vm, local);

	return data;
}

/* print statistics of region vm. If vm is NULL, use Vmregion */
ssize_t vmdbstat(Vmalloc_t* vm)
{	Vmstat_t	st;
	char		buf[1024], *bufp;

	vmstat(vm ? vm : Vmregion, &st);
	bufp = buf;
	bufp = (*_Vmstrcpy)(bufp, "n_busy", '=');
	bufp = (*_Vmstrcpy)(bufp, (*_Vmitoa)(VLONG(st.n_busy),-1), ',');
	bufp = (*_Vmstrcpy)(bufp, " s_busy", '=');
	bufp = (*_Vmstrcpy)(bufp, (*_Vmitoa)(VLONG(st.s_busy),-1), '\n');
	bufp = (*_Vmstrcpy)(bufp, "n_free", '=');
	bufp = (*_Vmstrcpy)(bufp, (*_Vmitoa)(VLONG(st.n_free),-1), ',');
	bufp = (*_Vmstrcpy)(bufp, " s_free", '=');
	bufp = (*_Vmstrcpy)(bufp, (*_Vmitoa)(VLONG(st.s_free),-1), '\n');
	bufp = (*_Vmstrcpy)(bufp, "m_busy", '=');
	bufp = (*_Vmstrcpy)(bufp, (*_Vmitoa)(VLONG(st.m_busy),-1), ',');
	bufp = (*_Vmstrcpy)(bufp, " m_free", '=');
	bufp = (*_Vmstrcpy)(bufp, (*_Vmitoa)(VLONG(st.m_free),-1), '\n');
	bufp = (*_Vmstrcpy)(bufp, "n_segment", '=');
	bufp = (*_Vmstrcpy)(bufp, (*_Vmitoa)(VLONG(st.n_seg),-1), ',');
	bufp = (*_Vmstrcpy)(bufp, " extent", '=');
	bufp = (*_Vmstrcpy)(bufp, (*_Vmitoa)(VLONG(st.extent),-1), '\n');
	*bufp = 0;
	write(Dbfd, buf, strlen(buf));
	return strlen(buf);
}

static Vmethod_t _Vmdebug =
{
	dballoc,
	dbresize,
	dbfree,
	dbaddr,
	dbsize,
	dbcompact,
	dbalign,
	VM_MTDEBUG
};

Vmethod_t*	Vmdebug = &_Vmdebug;

#ifdef NoF
NoF(vmdebug)
#endif

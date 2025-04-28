/***********************************************************************
*                                                                      *
*              This file is part of the ksh 93u+m package              *
*             Copyright (c) 2025 Contributors to ksh 93u+m             *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                  Martijn Dekker <martijn@inlv.org>                   *
*                                                                      *
***********************************************************************/

/*
 * New Vmalloc: a small interface around the standard memory allocator
 * that implement allocation regions and automatic initialization.
 */

#ifndef _VMALLOC_H
#define _VMALLOC_H

typedef struct
{
	void		*alloc;			/* tree of pointers & their alloc sizes	*/
	uint32_t	options;		/* option bits for the region		*/
	void		(*outofmemory)(size_t);	/* called when malloc, etc. fails	*/
} Vmalloc_t;

extern Vmalloc_t	*vmopen(void);
extern void		*vmalloc(Vmalloc_t*, size_t);
extern void		*vmresize(Vmalloc_t*, void*, size_t);
extern void		*_Vm_newoldof_(Vmalloc_t*, void*, size_t, int);
extern char		*vmstrdup(Vmalloc_t*, const char*);
extern void		vmfree(Vmalloc_t*, void*);
extern void		vmclear(Vmalloc_t*);
extern void		vmclose(Vmalloc_t*);

/* region option bits */
#define VM_INIT		0x01			/* initialize allocated/grown memory	*/
#define VM_FREEONFAIL	0x02			/* vmresize frees block on resize fail	*/

/* legacy */
#define vmnewof(v,p,t,n,x)	( (t*)_Vm_newoldof_((v), (p), sizeof(t)*(n)+(x), 1) )
#define vmoldof(v,p,t,n,x)	( (t*)_Vm_newoldof_((v), (p), sizeof(t)*(n)+(x), 0) )

#endif /* _VMALLOC_H */

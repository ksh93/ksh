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

#include <ast.h>
#include <ast_release.h>
#include <cdt.h>
#include <vmalloc.h>

#if _AST_release
#define NDEBUG
#endif
#include <assert.h>

/*
 * Cdt discipline for Vmalloc regions: keep a sorted list of allocated pointers and their sizes.
 */

typedef struct _Vmmeta_s_
{
	Dtlink_t	links;		/* space for Cdt links			*/
	void		*ap;		/* pointer to allocated memory block	*/
	size_t		size;		/* the size of the allocated block	*/
} Vmmeta_t;

static int compare_addresses(Dt_t* dict, void *sp, void *dp, Dtdisc_t *disc)
{
	uintptr_t	s = (uintptr_t)sp;
	uintptr_t	d = (uintptr_t)dp;
	NOT_USED(dict);
	NOT_USED(disc);
	return s < d ? -1 : s > d;
}

static Dtdisc_t vmdisc =
{
	offsetof(Vmmeta_t, ap),		/* key: where the key resides		*/
	-1, 				/* size: key size/type (<0 for pointer)	*/
	offsetof(Vmmeta_t, links),	/* link: offset to Dtlink_t field	*/
	0,				/* makef: object constructor		*/
	0,				/* freef: object destructor		*/
	compare_addresses,		/* comparf: to compare two objects	*/
	0,				/* hashf: to compute hash value 	*/
	0,				/* memoryf: to allocate/free memory	*/
	0				/* eventf: to process events		*/
};

/*
 * Helper functions for failure handling.
 */

static void *fail(Vmalloc_t *vm, size_t size, void *tofree1, void *tofree2)
{
	if (tofree1)
		free(tofree1);
	if (tofree2)
		free(tofree2);
	if (vm->outofmemory)
		(*vm->outofmemory)(size); /* may abort or longjmp */
	return NULL;
}

static void noreturn notallocated(Vmalloc_t *vm, void *ap, char *fn)
{
	sfprintf(sfstderr,"\n*** %s: pointer %p not allocated in region %p\n", fn, ap, vm);
	sfsync(NULL);
	abort();
}

/*
 * Open a new region.
 */
Vmalloc_t *vmopen(void)
{
	Vmalloc_t	*vm;

	if (!(vm = calloc(1, sizeof(Vmalloc_t))))
		return NULL;
	if (!(vm->alloc = dtopen(&vmdisc, Dtoset)))
	{
		free(vm);
		return NULL;
	}
	return vm;
}

/*
 * Allocate a block in a region.
 */
void *vmalloc(Vmalloc_t *vm, size_t size)
{
	Vmmeta_t	*mp;

	assert(vm != NULL);
	assert(size > 0);
	if (!(mp = calloc(1, sizeof(Vmmeta_t))))
		return fail(vm, 0, NULL, NULL);
	mp->size = size;
	if (!(mp->ap = vm->options & VM_INIT ? calloc(1, size) : malloc(size)))
		return fail(vm, size, mp, NULL);
	if (!dtinsert(vm->alloc, mp))
		return fail(vm, 0, mp->ap, mp);
	return mp->ap;
}

/*
 * Resize a block in a region.
 * If ap is NULL, allocates a new block.
 * If size is 0, ap is freed.
 */
void *vmresize(Vmalloc_t *vm, void *ap, size_t size)
{
	Vmmeta_t	*mp;
	void		*tmp;

	if (!ap)
		return vmalloc(vm, size);
	if (!size)
	{
		vmfree(vm, ap);
		return NULL;
	}
	assert(vm != NULL);
	if (!(mp = dtmatch(vm->alloc, ap)))
		notallocated(vm, ap, "vmresize");
	if (!(tmp = realloc(ap, size)))
	{
		if (vm->options & VM_FREEONFAIL)
		{
			tmp = dtdetach(vm->alloc, mp);
			assert(tmp == mp);
			return fail(vm, size, ap, mp);
		}
		return fail(vm, size, NULL, NULL);
	}
	ap = tmp;
	/* Initialize added memory */
	if ((vm->options & VM_INIT) && (size > mp->size))
		memset((char*)ap + mp->size, 0, size - mp->size);
	/* Update and re-sort the housekeeping node */
	tmp = dtdetach(vm->alloc, mp);
	assert(tmp == mp);
	mp->ap = ap;
	mp->size = size;
	if (!dtinsert(vm->alloc, mp))
		return fail(vm, 0, ap, mp);
	return ap;
}

/*
 * Helper function for vmnewof() and vmoldof() macros.
 * Allocate or resize a block in a region, with or without initialization of new memory.
 */
void *_Vm_newoldof_(Vmalloc_t *vm, void *ap, size_t size, int init)
{
	uint32_t	save_opt;

	assert(vm != NULL);
	save_opt = vm->options;
	if (init)
		vm->options |= VM_INIT;
	else
		vm->options &= ~VM_INIT;
	ap = vmresize(vm, ap, size);
	vm->options = save_opt;
	return ap;
	
}

/*
 * Return a copy of s using vmalloc, or NULL on failure.
 */
char *vmstrdup(Vmalloc_t *vm, const char *s)
{
	Vmmeta_t	*mp;

	assert(vm != NULL);
	assert(s != NULL);
	if (!(mp = calloc(1, sizeof(Vmmeta_t))))
		return fail(vm, 0, NULL, NULL);
	if (!(mp->ap = malloc(mp->size = strlen(s) + 1)))
		return fail(vm, mp->size, mp, NULL);
	if (!dtinsert(vm->alloc, mp))
		return fail(vm, 0, mp->ap, mp);
	return memcpy(mp->ap, s, mp->size);
}

/*
 * Free an allocated block from a region.
 */
void vmfree(Vmalloc_t *vm, void *ap)
{
	Vmmeta_t	*mp;

	assert(vm != NULL);
	assert(ap != NULL);
	if (!(mp = dtmatch(vm->alloc, ap)))
		notallocated(vm, ap, "vmfree");
	assert(mp->size > 0);
	free(ap);
	ap = dtdetach(vm->alloc, mp);
	assert(ap == mp);
	free(mp);
}

/*
 * Free all allocated memory from a region.
 */
void vmclear(Vmalloc_t *vm)
{
	Vmmeta_t	*mp;
	Vmmeta_t	*mpnext;

	assert(vm != NULL);
	assert(vm->alloc != NULL);
	for (mp = dtfirst(vm->alloc); mp; mp = mpnext)
	{
		mpnext = dtnext(vm->alloc, mp);
		free(mp->ap);
		assert(mp->size > 0);
		free(mp);
	}
	dtclear(vm->alloc);
}

/*
 * Free a region, including its allocated memory.
 */
void vmclose(Vmalloc_t *vm)
{
	vmclear(vm);
	dtclose(vm->alloc);
	free(vm);
}

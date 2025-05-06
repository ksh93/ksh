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
 * that implements allocation regions and automatic initialization.
 */

#include <vmalloc.h>

/*
 * Keep allocations in a doubly linked list.
 */
typedef struct Vmblock
{
	size_t		size;		/* the size of the allocated block	*/
	struct Vmblock	*prev;		/* previous block in list		*/
	struct Vmblock	*next;		/* next block in list			*/
#if __STDC_VERSION__ >= 199901L
	max_align_t	vblock[];	/* the virtual allocated block, aligned	*/
#else
	max_align_t	vblock[1];	/* ...C90 fallback with struct hack	*/
#endif
} Vmblock_t;

#define VBLOCKOFFSET	offsetof(Vmblock_t, vblock)

/*
 * Helper function for failure handling.
 */
static void *fail(Vmalloc_t *vm, size_t size)
{
	if (vm->outofmemory)
		(*vm->outofmemory)(size); /* may abort or longjmp */
	return NULL;
}

/*
 * Open a new region.
 */
Vmalloc_t *vmopen(void)
{
	return calloc(1, sizeof(Vmalloc_t));
}

/*
 * Allocate a block in a region.
 */
void *vmalloc(Vmalloc_t *vm, size_t size)
{
	Vmblock_t	*bp;

	if (!(bp = (vm->options & VM_INIT) ? calloc(1, size + VBLOCKOFFSET) : malloc(size + VBLOCKOFFSET)))
		return fail(vm, size);
	bp->size = size;
	/* insert at front of list */
	bp->prev = NULL;
	if (bp->next = vm->_list_)
		bp->next->prev = bp;
	vm->_list_ = bp;
	return (char*)bp + VBLOCKOFFSET;
}

/*
 * Resize a block in a region.
 * If ap is NULL, allocates a new block.
 * If size is 0, ap is freed.
 */
void *vmresize(Vmalloc_t *vm, void *ap, size_t size)
{
	Vmblock_t	*bp, *tmp;

	if (!ap)
		return vmalloc(vm, size);
	if (!size)
	{
		vmfree(vm, ap);
		return NULL;
	}
	bp = (Vmblock_t*)((char*)ap - VBLOCKOFFSET);
	/* Resize block */
	if (!(tmp = realloc(bp, size + VBLOCKOFFSET)))
	{
		if (vm->options & VM_FREEONFAIL)
			free(bp);
		return fail(vm, size);
	}
	if (tmp != bp)
	{
		bp = tmp;
		ap = (char*)bp + VBLOCKOFFSET;
		if (bp->prev)
			bp->prev->next = bp;
		if (bp->next)
			bp->next->prev = bp;
	}
	/* Initialize added memory */
	if ((vm->options & VM_INIT) && (size > bp->size))
		memset((char*)ap + bp->size, 0, size - bp->size);
	bp->size = size;
	return ap;
}

/*
 * Helper function for vmnewof() and vmoldof() macros.
 * Allocate or resize a block in a region, with or without initialization of new memory.
 */
void *_Vm_newoldof_(Vmalloc_t *vm, void *ap, size_t size, int init)
{
	uint32_t	save_opt;

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
	Vmblock_t	*bp;
	size_t		size;

	if (!(bp = malloc((size = strlen(s) + 1) + VBLOCKOFFSET)))
		return fail(vm, size);
	bp->size = size;
	/* insert at front of list */
	bp->prev = NULL;
	if (bp->next = vm->_list_)
		bp->next->prev = bp;
	vm->_list_ = bp;
	return memcpy((char*)bp + VBLOCKOFFSET, s, size);
}

/*
 * Free an allocated block from a region.
 */
void vmfree(Vmalloc_t *vm, void *ap)
{
	Vmblock_t	*bp;

	bp = (Vmblock_t*)((char*)ap - VBLOCKOFFSET);
	if (!bp->prev)
		vm->_list_ = bp->next;
	else
		bp->prev->next = bp->next;
	if (bp->next)
		bp->next->prev = bp->prev;
	free(bp);
}

/*
 * Free all allocated memory from a region.
 */
void vmclear(Vmalloc_t *vm)
{
	Vmblock_t	*bp, *bpnext;

	bpnext = vm->_list_;
	while (bp = bpnext)
	{
		bpnext = bp->next;
		free(bp);
	}
	vm->_list_ = NULL;
}

/*
 * Free a region, including its allocated memory.
 */
void vmclose(Vmalloc_t *vm)
{
	vmclear(vm);
	free(vm);
}

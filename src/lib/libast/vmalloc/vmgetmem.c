/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
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
*                                                                      *
***********************************************************************/
#include	<vmalloc.h>

/*
 * vm open/close/resize - a handy default for discipline memory functions
 *
 *	vmgetmem(0,0,0)		open new region
 *	vmgetmem(r,0,0)		free region
 *	vmgetmem(r,0,n)		allocate n bytes initialized to 0
 *	vmgetmem(r,p,0)		free p
 *	vmgetmem(r,p,n)		realloc p to n bytes
 *
 * Written by Glenn S. Fowler.
 */

void* vmgetmem(Vmalloc_t* vm, void* data, size_t size)
{
	if (!vm)
		return vmopen(Vmdcheap, Vmbest, 0);
	if (data || size)
		return vmresize(vm, data, size, VM_RSMOVE|VM_RSCOPY|VM_RSZERO);
	vmclose(vm);
	return NULL;
}

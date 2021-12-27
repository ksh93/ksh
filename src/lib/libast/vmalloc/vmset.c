/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2021 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 1.0                  *
*                    by AT&T Intellectual Property                     *
*                                                                      *
*                A copy of the License is available at                 *
*          http://www.eclipse.org/org/documents/epl-v10.html           *
*         (with md5 checksum b35adb5213ca9657e911e9befb180842)         *
*                                                                      *
*              Information and Software Systems Research               *
*                            AT&T Research                             *
*                           Florham Park NJ                            *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  David Korn <dgk@research.att.com>                   *
*                   Phong Vo <kpv@research.att.com>                    *
*                                                                      *
***********************************************************************/
#if defined(_UWIN) && defined(_BLD_ast)

void _STUB_vmset(){}

#else

#include	"vmhdr.h"


/*	Set the control flags for a region.
**
**	Written by Kiem-Phong Vo, kpv@research.att.com, 01/16/94.
*/
int vmset(reg Vmalloc_t*	vm,	/* region being worked on		*/
	  int			flags,	/* flags must be in VM_FLAGS		*/
	  int			on)	/* !=0 if turning on, else turning off	*/
{
	int		mode;
	Vmdata_t	*vd = vm->data;

	if(flags == 0 && on == 0)
		return vd->mode;

	SETLOCK(vm, 0);

	mode = vd->mode;
	if(on)
		vd->mode |=  (flags&VM_FLAGS);
	else	vd->mode &= ~(flags&VM_FLAGS);

	CLRLOCK(vm, 0);

	return mode;
}

#endif

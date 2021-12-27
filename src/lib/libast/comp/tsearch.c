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
/*
 * tsearch() for systems that have <search.h> but no tsearch()
 * why would such a system provide the interface but not the
 * implementation? that's what happens when one slimes their
 * way through standards compliance
 *
 * NOTE: please excuse the crude feature test
 */

#if !_UWIN

void _STUB_tsearch(){}

#else

#if _PACKAGE_ast
#include	<ast.h>
#endif

#define tdelete		______tdelete
#define tfind		______tfind
#define tsearch		______tsearch
#define twalk		______twalk

#include	<search.h>

#undef	tdelete
#undef	tfind
#undef	tsearch
#undef	twalk

#include	"dthdr.h"

extern void*		dtfinger(Dt_t*);

/*	POSIX tsearch library based on libcdt
**	Written by Kiem-Phong Vo (AT&T Research, 07/19/95)
*/

typedef struct _tree_s
{	Dtlink_t	link;
	void*		key;
} Tree_t;

typedef struct _treedisc_s
{	Dtdisc_t	disc;
	int(*		comparf)(const void*, const void*);
} Treedisc_t;

#if defined(__EXPORT__)
#define extern	__EXPORT__
#endif

/* compare function */
static int treecompare(Dt_t* dt, char* one, char* two, Dtdisc_t* disc)
{
	return (*((Treedisc_t*)disc)->comparf)((void*)one,(void*)two);
}

static Treedisc_t	Treedisc =
{	{ sizeof(Dtlink_t), -1,	/* object is key		*/
	  0,
	  NIL(Dtmake_f), NIL(Dtfree_f),
	  treecompare,
	  NIL(Dthash_f),
	  NIL(Dtmemory_f),
	  NIL(Dtevent_f)
	},
	0
};

extern
void* tsearch(const void* key, void** rootp,
		int(*comparf)(const void*,const void*) )
{
	reg Dt_t*	dt;
	reg Tree_t*	o;

	if(!rootp ||
	   (!(dt = *((Dt_t**)rootp)) && !(dt = dtopen((Dtdisc_t*)(&Treedisc),Dtoset))) )
		return NIL(void*);

	/* dangerous to set comparf on each call but that's tsearch */
	Treedisc.comparf = comparf;

	if(!(o = (Tree_t*)dtmatch(dt,key)) )
	{	if(!(o = (Tree_t*)malloc(sizeof(Tree_t))) )
			return NIL(void*);
		o->key = (void*)key;
		dtinsert(dt,o);
	}

	if(o)
		*rootp = (void*)dt;
	else if(*rootp == NIL(void*) )
		dtclose(dt);

	return (void*)(&o->key);
}

extern
void* tfind(const void* key, void*const* rootp,
		int(*comparf)(const void*, const void*) )
{
	reg Dt_t*	dt;
	reg Tree_t*	o;

	if(!rootp || !(dt = *((Dt_t**)rootp)) )
		return NIL(void*);
	Treedisc.comparf = comparf;

	return (o = (Tree_t*)dtmatch(dt,key)) ? (void*)(&o->key) : NIL(void*);
}

/* the original tdelete() specifies that it will return the parent pointer
** in the tree if there is one. Since we are using a splay tree, a deleted
** node is always rotated to the root first. So this implementation always
** returns the key of the new root.
*/
extern
void* tdelete(const void* key, void** rootp,
		int(*comparf)(const void*, const void*) )
{
	reg Dt_t*	dt;
	reg Tree_t*	o;
	Tree_t		obj;

	if(!rootp || !(dt = *((Dt_t**)rootp)) )
		return NIL(void*);

	Treedisc.comparf = comparf;

	obj.key = (void*)key;
	dtdelete(dt,&obj);

	if(!(o = dtfinger(dt)) )
	{	dtclose(dt);
		*rootp = NIL(void*);
	}

	return o ? (void*)(&o->key) : NIL(void*);
}

/* the below routine assumes a particular layout of Dtlink_t.
** If this ever gets changed, this routine should be redone.
*/
#define lchild	link.lh.__left
#define rchild	link.rh.__rght

static void _twalk(Tree_t* obj, void(*action)(const void*,VISIT,int), int level)
{	if(!obj->lchild && !obj->rchild)
		(*action)((void*)obj,leaf,level);
	else
	{	(*action)((void*)obj,preorder,level);
		if(obj->lchild)
			_twalk((Tree_t*)obj->lchild,action,level+1);
		(*action)((void*)obj,postorder,level);
		if(obj->rchild)
			_twalk((Tree_t*)obj->rchild,action,level+1);
		(*action)((void*)obj,endorder,level);
	}
}

/* the original twalk allows specifying arbitrary node to start traversal.
** Since our root is a dictionary structure, the search here will start
** at whichever node happens to be current root.
*/
extern
void twalk(const void* root, void(*action)(const void*,VISIT,int) )
{
	reg Tree_t*	o;

	if(root && (o = (Tree_t*)dtfinger((Dt_t*)root)) )
		_twalk(o,action,0);
}

#endif

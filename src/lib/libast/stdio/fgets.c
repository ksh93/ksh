/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2024 Contributors to ksh 93u+m           *
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

#include "stdhdr.h"

extern char*
_stdgets(Sfio_t* f, char* us, int n, int isgets)
{
	int		p;
	unsigned char*	is;
	unsigned char*	ps;

	if(n <= 0 || !us || (f->mode != SFIO_READ && _sfmode(f,SFIO_READ,0) < 0))
		return NULL;

	SFLOCK(f,0);

	n -= 1;
	is = (uchar*)us;
	
	while(n)
	{	/* peek the read buffer for data */
		if((p = f->endb - (ps = f->next)) <= 0 )
		{	f->getr = '\n';
			f->mode |= SFIO_RC;
			if(SFRPEEK(f,ps,p) <= 0)
				break;
		}

		if(p > n)
			p = n;

#if _lib_memccpy
		if((ps = (uchar*)memccpy((char*)is,(char*)ps,'\n',p)) != NULL)
			p = ps-is;
		is += p;
		ps  = f->next+p;
#else
		if(!(f->flags&(SFIO_BOTH|SFIO_MALLOC)))
		{	while(p-- && (*is++ = *ps++) != '\n')
				;
			p = ps-f->next;
		}
		else
		{	int	c = ps[p-1];
			if(c != '\n')
				ps[p-1] = '\n';
			while((*is++ = *ps++) != '\n')
				;
			if(c != '\n')
			{	f->next[p-1] = c;
				if((ps-f->next) >= p)
					is[-1] = c;
			}
		}
#endif

		/* gobble up read data and continue */
		f->next = ps;
		if(is[-1] == '\n')
			break;
		else if(n > 0)
			n -= p;
	}

	if((_Sfi = is - ((uchar*)us)) <= 0)
		us = NULL;
	else if(isgets && is[-1] == '\n')
	{	is[-1] = '\0';
		_Sfi -= 1;
	}
	else	*is = '\0';

	SFOPEN(f,0);
	return us;
}

char*
fgets(char* s, int n, Sfio_t* f)
{
	return _stdgets(f, s, n, 0);
}

char*
gets(char* s)
{
	return _stdgets(sfstdin, s, BUFSIZ, 1);
}

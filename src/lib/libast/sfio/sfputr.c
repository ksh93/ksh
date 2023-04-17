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
#include	"sfhdr.h"

/*	Put out a null-terminated string
**
**	Written by Kiem-Phong Vo.
*/
ssize_t sfputr(Sfio_t*		f,	/* write to this stream	*/
	       const char*	s,	/* string to write	*/
	       int		rc)	/* record separator 	*/
{
	ssize_t		p, n, w, sn;
	uchar		*ps;
	char		*ss;

	if(!f || (f->mode != SF_WRITE && _sfmode(f,SF_WRITE,0) < 0))
		return -1;

	SFLOCK(f,0);

	f->val = sn = -1; ss = (char*)s; 
	for(w = 0; (*s || rc >= 0); )
	{	/* need to communicate string size to exception handler */
		if((f->flags&SF_STRING) && f->next >= f->endb )
		{	sn = sn < 0 ? strlen(s) : (sn - (s-ss));
			ss = (char*)s; /* save current checkpoint */
			f->val = sn + (rc >= 0 ? 1 : 0); /* space requirement */
			f->bits |= SF_PUTR; /* tell sfflsbuf to use f->val */
		}

		SFWPEEK(f,ps,p);
		f->bits &= ~SF_PUTR; /* remove any trace of this */

		if(p < 0 ) /* something not right about buffering */
			break;

		if(p == 0 || (f->flags&SF_WHOLE) )
		{	n = sn < 0 ? strlen(s) : sn - (s-ss);
			if(p >= (n + (rc < 0 ? 0 : 1)) )
			{	/* buffer can hold everything */
				if(n > 0)
				{	memcpy(ps, s, n);
					ps += n;
					w += n;
				}
				if(rc >= 0)
				{	*ps++ = rc;
					w += 1;
				}
				f->next = ps;
			}
			else
			{	/* create a reserve buffer to hold data */
				Sfrsrv_t*	rsrv;

				p = n + (rc >= 0 ? 1 : 0);
				if(!(rsrv = _sfrsrv(f, p)) )
					n = 0;
				else
				{	if(n > 0)
						memcpy(rsrv->data, s, n);
					if(rc >= 0)
						rsrv->data[n] = rc;
					if((n = SFWRITE(f,rsrv->data,p)) < 0 )
						n = 0;
				}

				w += n;
			}
			break;
		}

		if(*s == 0)
		{	*ps++ = rc;
			f->next = ps;
			w += 1;
			break;
		}

		/*
		 * Do not replace the following loop with memccpy(). The
		 * 'ps' and 's' buffers may overlap or even point to the
		 * same buffer. See: https://github.com/att/ast/issues/78
		 */
		for(; p > 0; --p, ++ps, ++s)
			if((*ps = *s) == 0)
				break;

		w += ps - f->next;
		f->next = ps;
	}

	/* sync unseekable shared streams */
	if(f->extent < 0 && (f->flags&SF_SHARE) )
		(void)SFFLSBUF(f,-1);

	/* check for line buffering */
	else if((f->flags&SF_LINE) && !(f->flags&SF_STRING) && (n = f->next-f->data) > 0)
	{	if(n > w)
			n = w;
		f->next -= n;
		(void)SFWRITE(f,f->next,n);
	}

	SFOPEN(f,0);
	return w;
}

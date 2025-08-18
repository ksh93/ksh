/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2011 AT&T Intellectual Property          *
*          Copyright (c) 2020-2024 Contributors to ksh 93u+m           *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*                  David Korn <dgk@research.att.com>                   *
*                  Martijn Dekker <martijn@inlv.org>                   *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
/*
 *   Routines to implement fast character input
 *
 *   David Korn
 *   AT&T Labs
 *
 */

#include	"shopt.h"
#include	<ast.h>
#include	<sfio.h>
#include	<error.h>
#include	<fcin.h>

Fcin_t _Fcin = {0};

/*
 * open stream <f> for fast character input
 */
int	fcfopen(Sfio_t* f)
{
	int	n;
	char	*buff;
	Fcin_t	save;
	errno = 0;
	_Fcin.fcbuff = _Fcin.fcptr;
	_Fcin._fcfile = f;
	fcsave(&save);
	if(!(buff=(char*)sfreserve(f,SFIO_UNBOUND,SFIO_LOCKR)))
	{
		fcrestore(&save);
		_Fcin.fcchar = 0;
		_Fcin.fcptr = _Fcin.fcbuff = &_Fcin.fcchar;
		_Fcin.fclast = 0;
		_Fcin._fcfile = NULL;
		return EOF;
	}
	n = sfvalue(f);
	fcrestore(&save);
	sfread(f,buff,0);
	_Fcin.fcoff = sftell(f);
	buff = (char*)sfreserve(f,SFIO_UNBOUND,SFIO_LOCKR);
	_Fcin.fclast = (_Fcin.fcptr=_Fcin.fcbuff=(unsigned char*)buff)+n;
	if(sffileno(f) >= 0)
		*_Fcin.fclast = 0;
	return n;
}


/*
 * With _Fcin.fcptr>_Fcin.fcbuff, the stream pointer is advanced and
 * If _Fcin.fclast!=0, performs an sfreserve() for the next buffer.
 * If a notify function has been set, it is called
 * If last is non-zero, and the stream is a file, 0 is returned when
 * the previous character is a 0 byte.
 */
int	fcfill(void)
{
	int	n;
	Sfio_t	*f;
	unsigned char	*last=_Fcin.fclast, *ptr=_Fcin.fcptr;
	if(!(f=fcfile()))
	{
		/* see whether pointer has passed null byte */
		if(ptr>_Fcin.fcbuff && *--ptr==0)
			_Fcin.fcptr=ptr;
		else
			_Fcin.fcoff = 0;
		return 0;
	}
	if(last)
	{
		if( ptr<last && ptr>_Fcin.fcbuff && *(ptr-1)==0)
			return 0;
		if(_Fcin.fcchar)
			*last = _Fcin.fcchar;
		if(ptr > last)
			_Fcin.fcptr = ptr = last;
	}
	if((n = ptr-_Fcin.fcbuff) && _Fcin.fcfun)
		(*_Fcin.fcfun)(f,(const char*)_Fcin.fcbuff,n,_Fcin.context);
	sfread(f, (char*)_Fcin.fcbuff, n);
	_Fcin.fcoff +=n;
	_Fcin._fcfile = 0;
	if(!last)
		return 0;
	else if(fcfopen(f) < 0)
		return EOF;
	return *_Fcin.fcptr++;
}

/*
 * Synchronize and close the current stream
 */
int fcclose(void)
{
	unsigned char *ptr;
	if(_Fcin.fclast==0)
		return 0;
	if((ptr=_Fcin.fcptr)>_Fcin.fcbuff && *(ptr-1)==0)
		_Fcin.fcptr--;
	if(_Fcin.fcchar)
		*_Fcin.fclast = _Fcin.fcchar;
	_Fcin.fclast = 0;
	_Fcin.fcleft = 0;
	return fcfill();
}

/*
 * Set the notify function that is called for each fcfill()
 */
void fcnotify(void (*fun)(Sfio_t*,const char*,int,void*),void* context)
{
	_Fcin.fcfun = fun;
	_Fcin.context = context;
}

#undef fcsave
extern void fcsave(Fcin_t *fp)
{
	*fp = _Fcin;
}

#undef fcrestore
extern void fcrestore(Fcin_t *fp)
{
	_Fcin = *fp;
}

#if SHOPT_MULTIBYTE
/* struct for part of a multibyte character that crosses buffer boundaries */
struct Extra
{
	unsigned char	buff[2*MB_LEN_MAX];
	unsigned char	*next;
};

int _fcmbget(short *len)
{
	static struct Extra	extra;
	int			i, c, n;
	/*
	 * Check if we need to piece together a split multibyte
	 * character started at the end of the previous buffer.
	 */
	if(_Fcin.fcleft)
	{
		if((c = mbsize(extra.next)) < 0)
			c = 1;
		if((_Fcin.fcleft -= c) <=0)
		{
			_Fcin.fcptr = (unsigned char*)fcfirst() - _Fcin.fcleft; 
			_Fcin.fcleft = 0;
		}
		*len = c;
		if(c==1)
			c = *extra.next++;
		else if(c==0)
			_Fcin.fcleft = 0;
		else
			c = mbchar(extra.next);
		return c;
	}
	switch(*len = mbsize(_Fcin.fcptr))
	{
	    case -1:
		/*
		 * Invalid multibyte character. Check if we're near the end of the buffer; if so,
		 * the multibyte character is probably split between this buffer and the next.
		 */
		if(_Fcin._fcfile && (n=(_Fcin.fclast-_Fcin.fcptr)) < MB_LEN_MAX)
		{
			memcpy(extra.buff, _Fcin.fcptr, n);
			_Fcin.fcptr = _Fcin.fclast;
			/* fcgetc() will read the next buffer via fcfill() */
			for(i=n; i < MB_LEN_MAX+n; i++)
				if((extra.buff[i] = fcgetc())==0)
					break;
			_Fcin.fcleft = n;
			extra.next = extra.buff;
			return _fcmbget(len);
		}
		*len = 1;
		/* FALLTHROUGH */
	    case 0:
	    case 1:
		c=fcget();
		break;
	    default:
		c = mbchar(_Fcin.fcptr);
	}
	return c;
}
#endif /* SHOPT_MULTIBYTE */

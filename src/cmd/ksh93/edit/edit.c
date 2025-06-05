/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2014 AT&T Intellectual Property          *
*          Copyright (c) 2020-2025 Contributors to ksh 93u+m           *
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
*               Anuradha Weeraman <anuradha@debian.org>                *
*               K. Eugene Carlson <kvngncrlsn@gmail.com>               *
*            SHIMIZU Akifumi <shimizu.akifumi@fujitsu.com>             *
*                                                                      *
***********************************************************************/
/*
 *  edit.c - common routines for vi and emacs one line editors in shell
 *
 *   David Korn				P.D. Sullivan
 *   AT&T Labs
 *
 *   Coded April 1983.
 */

#include	"shopt.h"
#include	<ast.h>
#include	<errno.h>
#include	<fault.h>
#include	"FEATURE/time"
#if _hdr_utime
#   include	<utime.h>
#   include	<ls.h>
#endif

#include	"defs.h"
#include	"variables.h"
#include	"io.h"
#include	"terminal.h"
#include	"history.h"
#include	"edit.h"
#include	"shlex.h"

#if SHOPT_ESH || SHOPT_VSH
static char *cursor_up;  /* move cursor up one line */
static char *erase_eos;  /* erase to end of screen */
#endif /*  SHOPT_ESH || SHOPT_VSH */

#if SHOPT_MULTIBYTE
#   define is_cntrl(c)	((c<=STRIP) && iscntrl(c))
#   define is_print(c)	((c&~STRIP) || isprint(c))
#   define genlen(str)	ed_genlen(str)
#else
#   define is_cntrl(c)	iscntrl(c)
#   define is_print(c)	isprint(c)
#   define genlen(str)	strlen(str)
#endif

#define printchar(c)	((c) ^ ('A'-cntl('A')))	/* assumes ASCII */

#define MINWINDOW	15	/* minimum width window */
#define RAWMODE		1
#define ECHOMODE	3
#define SYSERR	-1

static int keytrap(Edit_t *,char*, int, int, int);

#ifndef _POSIX_DISABLE
#   define _POSIX_DISABLE	0
#endif

#define ttyparm		(ep->e_ttyparm)
#define nttyparm	(ep->e_nttyparm)
static const char bellchr[] = "\a";	/* bell char */


/*
 * This routine returns true if fd refers to a terminal
 * This should be equivalent to isatty
 */
int tty_check(int fd)
{
	Edit_t *ep = (Edit_t*)(sh.ed_context);
	struct termios tty;
	Sfio_t *sp;
	if(fd < 0 || fd > sh.lim.open_max || sh.fdstatus[fd] == IOCLOSE
	|| (sp = sh.sftable[fd]) && (sfset(sp,0,0) & SFIO_STRING))
		return 0;
	ep->e_savefd = -1;
	return tty_get(fd,&tty)==0;
}

/*
 * Get the current terminal attributes
 * This routine remembers the attributes and just returns them if it
 *   is called again without an intervening tty_set()
 */

int tty_get(int fd, struct termios *tty)
{
	Edit_t *ep = (Edit_t*)(sh.ed_context);
	if(fd == ep->e_savefd)
		*tty = ep->e_savetty;
	else
	{
		while(tcgetattr(fd,tty) == SYSERR)
		{
			if(errno !=EINTR)
				return SYSERR;
			errno = 0;
		}
		/* save terminal settings if in canonical state */
		if(ep->e_raw==0)
		{
			ep->e_savetty = *tty;
			ep->e_savefd = fd;
		}
	}
	return 0;
}

/*
 * Set the terminal attributes
 * If fd<0, then current attributes are invalidated
 */

int tty_set(int fd, int action, struct termios *tty)
{
	Edit_t *ep = (Edit_t*)(sh.ed_context);
	if(fd >=0)
	{
		while(tcsetattr(fd, action, tty) == SYSERR)
		{
			if(errno !=EINTR)
				return SYSERR;
			errno = 0;
		}
		ep->e_savetty = *tty;
	}
	ep->e_savefd = fd;
	return 0;
}

/*{	TTY_COOKED( fd )
 *
 *	This routine will set the tty in cooked mode.
 *	It is also called by sh_done().
 *
}*/

void tty_cooked(int fd)
{
	Edit_t *ep = (Edit_t*)(sh.ed_context);
	ep->e_keytrap = 0;
	if(ep->e_raw==0)
		return;
	if(fd < 0)
		fd = ep->e_savefd;
	/*** don't do tty_set unless ttyparm has valid data ***/
	if(tty_set(fd, TCSANOW, &ttyparm) == SYSERR)
		return;
	ep->e_raw = 0;
	return;
}

/*{	TTY_RAW( fd )
 *
 *	This routine will set the tty in raw mode.
 *
}*/

int tty_raw(int fd, int echomode)
{
	int echo = echomode;
	Edit_t *ep = (Edit_t*)(sh.ed_context);
	if(ep->e_raw==RAWMODE)
		return echo?-1:0;
	else if(ep->e_raw==ECHOMODE)
		return echo?0:-1;
	if(tty_get(fd,&ttyparm) == SYSERR)
		return -1;
	if (!(ttyparm.c_lflag & ECHO ))
	{
		if(!echomode)
			return -1;
		echo = 0;
	}
#ifdef FLUSHO
	ttyparm.c_lflag &= ~FLUSHO;
#endif /* FLUSHO */
	nttyparm = ttyparm;
	nttyparm.c_iflag &= ~(IGNPAR|PARMRK|INLCR|IGNCR|ICRNL);
	nttyparm.c_iflag |= BRKINT;
	if(echo)
		nttyparm.c_lflag &= ~(ICANON);
	else
		nttyparm.c_lflag &= ~(ICANON|ISIG|ECHO|ECHOK);
	nttyparm.c_cc[VTIME] = 0;
	nttyparm.c_cc[VMIN] = 1;
#ifdef VREPRINT
	nttyparm.c_cc[VREPRINT] = _POSIX_DISABLE;
#endif /* VREPRINT */
#ifdef VDISCARD
	nttyparm.c_cc[VDISCARD] = _POSIX_DISABLE;
#endif /* VDISCARD */
#ifdef VDSUSP
	nttyparm.c_cc[VDSUSP] = _POSIX_DISABLE;
#endif /* VDSUSP */
#ifdef VWERASE
	if(ttyparm.c_cc[VWERASE] == _POSIX_DISABLE)
		ep->e_werase = cntl('W');
	else
		ep->e_werase = nttyparm.c_cc[VWERASE];
	nttyparm.c_cc[VWERASE] = _POSIX_DISABLE;
#else
	    ep->e_werase = cntl('W');
#endif /* VWERASE */
#ifdef VLNEXT
	if(ttyparm.c_cc[VLNEXT] == _POSIX_DISABLE )
		ep->e_lnext = cntl('V');
	else
		ep->e_lnext = nttyparm.c_cc[VLNEXT];
	nttyparm.c_cc[VLNEXT] = _POSIX_DISABLE;
#else
	ep->e_lnext = cntl('V');
#endif /* VLNEXT */
	ep->e_intr = ttyparm.c_cc[VINTR];
	ep->e_eof = ttyparm.c_cc[VEOF];
	ep->e_erase = ttyparm.c_cc[VERASE];
	ep->e_kill = ttyparm.c_cc[VKILL];
	if( tty_set(fd, TCSADRAIN, &nttyparm) == SYSERR )
		return -1;
	ep->e_ttyspeed = (cfgetospeed(&ttyparm)>=B1200?FAST:SLOW);
	ep->e_raw = (echomode?ECHOMODE:RAWMODE);
	return 0;
}

/*
 *	ED_WINDOW()
 *
 *	return the window size
 */
int ed_window(void)
{
	int	cols = sh.columns - 1;
	if(cols < MINWINDOW)
		cols = MINWINDOW;
	else if(cols > MAXWINDOW)
		cols = MAXWINDOW;
	return cols;
}

/*	ED_FLUSH()
 *
 *	Flush the output buffer.
 *
 */

void ed_flush(Edit_t *ep)
{
	int n = ep->e_outptr-ep->e_outbase;
	int fd = ERRIO;
	if(n<=0)
		return;
	write(fd,ep->e_outbase,(unsigned)n);
	ep->e_outptr = ep->e_outbase;
}

/*
 * send the bell character ^G to the terminal
 */

void ed_ringbell(void)
{
	write(ERRIO,bellchr,1);
}

#if SHOPT_ESH || SHOPT_VSH

/*
 * Get or update a tput (terminfo or termcap) capability string.
 */
static void get_tput(char *tp, char **cpp)
{
	Shopt_t	o = sh.options;
	char	*d = sh.st.trap[SH_DEBUGTRAP];
	char	*cp;
	sigblock(SIGINT);
	sh_offoption(SH_RESTRICTED);
	sh_offoption(SH_VERBOSE);
	sh_offoption(SH_XTRACE);
	sh.st.trap[SH_DEBUGTRAP] = NULL;
	sfprintf(sh.strbuf,".sh.value=${ \\command -p tput %s 2>/dev/null;}",tp);
	sh_trap(sfstruse(sh.strbuf),0);
	if((cp = nv_getval(SH_VALNOD)) && (!*cpp || strcmp(cp,*cpp)!=0))
	{
		if(*cpp)
			free(*cpp);
		*cpp = *cp ? sh_strdup(cp) : NULL;
	}
	else
	{
		if(*cpp)
			free(*cpp);
		*cpp = NULL;
	}
	nv_unset(SH_VALNOD,0);
	sh.st.trap[SH_DEBUGTRAP] = d;
	sh.options = o;
	sigrelease(SIGINT);
}

/*	ED_SETUP( max_prompt_size )
 *
 *	This routine sets up the prompt string
 *	The following is an unadvertised feature.
 *	  Escape sequences in the prompt can be excluded from the calculated
 *	  prompt length.  This is accomplished as follows:
 *	  - if the prompt string starts with "%\r, or contains \r%\r", where %
 *	    represents any char, then % is taken to be the quote character.
 *	  - strings enclosed by this quote character, and the quote character,
 *	    are not counted as part of the prompt length.
 */

void	ed_setup(Edit_t *ep, int fd, int reedit)
{
	char *pp;
	char *last, *prev;
	char *ppmax;
	int myquote = 0;
	size_t n;
	int qlen = 1, qwid;
	char inquote = 0;
	ep->e_fd = fd;
	ep->e_multiline = sh_editor_active() && sh_isoption(SH_MULTILINE);
	sh.winch = 0;
	ep->e_stkoff = stktell(sh.stk);
	ep->e_stkptr = stkfreeze(sh.stk,0);
#if SHOPT_MULTIBYTE
	ep->e_savedwidth = 0;
#endif /* SHOPT_MULTIBYTE */
	if(!(last = sh.prompt))
		last = "";
	sh.prompt = 0;
	if(sh.hist_ptr)
	{
		History_t *hp = sh.hist_ptr;
		ep->e_hismax = hist_max(hp);
		ep->e_hismin = hist_min(hp);
	}
	else
	{
		ep->e_hismax = ep->e_hismin = ep->e_hloff = 0;
	}
	ep->e_hline = ep->e_hismax;
	ep->e_wsize = sh_editor_active() ? ed_window()-2 : MAXLINE;
	ep->e_winsz = ep->e_wsize+2;
	ep->e_crlf = 1;
	ep->e_plen = 0;
	/*
	 * Prepare e_prompt buffer for use when redrawing the command line.
	 * Use only the last line of the potentially multi-line prompt.
	 */
	pp = ep->e_prompt;
	ppmax = pp+PRSIZE-1;
	*pp++ = '\r';
	{
		int c;
		while(prev = last, c = mbchar(last)) switch(c)
		{
			case ESC:
			{
				int skip=0;
				if(*last == ']')
				{
					/*
					 * Cut out dtterm/xterm Operating System Commands that set window/icon title, etc.
					 * See: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
					 */
					char *cp = last + 1;
					while(*cp >= '0' && *cp <= '9')
						cp++;
					if(*cp++ == ';')
					{
						while(c = *cp++)
						{
							if(c == '\a')			/* legacy terminator */
								break;
							if(c == ESC && *cp == '\\')	/* recommended terminator */
							{
								cp++;
								break;
							}
						}
						if(!c)
							break;
						last = cp;
						continue;
					}
				}
				/*
				 * Try to add the length of included escape sequences to qlen
				 * which is subtracted from the physical length of the prompt.
				 */
				ep->e_crlf = 0;
				if(pp<ppmax)
					*pp++ = ESC;
				for(n=1; c = *last++; n++)
				{
					if(pp < ppmax)
						*pp++ = c;
					if(c=='\a' || c==ESC || c=='\r')
						break;
					if(skip || (c>='0' && c<='9'))
					{
						skip = 0;
						continue;
					}
					if(n==3 && (c=='?' || c=='!' || c==':'))
						continue;
					else if(n>1 && c==';')
						skip = 1;
					else if(n>2 || (c!='[' && c!=']' && c!='('))
						break;
				}
				if(c==0 || c==ESC || c=='\r')
					last--;
				qlen += (n+1);
				break;
			}
			case '\b':
				if(pp>ep->e_prompt+1)
					pp--;
				break;
			case '\r':
				if(pp == (ep->e_prompt+2)) /* quote char */
					myquote = *(pp-1);
				/* FALLTHROUGH */

			case '\n':
				/* start again */
				ep->e_crlf = 1;
				qlen = 1;
				inquote = 0;
				pp = ep->e_prompt+1;
				break;

			case '\t':
				/* expand tabs */
				while((pp-ep->e_prompt)%TABSIZE)
				{
					if(pp >= ppmax)
						break;
					*pp++ = ' ';
				}
				break;

			case '\a':
				/* cut out bells */
				break;

			default:
				if(c==myquote)
				{
					qlen += inquote;
					inquote ^= 1;
				}
				if(pp < ppmax)
				{
					if(inquote)
						qlen++;
					else if(!is_print(c))
						ep->e_crlf = 0;
					if((qwid = last - prev) > 1)
						qlen += qwid - mbwidth(c);
					while(prev < last && pp < ppmax)
						*pp++ = *prev++;
				}
				break;
		}
	}
	if(pp-ep->e_prompt > qlen)
		ep->e_plen = pp - ep->e_prompt - qlen;
	*pp = 0;
	if(ep->e_multiline)
	{
		static char *oldterm;
		Namval_t *np = nv_search("TERM",sh.var_tree,0);
		char *term = NULL;
		if(np && nv_isattr(np,NV_EXPORT))
			term = nv_getval(np);
		if(!term)
			term = "";
		if(!oldterm || strcmp(term,oldterm))
		{
			get_tput(TINF_CURSOR_UP,&cursor_up);
			get_tput(TINF_ERASE_EOS,&erase_eos);
			if(!cursor_up)
				get_tput(TCAP_CURSOR_UP,&cursor_up);
			if(!erase_eos)
				get_tput(TCAP_ERASE_EOS,&erase_eos);
			if(oldterm)
				free(oldterm);
			oldterm = sh_strdup(term);
		}
		if(cursor_up && erase_eos)
			ep->e_wsize = MAXLINE - (ep->e_plen + 1);
		else
			ep->e_multiline = 0;
	}
	if(!ep->e_multiline && (ep->e_wsize -= ep->e_plen) < 7)
	{
		int shift = 7-ep->e_wsize;
		ep->e_wsize = 7;
		pp = ep->e_prompt+1;
		strcopy(pp,pp+shift);
		ep->e_plen -= shift;
		last[-ep->e_plen-2] = '\r';
	}
	sfsync(sfstderr);
	if(fd == sffileno(sfstderr))
	{
		/* can't use output buffer when reading from stderr */
		static char *buff;
		if(!buff)
			buff = (char*)sh_malloc(MAXLINE);
		ep->e_outbase = ep->e_outptr = buff;
		ep->e_outlast = ep->e_outptr + MAXLINE - 1;
	}
	else
	{
		qlen = sfset(sfstderr,SFIO_READ,0);
		/* make sure SFIO_READ not on */
		ep->e_outbase = ep->e_outptr = sfreserve(sfstderr,SFIO_UNBOUND,SFIO_LOCKR);
		ep->e_outlast = ep->e_outptr + sfvalue(sfstderr) - 1;
		if(qlen)
			sfset(sfstderr,SFIO_READ,1);
		sfwrite(sfstderr,ep->e_outptr,0);
	}
	ep->e_eol = reedit;
	if(ep->e_default && (pp = nv_getval(ep->e_default)))
	{
		n = strlen(pp);
		if(n > LOOKAHEAD)
			n = LOOKAHEAD;
		ep->e_lookahead = n;
		while(n-- > 0)
			ep->e_lbuf[n] = *pp++;
		ep->e_default = 0;
	}
}

void ed_putstring(Edit_t *ep, const char *str)
{
	int c;
	while (c = mbchar(str))
		ed_putchar(ep, c < 0 ? '?' : c);
}

static void ed_nputchar(Edit_t *ep, int n, int c)
{
	while(n-->0)
		ed_putchar(ep,c);
}
#endif /* SHOPT_ESH || SHOPT_VSH */

/*
 * Show any buffered 'set -b' job notification(s)
 */
static void flush_notifybuf(void)
{
	char *cp;
	if(sh.notifybuf && (cp = sfstruse(sh.notifybuf)) && *cp)
		sfputr(sfstderr, cp, -1);
}

/*
 * Do read, restart on interrupt unless SH_SIGSET or SH_SIGTRAP is set
 * Use select(2) (via sfpkrd()) to wait for input if possible
 *
 * The return value is the number of bytes read, or < 0 for EOF.
 *
 * Unfortunately, systems that get interrupted from slow reads update
 * this access time for the terminal (in violation of POSIX).
 * The fixtime() macro resets the time to the time at entry in
 * this case.  This is not necessary for systems that have select().
 */
int ed_read(void *context, int fd, char *buff, int size, int reedit)
{
	Edit_t *ep = (Edit_t*)context;
	int rv= -1;
	int delim = ((ep->e_raw&RAWMODE)?nttyparm.c_cc[VEOL]:'\n');
	int mode = -1;
	int (*waitevent)(int,long,int) = sh.waitevent;
	NOT_USED(reedit);
	/* sfpkrd must use select(2) to intercept SIGWINCH for ed_read */
	if(size < 0)
	{
		mode = 2;
		size = -size;
	}
	sh_onstate(SH_TTYWAIT);
	errno = EINTR;
	sh.waitevent = 0;
	while(rv<0 && errno==EINTR)
	{
		if(sh.trapnote&(SH_SIGSET|SH_SIGTRAP))
			goto done;
#if SHOPT_ESH || SHOPT_VSH
		/*
		 * If sh.winch is set, the number of window columns changed and/or there is a buffered
		 * job notification. When using a line editor, erase and redraw the command line.
		 */
		if(sh.winch && sh_editor_active() && sh_isstate(SH_INTERACTIVE))
		{
			int	n;
			if(!ep->e_prompt)
			{
				/* ed_emacsread or ed_viread was unable to put the tty in raw mode */
				flush_notifybuf();
				goto skipwinch;
			}
			ed_putchar(ep,'\r');
			/*
			 * Try to move cursor to start of first line and pray it works... it's very
			 * failure-prone if the window size changed, especially on modern terminals
			 * that break the whole terminal abstraction by rewrapping lines themselves :(
			 */
			if(ep->e_multiline)
			{
				n = (ep->e_plen + ep->e_peol) / ep->e_winsz;
				while(n-- > 0)
					ed_putstring(ep,cursor_up);
				/* clear the current command line */
				ed_putstring(ep,erase_eos);
			}
			else
			{
				ed_nputchar(ep, sh.columns - 1, ' ');
				ed_putchar(ep,'\r');
			}
			ed_flush(ep);
			flush_notifybuf();
			/* update window size */
			ep->e_winsz = sh.columns - 1;
			if(ep->e_winsz < MINWINDOW)
				ep->e_winsz = MINWINDOW;
			if(!ep->e_multiline)
			{
				if(ep->e_wsize < MAXLINE)
					ep->e_wsize = ep->e_winsz-2;
				if(ep->e_wsize > ep->e_plen)
					ep->e_wsize -= ep->e_plen;
			}
			/* redraw command line */
#if SHOPT_ESH && SHOPT_VSH
			if(sh_isoption(SH_VI))
				vi_redraw(ep->e_vi);
			else
				emacs_redraw(ep->e_emacs);
#elif SHOPT_VSH
			vi_redraw(ep->e_vi);
#elif SHOPT_ESH
			emacs_redraw(ep->e_emacs);
#endif /* SHOPT_ESH && SHOPT_VSH */
		}
	skipwinch:
#endif /* SHOPT_ESH || SHOPT_VSH */
		sh.winch = 0;
		/* an interrupt that should be ignored */
		errno = 0;
		if(!waitevent || (rv=(*waitevent)(fd,-1L,0))>=0)
			rv = sfpkrd(fd,buff,size,delim,-1L,mode);
	}
	if(rv < 0)
	{
#if _hdr_utime
#		define fixtime()	if(isdevtty)utime(ep->e_tty,&utimes)
		int	isdevtty=0;
		struct stat statb;
		struct utimbuf utimes;
	 	if(errno==0 && !ep->e_tty)
		{
			if((ep->e_tty=ttyname(fd)) && stat(ep->e_tty,&statb)>=0)
			{
				ep->e_tty_ino = statb.st_ino;
				ep->e_tty_dev = statb.st_dev;
			}
		}
		if(ep->e_tty_ino && fstat(fd,&statb)>=0 && statb.st_ino==ep->e_tty_ino && statb.st_dev==ep->e_tty_dev)
		{
			utimes.actime = statb.st_atime;
			utimes.modtime = statb.st_mtime;
			isdevtty=1;
		}
#else
#		define fixtime()
#endif /* _hdr_utime */
		while(1)
		{
			rv = read(fd,buff,size);
			if(rv>=0 || errno!=EINTR)
				break;
			if(sh.trapnote&(SH_SIGSET|SH_SIGTRAP))
				goto done;
			/* an interrupt that should be ignored */
			fixtime();
		}
	}
	else if(rv>=0 && mode>0)
		rv = read(fd,buff,rv>0?rv:1);
done:
	sh.waitevent = waitevent;
	sh_offstate(SH_TTYWAIT);
	return rv;
}


/*
 * put <string> of length <nbyte> onto lookahead stack
 * if <type> is non-zero, the negation of the character is put
 *    onto the stack so that it can be checked for KEYTRAP
 * putstack() returns 1 except when in the middle of a multi-byte char
 */
static int putstack(Edit_t *ep,char string[], int nbyte, int type)
{
	int c;
#if SHOPT_MULTIBYTE
	char *endp, *p=string;
	int size, offset = ep->e_lookahead + nbyte;
	*(endp = &p[nbyte]) = 0;
	endp = &p[nbyte];
	do
	{
		c = (int)((*p) & STRIP);
		if(c< 0x80 && c!='<')
		{
			if (type)
				c = -c;
#   ifndef CBREAK
			if(c == '\0')
			{
				/*** user break key ***/
				ep->e_lookahead = 0;
				kill(sh.current_pid,SIGINT);
				siglongjmp(ep->e_env, UINTR);
			}
#   endif /* CBREAK */

		}
		else
		{
			char *prevp = p;
		again:
			if((c=mbchar(p)) >=0)
			{
				p--;	/* incremented below */
				if(type)
					c = -c;
			}
			else if((endp-p) < mbmax())
			{
				if(errno == EILSEQ)
					errno = 0;
				if ((c=ed_read(ep,ep->e_fd,endp, 1,0)) == 1)
				{
					p = prevp;
					*++endp = 0;
					goto again;
				}
				return c;
			}
			else if(errno == EILSEQ)
				errno = 0;
			else
			{
				ed_ringbell();
				c = -(int)((*p) & STRIP);
				offset += mbmax()-1;
			}
		}
		ep->e_lbuf[--offset] = c;
		p++;
	}
	while (p < endp);
	/* shift lookahead buffer if necessary */
	if(offset -= ep->e_lookahead)
	{
		for(size=offset;size < nbyte;size++)
			ep->e_lbuf[ep->e_lookahead+size-offset] = ep->e_lbuf[ep->e_lookahead+size];
	}
	ep->e_lookahead += nbyte-offset;
#else
	while (nbyte > 0)
	{
		c = string[--nbyte] & STRIP;
		ep->e_lbuf[ep->e_lookahead++] = (type?-c:c);
#   ifndef CBREAK
		if( c == '\0' )
		{
			/*** user break key ***/
			ep->e_lookahead = 0;
			kill(sh.current_pid,SIGINT);
			siglongjmp(ep->e_env, UINTR);
		}
#   endif /* CBREAK */
	}
#endif /* SHOPT_MULTIBYTE */
	return 1;
}

/*
 * routine to perform read from terminal for vi and emacs mode
 * <mode> can be one of the following:
 *   -2		vi insert mode - key binding is in effect
 *   -1		vi control mode - key binding is in effect
 *   0		normal command mode - key binding is in effect
 *   1		edit keys not mapped
 *   2		Next key is literal
 */
int ed_getchar(Edit_t *ep,int mode)
{
	int n = 0, c;
	char *readin = fmtbuf(LOOKAHEAD + mbmax());
	if(!ep->e_lookahead)
	{
		ed_flush(ep);
		ep->e_inmacro = 0;
		*ep->e_vi_insert = (mode==-2);
		if((n=ed_read(ep,ep->e_fd,readin,-LOOKAHEAD,0)) > 0)
			n = putstack(ep,readin,n,1);
		*ep->e_vi_insert = 0;
	}
	if(ep->e_lookahead)
	{
		/* check for possible key mapping */
		if((c = ep->e_lbuf[--ep->e_lookahead]) < 0)
		{
			if(mode<=0 && -c == ep->e_intr)
				killpg(getpgrp(),SIGINT);
			if(mode<=0 && sh.st.trap[SH_KEYTRAP])
			{
				ep->e_keytrap = 1;
				n = mbconv(readin, -c);
				if(n==1 && readin[0]==ESC)
				{
					while(1)
					{
						if(!ep->e_lookahead)
						{
							if((c=sfpkrd(ep->e_fd,readin+n,1,'\r',(mode?400L:-1L),0))>0)
								putstack(ep,readin+n,c,1);
						}
						if(!ep->e_lookahead)
							break;
						if((c=ep->e_lbuf[--ep->e_lookahead])>=0)
						{
							ep->e_lookahead++;
							break;
						}
						c = -c;
						readin[n++] = c;
						if(c>='0' && c<='9' && n>2)
							continue;
						if(n>2 || (c!= '['  &&  c!= 'O'))
							break;
					}
				}
				if(n=keytrap(ep,readin,n,LOOKAHEAD-n,mode))
				{
					putstack(ep,readin,n,0);
					c = ep->e_lbuf[--ep->e_lookahead];
				}
				else
					c = ed_getchar(ep,mode);
				ep->e_keytrap = 0;
			}
			else
				c = -c;
		}
		/*** map '\r' to '\n' ***/
		if(c == '\r' && mode!=2)
			c = '\n';
		if(ep->e_tabcount && !(c=='\t'||c==ESC || c=='\\' || c=='=' || c==cntl('L') || isdigit(c)))
			ep->e_tabcount = 0;
	}
	else
		siglongjmp(ep->e_env,(n==0?UEOF:UINTR));
	return c;
}

#if SHOPT_ESH || SHOPT_VSH
void ed_ungetchar(Edit_t *ep,int c)
{
	if (ep->e_lookahead < LOOKAHEAD)
		ep->e_lbuf[ep->e_lookahead++] = c;
	return;
}
#endif /* SHOPT_ESH || SHOPT_VSH */

#if SHOPT_ESH || SHOPT_VSH

/*
 * put a byte into the output buffer
 */

#if SHOPT_MULTIBYTE
static void	ed_putbyte(Edit_t *ep,int c)
#else
void		ed_putchar(Edit_t *ep,int c)
#endif /* SHOPT_MULTIBYTE */
{
	char *dp = ep->e_outptr;
	if(!dp)
		return;
	*dp++ = c;
	*dp = '\0';
	if(dp >= ep->e_outlast)
		ed_flush(ep);
	else
		ep->e_outptr = dp;
}

#if SHOPT_MULTIBYTE
/*
 * put a character into the output buffer
 */

void	ed_putchar(Edit_t *ep,int c)
{
	char buf[8];
	int size, i;
	/* check for placeholder */
	if(c == MARKER)
		return;
	size = mbconv(buf, (wchar_t)c);
	for (i = 0; i < size; i++)
		ed_putbyte(ep,buf[i]);
}
#endif /* SHOPT_MULTIBYTE */

#endif /* SHOPT_ESH || SHOPT_VSH */

#if SHOPT_ESH || SHOPT_VSH
/*
 * returns the line and column corresponding to offset <off> in the physical buffer
 * if <cur> is non-zero and <= <off>, then corresponding <curpos> will start the search
 */
Edpos_t ed_curpos(Edit_t *ep,genchar *phys, int off, int cur, Edpos_t curpos)
{
	genchar *sp=phys;
	int c=1, col=ep->e_plen;
	Edpos_t pos;
#if SHOPT_MULTIBYTE
	char p[16];
#endif /* SHOPT_MULTIBYTE */
	if(cur && off>=cur)
	{
		sp += cur;
		off -= cur;
		pos = curpos;
		col = pos.col;
	}
	else
	{
		pos.line = 0;
		while(col > ep->e_winsz)
		{
			pos.line++;
			col -= (ep->e_winsz+1);
		}
	}
	while(off-->0)
	{
		if(c)
			c = *sp++;
#if SHOPT_MULTIBYTE
		if(c && (mbconv(p, (wchar_t)c))==1 && p[0]=='\n')
#else
		if(c=='\n')
#endif /* SHOPT_MULTIBYTE */
			col = 0;
		else
			col++;
		if(col >  ep->e_winsz)
			col = 0;
		if(col==0)
			pos.line++;
	}
	pos.col = col;
	return pos;
}
#endif /* SHOPT_ESH || SHOPT_VSH */

#if SHOPT_ESH || SHOPT_VSH
int ed_setcursor(Edit_t *ep,genchar *physical,int old,int new,int first)
{
	static int oldline;
	int delta;
	int clear = 0;
	Edpos_t newpos;

	delta = new - old;
	if(first < 0)
	{
		first = 0;
		clear = 1;
	}
	if( delta == 0  &&  !clear)
		return new;
	if(ep->e_multiline)
	{
		ep->e_curpos = ed_curpos(ep, physical, old,0,ep->e_curpos);
		if(clear && old>=ep->e_peol && (clear=ep->e_winsz-ep->e_curpos.col)>0)
		{
			ed_nputchar(ep,clear,' ');
			ed_nputchar(ep,clear,'\b');
			return new;
		}
		newpos =     ed_curpos(ep, physical, new,old,ep->e_curpos);
		if(ep->e_curpos.col==0 && ep->e_curpos.line>0 && oldline<ep->e_curpos.line && delta<0)
			ed_putstring(ep,"\r\n");
		oldline = newpos.line;
		if(ep->e_curpos.line > newpos.line)
		{
			int n,pline,plen=ep->e_plen;
			for(;ep->e_curpos.line > newpos.line; ep->e_curpos.line--)
				ed_putstring(ep,cursor_up);
			pline = plen/(ep->e_winsz+1);
			if(newpos.line <= pline)
				plen -= pline*(ep->e_winsz+1);
			else
				plen = 0;
			if((n=plen- ep->e_curpos.col)>0)
			{
				ep->e_curpos.col += n;
				ed_putchar(ep,'\r');
				if(!ep->e_crlf && pline==0)
					ed_putstring(ep,ep->e_prompt);
				else
				{
					int m = ep->e_winsz+1-plen;
					ed_putchar(ep,'\n');
					n = plen;
					if(m < genlen(physical))
					{
						while(physical[m] && n-->0)
							ed_putchar(ep,physical[m++]);
					}
					ed_nputchar(ep,n,' ');
					ed_putstring(ep,cursor_up);
				}
			}
		}
		else if(ep->e_curpos.line < newpos.line)
		{
			ed_nputchar(ep, newpos.line-ep->e_curpos.line,'\n');
			ep->e_curpos.line = newpos.line;
			ed_putchar(ep,'\r');
			ep->e_curpos.col = 0;
		}
		delta = newpos.col - ep->e_curpos.col;
		old   =  new - delta;
	}
	else
		newpos.line=0;
	if(delta<0)
	{
		int bs= newpos.line && ep->e_plen>ep->e_winsz;
		/*** move to left ***/
		delta = -delta;
		/*** attempt to optimize cursor movement ***/
		if(!ep->e_crlf || bs || (2*delta <= ((old-first)+(newpos.line?0:ep->e_plen))) )
		{
			ed_nputchar(ep,delta,'\b');
			delta = 0;
		}
		else
		{
			if(newpos.line==0)
				ed_putstring(ep,ep->e_prompt);
			else
			{
				first = 1+(newpos.line*ep->e_winsz - ep->e_plen);
				ed_putchar(ep,'\r');
			}
			old = first;
			delta = new-first;
		}
	}
	while(delta-->0)
		ed_putchar(ep,physical[old++]);
	return new;
}
#endif /* SHOPT_ESH || SHOPT_VSH */

#if SHOPT_ESH || SHOPT_VSH
/*
 * copy virtual to physical and return the index for cursor in physical buffer
 */
int ed_virt_to_phys(Edit_t *ep,genchar *virt,genchar *phys,int cur,int voff,int poff)
{
	genchar *sp = virt;
	genchar *dp = phys;
	int c;
	genchar *curp = sp + cur;
	genchar *dpmax = phys+MAXLINE;
	int d, r;
	sp += voff;
	dp += poff;
	for(r=poff;c= *sp;sp++)
	{
		if(curp == sp)
			r = dp - phys;
#if SHOPT_MULTIBYTE
		d = mbwidth((wchar_t)c);
		if(d==1 && is_cntrl(c))
			d = -1;
		if(d>1)
		{
			/* multiple width character put in place holders */
			*dp++ = c;
			while(--d >0)
				*dp++ = MARKER;
			/* in vi mode the cursor is at the last character */
			if(dp>=dpmax)
				break;
			continue;
		}
		else
#else
		d = (is_cntrl(c)?-1:1);
#endif	/* SHOPT_MULTIBYTE */
		if(d<0)
		{
			if(c=='\t')
			{
				c = dp-phys;
				c += ep->e_plen;
				c = TABSIZE - c%TABSIZE;
				while(--c>0)
					*dp++ = ' ';
				c = ' ';
			}
			else
			{
				*dp++ = '^';
				c = printchar(c);
			}
			if(curp == sp)
				r = dp - phys;
		}
		*dp++ = c;
		if(dp>=dpmax)
			break;
	}
	*dp = 0;
	ep->e_peol = dp-phys;
	return r;
}
#endif /* SHOPT_ESH || SHOPT_VSH */

#if (SHOPT_ESH || SHOPT_VSH) && SHOPT_MULTIBYTE
/*
 * convert external representation <src> to an array of genchars <dest>
 * <src> and <dest> can be the same
 * returns number of chars in dest
 */

int	ed_internal(const char *src, genchar *dest)
{
	const unsigned char *cp = (unsigned char *)src;
	int c;
	wchar_t *dp = (wchar_t*)dest;
	if(dest == (genchar*)roundof((uintptr_t)cp,sizeof(genchar)))
	{
		genchar buffer[MAXLINE];
		c = ed_internal(src,buffer);
		ed_gencpy((genchar*)dp,buffer);
		return c;
	}
	while(*cp)
		*dp++ = mbchar(cp);
	*dp = 0;
	return dp - (wchar_t*)dest;
}
#endif /* (SHOPT_ESH || SHOPT_VSH) && SHOPT_MULTIBYTE */

#if SHOPT_MULTIBYTE
/*
 * convert internal representation <src> into character array <dest>.
 * The <src> and <dest> may be the same.
 * returns number of chars in dest.
 */

int	ed_external(const genchar *src, char *dest)
{
	genchar wc;
	char *dp = dest;
	char *dpmax = dp+sizeof(genchar)*MAXLINE-2;
	if((char*)src == dp)
	{
		int c;
		char buffer[MAXLINE*sizeof(genchar)] = "";
		c = ed_external(src,buffer);

#if _lib_wcscpy
		wcscpy((wchar_t *)dest,(const wchar_t *)buffer);
#else
		strcopy(dest,buffer);
#endif
		return c;
	}
	while((wc = *src++) && dp<dpmax)
	{
		ssize_t size;
		if((size = mbconv(dp, wc)) < 0)
		{
			/* copy the character as is */
			size = 1;
			*dp = wc;
		}
		dp += size;
	}
	*dp = 0;
	return dp-dest;
}
#endif /* SHOPT_MULTIBYTE */

#if (SHOPT_ESH || SHOPT_VSH) && SHOPT_MULTIBYTE
/*
 * copy <sp> to <dp>
 */

void	ed_gencpy(genchar *dp,const genchar *sp)
{
	dp = (genchar*)roundof((uintptr_t)dp,sizeof(genchar));
	sp = (const genchar*)roundof((uintptr_t)sp,sizeof(genchar));
	while(*dp++ = *sp++);
}
#endif /* (SHOPT_ESH || SHOPT_VSH) && SHOPT_MULTIBYTE */

#if (SHOPT_ESH || SHOPT_VSH) && SHOPT_MULTIBYTE
/*
 * copy at most <n> items from <sp> to <dp>
 */

void	ed_genncpy(genchar *dp,const genchar *sp, int n)
{
	dp = (genchar*)roundof((uintptr_t)dp,sizeof(genchar));
	sp = (const genchar*)roundof((uintptr_t)sp,sizeof(genchar));
	while(n-->0 && (*dp++ = *sp++));
}
#endif /* (SHOPT_ESH || SHOPT_VSH) && SHOPT_MULTIBYTE */

#if (SHOPT_ESH || SHOPT_VSH) && SHOPT_MULTIBYTE
/*
 * find the string length of <str>
 */

int	ed_genlen(const genchar *str)
{
	const genchar *sp = str;
	sp = (const genchar*)roundof((uintptr_t)sp,sizeof(genchar));
	while(*sp++);
	return sp-str-1;
}
#endif /* (SHOPT_ESH || SHOPT_VSH) && SHOPT_MULTIBYTE */

/*
 * Execute keyboard trap on given buffer <inbuff> of given size <isize>
 * <mode> < 0 for vi insert mode
 */
static int keytrap(Edit_t *ep,char *inbuff,int insize, int bufsize, int mode)
{
	char *cp;
	int savexit;
	Lex_t *lexp = (Lex_t*)sh.lex_context, savelex;
#if SHOPT_MULTIBYTE
	char buff[MAXLINE];
	ed_external(ep->e_inbuf,cp=buff);
#else
	cp = ep->e_inbuf;
#endif /* SHOPT_MULTIBYTE */
	inbuff[insize] = 0;
	ep->e_col = ep->e_cur;
	if(mode== -2)
	{
		ep->e_col++;
		*ep->e_vi_insert = ESC;
	}
	else
		*ep->e_vi_insert = 0;
	nv_putval(ED_CHRNOD,inbuff,NV_NOFREE);
	nv_putval(ED_COLNOD,(char*)&ep->e_col,NV_NOFREE|NV_INTEGER);
	nv_putval(ED_TXTNOD,(char*)cp,NV_NOFREE);
	nv_putval(ED_MODENOD,ep->e_vi_insert,NV_NOFREE);
	savexit = sh.savexit;
	savelex = *lexp;
	sh_trap(sh.st.trap[SH_KEYTRAP],0);
	*lexp = savelex;
	sh.savexit = savexit;
	if((cp = nv_getval(ED_CHRNOD)) == inbuff)
		nv_unset(ED_CHRNOD,0);
	else if(bufsize>0)
	{
		strncopy(inbuff,cp,bufsize);
		inbuff[bufsize-1]='\0';
		insize = strlen(inbuff);
	}
	else
		insize = 0;
	nv_unset(ED_TXTNOD,0);
	return insize;
}

void	*ed_open(void)
{
	Edit_t *ed = sh_newof(0,Edit_t,1,0);
	strcpy(ed->e_macro,"_??");
	return ed;
}

/*
 * tcgetattr and tcsetattr are mapped to these versions in terminal.h
 */

#undef tcgetattr
int sh_tcgetattr(int fd, struct termios *tty)
{
	int r,err = errno;
	while((r=tcgetattr(fd,tty)) < 0 && errno==EINTR)
		errno = err;
	return r;
}

#undef tcsetattr
int sh_tcsetattr(int fd, int cmd, struct termios *tty)
{
	int r,err = errno;
	while((r=tcsetattr(fd,cmd,tty)) < 0 && errno==EINTR)
		errno = err;
	return r;
}

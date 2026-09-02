/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1982-2014 AT&T Intellectual Property          *
*          Copyright (c) 2020-2026 Contributors to ksh 93u+m           *
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
*         hyenias <58673227+hyenias@users.noreply.github.com>          *
*                Govind Kamat <govind_kamat@yahoo.com>                 *
*               Vincent Mihalkovic <vmihalko@redhat.com>               *
*                                                                      *
***********************************************************************/
/*
 *   History file manipulation routines
 *
 *   David Korn
 *   AT&T Labs
 *
 */

/*
 * Each command in the history file starts on an even byte and is null-terminated.
 * The first byte must contain the special character HIST_UNDO and the second
 * byte is the version number.  The sequence HIST_UNDO 0, following a command,
 * nullifies the previous command.  A six-byte sequence starting with
 * HIST_CMDNO is used to store the command number so that it is not necessary
 * to read the file from beginning to end to get to the last block of
 * commands.  This format of this sequence is different in version 1
 * than in version 0.  Version 1 allows commands to use the full 8-bit
 * character set.  It can understand version 0 format files.
 */


#include "FEATURE/options"
#include <ast.h>

#if !SHOPT_SCRIPTONLY

#define HIST_MAX	((ssize_t)sizeof(int)*HIST_BSIZE)
#define HIST_BIG	(0100000-1024)	/* 1K less than maximum short */
#define HIST_LINE	32		/* typical length for history line */
#define HIST_MARKSZ	6
#define HIST_RECENT	600
#define HIST_UNDO	0201		/* invalidate previous command */
#define HIST_CMDNO	0202		/* next 3 bytes give command number */
#define HIST_BSIZE	4096		/* size of history file buffer */
#define HIST_DFLT	512		/* default size of history list */

#if SHOPT_AUDIT
#   define _HIST_AUDIT	Sfio_t	*auditfp; \
			char	*tty; \
			int	auditmask;
#else
#   define _HIST_AUDIT
#endif

#define _HIST_PRIVATE \
	off_t	histcnt;	/* offset into history file */\
	off_t	histmarker;	/* offset of last command marker */ \
	ssize_t	histflush;	/* set if flushed outside of hflush() */\
	int	histmask;	/* power of two mask for histcnt */ \
	int	histlockfd;	/* lock file descriptor for shared HISTFILE */ \
	int	histlockcnt;	/* nested history lock counter */ \
	char	histbuff[HIST_BSIZE+1];	/* history file buffer */ \
	int	histwfail; \
	_HIST_AUDIT \
	off_t	histcmds[2];	/* offset for recent commands, must be last */

#define hist_ind(hp,c)	((int)((c)&(hp)->histmask))

#include	<sfio.h>
#include	"FEATURE/time"
#include	<error.h>
#include	<ls.h>
#include	"defs.h"
#include	"variables.h"
#include	"path.h"
#include	"builtins.h"
#include	"io.h"
#include	"history.h"

#ifndef O_BINARY
#   define O_BINARY	0
#endif /* O_BINARY */

int	_Hist = 0;
static void	hist_marker(char*,long);
static History_t* hist_trim(History_t*, int);
static int	hist_nearend(History_t*,Sfio_t*, off_t);
static int	hist_check(int);
static int	hist_clean(int);
static ssize_t	hist_write(Sfio_t*, const void*, size_t, Sfdisc_t*);
static int	hist_exceptf(Sfio_t*, int, void*, Sfdisc_t*);

static int	histinit;
static mode_t	histmode;
static History_t *hist_ptr;

#if SHOPT_ACCTFILE
    static int	acctfd;
    static char *logname;
#   include <pwd.h>

    static int  acctinit(History_t *hp)
    {
	char *cp, *acctfile;
	Namval_t *np = nv_search("ACCTFILE",sh.var_tree,0);

	if(!np || !(acctfile=nv_getval(np)))
		return 0;
	if(!(cp = getlogin()))
	{
		struct passwd *userinfo = getpwuid(sh.userid);
		if(userinfo)
			cp = userinfo->pw_name;
		else
			cp = "unknown";
	}
	logname = sh_strdup(cp);
	if((acctfd=sh_open(acctfile,O_BINARY|O_WRONLY|O_APPEND|O_CREAT|O_cloexec,S_IRUSR|S_IWUSR))>=0 && acctfd < 10)
	{
		int n;
		if((n = sh_fcntl(acctfd, F_dupfd_cloexec, 10)) >= 0)
		{
			sh_close(acctfd);
			acctfd = n;
		}
	}
	if(acctfd < 0)
	{
		acctfd = 0;
		return 0;
	}
	if(sh_isdevfd(acctfile))
	{
		char newfile[16];
		sfsprintf(newfile,sizeof(newfile),"/dev/fd/%d\0",acctfd);
		nv_putval(np,newfile,NV_RDONLY);
	}
	else if(!(sh.fdstatus[acctfd]&IOCLEX))
		sh_fcntl(acctfd,F_SETFD,FD_CLOEXEC);
	return 1;
    }
#endif /* SHOPT_ACCTFILE */

#if SHOPT_AUDIT
static int sh_checkaudit(const char *name, char *logbuf, size_t len)
{
	char	*cp, *last;
	uid_t	id1, id2;
	int	r=0, fd;
	ssize_t	n;
	if((fd=open(name, O_RDONLY|O_cloexec)) < 0)
		return 0;
	if((n = read(fd, logbuf,len-1)) < 0)
		goto done;
	while(logbuf[n-1]=='\n')
		n--;
	logbuf[n] = 0;
	if(!(cp=strchr(logbuf,';')) && !(cp=strchr(logbuf,' ')))
		goto done;
	*cp = 0;
	do
	{
		cp++;
		id1 = id2 = (uid_t)strtol(cp,&last,10);
		if(*last=='-')
			id1 = (uid_t)strtol(last+1,&last,10);
		if(sh.euserid >=id1 && sh.euserid <= id2)
			r |= 1;
		if(sh.userid >=id1 && sh.userid <= id2)
			r |= 2;
		cp = last;
	}
	while(*cp==';' ||  *cp==' ');
done:
	sh_close(fd);
	return r;
}
#endif /* SHOPT_AUDIT */

static const unsigned char hist_stamp[2] = { HIST_UNDO, HIST_VERSION };
static const Sfdisc_t hist_disc = { NULL, hist_write, NULL, hist_exceptf, NULL};

static int hist_setlock(int fd, short type)
{
	struct flock lock = { .l_type = type, .l_whence = SEEK_SET };
	while(fcntl(fd, F_SETLKW, &lock) < 0)
		if(errno != EINTR)
			return -1;
	return 0;
}

static int hist_rewrite_head(History_t *hp, const void *buf, size_t len)
{
	int fd = hp->histlockfd;
	int flags;
	ssize_t wr;
	if(fd < 0)
		return -1;
	if((flags = fcntl(fd, F_GETFL)) < 0)
		return -1;
	if((flags&O_APPEND) && fcntl(fd, F_SETFL, flags & ~O_APPEND) < 0)
		return -1;
	if(lseek(fd, (off_t)0, SEEK_SET) < 0)
	{
		if(flags&O_APPEND)
			fcntl(fd, F_SETFL, flags);
		return -1;
	}
	wr = write(fd, buf, len);
	if(flags&O_APPEND)
		fcntl(fd, F_SETFL, flags);
	if(wr != (ssize_t)len)
		return -1;
	if(sfseek(hp->histfp, (off_t)0, SEEK_END) < 0)
		return -1;
	if(sfpurge(hp->histfp) < 0)
		return -1;
	return 0;
}

static int hist_lock(History_t *hp)
{
	if(!hp || hp->histlockfd < 0)
		return 0;
	if(hp->histlockcnt++ > 0)
		return 0;
	if(hist_setlock(hp->histlockfd, F_WRLCK) == 0)
		return 0;
	hp->histlockcnt--;
	return -1;
}

static void hist_unlock(History_t *hp)
{
	if(!hp || hp->histlockfd < 0 || hp->histlockcnt <= 0)
		return;
	if(--hp->histlockcnt > 0)
		return;
	hist_setlock(hp->histlockfd, F_UNLCK);
}

static void hist_touch(void *handle)
{
	touch((char*)handle, 0, 0, 0);
}

/*
 * open the history file
 * if HISTNAME is not given and userid==0 then no history file.
 * if HISTFILE is longer than HIST_MAX bytes then it is cleaned up.
 * sh_histinit() returns 1 if history file is open.
 */
int  sh_histinit(void)
{
	int fd = -1;
	History_t *hp;
	char *histname;
	char *fname=0;
	int histmask, maxlines, hist_start=0, n;
	char *cp;
	off_t hsize = 0;
	int lockfd = -1;

	if(sh.hist_ptr=hist_ptr)
		return 1;
	if(sh.subshell && !sh.subshare)
		sh_subfork();
	if(!(histname = nv_getval(HISTFILE)))
	{
		ptrdiff_t offset = stktell(sh.stk);
		if(cp=nv_getval(HOME))
			sfputr(sh.stk,cp,-1);
		sfputr(sh.stk,hist_fname,0);
		stkseek(sh.stk,offset);
		histname = stkptr(sh.stk,offset);
	}
	cp = path_relative(histname);
	if(!histinit)
		histmode = S_IRUSR|S_IWUSR;
	if((fd=sh_open(cp,O_BINARY|O_APPEND|O_RDWR|O_CREAT|O_cloexec,histmode))>=0)
		hsize=0;
	if(fd > 0 && fd < 10)
	{
		if((n=sh_fcntl(fd,F_dupfd_cloexec,10))>=0)
		{
			sh_close(fd);
			fd=n;
		}
	}
	if(fd >= 0 && hist_setlock(fd, F_WRLCK) == 0)
	{
		lockfd = fd;
		if((hsize=lseek(fd,0,SEEK_END)) < 0)
			hsize = 0;
	}
	else if(fd >= 0 && (hsize=lseek(fd,0,SEEK_END)) < 0)
		hsize = 0;
	/* make sure that file has history file format */
	if(hsize && hist_check(fd))
	{
		if(ftruncate(fd, (off_t)0) >= 0)
		{
			lseek(fd,0,SEEK_SET);
			hsize = 0;
		}
		else
		{
			if(lockfd >= 0)
				hist_setlock(lockfd, F_UNLCK);
			sh_close(fd);
			fd = -1;
			lockfd = -1;
		}
	}
	if(fd < 0)
	{
		/* don't allow root a history_file in /tmp */
		if(sh.userid)
		{
			if(!(fname = pathtmp(NULL,0,0,NULL)))
				goto fail_return;
			fd = sh_open(fname,O_BINARY|O_APPEND|O_CREAT|O_RDWR|O_cloexec,S_IRUSR|S_IWUSR);
		}
	}
	if(fd<0)
		goto fail_return;
	if(!(sh.fdstatus[fd]&IOCLEX))
		sh_fcntl(fd,F_SETFD,FD_CLOEXEC);  /* set the file to close-on-exec */
	if(cp=nv_getval(HISTSIZE))
	{
		long long m = strtoll(cp, NULL, 10);
		if(m>HIST_MAX)
			m = HIST_MAX;
		else if(m<0)
			m = HIST_DFLT;
		maxlines = (int)m;
	}
	else
		maxlines = HIST_DFLT;
	for(histmask=16;histmask <= maxlines; histmask <<=1 );
	histmask--;
	hp = sh_calloc(1, sizeof(History_t) + (size_t)histmask * sizeof(off_t));
	sh.hist_ptr = hist_ptr = hp;
	hp->histsize = maxlines;
	hp->histmask = histmask;
	sh.fdstatus[fd] = IOHIST;  /* tell sftrack to set IOCLEX (close-on-exec bit) */
	hp->histfp= sfnew(NULL,hp->histbuff,HIST_BSIZE,fd,SFIO_READ|SFIO_WRITE|SFIO_APPENDWR|SFIO_SHARE);
	if(!hp->histfp)
	{
		free(hp);
		sh_close(fd);
		goto fail_return;
	}
	memset((char*)hp->histcmds,0,sizeof(off_t)*(size_t)(hp->histmask+1));
	hp->histind = 1;
	hp->histcmds[1] = 2;
	hp->histcnt = 2;
	hp->histlockfd = lockfd;
	hp->histlockcnt = lockfd >= 0;
	hp->histname = sh_strdup(histname);
	hp->histdisc = hist_disc;
	if(hsize==0)
	{
		/* put special characters at front of file */
		sfwrite(hp->histfp,(char*)hist_stamp,2);
		sfsync(hp->histfp);
	}
	/* initialize history list */
	else
	{
		int first,last;
		off_t mark,size = (HIST_MAX/4)+maxlines*HIST_LINE;
		hp->histind = first = (int)hist_nearend(hp,hp->histfp,hsize-size);
		histinit = 1;
		hist_eof(hp);	 /* this sets histind to last command */
		if((hist_start = (last=hp->histind)-maxlines) <=0)
			hist_start = 1;
		mark = hp->histmarker;
		while(first > hist_start)
		{
			size += size;
			first = hist_nearend(hp,hp->histfp,hsize-size);
			hp->histind = first;
		}
		histinit = hist_start;
		hist_eof(hp);
		if(!histinit)
		{
			sfseek(hp->histfp,hp->histcnt=hsize,SEEK_SET);
			hp->histind = last;
			hp->histmarker = mark;
		}
		histinit = 0;
	}
	if(fname)
	{
		unlink(fname);
		free(fname);
	}
	if(hist_clean(fd) && hist_start>1 && hsize > HIST_MAX)
	{
#ifdef DEBUG
		sfprintf(sfstderr,"%jd: hist_trim hsize=%jd\n",(Sflong_t)sh.current_pid,(intmax_t)hsize);
		sfsync(sfstderr);
#endif /* DEBUG */
		hp = hist_trim(hp,(int)hp->histind-maxlines);
	}
	hist_unlock(hp);
	sfdisc(hp->histfp,&hp->histdisc);
	HISTCUR->nvalue = &hp->histind;
	sh_timeradd(1000L*(HIST_RECENT-30), 1, hist_touch, hp->histname);
#if SHOPT_ACCTFILE
	if(sh_isstate(SH_INTERACTIVE))
		acctinit(hp);
#endif /* SHOPT_ACCTFILE */
#if SHOPT_AUDIT
	{
		char buff[SFIO_BUFSIZE];
		hp->auditfp = 0;
		if(sh_isstate(SH_INTERACTIVE) && (hp->auditmask = sh_checkaudit(SHOPT_AUDITFILE, buff, sizeof(buff))))
		{
			if((fd=sh_open(buff,O_BINARY|O_WRONLY|O_APPEND|O_CREAT|O_cloexec,S_IRUSR|S_IWUSR))>=0 && fd < 10)
			{
				if((n = sh_fcntl(fd,F_dupfd_cloexec, 10)) >= 0)
				{
					sh_close(fd);
					fd = n;
				}
			}
			if(fd>=0)
			{
				const char *tty;
				if(!(sh.fdstatus[fd]&IOCLEX))
					sh_fcntl(fd,F_SETFD,FD_CLOEXEC);
				tty = ttyname(2);
				hp->tty = sh_strdup(tty?tty:"notty");
				sh.fdstatus[fd] = IOHIST;  /* tell sftrack to set IOCLEX (close-on-exec bit) */
				hp->auditfp = sfnew(NULL,NULL,(size_t)-1,fd,SFIO_WRITE);
			}
		}
	}
#endif
	return 1;
fail_return:
	if(lockfd >= 0)
		hist_setlock(lockfd, F_UNLCK);
	if(fd >= 0)
		sh_close(fd);
	return 0;
}

/*
 * close the history file and free the space
 */
void hist_close(History_t *hp)
{
	if(hp->histlockfd >= 0 && hp->histlockcnt > 0)
	{
		hp->histlockcnt = 1;
		hist_unlock(hp);
	}
	sfclose(hp->histfp);
#if SHOPT_AUDIT
	if(hp->auditfp)
	{
		if(hp->tty)
			free(hp->tty);
		sfclose(hp->auditfp);
	}
#endif /* SHOPT_AUDIT */
	free(hp);
	hist_ptr = 0;
	sh.hist_ptr = 0;
#if SHOPT_ACCTFILE
	if(acctfd)
	{
		sh_close(acctfd);
		acctfd = 0;
	}
#endif /* SHOPT_ACCTFILE */
}

/*
 * check history file format to see if it begins with special byte
 */
static int hist_check(int fd)
{
	unsigned char magic[2];
	lseek(fd,0,SEEK_SET);
	if((read(fd,(char*)magic,2)!=2) || (magic[0]!=HIST_UNDO))
		return 1;
	return 0;
}

/*
 * clean out history file OK if not modified in HIST_RECENT seconds
 */
static int hist_clean(int fd)
{
	struct stat statb;
	return fstat(fd,&statb)>=0 && (time(NULL)-statb.st_mtime) >= HIST_RECENT;
}

/*
 * Copy the last <n> commands to a new file and make this the history file
 */
static History_t* hist_trim(History_t *hp, int n)
{
	char *cp, *tmpname = NULL;
	int incmd=1, c=0;
	int fd = -1, index, started_copyback = 0;
	ssize_t r;
	int histfd = sffileno(hp->histfp);
	History_t *hist_old = hp;
	Sfio_t *hist_new = NULL;
	char *buff, *endbuff;
	char tmpbuff[HIST_BSIZE+1];
	char locbuff[HIST_MARKSZ];
	char copybuff[HIST_BSIZE];
	off_t oldp, newp;
	off_t histcnt = 2, histmarker = 2;
	int histind = 1;
	tmpname = pathtmp(NULL,0,0,NULL);
	if(!tmpname)
		goto trimfail;
	fd = sh_open(tmpname,O_BINARY|O_RDWR|O_CREAT|O_EXCL|O_cloexec,S_IRUSR|S_IWUSR);
	if(fd < 0)
		goto trimfail;
	hist_new = sfnew(NULL,tmpbuff,HIST_BSIZE,fd,SFIO_READ|SFIO_WRITE);
	if(!hist_new)
	{
		sh_close(fd);
		fd = -1;
		goto trimfail;
	}
	sfwrite(hist_new,(char*)hist_stamp,2);
	if(--n < 0)
		n = 0;
	newp = hist_seek(hist_old,++n);
	while(1)
	{
		if(!incmd)
		{
			histind++;
			if(histcnt > histmarker+HIST_BSIZE/2)
			{
				hist_marker(locbuff,histind);
				sfwrite(hist_new,locbuff,HIST_MARKSZ);
				histcnt += HIST_MARKSZ;
				histmarker = histcnt;
			}
			oldp = newp;
			newp = hist_seek(hist_old,++n);
			if(newp <=oldp)
				break;
		}
		if(!(buff=(char*)sfreserve(hist_old->histfp,SFIO_UNBOUND,0)))
			break;
		*(endbuff=(cp=buff)+sfvalue(hist_old->histfp)) = 0;
		/* copy to null byte */
		incmd = 0;
		while(*cp++);
		if(cp > endbuff)
			incmd = 1;
		else if(*cp==0)
			cp++;
		if(cp > endbuff)
			cp = endbuff;
		c = (int)(cp-buff);
		histcnt += c;
		sfwrite(hist_new,buff,(size_t)c);
	}
	sfputc(hist_new,HIST_UNDO);
	sfputc(hist_new,0);
	if(sfsync(hist_new) < 0 || sfseek(hist_new,(off_t)0,SEEK_SET) < 0)
	{
		sfclose(hist_new);
		hist_new = NULL;
		goto trimfail;
	}
	if(sfpurge(hist_old->histfp) < 0 || ftruncate(histfd,(off_t)0) < 0 || sfseek(hist_old->histfp,(off_t)0,SEEK_SET) < 0)
	{
		sfclose(hist_new);
		hist_new = NULL;
		goto trimfail;
	}
	started_copyback = 1;
	while((r=sfread(hist_new,copybuff,sizeof(copybuff))) > 0)
	{
		if(sfwrite(hist_old->histfp,copybuff,(size_t)r) != r)
		{
			sfclose(hist_new);
			hist_new = NULL;
			goto trimfail;
		}
	}
	if(r < 0 || sfsync(hist_old->histfp) < 0 || sfseek(hist_old->histfp,(off_t)0,SEEK_END) < 0)
	{
		sfclose(hist_new);
		hist_new = NULL;
		goto trimfail;
	}
	sfclose(hist_new);
	hist_new = NULL;
	unlink(tmpname);
	free(tmpname);
	sfpurge(hist_old->histfp);
	memset((char*)hist_old->histcmds,0,sizeof(off_t)*(size_t)(hist_old->histmask+1));
	index = histinit;
	hist_old->histind = 1;
	hist_old->histcmds[1] = 2;
	hist_old->histcnt = hist_old->histmarker = 2;
	histinit = 1;
	hist_eof(hist_old);
	histinit = index;
	return hist_ptr = hist_old;
trimfail:
	if(hist_new)
		sfclose(hist_new);
	if(tmpname)
	{
		if(!started_copyback)
			unlink(tmpname);
		free(tmpname);
	}
	errormsg(SH_DICT,ERROR_warn(0),e_histtrim,hist_old->histname);
	return hist_ptr = hist_old;
}

/*
 * position history file at size and find next command number
 */
static int hist_nearend(History_t *hp, Sfio_t *iop, off_t size)
{
	unsigned char *cp, *endbuff;
	int incmd=1;
	ssize_t n;
	unsigned char *buff, marker[4];
	if(size <= 2L || sfseek(iop,size,SEEK_SET)<0)
		goto begin;
	/* skip to marker command and return the number */
	/* numbering commands occur after a null and begin with HIST_CMDNO */
	while(cp=buff=(unsigned char*)sfreserve(iop,SFIO_UNBOUND,SFIO_LOCKR))
	{
		n = sfvalue(iop);
		*(endbuff=cp+n) = 0;
		while(1)
		{
			/* check for marker */
			if(!incmd && *cp++==HIST_CMDNO && *cp==0)
			{
				n = cp+1 - buff;
				incmd = -1;
				break;
			}
			incmd = 0;
			while(*cp++);
			if(cp>endbuff)
			{
				incmd = 1;
				break;
			}
			if(*cp==0 && ++cp>endbuff)
				break;
		}
		size += n;
		sfread(iop,(char*)buff,(size_t)n);
		if(incmd < 0)
		{
			if((n=sfread(iop,(char*)marker,4))==4)
			{
				n = (marker[0]<<16)|(marker[1]<<8)|marker[2];
				if(n < size/2)
				{
					hp->histmarker = hp->histcnt = size+4;
					return (int)n;
				}
				n=4;
			}
			if(n >0)
				size += n;
			incmd = 0;
		}
	}
begin:
	sfseek(iop,(off_t)2,SEEK_SET);
	hp->histmarker = hp->histcnt = 2L;
	return 1;
}

/*
 * This routine reads the history file from the present position
 * to the end-of-file and puts the information in the in-core
 * history table
 * Note that HIST_CMDNO is only recognized at the beginning of a command
 * and that HIST_UNDO as the first character of a command is skipped
 * unless it is followed by 0.  If followed by 0 then it cancels
 * the previous command.
 */
void hist_eof(History_t *hp)
{
	char *cp,*first,*endbuff;
	int incmd = 0;
	off_t count = hp->histcnt;
	int oldind=0;
	ptrdiff_t skip=0;
	ssize_t n;
	off_t last;
	hist_lock(hp);
	last = sfseek(hp->histfp,0,SEEK_END);
	if(last < count)
	{
		last = -1;
		count = 2+HIST_MARKSZ;
		oldind = hp->histind;
		if((hp->histind -= hp->histsize) < 0)
			hp->histind = 1;
	}
again:
	sfseek(hp->histfp,count,SEEK_SET);
	while(cp=(char*)sfreserve(hp->histfp,SFIO_UNBOUND,0))
	{
		n = sfvalue(hp->histfp);
		*(endbuff = cp+n) = 0;
		first = cp += skip;
		while(1)
		{
			while(!incmd)
			{
				if(cp>first)
				{
					count += (cp-first);
					n = hist_ind(hp, ++hp->histind);
						hp->histcmds[n] = count;
					first = cp;
				}
				switch(*((unsigned char*)(cp++)))
				{
					case HIST_CMDNO:
						if(*cp==0)
						{
							hp->histmarker=count+2;
							cp += (HIST_MARKSZ-1);
							hp->histind--;
							if(!histinit && (cp <= endbuff))
							{
								unsigned char *marker = (unsigned char*)(cp-4);
								hp->histind = ((marker[0]<<16)|(marker[1]<<8)|marker[2] -1);
							}
						}
						break;
					case HIST_UNDO:
						if(*cp==0)
						{
							cp+=1;
							hp->histind-=2;
						}
						break;
					default:
						cp--;
						incmd = 1;
				}
				if(cp > endbuff)
				{
					cp++;
					goto refill;
				}
			}
			first = cp;
			while(*cp++);
			if(cp > endbuff)
				break;
			incmd = 0;
			while(*cp==0)
			{
				if(++cp > endbuff)
					goto refill;
			}
		}
	refill:
		count += (--cp-first);
		skip = cp-endbuff;
		if(!incmd && !skip)
			hp->histcmds[hist_ind(hp,++hp->histind)] = count;
	}
	hp->histcnt = count;
	if(incmd && last)
	{
		sfputc(hp->histfp,0);
		hist_cancel(hp);
		count = 2;
		skip = 0;
		oldind -= hp->histind;
		hp->histind = hp->histind-hp->histsize + oldind +2;
		if(hp->histind<0)
			hp->histind = 1;
		if(last<0)
		{
			char buff[2+HIST_MARKSZ];
			memcpy(buff,(char*)hist_stamp,2);
			hist_marker(buff+2,hp->histind);
			hist_rewrite_head(hp,buff,sizeof(buff));
		}
		last = 0;
		goto again;
	}
	hist_unlock(hp);
}

/*
 * This routine will cause the previous command to be cancelled
 */
void hist_cancel(History_t *hp)
{
	int c;
	if(!hp)
		return;
	hist_lock(hp);
	sfputc(hp->histfp,HIST_UNDO);
	sfputc(hp->histfp,0);
	sfsync(hp->histfp);
	hp->histcnt += 2;
	c = hist_ind(hp,--hp->histind);
	hp->histcmds[c] = hp->histcnt;
	hist_unlock(hp);
}

/*
 * flush the current history command
 */
void hist_flush(History_t *hp)
{
	char *buff;
	if(hp)
	{
		hist_lock(hp);
		if(buff=(char*)sfreserve(hp->histfp,0,SFIO_LOCKR))
		{
			hp->histflush = sfvalue(hp->histfp)+1;
			sfwrite(hp->histfp,buff,0);
		}
		else
			hp->histflush=0;
		if(sfsync(hp->histfp)<0)
		{
			hist_unlock(hp);
			hist_close(hp);
			if(!sh_histinit())
				sh_offoption(SH_HISTORY);
			return;
		}
		hp->histflush = 0;
		hist_unlock(hp);
	}
}

/*
 * This is the write discipline for the history file
 * When called from hist_flush(), trailing newlines are deleted and
 * a zero byte.  Line sequencing is added as required
 */
static ssize_t hist_write(Sfio_t *iop,const void *buff,size_t insize,Sfdisc_t* handle)
{
	History_t *hp = (History_t*)handle;
	char *bufptr = ((char*)buff)+insize;
	ssize_t size = (ssize_t)insize;
	off_t cur;
	int c,saved=0;
	char saveptr[HIST_MARKSZ];
	hist_lock(hp);
	if(!hp->histflush)
	{
		size = write(sffileno(iop),(char*)buff,(size_t)size);
		hist_unlock(hp);
		return size;
	}
	if((cur = lseek(sffileno(iop),0,SEEK_END)) <0)
	{
		errormsg(SH_DICT,2,"hist_flush: EOF seek failed errno=%d",errno);
		hist_unlock(hp);
		return -1;
	}
	hp->histcnt = cur;
	/* remove whitespace from end of commands */
	while(--bufptr >= (char*)buff)
	{
		c= *bufptr;
		if(!isspace(c))
		{
			if(c=='\\' && *(bufptr+1)!='\n')
				bufptr++;
			break;
		}
	}
	/* don't count empty lines */
	if(++bufptr <= (char*)buff)
	{
		hist_unlock(hp);
		return (ssize_t)insize;
	}
	*bufptr++ = '\n';
	*bufptr++ = 0;
	size = (ssize_t)(bufptr - (char*)buff);
#if	 SHOPT_AUDIT
	if(hp->auditfp)
	{
		time_t	t=time(NULL);
		sfprintf(hp->auditfp, "%u;%ju;%s;%*s%c",
			 sh_isoption(SH_PRIVILEGED) ? sh.euserid : sh.userid,
			 (Sfulong_t)t, hp->tty, size, buff, 0);
		sfsync(hp->auditfp);
	}
#endif	/* SHOPT_AUDIT */
#if	SHOPT_ACCTFILE
	if(acctfd)
	{
		ssize_t timechars;
		ptrdiff_t offset = stktell(sh.stk);
		sfputr(sh.stk,buff,-1);
		stkseek(sh.stk,stktell(sh.stk) - 1);
		timechars = sfprintf(sh.stk, "\t%s\t%lx\n",logname,(unsigned long)time(NULL));
		lseek(acctfd, 0, SEEK_END);
		write(acctfd, stkptr(sh.stk,offset), (size_t)(size - 2 + timechars));
		stkseek(sh.stk,offset);

	}
#endif /* SHOPT_ACCTFILE */
	if(size&01)
	{
		size++;
		*bufptr++ = 0;
	}
	hp->histcnt += size;
	c = hist_ind(hp,++hp->histind);
	hp->histcmds[c] = hp->histcnt;
	if(hp->histflush>HIST_MARKSZ && hp->histcnt > hp->histmarker+HIST_BSIZE/2)
	{
		memcpy(saveptr,bufptr,HIST_MARKSZ);
		saved=1;
		hp->histcnt += HIST_MARKSZ;
		hist_marker(bufptr,hp->histind);
		hp->histmarker = hp->histcmds[hist_ind(hp,c)] = hp->histcnt;
		size += HIST_MARKSZ;
	}
	errno = 0;
	size = write(sffileno(iop),(char*)buff,(size_t)size);
	if(saved)
		memcpy(bufptr,saveptr,HIST_MARKSZ);
	if(size>=0)
	{
		hp->histwfail = 0;
		hist_unlock(hp);
		return (ssize_t)insize;
	}
	hist_unlock(hp);
	return -1;
}

/*
 * Put history sequence number <n> into buffer <buff>
 * The buffer must be large enough to hold HIST_MARKSZ chars
 */
static void hist_marker(char *buff,long cmdno)
{
	*buff++ = HIST_CMDNO;
	*buff++ = 0;
	*buff++ = (char)(cmdno>>16);
	*buff++ = (char)(cmdno>>8);
	*buff++ = (char)cmdno;
	*buff++ = 0;
}

/*
 * return byte offset in history file for command <n>
 */
off_t hist_tell(History_t *hp, int n)
{
	return hp->histcmds[hist_ind(hp,n)];
}

/*
 * seek to the position of command <n>
 */
off_t hist_seek(History_t *hp, int n)
{
	if(!(n >= hist_min(hp) && n < hist_max(hp)))
		return -1;
	return sfseek(hp->histfp,hp->histcmds[hist_ind(hp,n)],SEEK_SET);
}

/*
 * write the command starting at offset <offset> onto file <outfile>.
 * if character <last> appears before newline it is deleted
 * each new-line character is replaced with string <nl>.
 */
void hist_list(History_t *hp,Sfio_t *outfile, off_t offset,int last, char *nl)
{
	int oldc=0;
	int c;
	if(offset<0 || !hp)
	{
		sfputr(outfile,sh_translate(e_unknown),'\n');
		return;
	}
	sfseek(hp->histfp,offset,SEEK_SET);
	while((c = sfgetc(hp->histfp)) != EOF)
	{
		if(c && oldc=='\n')
			sfputr(outfile,nl,-1);
		else if(last && (c==0 || (c=='\n' && oldc==last)))
			return;
		else if(oldc)
			sfputc(outfile,oldc);
		oldc = c;
		if(c==0)
			return;
	}
	return;
}

/*
 * find index for last line with given string
 * If flag==0 then line must begin with string
 * direction < 1 for backwards search
*/
Histloc_t hist_find(History_t*hp,char *string,int index1,int flag,int direction)
{
	int index2;
	off_t offset;
	ptrdiff_t *coffset=0;
	Histloc_t location;
	location.hist_command = -1;
	location.hist_char = 0;
	location.hist_line = 0;
	if(!hp)
		return location;
	/* leading ^ means beginning of line unless escaped */
	if(flag)
	{
		index2 = *string;
		if(index2=='\\')
			string++;
		else if(index2=='^')
		{
			flag=0;
			string++;
		}
	}
	if(flag)
		coffset = &location.hist_char;
	index2 = (int)hp->histind;
	if(direction<0)
	{
		index2 -= hp->histsize;
		if(index2<1)
			index2 = 1;
		if(index1 <= index2)
			return location;
	}
	else if(index1 >= index2)
		return location;
	while(index1!=index2)
	{
		direction>0?++index1:--index1;
		offset = hist_tell(hp,index1);
		if((location.hist_line=hist_match(hp,offset,string,coffset))>=0)
		{
			location.hist_command = index1;
			return location;
		}
		/* allow a search to be aborted */
		if(sh.trapnote & SH_SIGSET)
			break;
	}
	return location;
}

/*
 * search for <string> in history file starting at location <offset>
 * If coffset==0 then line must begin with string
 * returns the line number of the match if successful, otherwise -1
 */
int hist_match(History_t *hp,off_t offset,char *string,ptrdiff_t *coffset)
{
	unsigned char *first, *cp;
	int c=1,line=0;
	ssize_t m;
	size_t n;
	sfseek(hp->histfp,offset,SEEK_SET);
	if(!(cp = first = (unsigned char*)sfgetr(hp->histfp,0,0)))
		return -1;
	m = sfvalue(hp->histfp);
	n = strlen(string);
	while(m > (ssize_t)n)
	{
		if(strncmp((char*)cp,string,n)==0)
		{
			if(coffset)
				*coffset = cp-first;
			return line;
		}
		if(!coffset)
			break;
		if(*cp=='\n')
			line++;
		if((c=mbsize(cp)) < 0)
			c = 1;
		cp += c;
		m -= c;
	}
	return -1;
}


#if SHOPT_ESH || SHOPT_VSH
/*
 * copy command <command> from history file to s1
 * at most <size> characters copied
 * if s1==0 the number of lines for the command is returned
 * line=linenumber  for emacs copy and only this line of command will be copied
 * line < 0 for full command copy
 * -1 returned if there is no history file
 */
int hist_copy(char *s1,int size,int command,int line)
{
	int c;
	History_t *hp = sh.hist_ptr;
	int count = 0;
	char *const s1orig = s1;
	char *const s1max = s1 ? s1 + size : NULL;
	if(!hp)
		return -1;
	hist_seek(hp,command);
	while ((c = sfgetc(hp->histfp)) && c!=EOF)
	{
		if(c=='\n')
		{
			if(count++ ==line)
				break;
			else if(line >= 0)
				continue;
		}
		if(s1 && (line<0 || line==count))
		{
			if(s1 >= s1max)
			{
				*--s1 = 0;
				break;
			}
			*s1++ = (char)c;
		}
	}
	sfseek(hp->histfp,0,SEEK_END);
	if(s1==0)
		return count;
	if(count && s1 > s1orig && (c = *(s1 - 1)) == '\n')
		s1--;
	*s1 = '\0';
	return count;
}

/*
 * return true if c is a word boundary character, i.e. the
 * character following c is considered to start a new word
 */

int hist_iswordbndry(int c)
{
	return isspace(c) || strchr("|&;()`<>",c);
}

/*
 * return word number <word> from command number <command>
 */
char *hist_word(char *string,int size,int word)
{
	int c;
	int is_boundary;
	int quoted;
	char *s1 = string;
	unsigned char *cp = (unsigned char*)s1;
	int flag = 0;
	History_t *hp = hist_ptr;
	if(!hp)
		return NULL;
	hist_copy(string,size,hp->histind-1,-1);
	for(quoted=0;c = (int)*cp;cp++)
	{
		is_boundary = !quoted && hist_iswordbndry(c);
		if(is_boundary && flag)
		{
			*cp = 0;
			if(--word==0)
				break;
			flag = 0;
		}
		else if(is_boundary==0 && flag==0)
		{
			s1 = (char*)cp;
			flag++;
		}
		if (c=='\'' && !quoted)
		{
			for(cp++;*cp && *cp != c;cp++)
				;
		}
		else if (c=='\"' && !quoted)
		{
			for(cp++;*cp && (*cp != c || quoted);cp++)
				quoted = *cp=='\\' ? !quoted : 0;
		}
		else if (c=='$' && cp[1]=='\'' && !quoted)
		{
			for(cp+=2; *cp && (*cp != '\'' || quoted); cp++)
				quoted = *cp=='\\' ? !quoted : 0;
		}
		quoted = *cp=='\\' ? !quoted : 0;
	}
	*cp = 0;
	if(s1 != string)
		/* We can't use strcpy() because the two buffers may overlap. */
		strcopy(string,s1);
	return string;
}

#endif	/* SHOPT_ESH */

#if SHOPT_ESH
/*
 * given the current command and line number,
 * and number of lines back or forward,
 * compute the new command and line number.
 */
Histloc_t hist_locate(History_t *hp,int command,int line,int lines)
{
	Histloc_t next;
	line += lines;
	if(!hp)
	{
		command = -1;
		goto done;
	}
	if(lines > 0)
	{
		int count;
		while(command <= hp->histind)
		{
			count = hist_copy(NULL,0, command,-1);
			if(count > line)
				goto done;
			line -= count;
			command++;
		}
	}
	else
	{
		int least = hp->histind-hp->histsize;
		while(1)
		{
			if(line >=0)
				goto done;
			if(--command < least)
				break;
			line += hist_copy(NULL,0, command,-1);
		}
		command = -1;
	}
done:
	next.hist_line = line;
	next.hist_command = command;
	return next;
}
#endif	/* SHOPT_ESH */


/*
 * Handle history file exceptions
 */
static int hist_exceptf(Sfio_t* fp, int type, void *data, Sfdisc_t *handle)
{
	int newfd,oldfd,lockcnt=0,relock=0;
	History_t *hp = (History_t*)handle;
	NOT_USED(data);
	if(type==SFIO_WRITE)
	{
		if(errno==ENOSPC || hp->histwfail++ >= 10)
			return 0;
		/* write failure could be NFS problem, try to reopen */
		oldfd = sffileno(fp);
		if(oldfd < 0)
			return -1;
		if(hp->histlockfd==oldfd && hp->histlockcnt>0)
		{
			relock = 1;
			lockcnt = hp->histlockcnt;
			hp->histlockfd = -1;
			hp->histlockcnt = 0;
		}
		sh_close(oldfd);
		if((newfd=sh_open(hp->histname,O_BINARY|O_APPEND|O_CREAT|O_RDWR|O_cloexec,S_IRUSR|S_IWUSR)) >= 0)
		{
			if(newfd != oldfd)
			{
				int dupfd = sh_fcntl(newfd, F_dupfd_cloexec, oldfd);
				sh_close(newfd);
				if(dupfd != oldfd)
				{
					if(dupfd > -1)
						sh_close(dupfd);
					if(relock)
					{
						hp->histlockfd = oldfd;
						hp->histlockcnt = 0;
					}
					return -1;
				}
			}
			if(relock)
			{
				if(hist_setlock(oldfd,F_WRLCK) < 0)
				{
					hp->histlockfd = oldfd;
					hp->histlockcnt = 0;
					return -1;
				}
				hp->histlockfd = oldfd;
				hp->histlockcnt = lockcnt;
			}
			if(!(sh.fdstatus[oldfd]&IOCLEX))
				sh_fcntl(oldfd,F_SETFD,FD_CLOEXEC);
			if(lseek(oldfd,0,SEEK_END) < hp->histcnt)
			{
				int index = hp->histind;
				lseek(oldfd,(off_t)2,SEEK_SET);
				hp->histcnt = 2;
				hp->histind = 1;
				hp->histcmds[1] = 2;
				hist_eof(hp);
				hp->histmarker = hp->histcnt;
				hp->histind = index;
			}
			return 1;
		}
		if(relock)
		{
			hp->histlockfd = oldfd;
			hp->histlockcnt = 0;
		}
		errormsg(SH_DICT,2,"History file write error-%d %s: file unrecoverable",errno,hp->histname);
		return -1;
	}
	return 0;
}

#else
NoN(history)
#endif /* !SHOPT_SCRIPTONLY */

/***********************************************************************
 *                                                                      *
 *               This software is part of the ast package               *
 *          Copyright (c) 1989-2011 AT&T Intellectual Property          *
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
 *               Glenn Fowler <glenn.s.fowler@gmail.com>                *
 *                                                                      *
 ***********************************************************************/
/*
 * David Korn
 * AT&T Research
 *
 * banner [-lp] [-b char] [-d char] [-f char] [-w width] [ message ... ]
 */

static const char usage[] =
	"[-?\n@(#)$Id: banner (ksh 93u+m) 2021-11-22 $\n]"
	"[-author?David Korn <dgk@research.att.com>]"
	"[-copyright?Copyright (c) 2001-2013 AT&T Intellectual Property]"
	"[-license?http://www.eclipse.org/org/documents/epl-v10.html]"
	"[--catalog?builtin]"
	"[+NAME?banner - print large banner]"
	"[+DESCRIPTION?\bbanner\b prints a large banner on the standard output (up to 30 characters).]"

	"[b:background-characters]:[bg_delimiter?This sets the characters used for the background of each string. The "
	"default value is ' '.]"
	"[d|f:foreground-characters]:[delimiter?The banner print character is \achar\a, multiple can be specified for "
	"each "
	"letter. The default value is #.]"
	"[l:use-lpd-font?Use NetBSD's modern LPD font instead of the classic System V font.]"
	"[p:use-public-domain-font?Use the font from the public domain implementation of sysvbanner.]"
	"[w:print-width]#[width?The banner print width is \awidth\a, which is an unsigned 16-bit value. The default "
	"value is 240.]"

	"\n"
	"\n[ message ... ]\n"
	"\n"
	"[+SEE ALSO?\blpr\b(1), \bpr\b(1)]";

#include <cmd.h>
#include <ccode.h>
#include "lpd.h"
#include "sysv.h"
#include "pd.h"

/* Macros for each font */
#define SYSV 0
#define LPD 1
#define PD 2

/* Create a banner to print on screen */
static void create_banner(const char *restrict string, const char *restrict delim, unsigned short width,
						  unsigned char font, char bg_char)
{
	int c, n;
	unsigned int i;
	size_t j = strlen(string);
	unsigned mask;
	const char *cp, *dp;

	if(j > width / 8)
	{
		error(ERROR_exit(1), "up to %d char%s per arg", width / 8, (width / 8) == 1 ? "" : "s");
		UNREACHABLE();
	}
	for(i = 0; i < 9; i++)
	{
		dp = delim;
		for(n = 0, cp = string; c = *cp++ & 0x07f; dp++)
		{
			if(*dp == 0)
				dp = delim;

			/* Use the specified font */
			switch(font)
			{
			    case SYSV:
				mask = bandata_sysv[c - 32][i];
				break;
			    case LPD:
				mask = bandata_lpd[c - 32][i];
				break;
			    case PD:
				mask = bandata_pd[c - 32][i];
				break;
			}

			if(mask == 0)
			{
				n += 8;
				continue;
			}

			for(j = 0x80; j > 0; j >>= 1)
			{
				if(mask & j)
				{
					if(n)
					{
						sfnputc(sfstdout, bg_char, n);
						n = 0;
					}
					sfputc(sfstdout, *dp);
				}
				else
					n++;
			}
		}

		sfnputc(sfstdout, bg_char, n); /* Fill the background with the desired background char */
		sfputc(sfstdout, '\n');
	}
}

int b_banner(int argc, char *argv[], Shbltin_t *context)
{
	int n;
	char *bg_char = " ";
	char *cp, *delim = "#";
	unsigned char font = SYSV, x = 0;
	unsigned short width = 240; /* 30 characters per banner */
	NOT_USED(argc);

	error_info.id = "banner";
	while((n = optget(argv, usage))) switch(n)
	{
	    /* Background characters (one for each banner) */
	    case 'b':
		bg_char = opt_info.arg;
		break;

	    /* Delimiter characters for each letter of a banner */
	    case 'd':
	    case 'f':
		delim = opt_info.arg;
		break;

	    /* Restrict the width of a banner (i.e. -w 80 = 10 chars) */
	    case 'w':
		width = (short)opt_info.num;
		break;

	    /* LPD Font */
	    case 'l':
		font = LPD;
		break;

	    /* PD Banner Font */
	    case 'p':
		font = PD;
		break;

	    /* Invalid argument */
	    case ':':
		error(2, "%s", opt_info.arg);
		break;
	    case '?':
		error(ERROR_usage(2), "%s", opt_info.arg);
		UNREACHABLE();
	}
	argv += opt_info.index;
	if(error_info.errors || !*argv)
	{
		error(ERROR_usage(2), "%s", optusage(NiL));
		UNREACHABLE();
	}
	sfset(sfstdout, SF_LINE, 0);

	/* Create banners with a loop */
	while((cp = *argv++))
	{
		create_banner(cp, delim, width, font, bg_char[x]);
		if(strlen(bg_char) > (x + 1))
			x++;
		else
			x = 0;
	}

	return 0;
}

/***********************************************************************
*                                                                      *
*              This file is part of the ksh 93u+m package              *
*             Copyright (c) 2026 Contributors to ksh 93u+m             *
*                      and is licensed under the                       *
*                 Eclipse Public License, Version 2.0                  *
*                                                                      *
*                A copy of the License is available at                 *
*      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      *
*         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         *
*                                                                      *
*            Johnothan King <johnothanking@protonmail.com>             *
*                                                                      *
***********************************************************************/
/*
 * strtoui() implementation
 */

#define S2I_function	strtoui
#define S2I_number	int
#define S2I_unumber	unsigned int
#define S2I_unsigned	1

#include "strtoi.h"

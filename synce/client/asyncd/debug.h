/*	
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _DEBUG_H
#define _DEBUG_H

#include	<stdio.h>
#include	<unistd.h>
#include	<syslog.h>
#include	<stdarg.h>

#ifndef EXTERN
#define EXTERN extern
#endif

/*
 * Variables
 */

#ifndef USE_SYSLOG
EXTERN FILE *fp_console;
EXTERN char service_name[64];
#endif

void log_debug (char *fmt,...);
void log_err (char *s);
void initdebug(char *service);
void closedebug(void);

#endif

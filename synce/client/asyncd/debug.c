/*	
 *   debug - Routines d'affichage d'erreur
 *
 *   Copyright (C) unknown ?
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

#define	EXTERN

#include	"debug.h"

/* Syslog or not syslog ? */
#ifdef USE_SYSLOG

void log_debug (const char *fmt,...)
{
    char msg[256];

    va_list	listv;
    va_start(listv,fmt);
    vsprintf (msg, fmt, listv);
    syslog(LOG_DEBUG, msg);
}

void	initdebug(const char *service)
{
    openlog(service, LOG_PID | LOG_CONS, LOG_DAEMON);
}

void	closedebug(void)
{
	closelog();
}

void log_err (const char *s)
{
    syslog(LOG_ERR, s);
}

#else

void log_debug (char *fmt,...)
{
    va_list	listv;
    va_start(listv,fmt);
    fprintf (fp_console, "%s[%d] ",service_name, getpid());
    vfprintf (fp_console, fmt, listv);
    fprintf (fp_console, "\n\r");
}

void	initdebug(char *service)
{
    strcpy(service_name,service);
/*    if ((fp_console = fopen ("/dev/console", "w")) == NULL) {
        perror ("/dev/console");
        exit (1);
    }*/
	fp_console = stderr;
}

void	closedebug(void)
{
//    fclose (fp_console);
}

void log_err (char *s)
{
}

#endif /* USE_SYSLOG */


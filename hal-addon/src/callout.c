/* 
 * Copyright (c) 2005 Andrei Yurkevich <urruru@ru.ru>
 * Copyright (c) 2002 David Eriksson <twogood@users.sourceforge.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pty.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <glib.h>

#ifndef MAX_PATH
#define MAX_PATH 255
#endif

#define PPPD_TIMEOUT	10
#define INFO_TIMEOUT    30  /* interval between device info updates */

#include <config.h>
#include "misc.h"
#include "hal.h"
#include "udevinfo.h"
#include "info.h"

gboolean hal_verbose = 0;

static int    fdpty_pppd = 0;
static int    fdpty_dccm = 0;
static pid_t  pid_pppd = 0;
static pid_t  pid_dccm = 0;
static int    pppd_running = 0;
static int    dccm_running = 0;

/*
 * Drop all but necessary privileges from the callout when it runs as root.  Set the
 *  running user id to SYNCE_USER and group to SYNCE_GROUP
 *  (borrowed from HAL daemon)
 */

static void
drop_privileges ()
{
	struct passwd *pw = NULL;
	struct group *gr = NULL;

	/* determine user id */
	pw = getpwnam (SYNCE_USER);
	if (!pw)  {
		SHC_ERROR (("user " SYNCE_USER " does not exist"));
		exit (-1);
	}

	/* determine primary group id */
	gr = getgrnam (SYNCE_GROUP);
	if (!gr) {
		SHC_ERROR (("group " SYNCE_GROUP " does not exist"));
		exit (-1);
	}

	if (initgroups (SYNCE_USER, gr->gr_gid)) {
		SHC_ERROR (("could not initialize groups"));
		exit (-1);
	}

	if (setgid (gr->gr_gid)) {
		SHC_ERROR (("could not set group id"));
		exit (-1);
	}

	if( setuid (pw->pw_uid)) {
		SHC_ERROR (("could not set user id"));
		exit (-1);
	}
}        

static void
start_dccm_server (char  *passwd)
{
	gchar *dccm_argv[8] = { NULL, };
	gchar *dccm_envv[4] = { NULL, };
	guint i = 0;
        
	dccm_argv[i++] = DCCM_CMD;
	dccm_argv[i++] = "-f";      /* run in foreground */
	dccm_argv[i++] = "-s";      /* only run one session */

	if (strlen (passwd ? passwd : "") > 0) {
		dccm_argv[i++] = "-p";
		dccm_argv[i++] = passwd;
	}
	dccm_argv[i] = NULL;
	
	i = 0;
	dccm_envv[i++] = "SYNCE_CONF_DIR=" SYNCE_CONNECTION_DIR;
	dccm_envv[i++] = "SYNCE_PID_FILE_MODE=0640";
	dccm_envv[i++] = "SYNCE_CONNECTION_FILE_MODE=0640";
	dccm_envv[i++] = NULL;
    
	pid_dccm = forkpty (&fdpty_dccm, NULL, NULL, NULL);
    
	if (pid_dccm == -1) { /* fork() failed */
		SHC_ERROR ("Cannot fork(): %s", strerror(errno));
		exit (-1);
    }
    else if (pid_dccm == 0) { /* we are the child */
		usleep (1000);
		SHC_INFO ("dropping priveleges to " SYNCE_USER ":" SYNCE_GROUP);
		drop_privileges ();
		
		execve (dccm_argv[0], (char *const *) dccm_argv, (char *const *) dccm_envv); 

		SHC_ERROR ("cannot execve(): %s\n", strerror(errno));
		exit (-1);
    }
	else {/* we are the parent */
		SHC_INFO ("Started dccm, pid=%d", pid_dccm);
		dccm_running = 1;
	}
}

static void
start_ppp_connection (const char  *serial_device)
{
	const char *pppd_argv[] = {
		"/usr/sbin/pppd",
		serial_device, "115200",
		"connect", SYNCE_SERIAL_CHAT,
        "nodefaultroute",
		"local",
		"192.168.131.102:192.168.131.201",
		"ms-dns", "192.168.131.102",
        "crtscts",
		"nodetach",
		"noauth",
		"linkname", "synce-device", 
        NULL };
        
    pid_pppd = forkpty (&fdpty_pppd, NULL, NULL, NULL);
        
    if (pid_pppd == -1) { /* fork() failed */
        SHC_ERROR ("Cannot fork(): %s\n", strerror(errno));
        exit (1);
    }
    else if (pid_pppd == 0) { /* we are the child */
		
        usleep (1000);
        execve (pppd_argv[0], (char *const *) pppd_argv, NULL); 
		
        SHC_ERROR ("Cannot execve(): %s", strerror(errno));
        exit (1);
    }
    else { /* we are the parent */
        SHC_INFO ("Started pppd, pid=%d", pid_pppd);
        pppd_running = 1;
    }
}

static void
stop_ppp_connection ()
{
	SHC_INFO ("Terminating pppd...");
    write (fdpty_pppd, "\x03", 1);
}

static void
stop_dccm_server ()
{
	SHC_INFO ("Terminating dccm...");
    write (fdpty_dccm, "\x03", 1);
}

void
sigchld_handler (int        signal,
				 siginfo_t  *info,
				 void       *ucontext)
{
    if (info->si_pid == pid_pppd) {
        waitpid (pid_pppd, NULL, 0);
        SHC_INFO ("SIGCHLD from pppd[%d]", pid_pppd);
        pppd_running = 0;
    }
    else if (info->si_pid == pid_dccm) {
        waitpid (pid_dccm, NULL, 0);
        SHC_INFO ("SIGCHLD from dccm[%d]", pid_dccm);
        dccm_running = 0;
    }
    else
        SHC_WARN ("Caught SIGCHLD from unknown process %d", info->si_pid);
}

static void
sighup_handler (int  n)
{
	/* FIXME: implement */
	SHC_WARN ("Ignoring SIGHUP for now...");
}

static void
sigterm_handler (int  n)
{
	SHC_INFO ("Caught SIGTERM");
	stop_ppp_connection ();
	stop_dccm_server ();
}

int
main (int   argc,
	  char  *argv[])
{
  char buf[256] = { '\0', };
  char *serial_device = NULL;
  char *udi = getenv ("UDI");
  
  setenv ("SYNCE_CONF_DIR", SYNCE_CONNECTION_DIR, 0);
	
#ifndef ENABLE_VERBOSE
  if ((getenv ("HALD_VERBOSE")) != NULL)
#endif
	  hal_verbose = 1;	
	
	if (geteuid() != 0) {
		char *p = rindex (argv[0], '/') + 1;
		SHC_ERROR ("%s should be installed setuid root or run with root privileges!",
		           p ? (*p ? p : argv[0]) : argv[0]);
		exit (1);
	}
	
	/* first, read environment for device file */
	if ((serial_device = getenv ("SYNCE_SERIAL_DEVICE")) != NULL) {
		SHC_INFO ("using serial device %s (from $SYNCE_SERIAL_DEVICE)", serial_device);
		goto got_device;
	}
	
	if (udi == NULL) {
		SHC_ERROR ("UDI is NULL, quiting");
		return 1;
	}
	
	/* next, ask HAL device list */
	if ((serial_device = hal_check_device (udi)) != NULL) {
		SHC_INFO ("using serial device %s (from HAL)", serial_device);
		goto got_device;
	}
	
	/* last, ask udev */
	SHC_WARN ("serial device not set in HAL, using udevinfo");
	char *sysfs_path = hal_get_sysfs_path (udi);
	
	if (sysfs_path == NULL) {
		SHC_ERROR ("could not get sysfs path for device %s", udi);
		return 1;
	}
	
	sleep (1);
	
	if ((serial_device = udevinfo_get_node (sysfs_path)) != NULL)
		goto got_device;
	
	SHC_ERROR ("could not get device node for %s", sysfs_path);
	return 1;
	
got_device:
    
	/* install SIGTERM and SIGHUP handlers */
	signal(SIGHUP, sighup_handler);
	signal(SIGTERM, sigterm_handler);	

    setuid (0);
    setgid (0);

    struct sigaction sa;
    sa.sa_sigaction = sigchld_handler;
    sa.sa_flags = SA_SIGINFO;
    
    if (sigaction (SIGCHLD, &sa, NULL) != 0) {
        SHC_ERROR ("Cannot install SIGCHLD handler: %s", strerror (errno));
        return (1);
    }
    
    start_dccm_server (NULL); /* TODO: password */
	usleep (1000); /* prevent race condition */
    start_ppp_connection (serial_device);
    
    FILE *fpppd = fdopen (fdpty_pppd, "r");
	FILE *fdccm = fdopen (fdpty_dccm, "r");

    if (fpppd == NULL || fdccm == NULL) {
        SHC_ERROR ("Cannot fdopen() pty");
        stop_ppp_connection ();
        stop_dccm_server ();
        
        while (dccm_running && pppd_running) usleep (500); /* wait for pppd and dccm to stop */
            
        return (1);
    }
	
	gboolean dccm_got_connection = FALSE;
	fd_set fds_dccm, fds_pppd;
	struct timeval tv;
	int retval;
	time_t info_update = 0;
	
	/* make reading from dccm and pppd non-blocking */
	fcntl (fdpty_dccm, F_SETFL, O_NONBLOCK);
	fcntl (fdpty_pppd, F_SETFL, O_NONBLOCK);
	
    while (pppd_running && dccm_running) {
        memset (buf, 0, 256);
		
		/* read output from dccm, if any */
		FD_ZERO (&fds_dccm);
		FD_SET (fdpty_dccm, &fds_dccm);
		tv.tv_sec = 0;
		tv.tv_usec = 5000;
		
		retval = select (fdpty_dccm+1, &fds_dccm, NULL, NULL, &tv);
		if (retval == -1)
			SHC_WARN ("select() failed: %s", strerror (errno));
		else if (retval) {
			while (fgets (buf, 255, fdccm) != NULL) {
				if (buf[strlen (buf)-1] == '\n') {
					buf[strlen (buf)-1] = '\0';
				}
				SHC_INFO ("%s", buf);
				if (strstr (buf, "Talking to"))
					dccm_got_connection = TRUE;
			}
		}

		/* read output from pppd, if any */
		FD_ZERO (&fds_pppd);
		FD_SET (fdpty_pppd, &fds_pppd);
		tv.tv_sec = 0;
		tv.tv_usec = 5000;
		
		retval = select (fdpty_pppd+1, &fds_pppd, NULL, NULL, &tv);
		if (retval == -1)
			SHC_WARN ("select() failed: %s", strerror (errno));
		else if (retval) {
			while (fgets (buf, 255, fpppd) != NULL) {
				if (buf[strlen (buf)-1] == '\n') {
					buf[strlen (buf)-1] = '\0';
				}
				SHC_INFO ("%s", buf);
			}
		}
			
		/* update HAL properties for the connected device */
		if (dccm_got_connection) {
			if (info_update + INFO_TIMEOUT < time (NULL)) {
				hal_set_property (udi, PROPERTY_TYPE_BOOL,
								  "pda.pocketpc.connected",
								  (gpointer) TRUE);
								  
				hal_add_capability (udi, "battery");
				
				hal_set_property (udi, PROPERTY_TYPE_STRING,
				                  "battery.type", "pda");
				
				gchar *str = get_device_os_version ();
				if (str) {
					hal_set_property (udi, PROPERTY_TYPE_STRING,
									  "pda.pocketpc.os_version",
									  (gpointer) str);
					g_free (str);
				}
				
				str = get_device_arch ();
				if (str) {
					hal_set_property (udi, PROPERTY_TYPE_STRING,
									  "pda.pocketpc.arch",
									  (gpointer) str);
					g_free (str);
				}
				
				BatteryStatus *status = get_device_battery_status ();
				if (status) {
					hal_set_property (udi, PROPERTY_TYPE_BOOL,
					                  "battery.present",
									  (gpointer) status->present);
					hal_set_property (udi, PROPERTY_TYPE_BOOL,
					                  "battery.is_rechargeable",
									  (gpointer) status->rechargeable);
					hal_set_property (udi, PROPERTY_TYPE_BOOL,
					                  "battery.rechargeable.is_charging",
									  (gpointer) status->is_charging);
					hal_set_property (udi, PROPERTY_TYPE_BOOL,
					                  "battery.rechargeable.is_discharging",
									  (gpointer) status->is_discharging);
					hal_set_property (udi, PROPERTY_TYPE_STRING,
					                  "battery.charge_level.unit",
									  (gpointer) "percent");
					hal_set_property (udi, PROPERTY_TYPE_INT,
					                  "battery.charge_level.design",
									  (gpointer) 100);
					hal_set_property (udi, PROPERTY_TYPE_INT,
					                  "battery.charge_level.current",
									  (gpointer) status->level);
					hal_set_property (udi, PROPERTY_TYPE_INT,
					                  "battery.charge_level.last_full",
									  (gpointer) 100);
					hal_set_property (udi, PROPERTY_TYPE_INT,
					                  "battery.remaining_time",
									  (gpointer) status->remaining_time);
				}
				info_update = time (NULL);
			}
		}
    }
    
    if (dccm_running) {
        stop_dccm_server ();
        while (dccm_running) usleep (100);  /* wait for dccm to exit */
    }
    else if (pppd_running) {
        SHC_WARN ("dccm[%d] terminated", pid_dccm);
        while (read (fdpty_dccm, buf, 255) > 0)
            SHC_INFO ("dccm[%d]: %s", pid_dccm, buf);
        stop_ppp_connection ();
        while (pppd_running) usleep (100); /* wait for pppd to exit */
    }
    
    return (0);
}

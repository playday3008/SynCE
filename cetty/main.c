/*
 * Some of the code from mgetty
 * $Id$ Copyright (c) Gert Doering
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/times.h>

#include <sys/stat.h>
#include <signal.h>

#include "config.h"
#include "debug.h"

#include <ezV24/ezV24.h>

/* Waiting for a connection */
static int waiting=1;
static v24_port_t* UsedPort;


/* create a file with the process ID of the mgetty currently
 * active on a given device in it.
 */
static char pid_file_name[ MAXPATH ];
static void make_pid_file ( void )
{
	FILE * fp;

	sprintf( pid_file_name, "%s/cetty.pid", VARRUNDIR );

	fp = fopen( pid_file_name, "w" );
	if ( fp == NULL )
	{
		log_err( "can't create pid file :");
		log_err(  pid_file_name );
	} else {
		fprintf( fp, "%d\n", (int) getpid() ); fclose( fp );
	}
	if ( chmod( pid_file_name, 0644 ) != 0 )
	{
		log_err( "can't chmod() pid file" );
	}
}

void dumpErrorMessage ( int rc )
{
	switch ( rc )
	{
		case V24_E_OK: log_err("error-msg: V24_E_OK"); break;
		case V24_E_ILLBAUD: log_err("error-msg: V24_E_ILLBAUD"); break;
		case V24_E_ILLDATASZ: log_err("error-msg: V24_E_ILLDATASZ"); break;
		case V24_E_ILLHANDLE: log_err("error-msg: V24_E_ILLHANDLE"); break;
		case V24_E_ILLTIMEOUT: log_err("error-msg: V24_E_ILLTIMEOUT"); break;
		case V24_E_OPEN_LOCK: log_err("error-msg: V24_E_OPEN"); break;
		case V24_E_CREATE_LOCK: log_err("error-msg: V24_E_CREATE_LOCK"); break;
		case V24_E_KILL_LOCK: log_err("error-msg: V24_E_KILL_LOCK"); break;
		case V24_E_LOCK_EXIST: log_err("error-msg: V24_E_LOCK_EXIST"); break;
		case V24_E_NOMEM: log_err("error-msg: V24_E_NOMEM"); break;
		case V24_E_NULL_POINTER: log_err("error-msg: V24_E_NULL_POINTER"); break;
		case V24_E_OPEN: log_err("error-msg: V24_E_OPEN"); break;
		case V24_E_READ: log_err("error-msg: V24_E_READ"); break;
		case V24_E_WRITE: log_err("error-msg: V24_E_WRITE"); break;
		case V24_E_NOT_IMPLEMENTED: log_err("error-msg: V24_E_NOT_IMPLEMENTED"); break;
		case V24_E_DBG_STALE_LOCK: log_err("debug-msg: V24_E_DBG_STALE_LOCK"); break;
		default:  log_err("error-msg undefined?!?!"); break;
	}
}

static void mySignalHandler ( int reason )
{
	// just close the serial port
	(void) unlink( pid_file_name );
	closedebug();
	if( UsedPort )
	{
		v24ClosePort(UsedPort);
		UsedPort = NULL;
	}
	exit(99);
}

static void mySignalHandlerConnect ( int reason )
{
	if( waiting && UsedPort )
	{
		log_debug("info: trying to wake up device");
		v24FlushRxQueue( UsedPort );
		v24FlushTxQueue( UsedPort );
		v24SetDTR( UsedPort, 0 );
		v24SetRTS( UsedPort, 0 );
		sleep( 1 );
		v24SetRTS( UsedPort, 1 );
		v24Putc( UsedPort, '\r' );
		sleep( 1 );
		v24SetDTR( UsedPort, 1 );
	}
}

static void installSignalhandler ( void )
{
	signal(SIGINT,mySignalHandler);
	signal(SIGTERM,mySignalHandlerConnect);	
	signal(SIGUSR1,mySignalHandlerConnect);	
	signal(SIGUSR2,mySignalHandlerConnect);	
}

int main( int argc, char ** argv)
{
	int rc;
	int MyTimeOut = 1;
	int kar=0;
	char buf[16];
	int ppp_level = 0, ppp_escaped = 0;
	char   ppp_ch;
	pid_t pid;
	int MyOpenFlags = V24_STANDARD;
	char * DeviceName;


	/* Init debug channel (syslog) */
	initdebug("cetty");

	/* check command-line arguments */
	if( argc < 3 )
	{
		log_err("Syntax : cetty <device> <path_to_pppd> [<pppd_arg1> [<pppd_arg2>] ...]");
		return 1;
	}

	DeviceName = argv[1];

	log_debug("Using device '%s'", DeviceName );

	/* When dying, close the serial port */
	installSignalhandler();

	make_pid_file();

	MyOpenFlags |= V24_LOCK | V24_NON_BLOCK/* | V24_RTS_CTS*/ /*| V24_DEBUG_ON*/;
	UsedPort = NULL;

	do {
		/**
		 * Wait until the Lock is clear (serial port is not used anymore by pppd)
		 */
		do {
			UsedPort=v24OpenPort(DeviceName, MyOpenFlags);
			if ( UsedPort==NULL )
			{
				sleep( 1 );
			}
		} while( UsedPort == NULL );
	
		log_debug("info: port %s opened", DeviceName);

		/**
		 * then we have to configure the port.
		 */
		rc=v24SetParameters(UsedPort,V24_B115200,V24_8BIT,V24_NONE);
		if ( rc!=V24_E_OK )
		{
			dumpErrorMessage(rc);
			v24ClosePort(UsedPort);
			UsedPort = NULL;
			log_err("v24SetParameters : unable to set to 115200/8/NONE");
			return 1;
		}
		log_debug("info: parameter set to 115200,8,N,1");

		/**
		 * Setting timeouts
		 */
		if ( MyTimeOut > 0 )
		{
			rc=v24SetTimeouts(UsedPort,MyTimeOut*10);
			if ( rc==V24_E_NOT_IMPLEMENTED )
			{
				log_debug("info: setup of timeout is not available!");
			}
			else if ( rc!=V24_E_OK )
			{
				dumpErrorMessage(rc);
				v24ClosePort(UsedPort);
				UsedPort = NULL;
				log_err("v24SetTimeouts : unable to set");
				return 1;
			} else {
				log_debug("info: timeout is set to %dsec",MyTimeOut);
			}
		}

		/* We clean a little */
		log_debug("info: flushing data");
		v24FlushRxQueue( UsedPort );
		v24FlushTxQueue( UsedPort );

		/* We tell windows CE to start sending data */
		log_debug("info: Setting control lines");

		v24SetRTS( UsedPort, 1 );
		v24SetDTR( UsedPort, 1 );

		waiting = 1;
		ppp_level = 0;

		/* Then we wait for data */
		do {
			kar=v24Getc( UsedPort);
			if( ( kar == 0 ) || (kar == -1) )
			{
				rc = v24QueryErrno( UsedPort );
				if( rc != V24_E_TIMEOUT )
				{
					dumpErrorMessage(rc);
					v24ClosePort(UsedPort);
					UsedPort = NULL;
					log_err("v24Getc : an error occured");
					return 1;
				}
			} else if( kar == 'C' )
			{
				waiting = 0;
				rc = v24Read( UsedPort, buf, 5 );
				if ( rc < 0 )
				{
					rc = v24QueryErrno( UsedPort );
					dumpErrorMessage(rc);
					v24ClosePort(UsedPort);
					UsedPort = NULL;
					log_err("v24Read : an error occured");
					return 1;
				} else {
					if( rc != 5 )
					{
						log_debug("Received only %d chars !", rc );
						v24ClosePort(UsedPort);
						UsedPort = NULL;
						log_err("v24Read : bad number of chars");
						return 1;
					} else {
						if( strncmp( buf, "LIENT", 5 ) == 0 )
						{
							log_debug("received CLIENT, sent SERVER");
							sprintf(buf, "CLIENTSERVER");
							v24Write( UsedPort, buf, 12 );
						} else {
							buf[ 5 ] = 0;
							log_debug("Received %s !", buf );
							log_err("v24Read : not CLIENT !");
							v24ClosePort(UsedPort);
							UsedPort = NULL;
							return 1;
						}
					}
				}

			} else {
				waiting = 0;
				ppp_ch = kar;
	   		 		if (ppp_escaped) {
	   			 		ppp_ch = PPP_UNESCAPE(kar);
	   			 		ppp_escaped = 0;
	   		 		}
					if (ppp_ch == (char) PPP_ESCAPE) {
						ppp_escaped = 1;
					} else if (ppp_ch == (char) PPP_FRAME) {
						ppp_level = 1;
					} else if (ppp_ch == (char) PPP_STATION && ppp_level == 1) {
						ppp_level = 2;
					} else if (ppp_ch == (char) PPP_CONTROL && ppp_level == 2) {
						ppp_level = 3;
					} else if (ppp_ch == (char) PPP_LCP_HI && ppp_level == 3) {
						ppp_level = 4;
					} else if (ppp_ch == (char) PPP_LCP_LOW && ppp_level == 4)
					{
						ppp_level = 5;
					} else {
						ppp_level = 0;
						ppp_escaped = 0;
					}
				log_debug("ppp_level = %d", ppp_level );
			}
		} while ( ppp_level < 5 );
		/* At the end of all the stuff, we have close the port. ;-)
		*/

		rc=v24ClosePort(UsedPort);
		UsedPort = NULL;
		if ( rc!=V24_E_OK )
		{
			dumpErrorMessage(rc);
		} else {
			log_debug("info: port closed!");
		}

		if( ppp_level == 5 )
		{
			pid = fork();
			if( pid == 0 )
			{
				log_debug("info: starting pppd");
				execv( argv[2], argv+3 );
				return 0;
			} else if (pid > 0)
			{
				log_debug("info: sleeping 5s");
				sleep( 5 );
				log_debug("info: done sleeping !");
			} else {
				log_err("fork : error !");
			}
		}
	} while(1);

	return 0;
}

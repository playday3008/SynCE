/*
 *   asyncd - prototype daemon asyncd v0.01
 *
 *   Copyright (C) 2001 Ludovic LANGE.
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

#ifdef DEBUG
#include "debug.h"
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iconv.h>
#include <errno.h>
#include <string.h>

#include "little_endian.h"

#define ASYNCD_INPUT	0
#define ASYNCD_OUTPUT	1
#define PING 0x12345678
#define MAXBUF 512
#define TRUE (1==1)
#define FALSE !TRUE
#define LOCKBUFFERSIZE ( 2 * ( 4 + 1 ) )

#ifdef DEBUG
void printbuf( unsigned char * buf, unsigned int size )
{
	char buffer[4096];
	unsigned int i,j;
	for( i=0,j=0; (i<size) && (i<40); i++ )
	{
		sprintf( (buffer+j), "%02X ", (unsigned) buf[i] );
		j+=3;
	}
	sprintf( (buffer+j), "\n" );
	log_debug( buffer );
}
#endif

int compute_password( char *passphrase, char xorvalue, unsigned char *lockbuffer, unsigned int *lockbuffersize )
{
	int result = FALSE;
        char * in;
        char * out;
        iconv_t cd;
        size_t res;
	size_t inlen;
	size_t outlen;
	size_t i;

        inlen = 1 + strlen( passphrase );
	outlen = *lockbuffersize;
        cd = iconv_open( "UNICODELITTLE", "latin1" );
	if(cd == (iconv_t)-1)
	{
		log_debug("conversion not available");
		return FALSE;
	}

        in = (char *) passphrase;
        out = (char *) lockbuffer;
        res = iconv( cd, &in, &inlen, &out, &outlen );

	if(res==(size_t)-1)
	{
		log_debug("iconv() failed");
	}
        iconv_close( cd );

	log_debug ("compute_password : iconv Ok, passphrase = %s, xorvalue = 0x%X, res = %d, inlen = %d, outlen = %d", passphrase, xorvalue, res, inlen, outlen);

	*lockbuffersize = *lockbuffersize - outlen;
	for( i=0; i< *lockbuffersize; i++ )
	{
		lockbuffer[i] = (char) (lockbuffer[i] ^ xorvalue);
	}

#ifdef DEBUG
	printbuf( lockbuffer, *lockbuffersize );
#endif /* DEBUG */

	result = TRUE;

	return result;
}

int removeinfo( void )
{
        struct stat st;

        int result;

        result = stat( INFOFILE, &st );
        log_debug("stat...result = %d", result);
        if( result != -1 )
        {
                log_debug("Ready to unlink...");
                result = unlink( INFOFILE );
                log_debug("unlink...result = %d", result);
        }
	return result;
}

int saveinfo( char * devicetype, char * platformtype, char * devicename, unsigned char *lockbuffer, int lockbuffersize )
{
        struct sockaddr_in sockin;
        FILE * infofile;
        int result = -1;
        int file;
	int i;

        socklen_t sinlen = sizeof( struct sockaddr_in );
        result = getpeername( 0, (struct sockaddr *) &sockin, (socklen_t *)&sinlen );

        if( result <0 )
        {
                log_debug("Can't get socket name ! ");
                return result;
        }

        file = open( INFOFILE, O_RDWR | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE | S_IRGRP | S_IROTH );
        if( file < 0 )
        {
                log_debug("Can't open information file ");
                return result;
        }
        if( (infofile = fdopen( file, "a+" )) == NULL )
        {
                log_debug("Can't reopen information file ");
                return result;
        }
        fprintf( infofile, "# This file lists the known devices connected to this computer.\n#\n\n" );
        fprintf( infofile, "device %s {\n", inet_ntoa( /*(struct in_addr)*/ (sockin.sin_addr) ) );
        fprintf( infofile, "\tdevice-type \"%s\";\n", devicetype );
        fprintf( infofile, "\tplatform-type \"%s\";\n", platformtype );
        fprintf( infofile, "\tdevice-name \"%s\";\n", devicename );
	if( (lockbuffer!=NULL) && (lockbuffersize>0) )
	{
        	fprintf( infofile, "\tpassphrase-size \"%d\";\n", lockbuffersize );
        	fprintf( infofile, "\tpassphrase-data \"" );
		
		for( i=0; (i<lockbuffersize); i++ )
		{
			fprintf( infofile, "%02X", (unsigned) lockbuffer[i] );
			if( (i+1)<lockbuffersize )
			{
				fprintf( infofile, " " );
			}
		}
		fprintf( infofile, "\";\n" );
	}
        fprintf( infofile, "}\n\n" );
        fclose( infofile );
	return result;
}

int checkpacket( unsigned char * buf, int size, unsigned char *lockbuffer, int lockbuffersize )
{
	int result = FALSE;
        long offset1;
        size_t strglen;
        size_t nameslen;
        long * ptrlng;
        char * in;
        char names[1024];
        char * out;
	char * devicetype;
        char * platformtype;
        char * devicename;
        iconv_t cd;
        size_t res;

	log_debug ("buffer : 0x%08X", buf);
        ptrlng = (long*)(buf+4);                /* Start of buffer; */
	log_debug ("ptrlng : 0x%08X", ptrlng);
        offset1 = letoh32(* ptrlng );        /* Offset to the string len, or 0x24 */
	log_debug ("offset1 : 0x%08X", offset1);

        ptrlng = (long*)(buf + offset1 );
	log_debug ("ptrlng : 0x%08X", ptrlng);
        strglen = (size_t)letoh32(* (ptrlng) );
	log_debug ("strglen : 0x%08X", strglen);
        ptrlng = (long*)(buf + offset1 + 4 );

#ifdef DEBUG
        printbuf( (unsigned char*)ptrlng, strglen );
#endif
	/* Conversion */
	/* ---------- */
        cd = iconv_open( "latin1", "UNICODELITTLE" );
	if(cd == (iconv_t)-1)
	{
		log_debug("conversion not available");
		return FALSE;
	}
        in = (char *) ptrlng;
        out = (char *) names;
        nameslen = 1024;
        res = iconv( cd, &in, &strglen, &out, &nameslen );
	if(res==(size_t)-1)
	{
		log_debug("iconv() failed");
	}
        iconv_close( cd );
	log_debug ("conv Ok, res = %d, strglen = %d, nameslen = %d, names = %s", res, strglen, nameslen, names);

	/* Split the different parts of the device identification */
	/* ------------------------------------------------------ */
	devicename = names;
	log_debug ("names = 0x%08X, names = %s, devicename = %s", names, names, devicename );

	/* Skip first '\0' and bookmark the position */
        for( in=names; (in<out)&&((*in)!=0); in++ );
        in++;
        platformtype = in;
	log_debug ("names = 0x%08X, out = 0x%08X, platformtype = 0x%08X, platformtype = %s", names, out, platformtype, platformtype);

	/* Skip second '\0' and bookmark the position */
        for( ; (in<out) && ( (*in)!=0); in++);
        in++;
        devicetype = in;
	log_debug ("names = 0x%08X, out = 0x%08X, devicetype = 0x%08X, devicetype = %s", names, out, devicetype, devicetype);

        saveinfo( devicetype, platformtype, devicename, lockbuffer, lockbuffersize );

	/* We should check the packet, then, if it's ok, fork a process
	   that mount the device as a filesystem. */
	result = TRUE;
	return result;
}

/*
 *	Partie principale
 */
int main (int ac, char **av)
{
	unsigned char buffer[MAXBUF];
	unsigned int error;
	fd_set set;
	unsigned long pktsz;
	struct timeval tv;
	int retval;
	int end_connexion = FALSE;
	int buffer_ok = FALSE;
	int pending = FALSE;
	int locked = FALSE;
	int locked_pending = FALSE;
	unsigned int locksignature = 0;
	unsigned int lockbuffersize = LOCKBUFFERSIZE;
	unsigned char lockbuffer[1+LOCKBUFFERSIZE];

#ifdef DEBUG
	initdebug("asyncd");
	log_debug ("Client connection...");
#endif

	while (!end_connexion) {
		/* IN */
		
		FD_ZERO( &set );
		FD_SET( ASYNCD_INPUT, &set );
		
		tv.tv_sec = 5;	/* 5sec */
		tv.tv_usec = 0;	

		retval = select( (1+ASYNCD_INPUT), &set, NULL, NULL, &tv );
	
		if( retval )	/* Data read */
		{
#ifdef DEBUG
			log_debug("Data received :");
#endif
			if(locked_pending)
			{
				error=read( ASYNCD_INPUT, buffer, 2);
				/* 1 -> correct password */
				/* 0 -> wrong password, you have 3 attempts altogether */
#ifdef DEBUG
				log_debug("locked reply pending:");
				printbuf(buffer, 2);
#endif
			
				locked_pending=FALSE;
				goto ping;
			}
			
			error=read( ASYNCD_INPUT, buffer, 4);
		
			pktsz = letoh32(*(long *)buffer);
			if( pktsz == PING )
			{
				/* Is there a pending ping ? */
				if( pending )
				{
#ifdef DEBUG
					log_debug(" ping reply received");
#endif
					pending = FALSE;
				} else {
#ifdef DEBUG
					log_debug(" unexpected ping reply!");
#endif
					end_connexion = TRUE;
				}
			}
			else
			{
				
#ifdef DEBUG
				log_debug (" packet size (hex) : %02X %02X %02X %02X",buffer[0], buffer[1], buffer[2], buffer[3]);
				log_debug (" packet size (int) : 0x%08X %lu",pktsz, pktsz);
#endif

				if( pktsz > 0 )
				{
					if( pktsz < MAXBUF-4 )
					{
						error=read(ASYNCD_INPUT, &buffer[4], pktsz);
#ifdef DEBUG
						log_debug(" buffer received");
						printbuf( buffer, pktsz );
#endif
						/* Here, we should call a functionc to analyze the buffer
						   and, if it's a valid Device identification, fork a
						   process that will mount the filesystem. */

						if(locked)
						{
							/*unsigned char lockbuffer[]={0xa, 0x0, 0xce, 0xfd, 0xc8, 0xfd, 0xc9, 0xfd, 0xcf, 0xfd, 0xfd, 0xfd};*/
							char *passphrase = "3542";
							compute_password( passphrase, locksignature & 0xFF, lockbuffer+2, &lockbuffersize ); 
							*((short *)lockbuffer) = htole16( lockbuffersize );
							
							error = write( ASYNCD_OUTPUT, lockbuffer, lockbuffersize + 2 );

							/* 1234: 0x0a 0x00 0xcc 0xfd 0xcf 0xfd 0xce 0xfd 0xc9 0xfd 0xfd 0xfd */
							/* 1111: 0x0a 0x00 0xcc 0xfd 0xcc 0xfd 0xcc 0xfd 0xcc 0xfd 0xfd 0xfd */
							/* algorithm to "encrypt" password:
							   
							   take unicode representation of 4 letter password + 0 term
							   xor each byte with 0xfd
							   that's it!
							   
							*/

#ifdef DEBUG				
							log_debug(" sending passphrase");
							printbuf( buffer, lockbuffersize + 2);
#endif
							locked_pending = TRUE;

							buffer_ok = checkpacket( buffer, pktsz, lockbuffer, lockbuffersize + 2 );

						}
						else
						{
							buffer_ok = checkpacket( buffer, pktsz, NULL, 0 );
							goto ping;
						}
					} else {
						if( pktsz > 0x00010000 )
						{
							locksignature = pktsz;
							log_debug(" device is locked, signature = 0x%08X", locksignature);
							locked = TRUE;
						} else {
#ifdef DEBUG
							log_debug(" buffer too small !! Aborting");
#endif
							end_connexion = TRUE;
						}
					}
				} else {
#ifdef DEBUG
					log_debug(" empty data - can be normal");
#endif
				}
			}
		} else {	/* No data, timeout */
#ifdef DEBUG				
			log_debug("Timeout received :");
#endif
			if( locked_pending)
			{
				end_connexion = TRUE;
				break;
			}

			if( buffer_ok && !pending )
			{
			  ping:
				*(long *)buffer = htole32(PING);
				error = write( ASYNCD_OUTPUT, buffer, 4 );
#ifdef DEBUG				
				log_debug(" sending ping");
				printbuf( buffer, 4 );
#endif				
				pending = TRUE;
			} else {
#ifdef DEBUG				
				log_debug(" timeout, aborting");
#endif
				end_connexion = TRUE;
			}
		}
	}

        removeinfo();

#ifdef DEBUG
	closedebug();
#endif

	return 0;
}


/*****************************************************************************
 * Avantgo CE Stream Protocol Commands
 * This file describes the basic operations performed over the stream
 * to the Avantgo libraries. The actual I/O is handled by the reader,
 * which should be specified in syncmain.c.
 */


/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is "agsync"
 *
 * The Initial Developer of the Original Code is
 * Michael Jarrett
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */


#ifndef SYNCSTREAM_H
#define SYNCSTREAM_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WITH_AGSYNC

/* Set to the Avantgo-returned error in error conditions.
   A result code will always be 0 for Ok, 1 for Ok but there's more stuff,
   2 for error, or -1 for a socket problem. In case of result 2, the
   error code will be stored in asErrno
*/

extern int asErrno;

struct AGUserConfig;
struct AGDBConfig;
struct AGDeviceInfo;
struct AGRecord;
struct AGReader;
struct AGWriter;

/**
 * Writes an "End Session" command to the stream.
 * This does NOT read off the end-of-stream stuff from the RAPI
 * protocol.
 * @return The result code.
 */
int asEndSession(AGReader *r, AGWriter *w);

/**
 * Retrieve a user config from the device.
 * @return The user configuration read, or NULL on error.
 *         If NULL, the state of the passed configuration structure
 *         is undefined, and asErrno may or may not be set.
 */
AGUserConfig *asGetUserConfig(AGReader *r, AGWriter *w,
				     AGUserConfig *uc);


/**
 * Write a user config to the device.
 * @return Result code of operation.
 */ 
int asPutUserConfig(AGReader *r, AGWriter *w, AGUserConfig *uc);


/**
 * Begin server.
 * This specifies that we are now processing for a specific server UID.
 * @return Result code. If 2, error code should be in asErrno.
 *         I've seen 0x00 in asErrno mean the uid can't be found.
 */
int asStartServer(AGReader *r, AGWriter *w, int uid);


/**
 * End server.
 * End of block specified by asStartServer. Does it even exist?
 * @return Result code. Practically always 0.
 */
int asEndServer(AGReader *r, AGWriter *w);


/**
 * Read device info from the device.
 * @return The device info read, or NULL if failed.
 *         If NULL returned, state of devInfo undefined, and
 *         asErrno may or may not be set.
 */
AGDeviceInfo *asGetDeviceInfo(AGReader *r, AGWriter *w,
				     AGDeviceInfo *devInfo);



/**
 * Read record from the device.
 * Unfortunately, since we need information from the record ahead of time,
 * we have to allocate it. It's up to the receiver to free it (or not).
 * @return Result code.
 *         If 0, no more records available, agrPtr= NULL.
 *         If 1, one record read. agrPtr= ptr to new record.
 *         If 2, asErrno is set. agrPtr= NULL.
 */
int asGetNextRecord(AGReader *r, AGWriter *w, AGRecord **agrPtr);

/**
 * Read next MODIFIED record from the device.
 * Unfortunately, since we need information from the record ahead of time,
 * we have to allocate it. It's up to the receiver to free it (or not).
 * @return Result code.
 *         If 0, no more records available, agrPtr= NULL.
 *         If 1, one record read. agrPtr= ptr to new record.
 *         If 2, asErrno is set. agrPtr= NULL.
 */
int asGetNextModifiedRecord(AGReader *r, AGWriter *w,AGRecord **agrPtr);


/**
 * Open/Initialize a database.
 * Return Standard result code.
 */
int asOpenDatabase(AGReader *r, AGWriter *w, AGDBConfig *db);



/**
 * Perform a command on the device, as specified by AGProtocol.h.
 * @return Always AGCLIENT_CONTINUE. For some reason all commands
 *         implicity request more commands.
 */
int asPerformCommand(AGReader *r, AGWriter *w, int cmd,
			    unsigned char *cmdData, int len);

#endif
#endif

///////////////////////////////////////////////////////////////////////////////
// ERRORCODE.C
//
// Error string handler module for RTFCOMP
//
// Dr J A Gow : 28/02/07
//
// This file is distributed under the terms and conditions of the LGPL - please
// see the file LICENCE in the package root directory.
//
//
///////////////////////////////////////////////////////////////////////////////

#include <rtfcomp/rtfcomp.h>
#include "sysincludes.h"


const char *	ecs[LZRTF_ERR_MAXERRCODE] = {
						"No error",
						"Out of memory",
						"Bad size in compressed RTF block",
						"Bad CRC in compressed RTF block",
						"Invalid arguments to function",
						"Bad magic number in compressed RTF block",
						"Invalid data in input stream"
					};


const char * ivec = "Invalid error code";

///////////////////////////////////////////////////////////////////////////////
// LZRTFGetStringErrorCode
//
// EXPORTED, DLLAPI
//
// Return a user-friendly error message for each defined error code.
//
///////////////////////////////////////////////////////////////////////////////

_DLLAPI const char * LZRTFGetStringErrorCode(int ec)
{
	if((ec>(LZRTF_ERR_MAXERRCODE-1))||(ec<0)) {
		return ivec;
	} else {
		return ecs[ec];
	}
}

/***************************************************************************
 *   Copyright (C) 2001 by Ludovic LANGE                                   *
 *   ludovic.lange@free.fr                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#include "windows.h"
#include "rapi.h"
#include <stddef.h>
#include <stdio.h>

#define BUFSIZE 16384

int main( int argc, char* argv[] )
{

//	int retval;
//	unsigned char buffer[BUFSIZE];
//	long size=BUFSIZE;
//	long index =0, lng;
//	int i;
//	HKEY hKey;
	WORD cFindData;
	HANDLE hnd;
//	unsigned char bufclass[BUFSIZE];
//	long sizeclass=BUFSIZE;
//	long cSubKeys;
	BOOL result;
	CEDB_FIND_DATA *pFindData;
	WORD cPropID;
	CEPROPID	rgPropID;
	LPBYTE lpBuffer;
	DWORD	cbBuffer;

	if( CeRapiInit( "192.168.131.201") )
	{
                pFindData = NULL;
/*		printf("Testing CeFindAllDatabase : ");
		result = CeFindAllDatabases( 0,
                        FAD_OID |
                        FAD_FLAGS |
                        FAD_NAME |
                        FAD_TYPE |
                        FAD_NUM_RECORDS |
                        FAD_NUM_SORT_ORDER |
			FAD_SIZE |
			FAD_LAST_MODIFIED |
                        FAD_SORT_SPECS 
			, &cFindData, &pFindData );*/
		printf("Testing CeOpenDatabase : ");
		hnd = CeOpenDatabase( 0xD0D, NULL, 0, CEDB_AUTOINCREMENT, NULL );
		if( hnd == INVALID_HANDLE_VALUE )
		{
			printf( "Error !\n" );
			switch( CeGetLastError() )
			{
				case ERROR_INVALID_PARAMETER:
					printf("Error_Invalid_Parameter\n");
					break;

				case ERROR_FILE_NOT_FOUND:
					printf("Error_File_Not_Found\n");
					break;

				case ERROR_NOT_ENOUGH_MEMORY:
					printf("Error_Not_Enough_Memory\n");
					break;
			}
		} else {
			printf( "result : handle = %08X\n", hnd );
			cbBuffer = 0;
			lpBuffer = NULL;
			result = CeReadRecordProps( hnd, CEDB_ALLOWREALLOC, &cPropID, &rgPropID, &lpBuffer, &cbBuffer );
			printf( "result : lpBuffer = %08lX cbBuffer = %08X\n", lpBuffer, cbBuffer );
			if( lpBuffer )
			{
				LocalFree( lpBuffer );
			}
			CeCloseHandle( hnd );
		}

	} else {
		printf( "Error initializing library !\n" );
	}

	CeRapiUninit();

	return 0;

}

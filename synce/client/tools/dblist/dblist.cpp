/***************************************************************************
 *   Copyright (C) 2001 by Ludovic LANGE                                   *
 *   ludovic.lange@free.fr                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#include <stdlib.h>
#include <ctype.h>
#include <rapi.h>
#include <unistd.h>

void BackupOneOrMoreDB( )
{
	char name[ CEDB_MAXDBASENAMELEN ];
	LPCEDB_FIND_DATA pFindData = NULL;
	WORD cFindData = 0;
	WORD i;


	if ( CeFindAllDatabases( 0, 0xFFFF, &cFindData, &pFindData ) )
	{
		for ( i = 0 ; i < cFindData ; i++ )
		{
			wcstombs( name, pFindData[ i ].DbInfo.szDbaseName, CEDB_MAXDBASENAMELEN );
                        printf("Database : id = 0x%08X '%s' ", ( unsigned ) pFindData[ i ].OidDb, name );
			printf( "type = %d ", ( unsigned ) pFindData[ i ].DbInfo.dwDbaseType );
			printf( "size = %d\n", pFindData[ i ].DbInfo.wNumRecords );
		}
	}
	else
	{
		printf( "CeFindAllDatabases() returned error...\n" );
	}

	if ( pFindData )
        {
		CeRapiFreeBuffer( pFindData );
        }
}

int main( int argc, char *argv[] )
{

	if( CeRapiInit() != E_SUCCESS )
	{
		printf( "Connection NOK !\n" );
		return 1;
	} else {
        	printf( "Connected\n" );
        }

	BackupOneOrMoreDB( );

	CeRapiUninit();

	return 0;
}

/***************************************************************************
 *   Copyright (C) 2001 by Ludovic LANGE                                   *
 *   ludovic.lange@free.fr                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#include <libxml/tree.h>
#include <stdlib.h>
#include <ctype.h>
#include <rapi.h>
#include <unistd.h>

void BackupProp( xmlNodePtr tree, CEOID oid, WORD cPropID, LPBYTE lpBuffer, DWORD cbBuffer )
{
	DWORD i;
	DWORD j;
	PCEPROPVAL pval = ( PCEPROPVAL ) lpBuffer;
	char buffer[ 1024 ];
	xmlNodePtr subtree1, subtree2;

	subtree1 = xmlNewChild( tree, NULL, ( unsigned char * ) "RECORD", NULL );
	sprintf( buffer, "0x%08X", ( unsigned ) oid );
	xmlSetProp( subtree1, ( unsigned char * ) "oid", ( unsigned char * ) buffer );
	sprintf( buffer, "%d", cPropID );
	xmlSetProp( subtree1, ( unsigned char * ) "cPropID", ( unsigned char * ) buffer );
	sprintf( buffer, "%ld", cbBuffer );
	xmlSetProp( subtree1, ( unsigned char * ) "cbBuffer", ( unsigned char * ) buffer );

	printf( "Property : 0x%-8X, : ", ( unsigned ) oid );

	for ( i = 0; i < cPropID; i++ )
	{
		subtree2 = NULL;
		printf( " propid = %-4X ", ( unsigned ) ( pval[ i ].propid >> 16 ) );
		switch ( ( pval[ i ].propid & 0xFFFF ) )
		{
			case CEVT_BLOB:
				printf( "BLOB (size : %ld, ptr %-8X) ", pval[ i ].val.blob.dwCount, ( unsigned ) pval[ i ].val.blob.lpb );
				for ( j = 0; j < pval[ i ].val.blob.dwCount; j++ )
				{
					sprintf( buffer + 3 * j, "%02X ", pval[ i ].val.blob.lpb[ j ] );
				}
				subtree2 = xmlNewChild( subtree1, NULL, ( unsigned char * ) "BLOB", ( unsigned char * ) buffer );
				sprintf( buffer, "%ld", pval[ i ].val.blob.dwCount );
				xmlSetProp( subtree2, ( unsigned char * ) "dwCount", ( unsigned char * ) buffer );
				break;
			case CEVT_BOOL:
				printf( "BOOL %d ", ( unsigned ) pval[ i ].val.boolVal );
				sprintf( buffer, "%s", pval[ i ].val.boolVal ? "true" : "false" );
				subtree2 = xmlNewChild( subtree1, NULL, ( unsigned char * ) "BLOB", ( unsigned char * ) buffer );
				break;
			case CEVT_FILETIME:
				printf( "FILETIME 0x%-8x ", pval[ i ].val.filetime );
				sprintf( buffer, "0x%08X", pval[ i ].val.filetime );
				subtree2 = xmlNewChild( subtree1, NULL, ( unsigned char * ) "FILETIME", ( unsigned char * ) buffer );
				break;
			case CEVT_I2:
				printf( "16bit INT %d ", pval[ i ].val.iVal );
				sprintf( buffer, "%d", pval[ i ].val.iVal );
				subtree2 = xmlNewChild( subtree1, NULL, ( unsigned char * ) "I2", ( unsigned char * ) buffer );
				break;
			case CEVT_I4:
				printf( "32bit INT %ld ", pval[ i ].val.lVal );
				sprintf( buffer, "%ld", pval[ i ].val.lVal );
				subtree2 = xmlNewChild( subtree1, NULL, ( unsigned char * ) "I4", ( unsigned char * ) buffer );
				break;
			case CEVT_LPWSTR:
				printf( "LPWSTR 0x%-8x ", ( unsigned ) pval[ i ].val.lpwstr );
				wcstombs( buffer, pval[ i ].val.lpwstr , 1024 );
				printf( " '%s' ", buffer );
				subtree2 = xmlNewChild( subtree1, NULL, ( unsigned char * ) "LPWSTR", ( unsigned char * ) buffer );
				break;
			case CEVT_R8:
				printf( "64bit INT %e ", pval[ i ].val.dblVal );
				sprintf( buffer, "%e", pval[ i ].val.dblVal );
				subtree2 = xmlNewChild( subtree1, NULL, ( unsigned char * ) "R8", ( unsigned char * ) buffer );
				break;
			case CEVT_UI2:
				printf( "16bit UINT %d ", pval[ i ].val.uiVal );
				sprintf( buffer, "%d", pval[ i ].val.uiVal );
				subtree2 = xmlNewChild( subtree1, NULL, ( unsigned char * ) "UI2", ( unsigned char * ) buffer );
				break;
			case CEVT_UI4:
				printf( "32bit UINT %ld ", pval[ i ].val.ulVal );
				sprintf( buffer, "%ld", pval[ i ].val.ulVal );
				subtree2 = xmlNewChild( subtree1, NULL, ( unsigned char * ) "UI4", ( unsigned char * ) buffer );
				break;
			default:
				printf( "UNKNOWN 0x%08lX ", pval[ i ].val.ulVal );
				sprintf( buffer, "0x%08lX", pval[ i ].val.ulVal );
				subtree2 = xmlNewChild( subtree1, NULL, ( unsigned char * ) "UNKNOWN", ( unsigned char * ) buffer );
				break;
		}
		sprintf( buffer, "0x%04lX", ( pval[ i ].propid >> 16 ) );
		xmlSetProp( subtree2, ( unsigned char * ) "propid", ( unsigned char * ) buffer );

	}
	/*for( i=0; i<cbBuffer; i++)
{
		if( isalnum( lpBuffer[i] ) )
		{
			printf("%c", lpBuffer[i] );
		} else {
			printf("<%02X>", lpBuffer[i] );
		}
}*/
	printf( "\n" );
}

void BackupOneDB( xmlNodePtr tree, CEDB_FIND_DATA findData )
{
	CEOID oid;
	CEGUID ceguid;
	//	BOOL mounted;
	WORD propID;
	LPBYTE lpBuffer;
	DWORD cbBuffer;
	DWORD index;
	DWORD error;
	xmlNodePtr subtree0, subtree1, subtree2, subtree3;
	char buffer[ 1024 ];

	index = 0;
	error = 0;

	printf( "Backuping database OID : 0x%-8X\n", ( unsigned ) findData.OidDb );

	subtree0 = xmlNewChild( tree, NULL, ( unsigned char * ) "DATABASE", NULL );

	wcstombs( buffer, findData.DbInfo.szDbaseName , CEDB_MAXDBASENAMELEN );
	xmlSetProp( subtree0, ( unsigned char * ) "szDbaseName", ( unsigned char * ) buffer );
	sprintf( buffer, "0x%08X", ( unsigned ) findData.DbInfo.dwDbaseType );
	xmlSetProp( subtree0, ( unsigned char * ) "dwDbaseType", ( unsigned char * ) buffer );
	sprintf( buffer, "%d", findData.DbInfo.wNumRecords );
	xmlSetProp( subtree0, ( unsigned char * ) "wNumRecords", ( unsigned char * ) buffer );
	sprintf( buffer, "%d", findData.DbInfo.wNumSortOrder );
	xmlSetProp( subtree0, ( unsigned char * ) "wNumSortOrder", ( unsigned char * ) buffer );
	sprintf( buffer, "0x%08X", findData.DbInfo.ftLastModified );
	xmlSetProp( subtree0, ( unsigned char * ) "ftLastModified", ( unsigned char * ) buffer );
	sprintf( buffer, "%ld", findData.DbInfo.dwSize );
	xmlSetProp( subtree0, ( unsigned char * ) "dwSize", ( unsigned char * ) buffer );
	xmlSetProp( subtree0, ( unsigned char * ) "compressed", ( findData.DbInfo.dwFlags & CEDB_NOCOMPRESS ) ? ( unsigned char * ) "false" : ( unsigned char * ) "true" );
	sprintf( buffer, "0x%08X", ( unsigned ) findData.OidDb );
	xmlSetProp( subtree0, ( unsigned char * ) "OidDb", ( unsigned char * ) buffer );


	subtree1 = xmlNewChild( subtree0, NULL, ( unsigned char * ) "PROPERTIES", NULL );

	if ( findData.DbInfo.wNumSortOrder > 0 )
	{
		subtree2 = xmlNewChild( subtree1, NULL, ( unsigned char * ) "SORTORDERS", NULL );
		SORTORDERSPEC *sos = findData.DbInfo.rgSortSpecs;
		for ( int i = 0; i < findData.DbInfo.wNumSortOrder; i++ )
		{
			subtree3 = xmlNewChild( subtree2, NULL, ( unsigned char * ) "ORDER", NULL );
			sprintf( buffer, "0x%08X", ( unsigned ) sos[ i ].propid );
			xmlSetProp( subtree3, ( unsigned char * ) "propid", ( unsigned char* ) buffer );
			sprintf( buffer, "0x%08X", ( unsigned ) sos[ i ].dwFlags );
			xmlSetProp( subtree3, ( unsigned char * ) "flags", ( unsigned char* ) buffer );
		}
	}


	subtree1 = xmlNewChild( subtree0, NULL, ( unsigned char * ) "VALUES", NULL );

	CREATE_INVALIDGUID( &ceguid );

	//	mounted = CeMountDBVol( &ceguid,

	HANDLE hDb = CeOpenDatabase( &( findData.OidDb ), NULL, 0, CEDB_AUTOINCREMENT, NULL );

	if ( hDb == INVALID_HANDLE_VALUE )
	{
		printf( "Invalid handle value..\n" );
	}
	else
	{
		printf( "Database opened, handle = 0x%-8X\n", ( unsigned ) hDb );
		lpBuffer = NULL;
		do
		{
			cbBuffer = 0;
			propID = 0;
			oid = CeReadRecordProps( hDb, CEDB_ALLOWREALLOC, &propID, NULL, &lpBuffer, &cbBuffer );
			if ( oid == 0 )
			{
				error = CeGetLastError();
				switch ( error )
				{
					case ERROR_INVALID_PARAMETER:
						printf( "Invalid parameter !\n" );
						break;
					case ERROR_NO_DATA:
						printf( "No data !\n" );
						break;
					case ERROR_INSUFFICIENT_BUFFER:
						printf( "Insufficient buffer !\n" );
						break;
					case ERROR_KEY_DELETED:
						printf( "Key deleted !\n" );
						break;
					case ERROR_NO_MORE_ITEMS:
						printf( "No more items !\n" );
						break;
				}
			}
			else
			{
				printf( "Ok index = %ld, oid = 0x%-8X, propID = %d, cbBuffer = %ld!\n", index, ( unsigned ) oid, propID, cbBuffer );
				BackupProp( subtree1, oid, propID, lpBuffer, cbBuffer );
			}
			if ( ( cbBuffer > 0 ) && ( lpBuffer ) )
			{
				LocalFree( lpBuffer );
				lpBuffer = NULL;
			}
			index ++;
		}
		while ( ( oid != 0 ) && ( error != ERROR_NO_MORE_ITEMS ) && ( index < 70 ) );
	}

	if ( hDb != INVALID_HANDLE_VALUE )
	{
		CeCloseHandle( hDb );
	}
}

void BackupOneOrMoreDB( xmlNodePtr tree, DWORD type )
{
	char name[ CEDB_MAXDBASENAMELEN ];
	LPCEDB_FIND_DATA pFindData = NULL;
	WORD cFindData = 0;
	WORD i;


	if ( CeFindAllDatabases( type, 0xFFFF, &cFindData, &pFindData ) )
	{
		for ( i = 0 ; i < cFindData ; i++ )
		{
			wcstombs( name, pFindData[ i ].DbInfo.szDbaseName, CEDB_MAXDBASENAMELEN );
                        printf("Backup database : id = 0x%08X'%s' ", ( unsigned ) pFindData[ i ].OidDb, name );
			printf( "type = %d ", ( unsigned ) pFindData[ i ].DbInfo.dwDbaseType );
			printf( "size = %d\n", pFindData[ i ].DbInfo.wNumRecords );
			BackupOneDB( tree, pFindData[ i ] );
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
	xmlDocPtr doc;
	char buffer[ 1024 ];
	int dbtype = 0;
        int c=0;

        sprintf( buffer, "dbbackup.xml" );
        while( c!=-1 )
        {
                c=getopt( argc, argv, "t?h" );
                switch( c )
                {
                        case -1:
                                break;
                        case 't':
                                dbtype = atoi( argv[ optind ] );
                                sprintf( buffer, "dbbackup-type-%08X.xml", dbtype );
                                break;
                        default:
                        case '?':
                        case 'h':
                                fprintf( stderr, "DBBackup [-t <type>]\n" );
                                return 1;
                                break;
                }
        }

	if( CeRapiInit() != E_SUCCESS )
	{
		printf( "Connection NOK !\n" );
		return 1;
	} else {
        	printf( "Connected\n" );
        }

	doc = xmlNewDoc( ( unsigned char * ) "1.0" );

	// Repository
	doc->children = xmlNewDocNode( doc, NULL, ( unsigned char * ) "DBBACKUP", NULL );
	xmlSetProp( doc->children, ( unsigned char * ) "version", ( unsigned char * ) "0.1" );

	BackupOneOrMoreDB( doc->children, dbtype );

	//xmlSaveFile( buffer, doc );
	xmlSaveFileEnc( buffer, doc, "UTF-8" );

	CeRapiUninit();

	return 0;
}

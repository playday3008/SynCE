/***************************************************************************
 *   Copyright (C) 2001 by Ludovic LANGE                                   *
 *   ludovic.lange@free.fr                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************
 * Parts of this file are :                                                *
 * (c) mingw header files.                                                 *
 * "This file is part of a free library for the Win32 API.                 *
 * This library is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE."                   *
 ***************************************************************************/
/* rapi.h - main header file for the RAPI API
 
        NOTE: This strictly does not belong in the Win32 API since it's
        really part of Platform SDK.
 
*/

#ifndef _RAPI_H
#define _RAPI_H

#include <windows.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

	typedef struct
	{
		DWORD a;
		DWORD b;
		DWORD c;
		DWORD d;
	}
	CEGUID;
	typedef CEGUID *PCEGUID;

#define CREATE_INVALIDGUID(guid) (memset((guid), -1, sizeof((guid))))

#define MAX_PATH 256

	typedef struct IRAPIStream
	{
		struct IRAPIStreamVtbl * lpVtbl;
	}
	IRAPIStream;

	typedef struct IRAPIStreamVtbl IRAPIStreamVtbl;

	typedef enum tagRAPISTREAMFLAG
	{
	    STREAM_TIMEOUT_READ
	} RAPISTREAMFLAG;

	struct IRAPIStreamVtbl
	{
		HRESULT ( * SetRapiStat ) ( IRAPIStream *, RAPISTREAMFLAG, DWORD );
		HRESULT ( * GetRapiStat ) ( IRAPIStream *, RAPISTREAMFLAG, DWORD * );
	};

	typedef HRESULT ( STDAPICALLTYPE RAPIEXT ) ( DWORD, BYTE, DWORD, BYTE, IRAPIStream * );

	typedef struct _RAPIINIT
	{
		DWORD cbSize;
		HANDLE heRapiInit;
		HRESULT hrRapiInit;
	}
	RAPIINIT;

#define CERAPI_E_ALREADYINITIALIZED 0x8004101
	STDAPI CeRapiInit(void);
	STDAPI CeRapiInitEx ( RAPIINIT* );
	STDAPI_( BOOL ) CeCreateProcess ( LPCWSTR, LPCWSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES,
	                                  BOOL, DWORD, LPVOID, LPWSTR, LPSTARTUPINFO, LPPROCESS_INFORMATION );
	STDAPI CeRapiUninit(void);

#define GENERIC_WRITE              0x40000000
#define GENERIC_READ               0x80000000
#define FILE_SHARE_READ            0x00000001

/* dwCreationDisposition */
#define CREATE_NEW          1
#define CREATE_ALWAYS       2
#define OPEN_EXISTING       3
#define OPEN_ALWAYS         4
#define TRUNCATE_EXISTING   5
#define OPEN_FOR_LOADER     6

	STDAPI_( HANDLE ) CeCreateFile ( LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE );
	STDAPI_( BOOL ) CeCreateDirectory ( LPCWSTR, LPSECURITY_ATTRIBUTES );
	STDAPI_( DWORD ) CeGetLastError ( void );
	STDAPI_( BOOL ) CeGetFileTime ( HANDLE, LPFILETIME, LPFILETIME, LPFILETIME );
	STDAPI_( BOOL ) CeCloseHandle ( HANDLE );

	/* LLA ADDON */

	STDAPI_( HRESULT ) CeRapiFreeBuffer( LPVOID Buffer );
	STDAPI_( HRESULT ) CeRapiGetError( void );
	STDAPI_( HRESULT ) CeRapiInvoke( LPCWSTR, LPCWSTR, DWORD, BYTE *, DWORD *, BYTE **, IRAPIStream **, DWORD );

	STDAPI_( BOOL ) CeCopyFile( LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists );
	STDAPI_( BOOL ) CeDeleteFile( LPCWSTR lpFileName );

	typedef struct _CE_FIND_DATA
	{
		DWORD dwFileAttributes;
		FILETIME ftCreationTime;
		FILETIME ftLastAccessTime;
		FILETIME ftLastWriteTime;
		DWORD nFileSizeHigh;
		DWORD nFileSizeLow;
		DWORD dwOID;
		TCHAR cFileName[ MAX_PATH ];
	}
	CE_FIND_DATA, *LPCE_FIND_DATA;

	typedef CE_FIND_DATA** LPLPCE_FIND_DATA;

#define FAF_ATTRIBUTES			((DWORD) 0x00001)
#define FAF_CREATION_TIME		((DWORD) 0x00002)
#define FAF_LASTACCESS_TIME		((DWORD) 0x00004)
#define FAF_LASTWRITE_TIME		((DWORD) 0x00008)

#define FAF_SIZE_HIGH			((DWORD) 0x00010)
#define FAF_SIZE_LOW			((DWORD) 0x00020)
#define FAF_OID				((DWORD) 0x00040)
#define FAF_NAME			((DWORD) 0x00080)

#define FAF_ATTRIB_CHILDREN		((DWORD) 0x01000)
#define FAF_ATTRIB_NO_HIDDEN		((DWORD) 0x02000)
#define FAF_FOLDERS_ONLY		((DWORD) 0x04000)
#define FAF_NO_HIDDEN_SYS_ROMMODULES	((DWORD) 0x08000)

#define FILE_ATTRIBUTE_READONLY		0x00000001
#define FILE_ATTRIBUTE_HIDDEN		0x00000002
#define FILE_ATTRIBUTE_SYSTEM		0x00000004
#define FILE_ATTRIBUTE_1 		0x00000008

#define FILE_ATTRIBUTE_DIRECTORY	0x00000010
#define FILE_ATTRIBUTE_ARCHIVE		0x00000020
#define FILE_ATTRIBUTE_INROM		0x00000040
#define FILE_ATTRIBUTE_NORMAL		0x00000080

#define FILE_ATTRIBUTE_TEMPORARY	0x00000100
#define FILE_ATTRIBUTE_2        	0x00000200
#define FILE_ATTRIBUTE_3        	0x00000400
#define FILE_ATTRIBUTE_COMPRESSED	0x00000800

#define FILE_ATTRIBUTE_ROMSTATICREF	0x00001000
#define FILE_ATTRIBUTE_ROMMODULE	0x00002000
#define FILE_ATTRIBUTE_4        	0x00004000
#define FILE_ATTRIBUTE_5        	0x00008000

#define FILE_ATTRIBUTE_HAS_CHILDREN	0x00010000
#define FILE_ATTRIBUTE_SHORTCUT		0x00020000
#define FILE_ATTRIBUTE_6        	0x00040000
#define FILE_ATTRIBUTE_7        	0x00080000

#define FAF_GETTARGET        ((DWORD)                    0x10000)
	STDAPI_( BOOL ) CeFindAllFiles( LPCWSTR szPath, DWORD dwFlags, LPDWORD lpdwFoundCount, LPLPCE_FIND_DATA ppFindDataArray );
	STDAPI_( HANDLE ) CeFindFirstFile( LPCWSTR lpFileName, LPCE_FIND_DATA lpFileFindData );
	STDAPI_( BOOL ) CeFindNextFile( HANDLE hFindFile,  /*LPWIN32_FIND_DATA ?*/LPCE_FIND_DATA lpFileFindData );
	STDAPI_( DWORD ) CeGetFileAttributes( LPCWSTR lpFileName );
	STDAPI_( DWORD ) CeGetFileSize( HANDLE hFile, LPDWORD lpFileSizeHigh );


#define CSIDL_PROGRAMS           0x0002
#define CSIDL_PERSONAL           0x0005
#define CSIDL_FAVORITES_GRYPHON  0x0006
#define CSIDL_STARTUP            0x0007
#define CSIDL_RECENT             0x0008
#define CSIDL_STARTMENU          0x000b
#define CSIDL_DESKTOPDIRECTORY   0x0010
#define CSIDL_FONTS		 0x0014
#define CSIDL_FAVORITES		 0x0016

	STDAPI_( DWORD )  CeGetSpecialFolderPath(  int nFolder,  DWORD nBufferLength,  LPWSTR lpBuffer );

	STDAPI_( DWORD ) CeGetTempPath( DWORD nBufferLength, LPWSTR lpBuffer );
	STDAPI_( BOOL ) CeMoveFile( LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName );
	STDAPI_( BOOL ) CeReadFile( HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped );
        STDAPI_( BOOL ) CeWriteFile( HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped );
	STDAPI_( BOOL ) CeRemoveDirectory( LPCWSTR lpPathName );
	STDAPI_( BOOL ) CeSetEndOfFile( HANDLE hFile );
	STDAPI_( BOOL ) CeSetFileAttributes( LPCWSTR lpFileName, DWORD dwFileAttributes );
	STDAPI_( DWORD ) CeSetFilePointer( HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod );
	STDAPI_( BOOL ) CeSetFileTime( HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime );
	STDAPI_( DWORD ) CeSHCreateShortcut( LPWSTR lpszShortcut, LPWSTR lpszTarget );
	STDAPI_( BOOL ) CeSHGetShortcutTarget( LPWSTR lpszShortcut, LPWSTR lpszTarget, INT cbMax );

#define HKEY_CLASSES_ROOT           ( ( HKEY ) 0x80000000 )
#define HKEY_CURRENT_USER           ( ( HKEY ) 0x80000001 )
#define HKEY_LOCAL_MACHINE          ( ( HKEY ) 0x80000002 )
#define HKEY_USERS                  ( ( HKEY ) 0x80000003 )
#define REG_CREATED_NEW_KEY         ( 0x00000001L )
#define REG_OPENED_EXISTING_KEY     ( 0x00000002L )

#define REG_NONE		0
#define REG_SZ			1
#define	REG_EXPAND_SZ		2
#define REG_BINARY		3
#define REG_DWORD		4
#define REG_LINK		6
#define REG_MULTI_SZ		7
#define REG_RESOURCE_LIST	8

#define ERROR_NO_MORE_ITEMS         259
	STDAPI_( LONG )  CeRegOpenKeyEx(  HKEY,  LPCWSTR,  DWORD,  REGSAM,  PHKEY );
	STDAPI_( LONG )  CeRegEnumKeyEx(  HKEY,  DWORD,  LPWSTR,  LPDWORD,  LPDWORD,  LPWSTR,  LPDWORD,  PFILETIME );
	STDAPI_( LONG )  CeRegCreateKeyEx(  HKEY,  LPCWSTR,  DWORD,  LPWSTR,  DWORD,  REGSAM,  LPSECURITY_ATTRIBUTES,  PHKEY,  LPDWORD );
	STDAPI_( LONG )  CeRegCloseKey(  HKEY );
	STDAPI_( LONG )  CeRegDeleteKey(  HKEY,  LPCWSTR );
	STDAPI_( LONG )  CeRegEnumValue(  HKEY,  DWORD,  LPWSTR,  LPDWORD,  LPDWORD,  LPDWORD,  LPBYTE,  LPDWORD );
	STDAPI_( LONG )  CeRegDeleteValue(  HKEY,  LPCWSTR );
	STDAPI_( LONG )  CeRegQueryInfoKey(  HKEY,  LPWSTR,  LPDWORD,  LPDWORD,  LPDWORD,  LPDWORD,  LPDWORD,  LPDWORD,  LPDWORD,  LPDWORD,  LPDWORD,  PFILETIME );
	STDAPI_( LONG )  CeRegQueryValueEx(  HKEY,  LPCWSTR,  LPDWORD,  LPDWORD,  LPBYTE,  LPDWORD );
	STDAPI_( LONG )  CeRegSetValueEx(  HKEY,  LPCWSTR,  DWORD,  DWORD,  LPBYTE,  DWORD );

	/* Database */
	typedef DWORD CEPROPID;
	typedef CEPROPID *PCEPROPID;
#define TypeFromPropID(propid) LOWORD(propid)
	typedef DWORD CEOID;
	typedef CEOID *PCEOID;

#define CEDB_MAXDBASENAMELEN    32
#define CEDB_MAXSORTORDER       4

#define CEDB_VALIDNAME          0x0001
#define CEDB_VALIDTYPE          0x0002
#define CEDB_VALIDSORTSPEC      0x0004
#define CEDB_VALIDMODTIME       0x0008

#define CEDB_AUTOINCREMENT              0x00000001
#define CEDB_NOCOMPRESS                 0x00010000

#define CEDB_SORT_DESCENDING            0x00000001
#define CEDB_SORT_CASEINSENSITIVE       0x00000002
#define CEDB_SORT_UNKNOWNFIRST          0x00000004
#define CEDB_SORT_GENERICORDER          0x00000008

#define CEDB_SEEK_CEOID                 0x00000001
#define CEDB_SEEK_BEGINNING             0x00000002
#define CEDB_SEEK_END                   0x00000004
#define CEDB_SEEK_CURRENT               0x00000008

#define CEDB_SEEK_VALUESMALLER          0x00000010
#define CEDB_SEEK_VALUEFIRSTEQUAL       0x00000020
#define CEDB_SEEK_VALUEGREATER          0x00000040
#define CEDB_SEEK_VALUENEXTEQUAL        0x00000080

#define CEDB_PROPNOTFOUND               0x0100
#define CEDB_PROPDELETE                 0x0200
#define CEDB_MAXDATABLOCKSIZE           4092
#define CEDB_MAXPROPDATASIZE            (CEDB_MAXDATABLOCKSIZE*16)
#define CEDB_MAXRECORDSIZE              (128*1024)
#define CEDB_ALLOWREALLOC               0x00000001

#define FAD_OID                         0x0001
#define FAD_FLAGS                       0x0002
#define FAD_NAME                        0x0004
#define FAD_TYPE                        0x0008

#define FAD_NUM_RECORDS                 0x0010
#define FAD_NUM_SORT_ORDER              0x0020
#define FAD_SIZE                        0x0040
#define FAD_LAST_MODIFIED               0x0080

#define FAD_SORT_SPECS                  0x0100

	typedef struct _SORTORDERSPEC
	{
		CEPROPID propid;
		DWORD dwFlags;
	}
	SORTORDERSPEC;

	typedef struct _CEDBASEINFO
	{
		DWORD dwFlags;
		WCHAR szDbaseName[ CEDB_MAXDBASENAMELEN ];
		DWORD dwDbaseType;
		WORD wNumRecords;
		WORD wNumSortOrder;
		DWORD dwSize;
		FILETIME ftLastModified;
		SORTORDERSPEC rgSortSpecs[ CEDB_MAXSORTORDER ];
	}
	CEDBASEINFO;

	typedef struct _CEDB_FIND_DATA
	{
		CEOID OidDb;
		CEDBASEINFO DbInfo;
	}
	CEDB_FIND_DATA, *LPCEDB_FIND_DATA;

	typedef CEDB_FIND_DATA ** LPLPCEDB_FIND_DATA;

	typedef struct _CEBLOB
	{
		DWORD dwCount;
		LPBYTE lpb;
	}
	CEBLOB;

#define CEVT_I2         2
#define CEVT_I4         3
#define CEVT_R8         5
#define CEVT_BOOL       11
#define CEVT_UI2        18
#define CEVT_UI4        19
#define CEVT_LPWSTR     31
#define CEVT_FILETIME   64
#define CEVT_BLOB       65

	typedef union _CEVALUNION {
		short iVal;
		USHORT uiVal;
		long lVal;
		ULONG ulVal;
		FILETIME filetime;
		LPWSTR lpwstr;
		CEBLOB blob;
		BOOL boolVal;
		double dblVal;
	} CEVALUNION;

	typedef struct _CEPROPVAL
	{
		CEPROPID propid;
		WORD wLenData;
		WORD wFlags;
		CEVALUNION val;
	}
	CEPROPVAL, *PCEPROPVAL;

	STDAPI_( BOOL ) CeFindAllDatabases( DWORD, WORD, LPWORD, LPLPCEDB_FIND_DATA );
	STDAPI_( HANDLE ) CeOpenDatabase( PCEOID, LPWSTR, CEPROPID, DWORD, HWND );
	STDAPI_( CEOID ) CeReadRecordProps( HANDLE, DWORD, LPWORD, CEPROPID*, LPBYTE*, LPDWORD );
	/*
	STDAPI_(BOOL) CeCheckPassword( LPWSTR );

	STDAPI_(CEOID) CeCreateDatabase( LPWSTR, DWORD, WORD, SORTORDERSPEC* );
	STDAPI_(BOOL) CeDeleteDatabase( CEOID );
	STDAPI_(HANDLE) CeFindFirstDatabase( DWORD );
	STDAPI_(CEOID) CeFindNextDatabase( HANDLE );
	STDAPI_(CEOID) CeSeekDatabase( HANDLE, DWORD, LONG, LPDWORD );
	STDAPI_(BOOL) CeSetDatabaseInfo( CEOID, CEDBASEINFO* );

	STDAPI_(BOOL) CeDeleteRecord( HANDLE, CEOID );
	 
	STDAPI_(BOOL) CeFindClose( HANDLE );

	STDAPI_(int) CeGetClassName( HWND, LPWSTR, int );
	STDAPI_(INT) CeGetDesktopDeviceCaps( INT );
	STDAPI_(BOOL) CeGetStoreInformation( LPSTORE_INFORMATION );
	STDAPI_(VOID) CeGetSystemInfo( LPSYSTEM_INFO );
	STDAPI_(INT) CeGetSystemMetrics( INT );
	STDAPI_(BOOL) CeGetSystemPowerStatusEx( PSYSTEM_POWER_STATUS_EX, BOOL );
	STDAPI_(BOOL) CeGetVersionEx( LPCEOSVERSIONINFO );
	STDAPI_(HWND) CeGetWindow( HWND, UINT );
	STDAPI_(LONG) CeGetWindowLong( HWND, int );
	STDAPI_(int) CeGetWindowText( HWND, LPWSTR, int );
	STDAPI_(VOID) CeGlobalMemoryStatus( LPMEMORYSTATUS );

	STDAPI_(BOOL) CeOidGetInfo( CEOID, CEOIDINFO* );


	STDAPI_(CEOID) CeWriteRecordProps( HANDLE, CEOID, WORD, CEPROPVAL* );
	*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _RAPI_H */


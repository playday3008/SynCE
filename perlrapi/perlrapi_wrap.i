/* Perl Wrapper for librapi2 of the SynCE Project (http://synce.sourceforge.net)
 *
 * AUTHOR: Andreas Pohl <osar@users.sourceforge.net>
 *
 * NOTE: A lot of work was done by Richard Taylor, the author of the PyRapi, so
 * have look at his project. I kept most of the behavior of the PyRapi, so both
 * wrappers should be usable very similar in their language.
 *
 * $Id$
 */


%module Rapi2
%{
#include "rapi.h"
#include "synce_log.h"
%}
  
/*
 * Include all of the constant definitions.
 * Function and type definitions are masked by the SWIG 
 * preprocessor directive.
 */

%include "rapi.h" 

%include "typemaps.i"

/*
 * Tracing functions
 * TODO: Add synce_trace etc.
 */
void synce_log_set_level(int);

%typemap(out) HRESULT
{
  if(S_OK != $1)
    croak("Failed to initialize rapi");

  $result=sv_newmortal();
  sv_setiv($result, (IV)$1);
  argvi++;
}

HRESULT CeRapiInit(void);



/******************************
 ***  FILE ACCESS FUNTIONS  ***
 ******************************/

/* 
 * SWIG does not understand u_int style type declarations
 * so we have to tell it what the rapi types are.
 */

typedef unsigned int WORD;
typedef unsigned long DWORD;
typedef unsigned int BOOL;
typedef unsigned long * LPDWORD;

typedef char * LPSTR;
typedef char * WCHAR;
typedef const char * LPCSTR;

/* 
 * FILETIME must be mapped to and from unixtime. The typemap is used for 
 * functions that take FILETIME as a parameter and the GETSET macro is 
 * used to add vertual members to classes that have FILETIME members.
 */

typedef struct _FILETIME
{
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

%typemap(in) FILETIME
{
  if (SvIOK($input))
    $1=filetime_from_unixtime((time_t)SvIV($input));
  else 
    croak("expected an integer (time since epoch).");
}

%define FILETIMEGETSET(type,name)
  %{
  const unsigned int type ## _ ## name ## _get(type *ptr)
    {
      return (const unsigned int) filetime_to_unix_time(&ptr-> ## name);
    }
  void type ## _ ## name ##_set(type *ptr, const unsigned int value) 
    {
      // TODO: check length of string.
      filetime_from_unix_time((time_t)value, &ptr-> ## name );
    }
  %}
%enddef

/*
 * SWIG does not understand unicode so these macros and typemaps provide 
 * conversion to and from ascii for structures and functions that use wide 
 * character arrays.
 */

%typemap(in) LPCWSTR, LPWSTR
{
  if (SvPOK($input)) 
    $1 = wstr_from_ascii(SvPV_nolen($input));
  else 
    croak("expected a string.");
}


%define WCHARGETSET(type,name)
  %{
  const char * type ## _ ## name ## _get(type *ptr)
    {
      return (const char *) wstr_to_ascii(ptr-> ## name);
    }
  void type ## _ ## name ## _set(type *ptr, const char *value)
    {
      //ptr-> ## name=(LPWSTR)malloc(sizeof(WCHAR)*strlen(value)+1);
      wstr_strcpy(ptr-> ## name, wstr_from_ascii(value));
    }
  %}
%enddef

%define LPWSTRGETSET(type,name)
  %{
  const char * type ## _ ## name ## _get(type *ptr)
    {
      return (const char *) wstr_to_ascii(ptr-> ## name);
    }
  void type ## _ ## name ## _set(type *ptr, const char *value)
    {
      ptr-> ## name=(LPWSTR)malloc(sizeof(WCHAR)*strlen(value)+1);
      wstr_strcpy(ptr-> ## name, wstr_from_ascii(value));
    }
  %}
%enddef

%typemap(in,numinputs=0) LPSECURITY_ATTRIBUTES lpSecurityAttributes (LPSECURITY_ATTRIBUTES temp)
{
  $1 = NULL;
}

/*
 * Checking the return value of functions that return handles.
 */

%typemap(out) HANDLE
{
  if($1 == INVALID_HANDLE_VALUE)
    croak("bad file handle.");

  HANDLE *ph=(HANDLE *)malloc(sizeof(HANDLE));
  *ph=$1;
  $result=sv_newmortal();
  SWIG_MakePtr($result, (void *)ph, SWIGTYPE_p_HANDLE, 0);
  argvi++;
}

/* 
 *  File handling functions.
 */

typedef struct _CE_FIND_DATA
{
  DWORD dwFileAttributes; 
  // FILETIME ftCreationTime; 
  // FILETIME ftLastAccessTime; 
  // FILETIME ftLastWriteTime; 
  DWORD nFileSizeHigh; 
  DWORD nFileSizeLow; 
  DWORD dwOID; 
  //WCHAR cFileName[MAX_PATH]; 
  %extend {
    const unsigned int ftCreationTime;
    const unsigned int ftLastAccessTime;
    const unsigned int ftLastWriteTime;
    const char *cFileName;
  }
} CE_FIND_DATA, *LPCE_FIND_DATA, **LPLPCE_FIND_DATA; 

FILETIMEGETSET(CE_FIND_DATA,ftCreationTime)
FILETIMEGETSET(CE_FIND_DATA,ftLastAccessTime)
FILETIMEGETSET(CE_FIND_DATA,ftLastWriteTime)
WCHARGETSET(CE_FIND_DATA,cFileName)

%typemap(argout) (LPDWORD lpdwFoundCount, LPLPCE_FIND_DATA ppFindDataArray)
{
  AV *ret=newAV();
  SV *sv;
  int i;
  CE_FIND_DATA *pFD=*$2;

  for(i=0;i<*$1;i++)
  {
    sv=newSV(0);
    SWIG_MakePtr(sv, (void *)pFD++ , SWIGTYPE_p_CE_FIND_DATA, 0);
    av_push(ret, sv);
  }

  $result=newRV((SV*)ret);
  sv_2mortal($result);
  argvi++;
}

%typemap(in,numinputs=0)  (LPDWORD lpdwFoundCount, LPLPCE_FIND_DATA ppFindDataArray) 
  (DWORD temp1, LPCE_FIND_DATA temp2)
{
  $1 = &temp1;
  $2 = &temp2;
}

%apply unsigned int *OUTPUT { LPDWORD lpdwFoundCount };

%typemap(out) BOOL
{
  $result=sv_newmortal();
  sv_setiv($result, (IV)$1);
  argvi++;
}


BOOL CeCloseHandle(HANDLE hObject);

HANDLE CeCreateFile(LPCWSTR lpFileName, 
		    DWORD dwDesiredAccess = GENERIC_READ, 
		    DWORD dwShareMode = 0, 
		    LPSECURITY_ATTRIBUTES lpSecurityAttributes, 
		    DWORD dwCreationDisposition = OPEN_EXISTING, 
		    DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL, 
		    HANDLE hTemplateFile = 0); 

%typemap(in) (LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead)
  (DWORD tmp)
{
  $3 = &tmp;
  if (SvIOK($input))
  {
    $2=SvIV($input);
    if(!($1=(LPVOID)malloc($2)))
      croak("out of memory");
  }
  else
    croak("expected an int for parameter 2");
}

%typemap(argout) (LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead)
{
  $result=sv_newmortal();
  sv_setpvn((SV *)$result, (char *)$1, *$3);
  argvi++;
}

BOOL CeReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, 
		LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped = NULL);

%typemap(in) (LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten) 
  (DWORD tmp)
{
  $3=&tmp;
  if(!SvPOK($input))
    croak("expected a string");
  $1=SvPV($input, $2);
}

%typemap(argout) (LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten)
{
  $result=sv_newmortal();
  sv_setiv($result, (IV)*$3);
  argvi++;
}

BOOL CeWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, 
		 LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped = NULL); 


BOOL CeCopyFileA(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists);

BOOL CeFindAllFiles(LPCWSTR szPath, DWORD dwFlags, LPDWORD lpdwFoundCount, 
		    LPLPCE_FIND_DATA ppFindDataArray);

DWORD CeGetLastError( void );

BOOL CeCopyFile(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists = 0);

BOOL CeCreateDirectory(LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL);

BOOL CeDeleteFile(LPCWSTR lpFileName);

%typemap(argout) (LPCWSTR lpFileName, LPCE_FIND_DATA lpFindFileData) 
{
  $result=sv_newmortal();
  SWIG_MakePtr($result, $2, SWIGTYPE_p_CE_FIND_DATA, 0);
  argvi++;
}

%typemap(in,numinputs=0) (LPCE_FIND_DATA lpFindFileData)
{
  $1=(LPCE_FIND_DATA)malloc(sizeof(CE_FIND_DATA));
}


HANDLE CeFindFirstFile(LPCWSTR lpFileName, LPCE_FIND_DATA lpFindFileData);

%typemap(argout) (HANDLE hFindFile, LPCE_FIND_DATA lpFindFileData)
{
  $result=sv_newmortal();
  SWIG_MakePtr($result, $2, SWIGTYPE_p_CE_FIND_DATA, 0);
  argvi++;
}

BOOL CeFindNextFile(HANDLE hFindFile, 
		    LPCE_FIND_DATA lpFindFileData); 

BOOL CeFindClose(HANDLE hFindFile);

DWORD CeGetFileAttributes(LPCWSTR lpFileName);

%typemap(in,numinputs=0) (DWORD nBufferLength, LPWSTR lpBuffer) 
  (WCHAR tmp[MAX_PATH])
{
  $1 = MAX_PATH;
  $2 = tmp;
}

%typemap(argout) (DWORD nBufferLength, LPWSTR lpBuffer)
{
  $result=sv_newmortal();
  sv_setpvn($result, wstr_to_ascii($2), result);
  argvi++;
}

DWORD CeGetSpecialFolderPath(int nFolder, DWORD nBufferLength, LPWSTR lpBuffer);

BOOL CeMoveFile(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName);

BOOL CeRemoveDirectory(LPCWSTR lpPathName);

BOOL CeSetFileAttributes(LPCWSTR lpFileName, DWORD dwFileAttributes);



/***********************************
 ***  DATABASE ACCESS FUNCTIONS  ***
 ***********************************/

typedef DWORD CEPROPID;
typedef CEPROPID *PCEPROPID;
typedef DWORD CEOID;
typedef CEOID *PCEOID;

typedef struct _CEBLOB {
  DWORD dwCount;
  //LPBYTE lpb;
  %extend
  {
    const char *lpb;
  }
} CEBLOB;

%{
  const char *CEBLOB_lpb_get(CEBLOB *ptr)
    {
      // This works just for BLOBs with string data.
      // TODO: Make this work for any binray data.
      char *ret=(char *)malloc(ptr->dwCount+1);
      memcpy(ret, ptr->lpb, ptr->dwCount);
      ret[ptr->dwCount]='\0';
      return ret;
    }
  void CEBLOB_lpb_set(CEBLOB *ptr, const char *value)
    {
      ptr->dwCount=(DWORD)strlen(value);
      ptr->lpb=(LPBYTE)malloc(ptr->dwCount);
      memcpy(ptr->lpb, value, ptr->dwCount);
    }
%}

typedef union _CEVALUNION
{
  short iVal; 
  //USHORT uiVal;
  WORD uiVal;
  long lVal; 
  //ULONG ulVal;
  DWORD ulVal;
  //FILETIME filetime;
  //LPWSTR lpwstr; 
  %extend
  {
    const unsigned int filetime;
    const char *lpwstr; 
  }

  CEBLOB blob; 
  BOOL boolVal;
  double dblVal;
} CEVALUNION;


FILETIMEGETSET(CEVALUNION, filetime)
LPWSTRGETSET(CEVALUNION, lpwstr)
  
typedef struct _CEPROPVAL
{ 
  //CEPROPID propid;
  WORD wLenData;
  WORD wFlags;
  CEVALUNION val;
  %extend
  {
    unsigned int type;
    unsigned int propid;
  }
} CEPROPVAL;

%{
  const unsigned int CEPROPVAL_type_get(CEPROPVAL *ptr) {
    return ptr->propid & 0xFFFF;
  }
  void CEPROPVAL_type_set(CEPROPVAL *ptr, const unsigned int value) {
    //printf("Setting propid to: %d\n", value);
    ptr->propid = (ptr->propid & 0xFFFF0000) | value;
    //printf("propid set to: %d\n", ptr->propid);
  }
  const unsigned int CEPROPVAL_propid_get(CEPROPVAL *ptr) {
    return (ptr->propid >> 16);
  }
  void CEPROPVAL_propid_set(CEPROPVAL *ptr, const unsigned int value) {
    CEPROPID tmp = (CEPROPID)value;
    ptr->propid = (tmp << 16) | ( ptr->propid & 0xFFFF);
  }
%}

typedef CEPROPVAL *PCEPROPVAL; 

typedef struct _SORTORDERSPEC {
	CEPROPID propid;
	DWORD dwFlags;
} SORTORDERSPEC; 

typedef struct _CEDBASEINFO {
  DWORD dwFlags; 
  //WCHAR szDbaseName[CEDB_MAXDBASENAMELEN];
  DWORD dwDbaseType;
  WORD wNumRecords;
  WORD wNumSortOrder;
  DWORD dwSize;
  //FILETIME ftLastModified;
  %extend {
    const unsigned int ftLastModified;
    const char * szDbaseName; 
  }
  SORTORDERSPEC rgSortSpecs[CEDB_MAXSORTORDER];
} CEDBASEINFO;

FILETIMEGETSET(CEDBASEINFO,ftLastModified)
WCHARGETSET(CEDBASEINFO,szDbaseName)

typedef struct _CEDB_FIND_DATA {
	CEOID OidDb;
	CEDBASEINFO DbInfo;
} CEDB_FIND_DATA, *LPCEDB_FIND_DATA, **LPLPCEDB_FIND_DATA;

typedef struct _CEFILEINFO
{
  DWORD dwAttributes; 
  CEOID oidParent; 
  //WCHAR szFileName[MAX_PATH]; 
  //FILETIME ftLastChanged;
  DWORD dwLength; 
  %extend
  {
    const unsigned int ftLastChanged;
    const char *szFileName;
  }
} CEFILEINFO; 

FILETIMEGETSET(CEFILEINFO,ftLastChanged)
WCHARGETSET(CEFILEINFO,szFileName)

typedef struct _CEDIRINFO
{
  DWORD dwAttributes; 
  CEOID oidParent; 
  //WCHAR szDirName[MAX_PATH];
  %extend
  {
    const char *szDirName;
  }
} CEDIRINFO; 

WCHARGETSET(CEDIRINFO,szDirName)
  
typedef struct _CERECORDINFO {
	CEOID oidParent; 
} CERECORDINFO;

typedef struct _CEOIDINFO { 
	WORD wObjType;
	WORD wPad;
	union {
		CEFILEINFO infFile;
		CEDIRINFO infDirectory;
		CEDBASEINFO infDatabase;
		CERECORDINFO infRecord;
	} u;
} CEOIDINFO;


%typemap(in) (WORD wNumSortOrder, SORTORDERSPEC *rgSortSpecs)
{
  if(! SvROK($input) && &PL_sv_undef != $input)
    craok("expected an ARRAY reference as parameter 3");
  if(&PL_sv_undef != $input)
  {
    if(SVt_PVAV != SvTYPE(SvRV($input)))
      croak("expected an ARRAY reference as parameter 3");
    
    AV *av=(AV *)SvRV($input);
    // av_len returns the highest index value in array, so 0 means 1 element.
    $1=(WORD)av_len(av)+1;
    $2 = (SORTORDERSPEC *)malloc($1*sizeof(SORTORDERSPEC));
    int i;
    for(i=0; i<$1; i++)
    {
      SORTORDERSPEC *tmp;
      SV *sv=av_shift(av);
      if((SWIG_ConvertPtr(sv, (void **) &tmp, SWIGTYPE_p_SORTORDERSPEC, 0)) < 0) 
	croak("no SORTORDERSPEC, the array must contain only SORTORDERSPEC values");
      
      memcpy(&($2[i]), tmp, sizeof(SORTORDERSPEC));
    }
  }
  else
  {
    $1=0;
    $2=NULL;
  }
}


CEOID CeCreateDatabase(LPWSTR lpszName, DWORD dwDbaseType, WORD wNumSortOrder, 
		       SORTORDERSPEC *rgSortSpecs);

BOOL CeDeleteDatabase(CEOID oid);


%typemap(argout) (LPWORD cFindData, LPLPCEDB_FIND_DATA ppFindData)
{
  AV *ret=newAV();
  SV *sv;
  int i;
  CEDB_FIND_DATA *pFD=*$2;

  for(i=0;i<*$1;i++)
  {
    sv=newSV(0);
    SWIG_MakePtr(sv, (void *)pFD++ , SWIGTYPE_p_CEDB_FIND_DATA, 0);
    av_push(ret, sv);
  }

  $result=newRV((SV*)ret);
  sv_2mortal($result);
  argvi++;
}


%typemap(in,numinputs=0) LPWORD cFindData (WORD temp) 
{
  $1 = &temp;
}

%typemap(in,numinputs=0) LPLPCEDB_FIND_DATA ppFindData (LPCEDB_FIND_DATA temp) {
  $1 = &temp;
}

BOOL CeFindAllDatabases(DWORD dwDbaseType = 0, WORD wFlags = 0xFFFF,
			LPWORD cFindData, LPLPCEDB_FIND_DATA ppFindData);

HANDLE CeFindFirstDatabase(DWORD dwDbaseType = 0);

CEOID CeFindNextDatabase(HANDLE hEnum);


%typemap(argout) CEOIDINFO *poidInfo
{
  if(OBJTYPE_INVALID == $1->wObjType)
    croak("bad CEOID");
  
  $result=sv_newmortal();
  SWIG_MakePtr($result, $1, SWIGTYPE_p_CEOIDINFO, 0);
  argvi++;
}

%typemap(in, numinputs=0) CEOIDINFO *poidInfo
{
  $1=(CEOIDINFO *)malloc(sizeof(CEOIDINFO));
}

BOOL CeOidGetInfo(CEOID oid, CEOIDINFO *poidInfo); 

/* Take a CEOID as input and pass a pinter to it to the C func. */
%typemap(in) PCEOID
{
  CEOID temp;
  if(! SvIOK($input))
    croak("expected an integer (database oid)");
  
  temp = (CEOID)SvIV($input);
  $1 = &temp;
}

%typemap(in,numinputs=0) HWND hwndNotify (HWND temp) {
  $1 = temp;
}

HANDLE CeOpenDatabase(PCEOID poid, LPWSTR lpszName, CEPROPID propid = 0, 
		      DWORD dwFlags = CEDB_AUTOINCREMENT, HWND hwndNotify); 


/* 
 * typemaps to support CeReadRecordProps
 */

%typemap(in,numinputs=0)
  (LPWORD lpcPropID, CEPROPID *rgPropID, LPBYTE *lplpBuffer, LPDWORD lpcbBuffer)
  (WORD temp1, CEPROPID temp2, BYTE *temp3, DWORD temp4)
{
  $1 = &temp1;
  $2 = NULL;
  $3 = &temp3;
  $4 = &temp4;
}

%typemap(argout) (LPBYTE *lplpBuffer, LPDWORD lpcbBuffer)
{
  AV *ret=newAV();
  SV *sv;
  int i;
  CEPROPVAL *val=(CEPROPVAL *)*$1;
  
  for(i=0; i<*arg3; i++)
  {
    sv=newSV(0);
    SWIG_MakePtr(sv, (void *)val++ , SWIGTYPE_p_CEPROPVAL, 0);
    av_push(ret, sv);
  }

  $result=newRV((SV*)ret);
  sv_2mortal($result);
  argvi++;  
}

CEOID CeReadRecordProps(HANDLE hDbase, DWORD dwFlags = CEDB_ALLOWREALLOC, 
		LPWORD lpcPropID, CEPROPID *rgPropID, LPBYTE *lplpBuffer, 
		LPDWORD lpcbBuffer); 

/*
 * typemaps to support CeSeekDatabase
 *
 * CeSeekDatabase ( bdh, seek_type, value )
 *    Returns (rec_oid, index) for success ( 0, index) for failure.
 *    The type of the values parameter is dependant on the seek_type
 *       CEDB_SEEK_CEOID            record_oid
 *       CEDB_SEEK_VALUESMALLER     CEPROPVAL
 *       CEDB_SEEK_VALUEFIRSTEQUAL  CEPROPVAL
 *       CEDB_SEEK_VALUENEXTEQUAL   CEPROPVAL
 *       CEDB_SEEK_VALUEGREATER     CEPROPVAL
 *       CEDB_SEEK_BEGINNING        index forwards from start
 *       CEDB_SEEK_CURRENT          index (positive for forward, neg for back)
 *       CEDB_SEEK_END              index backwards from end (neg index)
 */

%typemap(in) DWORD dwSeekType
{
  if(! SvIOK($input))
    croak("expected an integer for dwSeekType");
  $1=(DWORD)SvIV($input);
}

%typemap(in) DWORD dwValue
{
  switch(arg2)
  {
  case CEDB_SEEK_CEOID:
  case CEDB_SEEK_BEGINNING:
  case CEDB_SEEK_CURRENT:
  case CEDB_SEEK_END:
    if(! SvIOK($input))
      croak("expected an integer for dwValue if dwSeekType is set to %s, %s, %s or %s",
	    "CEDB_SEEK_CEOID", "CEDB_SEEK_BEGINNING", "CEDB_SEEK_CURRENT",
	    "CEDB_SEEK_END");
    $1=(DWORD)SvIV($input);
    break;
  case CEDB_SEEK_VALUESMALLER:
  case CEDB_SEEK_VALUEFIRSTEQUAL:
  case CEDB_SEEK_VALUENEXTEQUAL:
  case CEDB_SEEK_VALUEGREATER:
    {
      CEPROPVAL *val;
      if (SWIG_ConvertPtr($input, (void **) &val, SWIGTYPE_p_CEPROPVAL, 0) < 0)
	croak("expected _p_CEPROPVAL for dwValue if dwSeekType is set to %s, %s, %s or %s",
	      "CEDB_SEEK_VALUESMALLER", "CEDB_SEEK_VALUEFIRSTEQUAL",
	      "CEDB_SEEK_VALUENEXTEQUAL", "CEDB_SEEK_VALUEGREATER");
      $1=(DWORD)val;
      break;
    }
  default:
    croak("unknown seek type");
  }  
}

%typemap(in,numinputs=0) LPDWORD lpdwIndex (DWORD temp)
{
  $1 = &temp;
}

%typemap(argout) LPDWORD lpdwIndex
{
  $result=sv_newmortal();
  sv_setiv($result, *$1);
  argvi++;
}

CEOID CeSeekDatabase(HANDLE hDatabase, DWORD dwSeekType, DWORD dwValue, 
		     LPDWORD lpdwIndex);

/* 
 * typemaps to support CeWriteRecordProps
 *
 * CeWriteRecordProps (dbh, rec_oid, values)
 *     Returns the rec_oid for success 0 for failure. 
 *     values is a tuple of of CEPROPVAL classes.
 */

%typemap(in) (WORD cPropID, CEPROPVAL *rgPropVal)
{
  if(! SvROK($input))
    croak("expected an ARRAY reference for rgPropVal");
  if(SVt_PVAV != SvTYPE(SvRV($input)))
    croak("expected an ARRAY reference for rgPropVal");

  AV *av=(AV *)SvRV($input);
  // av_len returns the highest index value in array, so 0 means 1 element.
  $1=(WORD)av_len(av)+1;
  if($1<1)
    croak("the given array has zero length %d", $1);
  
  $2=(CEPROPVAL *)malloc($1 * sizeof(CEPROPVAL));
  int i;
  for(i=0; i<$1; i++)
  {
    CEPROPVAL *tmp;
    SV *sv=av_shift(av);
    if((SWIG_ConvertPtr(sv, (void **) &tmp, SWIGTYPE_p_CEPROPVAL, 0)) < 0) 
      croak("no CEPROPVAL, the array must contain only CEPROPVAL values");
    
    memcpy(&($2[i]), tmp, sizeof(CEPROPVAL));
  }
}

CEOID CeWriteRecordProps(HANDLE hDbase, CEOID oidRecord, WORD cPropID, 
			 CEPROPVAL *rgPropVal);




/***********************************
 ***  REGISTRY ACCESS FUNCTIONS  ***
 ***********************************/

typedef int LONG;
typedef int HKEY;
typedef int REGSAM;

%typemap(in,numinputs=0) PHKEY phkResult (HKEY k)
{
  $1=&k;
}

%typemap(argout) PHKEY phkResult
{
  $result=sv_newmortal();
  sv_setiv($result, *$1);
  argvi++;
}

LONG CeRegOpenKeyEx(HKEY hKey, LPCWSTR lpszSubKey, DWORD ulOptions=0, REGSAM samDesired=0, 
		    PHKEY phkResult);

LONG CeRegCloseKey(HKEY hKey);

%typemap(in, numinputs=0) (DWORD Reserved, LPWSTR lpszClass, DWORD ulOptions=0,
			   REGSAM samDesired,
			   LPSECURITY_ATTRIBUTES lpSecurityAttributes) {}
%typemap(in, numinputs=0) LPDWORD lpdwDisposition {}

LONG CeRegCreateKeyEx(HKEY hKey, LPCWSTR lpszSubKey, DWORD Reserved=0,
		      LPWSTR lpszClass=NULL, DWORD ulOptions=0, REGSAM samDesired=0,
		      LPSECURITY_ATTRIBUTES lpSecurityAttributes=NULL, PHKEY phkResult, 
		      LPDWORD lpdwDisposition=NULL);


/*
 * typmaps to support CeRegQueryInfoKey
 */

%typemap(in, numinputs=0) LPDWORD (DWORD t)
{
  $1=&t;
}

%typemap(in, numinputs=0) PFILETIME lpftLastWriteTime (FILETIME t)
{
  $1=&t;
}

%typemap(in, numinputs=0) (LPWSTR lpClass=NULL, LPDWORD lpcbClass,
			   LPDWORD lpReserved) {}

%typemap(argout) (LPDWORD lpcbClass, LPDWORD lpReserved) {}

%typemap(argout) LPDWORD
{
  $result=sv_newmortal();
  sv_setiv($result, *$1);
  argvi++;
}

%typemap(argout) PFILETIME
{
  unsigned int ret;
  if($1)
  {
    ret=(unsigned int)filetime_to_unix_time($1);
    $result=sv_newmortal();
    sv_setuv($result, ret);
    argvi++;
  }
}

LONG CeRegQueryInfoKey(HKEY hKey, LPWSTR lpClass=NULL, LPDWORD lpcbClass=0, 
		       LPDWORD lpReserved=0, LPDWORD lpcSubKeys,
		       LPDWORD lpcbMaxSubKeyLen, LPDWORD lpcbMaxClassLen, 
		       LPDWORD lpcValues, LPDWORD lpcbMaxValueNameLen, 
		       LPDWORD lpcbMaxValueLen, LPDWORD lpcbSecurityDescriptor, 
		       PFILETIME lpftLastWriteTime);


/*
 * typmaps to support CeRegQueryValueEx
 */

%typemap(in, numinputs=0) (LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
  (DWORD type, DWORD s)
{
  // SWIG sets the arguments after the 'in'-typemaps, but we need it here.
  arg1 = (HKEY) SvIV(ST(0));
  if (SvPOK(ST(1))) 
    arg2 = wstr_from_ascii(SvPV_nolen(ST(1)));
  else 
    croak("expected a string.");

  // firstly get the needed size
  LONG ret=CeRegQueryValueEx(arg1, arg2, NULL, NULL, NULL, &s);
  if(ERROR_SUCCESS == ret)
  {
    // allocate memory
    $2=(LPBYTE)malloc(s);
    $1=&type;
    $3=&s;
  }
  else
    croak("can not receive the buffer size: CeRegQueryValueEx failed (ret=%d)",
	  ret);
}

%typemap(in, numinputs=0) LPDWORD lpReserved {}
%typemap(argout) LPDWORD lpReserved {}

%typemap(argout) (LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
  if(ERROR_SUCCESS == result)
  {  
    // put the type on the stack
    $result=sv_newmortal();
    sv_setiv($result, *$1);
    argvi++;
    // put the data on the stack
    switch(*$1)
    {
    case REG_SZ:
    case REG_EXPAND_SZ:
      $result=sv_newmortal();
      sv_setpv($result, wstr_to_ascii((WCHAR *)$2));
      argvi++;
      break;
    case REG_MULTI_SZ: // An array of null-terminated strings, terminated by
      // two null characters.
      {
	AV *ret=newAV();
	SV *sv;
	int i;
	for(i=0; i<*$3; i++)
	{
	  if(! (short)$2[i])
	  {
	    sv=newSVpv(wstr_to_ascii((WCHAR *)&$2[i+2]), 0);
	    av_push(ret, sv);
	  }
	}
	$result=newRV((SV*)ret);
	sv_2mortal($result);
	argvi++;
	break;
      }
    case REG_DWORD:
    case REG_DWORD_BIG_ENDIAN:
      $result=sv_newmortal();
      sv_setiv($result, *$2);
      argvi++;
      break;
    case REG_BINARY:
    case REG_NONE:
    case REG_LINK:
      $result=sv_newmortal();
      sv_setpvn($result, (const char *)$2, *$3);
      argvi++;
      break;
    default:
      croak("unknown type: 0x%x", *$1);
    }
  }
}

LONG CeRegQueryValueEx(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, 
		       LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);


/*
 * typmaps to support CeRegEnumValue
 */

%typemap(in, numinputs=0) (LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
  (DWORD type, DWORD NameLen, DWORD ValueLen)
{
  // SWIG sets the arguments after the 'in'-typemaps, but we need it here.
  arg1 = (HKEY) SvIV(ST(0));

  // firstly get the needed size
  //LONG ret=CeRegQueryInfoKey(arg1, NULL, 0, 0, NULL, NULL, NULL, NULL,
  //			     &NameLen, NULL /* &ValueLen */, NULL, NULL);

  // TODO: Get the needed buffer size for a ValueName. CeRegQueryInfoKey works only
  // before a call to the CeRegEnumValue function. I don't wanted to put this task
  // on the Perl side. So I set the maximal buffersize to 512 bytes.
  NameLen=512; //sizeof(WCHAR)*(NameLen+1);
  
  //if(ERROR_SUCCESS == ret)
  {
    // allocate memory
    //$2=(LPBYTE)malloc(ValueLen);
    $2=NULL;
    //if(! $2)
    //  croak("can not allocate memory for $2");
    //$1=&type;
    //$3=&ValueLen;
    // lpszValueName
    arg3=(LPWSTR)malloc(NameLen);
    if(! arg3)
      croak("can not allocate memory for arg3");
    // lpcbValueName
    arg4=&NameLen;
  }
  //else
  //  croak("can not calculate the buffer size: CeRegQueryInfoKey failed (ret=%d)",
  //	  ret);
}

%typemap(in, numinputs=0) (LPWSTR lpszValueName, LPDWORD lpcbValueName) {}

/*
 * NOTE: CeRegEnumValue returns wrong data. The values (lpData) stand in the type
 * variable (lpType), and the count of databytes (lpcbData) in the data buffer
 * (lpData). I do not really understand this, and I do not know where in detail
 * lies the problem, maybe in the function on my CE device.
 *
 * TODO: Fix this problem and make CeRegEnumValue return the type and the data
 * like the CeRegQueryValueEx function.
 */
%typemap(argout) (LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {}

%typemap(argout)
  (LPWSTR lpszValueName, LPDWORD lpcbValueName),
  (LPWSTR lpName, LPDWORD lpcbName)
{
  if($1 && $2)
  {
    if(*$2)
    {
      $result=sv_newmortal();
      sv_setpvn($result, wstr_to_ascii($1), *$2);
      argvi++;
    }
  }
}

LONG CeRegEnumValue(HKEY hKey, DWORD dwIndex, LPWSTR lpszValueName, 
		    LPDWORD lpcbValueName, LPDWORD lpReserved=0,
		    LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);


/*
 * typemaps to support CeRegEnumKeyEx
 */

%typemap(in, numinputs=0)
  (LPWSTR lpName, LPDWORD lpcbName),
  (LPWSTR lpClass, LPDWORD lpcbClass)
{
  /*
   * NOTE: Here we have same problem as in the last typemaps to support
   * CeRegEnumValue: We do not know the needed buffer size. One way would
   * be to call CeRegQueryValueEx from Perl to get the max length. But
   * I would like to get rid of this from a Perl script, so I set the
   * buffer size to 512.
   *
   * TODO: Find a way to get the needed buffer size.
   */
  DWORD MaxSize=512;
  
  $1=(LPWSTR)calloc(MaxSize, 1);
  $2=&MaxSize;
}


LONG CeRegEnumKeyEx(HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcbName,
		    LPDWORD lpReserved=0, LPWSTR lpClass, LPDWORD lpcbClass, 
		    PFILETIME lpftLastWriteTime);


/*
 * typemaps to support CeRegSetValueEx
 */

%typemap(in) DWORD dwType
{
  if(! SvIOK($input))
    croak("expected an integer for dwType");
  $1=(DWORD)SvIV($input);
}

%typemap(in) const BYTE *lpData
{
  // arg4 is dwType, arg6 is cbData
  switch (arg4)
  {
  case REG_SZ:
  case REG_EXPAND_SZ:
    if(! SvPOK($input))
      croak("expected a string");
    $1=(LPBYTE)wstr_from_ascii(SvPV($input, arg6));
    arg6=sizeof(WCHAR)*arg6;
    break;
  case REG_DWORD:
  case REG_DWORD_BIG_ENDIAN:
    {
      if(! SvIOK($input))
	croak("expected an integer");
      DWORD val=SvIV($input);
      $1=(LPBYTE)&val;
      arg6=sizeof(DWORD);
      break;
    }
  case REG_MULTI_SZ:
    {
      if(! SvROK($input))
	croak("expected an ARRAY reference");
      if(SVt_PVAV != SvTYPE(SvRV($input)))
	croak("expected an ARRAY reference");
      AV *av=(AV *)SvRV($input);
      
      int bytes=0;
      int i, j, len;
      SV **psv;
      // calculate the buffer size
      for(i=0; i<av_len(av)+1; i++)
      {
	psv=av_fetch(av, i, 0);
	SvPV(*psv, len);
	bytes+=sizeof(WCHAR)*(len+1);
      }
      // copy the strings into the new buffer
      arg6=bytes;
      $1=(LPBYTE)malloc(bytes);
      j=0;
      for(i=0; i<av_len(av)+1; i++)
      {
	psv=av_fetch(av, i, 0);
	char *str=SvPV(*psv, len);
	memcpy(&$1[j], wstr_from_ascii(str), sizeof(WCHAR)*(len+1));
	j+=len+1;
      }
      break;
    }
  case REG_BINARY:
  case REG_NONE:
  case REG_LINK:
    if(! SvPOK($input))
      croak("expected a string");
    $1=SvPV($input, arg6);
    break;  
  default:
    croak("unknown type: 0x%x", arg4);
  }
}

// cbData is already set (by the last typemap)
%typemap(in, numinputs=0) DWORD cbData {}

%typemap(in, numinputs=0) DWORD Reserved {}

LONG CeRegSetValueEx(HKEY hKey, LPCWSTR lpValueName, DWORD Reserved=0, 
		     DWORD dwType, const BYTE *lpData, DWORD cbData);


/* EOF */

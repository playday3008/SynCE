/* 
   Python wrapper for the synce project (http://synce.sourceforge.net/synce/).
   
   Richard Taylor
   Started: November 2002

   Lots still to do:
        Complete the database write functions.
	Sort out proper exception handling.
	Add the registry functions.

*/


%module pyrapi
%{
#include "rapi.h"

static PyObject *PyRapiError;

%}


/* 
   Call the rapi init function as part of the module load. This might not be 
   the correct thing to do but it will work for now.
*/

%init %{
  PyRapiError = PyErr_NewException("pyrapi.error", NULL, NULL);
  PyDict_SetItemString(d, "error", PyRapiError);

  CeRapiInit();
%}

%include "typemaps.i"

/*
  It would be nice if these constant could be pulled from a header file. 
  There is a bug in my version of SWIG which stops it from recognising
  hex values in #defines, so I have had to replicate the constants here.
*/

/* dwShareMode */
%constant GENERIC_WRITE   =  0x40000000;
%constant FILE_SHARE_READ =  0x00000001;
%constant GENERIC_READ    =  0x80000000;

/* dwCreationDisposition */
%constant CREATE_NEW        =  1;
%constant CREATE_ALWAYS     =  2;
%constant OPEN_EXISTING     =  3;
%constant OPEN_ALWAYS       =  4;
%constant TRUNCATE_EXISTING =  5;
%constant OPEN_FOR_LOADER   =  6;

/* dwFlagsAndAttributes */
%constant FILE_ATTRIBUTE_READONLY = 0x00000001;
%constant FILE_ATTRIBUTE_HIDDEN   = 0x00000002;
%constant FILE_ATTRIBUTE_SYSTEM   = 0x00000004;
%constant FILE_ATTRIBUTE_1        = 0x00000008;

%constant FILE_ATTRIBUTE_DIRECTORY = 0x00000010;
%constant FILE_ATTRIBUTE_ARCHIVE   = 0x00000020;
%constant FILE_ATTRIBUTE_INROM     = 0x00000040;
%constant FILE_ATTRIBUTE_NORMAL    = 0x00000080;

%constant FILE_ATTRIBUTE_TEMPORARY  = 0x00000100;
%constant FILE_ATTRIBUTE_2          = 0x00000200;
%constant FILE_ATTRIBUTE_3          = 0x00000400;
%constant FILE_ATTRIBUTE_COMPRESSED = 0x00000800;

%constant FILE_ATTRIBUTE_ROMSTATICREF = 0x00001000;
%constant FILE_ATTRIBUTE_ROMMODULE    = 0x00002000;
%constant FILE_ATTRIBUTE_4            = 0x00004000;
%constant FILE_ATTRIBUTE_5            = 0x00008000;

%constant FILE_ATTRIBUTE_HAS_CHILDREN = 0x00010000;
%constant FILE_ATTRIBUTE_SHORTCUT     = 0x00020000;
%constant FILE_ATTRIBUTE_6            = 0x00040000;
%constant FILE_ATTRIBUTE_7            = 0x00080000;


/* 
   SWIG does not understand u_int style type declarations
   so we have to tell it what the rapi types are.
*/

typedef unsigned int WORD;
typedef unsigned int DWORD;
typedef unsigned int BOOL;
typedef unsigned int * LPDWORD;

typedef char * LPCSTR;
typedef char * WCHAR;

/* 
   FILETIME must be mapped to and from unixtime. The typemap is used for 
   functions that take FILETIME as a parameter and the GETSET macro is 
   used to add vertual members to classes that have FILETIME members.
*/

typedef struct _FILETIME
{
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

%typemap(python,in) FILETIME {
  if (PyInt_Check($input)) {
    $1 = filetime_from_unixtime((time_t)PyInt_AsLong($input));
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a integer (time since epoch).");
    return NULL;
  }
}

%define FILETIMEGETSET(type,name)
     %{
  const unsigned int type ## _ ## name ## _get(type *ptr) {
    return (const unsigned int) filetime_to_unix_time(&ptr-> ## name);
  }
  void type ## _ ## name ##_set(type *ptr, const unsigned int value) {
    // TODO: check length of string.
    filetime_from_unix_time((time_t)value, &ptr-> ## name );
  }

  %}
%enddef

/*
  SWIG does not understand unicode so these macros and typemaps provide 
  conversion to and from ascii for structures and functions that use wide 
  character arrays.
*/

%typemap(python,in) LPCWSTR, LPWSTR {
  if (PyString_Check($input)) {
    $1 = wstr_from_ascii(PyString_AsString($input));
    printf("String is : \"%s\"\n", wstr_to_ascii($1));

  } else {
    PyErr_SetString(PyExc_TypeError, "expected a string.");
    return NULL;
  }
}

%define WCHARGETSET(type,name)
     %{
  const char * type ## _ ## name ## _get(type *ptr) {
    return (const char *) wstr_to_ascii(ptr-> ## name);
  }

  void type ## _ ## name ## _set(type *ptr, const char *value) {
    // TODO: check length of string.
    wstr_strcpy(ptr-> ## name ,wstr_from_ascii(value));
  }
  %}
%enddef


/*
  I don't think that lpSecurityAttributes are actually used at all. But I might be wrong.
*/

%typemap(ignore) LPSECURITY_ATTRIBUTES lpSecurityAttributes ( LPSECURITY_ATTRIBUTES temp) {
  $1 = NULL;
}

/*
  Checking the return value of functions that return handles.
  This should not be returning TypeError.
*/

%typemap(python,out) HANDLE {

  if (result == INVALID_HANDLE_VALUE) {
      PyErr_SetString(PyExc_TypeError, "bad file handle.");
      return NULL;    
  }
  {
    HANDLE * resultptr;
    resultptr = (HANDLE *) malloc(sizeof(HANDLE ));
    memmove(resultptr, &result, sizeof(HANDLE ));
    $result = SWIG_NewPointerObj((void *) resultptr, SWIGTYPE_p_HANDLE, 1);
  }

}



/* 
   File handling functions.
*/

typedef struct _CE_FIND_DATA {
  DWORD dwFileAttributes; 
  //FILETIME ftCreationTime; 
  //FILETIME ftLastAccessTime; 
  //FILETIME ftLastWriteTime; 
  DWORD nFileSizeHigh; 
  DWORD nFileSizeLow; 
  DWORD dwOID; 
  //WCHAR cFileName[MAX_PATH]; 
  %addmethods {
    const unsigned int ftCreationTime;
    const unsigned int ftLastAccessTime;
    const unsigned int ftLastWriteTime;
    const char * cFileName;

  }
} CE_FIND_DATA, *LPCE_FIND_DATA, **LPLPCE_FIND_DATA; 

FILETIMEGETSET(CE_FIND_DATA,ftCreationTime)
FILETIMEGETSET(CE_FIND_DATA,ftLastAccessTime)
FILETIMEGETSET(CE_FIND_DATA,ftLastWriteTime)
WCHARGETSET(CE_FIND_DATA,cFileName)


%typemap(python,argout) LPLPCE_FIND_DATA {
  PyObject *return_tuple, *o2, *o3;
  int i;
  LPCE_FIND_DATA entry;
  return_tuple = PyTuple_New(*arg3);
  //printf("Number of files: %u\n", *arg3);

  entry = *$1;
  for (i = 0; i < *arg3; i++) {
    if (PyTuple_SetItem(return_tuple, i,  
			SWIG_NewPointerObj((void *)entry++, SWIGTYPE_p_CE_FIND_DATA, 0))) {
      PyErr_SetString(PyExc_TypeError, "failed to build return tuple.");
      return NULL;
    }

  }

  if ((!result) || ($result == Py_None)) {
    $result = return_tuple;
  } else {
    if (!PyTuple_Check($result)) {
      PyObject *o2 = $result;
      $result = PyTuple_New(1);
      PyTuple_SetItem($result,0,o2);
    }
    o3 = PyTuple_New(1);
    PyTuple_SetItem(o3,0,return_tuple);
    o2 = $result;
    $result = PySequence_Concat(o2,o3);
    Py_DECREF(o2);
    Py_DECREF(o3);
  }	
}

%typemap(ignore) LPLPCE_FIND_DATA ppFindDataArray (LPCE_FIND_DATA temp) {
  $1 = &temp;
}

%apply unsigned int *OUTPUT { LPDWORD lpdwFoundCount };

%typemap(python,out) BOOL {

  if (result != 1) {
      PyErr_SetString(PyExc_RuntimeError, "bad return value.");
      return NULL;    
  }
  {
    $result = Py_None; // Ignore a correct return
  }

}



BOOL CeCloseHandle( 
		HANDLE hObject);

HANDLE CeCreateFile(
		LPCWSTR lpFileName, 
		DWORD dwDesiredAccess = GENERIC_READ, 
		DWORD dwShareMode = 0, 
		LPSECURITY_ATTRIBUTES lpSecurityAttributes, 
		DWORD dwCreationDisposition = OPEN_EXISTING, 
		DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL, 
		HANDLE hTemplateFile = 0); 


%typemap(python,in) (LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead){
  if (PyInt_Check($input)) {
    $2 = PyInt_AsLong($input);
    if (!($1 = ($1_ltype) malloc($2))){
      PyErr_SetString(PyExc_TypeError, "out of memory.");
      return NULL;
    }
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a int.");
    return NULL;
  }
}

%typemap(python,argout) (LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead){
  $result = Py_BuildValue("(s#,i)",(char *)$1,*$3,*$3);
  free($1);
}

BOOL CeReadFile( 
		HANDLE hFile, 
		LPVOID lpBuffer, 
		DWORD nNumberOfBytesToRead, 
		LPDWORD lpNumberOfBytesRead, 
		LPOVERLAPPED lpOverlapped = NULL);

%typemap(python,in) (LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten){
  if (PyString_Check($input)) {
    $1 = PyString_AsString($input);
    $2 = strlen($1);
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a int.");
    return NULL;
  }
}

%typemap(python,argout) (LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten){
  $result = PyInt_FromLong(*$3);
}

BOOL CeWriteFile( 
		HANDLE hFile, 
		LPCVOID lpBuffer, 
		DWORD nNumberOfBytesToWrite, 
		LPDWORD lpNumberOfBytesWritten, 
		LPOVERLAPPED lpOverlapped = NULL); 


BOOL CeCopyFileA(
		LPCSTR lpExistingFileName, 
		LPCSTR lpNewFileName, 
		BOOL bFailIfExists);

BOOL CeFindAllFiles(
		LPCWSTR szPath, 
		DWORD dwFlags = FAF_NAME|FAF_ATTRIBUTES|FAF_CREATION_TIME|FAF_LASTACCESS_TIME|FAF_LASTWRITE_TIME|FAF_SIZE_LOW, 
		LPDWORD lpdwFoundCount, 
		LPLPCE_FIND_DATA ppFindDataArray);

DWORD CeGetLastError( void );


BOOL CeCopyFile(
		LPCWSTR lpExistingFileName, 
		LPCWSTR lpNewFileName, 
		BOOL bFailIfExists);

BOOL CeCreateDirectory(
		LPCWSTR lpPathName, 
		LPSECURITY_ATTRIBUTES lpSecurityAttributes);

BOOL CeDeleteFile(
		LPCWSTR lpFileName);

HANDLE CeFindFirstFile(
		LPCWSTR lpFileName, 
		LPCE_FIND_DATA lpFindFileData);

BOOL CeFindNextFile( 
		HANDLE hFindFile, 
		LPCE_FIND_DATA lpFindFileData); 

BOOL CeFindClose(
		HANDLE hFindFile);

DWORD CeGetFileAttributes(
		LPCWSTR lpFileName);

%constant CSIDL_PROGRAMS  =         0x0002;
%constant CSIDL_PERSONAL  =         0x0005;
%constant CSIDL_FAVORITES_GRYPHON = 0x0006;
%constant CSIDL_STARTUP           = 0x0007;
%constant CSIDL_RECENT            = 0x0008;
%constant CSIDL_STARTMENU         = 0x000b;
%constant CSIDL_DESKTOPDIRECTORY  = 0x0010;
%constant CSIDL_FONTS             = 0x0014;
%constant CSIDL_FAVORITES         = 0x0016;

DWORD CeGetSpecialFolderPath( 
		int nFolder, 
		DWORD nBufferLength, 
		LPWSTR lpBuffer);

BOOL CeMoveFile(
		LPCWSTR lpExistingFileName, 
		LPCWSTR lpNewFileName);

BOOL CeRemoveDirectory(
		LPCWSTR lpPathName);

BOOL CeSetFileAttributes(
		LPCWSTR lpFileName,
		DWORD dwFileAttributes);




/*
  Database functions.
*/

typedef DWORD CEPROPID;
typedef CEPROPID *PCEPROPID;
typedef DWORD CEOID;
typedef CEOID *PCEOID;

typedef struct _CEBLOB {
	DWORD dwCount;
	LPBYTE lpb;
} CEBLOB;

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
  //FILETIME filetime;
  //LPWSTR lpwstr; 
        %addmethods {
	  const unsigned int filetime;
	  const char * lpwstr; 
	}

	CEBLOB blob; 
	BOOL boolVal;
	double dblVal;
} CEVALUNION; 

FILETIMEGETSET(CEVALUNION,filetime)
WCHARGETSET(CEVALUNION,lpwstr)

typedef struct _CEPROPVAL { 
	CEPROPID propid;
	WORD wLenData;
	WORD wFlags;
	CEVALUNION val;
  %addmethods {
	  const unsigned int type;
  }
} CEPROPVAL;

%{
  const unsigned int CEPROPVAL_type_get(CEPROPVAL *ptr) {
    return ptr->propid & 0xFFFF;
  }
  void CEPROPVAL_type_set(CEPROPVAL *ptr, const unsigned int value) {
    // Should raise an exception.
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
        %addmethods {
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

CEOID CeCreateDatabase(
		LPWSTR lpszName, 
		DWORD dwDbaseType = 0, 
		WORD wNumSortOrder, 
		SORTORDERSPEC *rgSortSpecs);

BOOL CeDeleteDatabase(
		CEOID oid);

%typemap(python,argout) (LPWORD cFindData, LPLPCEDB_FIND_DATA ppFindData) {
  PyObject *return_tuple, *o2, *o3;
  int i;
  LPCEDB_FIND_DATA entry;
  return_tuple = PyTuple_New(*arg3);
  //printf("Number of databases: %u\n", *arg3);

  entry = *$2;
  for (i = 0; i < *arg3; i++) {
    if (PyTuple_SetItem(return_tuple, i,  
			SWIG_NewPointerObj((void *)entry++, SWIGTYPE_p_CEDB_FIND_DATA, 0))) {
      PyErr_SetString(PyExc_TypeError, "failed to build return tuple.");
      return NULL;
    }

  }

  if ((!result) || ($result == Py_None)) {
    $result = return_tuple;
  } else {
    if (!PyTuple_Check($result)) {
      PyObject *o2 = $result;
      $result = PyTuple_New(1);
      PyTuple_SetItem($result,0,o2);
    }
    o3 = PyTuple_New(1);
    PyTuple_SetItem(o3,0,return_tuple);
    o2 = $result;
    $result = PySequence_Concat(o2,o3);
    Py_DECREF(o2);
    Py_DECREF(o3);
  }	
}

%typemap(ignore) LPWORD cFindData (WORD temp) {
  $1 = &temp;
}

%typemap(ignore) LPLPCEDB_FIND_DATA ppFindData (LPCEDB_FIND_DATA temp) {
  $1 = &temp;
}

BOOL CeFindAllDatabases(
		DWORD dwDbaseType = 0, 
		WORD wFlags = 0xFFFF, 
		LPWORD cFindData, 
		LPLPCEDB_FIND_DATA ppFindData);

HANDLE CeFindFirstDatabase(
		DWORD dwDbaseType = 0);

CEOID CeFindNextDatabase(
		HANDLE hEnum);


%typemap(python,in) PCEOID {
  CEOID temp;
  if (PyInt_Check($input)) {
    temp = (CEOID) PyInt_AsLong($input);
    $1 = &temp;
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a integer (database oid).");
    return NULL;
  }

}

%typemap(ignore) HWND hwndNotify (HWND temp) {
  $1 = temp;
}

HANDLE CeOpenDatabase(
		PCEOID poid, 
		LPWSTR lpszName, 
		CEPROPID propid = 0, 
		DWORD dwFlags = CEDB_AUTOINCREMENT, 
		HWND hwndNotify); 

%typemap(ignore) (LPWORD lpcPropID, 
		  CEPROPID *rgPropID, 
		  LPBYTE *lplpBuffer, 
		  LPDWORD lpcbBuffer) (WORD temp1, 
				       CEPROPID temp2, 
				       BYTE *temp3, 
				       DWORD temp4){
  $1 = &temp1;
  $2 = NULL;
  $3 = &temp3;
  $4 = &temp4;

}

%typemap(python,argout) (LPBYTE *lplpBuffer, LPDWORD lpcbBuffer) {
  PyObject *return_tuple, *o2, *o3;
  int i;
  PCEPROPVAL entry;
  return_tuple = PyTuple_New(*arg3);
  //printf("Number of databases: %u\n", *arg3);

  entry = ( PCEPROPVAL )*$1;
  for (i = 0; i < *arg3; i++) {
    if (PyTuple_SetItem(return_tuple, i,  
			SWIG_NewPointerObj((void *)entry++, SWIGTYPE_p_CEPROPVAL, 0))) {
      PyErr_SetString(PyExc_TypeError, "failed to build return tuple.");
      return NULL;
    }

  }

  if ((!result) || ($result == Py_None)) {
    $result = return_tuple;
  } else {
    if (!PyTuple_Check($result)) {
      PyObject *o2 = $result;
      $result = PyTuple_New(1);
      PyTuple_SetItem($result,0,o2);
    }
    o3 = PyTuple_New(1);
    PyTuple_SetItem(o3,0,return_tuple);
    o2 = $result;
    $result = PySequence_Concat(o2,o3);
    Py_DECREF(o2);
    Py_DECREF(o3);
  }	
}

CEOID CeReadRecordProps(
		HANDLE hDbase, 
		DWORD dwFlags = CEDB_ALLOWREALLOC, 
		LPWORD lpcPropID, 
		CEPROPID *rgPropID, 
		LPBYTE *lplpBuffer, 
		LPDWORD lpcbBuffer); 

CEOID CeSeekDatabase(
		HANDLE hDatabase, 
		DWORD dwSeekType, 
		DWORD dwValue, 
		LPDWORD lpdwIndex);

CEOID CeWriteRecordProps(
		HANDLE hDbase, 
		CEOID oidRecord, 
		WORD cPropID, 
		CEPROPVAL *rgPropVal );


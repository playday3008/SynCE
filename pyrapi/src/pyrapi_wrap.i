/* 
   Python wrapper for the synce project (http://synce.sourceforge.net/synce/).
   
   Richard Taylor
   Started: November 2002

   Lots still to do:
        Complete the database write functions.
	Sort out proper exception handling.
	Add the registry functions.

   Version: $Header$ 
*/


%module pyrapi
%{
#include "rapi.h"

static PyObject *PyRapiError;

%}

/*
 * Include all of the constant definitions.
 * Function and type definitions are masked by the SWIG 
 * preprocessor directive.
 */

%include "rapi.h" 

/* 
   Declare new exception types for pyrapi errors.

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
   SWIG does not understand u_int style type declarations
   so we have to tell it what the rapi types are.
*/

typedef unsigned int WORD;
typedef unsigned long DWORD;
typedef unsigned int BOOL;
typedef unsigned long * LPDWORD;

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

%typemap(in) LPCWSTR, LPWSTR {
  if (PyString_Check($input)) {
    $1 = wstr_from_utf8(PyString_AsString($input));
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a string.");
    return NULL;
  }
}

%define WCHARGETSET(type,name)
     %{
  const char * type ## _ ## name ## _get(type *ptr) {
    return (const char *) wstr_to_utf8(ptr-> ## name);
  }

  void type ## _ ## name ## _set(type *ptr, const char *value) {
    // TODO: check length of string.
    wstr_strcpy(ptr-> ## name ,wstr_from_utf8(value));
  }
  %}
%enddef


/*
  I don't think that lpSecurityAttributes are actually used at all. But I might be wrong.
*/

%typemap(in,numinputs=0) LPSECURITY_ATTRIBUTES lpSecurityAttributes ( LPSECURITY_ATTRIBUTES temp) {
  $1 = NULL;
}

/*
  Checking the return value of functions that return handles.
  This should not be returning TypeError.
*/

%typemap(out) HANDLE {

  if ($1 == INVALID_HANDLE_VALUE) {
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
  %extend {
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


%typemap(python,argout) (LPDWORD lpdwFoundCount, LPLPCE_FIND_DATA ppFindDataArray) {
  PyObject *return_tuple, *o2, *o3;
  int i;
  LPCE_FIND_DATA entry;
  return_tuple = PyTuple_New(*$1);
  //printf("Number of files: %u\n", *arg3);

  entry = *$2;
  for (i = 0; i < *$1; i++) {
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


%typemap(in,numinputs=0)  (LPDWORD lpdwFoundCount, LPLPCE_FIND_DATA ppFindDataArray) 
     (DWORD temp1, LPCE_FIND_DATA temp2) {
  $1 = &temp1;
  $2 = &temp2;
}

//%apply unsigned int *OUTPUT { LPDWORD lpdwFoundCount };

%typemap(python,out) BOOL {

  if ($1 < 1) {
    {
      char err[256];
      snprintf(err, sizeof(err), "Rapi function failed, last error code was: %d", CeGetLastError());
      PyErr_SetString(PyExc_RuntimeError, err);
      return NULL; 
    }
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


%typemap(python,in) (LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead)(DWORD tmp){
  $3 = &tmp;
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
  if ( *$3 == 0 ){
    $result = Py_None;
  }
  else {
    $result = Py_BuildValue("s#",(char *)$1,*$3);
  }
  free($1);
}

BOOL CeReadFile( 
		HANDLE hFile, 
		LPVOID lpBuffer, 
		DWORD nNumberOfBytesToRead, 
		LPDWORD lpNumberOfBytesRead, 
		LPOVERLAPPED lpOverlapped = NULL);


%typemap(python,in) (LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten) (DWORD tmp, PyObject *buf){
  $3 = &tmp;
  $2 = PySequence_Size($input);
  buf = PyBuffer_FromObject($input,0,Py_END_OF_BUFFER); // convert to buffer

  if (PyBuffer_Check(buf)) { // probably not needed.
    if (PyObject_AsReadBuffer(buf,&$1,&$2) == -1){
      // Failed read buffer creation.
      PyErr_SetString(PyExc_TypeError, "unable to convert buffer.");
      return NULL;
    }
  } else {
    return NULL;
  }
  //printf("CeWriteFile writing buffer (length %d): %s\n", $2, $1);
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

//FAF_NAME|FAF_ATTRIBUTES|FAF_CREATION_TIME|FAF_LASTACCESS_TIME|FAF_LASTWRITE_TIME|FAF_SIZE_LOW, 
BOOL CeFindAllFiles(
		LPCWSTR szPath, 
		DWORD dwFlags,
		LPDWORD lpdwFoundCount, 
		LPLPCE_FIND_DATA ppFindDataArray);

DWORD CeGetLastError( void );


BOOL CeCopyFile(
		LPCWSTR lpExistingFileName, 
		LPCWSTR lpNewFileName, 
		BOOL bFailIfExists = 0);

BOOL CeCreateDirectory(
		LPCWSTR lpPathName, 
		LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL);

BOOL CeDeleteFile(
		LPCWSTR lpFileName);

%typemap(python,argout) (LPCWSTR lpFileName, LPCE_FIND_DATA lpFindFileData) {
  PyObject *find_data, *o2, *o3;
  
  find_data = SWIG_NewPointerObj($2, SWIGTYPE_LPCE_FIND_DATA, 1);

  if ((!result) || ($result == Py_None)) {
    $result = find_data;
  } else {
    if (!PyTuple_Check($result)) {
      PyObject *o2 = $result;
      $result = PyTuple_New(1);
      PyTuple_SetItem($result,0,o2);
    }
    o3 = PyTuple_New(1);
    PyTuple_SetItem(o3,0,find_data);
    o2 = $result;
    $result = PySequence_Concat(o2,o3);
    Py_DECREF(o2);
    Py_DECREF(o3);
  }	
}

%typemap(in,numinputs=0) (LPCE_FIND_DATA lpFindFileData) {
  $1 = (LPCE_FIND_DATA) malloc(sizeof(CE_FIND_DATA));
}


HANDLE CeFindFirstFile(
		LPCWSTR lpFileName, 
		LPCE_FIND_DATA lpFindFileData);

%typemap(python,argout) (HANDLE hFindFile, LPCE_FIND_DATA lpFindFileData) {
  PyObject *find_data;
  
  $result = SWIG_NewPointerObj($2, SWIGTYPE_LPCE_FIND_DATA, 1);
}

BOOL CeFindNextFile( 
		HANDLE hFindFile, 
		LPCE_FIND_DATA lpFindFileData); 

BOOL CeFindClose(
		HANDLE hFindFile);

DWORD CeGetFileAttributes(
		LPCWSTR lpFileName);

%typemap(in,numinputs=0) (DWORD nBufferLength,
                          LPWSTR lpBuffer) (WCHAR tmp[MAX_PATH]){
   $1 = MAX_PATH;
   $2 = tmp;
}

%typemap(python,argout) (DWORD nBufferLength,
			 LPWSTR lpBuffer) {
  char * ascii_str;
  
  if (!result) {
    PyErr_SetString(PyRapiError, "Error getting special folder name.");
    return NULL;
  }

  ascii_str = wstr_to_utf8($2);
  $result = Py_BuildValue("s",ascii_str);
}

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


typedef union _CEVALUNION {
	short iVal; 
	USHORT uiVal; 
	long lVal; 
	ULONG ulVal; 
  //FILETIME filetime;
  //LPWSTR lpwstr; 
        %extend {
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
  //CEPROPID propid;
	WORD wLenData;
	WORD wFlags;
	CEVALUNION val;
  %extend {
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


%typemap(in) (WORD wNumSortOrder, 
	      SORTORDERSPEC *rgSortSpecs) {
  int i;
  SORTORDERSPEC * tmp;

  if (PyTuple_Check($input)) {
    /* get size of tuple */
    $1 =  PyTuple_Size($input);

    if ( $1 == 0 ) {
      $2 = NULL;
    } else {
      /* create array of SORTORDERSPEC elements */
      $2 = (SORTORDERSPEC *) malloc($1*sizeof(SORTORDERSPEC));
      
      /* unpack argument tuple into array */
      for ( i = 0; i < $1; i++) {
	if ((SWIG_ConvertPtr(PyTuple_GetItem($input,i),(void **) &tmp, 
			     SWIGTYPE_p_SORTORDERSPEC,SWIG_POINTER_EXCEPTION | 0 )) == -1) 
	  SWIG_fail;
	memcpy(&($2[i]),tmp, sizeof(SORTORDERSPEC));
      }
    }
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a tuple.");
    return NULL;
  }
}

CEOID CeCreateDatabase(
		LPWSTR lpszName, 
		DWORD dwDbaseType, 
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

%typemap(in,numinputs=0) LPWORD cFindData (WORD temp) {
  $1 = &temp;
}

%typemap(in,numinputs=0) LPLPCEDB_FIND_DATA ppFindData (LPCEDB_FIND_DATA temp) {
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

%typemap(in,numinputs=0) HWND hwndNotify (HWND temp) {
  $1 = temp;
}

HANDLE CeOpenDatabase(
		PCEOID poid, 
		LPWSTR lpszName, 
		CEPROPID propid = 0, 
		DWORD dwFlags = CEDB_AUTOINCREMENT, 
		HWND hwndNotify); 


/* 
 * typemaps to support CeReadRecordProps
 */

%typemap(in,numinputs=0) 
     (LPWORD lpcPropID, 
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

  if ( *arg3 > 0 ) {
    return_tuple = PyTuple_New(*arg3);

    entry = ( PCEPROPVAL )*$1;
    for (i = 0; i < *arg3; i++) {
      if (PyTuple_SetItem(return_tuple, i,  
			  SWIG_NewPointerObj((void *)entry++, SWIGTYPE_p_CEPROPVAL, 0))) {
	PyErr_SetString(PyExc_TypeError, "failed to build return tuple.");
	return NULL;
      }

    }
  } else {
    return_tuple = Py_None;
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

%typemap(in) (	DWORD dwSeekType, 
		DWORD dwValue ) {

  PyObject *seek_type_obj, *seek_value_obj;

  if (!PyTuple_Check($input)) {
    PyErr_SetString(PyRapiError, "second parameter to CeSeekDatabase must be a two element tuple.");
    return NULL;
  }

  /* check size of tuple */
  {
    int num_args;
    num_args =  PyTuple_Size($input);
    if (num_args != 2) {
      PyErr_SetString(PyRapiError, "second parameter to CeSeekDatabase must be a two element tuple.");
      return NULL;
    }
  }
	
  /* Get the seek_type. */
  seek_type_obj = PyTuple_GetItem($input,0);
  if (!PyInt_Check(seek_type_obj)) {
    PyErr_SetString(PyRapiError, "seek_type not a valid seek_type, must evaluate to an int.");
    return NULL;
  }    
  $1 = (DWORD)PyInt_AsLong(seek_type_obj);

  /* Get the seek_value */

  seek_value_obj = PyTuple_GetItem($input,1);

  switch ($1) {
  case CEDB_SEEK_CEOID:
    {
      if (!PyInt_Check(seek_value_obj)) {
	PyErr_SetString(PyRapiError, "seek_value must be a record_oid for seek_type == CEDB_SEEK_CEOID.");
	return NULL;
      }    
      $2 = (DWORD)PyInt_AsLong(seek_value_obj);
    }
    break;
  case CEDB_SEEK_VALUESMALLER:
  case CEDB_SEEK_VALUEFIRSTEQUAL:
  case CEDB_SEEK_VALUENEXTEQUAL:
  case CEDB_SEEK_VALUEGREATER:
    {
      CEPROPVAL * prop_val;

      if ((SWIG_ConvertPtr(seek_value_obj,(void **) &prop_val, 
			   SWIGTYPE_p_CEPROPVAL,SWIG_POINTER_EXCEPTION | 0 )) == -1) {
	PyErr_SetString(PyRapiError, "seek_value must be a CEPROPVAL for seek_type == CEDB_SEEK_VALUEXXX.");
	return NULL;
      }
      $2 = (DWORD)prop_val;
    }
    break;
  case CEDB_SEEK_BEGINNING:      
  case CEDB_SEEK_CURRENT:         
  case CEDB_SEEK_END:
    {
      if (!PyInt_Check(seek_value_obj)) {
	PyErr_SetString(PyRapiError, "seek_value must be a index for offset seek_type.");
	return NULL;
      }    
      $2 = (DWORD)PyInt_AsLong(seek_value_obj);
    }
    break;
  default:
    PyErr_SetString(PyRapiError, "seek_type not a valid seek_type, must be one of CEDB_XXX constants.");
    return NULL;
  }
}

%typemap(in,numinputs=0) 
     (LPDWORD lpdwIndex)
   (DWORD temp){
  $1 = &temp;
}

%typemap(python,argout) (LPDWORD lpdwIndex) {
  PyObject *return_obj, *o2, *o3;

  return_obj   = PyInt_FromLong(*$1);

  if (!PyTuple_Check($result)) {
    PyObject *o2 = $result;
    $result = PyTuple_New(1);
    PyTuple_SetItem($result,0,o2);
  }
  o3 = PyTuple_New(1);
  PyTuple_SetItem(o3,0,return_obj);
  o2 = $result;
  $result = PySequence_Concat(o2,o3);
  Py_DECREF(o2);
  Py_DECREF(o3);
}

CEOID CeSeekDatabase(
		HANDLE hDatabase, 
		DWORD dwSeekType, 
		DWORD dwValue, 
		LPDWORD lpdwIndex);

/* 
 * typemaps to support CeWriteRecordProps
 *
 * CeWriteRecordProps (dbh, rec_oid, values)
 *     Returns the rec_oid for success 0 for failure. 
 *     values is a tuple of of CEPROPVAL classes.
 */

%typemap(in) (WORD cPropID, 
	      CEPROPVAL *rgPropVal) {

  int i;
  CEPROPVAL * tmp;

  if (PyTuple_Check($input)) {
    /* get size of tuple */
    $1 =  PyTuple_Size($input);

    //printf("Got %d fields to write.", $1);

    /* create array of CEPROPVAL elements */
    $2 = (CEPROPVAL *) malloc($1*sizeof(CEPROPVAL));

    /* unpack argument tuple into array */
    for ( i = 0; i < $1; i++) {
      if ((SWIG_ConvertPtr(PyTuple_GetItem($input,i),(void **) &tmp, 
			   SWIGTYPE_p_CEPROPVAL,SWIG_POINTER_EXCEPTION | 0 )) == -1) 
	SWIG_fail;
      memcpy(&($2[i]),tmp, sizeof(CEPROPVAL));
      //printf ("propid & 0xFFFF = %d\n", ($2[i].propid & 0xFFFF));
      //printf ("propid >> 16 = %d\n", ($2[i].propid >> 16));
    }
    //for ( i = 0; i < $1; i++) {
    //  printf ("real propid & 0xFFFF = %d\n", (($2[i].propid) & 0xFFFF));
    //  printf ("real propid >> 16 = %d\n", (($2[i].propid) >> 16));
    //  }
  } else {
    PyErr_SetString(PyExc_TypeError, "expected a tuple.");
    return NULL;
  }
}


CEOID CeWriteRecordProps(
		HANDLE hDbase, 
		CEOID oidRecord, 
		WORD cPropID, 
		CEPROPVAL *rgPropVal );


/*
 * functions to control logging.
 */

#define SYNCE_LOG_LEVEL_LOWEST    0

#define SYNCE_LOG_LEVEL_ERROR     1
#define SYNCE_LOG_LEVEL_WARNING   2
#define SYNCE_LOG_LEVEL_TRACE     3

#define SYNCE_LOG_LEVEL_HIGHEST   4

void synce_log_set_level(int level);


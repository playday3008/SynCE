/*****************************************************************************
 * pyrapi.c: a simple wrapper around the librapi2 library
 *
 *
 * Richard Taylor <r.taylor@bcs.org.uk>
 ****************************************************************************/

#include "Python.h"          /* python headers */
#include <stdio.h>
#include <string.h>

#include "rapi.h"            /* librapi2 headers */
#include "rapi_wstr.h"
#include "rapi_filetime.h" 



//
// Return values from main()
//
#define TEST_SUCCEEDED 0
#define TEST_FAILED 1

//
// HRESULT tests
//
#define VERIFY_HRESULT(call) \
if (FAILED((call))) { printf("FAIL.\n"); }

#define TEST_HRESULT(call) \
printf("Testing %s...", #call); \
VERIFY_HRESULT(call) else printf("ok.\n");

//
// Test return value
//
#define VERIFY_EQUAL(value, call) \
if ((value) != (call)) { printf("FAIL.\n"); }

#define TEST_EQUAL(value, call) \
printf("Testing %s...", #call); \
VERIFY_EQUAL((value), (call)) else printf("ok.\n");


//
// Test return value
//
#define VERIFY_NOT_EQUAL(value, call) \
if ((value) == (call)) { printf("FAIL.\n"); }

#define TEST_NOT_EQUAL(value, call) \
printf("Testing %s...", #call); \
VERIFY_NOT_EQUAL((value), (call)) else printf("ok.\n");


//
// Test to verify that the return value is zero
//
#define VERIFY_NOT_FALSE(call) VERIFY_NOT_EQUAL(0, (call))
#define TEST_NOT_FALSE(call) TEST_NOT_EQUAL(0, call)

static PyObject *ErrorObject;

#define onError(message) \
{ PyErr_SetString(ErrorObject, message); return NULL; }



/************************************************************************
 * file_access.c functions
 ***********************************************************************/

static char pyrapi_CeRapiInit__doc__[] = 
"Return a handle to an open file on the PocketPC";

static PyObject *
pyrapi_CeRapiInit(PyObject *self, PyObject *args)
{
  VERIFY_HRESULT(CeRapiInit());
  return Py_None;
}


static char pyrapi_CeCreateFile__doc__[] = 
"Return a handle to an open file on the PocketPC:
params: (PathToFile, FileMode)
result: CeFileHandle

FileMode must be either 'r' for read or 'w' for write.
";

static PyObject *
pyrapi_CeCreateFile(PyObject *self, PyObject *args)
{
  char * asci_path_name;
  char * mode;
  LPWSTR lpPathName;
  DWORD dwDesiredAccess;
  DWORD dwShareMode;
  LPSECURITY_ATTRIBUTES lpSecurityAttributes;
  DWORD dwCreationDisposition;
  DWORD dwFlagsAndAttributes;
  HANDLE hTemplateFile;

  HANDLE handle;

  if (!PyArg_ParseTuple(args, "ss:CeCreateFile", &asci_path_name, &mode))
    return NULL;

  dwDesiredAccess = GENERIC_READ;
  dwCreationDisposition = OPEN_EXISTING;

  switch (mode[0])
    {
    case 'r': // Read mode
      dwDesiredAccess = GENERIC_READ;
      dwCreationDisposition = OPEN_EXISTING;
      break;

    case 'w': // write mode
      dwDesiredAccess = GENERIC_WRITE;
      dwCreationDisposition = CREATE_ALWAYS;
      break;

    default: // unknown mode
      // Raise exception
      onError("Unknown filemode")
      break;
    }

  lpPathName = rapi_wstr_from_ascii(asci_path_name);
  printf("Remote filename: \"%s\"\n", rapi_wstr_to_ascii(lpPathName));

  dwShareMode = 0;
  lpSecurityAttributes = NULL;
  dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
  hTemplateFile = 0;

  
  // Open file for writing, create if it does not exist
  TEST_NOT_EQUAL(INVALID_HANDLE_VALUE, 
		 handle = CeCreateFile(lpPathName,
				       dwDesiredAccess,
				       dwShareMode, 
				       lpSecurityAttributes, 
				       dwCreationDisposition,
				       dwFlagsAndAttributes,
				       hTemplateFile));

  if (handle == INVALID_HANDLE_VALUE)
    onError("File handle creation failed");

  return Py_BuildValue("i",handle);
}

static char pyrapi_CeCloseHandle__doc__[] = 
"Close an open file handle.
params: (CeFileHandle)";


static PyObject *
pyrapi_CeCloseHandle(PyObject *self, PyObject *args)
{
  HANDLE handle;

  if (!PyArg_ParseTuple(args, "i", &handle))
    return NULL;

  CeCloseHandle(handle);

  return Py_None;
}

static char pyrapi_CeReadFile__doc__[] = 
"Read the contents of an open file handle.
params: (CeFileHandle, NumberOfBytesToRead)
result: (String, NumberOfBytesRead)";


static PyObject *
pyrapi_CeReadFile(PyObject *self, PyObject *args)
{
  PyObject *lpResult;
  LPWSTR lpBuffer;
  HANDLE hFile;
  DWORD nNumberOfBytesToRead;
  DWORD NumberOfBytesRead;
  LPOVERLAPPED lpOverlapped;
  //char * asci_result;

  if (!PyArg_ParseTuple(args, "ii", &hFile, &nNumberOfBytesToRead))
    return NULL;

  if (!(lpBuffer = malloc(nNumberOfBytesToRead)))
    onError("Out of memory, failed to alloc memory for read buffer");

  NumberOfBytesRead = 0 ;
  lpOverlapped = NULL;

  TEST_NOT_FALSE(CeReadFile(hFile, lpBuffer, nNumberOfBytesToRead, &NumberOfBytesRead, lpOverlapped));

  //asci_result = rapi_wstr_to_ascii(lpBuffer);
  //printf("Orig Buffer =  \"%s\"\n", lpBuffer);
  //printf("Buffer =  \"%s\"\n", asci_result);
  
  if (NumberOfBytesRead == 0) {
    free(lpBuffer);
    return Py_None;
  }

  lpResult = Py_BuildValue("(s#,i)",(char *)lpBuffer,NumberOfBytesRead,NumberOfBytesRead);
  free(lpBuffer);

  return lpResult;
}



static char pyrapi_CeWriteFile__doc__[] = 
"Write a string to an open file handle.
params: (file, string)
result: (NumberOfBytesWritten)";


static PyObject *
pyrapi_CeWriteFile(PyObject *self, PyObject *args)
{
  char * write_buffer;
  HANDLE hFile;
  DWORD NumberOfBytesWritten;
  LPOVERLAPPED lpOverlapped;

  if (!PyArg_ParseTuple(args, "is", &hFile, &write_buffer))
    return NULL;

  NumberOfBytesWritten = 0 ;
  lpOverlapped = NULL;

  TEST_NOT_FALSE(CeWriteFile(hFile, write_buffer, strlen(write_buffer), &NumberOfBytesWritten, lpOverlapped));

  return  Py_BuildValue("i",NumberOfBytesWritten);

}

/****************************************************************************
 * file_management.c functions
 ***************************************************************************/
static char pyrapi_CeFindAllFiles__doc__[] = 
"Find all files that match a pattern..
params: (PathPattern)
result: [(filename, file_attributes, creation_time, last_access_time, last_write_time)...]

Example PathPattern: r'\\My Documents\\*.*'

Note: as far as I can tell PocketPC lies about the creation time. It always sets the creation
time, last access time and last write time to the same time (the last write time I think). ";


static PyObject *
pyrapi_CeFindAllFiles(PyObject *self, PyObject *args)
{
  char * ascii_path_pattern;
  LPWSTR unicode_path_pattern;
  PyTupleObject * return_tuple;
  unsigned i = 0;

  CE_FIND_DATA* find_data = NULL;
  DWORD file_count = 0;
  DWORD flags = FAF_NAME|FAF_ATTRIBUTES|FAF_CREATION_TIME|FAF_LASTACCESS_TIME|FAF_LASTWRITE_TIME|FAF_SIZE_LOW;

  if (!PyArg_ParseTuple(args, "s", &ascii_path_pattern))
    return NULL;	

  unicode_path_pattern = rapi_wstr_from_ascii(ascii_path_pattern);

  TEST_NOT_FALSE(CeFindAllFiles(unicode_path_pattern, 
				flags, 
				&file_count, &find_data));


  return_tuple = (PyTupleObject *)PyTuple_New(file_count);
  printf("Number of files: %u\n", file_count);

  for (i = 0; i < file_count; i++) 
    {
      printf("File %3u: \"%s\"\n", i, rapi_wstr_to_ascii(find_data[i].cFileName));
  
      if (PyTuple_SetItem((PyObject *)return_tuple, i, 
			  Py_BuildValue("(slllll)",rapi_wstr_to_ascii(find_data[i].cFileName),
					find_data[i].dwFileAttributes,
					rapi_filetime_to_unix_time(&(find_data[i].ftCreationTime)),
					rapi_filetime_to_unix_time(&(find_data[i].ftLastAccessTime)),
					rapi_filetime_to_unix_time(&(find_data[i].ftLastWriteTime)),
					find_data[i].nFileSizeLow
					)))
	onError("Failed to add filename to result tuple");
    }
	
  TEST_HRESULT(CeRapiFreeBuffer(find_data));

  return (PyObject *)return_tuple;
}


/***************************************************************************
 * database.c functions
 *
 * These are probably better done as an extention type but this will 
 * have to do for now.
 **************************************************************************/
static char pyrapi_CeFindAllDatabases__doc__[] = 
"Find all of the databases on the PDA. 
params: None
result: ((name,oid,flags,type,num_records,num_sort_order, size),...)

I present I think that it is only safe to look at the name and then
pass the tuple corresponding to that name back into the CeOpenDatase function.
I except that I have got some of the type conversion wrong for the other 
paramters.
";

static PyObject *
pyrapi_CeFindAllDatabases(PyObject *self, PyObject *args)
{
  PyTupleObject * return_tuple;
  PyObject * temp;
  char * name;
  LPCEDB_FIND_DATA pFindData = NULL;
  WORD cFindData = 0;
  WORD i;
  DWORD type = 0;

  if (!PyArg_ParseTuple(args, "")) // Force no args
    return NULL;

  if ( CeFindAllDatabases( type, 0xFFFF, &cFindData, &pFindData ) )
    {
      return_tuple = (PyTupleObject *)PyTuple_New(cFindData);

      for ( i = 0 ; i < cFindData ; i++ )
	{
	  name = rapi_wstr_to_ascii(pFindData[ i ].DbInfo.szDbaseName);
	    //wcstombs( name, (const wchar_t *)pFindData[ i ].DbInfo.szDbaseName, CEDB_MAXDBASENAMELEN );
	  printf("Backup database : id = 0x%08X'%s' ", ( unsigned ) pFindData[ i ].OidDb, name );
	  printf( "type = %d ", ( unsigned ) pFindData[ i ].DbInfo.dwDbaseType );
	  printf( "size = %d\n", pFindData[ i ].DbInfo.wNumRecords );

	  // (name,oid,flags,type,num_records,num_sort_order, size)
	  temp = Py_BuildValue("(s,i,i,i,i,i)",
			       name,
			       pFindData[ i ].OidDb,
			       pFindData[ i ].DbInfo.dwFlags,
			       pFindData[ i ].DbInfo.wNumRecords,
			       pFindData[ i ].DbInfo.wNumSortOrder,
			       pFindData[ i ].DbInfo.dwSize
			       );

	  if (PyTuple_SetItem((PyObject *)return_tuple, i, temp))
	    onError("Failed to add filename to result tuple");
	}
    }
  else
    {
      onError( "CeFindAllDatabases() returned error..." );
    }
  
  if ( pFindData )
    {
     CeRapiFreeBuffer( pFindData );
    } 

  return (PyObject *)return_tuple;
}

static char pyrapi_CeOpenDatabase__doc__[] = 
"Open a database on the PDA and return a handle to it.
params: one element of the list returned from CeFindAllDataBases.
result: a open database handle to be passed back into the 
CeReadRecordProps function.
";

static PyObject *
pyrapi_CeOpenDatabase(PyObject *self, PyObject *args)
{
  char * name;
  int OidDb, dwFlags, wNumRecords, wNumSortOrder, dwSize;
  HANDLE hDb;

  if (!PyArg_ParseTuple(args, "(siiiii)",
			&name,
			&OidDb,
			&dwFlags,
			&wNumRecords,
			&wNumSortOrder,
			&dwSize))			       
    return NULL;


  hDb = CeOpenDatabase( &( OidDb ), NULL, 0, CEDB_AUTOINCREMENT, (HWND)NULL );
  if ( hDb == INVALID_HANDLE_VALUE )
    onError( "Invalid handle value." );

  return  Py_BuildValue("l",hDb);
    
}

static char pyrapi_CeReadRecordProps__doc__[] = 
"Read a single record from an open database.
params: database_handle
result: A dictionary keyed on the field_id.

The values of the dictionary are tuples int the form (type_name, value). 
The type are mapped to Python type in the following way:

BLOB -> STRING, 
BOOL -> INT, 
FILETYPE -> INT (suitable for passing to time.ctime),
INT16 -> INT,
INT32 -> INT,
STRING -> STRING,
INT64 -> FLOAT (I am not sure of the validity of this),
UINT16 -> INT,
UINT32 -> INT,
UNKNOWN -> STRING (Catch all for unknown types).
";


/* Maximum size (in bytes) of a record from the database */
#define BUFSIZE		1024*16

static PyObject *
pyrapi_CeReadRecordProps(PyObject *self, PyObject *args)
{
  PyDictObject * return_dict;
  PyTupleObject * return_tuple;

  HANDLE hDb;
  CEOID oid;
  LPBYTE lpBuffer = NULL;
  DWORD cbBuffer = 0;
  WORD  propID = 0;
  PCEPROPVAL pval;
  char buffer[ BUFSIZE ];
  int i,j;
  DWORD error;

  if (!PyArg_ParseTuple(args, "i", &hDb))
    return NULL;

  oid = CeReadRecordProps( hDb, CEDB_ALLOWREALLOC, &propID, NULL, &lpBuffer, &cbBuffer );

  if ( oid == 0 )
    {
      error = CeGetLastError();
      switch ( error )
	{
	case ERROR_INVALID_PARAMETER:
	  onError( "Invalid parameter to CeReadRecordProps!" );
	  break;
	case ERROR_NO_DATA:
	  onError( "No data in CeReadRecordProps property!" );
	  break;
	case ERROR_INSUFFICIENT_BUFFER:
	  onError( "Insufficient buffer !" );
	  break;
	case ERROR_KEY_DELETED:
	  onError( "Key deleted !" );
	  break;
	case ERROR_NO_MORE_ITEMS:
	  return Py_None;
	  break;
	default:
	  onError("Unknown error return from CeReadRecordProps.");
	}
    }

  //propID is now the number of properties
  //cbBuffer is now the length of lpBuffer
  
  pval = ( PCEPROPVAL ) lpBuffer;  // Get the return buffer into a prop structure.
  return_dict = (PyDictObject *)PyDict_New();

  // So we loop over all of the props (code knicked from DBBackup.c)
  for ( i = 0; i < propID; i++ )
    {
      printf( " propid = %-4X ", ( unsigned ) ( pval[ i ].propid >> 16 ) );
      switch ( ( pval[ i ].propid & 0xFFFF ) )
	{
	case CEVT_BLOB:
	  for ( j = 0; j < pval[ i ].val.blob.dwCount; j++ )
	    {
	      sprintf( buffer + 3 * j, "%02X ", pval[ i ].val.blob.lpb[ j ] );
	    }
	  sprintf( buffer, "%d", pval[ i ].val.blob.dwCount );
	  PyDict_SetItem((PyObject *)return_dict, Py_BuildValue("i", pval[ i ].propid >> 16),
			 Py_BuildValue("(ss)","BLOB",buffer));
	  break;
	case CEVT_BOOL:
	  PyDict_SetItem((PyObject *)return_dict, Py_BuildValue("i", pval[ i ].propid >> 16),
			 Py_BuildValue("(si)","BOOL", pval[ i ].val.boolVal ? 1 : 0));

	  break;
	case CEVT_FILETIME:
	  PyDict_SetItem((PyObject *)return_dict, Py_BuildValue("i", pval[ i ].propid >> 16),
			 Py_BuildValue("(si)","FILETIME", rapi_filetime_to_unix_time(&(pval[ i ].val.filetime))));

	  break;
	case CEVT_I2:
	  PyDict_SetItem((PyObject *)return_dict, Py_BuildValue("i", pval[ i ].propid >> 16),
			 Py_BuildValue("(si)","INT16", pval[ i ].val.iVal));

	  break;
	case CEVT_I4:
	  PyDict_SetItem((PyObject *)return_dict, Py_BuildValue("i", pval[ i ].propid >> 16),
			 Py_BuildValue("(sl)","INT32", pval[ i ].val.lVal));

	  break;
	case CEVT_LPWSTR:
	  PyDict_SetItem((PyObject *)return_dict, Py_BuildValue("i", pval[ i ].propid >> 16),
			 Py_BuildValue("(ss)","STRING", rapi_wstr_to_ascii( pval[ i ].val.lpwstr )));

	  break;
	case CEVT_R8:
	  PyDict_SetItem((PyObject *)return_dict, Py_BuildValue("i", pval[ i ].propid >> 16),
			 Py_BuildValue("(sd)","INT64", pval[ i ].val.dblVal));

	  break;
	case CEVT_UI2:
	  PyDict_SetItem((PyObject *)return_dict, Py_BuildValue("i", pval[ i ].propid >> 16),
			 Py_BuildValue("(sl)","UINT16", pval[ i ].val.uiVal));

	  break;
	case CEVT_UI4:
	  PyDict_SetItem((PyObject *)return_dict, Py_BuildValue("i", pval[ i ].propid >> 16),
			 Py_BuildValue("(sl)","UINT32", pval[ i ].val.ulVal));

	  break;
	default:
	  sprintf( buffer, "0x%08lX", (long unsigned int)(pval[ i ].val.ulVal) );
	  PyDict_SetItem((PyObject *)return_dict, Py_BuildValue("i", pval[ i ].propid >> 16),
			 Py_BuildValue("(ss)","UNKNOWN", buffer));

	  break;
	}
    }

  return_tuple = (PyTupleObject *)PyTuple_New(2);
  PyTuple_SetItem((PyObject *)return_tuple, 0, Py_BuildValue("l",oid));
  PyTuple_SetItem((PyObject *)return_tuple, 1,(PyObject *)return_dict );
  return (PyObject *)return_tuple;
}

/****************************************************************************
 * METHOD REGISTRATION TABLE
 *
 ***************************************************************************/

static struct PyMethodDef pyrapi_methods[] = {

  /* File access functions */
  { "CeWriteFile",   pyrapi_CeWriteFile,   METH_VARARGS, pyrapi_CeWriteFile__doc__},
  { "CeCloseHandle", pyrapi_CeCloseHandle, METH_VARARGS, pyrapi_CeCloseHandle__doc__},
  { "CeRapiInit",    pyrapi_CeRapiInit,    METH_VARARGS, pyrapi_CeRapiInit__doc__},
  { "CeCreateFile",  pyrapi_CeCreateFile,  METH_VARARGS, pyrapi_CeCreateFile__doc__},
  { "CeReadFile",    pyrapi_CeReadFile,    METH_VARARGS, pyrapi_CeReadFile__doc__},

  /* File management functions */
  { "CeFindAllFiles", pyrapi_CeFindAllFiles, METH_VARARGS, pyrapi_CeFindAllFiles__doc__},

  /* Database access functions */
  { "CeFindAllDatabases", pyrapi_CeFindAllDatabases, METH_VARARGS, pyrapi_CeFindAllDatabases__doc__},
  { "CeOpenDatabase",     pyrapi_CeOpenDatabase,     METH_VARARGS, pyrapi_CeOpenDatabase__doc__},
  { "CeReadRecordProps",  pyrapi_CeReadRecordProps,  METH_VARARGS, pyrapi_CeReadRecordProps__doc__},


  {NULL, NULL, 0, NULL}        /* Sentinel */
};

/***************************************************************************
 * INITIALISATION FUNCTION
 **************************************************************************/

void
initpyrapi(void)
{
  PyObject *m, *d;

  m = Py_InitModule("pyrapi", pyrapi_methods);

  d = PyModule_GetDict(m);
  ErrorObject = Py_BuildValue("s", "pyrapimod.error");
  PyDict_SetItemString(d, "error", ErrorObject);

  if (PyErr_Occurred())
    Py_FatalError("can't init pyrapi module");

  VERIFY_HRESULT(CeRapiInit());

}

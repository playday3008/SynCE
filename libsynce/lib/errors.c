/* $Id$ */
#include "synce.h"
#include "synce_log.h"

const char* synce_strerror(DWORD error)
{
	char* result = NULL;
	
	switch (error)
	{
		case S_OK: 			result = "Success";   break;
		case S_FALSE: 	result = "FALSE"; 		break;

    case E_ABORT:         result = "The operation was aborted because of an unspecified error"; break;
    case E_ACCESSDENIED:  result = "A general access-denied error"; break;
		case E_FAIL: 		      result = "An unspecified failure has occurred"; 	break;
    case E_HANDLE:        result = "An invalid handle was used"; break;
    case E_INVALIDARG:  	result = "One or more arguments are invalid."; break;

    case E_NOTIMPL:       result = "The method is not implemented"; break;
    case E_OUTOFMEMORY:   result = "The method failed to allocate necessary memory"; break;
    case E_PENDING:       result = "The data necessary to complete the operation is not yet available"; break;
    case E_POINTER:       result = "An invalid pointer was used"; break;
    case E_UNEXPECTED:    result = "A catastrophic failure has occurred"; break;

		case ERROR_FILE_NOT_FOUND:    result = "File not found"; break;
		case ERROR_PATH_NOT_FOUND:    result = "Path not found"; break;
		case ERROR_TOO_MANY_OPEN_FILES: result = "Too many open files"; break;
		case ERROR_ACCESS_DENIED: result = "Access is denied"; break;
		case ERROR_INVALID_HANDLE:    result = "Invalid file handle"; break;
		case ERROR_NOT_ENOUGH_MEMORY: result = "Not enough memory"; break;
		case ERROR_NO_MORE_FILES:     result = "There are no more files"; break;
		case ERROR_SEEK: result = "Drive seek error"; break;
		case ERROR_SHARING_VIOLATION: result = "Sharing violation"; break;
		case ERROR_NOT_SUPPORTED: result = "Not supported"; break;
		case ERROR_DUP_NAME: result = "Duplicate name"; break;
		case ERROR_FILE_EXISTS: result = "File exists"; break;
		case ERROR_INVALID_PARAMETER: result = "Invalid parameter"; break;
		case ERROR_DISK_FULL:         result = "Not enough space on disk"; break;
		case ERROR_INSUFFICIENT_BUFFER:   result = "Insufficient buffer"; break;
		case ERROR_INVALID_NAME:     result = "Invalid name"; break;
		case ERROR_DIR_NOT_EMPTY:     result = "Directory not empty"; break;
		case ERROR_ALREADY_EXISTS:     result = "File already exists"; break;
		case ERROR_NO_DATA:     result = "No more data in pipe"; break;
		case ERROR_NO_MORE_ITEMS:     result = "No more data available"; break;
		case ERROR_KEY_DELETED:     result = "Registry key marked for deletion"; break;
		
		default: 
			synce_trace("Unknown error code: 0x%08x", error);
			result = "Unknown error";
			break;
	}

	return result;
}

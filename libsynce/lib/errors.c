/* $Id$ */
#include "synce.h"
#include "synce_log.h"

char* synce_strerror(DWORD error)
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
		case ERROR_DIR_NOT_EMPTY:     result = "Directory not empty"; break;
		case ERROR_INVALID_PARAMETER: result = "Invalid parameter"; break;
		
		default: 
			synce_trace("Unknown error code: 0x%08x", error);
			result = "Unknown error";
			break;
	}

	return result;
}

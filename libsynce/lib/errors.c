/* $Id$ */
#include "synce.h"
#include "synce_log.h"

char* synce_strerror(DWORD error)
{
	char* result = NULL;
	
	switch (error)
	{
		case S_OK: 			result = "Success";   break;
		case S_FALSE: 	result = "False"; 		break;
		case E_FAIL: 		result = "Failure"; 	break;

		case ERROR_FILE_NOT_FOUND: result = "File not found"; break;
		case ERROR_PATH_NOT_FOUND: result = "Path not found"; break;
		case ERROR_DIR_NOT_EMPTY:  result = "Directory not empty"; break;
		
		default: 
			synce_trace("Unknown error code: 0x%08x", error);
			result = "Unknown error";
			break;
	}

	return result;
}

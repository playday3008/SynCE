/* $Id$ */
#include "rapi_internal.h"
#include "rapi.h"
#include "rapi_buffer.h"
#include "rapi_context.h"

bool CeReadFile(
		unsigned  hFile, 
		void*     lpBuffer, 
		unsigned  nNumberOfBytesToRead, 
		unsigned* lpNumberOfBytesRead, 
		void*     lpOverlapped)
{
	RapiContext* context = rapi_context_current();
	u_int32_t bytes_read = 0;

	rapi_buffer_write_uint32(context->input, 0x06);
	rapi_buffer_write_uint32(context->input, hFile);
	rapi_buffer_write_optional_out(context->input, lpBuffer, nNumberOfBytesToRead);
	rapi_buffer_write_optional_in(context->input, lpOverlapped, 0x14);

	if ( !rapi_context_call(context) )
		return false;

	bytes_read = rapi_buffer_read_uint32(context->output);
	if (lpNumberOfBytesRead)
		*lpNumberOfBytesRead = bytes_read;

	if (lpBuffer)
		rapi_buffer_read_data(context->output, lpBuffer, bytes_read);

	return true;
}


/* $Id$ */
#include "rapi.h"
#include "rapi_context.h"
#include "rapi_endian.h"

BOOL CeFindAllDatabases(
		DWORD dwDbaseType, 
		WORD wFlags, 
		LPWORD cFindData, 
		LPLPCEDB_FIND_DATA ppFindData)
{
	RapiContext* context = rapi_context_current();
	uint16_t count;
	
	rapi_trace("begin");
	
	rapi_context_begin_command(context, 0x2c);
	rapi_buffer_write_uint32(context->send_buffer, dwDbaseType);
	rapi_buffer_write_uint16(context->send_buffer, wFlags);

	if ( !rapi_context_call(context) )
		return false;

/*	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->last_error) )
		return false;
	rapi_trace("context->last_error=0x%08x", context->last_error);*/

	rapi_buffer_read_uint16(context->recv_buffer, &count);
	
	rapi_trace("found 0x%04x databases", count);

	if (cFindData)
		*cFindData = count;

	if (ppFindData && count > 0)
	{
		LPCEDB_FIND_DATA find_data = calloc(count, sizeof(CEDB_FIND_DATA));
		
		*ppFindData = find_data;
		
		if (find_data)
		{
			uint32_t i;
			uint32_t name_size = 0;
			
			for(i = 0; i < count; i++)
			{
				if (wFlags & FAD_OID)
					if ( !rapi_buffer_read_uint32(context->recv_buffer, &find_data[i].OidDb) )
						goto fail;

				if (wFlags & FAD_NAME)
					rapi_buffer_read_uint32(context->recv_buffer, &name_size);

				if (wFlags & FAD_FLAGS)
					rapi_buffer_read_uint32(context->recv_buffer, &find_data[i].DbInfo.dwFlags);

				if (wFlags & FAD_NAME)
				{
					if (name_size)
					{
						rapi_buffer_read_data(context->recv_buffer, find_data[i].DbInfo.szDbaseName, name_size * sizeof(WCHAR) );
						rapi_trace_wstr(find_data[i].DbInfo.szDbaseName);
					}
					else
					{
						rapi_error("name_size is 0");
						goto fail;
					}
				}

				if (wFlags & FAD_TYPE)
					rapi_buffer_read_uint32(context->recv_buffer, &find_data[i].DbInfo.dwDbaseType);

				if (wFlags & FAD_NUM_RECORDS)
					rapi_buffer_read_uint16(context->recv_buffer, &find_data[i].DbInfo.wNumRecords);

				if (wFlags & FAD_NUM_SORT_ORDER)
					rapi_buffer_read_uint16(context->recv_buffer, &find_data[i].DbInfo.wNumSortOrder);

				if (wFlags & FAD_SIZE)
					rapi_buffer_read_uint32(context->recv_buffer, &find_data[i].DbInfo.dwSize);

				if (wFlags & FAD_LAST_MODIFIED)
				{
					rapi_buffer_read_uint32(context->recv_buffer, &find_data[i].DbInfo.ftLastModified.dwLowDateTime);
					rapi_buffer_read_uint32(context->recv_buffer, &find_data[i].DbInfo.ftLastModified.dwHighDateTime);
				}

				if (wFlags & FAD_SORT_SPECS)
				{
					int j;

					for (j = 0; j < CEDB_MAXSORTORDER; j++)
					{
						rapi_buffer_read_uint32(context->recv_buffer, &find_data[i].DbInfo.rgSortSpecs[j].propid);
						rapi_buffer_read_uint32(context->recv_buffer, &find_data[i].DbInfo.rgSortSpecs[j].dwFlags);
					}
				}
			} /* for each database */
		}
		else
		{
			rapi_error("failed to allocate memory");
			goto fail;
		}

		*ppFindData = find_data;
	}

	return true;

fail:
	if (ppFindData && *ppFindData)
		free(ppFindData);
	return false;
}

HANDLE CeOpenDatabase(
		PCEOID poid, 
		LPWSTR lpszName, 
		CEPROPID propid, 
		DWORD dwFlags, 
		HWND hwndNotify)
{
	return INVALID_HANDLE_VALUE;
}

CEOID CeReadRecordProps(
		HANDLE hDbase, 
		DWORD dwFlags, 
		LPWORD lpcPropID, 
		CEPROPID *rgPropID, 
		LPBYTE *lplpBuffer, 
		LPDWORD lpcbBuffer)
{
	return 0;
}



/* $Id$ */
#include "rapi.h"
#include "rapi_context.h"
#include "rapi_endian.h"

BOOL CeDeleteDatabase(/*{{{*/
		CEOID oid)
{
	RapiContext* context = rapi_context_current();
	BOOL return_value = false;
	
	rapi_trace("begin");
	
	rapi_context_begin_command(context, 0x0f);
	rapi_buffer_write_uint32(context->send_buffer, oid);

	if ( !rapi_context_call(context) )
		goto exit;

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->last_error) )
		goto exit;
	rapi_trace("context->last_error=0x%08x", context->last_error);

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &return_value) )
		goto exit;
	
exit:
	return return_value;
}/*}}}*/

BOOL CeFindAllDatabases(/*{{{*/
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
				{
					if ( !rapi_buffer_read_uint32(context->recv_buffer, &find_data[i].OidDb) )
						goto fail;
					rapi_trace("oid=%08x", find_data[i].OidDb);
				}

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
}/*}}}*/

HANDLE CeFindFirstDatabase(/*{{{*/
		DWORD dwDbaseType)
{
	RapiContext* context = rapi_context_current();
	HANDLE return_value = INVALID_HANDLE_VALUE;
	
	rapi_trace("begin");
	
	rapi_context_begin_command(context, 0x0a);
	rapi_buffer_write_uint32(context->send_buffer, dwDbaseType);

	if ( !rapi_context_call(context) )
		goto exit;

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->last_error) )
		goto exit;
	rapi_trace("context->last_error=0x%08x", context->last_error);

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &return_value) )
		goto exit;
	
exit:
	return return_value;
}/*}}}*/

CEOID CeFindNextDatabase(/*{{{*/
		HANDLE hEnum)
{
	RapiContext* context = rapi_context_current();
	CEOID return_value = 0;
	
	rapi_trace("begin");
	
	rapi_context_begin_command(context, 0x0b);
	rapi_buffer_write_uint32(context->send_buffer, hEnum);

	if ( !rapi_context_call(context) )
		goto exit;

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->last_error) )
		goto exit;
	rapi_trace("context->last_error=0x%08x", context->last_error);

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &return_value) )
		goto exit;
	
exit:
	return return_value;
}/*}}}*/

HANDLE CeOpenDatabase(/*{{{*/
		PCEOID poid, 
		LPWSTR lpszName, 
		CEPROPID propid, 
		DWORD dwFlags, 
		HWND hwndNotify)
{
	RapiContext* context = rapi_context_current();
	HANDLE handle = INVALID_HANDLE_VALUE;
	
	rapi_trace("begin");
	
	rapi_context_begin_command(context, 0x0e);
	rapi_buffer_write_uint32(context->send_buffer, poid ? *poid : 0);
	rapi_buffer_write_uint32(context->send_buffer, propid);
	rapi_buffer_write_uint32(context->send_buffer, dwFlags);

	if ( !rapi_context_call(context) )
		goto exit;

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->last_error) )
		goto exit;
	rapi_trace("context->last_error=0x%08x", context->last_error);

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &handle) )
		goto exit;
	
exit:
	return handle;
}/*}}}*/

CEOID CeReadRecordProps(/*{{{*/
		HANDLE hDbase, 
		DWORD dwFlags, 
		LPWORD lpcPropID, 
		CEPROPID *rgPropID, 
		LPBYTE *lplpBuffer, 
		LPDWORD lpcbBuffer)
{
	RapiContext* context = rapi_context_current();
	CEOID return_value = 0;
	uint16_t prop_id_count = 0;
	uint32_t size = 0;
	uint32_t i;
	unsigned char* buffer = NULL;
	
	rapi_trace("begin");
	
	rapi_context_begin_command(context, 0x10);
	rapi_buffer_write_uint32(context->send_buffer, hDbase);
	rapi_buffer_write_uint32(context->send_buffer, dwFlags);
	rapi_buffer_write_uint32(context->send_buffer, 0);
	rapi_buffer_write_uint32(context->send_buffer, 0);
	rapi_buffer_write_uint32(context->send_buffer, 0);
	rapi_buffer_write_uint16(context->send_buffer, 0);

	if ( !rapi_context_call(context) )
		goto fail;

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->last_error) )
		goto fail;
	rapi_trace("context->last_error=0x%08x", context->last_error);

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &return_value) )
		goto fail;
	rapi_trace("return_value=0x%08x", return_value);
	
	if ( !rapi_buffer_read_uint32(context->recv_buffer, &size) )
		goto fail;
	rapi_trace("size=%i", size);

	if ( lpcbBuffer )
		*lpcbBuffer = size;

	if ( !rapi_buffer_read_uint16(context->recv_buffer, &prop_id_count) )
		goto fail;
	rapi_trace("prop_id_count=%i", prop_id_count);

	if (lpcPropID)
		*lpcPropID = prop_id_count;

	if (lplpBuffer)
	{ 
		CEPROPVAL* propval;
		
		buffer = calloc(1, size);
		*lplpBuffer = buffer;
		
		if (buffer)
		{
			if ( !rapi_buffer_read_data(context->recv_buffer, *lplpBuffer, size) )
			{
				rapi_error("failed to read buffer");
				goto fail;
			}

			propval = (CEPROPVAL*)buffer;

			for (i = 0; i < prop_id_count; i++)
			{
				switch (propval[i].propid & 0xffff)
				{
					/* XXX: we can get problems here on 64-bit platforms */
					
					case CEVT_BLOB:
						propval[i].val.blob.lpb = (LPBYTE)
							(buffer + letoh32((uint32_t)propval[i].val.blob.lpb));
						break;

					case CEVT_LPWSTR:
						propval[i].val.lpwstr = (LPWSTR)
							(buffer + letoh32((uint32_t)propval[i].val.lpwstr));
						rapi_trace_wstr(propval[i].val.lpwstr);
						break;

					/* TODO: convert endians on other data types! */
				}
			}
		}
		else
		{
 			rapi_error("failed to allocate 0x%08x bytes", size);
		}
	}

	return return_value;

fail:
 	rapi_error("failed");
	CeRapiFreeBuffer(buffer);
	return 0;
}/*}}}*/

CEOID CeSeekDatabase(/*{{{*/
		HANDLE hDatabase, 
		DWORD dwSeekType, 
		DWORD dwValue, 
		LPDWORD lpdwIndex)
{
	RapiContext* context = rapi_context_current();
	CEOID return_value = 0;
	
	rapi_trace("begin");

	if (!lpdwIndex)
	{
		rapi_error("lpdwIndex is NULL");
		goto fail;
	}
	
	rapi_context_begin_command(context, 0x13);
	rapi_buffer_write_uint32(context->send_buffer, hDatabase);
	rapi_buffer_write_uint32(context->send_buffer, dwSeekType);

	switch (dwSeekType)
	{
		case CEDB_SEEK_VALUESMALLER:
		case CEDB_SEEK_VALUEFIRSTEQUAL:
		case CEDB_SEEK_VALUEGREATER:
		case CEDB_SEEK_VALUENEXTEQUAL:
			rapi_error("Seek type by value is not yet supported", dwSeekType);
			goto fail;
	
		default:
			rapi_buffer_write_uint32(context->send_buffer, dwValue);
			break;
	}

	if ( !rapi_context_call(context) )
		goto fail;

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->last_error) )
		goto fail;
	rapi_trace("context->last_error=0x%08x", context->last_error);

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &return_value) )
		goto fail;
	
	if ( !rapi_buffer_read_uint32(context->recv_buffer, lpdwIndex) )
		goto fail;
	
	return return_value;

fail:
	return 0;
}/*}}}*/


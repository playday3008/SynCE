/* $Id$ */
#include "rapi.h"
#include "rapi_context.h"
#include "rapi_endian.h"
#include "rapi_wstr.h"   /* for unicode length function */

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

CEOID CeWriteRecordProps( HANDLE hDbase, CEOID oidRecord, WORD cPropID, CEPROPVAL* rgPropVal)/*{{{*/
{
	RapiContext* context = rapi_context_current();
	CEOID return_value = 0;

	uint32_t datalen, buflen;
	uint32_t i;


	/*
	 *	Format of the CeWriteRecordProps packet - primitives are encoded in the CEPROPVAL structures, lpwstr and blob properties are
	 *	attached to the end of the buffer and referenced by offset pointers
	 *
	 *		long hDBase | long oidRecord | long cPropID | long datalen (of following data) | n * CEPROPVAL | char[] data
	 *
	 *	Because CEPROPVAL is a union, the format is different for every type of prop:
	 *
	 *	long or short (iVal, uiVal, lVal, ulVal, boolVal): long propid | short wFlags | short wLenData (unused, set to 0) | short iVal or boolVal | short uiVal | long lVal or ulVal
	 *
	 *	FILETIME or double: long propid | short wFlags | short wLenData (unused) | DWORD FILETIME or double
	 *
	 *	lpwstr: long propid | short wFlags | short wLenData (unused) | long offset ( points to string data in data buffer, counted from beginning of CEPROPVALs)
	 *
	 *	blob: long propid | short wFlags | short wLenData (unused) | long blobsize | long offset (same as lpwstr)
	 */

	rapi_trace("begin");
	rapi_context_begin_command(context, 0x11);
	rapi_buffer_write_uint32(context->send_buffer, hDbase);    /* Parameter1 : */
	rapi_buffer_write_uint32(context->send_buffer, oidRecord); /* Parameter2 : Flags ? */ 
	rapi_buffer_write_uint16(context->send_buffer, cPropID);   /* Parameter3 */

	/*
	 * we have to go through the rgPropVals array three times:
	 *		1. to determine the size of the whole buffer, including data
	 *		2. to write out the CEPROPVAL array
	 *		3. to write the data segment
	 */

	/* calculate the length of the whole buffer, including the data segment at the end */

	buflen = cPropID * sizeof( CEPROPVAL ); /* length of all cepropvals */

	for ( i = 0; i < cPropID; i++ )
	{	
		switch ( ( rgPropVal[i].propid ) & 0xFFFF )
		{
			case CEVT_BLOB:
				buflen += rgPropVal[i].val.blob.dwCount;
				break;
			case CEVT_LPWSTR:
				buflen += sizeof(WCHAR) * ( rapi_wstr_strlen( rgPropVal[i].val.lpwstr ) + 1 );
				break;
			default:
				break;
		}
	}

	rapi_buffer_write_uint32(context->send_buffer, buflen);

	/*
		 second time: write n * CEPROPVAL. Can't do it in one block, as we have to adjust the buffer offsets
	 */

	datalen = cPropID * sizeof( CEPROPVAL ); /*  holds the offset to the end of the data buffer	*/

	for ( i = 0; i < cPropID; i++ )
	{
		rapi_buffer_write_uint32(context->send_buffer,rgPropVal[i].propid );
		rapi_buffer_write_uint16(context->send_buffer,rgPropVal[i].wLenData );
		rapi_buffer_write_uint16(context->send_buffer,rgPropVal[i].wFlags );

		switch ( ( rgPropVal[i].propid ) & 0xFFFF )
		{
			case CEVT_BLOB:
				rapi_buffer_write_uint32(context->send_buffer, rgPropVal[i].val.blob.dwCount );
				rapi_buffer_write_uint32(context->send_buffer, datalen );
				datalen += rgPropVal[i].val.blob.dwCount;
				break;
			case CEVT_LPWSTR:
				rapi_buffer_write_uint32(context->send_buffer, datalen );
				rapi_buffer_write_uint32(context->send_buffer, 0);
			        datalen += sizeof(WCHAR) * ( rapi_wstr_strlen( rgPropVal[i].val.lpwstr ) + 1 );
				break;
			case CEVT_I2:
			case CEVT_UI2:
			case CEVT_I4:
			case CEVT_UI4:
				rapi_buffer_write_uint32(context->send_buffer, rgPropVal[i].val.lVal );
				rapi_buffer_write_uint16(context->send_buffer, rgPropVal[i].val.iVal );
				rapi_buffer_write_uint16(context->send_buffer, rgPropVal[i].val.uiVal );
				break;
			case CEVT_BOOL:
				rapi_buffer_write_uint16(context->send_buffer, rgPropVal[i].val.boolVal  );
				rapi_buffer_write_uint16(context->send_buffer, 0);
				rapi_buffer_write_uint32(context->send_buffer, 0);
				break;
			case CEVT_FILETIME:
				/* this assumes that the FILETIME is already in ole32 format! Is this a problem? */
				rapi_buffer_write_uint32(context->send_buffer, rgPropVal[i].val.filetime.dwLowDateTime );
				rapi_buffer_write_uint32(context->send_buffer, rgPropVal[i].val.filetime.dwHighDateTime );
				break;
			case CEVT_R8:
				rapi_buffer_write_optional(context->send_buffer, &(rgPropVal[i].val.dblVal), 4, 1 );
				break;
			default:
				break;
		}
	}	
	/* 3. write the data segment */
	for ( i = 0; i < cPropID; i++ )
	{	
		switch ( ( rgPropVal[i].propid ) & 0xFFFF )
		{
			case CEVT_BLOB:
				rapi_buffer_write_data(context->send_buffer, 
						       rgPropVal[i].val.blob.lpb, rgPropVal[i].val.blob.dwCount );
				break;
			case CEVT_LPWSTR:
			        rapi_buffer_write_data(context->send_buffer, 
						 rgPropVal[i].val.lpwstr, sizeof(WCHAR) * ( rapi_wstr_strlen( rgPropVal[i].val.lpwstr ) + 1) );

				break;
			default:
				break;
		}
	}	

	if ( !rapi_context_call(context) )
		goto fail;

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->last_error) )
		goto fail;
	rapi_trace("context->last_error=0x%08x", context->last_error);

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &return_value) )
		goto fail;
	rapi_trace("return_value=0x%08x", return_value);


	return return_value;
fail:
	rapi_error("failed");
	/*CeRapiFreeBuffer(buffer);*/
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


/* $Id$ */
#include "rapi_api.h"
#include "rapi_context.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_CONFIG_H
#include "rapi_config.h"
#endif

#define RAPI_DATABASE_DEBUG 0

#if RAPI_DATABASE_DEBUG
#define rapi_database_trace(args...)    synce_trace(args)
#define rapi_database_trace_wstr(args...)    synce_trace_wstr(args)
#define rapi_database_warning(args...)  synce_warning(args)
#define rapi_database_error(args...)    synce_error(args)
#else
#define rapi_database_trace(args...)
#define rapi_database_trace_wstr(args...)
#define rapi_database_warning(args...)
#define rapi_database_error(args...)
#endif


CEOID _CeCreateDatabase(
		LPWSTR lpszName,
		DWORD dwDbaseType,
		WORD wNumSortOrder,
		SORTORDERSPEC *rgSortSpecs)
{
	RapiContext* context = rapi_context_current();
	CEOID return_value = 0;
	int i;

	rapi_database_trace("begin");

	rapi_context_begin_command(context, 0x0d);

	/* NOTE: strange marshalling order for parameters */

	rapi_buffer_write_uint32(context->send_buffer, dwDbaseType);
	rapi_buffer_write_uint16(context->send_buffer, wNumSortOrder);

	/* NOTE: We can't write whole rgSortSpecs buffer directly because we need to
	 * think about the byte order */
	for (i = 0; i < wNumSortOrder; i++)
	{
		rapi_buffer_write_uint32(context->send_buffer, rgSortSpecs[i].propid);
		rapi_buffer_write_uint32(context->send_buffer, rgSortSpecs[i].dwFlags);
	}

	rapi_buffer_write_string(context->send_buffer, lpszName);

	if ( !rapi_context_call(context) )
		goto fail;

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->last_error) )
		goto fail;
	rapi_database_trace("context->last_error=0x%08x", context->last_error);

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &return_value) )
		goto fail;

	return return_value;

fail:
	return 0;
}

BOOL _CeDeleteDatabase(/*{{{*/
		CEOID oid)
{
	RapiContext* context = rapi_context_current();
	BOOL return_value = false;

	rapi_database_trace("begin");

	rapi_context_begin_command(context, 0x0f);
	rapi_buffer_write_uint32(context->send_buffer, oid);

	if ( !rapi_context_call(context) )
		goto exit;

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->last_error) )
		goto exit;
	rapi_database_trace("context->last_error=0x%08x", context->last_error);

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &return_value) )
		goto exit;

exit:
	return return_value;
}/*}}}*/

BOOL _CeFindAllDatabases(/*{{{*/
		DWORD dwDbaseType,
		WORD wFlags,
		LPWORD cFindData,
		LPLPCEDB_FIND_DATA ppFindData)
{
	RapiContext* context = rapi_context_current();
	uint16_t count;

	rapi_database_trace("begin");

	rapi_context_begin_command(context, 0x2c);
	rapi_buffer_write_uint32(context->send_buffer, dwDbaseType);
	rapi_buffer_write_uint16(context->send_buffer, wFlags);

	if ( !rapi_context_call(context) )
		return false;

	rapi_buffer_read_uint16(context->recv_buffer, &count);

	rapi_database_trace("found 0x%04x databases", count);

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
					rapi_database_trace("oid=%08x", find_data[i].OidDb);
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
						rapi_database_trace_wstr(find_data[i].DbInfo.szDbaseName);
					}
					else
					{
						rapi_database_error("name_size is 0");
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
			rapi_database_error("failed to allocate memory");
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

HANDLE _CeFindFirstDatabase(/*{{{*/
		DWORD dwDbaseType)
{
	RapiContext* context = rapi_context_current();
	HANDLE return_value = INVALID_HANDLE_VALUE;

	rapi_database_trace("begin");

	rapi_context_begin_command(context, 0x0a);
	rapi_buffer_write_uint32(context->send_buffer, dwDbaseType);

	if ( !rapi_context_call(context) )
		goto exit;

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->last_error) )
		goto exit;
	rapi_database_trace("context->last_error=0x%08x", context->last_error);

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &return_value) )
		goto exit;

exit:
	return return_value;
}/*}}}*/

CEOID _CeFindNextDatabase(/*{{{*/
		HANDLE hEnum)
{
	RapiContext* context = rapi_context_current();
	CEOID return_value = 0;

	rapi_database_trace("begin");

	rapi_context_begin_command(context, 0x0b);
	rapi_buffer_write_uint32(context->send_buffer, hEnum);

	if ( !rapi_context_call(context) )
		goto exit;

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->last_error) )
		goto exit;
	rapi_database_trace("context->last_error=0x%08x", context->last_error);

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &return_value) )
		goto exit;

exit:
	return return_value;
}/*}}}*/

HANDLE _CeOpenDatabase(/*{{{*/
		PCEOID poid,
		LPWSTR lpszName,
		CEPROPID propid,
		DWORD dwFlags,
		HWND hwndNotify)
{
	RapiContext* context = rapi_context_current();
	HANDLE handle = INVALID_HANDLE_VALUE;

	rapi_database_trace("begin");

	rapi_context_begin_command(context, 0x0e);
	/* TODO: support databse open by name */
	rapi_buffer_write_uint32(context->send_buffer, poid ? *poid : 0);
	rapi_buffer_write_uint32(context->send_buffer, propid);
	rapi_buffer_write_uint32(context->send_buffer, dwFlags);

	if ( !rapi_context_call(context) )
		goto exit;

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->last_error) )
		goto exit;
	rapi_database_trace("context->last_error=0x%08x", context->last_error);

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &handle) )
		goto exit;

exit:
	return handle;
}/*}}}*/

#if SIZEOF_VOID_P == 4

CEOID _CeReadRecordProps(/*{{{*/
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

	rapi_database_trace("begin");

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
	rapi_database_trace("context->last_error=0x%08x", context->last_error);

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &return_value) )
		goto fail;
	rapi_database_trace("return_value=0x%08x", return_value);

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &size) )
		goto fail;
	rapi_database_trace("size=%i", size);

	if ( lpcbBuffer )
		*lpcbBuffer = size;

	if ( !rapi_buffer_read_uint16(context->recv_buffer, &prop_id_count) )
		goto fail;
	rapi_database_trace("prop_id_count=%i", prop_id_count);

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
				rapi_database_error("failed to read buffer");
				goto fail;
			}

			propval = (CEPROPVAL*)buffer;

			rapi_database_trace("buffer = %p", buffer);

			for (i = 0; i < prop_id_count; i++)
			{
				rapi_database_trace("propval[%i].propid = %08x", i, propval[i].propid);

				switch (propval[i].propid & 0xffff)
				{
					/* XXX: we can get problems here on 64-bit platforms */

					case CEVT_BLOB:

						rapi_database_trace("propval[%i].val.blob.dwCount = %08x",
								i, propval[i].val.blob.dwCount);
						rapi_database_trace("propval[%i].val.blob.lpb = %08x",
								i, propval[i].val.blob.lpb);

						propval[i].val.blob.lpb = (LPBYTE)
							(buffer + letoh32((uint32_t)propval[i].val.blob.lpb));
						rapi_database_trace("blob=%s",(char*)propval[i].val.blob.lpb);
						break;

					case CEVT_LPWSTR:
						rapi_database_trace("string offset = %p", propval[i].val.lpwstr);
						propval[i].val.lpwstr = (LPWSTR)
							(buffer + letoh32((uint32_t)propval[i].val.lpwstr));
						rapi_database_trace_wstr(propval[i].val.lpwstr);
						rapi_database_trace("propval[i].val.lpwstr = %p", propval[i].val.lpwstr);
						break;

					/* TODO: convert endians on other data types! */
				}
			}
		}
		else
		{
 			rapi_database_error("failed to allocate 0x%08x bytes", size);
		}
	}

	return return_value;

fail:
 	rapi_database_error("failed");
	CeRapiFreeBuffer(buffer);
	return 0;
}/*}}}*/

#define CONVERT_32(value) (propval->val.value = htole32(propval->val.value))

#define ALIGN(value) ( value = ((value)+3) & ~3 )

static bool PreparePropValForWriting(unsigned* data_offset, CEPROPVAL* propval)/*{{{*/
{
	bool success = true;

	rapi_database_trace("Preparing value of type %i", propval->propid & 0xffff);

	switch (propval->propid & 0xffff)
	{
		case CEVT_BLOB:
			propval->val.blob.lpb = (LPBYTE)htole32(*data_offset);

			rapi_database_trace("propval->val.blob.dwCount = %08x",
					propval->val.blob.dwCount);
			rapi_database_trace("propval->val.blob.lpb = %08x",
					propval->val.blob.lpb);

			*data_offset += propval->val.blob.dwCount;
			CONVERT_32(blob.dwCount);
			break;

		case CEVT_BOOL:
			CONVERT_32(boolVal);
			break;

		case CEVT_FILETIME:
			CONVERT_32(filetime.dwLowDateTime);
			CONVERT_32(filetime.dwHighDateTime);
			break;

		case CEVT_I2:
		case CEVT_I4:
			CONVERT_32(iVal);
			break;

		case CEVT_LPWSTR:
      if (propval->val.lpwstr)
			{
				size_t size = sizeof(WCHAR) * (wstr_strlen(propval->val.lpwstr) + 1);
				rapi_database_trace_wstr(propval->val.lpwstr);
				propval->val.lpwstr = (LPWSTR)htole32(*data_offset);
				rapi_database_trace("String offset: %p", propval->val.lpwstr);
				*data_offset += size;
			}
      else
      {
				rapi_database_error("String property value is NULL");
        success = false;
      }
			break;

		case CEVT_UI2:
		case CEVT_UI4:
			CONVERT_32(uiVal);
			break;

		default:
			rapi_database_error("Don't know how to prepare value type 0x%04x", propval->propid & 0xffff);
			success = false;
			break;
	}

	return success;
}/*}}}*/

CEOID _CeWriteRecordProps( HANDLE hDbase, CEOID oidRecord, WORD cPropID, CEPROPVAL* rgPropVal)/*{{{*/
{
	RapiContext* context = rapi_context_current();
	CEOID return_value = 0;

	CEPROPVAL* values = NULL;
	unsigned data_offset = 0;
	unsigned array_size = 0;
	unsigned total_size = 0;
	unsigned i;

	/*
	 *	Format of the CeWriteRecordProps packet - primitives are encoded in the CEPROPVAL structures, lpwstr and blob properties are
	 *	attached to the end of the buffer and referenced by offset pointers
	 *
	 *		long hDBase | long oidRecord | long cPropID | long datalen (of following data) | n * CEPROPVAL | char[] data
	 *
	 */

	rapi_database_trace("begin");
	rapi_context_begin_command(context, 0x11);
	rapi_buffer_write_uint32(context->send_buffer, hDbase);
	rapi_buffer_write_uint32(context->send_buffer, oidRecord);
	rapi_buffer_write_uint16(context->send_buffer, cPropID);

	/*
	 * we have to go through the rgPropVals array three times:
	 *		1. to determine the size of the whole buffer, including data
	 *		2. to write out the CEPROPVAL array
	 *		3. to write the data segment
	 */

	/* calculate the length of the whole buffer, including the data segment at the end */

	total_size = array_size = cPropID * sizeof(CEPROPVAL); /* length of all cepropvals */

	for ( i = 0; i < cPropID; i++ )
	{
		switch ( ( rgPropVal[i].propid ) & 0xFFFF )
		{
			case CEVT_BLOB:
				total_size += rgPropVal[i].val.blob.dwCount;
				break;
			case CEVT_LPWSTR:
				total_size += sizeof(WCHAR) * ( wstr_strlen( rgPropVal[i].val.lpwstr ) + 1 );
				break;

			default:
				break;
		}

		ALIGN(total_size);
	}

	rapi_database_trace("Array size = %i", array_size);
	rapi_database_trace("Total size = %i", total_size);

	rapi_buffer_write_uint32(context->send_buffer, total_size);

	/*
	 * 2. write out the CEPROPVAL array
	 *
	 * Make a copy of the buffer, modify the copy, and write the copy
	 */

	values = calloc(1, total_size);
	memcpy(values, rgPropVal, array_size);

	;

	for (i = 0, data_offset = array_size; i < cPropID; i++)
	{
		if (!PreparePropValForWriting(&data_offset, &values[i]))
		{
			rapi_database_error("PreparePropValForWriting failed");
			goto exit;
		}

		ALIGN(data_offset);
	}

	if (data_offset != total_size)
	{
		rapi_database_error("Data offset is %08x but should be %08x", data_offset, total_size);
		goto exit;
	}

	/*
	 * 3. write the data segment
	 */

	for (i = 0, data_offset = array_size; i < cPropID; i++)
	{
		size_t size = 0;

		switch ( rgPropVal[i].propid & 0xFFFF )
		{
			case CEVT_BLOB:
				size = rgPropVal[i].val.blob.dwCount;
				memcpy((LPBYTE)values + data_offset, rgPropVal[i].val.blob.lpb, size);
				break;

			case CEVT_LPWSTR:
				size = sizeof(WCHAR) * (wstr_strlen(rgPropVal[i].val.lpwstr) + 1);
				memcpy((LPBYTE)values + data_offset,
						rgPropVal[i].val.lpwstr, size);
				break;

			default:
				break;
		}

		data_offset += size;
		ALIGN(data_offset);
	}

	if (data_offset != total_size)
	{
		rapi_database_error("Data offset is %08x but should be %08x", data_offset, total_size);
		goto exit;
	}

	if (!rapi_buffer_write_data(context->send_buffer, values, total_size))
	{
		rapi_database_error("rapi_buffer_write_data failed");
		goto exit;
	}

	CeRapiFreeBuffer(values);
	values = NULL;

	if ( !rapi_context_call(context) )
	{
		rapi_database_error("rapi_context_call failed");
		goto fail;
	}

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->last_error) )
		goto fail;
	rapi_database_trace("context->last_error=0x%08x", context->last_error);

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &return_value) )
	{
		rapi_database_trace("failed to read return value");
		goto fail;
	}
	rapi_database_trace("return_value=0x%08x", return_value);

exit:
	CeRapiFreeBuffer(values);
	return return_value;

fail:
	return_value = 0;
	goto exit;
}/*}}}*/

#endif /* SIZEOF_VOID_P == 4 */

CEOID _CeSeekDatabase(/*{{{*/
		HANDLE hDatabase,
		DWORD dwSeekType,
		DWORD dwValue,
		LPDWORD lpdwIndex)
{
	RapiContext* context = rapi_context_current();
	CEOID return_value = 0;

	rapi_database_trace("begin");

	if (!lpdwIndex)
	{
		rapi_database_error("lpdwIndex is NULL");
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
			rapi_database_error("Seek type by value is not yet supported", dwSeekType);
			goto fail;

		default:
			rapi_buffer_write_uint32(context->send_buffer, dwValue);
			break;
	}

	if ( !rapi_context_call(context) )
		goto fail;

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->last_error) )
		goto fail;
	rapi_database_trace("context->last_error=0x%08x", context->last_error);

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &return_value) )
		goto fail;

	if ( !rapi_buffer_read_uint32(context->recv_buffer, lpdwIndex) )
		goto fail;

	return return_value;

fail:
	return 0;
}/*}}}*/

BOOL _CeDeleteRecord(/*{{{*/
    HANDLE hDatabase,
    CEOID oidRecord)
{
  RapiContext* context = rapi_context_current();
  BOOL return_value = false;

  rapi_database_trace("begin");

  rapi_context_begin_command(context, 0x12);
	rapi_buffer_write_uint32(context->send_buffer, hDatabase);
  rapi_buffer_write_uint32(context->send_buffer, oidRecord);

  if ( !rapi_context_call(context) )
    goto exit;

  if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->last_error) )
    goto exit;
  rapi_database_trace("context->last_error=0x%08x", context->last_error);

  if ( !rapi_buffer_read_uint32(context->recv_buffer, &return_value) )
    goto exit;

exit:
  return return_value;
}/*}}}*/

BOOL _CeSetDatabaseInfo(
    CEOID oidDbase,
    CEDBASEINFO* pNewInfo)
{
	RapiContext* context = rapi_context_current();
  BOOL return_value = false;

	rapi_database_trace("begin");

	rapi_context_begin_command(context, 0x14);
	rapi_buffer_write_uint32(context->send_buffer, oidDbase);
  assert(sizeof(CEDBASEINFO) == 0x78);
	rapi_buffer_write_data(context->send_buffer, pNewInfo, sizeof(CEDBASEINFO));

	if ( !rapi_context_call(context) )
		goto exit;

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->last_error) )
		goto exit;
	rapi_database_trace("context->last_error=0x%08x", context->last_error);

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &return_value) )
		goto exit;

exit:
	return return_value;
}


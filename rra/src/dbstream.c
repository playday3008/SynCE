/* $Id$ */
#include <rapi.h>
#include <synce_log.h>

static uint16_t dbstream_read16(uint8_t** stream)
{
	uint16_t value = letoh16(*(uint16_t*)*stream);
	*stream += sizeof(uint16_t);
	return value;
}

static uint32_t dbstream_read32(uint8_t** stream)
{
	uint32_t value = letoh32(*(uint32_t*)*stream);
	*stream += sizeof(uint32_t);
	return value;
}

static WCHAR* dbstream_read_string(uint8_t** stream)
{
	WCHAR* str = (WCHAR*)*stream;
	/*synce_trace_wstr(str);*/
	*stream += sizeof(WCHAR) * (wstrlen(str) + 1);
	return str;
}

/*
 * Code to convert a database stream to an array of CEPROPVAL structures.
 *
 * No memory will be allocated; strings and BLOBs will point into the stream.
 */

bool dbstream_to_propvals(
		uint8_t* stream,
		uint32_t count,
		CEPROPVAL* propval)
{
	int i;

	memset(propval, 0, count * sizeof(CEPROPVAL));

	for (i = 0; i < count; i++)
	{
		propval[i].propid = dbstream_read32(&stream);

		switch (propval[i].propid & 0xffff)
		{
			case CEVT_I2:
				propval[i].val.iVal = (int16_t)dbstream_read16(&stream);
				break;
			
			case CEVT_I4:
				propval[i].val.iVal = (int32_t)dbstream_read32(&stream);
				break;

			case CEVT_UI2:
				propval[i].val.iVal = dbstream_read16(&stream);
				break;
			
			case CEVT_UI4:
				propval[i].val.iVal = dbstream_read32(&stream);
				break;

#if 0
				/* what size? */
			case CEVT_BOOL: 
				printf("0x%08x/%u",  propval[i].val.boolVal, propval[i].val.boolVal); break;
#endif
			case CEVT_LPWSTR:
				propval[i].val.lpwstr = dbstream_read_string(&stream);
				break;

			case CEVT_FILETIME:
				propval[i].val.filetime.dwLowDateTime   = dbstream_read32(&stream);
				propval[i].val.filetime.dwHighDateTime  = dbstream_read32(&stream);
				break;

			case CEVT_BLOB:
				propval[i].val.blob.dwCount  = dbstream_read32(&stream);
				propval[i].val.blob.lpb = stream;
				stream += propval[i].val.blob.dwCount;
				break;

			default:
				synce_error("unknown data type for propid %08x", propval[i].propid);
				return false;
		}
	}

	return true;
}


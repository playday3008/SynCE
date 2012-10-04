/* $Id$ */
#include <rapi.h>
#include <synce_log.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

static uint16_t dbstream_read16(const uint8_t** stream)
{
  uint16_t value = letoh16(*(uint16_t*)*stream);
  *stream += sizeof(uint16_t);
  return value;
}

static uint32_t dbstream_read32(const uint8_t** stream)
{
  uint32_t value = letoh32(*(uint32_t*)*stream);
  *stream += sizeof(uint32_t);
  return value;
}

static WCHAR* dbstream_read_string(const uint8_t** stream)
{
  WCHAR* str = (WCHAR*)*stream;
  /*synce_trace_wstr(str);*/
  *stream += sizeof(WCHAR) * (wstrlen(str) + 1);
  return str;
}

/*
 * Code to convert a database stream to an array of CEPROPVAL structures.
 *
 * No memory will be allocated; strings and BLOBs will point into the input stream.
 */

bool dbstream_to_propvals(/*{{{*/
		const uint8_t* stream,
		uint32_t count,
		CEPROPVAL* propval)
{
  unsigned i;

  memset(propval, 0, count * sizeof(CEPROPVAL));

  for (i = 0; i < count; i++)
  {
    propval[i].propid = dbstream_read32(&stream);

    if (propval[i].propid & 0x400) /* CEVT_FLAG_EMPTY */
    {
      /* this flags seems to suggest an empty field */
      continue;
    }

    switch (propval[i].propid & 0xffff)
    {
    case CEVT_I2:
      propval[i].val.iVal = (int16_t)dbstream_read16(&stream);
      break;

    case CEVT_I4:
      propval[i].val.lVal = (int32_t)dbstream_read32(&stream);
      break;

    case CEVT_UI2:
      propval[i].val.uiVal = dbstream_read16(&stream);
      break;

    case CEVT_UI4:
      propval[i].val.ulVal = dbstream_read32(&stream);
      break;

#if 0
      /* what size? */
    case CEVT_BOOL:
      synce_debug("CEVT_BOOL: unknown size");
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
      propval[i].val.blob.lpb = (void*)stream;
      stream += propval[i].val.blob.dwCount;
      break;

    default:
      synce_error("unknown data type for propid %08x", propval[i].propid);
      return false;
    }
  }

  return true;
}/*}}}*/

static void dbstream_write16(uint8_t** stream, uint16_t value)
{
  *(uint16_t*)*stream = htole16(value);
  *stream += sizeof(uint16_t);
}

static void dbstream_write32(uint8_t** stream, uint32_t value)
{
  *(uint32_t*)*stream = htole32(value);
  *stream += sizeof(uint32_t);
}

static void dbstream_write_string(uint8_t** stream, WCHAR* str)
{
  if (stream && *stream && str)
  {
    size_t size = sizeof(WCHAR) * (wstrlen(str) + 1);
    memcpy(*stream, str, size);
    *stream += size;
  }
  else
    synce_error("One or more parameters are NULL!");
}


/*
 * Code to convert an array of CEPROPVAL structures to a database stream.
 */

bool dbstream_from_propvals(/*{{{*/
		CEPROPVAL* propval,
		uint32_t count,
		uint8_t** result,
		size_t* result_size)
{
  bool success = false;
  unsigned i;
  uint8_t* data = NULL;
  uint8_t* stream = NULL;
  size_t size = 8;

  /*
   * Find out stream size
   */
  for (i = 0; i < count; i++)/*{{{*/
  {
    size += 4;

    if (propval[i].propid & 0x400) /* CEVT_FLAG_EMPTY */
    {
      /* this flags seems to suggest an empty field */
      continue;
    }

    switch (propval[i].propid & 0xffff)
    {
    case CEVT_I2:
    case CEVT_UI2:
      size += 2;
      break;

    case CEVT_I4:
    case CEVT_UI4:
      size += 4;
      break;

#if 0
      /* what size? */
    case CEVT_BOOL:
      synce_debug("CEVT_BOOL: unknown size");
#endif
    case CEVT_LPWSTR:
      size += (wstrlen(propval[i].val.lpwstr) + 1) * 2;
      break;

    case CEVT_FILETIME:
      size += 8;
      break;

    case CEVT_BLOB:
      size += 4 + propval[i].val.blob.dwCount;
      break;

    default:
      synce_error("unknown data type for propid %08x", propval[i].propid);
      goto exit;
    }

  }/*}}}*/

  stream = data = calloc(1, size);

  dbstream_write32(&stream, count);
  dbstream_write32(&stream, 0);

  /*
   * Create stream
   */
  for (i = 0; i < count; i++)/*{{{*/
  {
    dbstream_write32(&stream, propval[i].propid);

    if (propval[i].propid & 0x400) /* CEVT_FLAG_EMPTY */
    {
      /* this flags seems to suggest an empty field */
      continue;
    }

    switch (propval[i].propid & 0xffff)
    {
    case CEVT_I2:
      dbstream_write16(&stream, propval[i].val.iVal);
      break;

    case CEVT_I4:
      dbstream_write32(&stream, propval[i].val.lVal);
      break;

    case CEVT_UI2:
      dbstream_write16(&stream, propval[i].val.uiVal);
      break;

    case CEVT_UI4:
      dbstream_write32(&stream, propval[i].val.ulVal);
      break;

#if 0
      /* what size? */
    case CEVT_BOOL:
      synce_debug("CEVT_BOOL: unknown size");
      printf("0x%08x/%u",  propval[i].val.boolVal, propval[i].val.boolVal); break;
#endif
    case CEVT_LPWSTR:
      if (propval[i].val.lpwstr)
	dbstream_write_string(&stream, propval[i].val.lpwstr);
      else
	synce_warning("String for propid %08x is NULL!", propval[i].propid);
      break;

    case CEVT_FILETIME:
      dbstream_write32(&stream, propval[i].val.filetime.dwLowDateTime);
      dbstream_write32(&stream, propval[i].val.filetime.dwHighDateTime);
      break;

    case CEVT_BLOB:
      assert(propval[i].val.blob.lpb);
      dbstream_write32(&stream, propval[i].val.blob.dwCount);
      memcpy(stream, propval[i].val.blob.lpb, propval[i].val.blob.dwCount);
      stream += propval[i].val.blob.dwCount;
      break;

    default:
      synce_error("unknown data type for propid %08x", propval[i].propid);
      goto exit;
    }
  }/*}}}*/

  if (stream != (data + size))
  {
    synce_error("Unexpected stream size, your memory may have become corrupted.");
    goto exit;
  }

  success = true;

exit:
  if (success)
  {
    if (result)
      *result = data;

    if (result_size)
      *result_size = size;
  }
  else
  {
    if (data)
      free(data);

    *result = NULL;
    *result_size = 0;
  }
		
  return success;
}/*}}}*/


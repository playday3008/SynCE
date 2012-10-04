/* $Id$ */
#include "strbuf.h"
#include <stdlib.h>
#include <string.h>
#include <synce_log.h>

static void strbuf_enlarge(StrBuf *strbuf, size_t size)
{
  if (strbuf->buffer_size < size)
  {
    size_t new_size = strbuf->buffer_size ? strbuf->buffer_size : 2;
    
    while (new_size < size)
      new_size <<= 1;

    strbuf->buffer = realloc(strbuf->buffer, new_size);
    strbuf->buffer_size = new_size;
  }
}

StrBuf* strbuf_new (const char *init)
{
  StrBuf* result = (StrBuf*)malloc(sizeof(StrBuf));
  memset(result, 0, sizeof(StrBuf));
  strbuf_append(result, init);
  return result;
}
  
void strbuf_destroy(StrBuf *strbuf, bool free_contents)
{
  if (free_contents)
    free(strbuf->buffer);

  free(strbuf);
}

StrBuf* strbuf_append (StrBuf *strbuf, const char* str)
{
  int length;

  if (!str)
    return strbuf;
 
  length = strlen(str);
  strbuf_enlarge(strbuf, strbuf->length + length + 1);
  memcpy(strbuf->buffer + strbuf->length, str, length + 1);
  strbuf->length += length;
  return strbuf;
}

StrBuf* strbuf_append_wstr(StrBuf* strbuf, WCHAR* wstr)
{
  if (wstr)
  {
    char* ascii_str = wstr_to_ascii(wstr);
    if (!ascii_str) {
      synce_warning("Failed to convert UCS2 string to ascii");
      return strbuf;
    }

    strbuf_append(strbuf, ascii_str);
    wstr_free_string(ascii_str);
  }

  return strbuf;
}

StrBuf* strbuf_append_c (StrBuf *strbuf, int c)
{
  strbuf_enlarge(strbuf, strbuf->length + 2);
  strbuf->buffer[strbuf->length++] = c;
  strbuf->buffer[strbuf->length] = '\0';
  return strbuf;
}

StrBuf* strbuf_append_crlf (StrBuf *strbuf)
{
  strbuf_enlarge(strbuf, strbuf->length + 3);
  strbuf->buffer[strbuf->length++] = '\r';
  strbuf->buffer[strbuf->length++] = '\n';
  strbuf->buffer[strbuf->length] = '\0';
  return strbuf;
}

#if 0
StrBuf* strbuf_printf(StrBuf *strbuf, const char* format, ...)
{
  va_list ap;
  
  va_start(ap, format);
  strbuf_vprintf(strbuf, format, ap);
  va_end(ap);
}

StrBuf* strbuf_vprintf(StrBuf *strbuf, const char* format, va_list ap);
{
  size_t bytes_left = 0;
  size_t increase = 64;
  int bytes_used = 0;
  
  for (;;)
  {
    strbuf_enlarge(strbuf, strbuf->length + increase);
    bytes_left = strbuf->buffer_size - strbuf->length;

    bytes_used = vsnprintf(strbuf->buffer + strbuf->length,
        bytes_left, format, ap);

    if (bytes_used > -1 && bytes_used < size)
      break;

    /* see the printf man page */
    if (bytes_used > -1)            /* glibc 2.1 */
      increase = bytes_used + 1;
    else                            /* glibc 2.0 */
      increase *= 2;
  };
  
  bytes->length += bytes_used;

  return strbuf;
}
#endif

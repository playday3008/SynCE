#ifndef __strbuf_h__
#define __strbuf_h__

#include <stdarg.h>
#include <sys/types.h>
#include <synce.h>

struct _StrBuf
{
  char *buffer;
  int length;
  size_t buffer_size;
};

typedef struct _StrBuf StrBuf;


StrBuf* strbuf_new (const char *init);
void strbuf_destroy (StrBuf *strbuf, bool free_contents);
StrBuf* strbuf_append (StrBuf *strbuf, const char* str);
StrBuf* strbuf_append_wstr(StrBuf* strbuf, WCHAR* wstr);
StrBuf* strbuf_append_c (StrBuf *strbuf, int c);
StrBuf* strbuf_append_crlf (StrBuf *strbuf);

#if 0
StrBuf* strbuf_printf(StrBuf *strbuf, const char* format, ...);
StrBuf* strbuf_vprintf(StrBuf *strbuf, const char* format, va_list ap);
#endif

#endif


#ifndef __strbuf_h__
#define __strbuf_h__

#include <string.h>
#include <stdbool.h>

struct _StrBuf
{
  char *buffer;
  int length;
  size_t buffer_size;
};

typedef struct _StrBuf StrBuf;


StrBuf* strbuf_new (const char *init);
void strbuf_free (StrBuf *strbuf, bool free_contents);
StrBuf* strbuf_append (StrBuf *strbuf, const char* str);
StrBuf* strbuf_append_c (StrBuf *strbuf, int c);

#endif


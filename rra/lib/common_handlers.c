/* $Id$ */
#define _GNU_SOURCE 1
#include "common_handlers.h"
#include "parser.h"
#include "generator.h"
#include "appointment_ids.h"
#include "task_ids.h"
#include "strbuf.h"
#include <rapi.h>
#include <synce_log.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "internal.h"
#include "rra_config.h"

#if HAVE_ICONV_H
#include <iconv.h>
#endif

#define INVALID_ICONV_HANDLE ((iconv_t)(-1))

#define CHARSET_ISO88591  "ISO_8859-1"
#define CHARSET_UTF8      "UTF-8"

#define STR_EQUAL(a,b)  (0 == strcasecmp(a,b))

static char* convert_string(const char* inbuf, const char* tocode, const char* fromcode)
{
  size_t length = strlen(inbuf);
  size_t inbytesleft = length, outbytesleft = length * 2;
  char* outbuf = malloc(outbytesleft + sizeof(char));
  char* outbuf_iterator = outbuf;
  ICONV_CONST char* inbuf_iterator = (ICONV_CONST char*)inbuf;
  size_t result;
  iconv_t cd = INVALID_ICONV_HANDLE;

  cd = iconv_open(tocode, fromcode);

  if (INVALID_ICONV_HANDLE == cd)
  {
    synce_error("iconv_open failed");
    return NULL;
  }

  result = iconv(cd, &inbuf_iterator, &inbytesleft, &outbuf_iterator, &outbytesleft);
  iconv_close(cd);

  if ((size_t)-1 == result)
  {
		synce_error("iconv failed: inbytesleft=%i, outbytesleft=%i, inbuf=\"%s\"", 
				inbytesleft, outbytesleft, inbuf);
		free(outbuf);
		return NULL;
  }

  *outbuf_iterator = '\0';

  return outbuf;
}

static char* convert_to_utf8(const char* inbuf)
{
  char* utf8 = convert_string(inbuf, CHARSET_UTF8, CHARSET_ISO88591);

  if (utf8)
  {
    char* result = NULL;
    unsigned char* q;
    StrBuf* euro_fix = strbuf_new(NULL);

    if (!utf8)
      return NULL;

    for (q = (unsigned char*)utf8; *q != '\0'; q++)
    {
      /* Special treatment of the euro symbol */
      if (*q == 0x80)
      {
        synce_warning("Euro symbol found, using workaround.");
#if 0
        strbuf_append_c(euro_fix, 0xe2);
        strbuf_append_c(euro_fix, 0x82);
        strbuf_append_c(euro_fix, 0xac);
#else
        strbuf_append(euro_fix, "[EURO]");
#endif
      }
      else
        strbuf_append_c(euro_fix, *q);
    }

    result = strdup(euro_fix->buffer);

    free(utf8);
    strbuf_destroy(euro_fix, true);
    return result;
  }
  else
    return NULL;
}

static char* convert_from_utf8(const char* source)
{
  char* result = NULL;
  const unsigned char* q;
  StrBuf* euro_fix = strbuf_new(NULL);

  if (!source)
    return NULL;

  for (q = (const unsigned char*)source; *q != '\0'; q++)
  {
    /* Special treatment of the euro symbol */
    if (q[0] == 0xe2 && q[1] == 0x82 && q[2] == 0xac)
    {
      synce_warning("Euro symbol found, using workaround.");
      strbuf_append(euro_fix, "[EURO]");
      q += 2;
    }
    else
      strbuf_append_c(euro_fix, *q);
  }

  result = convert_string(euro_fix->buffer, CHARSET_ISO88591, CHARSET_UTF8);

  strbuf_destroy(euro_fix, true);
  return result;
}

/*
   Categories
 */

bool on_mdir_line_categories(Parser* p, mdir_line* line, void* cookie)
{
  if (line)
  {
    return parser_add_string_from_line(p, ID_TASK_CATEGORIES, line);
  }
  else
    return false;
}

bool on_propval_categories(Generator* g, CEPROPVAL* propval, void* cookie)
{
  return generator_add_simple_propval(g, "CATEGORIES", propval);
}


/*
   Location
*/

bool on_mdir_line_location(Parser* p, mdir_line* line, void* cookie)
{
  if (line)
    return parser_add_string_from_line(p, ID_LOCATION, line);
  else
    return parser_add_string(p, ID_LOCATION, "");
}

bool on_propval_location(Generator* g, CEPROPVAL* propval, void* cookie)
{
  generator_add_simple_propval(g, "LOCATION", propval);
  return true;
}

/*
   Notes / Description
*/

bool on_mdir_line_description(Parser* p, mdir_line* line, void* cookie)
{
  bool success = false;
  StrBuf* note = strbuf_new(NULL);

  if (line && line->values)
  {
    char *q;
    char* source = NULL;

    if (parser_utf8(p))
    {
      source = convert_from_utf8(line->values[0]);
      if (!source)
      {
        synce_error("Failed to convert string from UTF-8");
        goto exit;
      }
    }
    else
      source = line->values[0];


    /* convert LF to CRLF */
    for (q = source; *q != '\0'; q++)
    {
      if (*q == '\n')
        strbuf_append_crlf(note);
      else
        strbuf_append_c(note, *q);
    }

    success = parser_add_blob(p, ID_NOTES, note->buffer, note->length);

    if (parser_utf8(p))
      free(source);
  }

exit:
  strbuf_destroy(note, true);
  return success;
}

static const char pwi_signature[] = "{\\pwi";

bool blob_is_pwi(CEBLOB* blob)
{
  return 
    blob->dwCount >= 5 &&
    0 == strncmp(pwi_signature, (const char*)blob->lpb, strlen(pwi_signature));
}

bool on_propval_notes(Generator* g, CEPROPVAL* propval, void* cookie)/*{{{*/
{
  assert(CEVT_BLOB == (propval->propid & 0xffff));

  if (propval->val.blob.dwCount)
  {
    if (blob_is_pwi(&propval->val.blob))
    {
      synce_warning("PocketWord Ink format for notes is not yet supported");
    }
    else
    {
      char* tmp = malloc(propval->val.blob.dwCount + 1);
      memcpy(tmp, propval->val.blob.lpb, propval->val.blob.dwCount);
      tmp[propval->val.blob.dwCount] = '\0';

      if (generator_utf8(g))
      {
        char* utf8 = convert_to_utf8(tmp);
        free(tmp);
        if (!utf8)
        {
          synce_error("Failed to convert string to UTF-8");
          return false;
        }
        tmp = utf8;
      }
      
      generator_add_simple(g, "DESCRIPTION", tmp);
      free(tmp);
    }
  }
  
  return true;
}/*}}}*/

/* Sensitivty / Class */

bool on_propval_sensitivity(Generator* g, CEPROPVAL* propval, void* cookie)/*{{{*/
{
  switch (propval->val.iVal)
  {
    case SENSITIVITY_PUBLIC:
      generator_add_simple(g, "CLASS", "PUBLIC");
      break;
      
    case SENSITIVITY_PRIVATE:
      generator_add_simple(g, "CLASS", "PRIVATE");
      break;

    default:
      synce_warning("Unknown sensitivity: %04x", propval->val.iVal);
      break;
  }
  return true;
}/*}}}*/

bool on_mdir_line_class(Parser* p, mdir_line* line, void* cookie)/*{{{*/
{
  if (line)
  {
    if (STR_EQUAL(line->values[0], "PUBLIC"))
      parser_add_int16(p, ID_SENSITIVITY, SENSITIVITY_PUBLIC);
    else if (
        STR_EQUAL(line->values[0], "PRIVATE") ||
        STR_EQUAL(line->values[0], "CONFIDENTIAL"))
      parser_add_int16(p, ID_SENSITIVITY, SENSITIVITY_PRIVATE);
    else
      synce_warning("Unknown value for CLASS: '%s'", line->values[0]);
    return true;
  }
  else
    return false;
}/*}}}*/


/* 
   Subject / Summary
*/

bool on_mdir_line_summary(Parser* p, mdir_line* line, void* cookie)
{
  if (line)
    return parser_add_string_from_line(p, ID_SUBJECT, line);
  else
    return false;
}

bool on_propval_subject(Generator* g, CEPROPVAL* propval, void* cookie)
{
  generator_add_simple_propval(g, "SUMMARY", propval);
  return true;
}



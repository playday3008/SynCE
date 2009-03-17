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

#define MINUTES_PER_DAY (24*60)

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

char* convert_to_utf8(const char* inbuf, const char* codepage)
{
  char* utf8 = convert_string(inbuf, CHARSET_UTF8, codepage);

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
      if (q[0] == 0xc2 && q[1] == 0x80)
      {
        strbuf_append_c(euro_fix, 0xe2);
        strbuf_append_c(euro_fix, 0x82);
        strbuf_append_c(euro_fix, 0xac);

        q++;
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

char* convert_from_utf8(const char* source, const char* codepage)
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
      strbuf_append_c(euro_fix, 0xc2);
      strbuf_append_c(euro_fix, 0x80);

      q += 2;
    }
    else
      strbuf_append_c(euro_fix, *q);
  }

  result = convert_string(euro_fix->buffer, codepage, CHARSET_UTF8);

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
  int i, j;

  /*
   * Remove the space character after the comma separator
   */
  for (i = 0, j = 0; propval->val.lpwstr[i]; i++)
    if (i && propval->val.lpwstr[i] == 0x20 &&
        propval->val.lpwstr[i - 1] == 0x2c)
      j++;
    else
      if (j)
        propval->val.lpwstr[i - j] = propval->val.lpwstr[i];
  for (; j > 0; j--)
    propval->val.lpwstr[i - j] = 0;

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

bool process_mdir_line_description(Parser* p, mdir_line* line, void* cookie, const char *codepage)
{
  bool success = false;
  StrBuf* note = strbuf_new(NULL);

  if (line && line->values)
  {
    char *q;
    char* source = NULL;

    if (parser_utf8(p))
    {
      source = convert_from_utf8(line->values[0], codepage);
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

    /* Windows CE require that NOTE is pair
     * if not we add a "End of text" character (0x3)
     * at end of NOTE before send it to pda.
     * We remove that character when we receive it
     * from pda.
     */

    if (note->length % 2)
    {
      strbuf_append_c(note, 0x3);
    }

    success = parser_add_blob(
        p, 
        ID_NOTES, 
        (const uint8_t*)note->buffer, 
        note->length);

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

bool process_propval_notes(Generator* g, CEPROPVAL* propval, void* cookie, const char *codepage)/*{{{*/
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
        char* utf8 = convert_to_utf8(tmp, codepage);
        free(tmp);
        if (!utf8)
        {
          synce_error("Failed to convert string to UTF-8");
          return false;
        }
        tmp = utf8;
      }

      /* Windows CE require that NOTE is pair
       * if not we add a "End of text" character (0x3)
       * at end of NOTE before send it to pda.
       * We remove that character when we receive it
       * from pda.
       */

      if (tmp[strlen(tmp) - 1] == 0x3)
        tmp[strlen(tmp) - 1] = 0x0;
      
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
  else {
    /* Task require a subject */
    return parser_add_string(p, ID_SUBJECT, "<No subject>");
  }
}

bool on_propval_subject(Generator* g, CEPROPVAL* propval, void* cookie)
{
  generator_add_simple_propval(g, "SUMMARY", propval);
  return true;
}

/*
    VAlarm
*/
void to_propval_trigger(Parser* parser, mdir_line* line, uint8_t related_support)
{
  int enable = 0;
  int duration = 0;

  char** data_type = mdir_get_param_values(line, "VALUE");
  char** related   = mdir_get_param_values(line, "RELATED");

  if (!line)
    goto exit;

  /* data type must be DURATION */
  if (data_type && data_type[0])
  {
    if (STR_EQUAL(data_type[0], "DATE-TIME"))
    {
      synce_warning("Absolute date/time for alarm is not supported");
      goto exit;
    }
    if (!STR_EQUAL(data_type[0], "DURATION"))
    {
      synce_warning("Unknown TRIGGER data type: '%s'", data_type[0]);
    goto exit;
    }
  }

  /* check related is supported */
  if ((related && related[0]) &&
      ((STR_EQUAL(related[0], "START") &&
       (related_support != REMINDER_RELATED_START)) ||
       (STR_EQUAL(related[0], "END") &&
       (related_support != REMINDER_RELATED_END))))
  {
    synce_warning("Alarms related are not supported");
    goto exit;
  }

  if (parser_duration_to_seconds(line->values[0], &duration) && duration <= 0)
  {
    enable = 1;
    duration = -duration / 60;
  }
  else
    duration = 0;

exit:

  parser_add_int16 (parser, ID_REMINDER_ENABLED, enable);
  parser_add_int32 (parser, ID_REMINDER_MINUTES_BEFORE_START, duration);
  parser_add_int32 (parser, ID_REMINDER_OPTIONS, REMINDER_LED|REMINDER_DIALOG|REMINDER_SOUND);
  parser_add_string(parser, ID_REMINDER_SOUND_FILE, "Alarm1.wav");
}

void to_icalendar_trigger(Generator* generator, CEPROPVAL* reminder_enabled, CEPROPVAL* reminder_minutes, uint8_t related)
{
  if (reminder_enabled && reminder_minutes && reminder_enabled->val.iVal)
  {
    char buffer[32];

    generator_add_simple(generator, "BEGIN", "VALARM");

    /* XXX: maybe this should correspond to ID_REMINDER_OPTIONS? */
    generator_add_simple(generator, "ACTION", "DISPLAY");

    if (!(reminder_minutes->val.lVal % MINUTES_PER_DAY))
      snprintf(buffer, sizeof(buffer), "-P%liD", 
      (long)(reminder_minutes->val.lVal / MINUTES_PER_DAY));
    else if (!(reminder_minutes->val.lVal % 60))
      snprintf(buffer, sizeof(buffer), "-PT%liH", 
      (long)(reminder_minutes->val.lVal / 60));
    else
      snprintf(buffer, sizeof(buffer), "-PT%liM", 
      (long)(reminder_minutes->val.lVal));

    generator_begin_line         (generator, "TRIGGER");

    generator_begin_parameter    (generator, "VALUE");
    generator_add_parameter_value(generator, "DURATION");
    generator_end_parameter      (generator);

    generator_begin_parameter    (generator, "RELATED");
    switch (related) 
    {
    case REMINDER_RELATED_END:
      generator_add_parameter_value(generator, "END");
      break;
    default:
      generator_add_parameter_value(generator, "START");
    }
    generator_end_parameter      (generator);

    generator_add_value          (generator, buffer);
    generator_end_line           (generator);
    generator_add_simple(generator, "END", "VALARM");
  }
};

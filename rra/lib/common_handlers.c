/* $Id$ */
#define _GNU_SOURCE 1
#include "common_handlers.h"
#include "parser.h"
#include "generator.h"
#include "appointment_ids.h"
#include "task_ids.h"
#include <rapi.h>
#include <synce_log.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "internal.h"

#define STR_EQUAL(a,b)  (0 == strcasecmp(a,b))


/*
   Location
*/

bool on_mdir_line_location(Parser* p, mdir_line* line, void* cookie)
{
  return parser_add_string_from_line(p, ID_LOCATION, line);
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
  assert(line->values);
  /* TODO: convert from utf-8 */
  /* TODO: convert LF to CRLF */
  return parser_add_blob(p, ID_NOTES, line->values[0], strlen(line->values[0]));
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
      /* TODO: convert to utf-8 */
      char* tmp = strndup((const char*)
          propval->val.blob.lpb, 
          propval->val.blob.dwCount);
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
  if (STR_EQUAL(line->values[0], "PUBLIC"))
    parser_add_int16(p, ID_SENSITIVITY, SENSITIVITY_PUBLIC);
  else if (
      STR_EQUAL(line->values[0], "PRIVATE") ||
      STR_EQUAL(line->values[0], "CONFIDENTIAL"))
    parser_add_int16(p, ID_SENSITIVITY, SENSITIVITY_PRIVATE);
  else
    synce_warning("Unknown value for CLASS: '%s'", line->values[0]);
  return true;
}/*}}}*/


/* 
   Subject / Summary
*/

bool on_mdir_line_summary(Parser* p, mdir_line* line, void* cookie)
{
  return parser_add_string_from_line(p, ID_SUBJECT, line);
}

bool on_propval_subject(Generator* g, CEPROPVAL* propval, void* cookie)
{
  generator_add_simple_propval(g, "SUMMARY", propval);
  return true;
}



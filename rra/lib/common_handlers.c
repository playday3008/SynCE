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

bool str_is_print(CEBLOB* blob)/*{{{*/
{
  unsigned i;
  
  for (i = 0; i < blob->dwCount; i++)
  {
    switch (blob->lpb[i])
    {
      case 0x0a: /* LF */
      case 0x0d: /* CR */
        break;

      default:
        if (!isprint(blob->lpb[i]))
          return false;
    }
  }

  return true;
}/*}}}*/

bool on_propval_notes(Generator* g, CEPROPVAL* propval, void* cookie)/*{{{*/
{
  assert(CEVT_BLOB == (propval->propid & 0xffff));

  if (propval->val.blob.dwCount)
  {
    if (str_is_print(&propval->val.blob))
    {
      char* tmp = strndup((const char*)
          propval->val.blob.lpb, 
          propval->val.blob.dwCount);
      generator_add_simple(g, "DESCRIPTION", tmp);
      free(tmp);
    }
    else
      synce_warning("Note format not yet supported");
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



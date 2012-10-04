/* $Id$ */
#ifndef __common_handlers_h__
#define __common_handlers_h__

#include <inttypes.h>
#include <stdbool.h>
#include <libmimedir.h>
#include <rapi.h>

struct _Generator;
struct _Parser;

bool on_propval_categories (struct _Generator* g, CEPROPVAL* propval, void* cookie);
bool on_propval_location   (struct _Generator* g, CEPROPVAL* propval, void* cookie);
bool on_propval_sensitivity(struct _Generator* g, CEPROPVAL* propval, void* cookie);
bool on_propval_subject    (struct _Generator* g, CEPROPVAL* propval, void* cookie);

bool on_mdir_line_categories (struct _Parser* p, mdir_line* line, void* cookie);
bool on_mdir_line_class      (struct _Parser* p, mdir_line* line, void* cookie);
bool on_mdir_line_location   (struct _Parser* p, mdir_line* line, void* cookie);
bool on_mdir_line_summary    (struct _Parser* p, mdir_line* line, void* cookie);

bool process_propval_notes   (struct _Generator* g, CEPROPVAL* propval, void* cookie, const char *codepage);
bool process_mdir_line_description(struct _Parser* p, mdir_line* line, void* cookie, const char *codepage);

#define REMINDER_RELATED_START 0
#define REMINDER_RELATED_END   1

void to_propval_trigger(struct _Parser* parser, mdir_line* line, uint8_t related_support);

void to_icalendar_trigger(struct _Generator* generator, CEPROPVAL* reminder_enabled, CEPROPVAL* reminder_minutes, uint8_t related);

char* convert_to_utf8(const char* inbuf, const char* codepage);
char* convert_from_utf8(const char* source, const char* codepage);

#endif


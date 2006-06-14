/* $Id$ */
#ifndef __parser_h__
#define __parser_h__

#include <synce.h>
#include <libmimedir.h>
#include <time.h>

struct _RRA_Timezone;

#define PARSER_UTF8 1

typedef struct _Parser Parser;
typedef struct _ParserProperty ParserProperty;
typedef struct _ParserComponent ParserComponent;

typedef bool (*ParserPropertyFunc)(Parser* p, mdir_line* line, void* cookie);

/* ParserProperty functions */
ParserProperty* parser_property_new(const char* name, ParserPropertyFunc func);
void parser_property_destroy(ParserProperty* ct);

/* ParserComponent functions */
ParserComponent* parser_component_new(const char* name);
void parser_component_destroy(ParserComponent* self);
void parser_component_add_parser_component(ParserComponent* self, ParserComponent* ct);
void parser_component_add_parser_property (ParserComponent* self, ParserProperty*  pt);

/* helper functions */
bool parser_duration_to_seconds  (const char* duration, int* seconds);
bool parser_datetime_to_struct   (const char* datetime, struct tm* tm, bool* is_utc);
bool parser_datetime_to_unix_time(const char* datetime, time_t* unix_time, bool* is_utc);
void parser_filetime_to_datetime(const FILETIME* filetime, char* date,  unsigned size);
bool parser_filetime_to_unix_time(const FILETIME* filetime, time_t* unix_time);

Parser* parser_new(
    ParserComponent* base_parser_component, 
    int flags,
    struct _RRA_Timezone* tzi, 
    void* cookie);
void parser_destroy(Parser* self);
bool parser_utf8(Parser* self);
bool parser_set_mimedir(Parser* self, const char* mimedir);
bool parser_run(Parser* self);
void parser_call_unused_properties(Parser* self);
bool parser_get_result(Parser* self, uint8_t** result, size_t* result_size);

/* add database records */
bool parser_add_blob  (Parser* self, uint16_t id, const uint8_t* data, size_t data_size);
bool parser_add_int16 (Parser* self, uint16_t id, int16_t value);
bool parser_add_int32 (Parser* self, uint16_t id, int32_t value);
bool parser_add_string(Parser* self, uint16_t id, const char* str);
bool parser_add_time  (Parser* self, uint16_t id, time_t value);
bool parser_add_filetime(Parser* self, uint16_t id, FILETIME* filetime);

bool parser_add_string_from_line(Parser* self, uint16_t id, mdir_line* line);
bool parser_add_time_from_line  (Parser* self, uint16_t id, mdir_line* line);
bool parser_add_localdate_from_line(Parser* self, uint16_t index, mdir_line* line);

typedef enum 
{
  PARSER_TIME_FORMAT_UNKNOWN,
  PARSER_TIME_FORMAT_DATE_AND_TIME,
  PARSER_TIME_FORMAT_ONLY_DATE
} ParserTimeFormat;

ParserTimeFormat parser_get_time_format(mdir_line* line);

#endif 


/* $Id$ */
#ifndef __parser_h__
#define __parser_h__

#include <stdbool.h>
#include <stdint.h>
#include <libmimedir.h>
#include <time.h>

#define PARSER_UTF8 1

typedef struct _Parser Parser;
typedef struct _PropertyType PropertyType;
typedef struct _ComponentType ComponentType;

typedef bool (*PropertyFunc)(Parser* p, mdir_line* line, void* cookie);

/* PropertyType functions */
PropertyType* property_type_new(const char* name, PropertyFunc func);
void property_type_destroy(PropertyType* ct);

/* ComponentType functions */
ComponentType* component_type_new(const char* name);
void component_type_destroy(ComponentType* self);
void component_type_add_component_type(ComponentType* self, ComponentType* ct);
void component_type_add_property_type (ComponentType* self, PropertyType*  pt);

/* helper functions */
bool parser_duration_to_seconds  (const char* duration, int* seconds);
bool parser_datetime_to_unix_time(const char* datetime, time_t* unix_time);

Parser* parser_new(ComponentType* base_component_type, int flags, void* cookie);
void parser_destroy(Parser* self);
bool parser_set_mimedir(Parser* self, const char* mimedir);
bool parser_run(Parser* self);
bool parser_get_result(Parser* self, uint8_t** result, size_t* result_size);

/* add database records */
bool parser_add_int16 (Parser* self, uint16_t id, int16_t value);
bool parser_add_int32 (Parser* self, uint16_t id, int32_t value);
bool parser_add_string(Parser* self, uint16_t id, const char* str);
bool parser_add_time  (Parser* self, uint16_t id, time_t value);

bool parser_add_string_from_line(Parser* self, uint16_t id, mdir_line* line);
bool parser_add_time_from_line  (Parser* self, uint16_t id, mdir_line* line);

#endif 


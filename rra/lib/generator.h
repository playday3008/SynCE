/* $Id$ */
#ifndef __generator_h__
#define __generator_h__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h> /* for size_t */

#define GENERATOR_UTF8 1

typedef struct _GeneratorProperty GeneratorProperty;
typedef struct _Generator Generator;

struct _CEPROPVAL;
typedef bool (*GeneratorPropertyFunc)(Generator* g, struct _CEPROPVAL* property, void* cookie);

Generator* generator_new(int flags, void* cookie);
void generator_destroy(Generator* self);

bool generator_add_property(Generator* self, uint16_t id, GeneratorPropertyFunc func);

bool generator_set_data(Generator* self, const uint8_t* data, size_t data_size);
bool generator_run(Generator* self);
bool generator_get_result(Generator* self, char** result);

bool generator_add_simple(Generator* self, const char* name, const char* value);
bool generator_add_with_type(Generator* self, const char* name, const char* type, const char* value);

#endif


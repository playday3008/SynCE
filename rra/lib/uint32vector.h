/* $Id$ */
#ifndef __uint32vector_h__
#define __uint32vector_h__

#include <synce.h>

struct _RRA_Uint32Vector
{
  uint32_t* items;
  size_t used;
  size_t size;
};

typedef struct _RRA_Uint32Vector RRA_Uint32Vector;

/** Create a new Uint32Vector */
RRA_Uint32Vector* rra_uint32vector_new();

/** Destroy RRA_Uint32Vector object */
void rra_uint32vector_destroy(RRA_Uint32Vector* v, bool free_items);

/** Add an item to vector */
RRA_Uint32Vector* rra_uint32vector_add(RRA_Uint32Vector* v, uint32_t value);

/** Add many items to vector */
RRA_Uint32Vector* rra_uint32vector_add_many(
    RRA_Uint32Vector* v, 
    uint32_t* values, 
    size_t count);

/** Sort vector */
void rra_uint32vector_sort(RRA_Uint32Vector* v);

/** Dump vector contents with synce_log() */
void rra_uint32vector_dump(RRA_Uint32Vector* v);

#endif

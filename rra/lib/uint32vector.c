/* $Id$ */
#include "uint32vector.h"
#include <synce_log.h>
#include <stdlib.h>

static void rra_uint32vector_enlarge(RRA_Uint32Vector* v, size_t size)
{
  if (v->size < size)
  {
    size_t new_size = v->size ? v->size : 2;

    while (new_size < size)
      new_size <<= 1;

    v->items = realloc(v->items, sizeof(uint32_t) * new_size);
    if (!v->items)
    {
      synce_error("Failed to allocate space for %i elements - crashing!", new_size);
    }

    v->size  = new_size;
  }
}

RRA_Uint32Vector* rra_uint32vector_new()
{
  return (RRA_Uint32Vector*)calloc(1, sizeof(RRA_Uint32Vector));
}

void rra_uint32vector_destroy(RRA_Uint32Vector* v, bool free_items)
{
  if (v)
  {
    if (free_items && v->items)
      free(v->items);
    free(v);
  }
}

RRA_Uint32Vector* rra_uint32vector_add(RRA_Uint32Vector* v, uint32_t value)
{
  rra_uint32vector_enlarge(v, v->used + 1);
  v->items[v->used++] = value;
  return v;
}

RRA_Uint32Vector* rra_uint32vector_add_many(
    RRA_Uint32Vector* v, 
    uint32_t* values, 
    size_t count)
{
  unsigned i;
  
  rra_uint32vector_enlarge(v, v->used + count);

  for (i = 0; i < count; i++)
  {
    v->items[v->used++] = values[i];
  }
  
  return v;
}

static int rra_uint32vector_compare(const void* a, const void* b)
{
  return *(const uint32_t*)a - *(const uint32_t*)b;
}

void rra_uint32vector_sort(RRA_Uint32Vector* v)
{
  qsort(v->items, v->used, sizeof(uint32_t), rra_uint32vector_compare);
}

void rra_uint32vector_dump(RRA_Uint32Vector* v)
{
  unsigned i;
  for (i = 0; i < v->used; i++)
    synce_trace("%i: %08x", i, v->items[i]);
}



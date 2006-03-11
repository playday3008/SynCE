/* $Id$ */
#define _BSD_SOURCE 1
#include "../lib/syncmgr.h"
#include "../lib/uint32vector.h"
#include "../lib/appointment_ids.h"
#include "../lib/dbstream.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define GET_ALL 0

typedef struct 
{
  int index;
  int total;
}Progress;

static bool callback (
    RRA_SyncMgrTypeEvent event, uint32_t type, uint32_t count, uint32_t* ids, void* cookie)
{
  RRA_Uint32Vector* all_ids = (RRA_Uint32Vector*)cookie;
  rra_uint32vector_add_many(all_ids, ids, count);
  return true;
}

static bool writer
  (uint32_t type_id, uint32_t object_id, const uint8_t* data, size_t data_size, void* cookie)
{
  Progress* p = (Progress*)cookie;
  uint32_t propval_count;
  CEPROPVAL* propvals;
  unsigned saved_count = 0;
  bool save = false;

  propval_count = letoh32(*(uint32_t*)(data + 0));

  propvals = (CEPROPVAL*)malloc(sizeof(CEPROPVAL) * propval_count);

  if (dbstream_to_propvals(data + 8, propval_count, propvals))
  {
#if GET_ALL
    saved_count = propval_count;
    save = true;
#else
    unsigned i;
    for (i = 0; i < propval_count; i++)
    {
      switch (propvals[i].propid >> 16)
      {
        case ID_RECURRENCE_PATTERN:
          save = true;
          /* fall through */

        case ID_RECURRENCE_TIMEZONE:
        case ID_UNIQUE:
        case ID_APPOINTMENT_START:
        case ID_DURATION:
        case ID_APPOINTMENT_TYPE:
          if (i != saved_count)
            memcpy(&propvals[saved_count], &propvals[i], sizeof(CEPROPVAL));

          saved_count++;
          break;
      }
    }
#endif

    if (save) 
    {
      uint8_t* save_data = NULL;
      size_t save_size = 0;
      if (dbstream_from_propvals(propvals, saved_count, &save_data, &save_size))
      {
        char filename[30];
        FILE* file = NULL;

#if GET_ALL
        snprintf(filename, sizeof(filename), "appointment-%08x.bin", object_id);
#else
        snprintf(filename, sizeof(filename), "recurrence-%08x.bin", object_id);
#endif
        file = fopen(filename, "w");

        if (file)
        {
          fwrite(save_data, save_size, 1, file);
          fclose(file);
        }
        else
        {
          fprintf(stderr, "Failed to open file '%s'\n", filename);
        }

        free(save_data);
      }
    }
  }
  else
    synce_error("Failed to convert database stream");

  free(propvals);

  p->index++;
  printf("\r%i%%", 100 * p->index / p->total);
  fflush(stdout);
  return true;
}

int main(int argc, char** argv)
{
  int result = 1;
  HRESULT hr;
  RRA_SyncMgr* syncmgr = NULL;
  uint32_t type_id = 0;
  RRA_Uint32Vector* all_ids = rra_uint32vector_new();
  Progress progress;
  bool got_event = false;

  /*synce_log_set_level(0);*/

  hr = CeRapiInit();
  if (FAILED(hr))
    goto exit;

  syncmgr = rra_syncmgr_new();

  if (!rra_syncmgr_connect(syncmgr))
  {
    fprintf(stderr, "Connection failed\n");
    goto exit;
  }

  type_id = rra_syncmgr_type_from_name(syncmgr, RRA_SYNCMGR_TYPE_APPOINTMENT)->id;

  rra_syncmgr_subscribe(
      syncmgr, 
      type_id,
      callback, 
      all_ids);

  if (!rra_syncmgr_start_events(syncmgr))
  {
    fprintf(stderr, "Failed to start events\n");
    goto exit;
  }

  printf("Getting appointment ids...\n");

  /* Process all events triggered by rra_syncmgr_start_events */
  while (rra_syncmgr_event_wait(syncmgr, 3, &got_event) && got_event)
  {
    rra_syncmgr_handle_event(syncmgr);
  }

  printf("Getting appointment data...\n");
  printf("0%%");
  fflush(stdout);

  progress.index = 0;
  progress.total = all_ids->used;

  rra_syncmgr_get_multiple_objects(
      syncmgr,
      type_id,
      all_ids->used,
      all_ids->items,
      writer,
      &progress);

  printf("\n");

exit:
  rra_syncmgr_destroy(syncmgr);

  CeRapiUninit();
  return result;
}

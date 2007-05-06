#include "liborange_log.h"

#if !WITH_LIBSYNCE

#include <stdarg.h>
#include <stdio.h>

static int current_log_level = ORANGE_LOG_LEVEL_HIGHEST;

void orange_log_set_level(int level)
{
  current_log_level = level;
}

void _orange_log(int level, const char* file, int line, const char* format, ...)
{
  va_list ap;

  if (level > current_log_level)
    return;

  fprintf(stderr, "[%s:%i] ", file, line);

  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);

  fprintf(stderr, "\n");
}

#endif

/* $Id */
#include "synce_log.h"
#include <stdarg.h>
#include <stdio.h>

/* evil static data */
static int current_log_level = SYNCE_LOG_LEVEL_HIGHEST;

void synce_log_set_level(int level)
{
	current_log_level = level;
}

void _synce_log(int level, const char* file, int line, const char* format, ...)
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

void _synce_log_wstr(int level, const char* file, int line, const char* name,
		const WCHAR* wstr)
{
	if (level <= current_log_level)
  {
    char* ascii = wstr_to_ascii(wstr);

    fprintf(stderr, "[%s:%i] %s=\"%s\"\n", file, line, name, ascii);

    wstr_free_string(ascii);
  }
}


/* $Id */
#include "rapi_log.h"
#include <stdarg.h>
#include <stdio.h>

/* evil static data */
static int current_log_level = RAPI_LOG_LEVEL_HIGHEST;

void _rapi_log(int level, const char* file, int line, const char* format, ...)
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


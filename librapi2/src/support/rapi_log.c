/* $Id */
#include "rapi_log.h"
#include <stdarg.h>
#include <stdio.h>

void _rapi_log(const char* file, int line, const char* format, ...)
{
	va_list ap;

	fprintf(stderr, "[%s:%i] ", file, line);
	
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	
	fprintf(stderr, "\n");
}


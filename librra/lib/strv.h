/* $Id$ */
#ifndef __strv_h__
#define __strv_h__

char** strsplit(const char* source, int separator);
void strv_dump(char** strv);
void strv_free(char** strv);

#endif

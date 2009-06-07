/*

    Support functions for FUR: caching of opened files
    FUR is Copyright (C) Riccardo Di Meo <riccardo@infis.univ.trieste.it>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.

    FUSE is Copyright of Miklos Szeredi <miklos@szeredi.hu>
*/
#ifndef CACHE_H
#define CACHE_H

#include <rapi.h>


typedef struct cache {
  HANDLE h;
  char *path;
  int flags;
  int counter;
  off_t offset;
} cache_t;


// FIXME dyn. alloc
#define TMPSIZE 100
extern cache_t opened[TMPSIZE];
extern int opened_counter;

void print_cache_entry(int index);
int cache_find(const char *name, HANDLE h);
int cache_find_name(const char *name);
int cache_find_empty(void);
int cache_add(HANDLE h, const char *name, off_t offset, int flags);
int cache_free(int index);
int cache_remove(char *name);


#endif

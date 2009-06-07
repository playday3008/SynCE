/*

    Support functions for FUR: caching of opened files
    FUR is Copyright (C) Riccardo Di Meo <riccardo@infis.univ.trieste.it>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.

    FUSE is Copyright of Miklos Szeredi <miklos@szeredi.hu>
*/
#include "cache.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "macros.h"

cache_t opened[TMPSIZE];
int opened_counter;

void print_cache_entry(int index)
{
  fprintf(stderr,"Handle %8.8x\n",opened[index].h);
  fprintf(stderr,"Path   %s\n",opened[index].path);
  fprintf(stderr,"Flags %s\n",opened[index].flags|O_RDWR?"O_RDWR":(opened[index].flags|O_WRONLY?"O_WRONLY":"O_RDONLY"));
  fprintf(stderr,"Offset %d\n",opened[index].offset);
}

// name can be NULL XOR h can be -1
int cache_find(const char *name,HANDLE h)
{
  int i;
  CVERB("Cache_find on %s %d\n",name,h);
  //return index on success -1 on failure
  assert(name==NULL || h==-1);
  assert(name!=NULL || h!=-1);
  
  if(name==NULL) 
    for(i=0;i<TMPSIZE;i++)
      if(h==opened[i].h) {
	CVERB("Found %d\n",i);
	CPRT(i);
	return i;
      }
      
  if(h==-1)
    for(i=0;i<TMPSIZE;i++) {
      if(opened[i].path==NULL)
	continue;
      if(!strcmp(name,opened[i].path)) {
	CVERB("Found %d\n",i);
	CPRT(i);
	return i;
      }
    }
  CVERB("Found nothing\n");
  return -1;
}

int cache_find_name(const char *name)
{
  int i;
  CVERB("Cache_find_name on %s\n",name);
  //return index on success -1 on failure
  assert(name!=NULL);
  
  for(i=0;i<TMPSIZE;i++) {
    if(opened[i].path==NULL)
      continue;
    if(!strcmp(name,opened[i].path)) {
      CVERB("Found %d\n",i);
      CPRT(i);
      return i;
    }
  }
  CVERB("Found nothing\n");
  return -1;
}

int cache_find_empty()
{
  int i;
  CVERB("Cache_find_empyy\n");
  if(opened_counter==TMPSIZE)
    return -1;
  for(i=0;i<TMPSIZE;i++) 
    if(opened[i].path==NULL) {
      CVERB("Found %d\n",i);
      return i;
    }
  CVERB("Found nothing\n");
  assert(1==0);
  return -1;
}

// both name and h valid
int cache_add(HANDLE h,const char *name,off_t offset,int flags)
{
  int index;
  CVERB("Cache_add\n");
  if((index=cache_find_empty())<0)
    return -1;
    
  opened[index].path=strdup(name);
  if(opened[index].path==NULL) 
    return -1;

  opened[index].h=h;
  opened[index].flags=flags;
  opened[index].counter=1;
  opened[index].offset=offset;

  opened_counter++;
  CPRT(index);  
  return 0;
}

int cache_free(int index)
{
  CVERB("Cache free called on %d\n",index);
  CPRT(index);
  free(opened[index].path);
  opened[index].path=NULL;
  opened[index].h=-1;
  opened[index].flags=0;
  opened[index].counter=0;
  opened[index].offset=-1;
  opened_counter--;
  return 0;
}

int cache_remove(char *name)
{
  int index;
    
  if(opened_counter==0)
    return -1;
  
  CVERB("Cache remove called for %s\n",name);
  
  assert(name!=NULL);

  if((index=cache_find_name(name))<0)
    return -1;

  assert(opened[index].counter<=0);

  cache_free(index);
  return 0;
}


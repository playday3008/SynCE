/*

    Support functions for FUR: registry files
    FUR is Copyright (C) Riccardo Di Meo <riccardo@infis.univ.trieste.it>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.

    FUSE is Copyright of Miklos Szeredi <miklos@szeredi.hu>
*/
#include "registry.h"
#include "special_names.h"
#include "macros.h"
#include <stdlib.h>
#include <string.h>
#include "registry_utils.h"
#include "fur_utils.h"


int registry_read(const char *path,void *buf,size_t size,off_t offset)
{
  return ReadAttribute(path,buf,size,offset);
}

int path_is_in_registry(const char *path)
{
  if(strncmp(PROC REGISTRY,path,proclen+registrylen)==0) {
    if(strlen(path)>proclen+registrylen)
      if(path[proclen+registrylen]!='/') {
	VERB("%s is NOT registry!\n",path);
	return 0;
      }
    
    VERB("%s IS registry!\n",path);
    return 1;
  }
  VERB("%s is NOT registry!\n",path);
  return 0;
}

int path_is_registry(const char *path)
{  
  if(path_is_in_registry(path) && strlen(path)==proclen+registrylen)
    return 1;
  return 0;
}

void registry_fill_dir(const char *path,void *buf,fuse_fill_dir_t filler)
{
  if(path_is_registry(path)) {
    filler(buf,LOCAL+1,0,0);
    filler(buf,ALL+1,0,0);
    filler(buf,ROOT+1,0,0);
    filler(buf,USER+1,0,0);
    return;
  } 
  ListRegDir(path,buf,filler);
}

void registry_getattr(const char *path,struct stat *stbuf)
{
  VERB("registry_getattr in %s\n",path);

  if(strcmp(PROC REGISTRY LOCAL,path)==0) {
    VERB("Special_getattr on %s\n",path);
    stbuf->st_mode=S_IFDIR | 0500;
    stbuf->st_nlink=2;
    stbuf->st_uid=getfileuid(NULL);
    stbuf->st_gid=getfilegid(NULL);
    return;
  }

  if(strcmp(PROC REGISTRY ALL,path)==0) {
    VERB("Special_getattr on %s\n",path);
    stbuf->st_mode=S_IFDIR | 0500;
    stbuf->st_nlink=2;
    stbuf->st_uid=getfileuid(NULL);
    stbuf->st_gid=getfilegid(NULL);
    return;
  }

  if(strcmp(PROC REGISTRY ROOT,path)==0) {
    VERB("Special_getattr on %s\n",path);
    stbuf->st_mode=S_IFDIR | 0500;
    stbuf->st_nlink=2;
    stbuf->st_uid=getfileuid(NULL);
    stbuf->st_gid=getfilegid(NULL);
    return;
  }

  if(strcmp(PROC REGISTRY USER,path)==0) {
    VERB("Special_getattr on %s\n",path);
    stbuf->st_mode=S_IFDIR | 0500;
    stbuf->st_nlink=2;
    stbuf->st_uid=getfileuid(NULL);
    stbuf->st_gid=getfilegid(NULL);
    return;
  }

  // Treat it as a directory
  if(PathIsAKey(path)) {
    VERB("Special_getattr on %s\n",path);
    stbuf->st_mode=S_IFDIR | 0500;
    stbuf->st_nlink=2;
    stbuf->st_uid=getfileuid(NULL);
    stbuf->st_gid=getfilegid(NULL);
    return;
  } else {
    VERB("Special_getattr on %s\n",path);
    stbuf->st_mode=S_IFREG | 0500;
    stbuf->st_nlink=1;
    stbuf->st_uid=getfileuid(NULL);
    stbuf->st_gid=getfilegid(NULL);
    
    stbuf->st_size= GetAttribSize(path);
    return;
  }


}

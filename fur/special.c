/*

    Support functions for FUR: special "/proc like" files
    FUR is Copyright (C) Riccardo Di Meo <riccardo@infis.univ.trieste.it>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.

    FUSE is Copyright of Miklos Szeredi <miklos@szeredi.hu>
*/
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <fuse.h>
#include <errno.h>

#include "special_names.h"

#include "fur_utils.h"
#include "special.h"

#include "fur_utils.h"
#include "macros.h"


void special_init(void)
{
  power_init();
}


int path_is_special(char *path)
{
  if(strncmp(PROC,path,proclen)==0) {
    if(strlen(path)>proclen)
      if(path[proclen]!='/') {
	VERB("%s is NOT special!\n",path);
	return 0;
      }

    VERB("%s IS special!\n",path);
    return 1;
  }
  VERB("%s is NOT special!\n",path);
  return 0;
}


int path_is_proc(char *path)
{
  if(path_is_special(path) && index(path+proclen,'\\')==NULL)
    return 1;
  return 0;
}


int special_fill_dir(char *path,void *buf,fuse_fill_dir_t filler)
{
  VERB("Special_fill_dir in %s\n",path);
  //  /proc  files
  if(strcmp(path,PROC)==0) {
    VERB("putting %s\n",POWER_STATUS_NAME +1);
    filler(buf,strdup(POWER_STATUS_NAME +1),0,0);
    VERB("putting %s\n",REGISTRY +1);
    filler(buf,strdup(REGISTRY+1),0,0);
  }

  if(strncmp(PROC REGISTRY,path,proclen+registrylen)==0) {
    VERB("putting registry files! .\n");
    registry_fill_dir(path,buf,filler);
  }
  return 0;
}

void special_getattr(char *path,struct stat *stbuf)
{
  VERB("Special_getattr in %s\n",path);

  if(path_is_in_registry(path))
    registry_getattr(path,stbuf);

  if(strcmp(PROC POWER_STATUS_NAME,path)==0) {
    VERB("Special_getattr on power file\n");
    stbuf->st_mode=S_IFREG | 0400;
    stbuf->st_nlink=1;
    stbuf->st_uid=getfileuid(NULL);
    stbuf->st_gid=getfilegid(NULL);

    stbuf->st_size=get_power_file_size();
    return;
  }

  if(strcmp(PROC REGISTRY,path)==0) {
    VERB("Special_getattr on %s\n",path);
    stbuf->st_mode=S_IFDIR | 0500;
    stbuf->st_nlink=2;
    stbuf->st_uid=getfileuid(NULL);
    stbuf->st_gid=getfilegid(NULL);
    return;
  }

  if(strcmp(PROC,path)==0) {
    VERB("Special_getattr on %s\n",path);
    stbuf->st_mode=S_IFDIR | 0500;
    stbuf->st_nlink=2;
    stbuf->st_uid=getfileuid(NULL);
    stbuf->st_gid=getfilegid(NULL);
    return;
  }
}

void special_set_proc(void *buf,fuse_fill_dir_t filler) 
{
  VERB("Special_set_proc\n");
  filler(buf,strdup(PROC+1),0,0);
}

int special_read_file(char *path,void *buf,size_t size,off_t offset)
{
  VERB("Special_read_file in %s\n",path);
  if(strcmp(PROC POWER_STATUS_NAME,path)==0) 
    return read_power_status(buf,size,offset);

  if(strncmp(PROC REGISTRY,path,proclen+registrylen)==0) {
    VERB("Special read on a registry file! .\n");
    return registry_read(path,buf,size,offset);
  }
  
  assert(0);
  return 0;
}


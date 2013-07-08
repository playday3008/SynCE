/*

    Support functions for FUR: registry files
    FUR is Copyright (C) Riccardo Di Meo <riccardo@infis.univ.trieste.it>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.

    FUSE is Copyright of Miklos Szeredi <miklos@szeredi.hu>
*/
#ifndef REGISTRY_H
#define REGISTRY_H

#include <fuse.h>
#include <rapi2.h>

void registry_fill_dir(IRAPISession *session, const char *path,void *buf,fuse_fill_dir_t filler);

int path_is_in_registry(const char *path);

int registry_read(IRAPISession *session, const char *path,void *buf,size_t size,off_t offset);

void registry_getattr(IRAPISession *session, const char *path,struct stat *stbuf);

#endif

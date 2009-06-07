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

void registry_fill_dir(char *path,void *buf,fuse_fill_dir_t filler);


#endif

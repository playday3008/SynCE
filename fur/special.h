/*

    Support functions for FUR: special "/proc like" files
    FUR is Copyright (C) Riccardo Di Meo <riccardo@infis.univ.trieste.it>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.

    FUSE is Copyright of Miklos Szeredi <miklos@szeredi.hu>
*/
#ifndef SPECIAL_H
#define SPECIAL_H

void special_init(void);

int path_is_special(const char *path);

void special_getattr(const char *path,struct stat *stbuf);

void special_set_proc(void *buf,fuse_fill_dir_t filler);

int special_fill_dir(const char *path,void *buf,fuse_fill_dir_t filler);

int special_read_file(const char *path,void *buf,size_t size,off_t offset);

#endif

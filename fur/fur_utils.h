/*

    Support functions for FUR: librapi2 wrapping and conversion.
    FUR is Copyright (C) Riccardo Di Meo <riccardo@infis.univ.trieste.it>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.

    FUSE is Copyright of Miklos Szeredi <miklos@szeredi.hu>
*/
#ifndef FUR_UTILS_H
#define FUR_UTILS_H
#include <rapi.h>
#include "cache.h"
#include <stdio.h>


#ifdef VERBOSE
extern FILE *logfile;
#endif

#define SET_FILE_POINTER_MISSING


void finalize(void);
int init(void);

// Convert a ascii path in wide char format, used in the 
// RAPI protocol.
LPWSTR path2cepath(const char *path, int dir);

// Placeholders, they return always the uid and gid of the user 
// who launched dccm
uid_t getfileuid(const char *path);
gid_t getfilegid(const char *path);

// Convert flags for "Open"
void Unix2CeFlags(int flags, DWORD *ceflags, DWORD *ceshare);

// Self explanatory 
int Unlink(const char *path);
int Rename(const char *src, const char *dst);
int RmDir(const char *path);
int MkDir(const char *path, mode_t flag);
int MkNod(const char *path, int mode, int dev);
int OpenFile(const char *path, int flags);
void ReleaseFile(const char *path);
// Bugged(Tm)
int readFile(const char *path, void *buffer, size_t size, off_t offset);
int writeFile(const char *path, const void *buffer, size_t size, off_t offset);

// This actually is a kludge to overcome the absence of 
// SetFilePointer in the librapi2. Hopefully they will disappear, 
// some day...
int Truncate(const char *path,off_t offset);


// Utility which need to be rewritten due the lack of a caching
// machanism of some sort.
int getAttrib(const char *path);
int isDir(int attrib);
void getFileInfo(const char *path, CE_FIND_DATA **data, unsigned int *count, int* fsize, time_t *atime, time_t *mtime, time_t *ctime);
int getStats(const char *path, CE_FIND_DATA **data, unsigned int *count);
char **getFileList(const char *path);

// Convert the flags of unix to flags apt to the
// rapi implementation (and also build the share perm. flags)
void Unix2CeFlags(int flags,DWORD *ceflags,DWORD *ceperms);

// Check for the prec. attribute if is ref. to a dir
// 0 no, 1 yes, -1 error/not existing
int isDir(int attribs);


// Check if the uid of the caller is the same of the dccm user.
/// doesn't work at present
int right_uid(void);

#endif

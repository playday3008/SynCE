#ifndef REGISTRY_UTILS_H
#define REGISTRY_UTILS_H
#include <rapi.h>
#include <synce.h>
#include <fuse.h>

void GetKeys(HKEY base,void *buf,fuse_fill_dir_t filler);

// FIXME slow, memoy get allocated and freed too often
char **path2list(const char *fake_path,int isdir);


// Apre la chiave che si trova alla fine del path
HKEY fakePathOpen(const char *fake_path,int isdir);

void ListRegDir(const char *fake_path,void *buf,fuse_fill_dir_t filler);

int ReadAttribute(const char *path,void *buf,size_t size,off_t offset);

int PathIsAKey(const char *fake_path);

int GetAttribSize(const char *path);

#endif

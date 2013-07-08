#ifndef REGISTRY_UTILS_H
#define REGISTRY_UTILS_H
#include <synce.h>
#include <rapi2.h>
#include <fuse.h>

void GetKeys(IRAPISession *session, HKEY base,void *buf,fuse_fill_dir_t filler);

// FIXME slow, memoy get allocated and freed too often
char **path2list(const char *fake_path,int isdir);


// Apre la chiave che si trova alla fine del path
HKEY fakePathOpen(IRAPISession *session, const char *fake_path,int isdir);

void ListRegDir(IRAPISession *session, const char *fake_path,void *buf,fuse_fill_dir_t filler);

int ReadAttribute(IRAPISession *session, const char *path,void *buf,size_t size,off_t offset);

int PathIsAKey(IRAPISession *session, const char *fake_path);

int GetAttribSize(IRAPISession *session, const char *path);

#endif

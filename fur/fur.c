/*

    FUSE api for FUR  
    FUR is Copyright (C) Riccardo Di Meo <riccardo@infis.univ.trieste.it>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.

    FUSE is Copyright of Miklos Szeredi <miklos@szeredi.hu>
*/


/*
  Changelog

  0.4.5 
  - Added a patch entirely provided by Alexander Logvinov that forces FUR into
  single threaded mode.
  - multithread support for FUR completely discarded: removed all
    locks and references to the pthread library

  0.4.4 Patch kindly contributed by Matthias Gruenewald
  - added a fuse hook to a void chmod call to allow konqueror and 
  FUR to smoothly work together without annoying pop-ups (should 
  be explicitly enabled in configure)	
  - workaround to fix some issues in the librapi when listing the
  "windows" directory on the device that disconnect the PDA on error

  0.4.3 
  - Added the time property to the files. All the code for this
  release has been kindly contributed by Matthias Gruenewald.

  0.4.2
  - Only documentation changes. Since now the Synce provides
    everything that FUR needs, FUR will be able to run from here until
    eternity withoug changes.

  0.3.0
  - support added for CeSetFilePointer and CeSetEndOfFile (in conjunction
  with the patch).

  0.2.3
  - Changed locks again in a dummy safe thread mode. Now Fur *IS*
  thread safe... well if we don't consider the cache :-/
  - found bug in seek?

  0.2.2
  
  - Cleaned the code from experimental features not needed that
  caused problems (uid/gid checks, fake directory for non dccm
  curious). Not a great loss since fuse is usually run per user.
  - Added a reference counter to the cache (which was what screwed
  all file operations so much...)
  - Rearranged locks in a more conservative way (first make it 
  compile, second make it works, then optimize it...)
  - Introduced Reset and SeekFoward/Backward which _should_ work.
   URGENT BUGS STILL OPEN 
  - Bug on truncate discovered: doesn't work for enlarging nor 
  cutting files...
  - Bad multitrhreading control maybe...

  0.2.1 

  - fixed wrong path conversion for names that 
  are not directly convertibiles in ASCII.
  - corrected the configuration script.
  
  0.2.0 
  
  - Starting release
*/

// FIXME
// memory leak with paths in readdir? does fuse take care of freeing that?
// permissions of files

// TODO 
// caching of requests
// reduce the number of rapi calls
// add more functions
// add more attributes
// better uid/gid check (issue a error if the filesystem is accessed by another
//                       user or show a fake content (a readme.txt i.e.) )
//  truncate works tough it shouldn... (should not be able to "restrict" files...") Investigate...
// utime /ctime/mtime not added (librapi call missing).
// setattr xattr ecc
// rearrange the locks


#define SIMIL_PROC
#define FUSE_USE_VERSION 22
#include <time.h>
#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>

#include "macros.h"
#include "special.h"


static int rapi_mknod(const char *path, mode_t mode, dev_t dev)
{
  int result;
  VERB(" ** rapi_mknod **\n");
  result=MkNod(path,mode,dev);
  return result;
}

static int rapi_unlink(const char *path)
{
  int res;
  VERB(" ** rapi_unlink **\n");
  res=Unlink(path);
  return res;
}

static int rapi_rename(const char *src,const char *dst)
{
  int res;
  VERB(" ** rapi_rename **\n");
  res=Rename(src,dst);
  return res;
}


static int rapi_rmdir(const char *path)
{
  int res;
  VERB(" ** rapi_rmdir **\n");
  res=RmDir(path);
  return res;
}


// Windows Ce ignores the security flags completly...
static int rapi_mkdir(const char *path,mode_t flags)
{
  VERB(" ** rapi_mkdir **\n");

  if(!MkDir(path,flags)) {
    return 0;
  } else {
    return -EPERM;
  }
}

const char back_off_message[]="This directory can be read only by the user\nwho own the dccm demon!\n\nHave a nice day!\n";
const char readme_path[]="/readme.txt";

static int rapi_getattr(const char *path, struct stat *stbuf)
{
  int res = 0;
  int isdir;

  int attrib;
  int size;
  time_t atime;
  time_t mtime;
  time_t ctime;

  VERB("Getattr on %s\n",path);  

  memset(stbuf, 0, sizeof(struct stat));

  if(strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0500;
    stbuf->st_nlink = 2;
    stbuf->st_uid=getfileuid(NULL);
    stbuf->st_gid=getfilegid(NULL);
    return res;
  }

#ifdef SIMIL_PROC
  if(path_is_special(path)) {
    special_getattr(path,stbuf);
    return res;
  }
#endif

  
  attrib=getAttrib(path);
  isdir=isDir(attrib);
/*   mode=getMode(attrib); */

  if(isdir==1) {
    // Is a directory
    VERB("Gettattr say that %s is a directory!\n",path);
    stbuf->st_mode = S_IFDIR | 0700;
    stbuf->st_nlink = 2;

    getFileInfo(path, NULL, NULL, &size, &atime, &mtime, &ctime);
    stbuf->st_atime = atime;
    stbuf->st_ctime = ctime;
    stbuf->st_mtime = mtime;

    stbuf->st_uid=getfileuid(path);
    stbuf->st_gid=getfilegid(path);
    return res;
  } else
    if (isdir==0) {
      VERB("Gettattr say that %s is a file!\n",path);
      // Is a file
      stbuf->st_mode = S_IFREG | 0700;
      stbuf->st_nlink = 1;
      // FIXME use a single call!
      getFileInfo(path, NULL, NULL, &size, &atime, &mtime, &ctime);
      stbuf->st_size = size;
      stbuf->st_atime = atime;
      stbuf->st_ctime = ctime;
      stbuf->st_mtime = mtime;
      VERB("stbuf->st_ctime: %d\n",stbuf->st_ctime);
      stbuf->st_uid=getfileuid(path);
      stbuf->st_gid=getfilegid(path);

      return res;
    } else {
      // Is nothing...
      VERB("Gettattr say that %s is nothing...!\n",path);
      return -ENOENT;
    }
  return -ENOENT;
}

static int rapi_readdir(const char *path,void *buf,fuse_fill_dir_t filler,off_t offset,struct fuse_file_info *fi)
{
  char **rapifiles;
  int i;
  int result;
  VERB(" ** rapi_readdir **\n");

  VERB("Listing requested for file %s\n",path);

  // Directory e parent
  filler(buf, ".", 0,0);
  filler(buf, "..", 0,0);

  // Check if we are in a virtual directory
#ifdef SIMIL_PROC
  // Add the "proc" file
  if(strcmp("/",path)==0)
    special_set_proc(buf,filler);

  // If we are in proc, fill only special files
  if(path_is_special(path)) {
    VERB("**** path %s IS special! ****\n",path);
    special_fill_dir(path,buf,filler);
    return 0;
  } else
    VERB("**** path %s is NOT special! ****\n",path);  
#endif   

  if((result=getAttrib(path))==-1) {
    return -ENOENT;
  }

  if(!isDir(result)) {
    return -ENOENT;
  }
    
  // Inserting the list of Device files
  rapifiles=getFileList(path);


  if(rapifiles==NULL) {
    return -errno;
  }

  i=0;
  
  // The list of file names has not to be freed? 
  // MMhhh.. smell like a memory leak to me :-/
  while(rapifiles[i]!=NULL) {
    VERB("File %d of list for %s:  %s\n",i,path,rapifiles[i]);
    filler(buf,rapifiles[i++],0,0);
  }

  free(rapifiles);
  return 0;
}

static int rapi_release(const char *path, struct fuse_file_info *fi)
{

#ifdef SIMIL_PROC
  if(path_is_special(path)) {
    return 0;
  }
#endif

  ReleaseFile(path);
  return 0;
}

static int rapi_open(const char *path, struct fuse_file_info *fi)
{
  int res;
  VERB(" ** rapi_open **\n");


#ifdef SIMIL_PROC
  if(path_is_special(path)) {
    return 0;
  }
#endif
  
  res=OpenFile(path,fi->flags);
  return res;
}

static int rapi_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi)
{
  int result;

  VERB(" ** rapi_read **\n");
#ifdef SIMIL_PROC
  if(path_is_special(path)) {
    result=special_read_file(path,buf,size,offset);
    return result;
  }
#endif

  if((result=readFile(path,buf,size,offset))==-1) {
    return -EIO;
  }
  return result;
}

static int rapi_write(const char *path, const char *buf, size_t size, off_t offset,struct fuse_file_info *fi)
{
  int result;

  VERB(" ** rapi_write **\n");

  if((result=writeFile(path,buf,size,offset))==-1) {
    return -EIO;
  }
  return result;
}

#ifdef VOID_CHMOD
static int rapi_chmod(const char *path, mode_t mode)
{
  int result=0;

  VERB(" ** rapi_chmod (NOT IMPLEMENTED) **\n");

  return result;
}
#endif

static struct fuse_operations hello_oper = {
  .getattr	= rapi_getattr,
  .readdir	= rapi_readdir,
  .open	        = rapi_open,
  .release      = rapi_release,
  .read	        = rapi_read,
  .write	= rapi_write,
  .mkdir        = rapi_mkdir,
  .mknod        = rapi_mknod,
/*   .truncate     = rapi_truncate, */
  .unlink       = rapi_unlink,
  .rmdir        = rapi_rmdir,
#ifdef VOID_CHMOD
  .chmod        = rapi_chmod,
#endif
  .rename       = rapi_rename
};

int main(int argc, char *argv[])
{
  int retval;
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
 

  // Initialize the RAPI support
  if(CeRapiInit()) {
    printf("Rapi not initialized!\n");
    exit(1);
  }
  
  init();
  fuse_opt_parse(&args, NULL, NULL, NULL);
  fuse_opt_add_arg(&args, "-s");
  retval=fuse_main(args.argc, args.argv, &hello_oper);

  fuse_opt_free_args(&args);
  // Finalize RAPI
  CeRapiUninit();

  return retval;
}

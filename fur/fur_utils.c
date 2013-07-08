/*

    Support functions for FUR: librapi2 wrapping and conversion.
    FUR is Copyright (C) Riccardo Di Meo <riccardo@infis.univ.trieste.it>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.

    FUSE is Copyright of Miklos Szeredi <miklos@szeredi.hu>
*/
#include <stdlib.h>
#include <stdio.h>
#include <rapi2.h>
#include <synce.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fuse.h>
#include <errno.h>

#include "macros.h"
#include "fur_utils.h"
#include "cache.h"
#include "special.h"
#include "special_names.h"

#ifdef VERBOSE
FILE *logfile;
#endif

// This will act as a replacement for the missing
// get/set time of the library
time_t mount_time;

void ReinitRAPI(IRAPISession *session)
{
    HRESULT hr;

    IRAPISession_CeRapiUninit(session);
    if(FAILED(hr = IRAPISession_CeRapiInit(session))) {
      printf("Rapi not initialized!: %s\n", synce_strerror(hr));
      exit(1);
    }
}

uid_t getfileuid(const char *path)
{
  //TODO something??
  VERB("getfileuid on %s\n",path);
  return getuid();
}

gid_t getfilegid(const char *path)
{
  //TODO something??
  VERB("getfilegid on %s\n",path);
  return getgid();
}

/* int Ce2UnixAttribs(DWORD ceattr) */
/* { */
/*   int unixattr=0100; */
  
/*   unixattr|=(FILE_ATTRIBUTE_READONLY & ceattr)?0400:0600; */
/*   unixattr|=(FILE_ATTRIBUTE_DIRECTORY & ceattr)?S_IFDIR:S_IFREG; */
/*   return unixattr; */
/* } */

/* int getMode(int attrib) */
/* { */
/*   if(attrib==-1) */
/*     return -1; */
/*   return attrib & FILE_ATTRIBUTE_DIRECTORY?1:0; */
/* } */

// FIXME: broken
// Convert the flags of unix to flags apt to the
// rapi implementation (and also build the share perm. flags)
// The policy is: permit sharing on read only file
void Unix2CeFlags(int flags,DWORD *ceflags,DWORD *ceshare)
{
  // FIXME!!!!!
  *ceflags=GENERIC_READ|GENERIC_WRITE;

  // Opening mode: coerced to READ
/*   *ceflags=(flags==O_RDONLY)?GENERIC_READ:*ceflags; */
/*   *ceflags=(flags==O_WRONLY)?GENERIC_WRITE:*ceflags; */
/*   *ceflags=(flags==O_RDWR)?GENERIC_WRITE|GENERIC_READ:*ceflags; */
  VERB("flags== 0x%8.8x 0x%8.8x\n",flags,*ceflags);
/*   *ceflags=GENERIC_READ; */

  // Sharing mode
  *ceshare=*ceflags & GENERIC_WRITE?0:FILE_SHARE_READ;
}

int Unlink(IRAPISession *session, const char *path)
{
  LPWSTR s;
  int cache_index;

  cache_index=cache_find_name(path);
  if(cache_index>=0) {
    assert(opened[cache_index].counter>0);
    return -EBUSY;
  }

  print_cache_entry(cache_index);
      
  if(!IRAPISession_CeDeleteFile(session, (LPCWSTR) (s=path2cepath(path,0)))) {
    free(s);
    return -EIO;
  } else {
    free(s);
    VERB("Unlink on %s succeded!\n",path);
    return 0;
  }
}

int Rename(IRAPISession *session, const char *src,const char *dst)
{
  LPWSTR s,t;
  int index1,index2;
  
  index1=cache_find_name(src);
  index2=cache_find_name(dst);
  if(index1>=0 || index2>=0) {
    VERB("Uno dei nomi appartiene ad un file utilizzato.\n");
    return -EBUSY;
  }

  if(getAttrib(session, dst)) {
    VERB("File %s gia' presente: lo cancello...",dst);
    Unlink(session, dst);
  }

  if(IRAPISession_CeMoveFile(session, s=path2cepath(src,0),t=path2cepath(dst,0))) {
    free(s);
    free(t);
    VERB("Rename of %s to %s succeded!\n",src,dst);
    return 0;
  } else {
    free(s);
    free(t);
    VERB("Rename of %s to %s failed!\n",src,dst);
    return -EACCES;
  }
}

int RmDir(IRAPISession *session, const char *path)
{
  LPWSTR s;
  if(!IRAPISession_CeRemoveDirectory(session, s=path2cepath(path,0))) {
    free(s);
    VERB("RmDir on %s failed!\n",path);
    return 1;
  } else {
    free(s);
    VERB("RmDir on %s succeded!\n",path);
    return 0;
  }
}

int MkDir(IRAPISession *session, const char *path,mode_t flag)
{
  LPWSTR s;
  if(!IRAPISession_CeCreateDirectory(session, s=path2cepath(path,0),NULL)) {
    free(s);
    VERB("MkDir on %s failed!\n",path);
    return 1;
  } else {
    free(s);
    VERB("MkDir on %s succeded!\n",path);
    return 0;
  }
}

int MkNod(IRAPISession *session, const char *path,int mode,int dev)
{
  LPWSTR s;
  HANDLE handle;
  
  VERB("MkNod started for %s\n",path);

  if((mode&S_IFREG)!=S_IFREG) {
    VERB("in Mknod: strange flag (%8.8x)!\n",mode);
    return 1;
  }

  VERB("MkNod check for file %s existance\n",path);
  handle=IRAPISession_CeCreateFile(session, s=path2cepath(path,0),0,0,0,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,0);
  free(s);
  if(handle==0) {
    VERB("in Mknod: check for precedent file failed!\n");
    return 1;
  }
  IRAPISession_CeCloseHandle(session, handle);

  VERB("MkNod create the file %s\n",path);
  handle=IRAPISession_CeCreateFile(session, s=path2cepath(path,0),GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,0);
  free(s);
  if(handle==0) {
    VERB("in Mknod: creation of a empty failed!\n");
    return 1;  
  }
  
  IRAPISession_CeCloseHandle(session, handle);
  VERB("Node %s created!\n",path);
  return 0;
}

// return a handle to the file or NULL
int OpenFile(IRAPISession *session, const char *path,int flags)
{
  LPWSTR s;
  DWORD ceflags,ceshare;
  int index;
  HANDLE handle;

  if((index=cache_find(path,-1))!=-1) {
    opened[index].counter++;
    return 0;
  }

  // If the file was opened, check if the file pointer 
  // was at 0 and the flag the same. 
  // In this case, nothing is to be done.
  if(index>=0) {
    VERB("Old file reopened: new flags %d (old was %d)\n",flags,opened[index].flags);
    VERB("The old pointer was 0x%8.8x\n",(unsigned int) opened[index].h);
    VERB("Nothing done for open: file alredy opened!\n");
    return 0;
  }
    
  // The file was not opened: open it now
  VERB("Im Opening file \"%s\" with flags %d\n",path,flags);
  
  // Get the right flags
  Unix2CeFlags(flags,&ceflags,&ceshare);
  
  // Get the handle
  handle=(HANDLE) IRAPISession_CeCreateFile(session, s=path2cepath(path,0),ceflags,ceshare,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
  free(s);

  // Check if the handle is OK
  if(handle==-1) {
    VERB("Open of %s failed!\n",path);
    return -EACCES;
  }

  VERB("File %s opened (0x%8.8x)!\n",path,handle);
  cache_add(handle,path,0,flags);
  return 0;
}

void ReleaseFile(IRAPISession *session, const char *path)
{
  int res;
  int index;
  VERB("ReleaseFile on %s\n",path);

  index=cache_find_name(path);
  if(index<0)
    return;
  assert(index>=0);
  
  opened[index].counter--;
  assert(opened[index].counter>=0);
  if(opened[index].counter>0) {
    return;
  }

  res=IRAPISession_CeCloseHandle(session, opened[index].h);
  if(!res) {
    VERB("CeCloseHandle failed to close %s!\n",opened[index].path);
  } else {
    VERB("CeCloseHandle succeded closing %s!\n",opened[index].path);
  }
  cache_free(index);
}

int tell(IRAPISession *session, int index)
{
  VERB("Tell of %s called!\n",opened[index].path);
  return IRAPISession_CeSetFilePointer(session, opened[index].h,0,NULL,FILE_CURRENT);
}


int GetOneFileSize(IRAPISession *session, int index)
{
  int result;
  int current=tell(session, index);
  
  assert(current==opened[index].offset);

  VERB("Attempting GetOneFileSize on %s (%d)\n",opened[index].path,index);
  result=IRAPISession_CeSetFilePointer(session, opened[index].h,0,NULL,FILE_END);
  IRAPISession_CeSetFilePointer(session, opened[index].h,current,NULL,FILE_BEGIN);
  VERB("File size is: %d\n",result);
  return result;
}

int Truncate(IRAPISession *session, const char *path,off_t newsize)
{
  int index;
  int removeme=0;

  index=cache_find_name(path);
  if(index<0) {
    if(OpenFile(session, path,O_WRONLY))
      return -EIO;
    removeme++;
  }
  index=cache_find_name(path);
  assert(index>=0);

  opened[index].offset=IRAPISession_CeSetFilePointer(session, opened[index].h,newsize,NULL,FILE_BEGIN);
  // Now set end of file is a null operation
/*   CeSetEndOfFile(opened[index].h); */

  if(removeme)
    ReleaseFile(session, path);

  return 0;
}

int readFile(IRAPISession *session, const char *path,void *buffer,size_t size,off_t offset)
{
  BOOL result;
  DWORD got;
  int index=cache_find_name(path);
  HANDLE handle;
  
  if(index<0) {
    OpenFile(session, path,O_RDWR);
  //FIXME check on open
    index=cache_find_name(path);
  }
  assert(index>=0);
  handle=opened[index].h;
 

  VERB("Im reading %d bytes with a offset of %d and pointer 0x%8.8x\n",size,(int) offset,handle);
  VERB("The handle is 0x%8.8x\n",handle);
  
  if(handle==0) {
    VERB("Handle null on readFile!\n");
    return -1;
  }

  if(offset!=opened[index].offset) {
    if((opened[index].offset=IRAPISession_CeSetFilePointer(session, (HANDLE) handle,offset,NULL,FILE_BEGIN))==-1) {
      VERB("File pointer not moved!\n");
      return -EIO;
    }
    VERB("Offset accounted!\n");
  }

  
  // Read the data in the (pre)filled buffer
  result=IRAPISession_CeReadFile(session, (HANDLE) handle,(LPVOID) buffer,(DWORD) size,&got,NULL);
  if(result==-1) {
    VERB("Data not read!\n");
    opened[index].offset=-1;
    return -1;
  }
  opened[index].offset+=got;
  VERB("%d bytes read from %s\n",got,opened[index].path);
  return got;
}

// Likely _Heavily_bugged_ im not sure to have handled the 
// offset fine
int writeFile(IRAPISession *session, const char *path,const void *buffer,size_t size,off_t offset)
{
  BOOL result;
  DWORD got;
  int index;
  HANDLE handle;

  index=cache_find_name(path);
  assert(index>=0);
  handle=opened[index].h;

  VERB("Im reading %d bytes with a offset of %d and pointer 0x%8.8x\n",size,(int) offset,handle);
  VERB("The handle is 0x%8.8x\n",handle);
  
  if(handle==0) {
    VERB("Handle null on readFile!\n");
    return -1;
  }

  if(offset!=opened[index].offset) {
    if((opened[index].offset=IRAPISession_CeSetFilePointer(session, (HANDLE) handle,offset,NULL,FILE_BEGIN))==-1) {
      VERB("File pointer not moved!\n");
      return -1;
    }
    VERB("Offset accounted!\n");
  }

  // Read the data in the (pre)filled buffer
  result=IRAPISession_CeWriteFile(session, (HANDLE) handle,(LPVOID) buffer,(DWORD) size,&got,NULL);
  if(result==-1) {
    VERB("Data not read!\n");
    opened[index].offset=-1;
    return -1;
  }
  opened[index].offset+=got;
  VERB("%d bytes writed to %s\n",got,path);
  return got;
}


LPWSTR path2cepath(const char *path,int dir)
{
  char *dstring;
  int i,j;

  LPWSTR s;

  dstring=calloc(2*strlen(path)+3,sizeof(char));
  if(dstring==NULL) 
    return NULL;

  for(i=0,j=0;i<strlen(path);i++) {
    switch(path[i]) {
    case '/':
      if(path[i+1]!='/') {
	dstring[j++]='\\';
/* 	dstring[j++]='\\'; */
      }
      break;
    default:
      dstring[j++]=path[i];
    }
  }
  if(dir) {
    if(dstring[j-1]=='\\')
      dstring[j]='*';
    else {
      dstring[j]='\\';
      dstring[j+1]='*';
    }
  }
  
  s=wstr_from_current(dstring);
  free(dstring);
  if(s==NULL)
    return NULL;
 
  return s;
}

int getAttrib(IRAPISession *session, const char *path)
{
  LPWSTR s;
  int result;

  result=IRAPISession_CeGetFileAttributes(session, s=path2cepath(path,0));
  free(s);

  return result;
}

int isDir(int attrib)
{
  if(attrib==-1)
    return -1;
  return attrib & FILE_ATTRIBUTE_DIRECTORY?1:0;
}

extern long int timezone;
extern int daylight;
extern char * tzname [2];

time_t convertCeTime(FILETIME *ftime)
{
  TIME_FIELDS timeFields;
  struct tm tp;
  time_t t;
  long int gmt_off;

  // We need to know the current offset from UTC for correcting the CE time
  // The CE time is UTC as seen by the PocketPC's current time
  // If the PocketPC is in summer daylight saving, it will be off 2 hours for Germany
  // If the PocketPC is in normal winter time period, it will be off 1 hours for Germany
  t=time(NULL);
  localtime_r(&t,&tp);
  gmt_off=tp.tm_gmtoff;
  VERB("tm_gmtoff: %d\n",gmt_off);

  time_fields_from_filetime(ftime, &timeFields);
  memset((void *)&tp, 0, sizeof(struct tm));
  tp.tm_sec=timeFields.Second+gmt_off;
  VERB("sec: %d\n",timeFields.Second);
  tp.tm_min=timeFields.Minute;
  VERB("minute: %d\n",timeFields.Minute);
  tp.tm_mday=timeFields.Day;
  VERB("day: %d\n",timeFields.Day);
  tp.tm_hour=timeFields.Hour;
  VERB("hour: %d\n",timeFields.Hour);
  tp.tm_mon=timeFields.Month-1;
  VERB("month: %d\n",timeFields.Month);
  tp.tm_year = timeFields.Year-1900;
  VERB("year: %d\n",timeFields.Year);
  tp.tm_wday=timeFields.Weekday;
  VERB("weekday: %d\n",timeFields.Weekday);
  tp.tm_yday=-1;
  tp.tm_isdst=-1;
  t=mktime(&tp);
  
  if (t!=-1) 
    return t;
  else
    return 0;
}

void getFileInfo(IRAPISession *session, const char *path, CE_FIND_DATA **data, unsigned int *count, int* fsize, time_t *atime, time_t *mtime, time_t *ctime)
{
  CE_FIND_DATA *fdata;
  unsigned int mcount;
  int res;

  
  // TODO FIXME: fdata have to be initialized??
  // FIXME: 

  *fsize=0;
  memset(atime, 0, sizeof(time_t));
  memset(mtime, 0, sizeof(time_t));
  memset(ctime, 0, sizeof(time_t));

  if((res=getStats(session, path,&fdata,&mcount))==1)
    return;
  
  // removed: let fur handle (imho buggy) fs situations where 2 differents 
  // have the same uppercase names (names differs only in case)
  //assert(mcount==1);


  *fsize=fdata[0].nFileSizeLow;
  *ctime=convertCeTime(&fdata[0].ftCreationTime);
  *mtime=convertCeTime(&fdata[0].ftLastWriteTime);
  *atime=convertCeTime(&fdata[0].ftLastAccessTime);

  // "Small" files
  assert(fdata[0].nFileSizeHigh==0);

  if(count!=NULL) 
    *count=mcount;
  if(data!=NULL) 
    *data=fdata;
  else
    IRAPISession_CeRapiFreeBuffer(session, fdata);
    
}

int getStats(IRAPISession *session, const char *path,CE_FIND_DATA **data,unsigned int *count)
{
  LPWSTR s;
  BOOL result;


  VERB("Attributes requested for %s\n",path);
  
  result=IRAPISession_CeFindAllFiles(session, (LPCWSTR) (s=path2cepath(path,0)),FAF_ATTRIBUTES|FAF_SIZE_LOW|FAF_OID|FAF_CREATION_TIME|FAF_LASTACCESS_TIME|FAF_LASTWRITE_TIME, count,data);
  free(s);
  if(!result) {

    // WORK AROUND
    // If the Windows directory is read, the RAPI library gets somehow broken
    // So reopen it to tget it working again
    ReinitRAPI(session);

    VERB("** Failed to read file list in getAttributes!\n");
    return 1;
  }

  VERB("Attributes obtained! (for %d files)\n",*count);

  return 0;
}


char **getFileList(IRAPISession *session, const char *path)
{
  LPWSTR s;
  int isdir;
  int hiddenp=1;
  DWORD fcount=-1;
  CE_FIND_DATA *find_data=NULL;
  int i;
  char **filelist=NULL;
  BOOL result;



  VERB("!!!!!!!!!!!!!!! THE PATH IS :   %s !!!!!!!!!!!!!!!\n",path);
  
  isdir=1;
  
  result=IRAPISession_CeFindAllFiles(session, s=path2cepath(path,isdir),(hiddenp?0:FAF_ATTRIB_NO_HIDDEN)|FAF_NAME,&fcount,&find_data);
  free(s);
  if(!result) {

    // WORK AROUND
    // If the Windows directory is read, the RAPI library gets somehow broken
    // So reopen it to tget it working again
    ReinitRAPI(session);

    VERB("** Failed to read file list in getFileList!\n");
    return NULL;    
  }

  filelist=(char **) calloc(fcount+1,sizeof(char *));
  if(filelist==NULL) {
    IRAPISession_CeRapiFreeBuffer(session, find_data);
    VERB("*** Error allocating memory in getFileList\n");
    return NULL;
  }
  // FIXME: memory leak o no?  
  for(i=0;i<fcount;i++) {
    filelist[i]=wstr_to_current(find_data[i].cFileName);
    if(filelist[i]==NULL) {
      for(;i>=0;i--)
	free(filelist[i]);
      free(filelist);
      IRAPISession_CeRapiFreeBuffer(session, find_data);
      return NULL;
    }
  }

  IRAPISession_CeRapiFreeBuffer(session, find_data);
  return filelist;
}

#ifdef VERBOSE
void finalize(void)
{
  fclose(logfile);
}
#endif


#ifdef VERBOSE
int init(IRAPISession *session, const char *logfile)
#else
int init(IRAPISession *session)
#endif
{
  int i;
  init_names();
  special_init(session);

/*   char *message=power_message(); */
/*   printf("%d\n",strlen(message)); */
/*   exit(0); */

#ifdef VERBOSE
  if (logfile == NULL)
    logfile=fopen("log.txt","w");
  else
    logfile=fopen(logfile,"w");
#endif

  for(i=0;i<TMPSIZE;i++) {
    opened[i].h=0;
    opened[i].path=NULL;
  };
  opened_counter=0;

#ifdef VERBOSE
  atexit(finalize);
#endif


  return 0;
}


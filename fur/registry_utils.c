#include "registry_utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h> 

#include "special_names.h"
#include "macros.h"


//FIXME fixed max registry name length
#define MAX_ATTRNAME 1000
void GetAttributes(HKEY base,void *buf,fuse_fill_dir_t filler)
{
  LPWSTR lpName;			
  DWORD num=MAX_ATTRNAME;

  lpName=calloc(MAX_ATTRNAME,sizeof(WCHAR));
  {
    int i=0;

    if(CeRegEnumValue(base,0,lpName,&num,NULL,NULL,NULL,NULL)!=ERROR_SUCCESS) {
      VERB("Failed!\n");
      return;
    }

    num=MAX_ATTRNAME;
    VERB("Getting values...\n");
    while(CeRegEnumValue(base,i,lpName,&num,NULL,NULL,NULL,NULL)==ERROR_SUCCESS) {
      char *string=wstr_to_current(lpName);
      printf("New value (%d) is: %s\n",i,string);
      filler(buf,string,0,0);
      free(string);
      i++;
      num=MAX_ATTRNAME;
    }
  }      
  free(lpName);
}


// Data must be read entirely until the offset
int ReadAttribute(char *path,void *buf,size_t size,off_t offset)
{
  void *null;

  int len=0;
  HKEY key;
  char *attrname;
  LPWSTR lattrname;
  int got=size;

  void *tmp;
  tmp=calloc(offset+size,1);
  if(tmp==NULL) 
    return -EIO;

  // Open the key
  if((key=fakePathOpen(path,0))==0) {
    VERB("Key not opened in GetAttribSize (%s)!\n",path);
    free(tmp);
    return -EIO;
  }

  attrname=rindex(path,'/')+1;
  lattrname=wstr_from_current(attrname);
  if(lattrname==NULL) {
    VERB("String not created in GetAttribSize (%s)!\n",attrname);
    CeRegCloseKey(key);
    free(tmp);
    return -EIO;
  }

  if(CeRegQueryValueEx(key,lattrname,NULL,NULL,tmp,(LPDWORD) &got)!=ERROR_SUCCESS) {
    VERB("CeRegQueryValueEx failed!\n");
    VERB("Attrname=%s\n",attrname);
    CeRegCloseKey(key);
    free(lattrname);
    free(tmp);
    return -EIO;
  }

  memcpy(buf,tmp+offset,size);
  free(tmp);
  CeRegCloseKey(key);

  free(lattrname);
  return size;


}

int GetAttribSize(char *path)
{
  int len=0;
  HKEY key;
  char *attrname;
  LPWSTR lattrname;
  int size;

  // Open the key
  if((key=fakePathOpen(path,0))==0) {
    VERB("Key not opened in GetAttribSize (%s)!\n",path);
    return -EIO;
  }

  attrname=rindex(path,'/')+1;
  lattrname=wstr_from_current(attrname);
  if(lattrname==NULL) {
    VERB("String not created in GetAttribSize (%s)!\n",attrname);
    CeRegCloseKey(key);
    return -EIO;
  }

  if(CeRegQueryValueEx(key,lattrname,NULL,NULL,NULL,(LPDWORD) &size)!=ERROR_SUCCESS) {
    VERB("CeRegQueryValueEx failed!\n");
    VERB("Attrname=%s\n",attrname);
    free(lattrname);
    return -ENOENT;
  }
  free(lattrname);
  return size;
}

//FIXME fixed max registry name length
#define MAX_REGNAME 1000
void GetKeys(HKEY base,void *buf,fuse_fill_dir_t filler)
{
  LPWSTR lpName;			
  DWORD num=MAX_REGNAME;

  lpName=calloc(MAX_REGNAME,sizeof(WCHAR));
  {
    int i=0;

    if(CeRegEnumKeyEx(base,0,lpName,&num,NULL,NULL,NULL,NULL)!=0) {
      VERB("Failed!\n");
      return;
    }

    num=MAX_REGNAME;
    VERB("Getting subkeys...\n");
    while(!CeRegEnumKeyEx(base,i,lpName,&num,NULL,NULL,NULL,NULL)) {
      char *string=wstr_to_current(lpName);
      printf("New key (%d) is: %s\n",i,string);
      filler(buf,string,0,0);
      free(string);
      i++;
      num=MAX_REGNAME;
    }
  }
      
  free(lpName);
}

char **path2list(char *fake_path,int isdir)
{
 int tokens=0;
 char *p=strdup(fake_path);
 char **array;
 int i,j;

 if(p==NULL)
   return NULL;
  
 for(i=0;i<strlen(fake_path);i++) 
   if(p[i]=='/') {
    tokens++;
    p[i]=0;
 }

 if(!isdir) 
   tokens--;

 array=calloc(tokens+1,sizeof(char *));
 if(array==NULL) {
   free(p);
   return NULL;
 }

 array[tokens]=NULL;

 for(i=0,j=0;j<tokens && i<strlen(fake_path);i++) 
   if(p[i]==0)
     array[j++]=&p[i+1];

 return array;
}

// Apre la chiave che si trova alla fine del path
HKEY fakePathOpen(char *fake_path,int isdir)
{
  char **array=path2list(fake_path,isdir);
  HKEY keytype=-1;
  HKEY newkey;
  int i;
  
  if(array==NULL) {
    VERB("Empty array\n");
    return 0;
  }

  if(array[1]==NULL || array[2]==NULL) {
    VERB("Name too short!\n");
    return 0;
  }
    
  
  assert(strcmp(PROC+1,array[0])==0);
  assert(strcmp(REGISTRY+1,array[1])==0);
  
  keytype=strcmp(array[2],1+LOCAL)==0?HKEY_LOCAL_MACHINE:keytype;
  keytype=strcmp(array[2],1+ALL)==0?HKEY_USERS:keytype;
  keytype=strcmp(array[2],1+ROOT)==0?HKEY_CLASSES_ROOT:keytype;
  keytype=strcmp(array[2],1+USER)==0?HKEY_CURRENT_USER:keytype;

  if(keytype==-1) {
    VERB("Keytype -1\n");
    free(array[0]-1);
    return 0;
  }
       
  i=3;
  while(array[i]!=NULL) {
    LPWSTR s;
    if(CeRegOpenKeyEx(keytype,s=wstr_from_current(array[i]),0,0,&newkey)!=0) {
      free(s);
      free(array[0]-1);
      CeRegCloseKey(keytype);
      VERB("Key not opened\n");
      return 0;
    }      
    free(s);
    CeRegCloseKey(keytype);
    keytype=newkey;
    VERB("%s opened as key 0x%8.8x!\n",array[i],keytype);
    i++;   
  }
 
  VERB("All done\n");
  free(array[0]-1);
  return keytype;
}

void ListRegDir(char *fake_path,void *buf,fuse_fill_dir_t filler)
{
  HKEY key=fakePathOpen(fake_path,1);
  if(key==0)
     return;
  printf("The key to be opened is= 0x%8.8x\n",key);

  GetAttributes(key,buf,filler);
  GetKeys(key,buf,filler);

  CeRegCloseKey(key);
}

int PathIsAKey(char *fake_path)
{
  HKEY key;
  if((key=fakePathOpen(fake_path,1))==0)
     return 0;
  CeRegCloseKey(key);
  return 1;
}

/*

    Power functions for FUR: special "/proc/apm like" file
    FUR is Copyright (C) Riccardo Di Meo <riccardo@infis.univ.trieste.it>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.

    FUSE is Copyright of Miklos Szeredi <miklos@szeredi.hu>
*/
#include "fur_utils.h"
#include "power.h"
#include "macros.h"
#include <stdlib.h>
#include <string.h>
#include <rapi2.h>

int power_message_size;

void power_init(IRAPISession *session)
{
  char *tmp=power_message(session);
  if(tmp==NULL) {
    ERR("Power init failed! (strange indeed...)\n");
    exit(1);
  }
  power_message_size=strlen(tmp);
  free(tmp);
}

int get_power_file_size(void)
{
  return power_message_size;
}


int read_power_status(IRAPISession *session, char *buf,size_t size,off_t offset)
{
  char *message=NULL;
  int maxlen;
  int toread;

  message=power_message(session);
  maxlen=strlen(message)-offset;
  if(maxlen<=0) {
    free(message);
    return 0;
  }
  toread=size<maxlen?size:maxlen;
  
  memcpy(buf,message+offset,toread);
  free(message);

  return toread;
}


// 6 chars
char *getACLineStatus(SYSTEM_POWER_STATUS_EX *d)
{
  return d->ACLineStatus?"online ":"offline";
}

// 11 chars
char *getBatteryFlag(SYSTEM_POWER_STATUS_EX *d)
{
  return d->BatteryFlag?"present    ":"not present";
}

int getBatteryLifePercent(SYSTEM_POWER_STATUS_EX *d)
{
  return d->BatteryLifePercent;
}

int getBatteryLifeTime(SYSTEM_POWER_STATUS_EX *d)
{
  return d->BatteryLifeTime;
}

int getBatteryFullLifeTime(SYSTEM_POWER_STATUS_EX *d)
{
  return d->BatteryFullLifeTime;
}

// 11 char
char *getBackupBatteryFlag(SYSTEM_POWER_STATUS_EX *d)
{
  return d->BackupBatteryFlag?"present    ":"not present";
}

int getBackupBatteryLifePercent(SYSTEM_POWER_STATUS_EX *d)
{
  return d->BackupBatteryLifePercent;
}

int getBackupBatteryLifeTime(SYSTEM_POWER_STATUS_EX *d)
{
  return d->BackupBatteryLifeTime;
}

int getBackupBatteryFullLifeTime(SYSTEM_POWER_STATUS_EX *d)
{
  return d->BackupBatteryFullLifeTime;
}

SYSTEM_POWER_STATUS_EX *GetPowerStatus(IRAPISession *session)
{
  SYSTEM_POWER_STATUS_EX *d;
  d=(SYSTEM_POWER_STATUS_EX *) calloc(1,sizeof(SYSTEM_POWER_STATUS_EX));
  
  IRAPISession_CeGetSystemPowerStatusEx(session, d,TRUE);
  return d;
}

#define MSG_SIZE 500
char *power_message(IRAPISession *session)
{
  char *result;

  SYSTEM_POWER_STATUS_EX *d=GetPowerStatus(session);
  
  if(d==NULL)
    return NULL;

  result=(char *) calloc(MSG_SIZE,sizeof(char));
  if(result==NULL)
    return NULL;

  sprintf(result,
	  "AC Line Status:                %7s\n"
	  "Battery:                       %11s\n"
	  "Battery Life:                  %3d%%\n"
          "Battery Life Time:             %3d%%\n"
          "Battery Full Life Time:        %3d%%\n"
          "Backup Battery Flag:           %11s\n"
          "Backup Battery Life:           %3d%%\n"
          "Backup Battery Life Time:      %3d%%\n"
          "Backup Battery Full Life Time: %3d%%\n",
	  getACLineStatus(d),
	  getBatteryFlag(d),
	  getBatteryLifePercent(d),
	  getBatteryLifeTime(d),
	  getBatteryFullLifeTime(d),
	  getBackupBatteryFlag(d),
	  getBackupBatteryLifePercent(d),
	  getBackupBatteryLifeTime(d),
	  getBackupBatteryFullLifeTime(d));

  IRAPISession_CeRapiFreeBuffer(session, d);

  return result;
}

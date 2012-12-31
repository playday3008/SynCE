/*

    Power functions for FUR: special "/proc/apm like" file
    FUR is Copyright (C) Riccardo Di Meo <riccardo@infis.univ.trieste.it>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.

    FUSE is Copyright of Miklos Szeredi <miklos@szeredi.hu>
*/
#ifndef POWER_H
#define POWER_H


#include <rapitypes.h>
#include "cache.h"

// Calculate the size of the power message file.
// (which *must* be constant through all the execution!)
void power_init(void);

// Extract power information from the structure
char *getACLineStatus(SYSTEM_POWER_STATUS_EX *d);
char *getBatteryFlag(SYSTEM_POWER_STATUS_EX *d);
int getBatteryLifePercent(SYSTEM_POWER_STATUS_EX *d);
int getBatteryLifeTime(SYSTEM_POWER_STATUS_EX *d);
int getBatteryFullLifeTime(SYSTEM_POWER_STATUS_EX *d);
char *getBackupBatteryFlag(SYSTEM_POWER_STATUS_EX *d);
int getBackupBatteryLifePercent(SYSTEM_POWER_STATUS_EX *d);
int getBackupBatteryLifeTime(SYSTEM_POWER_STATUS_EX *d);
int getBackupBatteryFullLifeTime(SYSTEM_POWER_STATUS_EX *d);

// Return a power status structure from the device
SYSTEM_POWER_STATUS_EX *GetPowerStatus(void);

// The size of the power message: 
int get_power_file_size(void);

int read_power_status(char *buf,size_t size,off_t offset);

// Return a message
char *power_message(void);

#endif

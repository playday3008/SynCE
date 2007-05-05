/* $Id$ */
#undef __STRICT_ANSI__
#define _GNU_SOURCE
#include "rapi2/rapi2_api.h"
#include "rapi_indirection.h"
#include <stdlib.h>
#if HAVE_CONFIG_H
#include "rapi_config.h"
#endif


/* The rapi2 application interface indirection */
struct rapi_ops_s rapi2_ops =
{
#ifndef SWIG
        &_CeCloseHandle2,                 /* CeCloseHandle */
        &_CeCreateFile2,                  /* CeCreateFile */
        &_CeReadFile2,                    /* CeReadFile */
        &_CeWriteFile2,                   /* CeWriteFile */
        &_CeSetFilePointer2,              /* CeSetFilePointer */
        &_CeSetEndOfFile2,                /* CeSetEndOfFile */
        &_CeGetFileTime2,                 /* CeGetFileTime */
        &_CeSetFileTime2,                 /* CeSetFileTime */
#endif /* SWIG */
        /*
         * File management functions
        */
#ifndef SWIG
        &_CeCopyFileA2,                   /* CeCopyFileA */
        &_CeCopyFile2,                    /* CeCopyFile */
        &_CeCreateDirectory2,             /* CeCreateDirectory */
        &_CeDeleteFile2,                  /* CeDeleteFile */
        &_CeFindAllFiles2,                /* CeFindAllFiles */
        &_CeFindFirstFile2,               /* CeFindFirstFile */
        NULL,                             /* CeFindNextFile */
        NULL,                             /* CeFindClose */
        &_CeGetFileAttributes2,           /* CeGetFileAttributes */
        &_CeGetFileSize2,                 /* CeGetFileSize */
        &_CeGetSpecialFolderPath2,        /* CeGetSpecialFolderPath */
        &_CeMoveFile2,                    /* CeMoveFile */
        &_CeRemoveDirectory2,             /* CeRemoveDirectory */
        NULL,                             /* CeSetFileAttributes */
        NULL,                             /* CeSHCreateShortcut */
        _CeSyncTimeToPc2,                 /* CeSyncTimeToPc */
#endif /* SWIG */
        /*
         * Database functions
        */
#ifndef SWIG
        NULL,                             /* CeCreateDatabase */
        NULL,                             /* CeDeleteDatabase */
        NULL,                             /* CeFindAllDatabases */
        NULL,                             /* CeFindFirstDatabase */
        NULL,                             /* CeFindNextDatabase */
        NULL,                             /* CeOpenDatabase */
        NULL,                             /* CeReadRecordProps */
        NULL,                             /* CeSeekDatabase */
        NULL,                             /* CeWriteRecordProps */
        NULL,                             /* CeDeleteRecord */
        NULL,                             /* CeSetDatabaseInfo */
#endif /* SWIG */
        /*
         * Registry
        */
#ifndef SWIG
        _CeRegCreateKeyEx2,               /* CeRegCreateKeyEx */
        _CeRegOpenKeyEx2,                 /* CeRegOpenKeyEx */
        _CeRegCloseKey2,                  /* CeRegCloseKey */
        _CeRegDeleteKey2,                 /* CeRegDeleteKey */
        _CeRegDeleteValue2,               /* CeRegDeleteValue */
        NULL,                             /* CeRegQueryInfoKey */
        _CeRegQueryValueEx2,              /* CeRegQueryValueEx */
        NULL,                             /* CeRegEnumValue */
        _CeRegEnumKeyEx2,                 /* CeRegEnumKeyEx */
        _CeRegSetValueEx2,                /* CeRegSetValueEx */
#endif /* SWIG */
        /*
         * Misc functions
        */
#ifndef SWIG
        NULL,                             /* CeCheckPassword */
        _CeCreateProcess2,                /* CeCreateProcess */
        _CeGetStoreInformation2,          /* CeGetStoreInformation */
        _CeGetSystemInfo2,                /* CeGetSystemInfo */
        _CeGetSystemPowerStatusEx2,       /* CeGetSystemPowerStatusEx */
        _CeGetVersionEx2,                 /* CeGetVersionEx */
        NULL,                             /* CeOidGetInfo */
        _CeProcessConfig2,                /* CeProcessConfig */
        _CeStartReplication2,             /* CeStartReplication */
        _CeSyncStart2,                    /* CeSyncStart */
        _CeSyncResume2,                   /* CeSyncResume */
        _CeSyncPause2,                    /* CeSyncPause */
        _CeGetSystemMemoryDivision2,      /* CeGetSystemMemoryDivision */
        NULL,                             /* CeSetSystemMemoryDivision */
        NULL,                             /* CeRegCopyFile */
        NULL,                             /* CeRegRestoreFile */
        NULL,                             /* CeKillAllApps */
#endif /* SWIG */
        /*
         * CeRapiInvoke stuff
        */
#ifndef SWIG
        NULL,                             /* IRAPIStream_Release */
        NULL,                             /* IRAPIStream_Read */
        NULL,                             /* IRAPIStream_Write */
        /* IRAPIStream_GetRawSocket, */   /* IRAPIStream_GetRawSocket */
        NULL,                             /* CeRapiInvoke */
        NULL,                             /* CeRapiInvokeA */
#endif /* SWIG */
};

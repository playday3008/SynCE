/* $Id$ */
#undef __STRICT_ANSI__
#define _GNU_SOURCE
#include "rapi/rapi_api.h"
#include "rapi_indirection.h"
#include <stdlib.h>
#if HAVE_CONFIG_H
#include "rapi_config.h"
#endif


/* The rapi application interface indirection */
struct rapi_ops_s rapi_ops =
{
#ifndef SWIG
        &_CeCloseHandle,                  /* CeCloseHandle */
        &_CeCreateFile,                   /* CeCreateFile */
        &_CeReadFile,                     /* CeReadFile */
        &_CeWriteFile,                    /* CeWriteFile */
        &_CeSetFilePointer,               /* CeSetFilePointer */
        &_CeSetEndOfFile,                 /* CeSetEndOfFile */
        NULL,                             /* CeGetFileTime */
        NULL,                             /* CeSetFileTime */
#endif /* SWIG */
        /*
         * File management functions
         */
#ifndef SWIG
        &_CeCopyFileA,                    /* CeCopyFileA */
        &_CeCopyFile,                     /* CeCopyFile */
        &_CeCreateDirectory,              /* CeCreateDirectory */
        &_CeDeleteFile,                   /* CeDeleteFile */
        &_CeFindAllFiles,                 /* CeFindAllFiles */
        &_CeFindFirstFile,                /* CeFindFirstFile */
        &_CeFindNextFile,                 /* CeFindNextFile */
        &_CeFindClose,                    /* CeFindClose */
        &_CeGetFileAttributes,            /* CeGetFileAttributes */
        &_CeGetFileSize,                  /* CeGetFileSize */
        &_CeGetSpecialFolderPath,         /* CeGetSpecialFolderPath */
        &_CeMoveFile,                     /* CeMoveFile */
        &_CeRemoveDirectory,              /* CeRemoveDirectory */
        &_CeSetFileAttributes,            /* CeSetFileAttributes */
        &_CeSHCreateShortcut,             /* CeSHCreateShortcut */
        &_CeSyncTimeToPc,                 /* CeSyncTimeToPc */
#endif /* SWIG */
        /*
         * Database functions
         */
#ifndef SWIG
        &_CeCreateDatabase,               /* CeCreateDatabase */
        &_CeDeleteDatabase,               /* CeDeleteDatabase */
        &_CeFindAllDatabases,             /* CeFindAllDatabases */
        &_CeFindFirstDatabase,            /* CeFindFirstDatabase */
        &_CeFindNextDatabase,             /* CeFindNextDatabase */
        &_CeOpenDatabase,                 /* CeOpenDatabase */
#if SIZEOF_VOID_P == 4
        &_CeReadRecordProps,              /* CeReadRecordProps */
#else
        NULL,                             /* CeReadRecordProps */
#endif
        &_CeSeekDatabase,                 /* CeSeekDatabase */
#if SIZEOF_VOID_P == 4
        &_CeWriteRecordProps,             /* CeWriteRecordProps */
#else
        NULL,                             /* CeWriteRecordProps */
#endif
        &_CeDeleteRecord,                 /* CeDeleteRecord */
        &_CeSetDatabaseInfo,              /* CeSetDatabaseInfo */
#endif /* SWIG */
        /*
         * Registry
         */
#ifndef SWIG
        &_CeRegCreateKeyEx,               /* CeRegCreateKeyEx */
        &_CeRegOpenKeyEx,                 /* CeRegOpenKeyEx */
        &_CeRegCloseKey,                  /* CeRegCloseKey */
        &_CeRegDeleteKey,                 /* CeRegDeleteKey */
        &_CeRegDeleteValue,               /* CeRegDeleteValue */
        &_CeRegQueryInfoKey,              /* CeRegQueryInfoKey */
        &_CeRegQueryValueEx,              /* CeRegQueryValueEx */
        &_CeRegEnumValue,                 /* CeRegEnumValue */
        &_CeRegEnumKeyEx,                 /* CeRegEnumKeyEx */
        &_CeRegSetValueEx,                /* CeRegSetValueEx */
#endif /* SWIG */
        /*
         * Misc functions
         */
#ifndef SWIG
        &_CeCheckPassword,                /* CeCheckPassword */
        &_CeCreateProcess,                /* CeCreateProcess */
        &_CeGetStoreInformation,          /* CeGetStoreInformation */
        &_CeGetSystemInfo,                /* CeGetSystemInfo */
        &_CeGetSystemPowerStatusEx,       /* CeGetSystemPowerStatusEx */
        &_CeGetVersionEx,                 /* CeGetVersionEx */
        &_CeOidGetInfo,                   /* CeOidGetInfo */
        &_CeProcessConfig,                /* CeProcessConfig */
        &_CeStartReplication,             /* CeStartReplication */
        NULL,                             /* CeSyncStart */
        NULL,                             /* CeSyncResume */
        NULL,                             /* CeSyncPause */
        &_CeGetSystemMemoryDivision,      /* CeGetSystemMemoryDivision */
        &_CeSetSystemMemoryDivision,      /* CeSetSystemMemoryDivision */
        &_CeRegCopyFile,                  /* CeRegCopyFile */
        &_CeRegRestoreFile,               /* CeRegRestoreFile */
        &_CeKillAllApps,                  /* CeKillAllApps */
        &_CeGetDiskFreeSpaceEx,            /* CeGetDiskFreeSpaceEx */
#endif /* SWIG */
        /*
         * CeRapiInvoke stuff
         */
#ifndef SWIG
        &_IRAPIStream_Release,            /* IRAPIStream_Release */
        &_IRAPIStream_Read,               /* IRAPIStream_Read */
        &_IRAPIStream_Write,              /* IRAPIStream_Write */
        /* &IRAPIStream_GetRawSocket, */     /* IRAPIStream_GetRawSocket */
        &_CeRapiInvoke,                   /* CeRapiInvoke */
        &_CeRapiInvokeA,                  /* CeRapiInvokeA */
#endif /* SWIG */
};

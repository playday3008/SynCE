/* $Id$ */
#ifndef __inno_h__
#define __inno_h__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zlib.h>

#define P __attribute__((packed))

typedef struct
{
  FILE* F;
  long StartPos;
  long InBytesLeft;
  uint8_t InBuffer[4096];
  uint8_t OutBuffer[4096];
  int InBufferCount;
  int OutBufferStart;
  int OutBufferCount;
  z_stream strm;
  bool Compressed;
  bool NoMoreData;
} TDeflateBlockReadData;

typedef struct
{
  uint32_t CompressedSize;
  uint32_t UncompressedSize;
} TNewBlockDeflateHeader;

typedef struct
{
  uint32_t ID;
  uint32_t OffsetTableOffset;
  uint32_t NotOffsetTableOffset;
} TSetupLdrExeHeader;

typedef struct
{
  uint32_t TotalSize;
  uint32_t OffsetEXE;
  uint32_t CompressedSizeEXE;
  uint32_t UncompressedSizeEXE;
  uint32_t AdlerEXE;
  uint32_t OffsetMsg;
  uint32_t Offset0;     /* "Inno Setup Setup Data..." */
  uint32_t Offset1;     /* file data */
} TSetupLdrOffsetTable;


#define SetupLdrExeHeaderOffset 0x30
#define SetupLdrExeHeaderID 0x6F6E6E49

#define UninstallerMsgTailID 0x67734D49


#define SETUP_ID_SIZE     64
#define SETUP_ID_FORMAT   "Inno Setup Setup Data (%i.%i.%i)"

#define SetupHeaderStrings        21
#define SetupLangOptionsStrings   5

typedef struct
{
  char* AppName;
  char* AppVerName;
  char* AppId;
  char* AppCopyright;
  char* AppPublisher;
  char* AppPublisherURL;
  char* AppSupportURL;
  char* AppUpdatesURL;
  char* AppVersion;
  char* DefaultDirName;
  char* DefaultGroupName;
  char* BaseFilename;
  char* LicenseText;
  char* InfoBeforeText;
  char* InfoAfterText;
  char* UninstallFilesDir;
  char* UninstallDisplayName;
  char* UninstallDisplayIcon;
  char* AppMutex;
  char* DefaultUserInfoName;
  char* DefaultUserInfoOrg;
  char LeadBytes[0x1c];
  uint32_t NumTypeEntries; 
  uint32_t NumComponentEntries; 
  uint32_t NumTaskEntries;
  uint32_t NumDirEntries; 
  uint32_t NumFileEntries; 
  uint32_t NumFileLocationEntries; 
  uint32_t NumIconEntries;
  uint32_t NumIniEntries; 
  uint32_t NumRegistryEntries; 
  uint32_t NumInstallDeleteEntries;
  uint32_t NumUninstallDeleteEntries; 
  uint32_t NumRunEntries; 
  uint32_t NumUninstallRunEntries;  
  P uint8_t stuff[0x35];
} TSetupHeader;

#define SETUP_HEADER_SIZE   0xd9

typedef struct
{
 char* LanguageName;
 char* DialogFontName;
 char* TitleFontName;
 char* WelcomeFontName;
 char* CopyrightFontName;
 uint32_t LanguageID;
 uint32_t DialogFontSize;
 uint32_t DialogFontStandardHeight;
 uint32_t TitleFontSize;
 uint32_t WelcomeFontSize;
 uint32_t CopyrightFontSize;
} TSetupLangOptions;

enum
{
  SetupTypeEntryStrings = 2,
  SetupComponentEntryStrings = 3,
  SetupTaskEntryStrings = 4,
  SetupDirEntryStrings = 3,
  SetupFileEntryStrings = 5,
  SetupFileLocationEntryStrings = 0,
  SetupIconEntryStrings = 8,
  SetupIniEntryStrings = 6,
  SetupRegistryEntryStrings = 5,
  SetupDeleteEntryStrings = 3,
  SetupRunEntryStrings = 8
};

typedef enum
{
  seType, seComponent, seTask, seDir, seFile, seFileLocation,
  seIcon, seIni, seRegistry, seInstallDelete, seUninstallDelete, seRun,
  seUninstallRun
} TEntryType;

typedef struct
{
  P uint32_t WinVersion;
  P uint32_t NTVersion;
  P uint16_t Word;
} TSetupVersionData;

#if 0
typedef enum
{
  toIsCustom = 1
} TSetupTypeOptions;

typedef struct
{
  char* Name;
  char *Description;
  TSetupVersionData MinVersion;
  TSetupVersionData OnlyBelowVersion;
  TSetupTypeOptions Options;
  uint32_t Size;
} TSetupTypeEntry; 
#endif

typedef struct
{
    char* SourceFilename; 
    char* DestName; 
    char* InstallFontName;
    char* Components; 
    char* Tasks;
    P TSetupVersionData MinVersion;
    P TSetupVersionData OnlyBelowVersion;
    uint32_t LocationEntry;
    uint32_t Attribs;
    uint32_t ExternalSize;
    P char stuff[4];
#if 0
    Options: set of (foConfirmOverwrite, foUninsNeverUninstall, foRestartReplace,
      foDeleteAfterInstall, foRegisterServer, foRegisterTypeLib, foSharedFile,
      foCompareTimeStamp, foFontIsntTrueType,
      foSkipIfSourceDoesntExist, foOverwriteReadOnly, foOverwriteSameVersion,
      foCustomDestName, foOnlyIfDestFileExists, foNoRegError,
      foUninsRestartDelete, foOnlyIfDoesntExist, foIgnoreVersion,
      foPromptIfOlder);
    FileType: (ftUserFile, ftUninstExe, ftRegSvrExe);
#endif
} TSetupFileEntry;

typedef struct
{
  uint32_t FirstDisk;
  uint32_t LastDisk;
  uint32_t StartOffset;
  uint32_t OriginalSize;
  uint32_t CompressedSize;
  uint32_t Adler;
  uint32_t Date;
  uint32_t FileVersionMS;
  uint32_t FileVersionLS;
} P TSetupFileLocationEntry;

#define SETUP_FILE_LOCATION_ENTRY_SIZE  0x29

typedef struct
{
  char* Name;
  char* Parameters; 
  char* WorkingDir; 
  char* RunOnceId;
  char* StatusMsg;
  char* Description;
  char* Components;
  char* Tasks;
#if 0
  TSetupVersionData MinVersion;
  TSetupVersionData OnlyBelowVersion;
  uint32_t ShowCmd;
#endif
#if 0
Wait: (rwWaitUntilTerminated, rwNoWait, rwWaitUntilIdle);
Options: set of (roShellExec, roSkipIfDoesntExist,
             roPostInstall, roUnchecked, roSkipIfSilent, roSkipIfNotSilent,
             roHideWizard);
#endif
} TSetupRunEntry;

#undef P

#endif

/* $Id$ */
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* dev_name = NULL;

static void show_usage(const char* name)
{
  fprintf(stderr,
      "Syntax:\n"
      "\n"
      "\t%s [-d LEVEL] [-p DEVNAME] [-h]\n"
      "\n"
      "\t-d LEVEL     Set debug log level\n"
      "\t                0 - No logging (default)\n"
      "\t                1 - Errors only\n"
      "\t                2 - Errors and warnings\n"
      "\t                3 - Everything\n"
      "\t-h           Show this help message\n"
      "\t-p DEVNAME   Mobile device name\n",    
      name);
}

static bool handle_parameters(int argc, char** argv)
{
  int c;
  int log_level = SYNCE_LOG_LEVEL_LOWEST;

  while ((c = getopt(argc, argv, "d:hp:")) != -1)
  {
    switch (c)
    {
      case 'd':
        log_level = atoi(optarg);
        break;

      case 'p':
        dev_name = optarg;
        break;

      case 'h':
      default:
        show_usage(argv[0]);
        return false;
    }
  }

  synce_log_set_level(log_level);

  return true;
}

static const char* version_string(CEOSVERSIONINFO* version)
{
  const char* result = "Unknown";

  if (version->dwMajorVersion == 4)
  {
    if (version->dwMinorVersion == 20 && version->dwBuildNumber == 1081)
      result = "Ozone: Pocket PC 2003 (?)";
    else if (version->dwMinorVersion == 21 && version->dwBuildNumber == 1088)
      result = "Microsoft Windows Mobile 2003 Pocket PC Phone Edition";
  } 
  else if (version->dwMajorVersion == 3 &&
      version->dwMinorVersion == 0)
  {
    switch (version->dwBuildNumber)
    {
      case 9348:  result = "Rapier: Pocket PC"; break;
      case 11171: result = "Merlin: Pocket PC 2002"; break;

      /* 
       * From:     Jonathan McDowell
       * To:       SynCE-Devel
       * Subject:  Re: [Synce-devel] Smartphone & installing CABs.
       * Date: 	   Mon, 26 May 2003 19:12:10 +0100  (20:12 CEST)
       */
      case 12255: result = "Stinger: Smart Phone 2002"; break;

      /* My Qtek 7070 */
      case 13121: result = "Stinger: Smart Phone 2002"; break;
    }
  }
  else if (version->dwMajorVersion == 2 &&
      version->dwMinorVersion == 1)
  {
    result = 
      "Gryphon: Windows CE for P/PC V1 (Palm-size PC)"
      " / "
      "Apollo: Windows CE for A/PC V1 (Auto PC)";
  }

  return result;
}

#define PROCESSOR_ARCHITECTURE_COUNT 8

static const char* architecture[] = {
  "Intel",
  "MIPS",
  "Alpha",
  "PPC",
  "SHX",
  "ARM",
  "IA64",
  "ALPHA64"
};

static const char* processor(int n)
{
  const char* result;

  switch (n)
  {
    case PROCESSOR_STRONGARM:    result = "StrongARM";  break;
    case PROCESSOR_MIPS_R4000:   result = "MIPS R4000"; break;
    case PROCESSOR_HITACHI_SH3:  result = "SH3";        break;

    default: result = "Unknown"; break;
  }

  return result;
}

#if 0
static void print_flag(unsigned flags, unsigned flag, const char*name, bool* first)
{
  if (flags & flag)
  {
    if (*first)
      *first = false;
    else
      putchar(' ');

    printf("%s", name);
  }
}
#endif

static const char* get_battery_flag_string(unsigned flag)
{
  const char* name;

  switch (flag)
  {
    case BATTERY_FLAG_HIGH:        name = "High";       break;
    case BATTERY_FLAG_LOW:         name = "Low";        break;
    case BATTERY_FLAG_CRITICAL:    name = "Critical";   break;
    case BATTERY_FLAG_CHARGING:    name = "Charging";   break;
    case BATTERY_FLAG_NO_BATTERY:  name = "NoBattery";  break;

    default: name = "Unknown"; break;
  }

  return name;
}

static const char* get_ACLineStatus_string(unsigned ACLineStatus)
{
  const char* status;

  switch (ACLineStatus)
  {
    case AC_LINE_OFFLINE:       status = "Offline";       break;
    case AC_LINE_ONLINE:        status = "Online";        break;
    case AC_LINE_BACKUP_POWER:  status = "Backup Power";  break;
    case AC_LINE_UNKNOWN:       status = "Unknown";       break;
    default:                    status = "Invalid";       break;
  }

  return status;
}

void print_battery_status(const char* name, unsigned flag, unsigned lifePercent, unsigned lifeTime, unsigned fullLifeTime)
{
  printf(
      "\nStatus for %s battery\n"
      "=========================\n"
      "Flag:          %i (%s)\n"
      ,
      name,
      flag,
      get_battery_flag_string(flag));

  printf("LifePercent:   ");
  if (BATTERY_PERCENTAGE_UNKNOWN == lifePercent)
    printf("Unknown\n");
  else
    printf("%i%%\n", lifePercent);

  printf("LifeTime:      ");
  if (BATTERY_LIFE_UNKNOWN == lifeTime)
    printf("Unknown\n");
  else
    printf("%i\n", lifeTime);

  printf("FullLifeTime:  ");
  if (BATTERY_LIFE_UNKNOWN == fullLifeTime)
    printf("Unknown\n");
  else
    printf("%i\n", fullLifeTime);


}

int main(int argc, char** argv)
{
  int result = 1;
  RapiConnection* connection = NULL;
  HRESULT hr;
  CEOSVERSIONINFO version;
  SYSTEM_INFO system;
  SYSTEM_POWER_STATUS_EX power;
  STORE_INFORMATION store;
  DWORD storage_pages = 0, ram_pages = 0, page_size = 0;

  if (!handle_parameters(argc, argv))
    goto exit;

  if ((connection = rapi_connection_from_name(dev_name)) == NULL)
  {
    fprintf(stderr, "%s: Could not obtain connection to device '%s'\n", 
        argv[0],
        dev_name?dev_name:"(Default)");
    goto exit;
  }
  rapi_connection_select(connection);
  hr = CeRapiInit();

  if (FAILED(hr))
  {
    fprintf(stderr, "%s: Unable to initialize RAPI: %s\n", 
        argv[0],
        synce_strerror(hr));
    goto exit;
  }

  /*
   * Version
   */

  memset(&version, 0, sizeof(version));
  version.dwOSVersionInfoSize = sizeof(version);

  if (CeGetVersionEx(&version))
  {
    char *details = wstr_to_current(version.szCSDVersion);
    char *platform = NULL;

    if (!details) {
            fprintf(stderr, "%s: Failed to convert version info to current encoding\n", argv[0]);
            details = strdup("");
    }

    if (VER_PLATFORM_WIN32_CE == version.dwPlatformId)
      platform = "(Windows CE)";

    printf(
        "Version\n"
        "=======\n"
        "Version:    %i.%i.%i (%s)\n"
        "Platform:   %i %s\n"
        "Details:    \"%s\"\n"
        "\n"
        ,
        version.dwMajorVersion,
        version.dwMinorVersion,
        version.dwBuildNumber,
        version_string(&version),
        version.dwPlatformId,
        platform ? platform : "",
        details
        );

    wstr_free_string(details);
  }
  else
  {
    fprintf(stderr, "%s: Failed to get version information: %s\n", 
        argv[0],
        synce_strerror(CeGetLastError()));
  }

  /*
   * System
   */

  memset(&system, 0, sizeof(system));

  CeGetSystemInfo(&system);
  {
    printf(
        "System\n"
        "======\n"
        "Processor architecture: %i (%s)\n"
        "Processor type:         %i (%s)\n"
        "Page size:              0x%x\n"
        "\n"
        ,
        system.wProcessorArchitecture,
        (system.wProcessorArchitecture < PROCESSOR_ARCHITECTURE_COUNT) ?
        architecture[system.wProcessorArchitecture] : "Unknown",
        system.dwProcessorType,
        processor(system.dwProcessorType),
        system.dwAllocationGranularity

        );
  }

  /*
   * Power
   */

  memset(&power, 0, sizeof(SYSTEM_POWER_STATUS_EX));

  if (CeGetSystemPowerStatusEx(&power, false))
  {
    printf(
        "Power\n"
        "=====\n"
        );

    printf("ACLineStatus: %02x (%s)\n", 
        power.ACLineStatus, get_ACLineStatus_string(power.ACLineStatus));

    print_battery_status("main", power.BatteryFlag, power.BatteryLifePercent,
        power.BatteryLifeTime, power.BatteryFullLifeTime);

    print_battery_status("backup", power.BackupBatteryFlag,
        power.BackupBatteryLifePercent, power.BackupBatteryLifeTime,
        power.BackupBatteryFullLifeTime);

    printf("\n");

  }
  else
  {
    fprintf(stderr, "%s: Failed to get battery status: %s\n", 
        argv[0],
        synce_strerror(CeGetLastError()));
  }

  /*
   * Store
   */
  memset(&store, 0, sizeof(store));

  if (CeGetStoreInformation(&store))
  {
    printf(
        "Store\n"
        "=====\n"
        "Store size: %i bytes (%i megabytes)\n"
        "Free space: %i bytes (%i megabytes)\n"
        "\n"
        ,
        store.dwStoreSize, store.dwStoreSize / (1024*1024),
        store.dwFreeSize,  store.dwFreeSize  / (1024*1024) 
        );
  }
  else
  {
    fprintf(stderr, "%s: Failed to get store information: %s\n", 
        argv[0],
        synce_strerror(CeGetLastError()));
  }

  if (CeGetSystemMemoryDivision(&storage_pages, &ram_pages, &page_size))
  {
    printf(
        "Memory for storage: %i bytes (%i megabytes)\n"
        "Memory for RAM:     %i bytes (%i megabytes)\n"
        "\n",
        storage_pages * page_size, storage_pages * page_size / (1024*1024),
        ram_pages     * page_size, ram_pages     * page_size / (1024*1024));
  }

  result = 0;

exit:
  CeRapiUninit();
  rapi_connection_destroy(connection);
  return result;
}

/* $Id$ */
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void show_usage(const char* name)
{
	fprintf(stderr,
			"Syntax:\n"
			"\n"
			"\t%s [-d LEVEL] [-h]\n"
			"\n"
			"\t-d LEVEL  Set debug log level\n"
			"\t              0 - No logging (default)\n"
			"\t              1 - Errors only\n"
			"\t              2 - Errors and warnings\n"
			"\t              3 - Everything\n"
			"\t-h        Show this help message\n",
			name);
}

static bool handle_parameters(int argc, char** argv)
{
	int c;
	int log_level = SYNCE_LOG_LEVEL_LOWEST;

	while ((c = getopt(argc, argv, "d:h")) != -1)
	{
		switch (c)
		{
			case 'd':
				log_level = atoi(optarg);
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
		case PROCESSOR_STRONGARM: result = "StrongARM"; break;

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
		case AC_LINE_OFFLINE: status = "Offline"; break;
		case AC_LINE_ONLINE: status = "Online"; break;
		case AC_LINE_BACKUP_POWER: status = "Backup Power"; break;
		case AC_LINE_UNKNOWN: status = "Unknown"; break;
		default: status = "Invalid"; break;
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
	HRESULT hr;
	CEOSVERSIONINFO version;
	SYSTEM_INFO system;
	SYSTEM_POWER_STATUS_EX power;
	STORE_INFORMATION store;
	
	if (!handle_parameters(argc, argv))
		goto exit;

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
		char *details = wstr_to_ascii(version.szCSDVersion);
		char *platform = NULL;

		if (VER_PLATFORM_WIN32_CE == version.dwPlatformId)
			platform = "(Windows CE)";

		printf(
				"Version\n"
				"=======\n"
				"Version:    %i.%i build %i\n"
				"Platform:   %i %s\n"
				"Details:    \"%s\"\n"
				"\n"
				,
				version.dwMajorVersion,
				version.dwMinorVersion,
				version.dwBuildNumber,
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

	result = 0;

exit:
	CeRapiUninit();
	return result;
}

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

int main(int argc, char** argv)
{
	int result = 1;
	HRESULT hr;
	CEOSVERSIONINFO version;
	SYSTEM_INFO system;
	
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

	memset(&system, 0, sizeof(system));

	CeGetSystemInfo(&system);
	{
		printf(
				"System\n"
				"======\n"
				"Processor architecture: %i (%s)\n"
				"Processor type:         %i (%s)\n"
				"Page size:              0x%x\n"
				,
				system.wProcessorArchitecture,
				(system.wProcessorArchitecture < PROCESSOR_ARCHITECTURE_COUNT) ?
				architecture[system.wProcessorArchitecture] : "Unknown",
				system.dwProcessorType,
				processor(system.dwProcessorType),
				system.dwAllocationGranularity
				
				);
	}

	result = 0;

exit:
	CeRapiUninit();
	return result;
}

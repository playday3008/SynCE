#include "test.h"

#define PROCESSOR_HITACHI_SH3   10003   // Windows CE
#define PROCESSOR_HITACHI_SH3E  10004   // Windows CE
#define PROCESSOR_HITACHI_SH4   10005   // Windows CE
#define PROCESSOR_MOTOROLA_821  821     // Windows CE
#define PROCESSOR_SHx_SH3       103     // Windows CE
#define PROCESSOR_SHx_SH4       104     // Windows CE
#define PROCESSOR_STRONGARM     2577    // Windows CE - 0xA11
#define PROCESSOR_ARM720        1824    // Windows CE - 0x720
#define PROCESSOR_ARM820        2080    // Windows CE - 0x820
#define PROCESSOR_ARM920        2336    // Windows CE - 0x920
#define PROCESSOR_ARM_7TDMI     70001   // Windows CE

int main()
{
	VERIFY_HRESULT(CeRapiInit());

	SYSTEM_INFO system_info;
	
	memset(&system_info, 0, sizeof(SYSTEM_INFO));
	printf("sizeof(SYSTEM_INFO) = %i\n", sizeof(SYSTEM_INFO));
	
	// This message will be generated but it should be ok:
	// "Overflow by 4 bytes. Parameter size is 36 bytes but max 32 bytes was expected."
	CeGetSystemInfo(&system_info);
	
	printf(
			"dwPageSize=%i\n"
			"lpMinimumApplicationAddress=%p\n"
			"lpMaximumApplicationAddress=%p\n"
			"dwActiveProcessorMask=%i\n"
			"dwNumberOfProcessors=%i\n"
			"dwProcessorType=%i\n"
			"dwAllocationGranularity=%i\n"
			"wProcessoOverflow byrLevel=%i\n"
			"wProcessorRevision=%i\n"
			"\n",
			system_info.dwPageSize,
			system_info.lpMinimumApplicationAddress,
			system_info.lpMaximumApplicationAddress,
			system_info.dwActiveProcessorMask,
			system_info.dwNumberOfProcessors,
			system_info.dwProcessorType,
			system_info.dwAllocationGranularity,
			system_info.wProcessorLevel,
			system_info.wProcessorRevision
			);

	switch (system_info.dwProcessorType)
	{
		case PROCESSOR_STRONGARM:
			printf("That's a StrongARM processor!\n");
			break;
		default:
			break;
	}
	
	// Sanity checks here
	
	if (0 == system_info.dwPageSize)
	{
		printf("Error: Page size cannot be 0\n");
		return TEST_FAILED;
	}
	
	if (0 == system_info.dwNumberOfProcessors)
	{
		printf("Error: Number of processors cannot be 0\n");
		return TEST_FAILED;
	}
	
	VERIFY_HRESULT(CeRapiUninit());
	return TEST_SUCCEEDED;
}


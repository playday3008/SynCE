#include "test.h"

int main()
{
	VERIFY_HRESULT(CeRapiInit());

	DWORD length;
	WCHAR dirname[MAX_PATH];
	VERIFY_NOT_FALSE(length = CeGetSpecialFolderPath(CSIDL_PERSONAL, MAX_PATH, dirname));
	wcscat(dirname, to_unicode("\\librapi test directory"));
	
	// XXX: not yet implemented in librapi
#ifdef WIN32
	TEST_NOT_EQUAL(0, CeCreateDirectory(dirname, NULL));
#else
	printf("not yet implemented in librapi\n");
#endif
	
	VERIFY_HRESULT(CeRapiUninit());
	return TEST_SUCCEEDED;
}


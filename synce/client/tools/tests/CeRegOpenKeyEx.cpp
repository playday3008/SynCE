#include "test.h"

int main()
{
	VERIFY_HRESULT(CeRapiInit());

	HKEY hkey;

	WCHAR * clsid = to_unicode("CLSID");
	TEST_EQUAL(ERROR_SUCCESS, CeRegOpenKeyEx(HKEY_CLASSES_ROOT, clsid, 0, 0, &hkey));
	TEST_EQUAL(ERROR_SUCCESS, CeRegCloseKey(hkey));

#if 0
	TEST_EQUAL(ERROR_SUCCESS, CeRegOpenKeyEx(HKEY_CLASSES_ROOT, NULL, 0, 0, &hkey));
	TEST_EQUAL(ERROR_SUCCESS, CeRegCloseKey(hkey));
#endif
	
	VERIFY_HRESULT(CeRapiUninit());
	return TEST_SUCCEEDED;
}


#include "test.h"

int main()
{
	VERIFY_HRESULT(CeRapiInit());

	DWORD index = 0;
	LONG result;

	for(;;)
	{
		WCHAR keyname[MAX_PATH];
		DWORD keyname_size = MAX_PATH;
		WCHAR classname[MAX_PATH];
		DWORD classname_size = MAX_PATH;
		FILETIME filetime;

		// XXX: class name parameters not yet supported
		result = CeRegEnumKeyEx(HKEY_CURRENT_USER, index, keyname, 
				&keyname_size, 0, classname, &classname_size, &filetime);

		if (ERROR_SUCCESS != result)
		{
			break;
		}
		
		printf("Key %li: name=\"%s\"\n", index, from_unicode(keyname));
		
		index++;
	}
	
	if (ERROR_NO_MORE_ITEMS != result)
	{
		printf("Error: %li\n", result);
		return TEST_FAILED;
	}

	VERIFY_HRESULT(CeRapiUninit());
	return TEST_SUCCEEDED;
}


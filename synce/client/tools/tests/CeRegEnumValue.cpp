#include "test.h"

int main()
{
	VERIFY_HRESULT(CeRapiInit());

	HKEY hkey;

	WCHAR * subkey = to_unicode("David");
	VERIFY_EQUAL(ERROR_SUCCESS, CeRegOpenKeyEx(HKEY_CURRENT_USER, subkey, 0, 0, &hkey));

	DWORD index = 0;
	LONG result;

	for(;;)
	{
		WCHAR valuename[MAX_PATH];
		DWORD valuename_size = MAX_PATH;
		BYTE data[MAX_PATH];
		DWORD data_size = MAX_PATH;
		DWORD type;

		result = CeRegEnumValue(hkey, index, valuename, 
				&valuename_size, NULL, &type, data, &data_size);

		if (ERROR_SUCCESS != result)
		{
			break;
		}
		else
		{
			printf("Value %li: name=\"%s\", name_size=%i, type=0x%lx, data_size=%lu\n", 
					index, from_unicode(valuename), valuename_size, type, data_size);
			
			index++;
		}
	}
	
	if (ERROR_NO_MORE_ITEMS != result)
	{
		printf("Error: %li\n", result);
		return TEST_FAILED;
	}

	VERIFY_EQUAL(ERROR_SUCCESS, CeRegCloseKey(hkey));
	VERIFY_HRESULT(CeRapiUninit());
	return TEST_SUCCEEDED;
}


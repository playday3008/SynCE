#include "test.h"

int main()
{
	VERIFY_HRESULT(CeRapiInit());

	HKEY hkey;

	// This test will fail without this subkey... :-)
	WCHAR * subkey = to_unicode("Comm");
	VERIFY_EQUAL(ERROR_SUCCESS, CeRegOpenKeyEx(HKEY_LOCAL_MACHINE, subkey, 0, 0, &hkey));

	WCHAR classname[MAX_PATH];
	DWORD classname_size = MAX_PATH;
	DWORD subkey_count = 0;
	DWORD max_subkey_name_size = 0;
	DWORD max_subkey_class_size = 0;
	DWORD value_count = 0;
	DWORD max_value_name_size = 0;
	DWORD max_value_data_size = 0;
	FILETIME filetime;

	classname[0] = 0;
	
	TEST_EQUAL(ERROR_SUCCESS, CeRegQueryInfoKey(hkey, classname,
				&classname_size, NULL, &subkey_count, &max_subkey_name_size, 
				&max_subkey_class_size, &value_count, &max_value_name_size,
			  &max_value_data_size, NULL, &filetime	));

	printf("Classname=\"%s\", subkey_count=%lu, max_subkey_name_size=%lu, "
			"max_subkey_class_size=%lu, value_count=%lu, max_value_name_size=%lu, "
			"max_value_data_size=%lu\n", 
			from_unicode(classname), subkey_count, max_subkey_name_size,
			max_subkey_class_size, value_count, max_value_name_size,
			max_value_data_size);

	VERIFY_EQUAL(ERROR_SUCCESS, CeRegCloseKey(hkey));
	VERIFY_HRESULT(CeRapiUninit());
	return TEST_SUCCEEDED;
}


#include "test.h"

int main()
{
	VERIFY_HRESULT(CeRapiInit());


	DWORD length;
	WCHAR filename[MAX_PATH];
	VERIFY_NOT_FALSE(length = CeGetSpecialFolderPath(CSIDL_PERSONAL, MAX_PATH, filename));
	wcscat(filename, to_unicode("\\librapi.txt"));
	
	HANDLE handle;

	// Open file for writing, create if it does not exist
	TEST_NOT_EQUAL(INVALID_HANDLE_VALUE, handle = CeCreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));

	// Write data to file
	char* write_buffer = "Hej hopp!\r\n";
	const DWORD should_write = strlen(write_buffer);
	DWORD did_write = (DWORD)-1;
	TEST_NOT_FALSE(CeWriteFile(handle, write_buffer, should_write, &did_write, NULL));
	if (should_write != did_write)
	{
		printf("Only wrote %u bytes to file of %u possible.\n", did_write, should_write);
		return TEST_FAILED;
	}

	// Close file
	TEST_NOT_FALSE(CeCloseHandle(handle));

	// Open file for reading, fail if it does not exist
	TEST_NOT_EQUAL(INVALID_HANDLE_VALUE, handle = CeCreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
	
	// Read data from file
	char read_buffer[MAX_PATH];
	DWORD did_read = (DWORD)-1;
	TEST_NOT_FALSE(CeReadFile(handle, read_buffer, MAX_PATH, &did_read, NULL));
	if (did_read != did_write)
	{
		printf("Wrote %u bytes but read %u.\n", did_write, did_read);
		return TEST_FAILED;
	}
	
	read_buffer[did_read] = 0;
	if (strcmp(write_buffer, read_buffer) != 0)
	{
		printf("Wrote \"%s\" but read \"%s\".\n", write_buffer, read_buffer);
		return TEST_FAILED;
	}
	
	// Close file
	TEST_NOT_FALSE(CeCloseHandle(handle));

	// Delete file
	// XXX: not implemented
	//TEST_NOT_FALSE(CeDeleteFile(filename));
	
	VERIFY_HRESULT(CeRapiUninit());
	return TEST_SUCCEEDED;
}


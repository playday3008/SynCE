//
// pput - PocketPUT
//
// Copies a file to a Pocket PC handheld
//
#include <malloc.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include "rapi.h"
#include "rapi_wstr.h"

//
// Return values from main()
//
#define TEST_SUCCEEDED 0
#define TEST_FAILED 1

//
// HRESULT tests
// 
#define VERIFY_HRESULT(call) \
if (FAILED((call))) { printf("FAIL.\n"); return TEST_FAILED; }

#define TEST_HRESULT(call) \
printf("Testing %s...", #call); \
VERIFY_HRESULT(call) else printf("ok.\n");

//
// Test return value
//
#define VERIFY_EQUAL(value, call) \
if ((value) != (call)) { printf("FAIL.\n"); return TEST_FAILED; }

#define TEST_EQUAL(value, call) \
printf("Testing %s...", #call); \
VERIFY_EQUAL((value), (call)) else printf("ok.\n");


//
// Test return value
//
#define VERIFY_NOT_EQUAL(value, call) \
if ((value) == (call)) { printf("FAIL.\n"); return TEST_FAILED; }

#define TEST_NOT_EQUAL(value, call) \
printf("Testing %s...", #call); \
VERIFY_NOT_EQUAL((value), (call)) else printf("ok.\n");


//
// Test to verify that the return value is zero
//
#define VERIFY_NOT_FALSE(call) VERIFY_NOT_EQUAL(0, (call))
#define TEST_NOT_FALSE(call) TEST_NOT_EQUAL(0, call)


int main(int argc, char**argv)
{
	if (2 != argc)
	{	
		fprintf(stderr, "Syntax:\n"
				"\t%s source-file\n"
				"\n"
				"Copy source-file to the \"My Documents\" directory on the Pocket PC\n", 
				argv[0]);
		return 1;
	}

	char* source_file = strdup(argv[1]);
	
	VERIFY_HRESULT(CeRapiInit());

	DWORD length;
	WCHAR dest_file[MAX_PATH];
	VERIFY_NOT_FALSE(length = CeGetSpecialFolderPath(CSIDL_PERSONAL, MAX_PATH, dest_file));
	rapi_wstr_append(dest_file, rapi_wstr_from_ascii("\\"), sizeof(dest_file));
	rapi_wstr_append(dest_file, rapi_wstr_from_ascii(basename(source_file)), sizeof(dest_file));
	printf("Remote filename: \"%s\"\n", rapi_wstr_to_ascii(dest_file));
	
	FILE* source = fopen(source_file, "r");
	if (!source)
	{
		return 1;
	}
			
	free(source_file);

	HANDLE handle;

	// Open file for writing, create if it does not exist
	TEST_NOT_EQUAL(INVALID_HANDLE_VALUE, handle = CeCreateFile(dest_file,
				GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0));
	
#define MYBUF_SIZE (16*1024)
	char * buffer = new char[MYBUF_SIZE];
	
	size_t bytes_read;
	while (0 != (bytes_read = fread(buffer, 1, MYBUF_SIZE, source)))
	{
		DWORD did_write = (DWORD)-1;
		TEST_NOT_FALSE(CeWriteFile(handle, buffer, bytes_read, &did_write, NULL));
		if (bytes_read != did_write)
		{
			printf("Only wrote %u bytes to file of %u possible.\n", did_write, bytes_read);
		}
		printf(".");
	}
	printf("\n");

	delete buffer;

	// Close files
	fclose(source);
	TEST_NOT_FALSE(CeCloseHandle(handle));

	VERIFY_HRESULT(CeRapiUninit());
	return TEST_SUCCEEDED;
}


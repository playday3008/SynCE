//
// pget - PocketGET
//
// Copies a file from a Pocket PC handheld
//
// Modified version of pput.
//
// Contributed by Richard Taylor <synce@rest.clara.co.uk>
//
#undef __STRICT_ANSI__
#define __USE_GNU
#include <stdlib.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include "rapi.h"

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
				"Copy source-file from the \"My Documents\" directory on the Pocket PC\n",
				argv[0]);
		return 1;
	}

	char* source_file = strdup(argv[1]);

	VERIFY_HRESULT(CeRapiInit());

	DWORD length;
	WCHAR source_path[MAX_PATH];
	VERIFY_NOT_FALSE(length = CeGetSpecialFolderPath(CSIDL_PERSONAL, MAX_PATH, source_path));
	wstr_append(source_path, wstr_from_ascii("\\"), sizeof(source_path));
	wstr_append(source_path, wstr_from_ascii(basename(source_file)), sizeof(source_path));
	printf("Remote filename: \"%s\"\n", wstr_to_ascii(source_path));

	FILE* dest = fopen(source_file, "w");
	if (!dest)
	{
		return 1;
	}

	free(source_file);

	HANDLE handle;

	// Open file for writing, create if it does not exist
	TEST_NOT_EQUAL(INVALID_HANDLE_VALUE, handle = CeCreateFile(source_path,
				GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));

#define MYBUF_SIZE (16*1024)
	char * buffer = new char[MYBUF_SIZE];

	size_t bytes_read;
	while (1)
	{
		TEST_NOT_FALSE(CeReadFile(handle, buffer, MYBUF_SIZE, &bytes_read, NULL));
		if (bytes_read == 0) break;
		size_t did_write =(DWORD)-1;
		did_write = fwrite(buffer, 1, bytes_read, dest);
		if (bytes_read != did_write)
		{
			printf("Only wrote %u bytes to file of %u possible.\n", did_write, bytes_read);
		}
		printf(".");
	}
	printf("\n");

	delete buffer;

	// Close files
	fclose(dest);
	TEST_NOT_FALSE(CeCloseHandle(handle));

	VERIFY_HRESULT(CeRapiUninit());
	return TEST_SUCCEEDED;
}


//
// pcp - PocketCoPy
//
// Copies a file to a Pocket PC handheld
//
#include <malloc.h>
#include <libgen.h>
#include "test.h"

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
	wcscat(dest_file, to_unicode("\\"));
	wcscat(dest_file, to_unicode(basename(source_file)));
	printf("Remote filename: \"%s\"\n", from_unicode(dest_file));
	
	FILE* source = fopen(source_file, "r");
	if (!source)
	{
		return 1;
	}
			
	free(source_file);

	HANDLE handle;

	// Open file for writing, create if it does not exist
	TEST_NOT_EQUAL(INVALID_HANDLE_VALUE, handle = CeCreateFile(dest_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
	
#define MYBUF_SIZE (16*1024)
	char * buffer = new char[MYBUF_SIZE];
	
	size_t bytes_read;
	while (0 != (bytes_read = fread(buffer, 1, MYBUF_SIZE, source)))
	{
		DWORD did_write = (DWORD)-1;
		TEST_NOT_FALSE(CeWriteFile(handle, buffer, bytes_read, &did_write, NULL));
		if (bytes_read != did_write)
		{
			printf("Only wrote %lu bytes to file of %u possible.\n", did_write, bytes_read);
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


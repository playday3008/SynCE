/* $Id$ */
#include "pcommon.h"
#include "rapi.h"
#include <synce_log.h>
#include <stdio.h>

void convert_to_backward_slashes(char* path)
{
	while (*path)
	{
		if ('/' == *path)
			*path = '\\';

		path++;
	}
}

/**
 * See if a filename is remote
 */
bool is_remote_file(const char* filename)
{
	return filename && ':' == filename[0];
}

static const WCHAR wide_backslash[] = {'\\', '\0'};

WCHAR* adjust_remote_path(WCHAR* old_path, bool free_path)
{
	/* Nothing to adjust if we have an absolute path */
	if ('\\' == old_path[0])
		return old_path;

	WCHAR path[MAX_PATH];
	if (!CeGetSpecialFolderPath(CSIDL_PERSONAL, sizeof(path), path))
	{
		fprintf(stderr, "Unable to get the \"My Documents\" path.\n");
		return NULL;
	}

	wstr_append(path, wide_backslash, sizeof(path));
	wstr_append(path, old_path, sizeof(path));

	if (free_path)
		wstr_free_string(old_path);

	synce_trace_wstr(path);
	return wstrdup(path);
}


typedef void (*ANYFILE_CLOSE)(AnyFile* file);
typedef bool (*ANYFILE_ACCESSOR)(AnyFile* file, unsigned char* buffer, size_t bytes, size_t* bytesAccessed);

struct _AnyFile
{
	union
	{
		HANDLE remote;
		FILE*  local;
	} handle;
	
	ANYFILE_ACCESSOR read;
	ANYFILE_ACCESSOR write;
	ANYFILE_CLOSE close;
		
};

static void anyfile_remote_close(AnyFile* file)
{
	CeCloseHandle(file->handle.remote);
}

static bool anyfile_remote_read(AnyFile* file, unsigned char* buffer, size_t bytes, size_t* bytesAccessed)
{
	return CeReadFile(file->handle.remote, buffer, bytes, bytesAccessed, NULL);
}

static bool anyfile_remote_write(AnyFile* file, unsigned char* buffer, size_t bytes, size_t* bytesAccessed)
{
	return CeWriteFile(file->handle.remote, buffer, bytes, bytesAccessed, NULL);
}

static void anyfile_local_close(AnyFile* file)
{
	if (file->handle.local)
		fclose(file->handle.local);
}

static bool anyfile_local_read(AnyFile* file, unsigned char* buffer, size_t bytes, size_t* bytesAccessed)
{
	*bytesAccessed = fread(buffer, 1, bytes, file->handle.local);
	return !ferror(file->handle.local);
}

static bool anyfile_local_write(AnyFile* file, unsigned char* buffer, size_t bytes, size_t* bytesAccessed)
{
	*bytesAccessed = fwrite(buffer, 1, bytes, file->handle.local);
	return !ferror(file->handle.local);
}


/**
 * Open remote file for reading or writing
 */
static AnyFile* anyfile_remote_open(const char* filename, ANYFILE_ACCESS access)
{
	WCHAR* wide_filename = wstr_from_ascii(filename);
	AnyFile* file = (AnyFile*)calloc(1, sizeof(AnyFile));

	switch (access)
	{
		case READ:  
			file->handle.remote = CeCreateFile(wide_filename, GENERIC_READ, 0, NULL, 
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			break;

		case WRITE: 
			file->handle.remote = CeCreateFile(wide_filename, GENERIC_WRITE, 0, NULL,
					CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
			break;
	}
	
	if (INVALID_HANDLE_VALUE == file->handle.remote)
	{
		synce_error("Failed to open file '%s': %s", 
				filename, synce_strerror(CeGetLastError()));
		free(file);
		file = NULL;
	}
	else
	{
		file->close = anyfile_remote_close;
		file->write = anyfile_remote_write;
		file->read  = anyfile_remote_read;
	}

	wstr_free_string(wide_filename);
	return file;
}

/**
 * Open local file for reading or writing
 */
static AnyFile* anyfile_local_open(const char* filename, ANYFILE_ACCESS access)
{
	AnyFile* file = (AnyFile*)calloc(1, sizeof(AnyFile));

	switch (access)
	{
		case READ:  
			file->handle.local = fopen(filename, "r");
			break;

		case WRITE: 
			file->handle.local = fopen(filename, "w");
			break;
	}
	
	if (NULL == file->handle.local)
	{
		free(file);
		file = NULL;
	}
	else
	{
		file->close = anyfile_local_close;
		file->write = anyfile_local_write;
		file->read  = anyfile_local_read;
	}

	return file;
}

/**
 * Open file
 */
AnyFile* anyfile_open(char* filename, ANYFILE_ACCESS access)
{
	AnyFile* file = NULL;
	
	if (is_remote_file(filename))
	{
		convert_to_backward_slashes(filename);
		file = anyfile_remote_open(filename + 1, access);
	}
	else
	{
		file = anyfile_local_open(filename, access);
	}

	return file;
}

void anyfile_close(AnyFile* file)
{
	(*file->close)(file);
}

bool anyfile_read(AnyFile* file, unsigned char* buffer, size_t bytes, size_t* bytesAccessed)
{
	return (*file->read)(file, buffer, bytes, bytesAccessed);
}

bool anyfile_write(AnyFile* file, unsigned char* buffer, size_t bytes, size_t* bytesAccessed)
{
	return (*file->write)(file, buffer, bytes, bytesAccessed);
}


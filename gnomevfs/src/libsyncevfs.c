#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <rapi.h>
#include <synce_log.h>

#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libgnomevfs/gnome-vfs-module.h>

typedef struct _DIR_HANDLE
{
	char *location;
	CE_FIND_DATA *data;
	int itemcount;
	int count;
} DIR_HANDLE;

static char* g_convert_to_backward_slashes(GnomeVFSURI *uri)
{
	char *path;
	char *tpath;
	char *location;
	int i;
	char a, b;
	int num;

	path = strdup(gnome_vfs_uri_get_path(uri));
	location = path;

	printf("g_convert_to_backward_slashes()\n");
	printf("unconverted: %s\n", location);

	if (gnome_vfs_uri_get_user_name(uri) != NULL
	 || gnome_vfs_uri_get_password(uri) != NULL
	 || gnome_vfs_uri_get_host_name(uri) != NULL
	 || gnome_vfs_uri_get_host_port(uri) != 0)
		return NULL;

	for(i = 0; i < strlen(path); i++)
	{
		if(path[i] == '/')
		{
			path[i] = '\\';
		}
		else if(path[i] == '%')
		{
			a = path[i+1]-48;
			b = path[i+2]-48;
			num = a*10+b;
			switch(num)
			{
				case 20:
					path[i] = ' ';
/*				default:
					return NULL;*/
			}
			tpath = strdup(path+(i+3));
			printf("tpath = %s\n", tpath);
			path[i+1] = '\0';
			strcat(path, tpath);
			free(tpath);
		}
	}
	printf("converted: %s\n", location);

	return location;
}

static int vfs_to_synce_mode(GnomeVFSOpenMode mode, int *open_mode, int *create_mode)
{
	if(mode & GNOME_VFS_OPEN_READ)
	{
		if(mode & GNOME_VFS_OPEN_WRITE)
		{
			*create_mode = CREATE_NEW;
			*open_mode = GENERIC_READ & GENERIC_WRITE;
		}
		else
		{
			*create_mode = OPEN_EXISTING;
			*open_mode = GENERIC_READ;
		}
	} 
	else
	{
		*create_mode = CREATE_NEW;
		*open_mode = GENERIC_WRITE;
	}
	return 0;
}

static GnomeVFSResult synce_open
(
	GnomeVFSMethod *method,
  GnomeVFSMethodHandle **method_handle_return,
  GnomeVFSURI *uri,
  GnomeVFSOpenMode mode,
  GnomeVFSContext *context)
{
	char *location = NULL;
	WCHAR *wide_path = NULL;
	int synce_open_mode;
	int synce_create_mode;
	HANDLE handle;
	char *tempstring;

	printf("--------------------------------------------\n");
	printf("synce_open()\n");

	location = g_convert_to_backward_slashes(uri);

	printf("location: %s\n", location);

	wide_path = wstr_from_ascii(location);

	tempstring = wstr_to_ascii(wide_path);
	printf("ajusted location: %s\n", tempstring); /* XXX: memory leak */
	free(tempstring);

	vfs_to_synce_mode(mode, &synce_open_mode, &synce_create_mode);

	printf("CeCreateFile()\n");
	handle = CeCreateFile
	(
		wide_path,
		synce_open_mode,
		0,
		NULL,
		synce_create_mode,
		FILE_ATTRIBUTE_NORMAL,
		0
	);

	free(location);
	wstr_free_string(wide_path);

	*((HANDLE *)method_handle_return) = handle;

	printf("Synce Error: %d %s\n", CeGetLastError(), synce_strerror(CeGetLastError()));
	printf("--------------------------------------------\n");
	if((handle == INVALID_HANDLE_VALUE) || (synce_open_mode & GENERIC_WRITE))
	{
		switch(CeGetLastError())
		{
			case 0:
				return GNOME_VFS_OK;
			case 2:
			case 3:
			case 123:
				printf("Failed\n");
				return GNOME_VFS_ERROR_INVALID_URI;
			case 80:
				printf("Failed\n");
				return GNOME_VFS_ERROR_FILE_EXISTS;
			default:
				printf("Failed\n");
				return GNOME_VFS_ERROR_GENERIC;
		}
	}
	else
	{
		return GNOME_VFS_OK;
	}
}

static GnomeVFSResult synce_create
(
	GnomeVFSMethod *method,
  GnomeVFSMethodHandle **method_handle_return,
  GnomeVFSURI *uri,
  GnomeVFSOpenMode mode,
  gboolean exclusive,
  guint perm,
  GnomeVFSContext *context
)
{
	char *location = NULL;
	WCHAR *wide_path = NULL;
	int synce_open_mode;
	int synce_create_mode;
	HANDLE handle;
	char *tempstring;

	printf("--------------------------------------------\n");
	printf("synce_create()\n");

	location = g_convert_to_backward_slashes(uri);
	printf("location: %s\n", location);

	wide_path = wstr_from_ascii(location);
	tempstring = wstr_to_ascii(wide_path);
	printf("wide_path: %s\n", tempstring);
	free(tempstring);

	vfs_to_synce_mode(mode, &synce_open_mode, &synce_create_mode);

	printf("CeCreateFile()");
	handle = CeCreateFile
	(
		wide_path,
		synce_open_mode,
		0,
		NULL,
		synce_create_mode,
		FILE_ATTRIBUTE_NORMAL,
		0
	);

	free(location);
	wstr_free_string(wide_path);

	*((HANDLE *)method_handle_return) = handle;

	printf("Synce Error: %d %s\n", CeGetLastError(), synce_strerror(CeGetLastError()));
	printf("--------------------------------------------\n");
	if((handle == INVALID_HANDLE_VALUE) || (synce_open_mode & GENERIC_WRITE))
	{
		switch(CeGetLastError())
		{
			case 0:
				return GNOME_VFS_OK;
			case 2:
			case 3:
			case 123:
				printf("Failed\n");
				return GNOME_VFS_ERROR_INVALID_URI;
			case 80:
				printf("Failed\n");
				return GNOME_VFS_ERROR_FILE_EXISTS;
			default:
				printf("Failed\n");
				return GNOME_VFS_ERROR_GENERIC;
		}
	}
	else
	{
		return GNOME_VFS_OK;
	}
}

static GnomeVFSResult synce_close
(
	GnomeVFSMethod *method,
  GnomeVFSMethodHandle *method_handle,
  GnomeVFSContext *context
)
{
	HANDLE handle;
	int result;

	printf("--------------------------------------------\n");
	printf("synce_close()\n");

	handle = (HANDLE) method_handle;

	printf("CeCloseHandle()\n");
	result = CeCloseHandle(handle);

	printf("Synce Error: %d %s\n", CeGetLastError(), synce_strerror(CeGetLastError()));
	printf("--------------------------------------------\n");
	if(!result)
		return GNOME_VFS_ERROR_GENERIC;
	else
		return GNOME_VFS_OK;
}

static GnomeVFSResult synce_read
(
	GnomeVFSMethod *method,
  GnomeVFSMethodHandle *method_handle,
  gpointer buffer,
  GnomeVFSFileSize num_bytes,
  GnomeVFSFileSize *bytes_read_return,
  GnomeVFSContext *context
)
{
	int result;
	size_t read_return;
	HANDLE handle;

	printf("--------------------------------------------\n");
	printf("synce_read()\n");

	handle = (HANDLE) method_handle;

	printf("CeReadFile\n");
	result = CeReadFile
	(
		handle,
		buffer,
		num_bytes,
		&read_return,
		NULL
	);

	printf("Synce Error: %d %s\n", CeGetLastError(), synce_strerror(CeGetLastError()));
	printf("--------------------------------------------\n");
	if(result == 0)
	{
		switch(CeGetLastError())
		{
			case 0:
				return GNOME_VFS_OK;
			case 3:
				printf("Failed\n");
				return GNOME_VFS_ERROR_INVALID_URI;
			case 80:
				printf("Failed\n");
				return GNOME_VFS_ERROR_FILE_EXISTS;
		}
	}
	else if((result != 0) & (read_return == 0))
	{
		printf("Failed\n");
		return GNOME_VFS_ERROR_EOF;
	}
	else
	{
		*bytes_read_return = read_return;
		return GNOME_VFS_OK;
	}
}

static GnomeVFSResult synce_write
(
	GnomeVFSMethod *method,
  GnomeVFSMethodHandle *method_handle,
  gconstpointer buffer,
  GnomeVFSFileSize num_bytes,
  GnomeVFSFileSize *bytes_written_return,
	GnomeVFSContext *context
)
{
	int result;
	HANDLE handle;
	size_t bytes_written;

	printf("--------------------------------------------\n");
	printf("synce_write()\n");

	handle = (HANDLE) method_handle;

	printf("CeWriteFile()\n");
	result = CeWriteFile
	(
		handle,
		buffer,
		num_bytes,
		&bytes_written,
		NULL
	);

	printf("Synce Error: %d %s\n", CeGetLastError(), synce_strerror(CeGetLastError()));
	printf("--------------------------------------------\n");
	if(result == 0)
	{
		printf("Failed\n");
		return GNOME_VFS_ERROR_GENERIC;
	}
	else if((result != 0) && (bytes_written == 0))
	{
		printf("End of file\n");
		*bytes_written_return = 0;
		return GNOME_VFS_ERROR_EOF;
	}
	else
	{
		*bytes_written_return = bytes_written;
		return GNOME_VFS_OK;
	}
}

static GnomeVFSResult synce_open_dir
(
	GnomeVFSMethod *method,
  GnomeVFSMethodHandle **method_handle,
  GnomeVFSURI *uri,
  GnomeVFSFileInfoOptions options,
  GnomeVFSContext *context
)
{
	char *location;
	char *new_path;
	DIR_HANDLE *dh;
	CE_FIND_DATA *data = NULL;
	int optionflags;
	int itemcount;
	WCHAR *tempwstr;
	
	printf("--------------------------------------------\n");
	printf("synce_open_dir()\n");

	location = g_convert_to_backward_slashes(uri);

	printf("location: %s\n", location);

	if(!location)
	{
		location = strdup("*");
	}
	else if(location[1] == '\0')
	{
		free(location);
		location = strdup("*");
	}
	else if(location[strlen(location)-1] == '\\')
	{
		new_path = (char *)malloc(sizeof(char)*(strlen(location)+2));
		strcpy(new_path, location);
		new_path[strlen(location)] = '*';
		new_path[strlen(location)+1] = '\0';
		free(location);
		location = new_path;
	}
	else
	{
		new_path = (char *)malloc(sizeof(char)*(strlen(location)+3));
		strcpy(new_path, location);
		new_path[strlen(location)] = '\\';
		new_path[strlen(location)+1] = '*';
		new_path[strlen(location)+2] = '\0';
		free(location);
		location = new_path;
	}

	optionflags =	
			FAF_ATTRIBUTES
		| FAF_CREATION_TIME
		| FAF_LASTACCESS_TIME
		| FAF_LASTWRITE_TIME
		| FAF_NAME
		|	FAF_SIZE_LOW
		|	FAF_OID;

	
	tempwstr = wstr_from_ascii(location);
	if(!CeFindAllFiles(tempwstr, optionflags, &itemcount, &data))
	{
		free(location);
		wstr_free_string(tempwstr);
		printf("Synce Error: %d %s\n", CeGetLastError(), synce_strerror(CeGetLastError()));
		printf("Fail\n");
		printf("--------------------------------------------\n");
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	wstr_free_string(tempwstr);

	dh = (DIR_HANDLE*) malloc(sizeof(DIR_HANDLE));

	dh->location = location;
	dh->itemcount = itemcount;
	dh->count = 0;
	dh->data = data;

	*((DIR_HANDLE **) method_handle) = dh;

	printf("Synce Error: %d %s\n", CeGetLastError(), synce_strerror(CeGetLastError()));
	printf("Ok\n");
	printf("--------------------------------------------\n");

	return GNOME_VFS_OK;
}

static GnomeVFSResult synce_close_dir
(
	GnomeVFSMethod *method,
  GnomeVFSMethodHandle *method_handle,
  GnomeVFSContext *context
)
{
	DIR_HANDLE *dh;
	HRESULT result; 

	printf("--------------------------------------------\n");
	printf("synce_close_dir()\n");

	dh = (DIR_HANDLE *)method_handle;

	free(dh->location);
	dh->data;
	result = CeRapiFreeBuffer(dh->data);

	if(result != S_OK)
	{
		printf("Failed\n");
		printf("--------------------------------------------\n");
		return GNOME_VFS_ERROR_GENERIC;
	}
	else
	{
		printf("Ok\n");
		printf("--------------------------------------------\n");
		return GNOME_VFS_OK;
	}
}

/* Have to fix the mime-type conversion */
static BOOL get_file_attributes
(
	GnomeVFSFileInfo *file_info,
	CE_FIND_DATA *entry
)
{
	(int)file_info->valid_fields =
			GNOME_VFS_FILE_INFO_FIELDS_TYPE
		| GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS
		| GNOME_VFS_FILE_INFO_FIELDS_FLAGS
		| GNOME_VFS_FILE_INFO_FIELDS_SIZE
		| GNOME_VFS_FILE_INFO_FIELDS_ATIME
		| GNOME_VFS_FILE_INFO_FIELDS_MTIME
		| GNOME_VFS_FILE_INFO_FIELDS_CTIME;

	file_info->name = wstr_to_ascii(entry->cFileName);

	if(entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		file_info->size = 0;
	else
		file_info->size = entry->nFileSizeLow;

	file_info->atime = filetime_to_unix_time(&entry->ftLastAccessTime);
	file_info->mtime = filetime_to_unix_time(&entry->ftLastWriteTime);
	file_info->ctime = filetime_to_unix_time(&entry->ftCreationTime);;

	(int) file_info->permissions = 0664;

	if(entry->dwFileAttributes & FILE_ATTRIBUTE_READONLY)
		(int) file_info->permissions |= 0222;

	if(entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		(int) file_info->permissions |= 0111;
		file_info->type = GNOME_VFS_FILE_TYPE_DIRECTORY;
		file_info->mime_type = g_strdup("x-directory/normal");
	}
	else
	{
		file_info->type = GNOME_VFS_FILE_TYPE_REGULAR;
		file_info->mime_type = (char *)
			gnome_vfs_mime_type_from_name_or_default
			((const char *)file_info->name, 
			 (const char *)GNOME_VFS_MIME_TYPE_UNKNOWN);
		file_info->mime_type = g_strdup(file_info->mime_type);
	}

	file_info->uid = getuid();
	file_info->gid = getgid();

	return TRUE;
}

static void get_root_attributes
(	
	GnomeVFSFileInfo *file_info
)
{
	(int)file_info->valid_fields =
			GNOME_VFS_FILE_INFO_FIELDS_TYPE
		| GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS
		| GNOME_VFS_FILE_INFO_FIELDS_FLAGS
		| GNOME_VFS_FILE_INFO_FIELDS_SIZE;

	file_info->name = strdup("My Device");

	file_info->size = 0;

	(int) file_info->permissions = 0775;

	file_info->type = GNOME_VFS_FILE_TYPE_DIRECTORY;
	file_info->mime_type = g_strdup("x-directory/normal");

	file_info->uid = getuid();
	file_info->gid = getgid();
}

static GnomeVFSResult synce_read_dir
(
	GnomeVFSMethod *method,
  GnomeVFSMethodHandle *method_handle,
  GnomeVFSFileInfo *file_info,
  GnomeVFSContext *context
)
{
	DIR_HANDLE *dh;
	HANDLE handle;
	BOOL result;

	printf("--------------------------------------------\n");
	printf("synce_read_dir()\n");

	dh = (DIR_HANDLE *)method_handle;

	if(dh->itemcount == dh->count)
	{
		printf("Synce Error: %d %s\n", 38, "End of file");
		printf("Ok\n");
		printf("--------------------------------------------\n");
		return GNOME_VFS_ERROR_EOF;
	}

	result = get_file_attributes(file_info, dh->data+dh->count);
	dh->count++;

	printf("Error %d: %s\n", 0, "Success");

	if(!result)
	{
		printf("Failed\n");
		printf("--------------------------------------------\n");
		return GNOME_VFS_ERROR_CORRUPTED_DATA;
	}
	else
	{
		printf("Name: %s\n", file_info->name);
		printf("Mime-type: %s\n", file_info->mime_type);
		printf("Ok\n");
		printf("--------------------------------------------\n");
		return GNOME_VFS_OK;
	}
}

static GnomeVFSResult synce_get_file_info
(
	GnomeVFSMethod *method,
  GnomeVFSURI *uri,
  GnomeVFSFileInfo *file_info,
  GnomeVFSFileInfoOptions options,
  GnomeVFSContext *context
)
{
	gchar *location;
	WCHAR *wide_path;
	CE_FIND_DATA entry;
	HANDLE handle;
	int optionflags;
	WCHAR *tempwstr;

	printf("--------------------------------------------\n");
	printf("synce_get_file_info()\n");

	location = g_convert_to_backward_slashes(uri);

	printf("%s\n", location);

	if(!location)
	{
			printf("Synce Error: %d %s\n", 2, "ERROR_FILE_NOT_FOUND");
			printf("Failed\n");
			printf("--------------------------------------------\n");
			return GNOME_VFS_ERROR_INVALID_URI;
	}
	else if(strcmp(location, "\\") == 0)
	{
		printf("Root folder\n");

		get_root_attributes(file_info);

		printf("Ok\n");
		printf("--------------------------------------------\n");
		return GNOME_VFS_OK;
	}
	else if(location[strlen(location)-1] == '\\')
	{
		printf("Folder with \\\n");
		/* This is a directory, chop of the \ to make it readable to FindFirstFile */
		location[strlen(location)-1] = '\0';
	}
	else
	{
		printf("Folder/File\n");
	}

	printf("CeFindFirstFile()\n");
	tempwstr = wstr_from_ascii(location);
	if(CeFindFirstFile(tempwstr, &entry) == INVALID_HANDLE_VALUE)
	{
		printf("Error %d: %s\n", CeGetLastError(), synce_strerror(CeGetLastError()));

		wstr_free_string(tempwstr);
		free(location);

		printf("Failed\n");
		printf("--------------------------------------------\n");

		switch(CeGetLastError())
		{
			case 2:
			case 18:
				return GNOME_VFS_ERROR_NOT_FOUND;
			case 123:
				return GNOME_VFS_ERROR_INVALID_URI;
			default:
				return GNOME_VFS_ERROR_GENERIC;
		}
	}
	else
	{
		get_file_attributes(file_info, &entry);

		wstr_free_string(tempwstr);
		free(location);

		printf("Name: %s\n", file_info->name);
		printf("Mime-type: %s\n", file_info->mime_type);
		printf("Ok\n");
		printf("--------------------------------------------\n");

		return GNOME_VFS_OK;
	}
}

static GnomeVFSResult synce_get_file_info_from_handle
(
	GnomeVFSMethod *method,
  GnomeVFSMethodHandle *method_handle,
  GnomeVFSFileInfo *file_info,
  GnomeVFSFileInfoOptions options,
  GnomeVFSContext *context
)
{
//	printf("synce_get_files_info_from_handle\n");

	return GNOME_VFS_ERROR_ACCESS_DENIED;
}

static gboolean synce_is_local
(
	GnomeVFSMethod *method,
  const GnomeVFSURI *uri
)
{
//	printf("synce_is_local\n");

	return GNOME_VFS_OK;
}

static GnomeVFSResult synce_mkdir
(
	GnomeVFSMethod *method,
  GnomeVFSURI *uri,
	guint perm,
  GnomeVFSContext *context
)
{
	gchar *location;
	WCHAR *tempwstr;

	printf("--------------------------------------------\n");
	printf("synce_mkdir()\n");

	location = g_convert_to_backward_slashes(uri);

	if(!location)
	{
		printf("Synce Error: %d %s\n", 13, "ERROR_INVALID_DATA");
		printf("Failed\n");
		printf("--------------------------------------------\n");
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	printf("CeCreateDirectory()\n");
	tempwstr = wstr_from_ascii(location);
	if(!CeCreateDirectory(tempwstr, NULL))
	{
		free(location);
		wstr_free_string(tempwstr);
		printf("Error %d: %s\n", CeGetLastError(), synce_strerror(CeGetLastError()));
		printf("Failed\n");
		printf("--------------------------------------------\n");
		return GNOME_VFS_ERROR_FILE_EXISTS;
	}

	free(location);
	wstr_free_string(tempwstr);

	printf("Error %d: %s\n", CeGetLastError(), synce_strerror(CeGetLastError()));
	printf("Ok\n");
	printf("--------------------------------------------\n");

	return GNOME_VFS_OK;
}

static GnomeVFSResult synce_rmdir
(
	GnomeVFSMethod *method,
  GnomeVFSURI *uri,
  GnomeVFSContext *context
)
{
	gchar *location;
	WCHAR *tempwstr;

	printf("--------------------------------------------\n");
	printf("synce_rmdir()\n");

	location = g_convert_to_backward_slashes(uri);

	if(!location)
	{
		printf("Synce Error: %d %s\n", 13, "ERROR_INVALID_DATA");
		printf("Failed\n");
		printf("--------------------------------------------\n");
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	printf("CeRemoveDirectory()\n");
	tempwstr = wstr_from_ascii(location);
	CeRemoveDirectory(tempwstr);
	wstr_free_string(tempwstr);

	free(location);

	printf("Error %d: %s\n", CeGetLastError(), synce_strerror(CeGetLastError()));
	switch(CeGetLastError())
	{
		case 145:
			printf("Failed\n");
			printf("--------------------------------------------\n");
			return GNOME_VFS_ERROR_DIRECTORY_NOT_EMPTY;
		case 2:
			printf("Failed\n");
			printf("--------------------------------------------\n");
			return GNOME_VFS_ERROR_NOT_FOUND;
		case 0:
			printf("Ok\n");
			printf("--------------------------------------------\n");
			return GNOME_VFS_OK;
		default:
			printf("Failed\n");
			printf("--------------------------------------------\n");
			return GNOME_VFS_ERROR_GENERIC;
	}
}

static GnomeVFSResult synce_move
(
	GnomeVFSMethod *method,
  GnomeVFSURI *old_uri,
  GnomeVFSURI *new_uri,
  gboolean force_replace,
  GnomeVFSContext *context
)
{
	gchar *new_location;
	WCHAR *new_wstr;
	gchar *old_location;
	WCHAR *old_wstr;
	int result;

	printf("--------------------------------------------\n");
	printf("synce_move()\n");

	new_location = g_convert_to_backward_slashes(new_uri);

	if(!new_location)
	{
		printf("Failed\n");
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	old_location = g_convert_to_backward_slashes(old_uri);

	if(!old_location)
	{
		free(new_location);
		printf("Synce Error: %d %s\n", 13, "ERROR_INVALID_DATA");
		printf("Failed\n");
		printf("--------------------------------------------\n");
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	printf("CeMoveFile()\n");
	old_wstr = wstr_from_ascii(old_location);
	new_wstr = wstr_from_ascii(new_location);
	result = CeMoveFile(old_wstr, new_wstr);

	if(CeGetLastError() == 183) /* If the destination file exists we end up here */
	{
		if(force_replace)
		/* if the user wants we delete the dest file and moves the source there */
		{
			result = CeDeleteFile(new_wstr);
			if(result == 0)
			{
				free(old_location);
				free(new_location);
				wstr_free_string(old_wstr);
				wstr_free_string(new_wstr);
				printf("Synce Error: %d %s\n", 5, "ERROR_ACCESS_DENIED");
				printf("Failed\n");
				printf("--------------------------------------------\n");
				return GNOME_VFS_ERROR_ACCESS_DENIED;
			}
			result = CeMoveFile(old_wstr, new_wstr);
		}
	}

	free(old_location);
	free(new_location);
	wstr_free_string(old_wstr);
	wstr_free_string(new_wstr);

	printf("CeGetLastError(): %d\n", CeGetLastError());
	switch(CeGetLastError())
	{
		case 0:
		case 18:
			printf("Ok\n");
			printf("--------------------------------------------\n");
			return GNOME_VFS_OK;
		case 183:
			printf("Failed\n");
			printf("--------------------------------------------\n");
			return GNOME_VFS_ERROR_FILE_EXISTS;
		default:
			printf("Failed\n");
			printf("--------------------------------------------\n");
			return GNOME_VFS_ERROR_GENERIC;
	}
}

static GnomeVFSResult synce_unlink
(
	GnomeVFSMethod *method,
  GnomeVFSURI *uri,
  GnomeVFSContext *context
)
{
	gchar *location;
	int result;
	WCHAR *tempwstr;

	printf("--------------------------------------------\n");
	printf("synce_unlink()\n");

	location = g_convert_to_backward_slashes(uri);
	tempwstr = wstr_from_ascii(location);

	if(!location)
	{
		free(location);
		wstr_free_string(tempwstr);
		printf("Synce Error: %d %s\n", 13, "ERROR_INVALID_DATA");
		printf("Failed\n");
		printf("--------------------------------------------\n");
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	printf("CeDeleteFile()\n");
	result = CeDeleteFile(tempwstr);

	wstr_free_string(tempwstr);
	free(location);

	if(result == 0)
	{
		printf("Failed\n");
		printf("--------------------------------------------\n");
		return GNOME_VFS_ERROR_NOT_FOUND;
	}
	else
	{
		printf("Ok\n");
		printf("--------------------------------------------\n");
		return GNOME_VFS_OK;
	}
}

static GnomeVFSResult synce_same_fs
(
	GnomeVFSMethod *method,
  GnomeVFSURI *a,
  GnomeVFSURI *b,
  gboolean *same_fs_return,
  GnomeVFSContext *context
)
{
//	printf("synce_same_fs\n");

	*same_fs_return = 1;

	return GNOME_VFS_OK;
}

static GnomeVFSMethod method = 
{
	sizeof(GnomeVFSMethod),
	synce_open,
	synce_create,
	synce_close,
	synce_read,
	synce_write,
	NULL,	/* seek */
	NULL,	/* tell */
	NULL,	/* truncate */
	synce_open_dir,
	synce_close_dir,
	synce_read_dir,
	synce_get_file_info,
	synce_get_file_info_from_handle,
	synce_is_local,
	synce_mkdir,
	synce_rmdir,
	synce_move,
	synce_unlink,
	synce_same_fs,
	NULL, /* set_file_info */
	NULL,	/* truncate */
	NULL, /* find_directory */
	NULL, /* create_symbolic_link */
};

GnomeVFSMethod *vfs_module_init(const char *method_name, const char *args)
{
	HRESULT hr = CeRapiInit();
	
	if (FAILED(hr))
	{
		fprintf(stderr, "Unable to initialize RAPI: %s\n", 
				synce_strerror(hr));
		return NULL;
	}

	return &method;
}

void vfs_module_shutdown(GnomeVFSMethod *method)
{
	CeRapiUninit();
}

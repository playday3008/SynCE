#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <rapi.h>
#include <synce_log.h>

#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libgnomevfs/gnome-vfs-module.h>

#ifdef SYNCE_DEBUG
#define D(x...) printf(x)
#else
#define D(x...)
#endif

#ifdef G_THREADS_ENABLED
#define MUTEX_NEW()     g_mutex_new ()
#define MUTEX_FREE(a)   g_mutex_free (a)
#define MUTEX_LOCK(a)   if ((a) != NULL) g_mutex_lock (a)
#define MUTEX_UNLOCK(a) if ((a) != NULL) g_mutex_unlock (a)
#else
#define MUTEX_NEW()     NULL
#define MUTEX_FREE(a)
#define MUTEX_LOCK(a)
#define MUTEX_UNLOCK(a)
#endif

typedef struct _DIR_HANDLE
{
	char *location;
	CE_FIND_DATA *data;
	int itemcount;
	int count;
} DIR_HANDLE;

static GMutex * mutex = NULL;

static char* g_convert_to_backward_slashes(GnomeVFSURI *uri)
{
	char *path;
	char *tpath;
	char *location;
	int i;
	char a, b;
	int num;
	gchar *p;

	D("g_convert_to_backward_slashes()\n");
	D("unconverted: %s\n", gnome_vfs_uri_get_path(uri));
	path = (char *) gnome_vfs_unescape_string(gnome_vfs_uri_get_path(uri),"");
	location = path;

	if (gnome_vfs_uri_get_user_name(uri) != NULL
	 || gnome_vfs_uri_get_password(uri) != NULL
	 || gnome_vfs_uri_get_host_name(uri) != NULL
	 || gnome_vfs_uri_get_host_port(uri) != 0) {
		g_free(path);	
		return NULL;
	}
	
	p = path;
	while (*p != '\0') {
		if (*p == '/') {
			*p = '\\';
		}
		p = g_utf8_next_char (p);
	}

	D("converted: %s\n", location);

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
	int err;

	D("--------------------------------------------\n");
	D("synce_open()\n");

	location = g_convert_to_backward_slashes(uri);

	D("location: %s\n", location);

	wide_path = wstr_from_utf8(location);

	if (mode & GNOME_VFS_OPEN_RANDOM) {
		return GNOME_VFS_ERROR_INVALID_OPEN_MODE;
	}
		
	vfs_to_synce_mode(mode, &synce_open_mode, &synce_create_mode);

	D("CeCreateFile()\n");
	MUTEX_LOCK (mutex);
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
	MUTEX_UNLOCK (mutex);

	free(location);
	wstr_free_string(wide_path);

	*((HANDLE *)method_handle_return) = handle;

	MUTEX_LOCK (mutex);
	err = CeGetLastError();
	MUTEX_UNLOCK (mutex);
	
	D("Synce Error: %d %s\n", err, synce_strerror(err));
	D("--------------------------------------------\n");
	if((handle == INVALID_HANDLE_VALUE) || (synce_open_mode & GENERIC_WRITE))
	{
		switch(err)
		{
			case 0:
				return GNOME_VFS_OK;
			case 2:
			case 3:
			case 123:
				D("Failed\n");
				return GNOME_VFS_ERROR_INVALID_URI;
			case 80:
				D("Failed\n");
				return GNOME_VFS_ERROR_FILE_EXISTS;
			default:
				D("Failed\n");
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
	int err;

	D("--------------------------------------------\n");
	D("synce_create()\n");

	location = g_convert_to_backward_slashes(uri);
	D("location: %s\n", location);

	wide_path = wstr_from_utf8(location);
	tempstring = wstr_to_utf8(wide_path);
	D("wide_path: %s\n", tempstring);
	free(tempstring);

	vfs_to_synce_mode(mode, &synce_open_mode, &synce_create_mode);

	D("CeCreateFile()");
	MUTEX_LOCK (mutex);
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
	MUTEX_UNLOCK (mutex);

	free(location);
	wstr_free_string(wide_path);

	*((HANDLE *)method_handle_return) = handle;

	MUTEX_LOCK (mutex);
	err = CeGetLastError();
	MUTEX_UNLOCK (mutex);
	
	D("Synce Error: %d %s\n", err, synce_strerror(err));
	D("--------------------------------------------\n");
	if((handle == INVALID_HANDLE_VALUE) || (synce_open_mode & GENERIC_WRITE))
	{
		switch(err)
		{
			case 0:
				return GNOME_VFS_OK;
			case 2:
			case 3:
			case 123:
				D("Failed\n");
				return GNOME_VFS_ERROR_INVALID_URI;
			case 80:
				D("Failed\n");
				return GNOME_VFS_ERROR_FILE_EXISTS;
			default:
				D("Failed\n");
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
	int err;

	D("------------------- synce_close() -----------------------\n");

	handle = (HANDLE) method_handle;

	D("synce_close: CeCloseHandle()\n");
	MUTEX_LOCK (mutex);
	result = CeCloseHandle(handle);
	err = CeGetLastError();
	MUTEX_UNLOCK (mutex);

	
	D("synce_close: Synce Error: %d %s\n", err, synce_strerror(err));
	D("------------------- synce_close() end -------------------\n");

	if(!result) {
		return GNOME_VFS_ERROR_GENERIC;
	} else {
		return GNOME_VFS_OK;
	}
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
	int err;
	
	D("------------------ synce_read() ---------------------\n");
	D("synce_read()\n");

	handle = (HANDLE) method_handle;

	D("CeReadFile\n");
	MUTEX_LOCK (mutex);
	result = CeReadFile
	(
		handle,
		buffer,
		num_bytes,
		&read_return,
		NULL
	);
	err = CeGetLastError();
	MUTEX_UNLOCK (mutex);

	D("synce_read: Synce Error: %d %s\n", err, synce_strerror(err));
	D("------------------ synce_read() end ---------------------\n");
	if(result == 0)
	{
		switch(err)
		{
			case 0:
				return GNOME_VFS_OK;
			case 3:
				D("synce_read: Failed\n");
				return GNOME_VFS_ERROR_INVALID_URI;
			case 80:
				D("synce_read: Failed\n");
				return GNOME_VFS_ERROR_FILE_EXISTS;
		}
	}
	else if((result != 0) & (read_return == 0))
	{
		D("synce_read: Failed\n");
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
	int err; 

	D("----------------- synce_write() -------------------\n");

	handle = (HANDLE) method_handle;

	D("CeWriteFile()\n");
	MUTEX_LOCK (mutex);
	result = CeWriteFile
	(
		handle,
		buffer,
		num_bytes,
		&bytes_written,
		NULL
	);
	err = CeGetLastError();
	MUTEX_UNLOCK (mutex);

	D("synce_write: Synce Error: %d %s\n", err, synce_strerror(err));
	D("----------------- synce_write() end ---------------\n");
	if(result == 0)
	{
		D("synce_write: Failed\n");
		return GNOME_VFS_ERROR_GENERIC;
	}
	else if((result != 0) && (bytes_written == 0))
	{
		D("synce_write: End of file\n");
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
	
	D("------------------ synce_open_dir() -------------------\n");

	location = g_convert_to_backward_slashes(uri);

	D("synce_open_dir: location: %s\n", location);

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

	
	tempwstr = wstr_from_utf8(location);
	
	MUTEX_LOCK (mutex); 
	if(!CeFindAllFiles(tempwstr, optionflags, &itemcount, &data))
	{
		free(location);
		wstr_free_string(tempwstr);
		D("synce_open_dir: Synce Error: %d %s\n", CeGetLastError(), synce_strerror(CeGetLastError()));
		D("synce_open_dir: Fail\n");
		D("------------------ synce_open_dir() end ---------------\n");
		MUTEX_UNLOCK (mutex); 
		return GNOME_VFS_ERROR_INVALID_URI;
	}
	MUTEX_UNLOCK (mutex); 

	wstr_free_string(tempwstr);

	dh = (DIR_HANDLE*) malloc(sizeof(DIR_HANDLE));

	dh->location = location;
	dh->itemcount = itemcount;
	dh->count = 0;
	dh->data = data;

	*((DIR_HANDLE **) method_handle) = dh;

#ifdef SYNCE_DEBUG
	MUTEX_LOCK (mutex); 
	D("synce_open_dir: Synce Error: %d %s\n", CeGetLastError(), synce_strerror(CeGetLastError()));
	MUTEX_UNLOCK (mutex);
#endif
	D("synce_open_dir: Ok\n");
	D("------------------ synce_open_dir() end ---------------\n");

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

	D("----------------- synce_close_dir() ------------------\n");

	dh = (DIR_HANDLE *)method_handle;

	free(dh->location);

	MUTEX_LOCK (mutex); 
	result = CeRapiFreeBuffer(dh->data);
	MUTEX_UNLOCK (mutex); 

	if(result != S_OK)
	{
		D("synce_close_dir: Failed\n");
		D("----------------- synce_close_dir() end --------------\n");
		return GNOME_VFS_ERROR_GENERIC;
	}
	else
	{
		D("synce_close_dir: Ok\n");
		D("----------------- synce_close_dir() end --------------\n");
		return GNOME_VFS_OK;
	}
	free(dh);
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

	file_info->name = wstr_to_utf8(entry->cFileName);

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

	D("------------------ synce_read_dir() --------------------\n");

	dh = (DIR_HANDLE *)method_handle;

	if(dh->itemcount == dh->count)
	{
		D("synce_read_dir: Synce Error: %d %s\n", 38, "End of file");
		D("synce_read_dir: Ok\n");
		D("------------------ synce_read_dir() end ----------------\n");
		return GNOME_VFS_ERROR_EOF;
	}

	result = get_file_attributes(file_info, dh->data+dh->count);
	dh->count++;

	D("synce_read_dir: Error %d: %s\n", 0, "Success");

	if(!result)
	{
		D("synce_read_dir: Failed\n");
		D("------------------ synce_read_dir() end ----------------\n");
		return GNOME_VFS_ERROR_CORRUPTED_DATA;
	}
	else
	{
		D("synce_read_dir: Name: %s\n", file_info->name);
		D("synce_read_dir: Mime-type: %s\n", file_info->mime_type);
		D("synce_read_dir: Ok\n");
		D("------------------ synce_read_dir() end ----------------\n");
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
	int err;

	D("------------- synce_get_file_info() -----------------\n");

	location = g_convert_to_backward_slashes(uri);

	D("%s\n", location);

	if(!location)
	{
			D("synce_get_file_info Synce Error: %d %s\n", 2, "ERROR_FILE_NOT_FOUND");
			D("synce_get_file_info Failed\n");
			D("------------- synce_get_file_info() end --------------\n");
			return GNOME_VFS_ERROR_INVALID_URI;
	}
	else if(strcmp(location, "\\") == 0)
	{
		D("synce_get_file_info: Root folder\n");

		get_root_attributes(file_info);

		D("synce_get_file_info: Ok\n");
		D("------------- synce_get_file_info() end --------------\n");
		return GNOME_VFS_OK;
	}
	else if(location[strlen(location)-1] == '\\')
	{
		D("synce_get_file_info: Folder with \\\n");
		/* This is a directory, chop of the \ to make it readable to FindFirstFile */
		location[strlen(location)-1] = '\0';
	}
	else
	{
		D("synce_get_file_info: Folder/File\n");
	}

	D("synce_get_file_info: CeFindFirstFile()\n");
	tempwstr = wstr_from_utf8(location);
	
	MUTEX_LOCK (mutex); 
	if(CeFindFirstFile(tempwstr, &entry) == INVALID_HANDLE_VALUE)
	{
		err = CeGetLastError();
		MUTEX_UNLOCK (mutex); 

		D("synce_get_file_info: Error %d: %s\n", err, synce_strerror(err));

		wstr_free_string(tempwstr);
		free(location);

		D("synce_get_file_info: Failed\n");
		D("------------- synce_get_file_info() end --------------\n");

		switch(err)
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
		MUTEX_UNLOCK (mutex); 
		get_file_attributes(file_info, &entry);

		wstr_free_string(tempwstr);
		free(location);

		D("synce_get_file_info: Name: %s\n", file_info->name);
		D("synce_get_file_info: Mime-type: %s\n", file_info->mime_type);
		D("synce_get_file_info: Ok\n");
		D("------------- synce_get_file_info() end --------------\n");

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
	D("synce_get_files_info_from_handle\n");

	return GNOME_VFS_ERROR_ACCESS_DENIED;
}

static gboolean synce_is_local
(
	GnomeVFSMethod *method,
  const GnomeVFSURI *uri
)
{
	return FALSE;
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

	D("--------------------------------------------\n");
	D("synce_mkdir()\n");

	location = g_convert_to_backward_slashes(uri);

	if(!location)
	{
		D("Synce Error: %d %s\n", 13, "ERROR_INVALID_DATA");
		D("Failed\n");
		D("--------------------------------------------\n");
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	D("CeCreateDirectory()\n");
	tempwstr = wstr_from_utf8(location);
	MUTEX_LOCK (mutex);
	if(!CeCreateDirectory(tempwstr, NULL))
	{
		free(location);
		wstr_free_string(tempwstr);
		D("Error %d: %s\n", CeGetLastError(), synce_strerror(CeGetLastError()));
		D("Failed\n");
		D("--------------------------------------------\n");
		MUTEX_UNLOCK (mutex);
		return GNOME_VFS_ERROR_FILE_EXISTS;
	}

	free(location);
	wstr_free_string(tempwstr);

	D("Error %d: %s\n", CeGetLastError(), synce_strerror(CeGetLastError()));
	D("Ok\n");
	D("--------------------------------------------\n");
	
	MUTEX_UNLOCK (mutex);
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
	int err;
	int result;

	D("----------------- synce_rmdir() -----------------------\n");

	location = g_convert_to_backward_slashes(uri);

	if(!location)
	{
		D("synce_rmdir: Synce Error: %d %s\n", 13, "ERROR_INVALID_DATA");
		D("synce_rmdir: Failed\n");
		D("----------------- synce_rmdir() end -------------------\n");
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	D("CeRemoveDirectory()\n");
	tempwstr = wstr_from_utf8(location);
	MUTEX_LOCK (mutex);
	result = CeRemoveDirectory(tempwstr);
	err = CeGetLastError();
	MUTEX_UNLOCK (mutex);
	wstr_free_string(tempwstr);

	free(location);
	D("synce_rmdir:'Result %d\n", result);
	D("synce_rmdir: Error %d: %s\n", err, synce_strerror(err));

	/* In librapi tool prmdir result is used to see if things went OK */
	if (!result) {
		switch(err)
		{
			case 145:
				D("synce_rmdir: Failed\n");
				D("--------------------------------------------\n");
				return GNOME_VFS_ERROR_DIRECTORY_NOT_EMPTY;
			case 2:
				D("synce_rmdir: Failed\n");
				D("--------------------------------------------\n");
				return GNOME_VFS_ERROR_NOT_FOUND;
			case 0:
				D("synce_rmdir: Ok\n");
				D("--------------------------------------------\n");
				return GNOME_VFS_OK;
			default:
				D("synce_rmdir: Failed\n");
				D("--------------------------------------------\n");
				return GNOME_VFS_ERROR_GENERIC;
		}
	}
	return GNOME_VFS_OK;
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
	int err;

	D("--------------------------------------------\n");
	D("synce_move()\n");

	new_location = g_convert_to_backward_slashes(new_uri);

	if(!new_location)
	{
		D("Failed\n");
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	old_location = g_convert_to_backward_slashes(old_uri);

	if(!old_location)
	{
		free(new_location);
		D("Synce Error: %d %s\n", 13, "ERROR_INVALID_DATA");
		D("Failed\n");
		D("--------------------------------------------\n");
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	D("CeMoveFile()\n");
	old_wstr = wstr_from_utf8(old_location);
	new_wstr = wstr_from_utf8(new_location);
	MUTEX_LOCK (mutex);
	result = CeMoveFile(old_wstr, new_wstr);
	err = CeGetLastError();
	MUTEX_UNLOCK (mutex);

	if(err == 183) /* If the destination file exists we end up here */
	{
		if(force_replace)
		/* if the user wants we delete the dest file and moves the source there */
		{
			MUTEX_LOCK (mutex);
			result = CeDeleteFile(new_wstr);
			MUTEX_UNLOCK (mutex);
			
			if(result == 0)
			{
				free(old_location);
				free(new_location);
				wstr_free_string(old_wstr);
				wstr_free_string(new_wstr);
				D("Synce Error: %d %s\n", 5, "ERROR_ACCESS_DENIED");
				D("Failed\n");
				D("--------------------------------------------\n");
				return GNOME_VFS_ERROR_ACCESS_DENIED;
			}
			MUTEX_LOCK (mutex);
			result = CeMoveFile(old_wstr, new_wstr);
			err = CeGetLastError();
			MUTEX_UNLOCK (mutex);
		}
	}

	free(old_location);
	free(new_location);
	wstr_free_string(old_wstr);
	wstr_free_string(new_wstr);

	D("CeGetLastError(): %d\n", err);
	switch(err)
	{
		case 0:
		case 18:
			D("Ok\n");
			D("--------------------------------------------\n");
			return GNOME_VFS_OK;
		case 183:
			D("Failed\n");
			D("--------------------------------------------\n");
			return GNOME_VFS_ERROR_FILE_EXISTS;
		default:
			D("Failed\n");
			D("--------------------------------------------\n");
			return GNOME_VFS_ERROR_GENERIC;
	}
}


static GnomeVFSResult
synce_set_file_info (GnomeVFSMethod *method,
		GnomeVFSURI *uri,
		const GnomeVFSFileInfo *info,
		GnomeVFSSetFileInfoMask mask,
		GnomeVFSContext *context)
{
	GnomeVFSURI *parent_uri, *new_uri;
	GnomeVFSResult result;

	/* For now, we only support changing the name. */
	if ((mask & ~(GNOME_VFS_SET_FILE_INFO_NAME)) != 0) {
		return GNOME_VFS_ERROR_NOT_SUPPORTED;
	}

	/*
	 * Returns an error for incoming names with "/" characters in them,
	 * instead of moving the file.
	 */
	if (g_utf8_strchr(info->name,-1,'/') != NULL) {
		return GNOME_VFS_ERROR_BAD_PARAMETERS;
	}

	/* Share code with do_move. */
	parent_uri = gnome_vfs_uri_get_parent (uri);
	if (parent_uri == NULL) {
		return GNOME_VFS_ERROR_NOT_FOUND;
	}
	new_uri = gnome_vfs_uri_append_file_name (parent_uri, info->name);
	gnome_vfs_uri_unref (parent_uri);
	result = synce_move (method, uri, new_uri, FALSE, context);
	gnome_vfs_uri_unref (new_uri);
	return result;
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

	D("--------------------------------------------\n");
	D("synce_unlink()\n");

	location = g_convert_to_backward_slashes(uri);
	tempwstr = wstr_from_utf8(location);

	if(!location)
	{
		free(location);
		wstr_free_string(tempwstr);
		D("Synce Error: %d %s\n", 13, "ERROR_INVALID_DATA");
		D("Failed\n");
		D("--------------------------------------------\n");
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	D("CeDeleteFile()\n");
	MUTEX_LOCK (mutex);
	result = CeDeleteFile(tempwstr);
	MUTEX_UNLOCK (mutex);

	wstr_free_string(tempwstr);
	free(location);

	if(result == 0)
	{
		D("Failed\n");
		D("--------------------------------------------\n");
		return GNOME_VFS_ERROR_NOT_FOUND;
	}
	else
	{
		D("Ok\n");
		D("--------------------------------------------\n");
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
	synce_set_file_info, 
	NULL,	/* truncate */
	NULL, /* find_directory */
	NULL, /* create_symbolic_link */
};

GnomeVFSMethod *vfs_module_init(const char *method_name, const char *args)
{
	if (!mutex)
		mutex = MUTEX_NEW ();
	MUTEX_LOCK (mutex); 
	HRESULT hr = CeRapiInit();
	MUTEX_UNLOCK (mutex); 
	
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
	MUTEX_LOCK (mutex);
	CeRapiUninit();
	MUTEX_UNLOCK (mutex);

	MUTEX_FREE(mutex);
}

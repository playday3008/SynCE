/* $Id$ */

/* play with defines to allow snprintf() */
#undef __STRICT_ANSI__
#define _GNU_SOURCE 1
#include "synce.h"
#include "synce_log.h"
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define DIRECTORY_NAME   			".synce"
#define CONNECTION_FILENAME		"active_connection"
#define SCRIPT_DIRECTORY      "scripts"

static bool make_sure_directory_exists(char* directory)
{
	struct stat dir_stat;

	/*
	 * Make sure that this directory exists
	 */

	if (stat(directory, &dir_stat) < 0)
	{
		if (mkdir(directory, 0700) < 0)
		{
			synce_error("Failed to create directory %s", directory);
			return false;
		}
	}

	return true;
}

/**
 * Get path to config files
 */
bool synce_get_directory(char** path)
{
	char buffer[256];
	
	/* XXX: not very thread-safe? */
	struct passwd* user = getpwuid(getuid());

	if (!path)
		return false;
	
	*path = NULL;

	if (!user)
		return false;
	
	snprintf(buffer, sizeof(buffer), "%s/" DIRECTORY_NAME, user->pw_dir);

	if (!make_sure_directory_exists(buffer))
		return false;
	
	*path = strdup(buffer);
	
	return true;
}

/**
 * Get file name for active connection info
 */
bool synce_get_connection_filename(char** filename)
{
	bool success = false;
	char* path = NULL;
	char buffer[256];

	if (!filename)
		goto exit;

	*filename = NULL;
	
	if (!synce_get_directory(&path))
		goto exit;

	snprintf(buffer, sizeof(buffer), "%s/" CONNECTION_FILENAME, path);
	*filename = strdup(buffer);

	success = true;

exit:
	if (path)
		free(path);
	return success;
}

bool synce_get_subdirectory(const char* name, char** directory)
{
	bool success = false;
	char* path = NULL;
	char buffer[256];

	if (!name || !directory)
		goto exit;

	if (strchr(name, '/'))	/* prevent bad names */
		goto exit;

	*directory = NULL;
	
	if (!synce_get_directory(&path))
		goto exit;

	snprintf(buffer, sizeof(buffer), "%s/%s", path, name);

	if (!make_sure_directory_exists(buffer))
		goto exit;
	
	*directory = strdup(buffer);

	success = true;

exit:
	if (path)
		free(path);
	return success;
}

bool synce_get_script_directory(char** directory)
{
	return synce_get_subdirectory(SCRIPT_DIRECTORY, directory);
}



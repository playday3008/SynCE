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

/**
 * Get path to config files
 */
bool synce_get_directory(char** path)
{
	char buffer[256];
	struct stat dir_stat;
	
	/* XXX: not very thread-safe? */
	struct passwd* user = getpwuid(getuid());

	if (!path)
		return false;
	
	*path = NULL;

	if (!user)
		return false;
	
	snprintf(buffer, sizeof(buffer), "%s/" DIRECTORY_NAME, user->pw_dir);

	/*
	 * Make sure that this directory exists
	 */

	if (stat(buffer, &dir_stat) < 0)
	{
		if (mkdir(buffer, 0700) < 0)
		{
			synce_error("Failed to create directory %s", buffer);
			return false;
		}
	}

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


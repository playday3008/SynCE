/* $Id$ */

/* play with defines to allow snprintf() */
#undef __STRICT_ANSI__
#define _GNU_SOURCE 1
#include "synce.h"
#include "synce_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

/** 
 * @defgroup SynceFiles Configuration directory and file management
 * @ingroup SynceUtils
 * @brief Manipulating the Synce configuration directory and files
 *
 * @{ 
 */ 

#define DIRECTORY_NAME                ".synce"
#define DEFAULT_CONNECTION_FILENAME   "active_connection"
#define SCRIPT_DIRECTORY              "scripts"

static char connection_filename[MAX_PATH] = {DEFAULT_CONNECTION_FILENAME};

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

/** @brief Get path to configuration files
 * 
 * This function queries the directory used to store
 * SynCE configuration files. By default this is the
 * .synce directory in the user's home directory,
 * but can be specified by the SYNCE_CONF_DIR
 * environment variable.
 * 
 * @param[out] path location to store the pointer to the newly allocated string
 * @return TRUE on success, FALSE on failure
 */ 
bool synce_get_directory(char** path)
{
  char buffer[MAX_PATH];
  char *p;
  struct passwd* user = NULL;

  if (!path)
    return false;

  /* if there is a preference for config dir set
     as an environment variable, use it */
  if ((p = getenv ("SYNCE_CONF_DIR")) != NULL) {
    if (make_sure_directory_exists (p)) {
      *path = strdup (p);
      return true;
    }
  }

  /* XXX: not very thread-safe? */
  user = getpwuid(getuid());

  *path = NULL;

  if (!user)
    return false;

  snprintf(buffer, sizeof(buffer), "%s/" DIRECTORY_NAME, user->pw_dir);

  if (!make_sure_directory_exists(buffer))
    return false;

  *path = strdup(buffer);

  return true;
}

/** @brief Set file name for active connection info
 * 
 * This function sets the location of the SynCE device
 * connection information file. By default this is "active_connection"
 * in the directory returned by synce_get_directory().
 * 
 * The filename cannot contain "..".
 * 
 * @deprecated This is only used with the legacy vdccm implementation of dccm.
 * 
 * @param[in] filename filename to use
 * @return TRUE on success, FALSE on failure
 */ 
bool synce_set_connection_filename(const char* filename)
{
  bool success = false;
  
  /* disallow file names containing '..' */
  if (filename && !strstr(filename, ".."))
  {
    /* Use snprintf to limit length and assure string is terminated with 0 */
    int n = snprintf(connection_filename, sizeof(connection_filename), "%s", filename);

    /* Return false if file name was too long or some other error occured */
    success = (n >= 0 && n < (int)sizeof(connection_filename));
  }

  if (!success)
    synce_warning("Invalid filename: '%s'", filename);

  return success;
}

/** @brief Restore the default file name for active connection info
 * 
 * This function restores the default location of the SynCE device
 * connection information file. By default this is "active_connection"
 * in the directory returned by synce_get_directory().
 * 
 * @deprecated This is only used with the legacy vdccm implementation of dccm.
 * 
 * @return TRUE on success, FALSE on failure
 */ 
bool synce_set_default_connection_filename()
{
  return synce_set_connection_filename(DEFAULT_CONNECTION_FILENAME);
}

/** @brief Get file name for active connection info
 * 
 * This function queries the location of the SynCE device
 * connection information file. By default this is "active_connection"
 * in the directory returned by synce_get_directory().
 *
 * @deprecated This is only used with the legacy vdccm implementation of dccm.
 * 
 * @param[out] filename location to store the pointer to the newly allocated string
 * @return TRUE on success, FALSE on failure
 */ 
bool synce_get_connection_filename(char** filename)
{
	bool success = false;
	char* path = NULL;
	char buffer[MAX_PATH];

	if (!filename)
		goto exit;

	*filename = NULL;
	
	if (!synce_get_directory(&path))
		goto exit;

	snprintf(buffer, sizeof(buffer), "%s/%s" , path, connection_filename);
	*filename = strdup(buffer);

	success = true;

exit:
	if (path)
		free(path);
	return success;
}

/** @brief Get path to configuration subdirectory
 * 
 * This function provides the full path to the specified
 * subdirectory of the SynCE configuration directory, as
 * provided by synce_get_directory().
 * 
 * @param[in] name name of the required subdirectory, cannot contain "/"
 * @param[out] path location to store the pointer to the newly allocated string
 * @return TRUE on success, FALSE on failure
 */ 
bool synce_get_subdirectory(const char* name, char** directory)
{
	bool success = false;
	char* path = NULL;
	char buffer[MAX_PATH];

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

/** @brief Get path to script subdirectory
 * 
 * This function provides the full path to the script
 * subdirectory of the SynCE configuration directory, as
 * provided by synce_get_directory().
 * 
 * @param[out] path location to store the pointer to the newly allocated string
 * @return TRUE on success, FALSE on failure
 */ 
bool synce_get_script_directory(char** directory)
{
	return synce_get_subdirectory(SCRIPT_DIRECTORY, directory);
}


/** @} */

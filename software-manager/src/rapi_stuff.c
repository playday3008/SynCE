
#include <rapi.h>
#include <stdio.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "rapi_stuff.h"

#include "pcommon.h"

#define RAPI_FAILED(x) (x<0)

GList* setup_rapi_and_create_list(GList *list, GtkWidget *progressbar) 
{
    HRESULT hr;
    LONG result;
    HKEY parent_key;
    WCHAR* parent_key_name = NULL;
    WCHAR* value_name = NULL;
    DWORD i;
    bool smartphone = false;

    hr = CeRapiInit();

    if (RAPI_FAILED(hr))
    {
        fprintf(stderr, "Unable to initialize RAPI: %s\n", 
                synce_strerror(hr));
        return (list);
    }

    /* Update the progressbar */
    if (progressbar != NULL) {
        gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressbar));
        while (gtk_events_pending()) {
            gtk_main_iteration();
        }
    }
    /* Path on SmartPhone 2002 */
    parent_key_name = wstr_from_ascii("Security\\AppInstall");

    result = CeRegOpenKeyEx(HKEY_LOCAL_MACHINE, parent_key_name, 0, 0, &parent_key);

    if (ERROR_SUCCESS == result)
    {
        smartphone = true;
    }
    else
    {
        smartphone = false;
        wstr_free_string(parent_key_name);

        /* Path on Pocket PC 2002 */
        parent_key_name = wstr_from_ascii("Software\\Apps");

        result = CeRegOpenKeyEx(HKEY_LOCAL_MACHINE, parent_key_name, 0, 0, &parent_key);

        if (ERROR_SUCCESS != result)
        {
            fprintf(stderr, "Unable to open parent registry key: %s\n", 
                    synce_strerror(result));
            return(list);
        }
    }
    value_name = wstr_from_ascii("Instl");

    /* Update the progressbar */
    if (progressbar != NULL) {
        gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressbar));
        while (gtk_events_pending()) {
            gtk_main_iteration();
        }
    }

    for (i = 0; ; i++)
    {
        WCHAR wide_name[MAX_PATH];
        DWORD name_size = sizeof(wide_name);
        HKEY program_key;
        DWORD installed = 0;
        DWORD value_size = sizeof(installed);

        result = CeRegEnumKeyEx(parent_key, i, wide_name, &name_size, 
                NULL, NULL, NULL, NULL);

        /* Update the progressbar */
        if (progressbar != NULL) {
            gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressbar));
            while (gtk_events_pending()) {
                gtk_main_iteration();
            }
        }
        if (ERROR_SUCCESS != result)
            break;

        if (smartphone)
        {
            char* name = wstr_to_utf8(wide_name);
            list = g_list_append(list,g_strdup(name));
            wstr_free_string(name);
        }
        else
        {
            result = CeRegOpenKeyEx(parent_key, wide_name, 0, 0, &program_key);
            if (ERROR_SUCCESS != result)
                continue;

            result = CeRegQueryValueEx(program_key, value_name, NULL, NULL,
                    (LPBYTE)&installed, &value_size);

            if (ERROR_SUCCESS == result && installed)
            {
                char* name = wstr_to_utf8(wide_name);
                list = g_list_append(list,g_strdup(name));
                wstr_free_string(name);
            }
            CeRegCloseKey(program_key);
        }

    }

    CeRegCloseKey(parent_key);

    return (list);
}

int rapi_mkdir(char *dir) {
    HRESULT hr;
    WCHAR* wide_path = NULL;
    char *path = NULL;
    
    path = (char *) strdup(dir);
    
    hr = CeRapiInit();

    if (RAPI_FAILED(hr))
    {
        fprintf(stderr, "Unable to initialize RAPI: %s\n",
                synce_strerror(hr));
        if (wide_path)
            wstr_free_string(wide_path);

        if (path)
            free(path);

        CeRapiUninit();
        return (-1);
    }

    convert_to_backward_slashes(path);
    wide_path = wstr_from_utf8(path);
    wide_path = adjust_remote_path(wide_path, true);
    
    if (!CeCreateDirectory(wide_path, NULL))
    {
        fprintf(stderr, "Failed to create directory '%s': %s\n",
                path,
                synce_strerror(CeGetLastError()));
        if (wide_path)
            wstr_free_string(wide_path);

        if (path)
            free(path);

        CeRapiUninit();
        return (-2);
    }

    if (wide_path)
        wstr_free_string(wide_path);

    if (path)
        free(path);

    CeRapiUninit();
    return 0;
}

static bool remote_copy(const char* ascii_source, const char* ascii_dest)
{
	return CeCopyFileA(ascii_source, ascii_dest, false);
}

#define ANYFILE_BUFFER_SIZE (64*1024)

static bool anyfile_copy(char* source_ascii, char* dest_ascii, size_t* bytes_copied)
{
	bool success = false;
	size_t bytes_read;
	size_t bytes_written;
	char* buffer = NULL;
	AnyFile* source = NULL;
	AnyFile* dest   = NULL;

	if (!(buffer = (char *) malloc(ANYFILE_BUFFER_SIZE)))
	{
		fprintf(stderr, "Failed to allocate buffer of size %i\n", ANYFILE_BUFFER_SIZE);
		goto exit;
	}

	if (!(source = anyfile_open(source_ascii, READ)))
	{
		fprintf(stderr, "Failed to open source file '%s'\n", source_ascii);
		goto exit;
	}

	if (!(dest = anyfile_open(dest_ascii, WRITE)))
	{
		fprintf(stderr, "Failed to open destination file '%s'\n", dest_ascii);
		goto exit;
	}

	for(;;)
	{
		if (!anyfile_read(source, buffer, ANYFILE_BUFFER_SIZE, &bytes_read))
		{
			fprintf(stderr, "Failed to read from source file '%s'\n", source_ascii);
			goto exit;
		}

		if (0 == bytes_read)
		{
			/* End of file */
			break;
		}

		if (!anyfile_write(dest, buffer, bytes_read, &bytes_written))
		{
			fprintf(stderr, "Failed to write to destination file '%s'\n", dest_ascii);
			goto exit;
		}

		if (bytes_written != bytes_read)
		{
			fprintf(stderr, "Only wrote %i bytes of %i to destination file '%s'\n", 
					bytes_written, bytes_read, dest_ascii);
			goto exit;
		}

		*bytes_copied += bytes_written;
	}

	success = true;

exit:
	if (buffer)
		free(buffer);
	
	if (source)
	{
		anyfile_close(source);
		free(source);
	}

	if (dest)
	{
		anyfile_close(dest);
		free(dest);
	}

	return success;
}

int rapi_copy(char *source, char *dest)
{
	int result = 1;
    HRESULT hr;
    time_t start;
    time_t duration;
    size_t bytes_copied = 0;

    hr = CeRapiInit();

    if (RAPI_FAILED(hr))
    {
        fprintf(stderr, "Unable to initialize RAPI: %s\n",
                synce_strerror(hr));
        goto exit;
    }

    if (!dest)
    {
        char* p;

        if (is_remote_file(source))
        {

            for (p = source + strlen(source); p != source; p--)
            {
                if (*p == '/' || *p == '\\')
                {
                    dest = (char *) strdup(p+1);
                    break;
                }
            }

            if (!dest || '\0' == dest[0])
            {
                fprintf(stderr, "Unable to extract destination filename from source path '%s'\n",
                        source);
                goto exit;
            }
        }
        else
        {
            WCHAR mydocuments[MAX_PATH];
            char* mydocuments_ascii = NULL;
            p = strrchr(source, '/');

            if (p)
                p++;
            else
                p = source;

            if ('\0' == *p)
            {
                fprintf(stderr, "Unable to extract destination filename from source path '%s'\n",
                        source);
                goto exit;
            }

            if (!CeGetSpecialFolderPath(CSIDL_PERSONAL, sizeof(mydocuments), mydocuments))
            {
                fprintf(stderr, "Unable to get the \"My Documents\" path.\n");
                goto exit;
            }

            dest = (char *) calloc(1, 1 + wstr_strlen(mydocuments) + 1 + strlen(p) + 1);

            mydocuments_ascii = wstr_to_ascii(mydocuments);

            strcat(dest, ":");
            strcat(dest, mydocuments_ascii);
            strcat(dest, "\\");
            strcat(dest, p);

            wstr_free_string(mydocuments_ascii);
        }
    }


    if (0 == strcmp(source, dest))
    {
        fprintf(stderr, "You don't want to copy a file to itself.\n");
        goto exit;
    }

    if (is_remote_file(source) && is_remote_file(dest))
    {
        /*
         *          * Both are remote; use CeCopyFile()
         *                   */
        if (!remote_copy(source, dest))
            goto exit;
    }
    else
    {
        start = time(NULL);

        /*
         *          * At least one is local, Use the AnyFile functions
         *                   */
        if (!anyfile_copy(source, dest,  &bytes_copied))
            goto exit;

    }

	result = 0;
exit:

    CeRapiUninit();
    return result;
}

int rapi_run(char *program, char *parameters)
{
	int result = 1;
	HRESULT hr;
	WCHAR* wide_program = NULL;
	WCHAR* wide_parameters = NULL;
	PROCESS_INFORMATION info;
    char *tmpprogram = NULL;

    tmpprogram = (char *) strdup(program);
    
	hr = CeRapiInit();

	if (RAPI_FAILED(hr))
	{
		fprintf(stderr, "Unable to initialize RAPI: %s\n", 
				synce_strerror(hr));
		goto exit;
	}

	convert_to_backward_slashes(tmpprogram);
	wide_program = wstr_from_utf8(tmpprogram);
	if (parameters)
		wide_parameters = wstr_from_utf8(parameters);

	memset(&info, 0, sizeof(info));
	
	if (!CeCreateProcess(
				wide_program,
				wide_parameters,
				NULL,
				NULL,
				false,
				0,
				NULL,
				NULL,
				NULL,
				&info
				))
	{
		fprintf(stderr, "Failed to execute '%s': %s\n", 
				tmpprogram,
				synce_strerror(CeGetLastError()));
		goto exit;
	}

	CeCloseHandle(info.hProcess);
	CeCloseHandle(info.hThread);

	result = 0;

exit:
	wstr_free_string(wide_program);
	wstr_free_string(wide_parameters);

	if (tmpprogram)
		free(tmpprogram);

	CeRapiUninit();
	return result;
}


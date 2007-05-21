/* $Id$ */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgnomevfs/gnome-vfs-init.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-volume.h>
#include <libgnomevfs/gnome-vfs-volume-monitor.h>
#include <synce.h>
#include <synce_log.h>
#include "config.h"

#define SCRIPT_NAME "synce-in-computer-folder.sh"

#define DISPLAY_NAME "Mobile Device"
#define URI "synce:///"
#define ICON synce-color

#define COMMAND_STRING_INSTALL      "install"
#define COMMAND_STRING_UNINSTALL    "uninstall"
#define COMMAND_STRING_CONNECT      "connect"
#define COMMAND_STRING_DISCONNECT   "disconnect"

#define COMMAND_INSTALL     1
#define COMMAND_UNINSTALL   2
#define COMMAND_CONNECT     3
#define COMMAND_DISCONNECT  4

static GnomeVFSVolume *find_volume(GnomeVFSVolumeMonitor *monitor)
{
  GnomeVFSVolume *volume;
  GList *volumes, *l;
  volumes = gnome_vfs_volume_monitor_get_mounted_volumes (monitor);

  for (l = volumes; l != NULL; l = l->next) {
    volume = l->data;

    synce_debug("Volume: '%s'", gnome_vfs_volume_get_display_name(volume));

    if (strcmp(gnome_vfs_volume_get_display_name(volume), 
      DISPLAY_NAME) == 0 )
      return volume;
  }

  return NULL;
}

static void show_usage(const char *argv0)
{
  fprintf(stderr, 
      "Syntax:\n"
      "\n"
      "%s install|uninstall|connect|disconnect\n"
      "\n"
      "  install     Call this program on device connect/disconnect\n"
      "  uninstall   Don't call this program on device connect/disconnect\n"
      "  connect     Add '" DISPLAY_NAME "' to Computer folder\n"
      "  disconnect  Remove '" DISPLAY_NAME "' from Computer folder\n"
      , argv0 
      );
}

static void callback (gboolean succeeded,
					  char *error,
					  char *detailed_error,
					  gpointer data)
{
  /* Do nothing */
  if (succeeded)
    synce_trace("Succeeded");
  else
    synce_trace("Failed: '%s' '%s'\n", error, detailed_error);
}


int main(int argc, const char **argv)
{
  GnomeVFSVolumeMonitor *monitor;
  GnomeVFSVolume *volume;
  int command = 0;

  if (argc < 2)
  {
    show_usage(argv[0]);
    return 1;
  }

  if (strcmp(argv[1], COMMAND_STRING_INSTALL) == 0)
    command = COMMAND_INSTALL;
  else if (strcmp(argv[1], COMMAND_STRING_UNINSTALL) == 0)
    command = COMMAND_UNINSTALL;
  else if (strcmp(argv[1], COMMAND_STRING_CONNECT) == 0)
    command = COMMAND_CONNECT;
  else if (strcmp(argv[1], COMMAND_STRING_DISCONNECT) == 0)
    command = COMMAND_DISCONNECT;
  else
  {
    show_usage(argv[0]);
    return 1;
  }

  synce_log_use_syslog();

  if (command == COMMAND_INSTALL || command == COMMAND_UNINSTALL)
  {
    char* script_directory = NULL;
    gchar* script_filename = NULL;
    
    if (!synce_get_script_directory(&script_directory))
    {
      fprintf (stderr, "Failed to get script directory.\n");
      return 1;
    }

    script_filename = g_strdup_printf("%s/" SCRIPT_NAME, script_directory);
    free(script_directory);
   
    if (command == COMMAND_INSTALL)
    {
      FILE* input = NULL;
      FILE* output = NULL;
      size_t bytes;
      char buffer[1024];

      input = fopen(SYNCE_IN_COMPUTER_FOLDER_SH, "r");
      if (!input)
      {
        fprintf(stderr, "Failed to open input file: '" SYNCE_IN_COMPUTER_FOLDER_SH "'.\n");
        return 1;
      }

      output = fopen(script_filename, "w+");
      if (!output)
      {
        fprintf(stderr, "Failed to open output file: '%s'.\n", script_filename);
        return 1;
      }
      
      bytes = fread(buffer, 1, sizeof(buffer), input);
      fwrite(buffer, 1, bytes, output);

      fclose(output);
      fclose(input);

      chmod(script_filename, 0755);
    }
    else  /* command == COMMAND_UNINSTALL */
    {
      unlink(script_filename);
    }

    g_free(script_filename);
  }
  else
  {
    if (!gnome_vfs_init()) 
    {
      fprintf (stderr, "Cannot initialize gnome-vfs.\n");
      return 1;
    }

    monitor = gnome_vfs_get_volume_monitor ();

    volume = find_volume(monitor);

    if (volume)
    {
      synce_trace("Found volume");

      if (command == COMMAND_DISCONNECT)
      {
        synce_info("Disconnecting SynCE volume");
        gnome_vfs_volume_unmount(volume, callback, NULL);
      }
    }
    else
    {
      synce_trace("Did not find volume");

      if (command == COMMAND_CONNECT)
      {
        synce_info("Connecting SynCE volume");
        gnome_vfs_connect_to_server(URI, DISPLAY_NAME, ICON);
      }
    }

    gnome_vfs_shutdown();
  }

  return 0;
}


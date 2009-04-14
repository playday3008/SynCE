/* $Id$ */
#define _BSD_SOURCE 1
#include <rapi.h>
#include <synce_log.h>
#include "matchmaker.h"
#include <stdio.h>
#include <time.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>

typedef enum
{
  COMMAND_HELP,
  COMMAND_STATUS,
  COMMAND_CREATE,
  COMMAND_CLEAR,
} COMMAND;

static void show_usage(const char* name)
{
  fprintf(stderr,
          "The purpose of this program is to manage partnerships with the currently\n"
          "connected (pre Windows Mobile 5) device.\n"
          "\n"
          "Syntax:\n"
          "\n"
          "\t%s [-d LEVEL] [status|create|clear INDEX]\n"
          "\n"
          "\t-d LEVEL          Set debug log level\n"
          "\t                  0 - No logging\n"
          "\t                  1 - Errors only (default)\n"
          "\t                  2 - Errors and warnings\n"
          "\t                  3 - Errors, warnings and info\n"
          "\t                  4 - Everything\n"
          "\tstatus   Show partnership status for device\n"
          "\tcreate   Create partnership with device\n"
          "\tclear    Clear a partnership on device\n"
          "\tINDEX    The partnership index (1 or 2)\n",
          name);
}

static bool handle_parameters(int argc, char** argv, COMMAND *command, uint32_t *index)
{
  int c;
  int arg_count;
  int log_level = SYNCE_LOG_LEVEL_ERROR;

  while ((c = getopt(argc, argv, "d:")) != -1)
  {
    switch (c)
    {
      case 'd':
        log_level = atoi(optarg);
        break;

      case 'h':
      default:
        show_usage(argv[0]);
        return false;
    }
  }

  synce_log_set_level(log_level);

  arg_count = argc - optind;
  if (arg_count == 0) {
          show_usage(argv[0]);
          return false;
  }

  if (0 == strcasecmp(argv[optind], "status"))
    *command = COMMAND_STATUS;
  else if (0 == strcasecmp(argv[optind], "create"))
    *command = COMMAND_CREATE;
  else if (0 == strcasecmp(argv[optind], "clear"))
    *command = COMMAND_CLEAR;
  else
  {
    show_usage(argv[0]);
    return false;
  }

  if ((*command == COMMAND_CLEAR))
  {
    if (arg_count < 2)
    {
      show_usage(argv[0]);
      return false;
    }
    optind++;
    *index = atol(argv[optind]);
  }

  return true;
}


int main(int argc, char** argv)
{
  int result = 1;
  RRA_Matchmaker* matchmaker = NULL;
  DWORD current_partner;
  uint32_t index = 0;
  HRESULT hr;
  int i;
  COMMAND command = COMMAND_HELP;

  if (!handle_parameters(argc, argv, &command, &index))
    goto exit;

  hr = CeRapiInit();
  if (FAILED(hr))
  {
    synce_error("Failed to initialize RAPI");
    goto exit;
  }

  matchmaker = rra_matchmaker_new();
  if (!matchmaker)
  {
    synce_error("Failed to create match-maker");
    goto exit;
  }

  switch (command)
  {
    case COMMAND_STATUS:
      if (rra_matchmaker_get_current_partner(matchmaker, &current_partner))
      {
        printf("Current partner index: %i\n", current_partner);
      }
      else
      {
        printf("Current partner index: none\n");
      }

      for (i = 1; i <= 2; i++)
      {
        uint32_t id = 0;
        char* name = NULL;

        if (rra_matchmaker_get_partner_id(matchmaker, i, &id))
        {
          printf("Partner %i id:    0x%08x\n", i, id);
        }

        if (rra_matchmaker_get_partner_name(matchmaker, i, &name))
        {
          printf("Partner %i name:  \"%s\"\n", i, name);
          rra_matchmaker_free_partner_name(name);
        }
      }
      break;

    case COMMAND_CREATE:
      if (rra_matchmaker_create_partnership(matchmaker, &index))
      {
        printf("Partnership creation succeeded. Using partnership index %i.\n", index);
      }
      else
      {
        fprintf(stderr, "Partnership creation failed. Maybe there is no empty partnership slot?\n");
        goto exit;
      }
      break;

    case COMMAND_CLEAR:
      if (index == 1 || index == 2)
      {
        if (rra_matchmaker_clear_partnership(matchmaker, index))
        {
          printf("Partnership cleaning succeeded.\n");
        }
        else
        {
          fprintf(stderr, "Partnership cleaning failed.\n");
        }
      }
      else
      {
        fprintf(stderr, "Invalid or missing index of partnership to clear.\n");
        goto exit;
      }
    break;
    default:
      goto exit;
  }

  result = 0;

exit:
  rra_matchmaker_destroy(matchmaker);
  CeRapiUninit();
  return result;
}


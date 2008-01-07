/* $Id$ */
#define _BSD_SOURCE 1
#include <rapi.h>
#include <synce_log.h>
#include "matchmaker.h"
#include <stdio.h>
#include <time.h>
#include <strings.h>
#include <stdlib.h>

typedef enum
{
  COMMAND_HELP,
  COMMAND_STATUS,
  COMMAND_CREATE,
  COMMAND_REPLACE,
  COMMAND_CLEAR,
} COMMAND;


int main(int argc, char** argv)
{
  int result = 1;
  RRA_Matchmaker* matchmaker = NULL;
  DWORD current_partner;
  uint32_t index = 0;
  HRESULT hr;
  int i;
  COMMAND command = COMMAND_HELP; 
  const char* command_string = "";

  if (argc >= 2)
    command_string = argv[1];

  if (0 == strcasecmp(command_string, "status"))
    command = COMMAND_STATUS;
  else if (0 == strcasecmp(command_string, "create"))
    command = COMMAND_CREATE;
  else if (0 == strcasecmp(command_string, "replace"))
    command = COMMAND_REPLACE;
  else if (0 == strcasecmp(command_string, "clear"))
    command = COMMAND_CLEAR;

  if (command == COMMAND_HELP)
  {
    printf(
        "The purpose of this program is to manage partnerships with the currently\n"
        "connected device.\n"
        "\n"
        "Syntax:\n"
        "\t%s [status|create|replace INDEX|clear INDEX]\n"
        "\n"
        "\tstatus   Show partnership status for device\n"
        "\tcreate   Create partnership with device\n"
        "\treplace  Replace partnership on device\n"
        "\tclear    Clear a partnership on device\n"
        "\tINDEX    The partnership index (1 or 2)\n",
        argv[0]
        );
    goto exit;
  }

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

    case COMMAND_REPLACE:
      if (argc >= 3)
        index = atol(argv[2]);

      if (index == 1 || index == 2)
      {
        if (rra_matchmaker_replace_partnership(matchmaker, index))
        {
          printf("Partnership replacement succeeded.\n");
        }
        else
        {
          fprintf(stderr, "Partnership replacement failed.\n");
        }
      }
      else
      {
        fprintf(stderr, "Invalid or missing index of partnership to replace.\n");
        goto exit;
      }
      break;
  case COMMAND_CLEAR:
      if (argc >= 3)
        index = atol(argv[2]);

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


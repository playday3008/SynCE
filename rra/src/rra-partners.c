/* $Id$ */
#define _BSD_SOURCE 1
#include <rapi.h>
#include <synce_log.h>
#include "librra.h"
#include <stdio.h>
#include <time.h>
#include <strings.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	int result = 1;
	RRA* rra = NULL;
	DWORD current_partner;
	HRESULT hr;
	int i;
  const char* command = "status";

	hr = CeRapiInit();
	if (FAILED(hr))
	{
		synce_error("Failed to initialize RAPI");
		goto exit;
	}

	rra = rra_new();

  if (argc >= 2)
    command = argv[1];

  if (0 == strcasecmp(command, "status"))/*{{{*/
  {
    if (!rra_partner_get_current(rra, &current_partner))
    {
      fprintf(stderr, "Failed to get current partner index");
      goto exit;
    }

    printf("Current partner index: %i\n", current_partner);

    for (i = 1; i <= 2; i++)
    {
      uint32_t id = 0;
      char* name = NULL;

      if (rra_partner_get_id(rra, i, &id))
      {
        printf("Partner %i id:    0x%08x\n", i, id);
      }

      if (rra_partner_get_name(rra, i, &name))
      {
        printf("Partner %i name:  \"%s\"\n", i, name);
        rra_partner_free_name(name);
      }
    }
  }/*}}}*/
  else if (0 == strcasecmp(command, "create"))/*{{{*/
  {
    uint32_t index = 0;
    if (rra_partner_create(rra, &index))
    {
      printf("Partnership creation succeeded. Using partnership index %i.\n", index);
    }
    else
    {
      fprintf(stderr, "Partnership creation failed.\n");
      goto exit;
    }
  }/*}}}*/
  else if (0 == strcasecmp(command, "replace"))/*{{{*/
  {
    uint32_t index = 0;
    
    if (argc >= 3)
      index = atol(argv[2]);

    if (index == 1 || index == 2)
    {
      if (rra_partner_replace(rra, index))
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
    
  }/*}}}*/
  else
  {
    printf(
        "Syntax:\n"
        "\t%s [status|create|replace INDEX]\n"
        "\n"
        "\tstatus   Show partnership status for device. (Default)\n"
        "\tcreate   Create partnership with device.\n"
        "\treplace  Replace partnership on device.\n"
        "\tINDEX    The index number (1 or 2).\n",
        argv[0]
        );
  }

	result = 0;
	
exit:
	rra_free(rra);
	CeRapiUninit();
	return result;
}


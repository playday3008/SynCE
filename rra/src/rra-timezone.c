/* $Id$ */
#include "timezone.h"
#include "generator.h"
#include <rapi.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* month_names[12] =/*{{{*/
{
  "January",
  "Februrary",
  "March",
  "April",
  "May",
  "June",
  "July",
  "August",
  "September",
  "October",
  "November",
  "December"
};/*}}}*/

static const char* month_string(unsigned month)/*{{{*/
{
  if (month >= 1 && month <= 12)
    return month_names[month-1];
  else
    return "Unknown";
}/*}}}*/

static const char* instance_string(unsigned instance)/*{{{*/
{
  switch (instance)
  {
    case 1: return "First week of month";
    case 2: return "Second week of month";
    case 3: return "Third week of month";
    case 4: return "Fourth week of month";
    case 5: return "Last week of month";
    default: return "Unknown";
  }
}/*}}}*/

void dump_vtimezone(TimeZoneInformation* tzi)/*{{{*/
{
  Generator* generator = generator_new(0, NULL);
  
  if (time_zone_generate_vtimezone(generator, tzi))
  {
    char* result = NULL;
    if (generator_get_result(generator, &result))
    {
      printf("%s", result);
      free(result);
    }
    else
      fprintf(stderr, "Failed to get generated VTIMEZONE\n");
  }
  else
    fprintf(stderr, "Failed to generate VTIMEZONE\n");

  generator_destroy(generator);
}/*}}}*/

int main(int argc, char** argv)
{
  int result = 1;
  HRESULT hr;
  TimeZoneInformation tzi;
  char* ascii_name = NULL;
  char* ascii_description = NULL;
  
  hr = CeRapiInit();
  if (FAILED(hr))
    goto exit;

  if (!time_zone_get_information(&tzi))
  {
    fprintf(stderr, "%s: Failed to get time zone information\n", argv[0]);
    goto exit;
  }

  if (argc >= 2)
  {
    const char* filename = argv[1];
    FILE* file = fopen(filename, "w");

    if (file)
    {
      fwrite(&tzi, sizeof(TimeZoneInformation), 1, file);
      fclose(file);
    }
    else
    {
      fprintf(stderr, "%s: Failed to open file %s: %s\n", 
          argv[0], filename, strerror(errno));
    }
  }

  ascii_name        = wstr_to_ascii(tzi.Name);
  ascii_description = wstr_to_ascii(tzi.Description);

  printf(
      "Bias:                  %i\n"
      "Name:                  %s\n"
      "StandardMonthOfYear:   %i (%s)\n"
      "StandardInstance:      %i (%s)\n"
      "StandardStartHour:     %i\n"
      "StandardBias:          %i\n"
      "Description:           %s\n"
      "DaylightMonthOfYear:   %i (%s)\n"
      "DaylightInstance:      %i (%s)\n"
      "DaylightStartHour:     %i\n"
      "DaylightBias:          %i\n"
/*      "Unknown #4:            %i\n"*/
      ,
      tzi.Bias,
      ascii_name,
      tzi.StandardMonthOfYear, month_string(tzi.StandardMonthOfYear),
      tzi.StandardInstance, instance_string(tzi.StandardInstance),
      tzi.StandardStartHour,
      tzi.StandardBias,
      ascii_description,
      tzi.DaylightMonthOfYear, month_string(tzi.DaylightMonthOfYear),
      tzi.DaylightInstance, instance_string(tzi.DaylightInstance),
      tzi.DaylightStartHour,
      tzi.DaylightBias/*,
      tzi.unknown4*/
        );

  dump_vtimezone(&tzi);

  result = 0;

exit:
  CeRapiUninit();
  wstr_free_string(ascii_name);
  wstr_free_string(ascii_description);
  return result;
}


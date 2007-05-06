#define _BSD_SOURCE 1
#include "liborange_internal.h"
/*#include <gsf/gsf-utils.h>*/
#include <gsf/gsf-output-stdio.h>
#include <gsf/gsf-input-stdio.h>
#include <gsf/gsf-infile.h>
#include <gsf/gsf-infile-msole.h>
#include <string.h>

#include "liborange_log.h"

bool orange_extract_msi(
    const char* input_filename,
    const char* output_directory)
{
  bool success = false;
  GError* error = NULL;
  GsfInput* input = NULL;
  GsfInfile* infile = NULL;
  int count;
  int i;
  const char* basename;

  basename = strrchr(input_filename, '/');
  if (basename)
    basename++;
  else
    basename = input_filename;

  input = gsf_input_stdio_new(input_filename, &error);

  infile = gsf_infile_msole_new(input, &error);
  if (error)
    goto exit;

  count = gsf_infile_num_children(infile);
  synce_trace("%i files in MS OLE archive", count);

  for (i = 0; i < count; i++)
  {
    char output_filename[256];
    snprintf(output_filename, sizeof(output_filename), "%s/%s.%04x", output_directory, basename, i);

    GsfInput* child = gsf_infile_child_by_index(infile, i);
    
    GsfOutput* output = gsf_output_stdio_new(output_filename, &error);
    if (!error)
    {
      gsf_input_copy(child, output);
      g_object_unref (G_OBJECT (output));
    }

    g_object_unref (G_OBJECT (child));
  }

  g_object_unref (G_OBJECT (infile));
  g_object_unref (G_OBJECT (input));

  success = true;
 
exit: 
  return success;
}

/* $Id$ */
#include <liborange.h>
#include <stdio.h>

static bool callback(
    const char* filename, 
    CabInfo* info,
    void* cookie)
{
  printf("File found: '%s'\n", filename);
  return true;
}


int main(int argc, char** argv)
{
  int result = 1;

  if (!orange_squeeze_file(argv[1], callback, NULL))
    goto exit;
  
exit:
  return result;
}


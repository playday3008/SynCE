/* $Id$ */
#include <rapi.h>
#include <synce_log.h>
#include <pcommon.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STR_EQUAL(a,b)  (0 == strcasecmp(a,b))

static void dump(const char *desc, void* data, size_t len)/*{{{*/
{
	uint8_t* buf = (uint8_t*)data;
	size_t i, j;
	char hex[8 * 3 + 1];
	char chr[8 + 1];

	printf("%s (%d bytes):\n", desc, len);
	for (i = 0; i < len + 7; i += 8) {
		for (j = 0; j < 8; j++) 
			if (j + i >= len) {
				hex[3*j+0] = ' ';
				hex[3*j+1] = ' ';
				hex[3*j+2] = ' ';
				chr[j] = ' ';
			} else {
				uint8_t c = buf[j + i];
				const char *hexchr = "0123456789abcdef";
				hex[3*j+0] = hexchr[(c >> 4) & 0xf];
				hex[3*j+1] = hexchr[c & 0xf];
				hex[3*j+2] = ' ';
				if (c > ' ' && c <= '~')
					chr[j] = c;
				else
					chr[j] = '.';
			}
		hex[8*3] = '\0';
		chr[8] = '\0';
		printf("  %04x: %s %s\n", i, hex, chr);
	}
}/*}}}*/

int main(int argc, char** argv)
{
  int result = 1;
  const char* parent_str  = NULL;
  char* key_name          = NULL;
  const char* value_name  = NULL;
  HKEY parent;
  HKEY key = 0;
  LONG error;
  WCHAR* value_name_wide = NULL;
  DWORD value_type;
  DWORD value_size = 0;
  uint8_t* value = NULL;
  HRESULT hr;
  
  if (argc < 4)
  {
    fprintf(stderr, "Too few parameters\n");
    goto exit;
  }
  
  parent_str  =        argv[1];
  key_name    = strdup(argv[2]);
  value_name  =        argv[3];

  /* handle abbreviations */
  if (STR_EQUAL(parent_str, "HKCR"))
    parent_str = "HKEY_CLASSES_ROOT";
  else if (STR_EQUAL(parent_str, "HKCU"))
    parent_str = "HKEY_CURRENT_USER";
  else if (STR_EQUAL(parent_str, "HKLM"))
    parent_str = "HKEY_LOCAL_MACHINE";
  else if (STR_EQUAL(parent_str, "HKU"))
    parent_str = "HKEY_USERS";

  if (STR_EQUAL(parent_str, "HKEY_CLASSES_ROOT"))
    parent = HKEY_CLASSES_ROOT;
  else if (STR_EQUAL(parent_str, "HKEY_CURRENT_USER"))
    parent = HKEY_CURRENT_USER;
  else if (STR_EQUAL(parent_str, "HKEY_LOCAL_MACHINE"))
    parent = HKEY_LOCAL_MACHINE;
  else if (STR_EQUAL(parent_str, "HKEY_USERS"))
    parent = HKEY_USERS;
  else
  {
    fprintf(stderr, "Invalid parent key\n");
    goto exit;
  }

  if (S_OK != (hr = CeRapiInit()))
  {
    fprintf(stderr, "%s: Unable to initialize RAPI: %s\n", 
        argv[0],
        synce_strerror(hr));
    goto exit;
  }

  convert_to_backward_slashes(key_name);

  if (!rapi_reg_open_key(parent, key_name, &key))
  {
    fprintf(stderr, "Failed to open key: '%s\\%s'\n", parent_str, key_name);
    goto exit;
  }

  value_name_wide = wstr_from_ascii(value_name);

  error = CeRegQueryValueEx(key, value_name_wide, NULL, NULL, NULL, &value_size);
  if (ERROR_SUCCESS != error)
  {
    fprintf(stderr, "Failed to get size of value '%s\\%s\\%s': %s\n", 
        parent_str, key_name, value_name,
        synce_strerror(error));
    goto exit;
  }

  value = calloc(1, value_size);
  if (!value)
  {
    fprintf(stderr, "Failed to allocate %i bytes", value_size);
    goto exit;
  }
  
  error = CeRegQueryValueEx(key, value_name_wide, NULL, &value_type, value, &value_size);
  if (ERROR_SUCCESS != error)
  {
    fprintf(stderr, "Failed to get value '%s\\%s\\%s': %s\n", 
        parent_str, key_name, value_name,
        synce_strerror(error));
    goto exit;
  }

  printf("[%s\\%s]\n\"%s\"=", parent_str, key_name, value_name);

  switch (value_type)
  {
    case REG_SZ:
      printf("\"%s\"", (const char*)value);
      break;
    
    case REG_DWORD:
      printf("dword=%08x\n", *(DWORD*)value);
      break;

    default:
      dump("Value", value, value_size);
      break;
  }

  result = 0;
  
exit:
  if (key)
    CeRegCloseKey(key);
  CeRapiUninit();
  if (value)
    free(value);
  if (key_name)
    free(key_name);
  wstr_free_string(value_name_wide);
  return result;
}


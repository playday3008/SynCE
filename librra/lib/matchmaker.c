/* $Id$ */
#define _BSD_SOURCE 1
#define _SVID_SOURCE 1
#include "matchmaker.h"
#include <synce_log.h>
#include <synce_ini.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char* PARTNERS =
	"Software\\Microsoft\\Windows CE Services\\Partners";

static const char* CURRENT_PARTNER  = "PCur";
static const char* PARTNER_ID       = "PId";
static const char* PARTNER_NAME     = "PName";

static const char* RRA_DIRECTORY    = "rra";

static const char* PARTERSHIP_FILENAME = "partner";
static const char* PARTERSHIP_SECTION  = "partnership";

struct _RRA_Matchmaker
{
  HKEY keys[3];
  IRAPISession *session;
};
 
#define KEY_PARTNERS    0
#define KEY_PARTNER_1   1
#define KEY_PARTNER_2   2
#define KEY_COUNT       3

RRA_Matchmaker* rra_matchmaker_new(IRAPISession *session)
{
  HKEY partnersKey;
  RRA_Matchmaker* result = calloc(1, sizeof(RRA_Matchmaker));

  if (!result)
  {
    synce_error("Failed to allocate an RRA_Matchmaker");
    return NULL;
  }

  IRAPISession_AddRef(session);
  result->session = session;

  WCHAR* name_wide = wstr_from_current(PARTNERS);
  if (!name_wide)
  {
    free(result);
    synce_error("Failed to convert registry key name to WSTR");
    return FALSE;
  }
  
  LONG res = IRAPISession_CeRegCreateKeyEx(result->session, HKEY_LOCAL_MACHINE,name_wide,0,NULL,0,0,NULL,&partnersKey,NULL);

  wstr_free_string(name_wide);

  if (res == ERROR_SUCCESS)
    result->keys[KEY_PARTNERS] = partnersKey;
  else
  {
    free(result);
    result = NULL;
    synce_error("Failed to open registry key HKLM\\%s: %s", PARTNERS, synce_strerror(res));
  }
  return result;
}

void rra_matchmaker_destroy(RRA_Matchmaker* matchmaker)
{
  if (matchmaker)
  {
    int i;
    
    for (i = 0; i < KEY_COUNT; i++)
      if (matchmaker->keys[i])
        IRAPISession_CeRegCloseKey(matchmaker->session, matchmaker->keys[i]);
    IRAPISession_Release(matchmaker->session);

    free(matchmaker);
  }
}

static bool rra_matchmaker_create_key(RRA_Matchmaker* matchmaker, uint32_t index)
{
  if (index == 1 || index == 2) 
  {
    if (matchmaker->keys[index])
    {
      /* Already have this key */
      return true;
    }
    else
    {
      char name[MAX_PATH];
      snprintf(name, sizeof(name), "%s\\P%i", PARTNERS, index);

      WCHAR* name_wide = wstr_from_current(name);
      if (!name_wide)
        return false;

      LONG result = IRAPISession_CeRegCreateKeyEx(matchmaker->session, HKEY_LOCAL_MACHINE, name_wide, 0, NULL, 0, 0, NULL, &matchmaker->keys[index], NULL);
      wstr_free_string(name_wide);

      return ERROR_SUCCESS == result;
    }
  }
  else
    return false;
}

static bool rra_matchmaker_open_key(RRA_Matchmaker* matchmaker, uint32_t index)
{
  if (index == 1 || index == 2) 
  {
    if (matchmaker->keys[index])
    {
      /* Already have this key */
      return true;
    }
    else
    {
      char name[MAX_PATH];
      snprintf(name, sizeof(name), "%s\\P%i", PARTNERS, index);

      WCHAR* name_wide = wstr_from_current(name);
      if (!name_wide)
        return FALSE;

      LONG result = IRAPISession_CeRegOpenKeyEx(matchmaker->session, HKEY_LOCAL_MACHINE, name_wide, 0, 0, &matchmaker->keys[index]);
      wstr_free_string(name_wide);

      return ERROR_SUCCESS == result;
    }
  }
  else
    return false;
}

bool rra_matchmaker_set_current_partner(RRA_Matchmaker* matchmaker, uint32_t index)
{
  if (!(index == 1 || index == 2))
    return false;

  WCHAR* name_wide = wstr_from_current(CURRENT_PARTNER);
  if (!name_wide)
    return FALSE;

  LONG result = IRAPISession_CeRegSetValueEx(matchmaker->session, matchmaker->keys[KEY_PARTNERS], name_wide, 0, REG_DWORD, (BYTE*)&index, sizeof(DWORD));
  wstr_free_string(name_wide);

  return ERROR_SUCCESS == result;
}

bool rra_matchmaker_get_current_partner(RRA_Matchmaker* matchmaker, uint32_t* index)
{
  DWORD type;
  DWORD size = sizeof(DWORD);
  WCHAR* name_wide = wstr_from_current(CURRENT_PARTNER);
  if (!name_wide)
    return FALSE;

  LONG result = IRAPISession_CeRegQueryValueEx(matchmaker->session, matchmaker->keys[KEY_PARTNERS], name_wide, NULL, &type, (LPBYTE)index, &size);
  wstr_free_string(name_wide);

  return
    ERROR_SUCCESS == result &&
    REG_DWORD == type &&
    sizeof(DWORD) == size;
}

static bool rra_matchmaker_set_partner_id(RRA_Matchmaker* matchmaker, uint32_t index, uint32_t id)
{
  if (!rra_matchmaker_create_key(matchmaker, index))
    return false;

  WCHAR* name_wide = wstr_from_current(PARTNER_ID);
  if (!name_wide)
    return FALSE;

  LONG result = IRAPISession_CeRegSetValueEx(matchmaker->session, matchmaker->keys[index], name_wide, 0, REG_DWORD, (BYTE*)&id, sizeof(DWORD));
  wstr_free_string(name_wide);

  return ERROR_SUCCESS == result;
}

bool rra_matchmaker_get_partner_id(RRA_Matchmaker* matchmaker, uint32_t index, uint32_t* id)
{
  if (!rra_matchmaker_open_key(matchmaker, index))
    return false;

  DWORD type;
  DWORD size = sizeof(DWORD);
  WCHAR* name_wide = wstr_from_current(PARTNER_ID);
  if (!name_wide)
    return FALSE;

  LONG result = IRAPISession_CeRegQueryValueEx(matchmaker->session, matchmaker->keys[index], name_wide, NULL, &type, (LPBYTE)id, &size);
  wstr_free_string(name_wide);

  return
    ERROR_SUCCESS == result &&
    REG_DWORD == type &&
    sizeof(DWORD) == size;
}

static bool rra_matchmaker_set_partner_name(RRA_Matchmaker* matchmaker, uint32_t index, const char* name)
{
  if (!rra_matchmaker_open_key(matchmaker, index))
    return false;

  WCHAR* name_wide = wstr_from_current(PARTNER_NAME);
  if (!name_wide)
    return false;
  WCHAR* value_wide = wstr_from_current(name);
  if (!value_wide) {
    wstr_free_string(name_wide);
    return false;
  }
  DWORD size = wstrlen(value_wide);

  LONG result = IRAPISession_CeRegSetValueEx(matchmaker->session, matchmaker->keys[index], name_wide, 0, REG_SZ, (BYTE*)value_wide, (size * 2) + 2);
  wstr_free_string(name_wide);
  wstr_free_string(value_wide);

  return ERROR_SUCCESS == result;
}

bool rra_matchmaker_get_partner_name(RRA_Matchmaker* matchmaker, uint32_t index, char** name)
{
  if (!rra_matchmaker_open_key(matchmaker, index))
    return false;

  bool success = false;
  DWORD type;
  DWORD size = 0;
  WCHAR* unicode = NULL;
  WCHAR* name_wide = wstr_from_current(PARTNER_NAME);
  if (!name_wide)
    return FALSE;

  LONG result = IRAPISession_CeRegQueryValueEx(matchmaker->session, matchmaker->keys[index], name_wide, NULL, &type, NULL, &size);

  if (ERROR_SUCCESS == result && REG_SZ == type)
  {
    unicode = calloc(1, size);
    result = IRAPISession_CeRegQueryValueEx(matchmaker->session, matchmaker->keys[index], name_wide, NULL, &type, (LPBYTE)unicode, &size);
  }

  if (ERROR_SUCCESS == result && REG_SZ == type)
  {
    *name = wstr_to_current(unicode);
    success = true;
  }

  free(unicode);
  wstr_free_string(name_wide);

  return success;	
}

static char* rra_matchmaker_get_filename(uint32_t id)
{
  char* path = NULL;
  char filename[256];

  if (!synce_get_subdirectory(RRA_DIRECTORY, &path))
    return NULL;

  snprintf(filename, sizeof(filename), "%s/%s-%08x", path, PARTERSHIP_FILENAME, id);

  free(path);

  return strdup(filename);
}

bool rra_matchmaker_new_partnership(RRA_Matchmaker* matchmaker, uint32_t index)
{
  bool success = false;
  char hostname[256];
  uint32_t other_id = 0;
  uint32_t id = 0;
  char* filename = NULL;
  char* p;

  if (index != 1 && index != 2)
  {
    synce_error("Invalid partnership index: %i", index);
    goto exit;
  }

  if (!rra_matchmaker_get_partner_id(matchmaker, index, &id))
    id = 0;

  if (0 != id)
  {
    synce_error("Partnership exists, not overwriting at index: %i", index);
    goto exit;
  }

  if (0 != gethostname(hostname, sizeof(hostname)))
  {
    synce_error("Failed to get hostname");
    goto exit;
  }

  /* only use the part of the hostname before the first '.' char */
  for (p = hostname; *p != '\0'; p++)
    if ('.' == *p) 
    {
      *p = '\0';
      break;
    }

  if (!rra_matchmaker_get_partner_id(matchmaker, 3-index, &other_id))
    other_id = 0;

  srandom(time(NULL));

  for (;;)
  {
    char* filename = NULL;
    struct stat dummy;
    
    /* better id generation? */
    id = (uint32_t)random();

    filename = rra_matchmaker_get_filename(id);
    
    if (0 == stat(filename, &dummy))
      id = 0;       /* file exists, try another id */

    free(filename);

    if (0 == id || 0xffffffff == id || other_id == id)
      continue;

    break;
  }

  success = false;
  if (rra_matchmaker_set_partner_id(matchmaker, index, id))
  {
    success = rra_matchmaker_set_partner_name(matchmaker, index, hostname);
    if (!success)
    {
      rra_matchmaker_set_partner_id(matchmaker, index, 0);
    }
  }

  if (success)
  {
    FILE* file = NULL;

    if (!(filename = rra_matchmaker_get_filename(id)))
    {
      synce_error("Failed to get filename for partner id %08x", id);
      goto exit;
    }
    
    if ((file = fopen(filename, "w")) == NULL)
    {
      synce_error("Failed to open file for writing: %s", filename);
      goto exit;
    }

    IRAPIDevice *device = IRAPISession_get_device(matchmaker->session);
    const char *name = IRAPIDevice_get_name(device);
    IRAPIDevice_Release(device);

    fprintf(file,
        "[device]\n"
        "name=%s\n"
        "\n"
        "[%s]\n"
        "%s=%i\n"
        "%s=%i\n"
        "%s=%s\n"
        ,
        name,
        PARTERSHIP_SECTION,
        CURRENT_PARTNER,  index,
        PARTNER_ID,       id,
        PARTNER_NAME,     hostname);

    fclose(file);
  }

exit:
  if (filename)
    free(filename);
  return success;
}

bool rra_matchmaker_clear_partnership(RRA_Matchmaker* matchmaker, uint32_t index)
{
  bool success = false;
  uint32_t id = 0;
  char* filename = NULL;

  if (index != 1 && index != 2)
  {
    synce_error("Bad index: %i", index);
    goto exit;
  }

  if (!rra_matchmaker_get_partner_id(matchmaker, index, &id))
    id = 0;

  success = 
    rra_matchmaker_set_partner_id(matchmaker, index, 0) &&
    rra_matchmaker_set_partner_name(matchmaker, index, "");

  if (success)
  {
    if (!(filename = rra_matchmaker_get_filename(id)))
    {
      synce_error("Failed to get filename for partner id %08x", id);
      goto exit;
    }
    
    if (remove(filename))
    {
      synce_error("Failed to erase file: %s", filename);
      goto exit;
    }
  }

exit:
  if (filename)
    free(filename);
  return success;
}

bool rra_matchmaker_have_partnership_at(RRA_Matchmaker* matchmaker, uint32_t index)
{
  bool success = false;
  uint32_t id;
  SynceIni* ini = NULL;

  if (!rra_matchmaker_get_partner_id(matchmaker, index, &id))
    id = 0;

  /* see if we have a partnership file */
  if (id)
  {
    char* filename = rra_matchmaker_get_filename(id);

    if (!filename)
    {
      synce_error("Failed to get filename for partner id %08x", id);
      goto exit;
    }

    ini = synce_ini_new(filename);
    free(filename);

    if (ini)
    {
      const char* local_name = synce_ini_get_string(ini, PARTERSHIP_SECTION, PARTNER_NAME);
      char* remote_name = NULL;

      /* verify that the hostnames match */
      if (local_name &&
          rra_matchmaker_get_partner_name(matchmaker, index, &remote_name) &&
          remote_name &&
          0 == strcmp(local_name, remote_name))
      {
        free(remote_name);
        success = true;
        goto exit;
      }
      else
      {
        synce_trace("Local host name '%s' and remote host name '%s' do not match", 
                    local_name, remote_name);
      }
    }
    else
    {
      synce_trace("Partnership file not found for ID %08x", id);
    }
  }
  else
  {
    synce_trace("Partnership slot %i is empty on device", index);
  }
  
exit:
  synce_ini_destroy(ini);
  return success;
}

bool rra_matchmaker_have_partnership(RRA_Matchmaker* matchmaker, uint32_t* index)
{
  bool success = false;
  int i;

  for (i = 0; i < 2; i++)
  {
    if (rra_matchmaker_have_partnership_at(matchmaker, i+1))
    {
      *index = i+1;
      success = true;
      goto exit;
    }
  }

exit:
  return success;
}

bool rra_matchmaker_create_partnership(RRA_Matchmaker* matchmaker, uint32_t* index)
{
  bool success = false;
  int i;
  uint32_t id;

  if ((success = rra_matchmaker_have_partnership(matchmaker, index)))
    goto exit;

  /* If we get here, we have no partnership with the device.
     We try to create a partnership on an empty slot */

  for (i = 0; i < 2; i++)
  {
    if (!rra_matchmaker_get_partner_id(matchmaker, i+1, &id))
      id = 0;

    if (0 == id)
    {
      if (rra_matchmaker_new_partnership(matchmaker, i+1))
      {
        *index = i+1;
        success = true;
        goto exit;
      }
    }
  }

  synce_error("Partnership not found and there are no empty partner slots on device.");

exit:
  if (success)
    success = rra_matchmaker_set_current_partner(matchmaker, *index);
  return success;
}


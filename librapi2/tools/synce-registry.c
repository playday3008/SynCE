/* $Id$ */
#include <rapi.h>
#include <synce_log.h>
#include <pcommon.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#define ACTION_READVAL 0
#define ACTION_WRITEVAL 1
#define ACTION_DELETEVAL 2
#define ACTION_LISTKEY 3
#define ACTION_NEWKEY 4
#define ACTION_DELETEKEY 5
#define ACTION_DUMP_REGISTRY 6


char* devname = NULL;
int action = ACTION_READVAL;
bool list_recurse = false ; 
DWORD valType = REG_SZ;
const char *prog_name;

#define STR_EQUAL(a,b)  (0 == strcasecmp(a,b))

int read_val(char *parent_str, char *key_str, HKEY key, LPCWSTR value_name_wide) ;


static void show_usage(const char* name)
{
	fprintf(stderr,
			"Syntax:\n"
			"\n"
			"\t%s [-d LEVEL] [-p DEVNAME] [-t TYPE ] [-h]\n"
                        "\t[-r] PARENTKEY KEY VALUE\t\tRead value\n"
                        "\t  -w PARENTKEY KEY VALUE NEWVALUE\tWrite value\n"
                        "\t  -l PARENTKEY KEY VALUE\t\tList key\n"
                        "\t  -n PARENTKEY KEY\t\t\tNew key\n"
                        "\t  -x PARENTKEY KEY VALUE\t\tDelete value (not supported)\n"
                        "\t  -X PARENTKEY KEY\t\t\tDelete key (not supported)\n"
			"\n"
			"\t-d LEVEL     Set debug log level\n"
			"\t                 0 - No logging (default)\n"
			"\t                 1 - Errors only\n"
			"\t                 2 - Errors and warnings\n"
			"\t                 3 - Everything\n"
			"\t-D           Dump complete registry to screen\n"
			"\t-L           Enable list recursion\n"
			"\t-h           Show this help message\n"
                        "\t-p DEVNAME   Mobile device name\n"
                        "\t-t TYPE      New type for writes:\n"
                        "\t\tsz         String (default)\n"
                        "\t\tdword      Double-word\n"
                        "\t\tdword_bige Big-endian double-word\n"
                        "\t\texpand_sz  String with path expansion\n"
                        "\t\tmulti_sz   Multiple strings (not supported)\n"
                        "\t\tbinary     Binary data (not supported)\n"
			"\tPARENTKEY    Registry parent (HKLM, etc.)\n"
			"\tKEY          Registry key\n"
			"\tVALUE        Registry value within key\n"
			"\tNEWVALUE     New value for writes\n"
                        "",
			name
                );
}


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

static bool handle_parameters(int argc, char** argv, char** parent_str, char** key_name, char** value_name, char **new_value)
{
	int c;
	int log_level = SYNCE_LOG_LEVEL_LOWEST;
	while ((c = getopt(argc, argv, "d:hp:t:rwxlnXDL")) != -1)
	{
		switch (c)
		{
			case 'd':
				log_level = atoi(optarg);
				break;

						case 'L':
								list_recurse = true ;
								break ;
                        case 'p':
                                devname = optarg;
                                break;
			
                        case 'r':
                                action = ACTION_READVAL;
                                break;

                        case 'w':
                                action = ACTION_WRITEVAL;
                                break;

                        case 'x':
                                action = ACTION_DELETEVAL;
                                break;

                        case 'l':
                                action = ACTION_LISTKEY;
                                break;

                        case 'n':
                                action = ACTION_NEWKEY;
                                break;

                        case 'X':
                                action = ACTION_DELETEKEY;
                                break;

                        case 'D':
                                action = ACTION_DUMP_REGISTRY;
                                break;
                        
						case 't':
                                if (strcasecmp(optarg,"binary") == 0)
                                {
                                  valType = REG_BINARY;
                                }
                                else if (strcasecmp(optarg,"dword")==0 || strcasecmp(optarg,"dword_little_endian") == 0)
                                {
                                  valType = REG_DWORD;
                                }
                                else if (strcasecmp(optarg,"dword_bige") == 0)
                                {
                                  valType = REG_DWORD_BIG_ENDIAN;
                                }
                                else if (strcasecmp(optarg,"expand_sz") == 0)
                                {
                                  valType = REG_EXPAND_SZ;
                                }
                                else if (strcasecmp(optarg,"multi_sz") == 0)
                                {
                                  valType = REG_MULTI_SZ;
                                }
                                else if (strcasecmp(optarg,"sz") == 0)
                                {
                                  valType = REG_SZ;
                                }
                                else
                                {
                                  fprintf(stderr,"%s: Unrecognized value type '%s' for argument -t\n",argv[0],optarg);
                                  return false;
                                }
                                break;

			case 'h':
			default:
				show_usage(argv[0]);
				return false;
		}
	}
				
	synce_log_set_level(log_level);

        argc -= optind;
        switch(action)
        {
          case ACTION_READVAL:
          case ACTION_DELETEVAL:
            if (argc != 3)
            {
              fprintf(stderr,"Wrong number of parameters\n");
              show_usage(argv[0]);
              return false;
            }
            break;
          case ACTION_WRITEVAL:
            if (argc != 4)
            {
              fprintf(stderr,"Wrong number of parameters\n");
              show_usage(argv[0]);
              return false;
            }
            break;
          case ACTION_DELETEKEY:
          case ACTION_LISTKEY:
            if (argc != 2)
            {
              fprintf(stderr,"Wrong number of parameters\n");
              show_usage(argv[0]);
              return false;
            }
            break;
        }


        switch(argc)
        {
          case 4:
            if ((*new_value = strdup(argv[optind+3])) == NULL)
              return false;
            /* fall through */
          case 3:
            if ((*value_name = strdup(argv[optind+2])) == NULL)
              return false;
            /* fall through */
          case 2:
            if ((*key_name = strdup(argv[optind+1])) == NULL)
              return false;
            /* fall through */
          case 1:
            if ((*parent_str = strdup(argv[optind])) == NULL)
              return false;
        }

	return true;
}

int delete_key(HKEY parent, LPCWSTR key_name_wide)
{
#if 0
  int error;
  
  if (ERROR_SUCCESS != (error=CeRegDeleteKey(parent, key_name_wide)))
  {
    fprintf(stderr,"Failed to delete key '%s': %s\n",
            wstr_to_ascii(key_name),
            synce_strerror(error));
    return 1;
  }
  return 0;
#else
  fprintf(stderr,"CeRegDeleteKey not yet implemented\n");
  return 1;
#endif      
}

int new_key(HKEY parent, LPCWSTR key_name_wide)
{
  int error;
  HKEY new_key;
  DWORD new_disposition;
  
  if (ERROR_SUCCESS != (error=CeRegCreateKeyEx(parent, key_name_wide, 0,0,0,0,NULL,&new_key,&new_disposition)))
  {
    fprintf(stderr,"Failed to create key '%s': %s\n",
            wstr_to_ascii(key_name_wide),
            synce_strerror(error));
    return 1;
  }
  if (ERROR_SUCCESS != (error=CeRegCloseKey(new_key)))
  {
    fprintf(stderr,"Failed to close key '%s': %s\n",
            wstr_to_ascii(key_name_wide),
            synce_strerror(error));
    return 1;
  }

  if (new_disposition != REG_CREATED_NEW_KEY)
  {
    fprintf(stderr,"Key '%s' already exists\n",
            wstr_to_ascii(key_name_wide));
    return 1;
  }
  
  return 0;
}


int list_key(HKEY key, char* key_path, bool do_recurse) 
{ 
  //First print the path till now:
  printf("\n[%s]\n", key_path );


  int result;
  int i;


  //First determine the size of the holding arrays for all 
  //subvalues/keys
  DWORD lpcSubKeys ; 
  DWORD lpcbMaxSubKeyLen ; 
  DWORD lpcbMaxClassLen ; 
  DWORD lpcValues ; 
  DWORD lpcbMaxValueNameLen ; 
  DWORD lpcbMaxValueLen ; 

  result = CeRegQueryInfoKey(key, NULL, NULL, NULL, 
		  &lpcSubKeys, &lpcbMaxSubKeyLen, &lpcbMaxClassLen, 
		  &lpcValues, &lpcbMaxValueNameLen, &lpcbMaxValueLen, 
		  NULL, NULL) ;

  //One important thing is to add +1 to all string lengths, since we need to 
  //add the zero-terminator also, which is not present in the device.
 
  lpcbMaxClassLen++ ; 
  lpcbMaxValueNameLen++ ; 
  lpcbMaxValueLen++ ; 
  lpcbMaxSubKeyLen++ ; 



	//First print all of the values
  for(i = 0; ; i++)
  {
	DWORD value_name_wide_size = lpcbMaxValueNameLen ; 
    WCHAR value_name_wide[value_name_wide_size];
	
	DWORD value_size = lpcbMaxValueLen ; 
	uint8_t value[value_size];

    DWORD value_type;



    result = CeRegEnumValue(key, i, value_name_wide , 
			&value_name_wide_size, NULL, &value_type, 
			value , &value_size );

    if (ERROR_NO_MORE_ITEMS == result)
      break;
    else if (ERROR_SUCCESS != result)
    {
      fprintf(stderr,"Failed to list subkey #%d: %s\n",
              i,
              synce_strerror(result));
      return 1;
    }
	
    printf("\"%s\"=",  wstr_to_ascii(value_name_wide));

    switch (value_type)
    {
      case REG_SZ:
        printf("\"%s\"\n", wstr_to_ascii((LPCWSTR)value));
        break;
      
      case REG_DWORD:
        printf("dword=%08x\n", *(DWORD*)value);
        break;
  
      default:
        dump("Value", value, value_size);
        break;
    }
  }

  
  for(i = 0; ; i++)
  {

    WCHAR wide_name[MAX_PATH];
    DWORD name_size = sizeof(wide_name);
    
    result = CeRegEnumKeyEx(key, i, wide_name, &name_size, NULL, NULL,
                            NULL, NULL);
    if (ERROR_NO_MORE_ITEMS == result)
      break;
    else if (ERROR_SUCCESS != result)
    {
      fprintf(stderr,"Failed to list subkey #%d: %s\n",
              i,
              synce_strerror(result));
      return 1;
    }


	if (!do_recurse){
	  //Then just print all the subkeys
	  printf("\n") ; 
	  fprintf(stdout, "[%s\\%s]\n", key_path, wstr_to_ascii(wide_name)) ; 
	}
	else{
	  HKEY childKey = 0 ; 
	  char* child_key_name = wstr_to_ascii( wide_name ) ; 
	  char child_key_path[MAX_PATH] ; 
	  sprintf(child_key_path,"%s\\%s", key_path, child_key_name) ; 
	  	
	  if (!rapi_reg_open_key( key, child_key_name, &childKey ))
	  { 
	  	return 1 ;
	  }
		
	  //We have childkey now
	  //Do list_key on this
	  list_key( childKey , child_key_path, do_recurse) ; 
    }
  }


  return 0 ; 
}




int dump_registry()
{
  //First the HKLM
  HKEY rootKey = HKEY_LOCAL_MACHINE ;
  
  int result = 0 ; 
  
  result = list_key( rootKey, "HKEY_LOCAL_MACHINE", true) ; 
  
  if (result != 0){
  	return result ; 
  }
  
  //Then the HKCU
  rootKey = HKEY_CURRENT_USER ; 
  
  result = 0 ; 
  
  result = list_key( rootKey, "HKEY_CURRENT_USER", true ) ; 
  
  if (result != 0){
  	return result ; 
  }
  
  
  
  //And finally the HKCR
  rootKey = HKEY_CLASSES_ROOT ; 
  
  result = 0 ; 
  
  result = list_key( rootKey, "HKEY_CLASSES_ROOT", true ) ; 
  
  if (result != 0){
  	return result ; 
  }
  
  return 0 ; 
}





int delete_val(HKEY key, LPCWSTR value_name)
{
#if 0
  int error;
  
  if (ERROR_SUCCESS != (error=CeRegDeleteKey(key, value_name)))
  {
    fprintf(stderr,"Failed to delete value '%s': %s\n",
            wstr_to_ascii(value_name),
            synce_strerror(error));
    return 1;
  }
  return 0;
#else
  fprintf(stderr,"CeRegDeleteValue not yet implemented\n");
  return 1;
#endif      

}


int read_val(char *parent_str, char *key_str, HKEY key, LPCWSTR value_name_wide)
{
  int error;
  DWORD value_size = 0;
  uint8_t* value = NULL;
  DWORD value_type;

  if (ERROR_SUCCESS != (error=CeRegQueryValueEx(key, value_name_wide, NULL, NULL, NULL, &value_size)))
  {
    fprintf(stderr, "Failed to get size of value '%s\\%s\\%s': %s\n", 
        parent_str, key_str, wstr_to_ascii(value_name_wide),
        synce_strerror(error));
    return 1;
  }

  value = calloc(1, value_size);
  if (!value)
  {
    fprintf(stderr, "Failed to allocate %i bytes", value_size);
    return 1;
  }
  
  if (ERROR_SUCCESS != (error=CeRegQueryValueEx(key, value_name_wide, NULL, &value_type, value, &value_size)))
  {
    fprintf(stderr, "Failed to get value '%s\\%s\\%s': %s\n", 
        parent_str, key_str, wstr_to_ascii(value_name_wide),
        synce_strerror(error));
    return 1;
  }

  printf("[%s\\%s]\n\"%s\"=", parent_str, key_str, wstr_to_ascii(value_name_wide));

  switch (value_type)
  {
    case REG_SZ:
      printf("\"%s\"\n", wstr_to_ascii((LPCWSTR)value));
      break;
    
    case REG_DWORD:
      printf("dword=%08x\n", *(DWORD*)value);
      break;

    default:
      dump("Value", value, value_size);
      break;
  }

  return 0;
}


int write_val(HKEY key, LPCWSTR value_name, char * new_value)
{
  int error;
  void *valBuf;
  int valSize;
  DWORD dwordVal;
  
  switch(valType)
  {
    case REG_SZ:
    case REG_EXPAND_SZ:
      valBuf = wstr_from_current(new_value);
      valSize = (wstrlen(valBuf)+1) * sizeof(WCHAR);
      break;
    case REG_DWORD:
      dwordVal = strtol(new_value,NULL,0);
      valBuf = &dwordVal;
      valSize = sizeof(DWORD);
      break;
    case REG_DWORD_BIG_ENDIAN:
      fprintf(stderr,"REG_DWORD_BIG_ENDIAN not yet supported.\n");
      return 1;
    case REG_MULTI_SZ:
      fprintf(stderr,"REG_MULTI_SZ not yet supported.\n");
      return 1;
    default:
      fprintf(stderr,"Unrecognized value type somehow!\n");
      return 1;
  }
  if (ERROR_SUCCESS != (error=CeRegSetValueEx(key,value_name,0,valType,valBuf,valSize)))
  {
    fprintf(stderr, "%s: Unable to set value of '%s' to REG_SZ '%s': %s\n", 
        prog_name,wstr_to_ascii(value_name),new_value,
        synce_strerror(error));
    return 1;
  }
  return 0;
}

int main(int argc, char** argv)
{
  int result = 1;
  RapiConnection* connection = NULL;
  char* parent_str  = NULL;
  char* key_name          = NULL;
  char* value_name  = NULL;
  char* new_value   = NULL;
  HKEY parent;
  HKEY key = 0;
  WCHAR* value_name_wide = NULL;
  WCHAR* key_name_wide = NULL;
  HRESULT hr;

  prog_name = argv[0];
  
  if (!handle_parameters(argc,argv,&parent_str,&key_name,&value_name,&new_value))
    goto exit;


  //Add this before anything, since we don't need the
  //parent_str etc..
  if (action==ACTION_DUMP_REGISTRY){
	  if ((connection = rapi_connection_from_name(devname)) == NULL)
	  {
		  fprintf(stderr, "%s: Could not find configuration at path '%s'\n", 
				  argv[0],
				  devname?devname:"(Default)");
		  goto exit;
	  }
	  rapi_connection_select(connection);
	  if (S_OK != (hr = CeRapiInit()))
	  {
		  fprintf(stderr, "%s: Unable to initialize RAPI: %s\n", 
				  argv[0],
				  synce_strerror(hr));
		  goto exit;
	  }

	  result = dump_registry() ;
	  if (result != 0){
		  fprintf(stdout, "SOMETHING WENT WRONG!!!!\n" ) ; 
	  }
	  goto exit;
  }



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

  if ((connection = rapi_connection_from_name(devname)) == NULL)
  {
    fprintf(stderr, "%s: Could not find configuration at path '%s'\n", 
        argv[0],
        devname?devname:"(Default)");
    goto exit;
  }
  rapi_connection_select(connection);
  if (S_OK != (hr = CeRapiInit()))
  {
    fprintf(stderr, "%s: Unable to initialize RAPI: %s\n", 
        argv[0],
        synce_strerror(hr));
    goto exit;
  }

  convert_to_backward_slashes(key_name);
  key_name_wide = wstr_from_current(key_name);

  if (action == ACTION_DELETEKEY)
  {
    result = delete_key(parent, key_name_wide);
    goto exit;
  }
  else if (action == ACTION_NEWKEY)
  {
    result = new_key(parent, key_name_wide);
    goto exit;
  }

  if (!rapi_reg_open_key(parent, key_name, &key))
  {
    fprintf(stderr, "Failed to open key: '%s\\%s'\n", parent_str, key_name);
    goto exit;
  }

  if (action == ACTION_LISTKEY)
  {
	char path[255] ; 
	sprintf(path,"%s\\%s", parent_str, key_name ) ; 
    result = list_key(key, path, list_recurse);
    goto exit;
  }

  value_name_wide = wstr_from_current(value_name);

  if (action == ACTION_DELETEVAL)
  {
    result = delete_val(key, value_name_wide);
  }
  else if (action == ACTION_READVAL)
  {
    result = read_val(parent_str, key_name, key, value_name_wide);
  }
  else if (action == ACTION_WRITEVAL)
  {
    result = write_val(key, value_name_wide, new_value);
  }
  
exit:
  if (key)
    CeRegCloseKey(key);
  CeRapiUninit();
  if (key_name)
    free(key_name);
  if (key_name_wide)
    wstr_free_string(key_name_wide);
  if (value_name_wide)
    wstr_free_string(value_name_wide);
  return result;
}


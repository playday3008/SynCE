
/* Config file parsed data structure. Don't deal with this direcly, it's yucky.
 * Use the config file routines. 
*/

/* A 64k configuration file is definitely bordering on viciously insane */
#define MAXIMUM_SANE_FILESIZE (64 * 1024)
#define _cfgMAX_JUMPBLOCK_ELEMENTS 128

#include "hash.h"

enum _cfgFinding {_cfgSECTIONEND, _cfgCOMMENTEND, _cfgKEYSTART, _cfgKEYEND,
	_cfgCOLON, _cfgVALSTART, _cfgVALEND};

struct configFile
{
	hash_table *sections;
	unsigned char *bbdg; /* The Big Block of Delicious Goo */
	size_t bbdgSize;
};

struct configFile *readConfigFile (char *filename);
struct configFile *_cfgParseConfigFile (struct configFile *cfg);
struct _cfgjumpBlock *_cfgNewJumpBlock (struct _cfgjumpBlock *current);
void _cfgdbgPrintConfigFile ();
int getConfigInt (struct configFile *cfg, char *section, char *key);
char *getConfigString (struct configFile *cfg, char *section, char *key);
int getConfigDouble (struct configFile *cfg, char *section, char *key);
void unloadConfigSection (void *data);
void unloadConfigFile (struct configFile *cfg);
void _cfgdbgPrintConfigFile(struct configFile *cfg);

/* $Id$ */
#include "rapi_internal.h"
#include "rapi.h"
#include "rapi_buffer.h"
#include "rapi_context.h"
#include "config/config.h"

HRESULT CeRapiInit(void)
{
	HRESULT result = E_UNEXPECTED;
	struct configFile* config;
	char* hostname;
	char* password;

	config = readConfigFile("/tmp/rapi.conf"); /* XXX: maybe use another path :-) */
	if (!config)
		return E_INVALIDARG;

	hostname = getConfigString(config, "device", "hostname");
	if (!hostname)
	{
		result = E_FAIL;
		goto fail;
	}

	/* TODO: more code here */

	result = S_OK;

fail:
	unloadConfigFile(config);
	return result;
}


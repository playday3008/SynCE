/* $Id$ */
#include "rapi_internal.h"
#include "rapi.h"
#include "rapi_buffer.h"
#include "rapi_context.h"
#include "config/config.h"
#include <synce_socket.h>
#include <string.h>

#define RAPI_PORT  990

HRESULT CeRapiFreeBuffer( 
		LPVOID Buffer)
{
	free(Buffer);
	return S_OK;
}

static bool rapi_login(SynceSocket* socket, const char* password, int key, unsigned char* reply)/*{{{*/
{
	bool success = false;
	bool password_correct = false;
	
	synce_trace("password=\"%s\", key=0x%08x", password, key);
	
	if (!reply)
	{
		synce_error("reply parameter is NULL");
		goto exit;
	}

	if (!synce_password_send(socket, password, key))
	{
		synce_error("failed to send password");
		goto exit;
	}

	if (!synce_password_recv_reply(socket, &password_correct))
	{
		synce_error("failed to get password reply");
		goto exit;
	}

	success = true;
	
exit:
	return success;
}/*}}}*/

HRESULT CeRapiInit(void)/*{{{*/
{
	RapiContext* context = rapi_context_current();
	HRESULT result = E_UNEXPECTED;
	struct configFile* config = NULL;
	char* hostname = NULL;
	char* password = NULL;
	int key = 0;

	if (context->is_initialized)
	{
		/* Fail immediately */
		return CERAPI_E_ALREADYINITIALIZED;
	}

	config = readConfigFile("/tmp/rapi.conf"); /* XXX: maybe use another path :-) */
	if (!config)
		return E_INVALIDARG;

	hostname = getConfigString(config, "device", "hostname");
	if (!hostname)
	{
		result = E_FAIL;
		goto fail;
	}

	if ( !synce_socket_connect(context->socket, hostname, RAPI_PORT) )
	{
		result = E_FAIL;
		goto fail;
	}

	password = getConfigString(config, "device", "password");
	key = getConfigInt(config, "device", "key");
	
	if (password && strlen(password) == 4)
	{
		unsigned char reply;
		if ( !rapi_login(context->socket, password, key, &reply) )
			goto fail;
	}

	context->is_initialized = true;
	result = S_OK;

fail:
	unloadConfigFile(config);
	return result;
}/*}}}*/

STDAPI CeRapiUninit(void)/*{{{*/
{
	RapiContext* context = rapi_context_current();
	
	if (context->is_initialized)
	{
		synce_socket_close(context->socket);
		context->is_initialized = false;
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}/*}}}*/

BOOL CeCheckPassword( /*{{{*/
		LPWSTR lpszPassword)
{
	RapiContext* context = rapi_context_current();
	BOOL return_value = 0;
	
	rapi_context_begin_command(context, 0x34);
	rapi_buffer_write_optional_string(context->send_buffer, lpszPassword);

	if ( !rapi_context_call(context) )
		return false;
	
	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &return_value);

	return return_value;
}/*}}}*/


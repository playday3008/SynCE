/* $Id$ */
#undef __STRICT_ANSI__
#define _GNU_SOURCE
#include "rapi_internal.h"
#include "rapi.h"
#include "rapi_buffer.h"
#include "rapi_context.h"
#include "config/config.h"
#include <synce_socket.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

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

	if (!synce_password_recv_reply(socket, 1, &password_correct))
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
	char* filename = NULL;
	struct configFile* config = NULL;
	char* ip_str = NULL;
	char* password = NULL;
	int key = 0;
	pid_t dccm_pid = 0;

	if (context->is_initialized)
	{
		/* Fail immediately */
		return CERAPI_E_ALREADYINITIALIZED;
	}

	if (!synce_get_connection_filename(&filename))
	{
		synce_error("failed to get connection filename");
		result = E_FAIL;
		goto fail;
	}

	config = readConfigFile(filename);
	if (!config)
	{
		synce_error("unable to open file: %s", filename);
		result = E_FAIL;
		goto fail;
	}

	dccm_pid = getConfigInt(config, "dccm", "pid");
	if (!dccm_pid)
	{
		synce_error("pid entry not found in %s", filename);
		result = E_FAIL;
		goto fail;
	}

	if (kill(dccm_pid, 0) < 0)
	{
		synce_error("dccm not running with pid %i", dccm_pid);
		result = E_FAIL;
		goto fail;
	}

	ip_str = getConfigString(config, "device", "ip");
	if (!ip_str)
	{
		synce_error("ip entry not found in %s", filename);
		result = E_FAIL;
		goto fail;
	}

	if ( !synce_socket_connect(context->socket, ip_str, RAPI_PORT) )
	{
		synce_error("failed to connect to %s", ip_str);
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
	if (filename)
		free(filename);
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


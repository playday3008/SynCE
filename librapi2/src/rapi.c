/* $Id$ */
#include "rapi_internal.h"
#include "rapi.h"
#include "rapi_buffer.h"
#include "rapi_context.h"
#include "rapi_wstr.h"
#include "rapi_endian.h"
#include "config/config.h"

#define RAPI_PORT  990

HRESULT CeRapiFreeBuffer( 
		LPVOID Buffer)
{
	free(Buffer);
	return S_OK;
}

static bool rapi_login(RapiSocket* socket, const char* password, int key, unsigned char* reply)
{
	bool success = false;
	unsigned char* encoded = NULL;
	size_t length = 0;
	size_t size = 0;
	uint16_t size_le = 0;
	size_t i = 0;
	
	rapi_trace("password=\"%s\", key=0x%08x", password, key);
	
	if (!password)
	{
		rapi_error("password parameter is NULL");
		goto exit;
	}

	if (!reply)
	{
		rapi_error("reply parameter is NULL");
		goto exit;
	}
	
	length = strlen(password);
	
	if (4 != length)
	{
		rapi_error("password is not four bytes");
		goto exit;
	}

	encoded = (unsigned char*)rapi_wstr_from_ascii(password);

	size = 2*(length+1);
	for (i = 0; i < size; i++)
	{
		encoded[i] ^= key;
	}

	size_le = htole16((uint16_t)size);

	if ( !rapi_socket_write(socket, &size_le, sizeof(uint16_t)) )
	{
		rapi_error("failed to write buffer size to socket");
		goto exit;
	}

	if ( !rapi_socket_write(socket, encoded, size) )
	{
		rapi_error("failed to write encoded password to socket");
		goto exit;
	}

	if ( !rapi_socket_read(socket, reply, 1) )
	{
		rapi_error("failed to write encoded password to socket");
		goto exit;
	}

	rapi_trace("reply=0x%02x", *reply);

	success = true;
	
exit:
	rapi_wstr_free_string(encoded);
	return success;
}

HRESULT CeRapiInit(void)
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

	if ( !rapi_socket_connect(context->socket, hostname, RAPI_PORT) )
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
}

STDAPI CeRapiUninit(void)
{
	RapiContext* context = rapi_context_current();
	
	if (context->is_initialized)
	{
		rapi_socket_close(context->socket);
		context->is_initialized = false;
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

BOOL CeCheckPassword( 
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
}


/* $Id$ */
#undef __STRICT_ANSI__
#define _GNU_SOURCE
#include "rapi_context.h"
#include <stdlib.h>
#include "config/config.h"
#include <synce_socket.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

#define CERAPI_E_ALREADYINITIALIZED  0x8004101

#define RAPI_PORT  990

#define RAPI_CONTEXT_DEBUG 0

#if RAPI_CONTEXT_DEBUG
#define rapi_context_trace(args...)    synce_trace(args)
#define rapi_context_warning(args...)  synce_warning(args)
#else
#define rapi_context_trace(args...)
#define rapi_context_warning(args...)
#endif
#define rapi_context_error(args...)    synce_error(args)

static RapiContext* current_context;

RapiContext* rapi_context_current()/*{{{*/
{
	/* TODO: make thread-safe version of this with thread private variables */

	if (!current_context)
	{
		current_context = rapi_context_new();
	}

	return current_context;
}/*}}}*/

RapiContext* rapi_context_new()/*{{{*/
{
	RapiContext* context = calloc(sizeof(RapiContext), 1);

	if (context)
	{
		if (!(
					(context->send_buffer  = rapi_buffer_new()) &&
					(context->recv_buffer = rapi_buffer_new()) &&
					(context->socket = synce_socket_new())
				 ))
		{
			rapi_context_free(context);
			return NULL;
		}
	}

	return context;
}/*}}}*/

void rapi_context_free(RapiContext* context)/*{{{*/
{
	if (context)
	{
		rapi_buffer_free(context->send_buffer);
		rapi_buffer_free(context->recv_buffer);
		synce_socket_free(context->socket);
		free(context);
	}
}/*}}}*/

HRESULT rapi_context_connect(RapiContext* context)
{
	HRESULT result = E_FAIL;
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
		goto fail;
	}

	config = readConfigFile(filename);
	if (!config)
	{
		synce_error("unable to open file: %s", filename);
		goto fail;
	}

	dccm_pid = getConfigInt(config, "dccm", "pid");
	if (!dccm_pid)
	{
		synce_error("pid entry not found in %s", filename);
		goto fail;
	}

	if (kill(dccm_pid, 0) < 0)
	{
		synce_error("dccm not running with pid %i", dccm_pid);
		goto fail;
	}

	ip_str = getConfigString(config, "device", "ip");
	if (!ip_str)
	{
		synce_error("ip entry not found in %s", filename);
		goto fail;
	}

	if ( !synce_socket_connect(context->socket, ip_str, RAPI_PORT) )
	{
		synce_error("failed to connect to %s", ip_str);
		goto fail;
	}

	password = getConfigString(config, "device", "password");
	key = getConfigInt(config, "device", "key");
	
	if (password && strlen(password))
	{
		bool password_correct = false;

		if (!synce_password_send(context->socket, password, key))
		{
			synce_error("failed to send password");
			goto fail;
		}

		if (!synce_password_recv_reply(context->socket, 1, &password_correct))
		{
			synce_error("failed to get password reply");
			goto fail;
		}

		if (!password_correct)
		{
			synce_error("invalid password");
			goto fail;
		}
	}

	context->is_initialized = true;
	result = S_OK;

fail:
	if (filename)
		free(filename);
	unloadConfigFile(config);
	return result;
}

bool rapi_context_begin_command(RapiContext* context, uint32_t command)/*{{{*/
{
	rapi_context_trace("command=0x%02x", command);
	
	rapi_buffer_free_data(context->send_buffer);
	
	if ( !rapi_buffer_write_uint32(context->send_buffer, command) )
		return false;

	return true;
}/*}}}*/
	
bool rapi_context_call(RapiContext* context)/*{{{*/
{
	if ( !rapi_buffer_send(context->send_buffer, context->socket) )
	{
		rapi_context_error("synce_socket_send failed");
		/* TODO: set context->last_error */
		return false;
	}

	if ( !rapi_buffer_recv(context->recv_buffer, context->socket) )
	{
		rapi_context_error("synce_socket_recv failed");
		/* TODO: set context->last_error */
		return false;
	}

	/* this is a boolean? */
	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->result_1) )
	{
		rapi_context_error("reading result_1 failed");
		return false;
	}

	rapi_context_trace("result 1 = 0x%08x", context->result_1);

	if (1 == context->result_1)
	{
		/* this is a HRESULT */
		if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->result_2) )
		{
			rapi_context_error("reading result_2 failed");
			return false;
		}

		rapi_context_error("result 2 = 0x%08x", context->result_2);

		/*	if (context->result_2 != 0)
				return false;*/
	}

	return true;
}/*}}}*/


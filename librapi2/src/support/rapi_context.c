/* $Id$ */
#undef __STRICT_ANSI__
#define _GNU_SOURCE
#include "rapi_context.h"
#include <stdlib.h>
#include <synce_socket.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#define USE_THREAD_SAFE_VERSION 1
#ifdef USE_THREAD_SAFE_VERSION
#include <pthread.h>
#endif

#define CERAPI_E_ALREADYINITIALIZED  0x8004101

#define RAPI_PORT  990

#define RAPI_CONTEXT_DEBUG 0

#if RAPI_CONTEXT_DEBUG
#define rapi_context_trace(args...)    synce_trace(args)
#define rapi_context_warning(args...)  synce_warning(args)
#else
#define rapi_context_trace(args...)    synce_trace(args)
#define rapi_context_warning(args...)  synce_warning(args)
#endif
#define rapi_context_error(args...)    synce_error(args)



#ifdef USE_THREAD_SAFE_VERSION
/*This holds the value for the key where the contexts are stored*/
static pthread_key_t context_key = -1;
/*This makes sure that the a key is created only once, we can also use pthread for this*/
static pthread_once_t key_is_created = PTHREAD_ONCE_INIT;
#else
static RapiContext* current_context;

#endif


extern struct rapi_ops_s rapi_ops;
extern struct rapi_ops_s rapi2_ops;


#ifdef USE_THREAD_SAFE_VERSION
/* This method will create a key to hold the context_key
 * with which each thread can access the thread local 
 * rapi_context.
 * It must be called ONLY once, there for this method
 * will always be used in combination with the 
 * pthread_once method.
 * Do not run it multiple times, this will result in 
 * unpredictable behaviour and might crash things!
 */
void create_pthread_key(){
	pthread_key_create( &context_key, NULL ) ;
}
#endif


RapiContext* rapi_context_current()/*{{{*/
{
#ifdef USE_THREAD_SAFE_VERSION
	/* If the key for the thread_local context variables was not initalized yet,
	 * do that now
	 */
	(void) pthread_once(&key_is_created, create_pthread_key);

	/* Get the thread local current_context */
	RapiContext* thread_current_context = (RapiContext*)pthread_getspecific(context_key) ;

	/* If the thread local context is not initialized, setup a new one
	 * DON'T forget to reload the thread local information that is updated 
	 * by the rapi_context_set(rapi_context_new())!
	 */
	if (!thread_current_context)
	{
		rapi_context_set(rapi_context_new());
		thread_current_context = (RapiContext*)pthread_getspecific(context_key) ;
	}
	
	return thread_current_context;
#else
	if (!current_context)
	{
			rapi_context_set(rapi_context_new());
	}

	return current_context;
#endif

}/*}}}*/

void rapi_context_set(RapiContext* context)
{
#ifdef USE_THREAD_SAFE_VERSION
	/* If the key for the thread_local context variables was not initalized yet,
	 * do that now
	 */
	(void) pthread_once(&key_is_created, create_pthread_key);
	
	/* Set the context for this thread in its local thread variable */
	pthread_setspecific(context_key, (void*) context ) ;
#else
	current_context = context;
#endif
}



RapiContext* rapi_context_new()/*{{{*/
{
	RapiContext* context = calloc(1, sizeof(RapiContext));

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
    if (context == rapi_context_current())
      rapi_context_set(NULL);

		rapi_buffer_free(context->send_buffer);
		rapi_buffer_free(context->recv_buffer);
		synce_socket_free(context->socket);
		free(context);
	}
}/*}}}*/

HRESULT rapi_context_connect(RapiContext* context)
{
    HRESULT result = E_FAIL;
    SynceInfo* info = NULL;

    if (context->is_initialized)
    {
        /* Fail immediately */
            return CERAPI_E_ALREADYINITIALIZED;
    }

    if (context->info)
        info = context->info;
    else
        info = synce_info_new(NULL);
    if (!info)
    {
        synce_error("Failed to get connection info");
        goto fail;
    }

    const char *transport = synce_info_get_transport(info);
    /*
     *  original dccm or vdccm
     */
    if (transport == NULL || ( strcmp(transport, "odccm") != 0 && strcmp(transport, "hal") != 0 ) ) {
        if (!synce_info_get_dccm_pid(info))
        {
            synce_error("DCCM PID entry not found for current connection");
            goto fail;
        }

        if (kill(synce_info_get_dccm_pid(info), 0) < 0)
        {
            if (errno != EPERM)
            {
                synce_error("DCCM not running with pid %i", synce_info_get_dccm_pid(info));
                goto fail;
            }
        }

        if (!synce_info_get_device_ip(info))
        {
            synce_error("IP entry not found for current connection");
            goto fail;
        }
    }

    if (transport == NULL || strncmp(transport,  "ppp", 3) == 0) {
        /*
         *  original dccm or vdccm
         */
        if ( !synce_socket_connect(context->socket, synce_info_get_device_ip(info), RAPI_PORT) )
        {
            synce_error("failed to connect to %s", synce_info_get_device_ip(info));
            goto fail;
        }

        const char *password = synce_info_get_password(info);
        if (password && strlen(password))
        {
            bool password_correct = false;

            if (!synce_password_send(context->socket, password, synce_info_get_key(info)))
            {
                synce_error("failed to send password");
                result = E_ACCESSDENIED;
                goto fail;
            }

            if (!synce_password_recv_reply(context->socket, 1, &password_correct))
            {
                synce_error("failed to get password reply");
                result = E_ACCESSDENIED;
                goto fail;
            }

            if (!password_correct)
            {
                synce_error("invalid password");
                result = E_ACCESSDENIED;
                goto fail;
            }
        }
        context->rapi_ops = &rapi_ops;
    } else {
        /*
         *  odccm, synce-hal, or proxy ?
         */
        if (strcmp(transport, "odccm") == 0 || strcmp(transport, "hal") == 0) {
            synce_socket_take_descriptor(context->socket, synce_info_get_fd(info));
        }
        else if ( !synce_socket_connect_proxy(context->socket, synce_info_get_device_ip(info)) )
        {
            synce_error("failed to connect to proxy for %s", synce_info_get_device_ip(info));
            goto fail;
        }

	/* rapi 2 seems to be used on devices with OS version of 5.1 or greater */

        int os_major = 0, os_minor = 0;
        synce_info_get_os_version(info, &os_major, &os_minor);
	if ((os_major > 4) && (os_minor > 0))
	  context->rapi_ops = &rapi2_ops;
	else
	  context->rapi_ops = &rapi_ops;
    }

    context->is_initialized = true;
    result = S_OK;

fail:
    if (!context->info)
        synce_info_destroy(info);
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
	context->rapi_error = E_UNEXPECTED;

	if ( !rapi_buffer_send(context->send_buffer, context->socket) )
	{
		rapi_context_error("rapi_buffer_send failed");
		context->rapi_error = E_FAIL;
		return false;
	}

	if ( !rapi_buffer_recv(context->recv_buffer, context->socket) )
	{
		rapi_context_error("rapi_buffer_recv failed");
		context->rapi_error = E_FAIL;
		return false;
	}

	/* this is a boolean? */
	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->result_1) )
	{
		rapi_context_error("reading result_1 failed");
		context->rapi_error = E_FAIL;
		return false;
	}

	rapi_context_trace("result 1 = 0x%08x", context->result_1);

	if (1 == context->result_1)
	{
		/* this is a HRESULT */
		if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->result_2) )
		{
			rapi_context_error("reading result_2 failed");
			context->rapi_error = E_FAIL;
			return false;
		}

		rapi_context_error("result 2 = 0x%08x", context->result_2);

		context->rapi_error = context->result_2;
		if (context->result_2 != 0)
		  return false;
	}

	context->rapi_error = S_OK;
	return true;
}/*}}}*/

bool rapi2_context_call(RapiContext* context)/*{{{*/
{
    context->rapi_error = E_UNEXPECTED;

    if ( !rapi_buffer_send(context->send_buffer, context->socket) )
    {
        rapi_context_error("rapi_buffer_send failed");
        context->rapi_error = E_FAIL;
        return false;
    }

    if ( !rapi_buffer_recv(context->recv_buffer, context->socket) )
    {
        rapi_context_error("rapi_buffer_recv failed");
        context->rapi_error = E_FAIL;
        return false;
    }

    context->rapi_error = S_OK;
    return true;
}/*}}}*/


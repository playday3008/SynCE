/* $Id$ */
#include <check.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "rapi_socket.h"
#include "rapi_buffer.h"

#define HOST "127.0.0.1"
#define PORT 0xde0

static const char* MESSAGE = "2GooD Productions";

START_TEST(test_echo_server)
{
	RapiSocket* socket;
	RapiBuffer* send_buffer = rapi_buffer_new();
	RapiBuffer* recv_buffer = rapi_buffer_new();
	bool success;

	signal(SIGPIPE, SIG_IGN);
	
	socket = rapi_socket_new();
	success = socket != NULL;
	
	fail_unless( success, "rapi_socket_new failed" );
	if (!success) goto fail;

	success = rapi_socket_connect(socket, HOST, PORT);
	fail_unless( success, "rapi_socket_connect failed" );
	if (!success) goto fail;

	success = rapi_buffer_write_data(send_buffer, MESSAGE, strlen(MESSAGE));
	fail_unless( success, "rapi_buffer_write_data failed" );
	if (!success) goto fail;

	success = rapi_buffer_send(send_buffer, socket);
	fail_unless( success, "rapi_socket_send failed" );
	if (!success) goto fail;

	success = rapi_buffer_recv(send_buffer, socket);
	fail_unless( success, "rapi_socket_recv failed" );
	if (!success) goto fail;

	success = rapi_buffer_get_raw(recv_buffer) != NULL;
	fail_unless( success, "arapi_socket_recv_buffer_get_raw returns null" );
	if (!success) goto fail;

	success = memcmp(MESSAGE, rapi_buffer_get_raw(recv_buffer), strlen(MESSAGE)) == 0;
	fail_unless( success , "wrong data returned" );
	if (!success) goto fail;

fail:
	success = false;
}
END_TEST

Suite *internals_suite (void) 
{ 
	Suite *s = suite_create ("librapi sockets"); 
	TCase *tc_socket = tcase_create ("rapi_socket");

	suite_add_tcase (s, tc_socket);
	tcase_add_test (tc_socket, test_echo_server); 

	return s; 
}

int main (void) 
{ 
	int nf; 
	Suite *s = internals_suite (); 
	SRunner *sr = srunner_create (s); 
	srunner_run_all (sr, CK_NORMAL); 
	nf = srunner_ntests_failed (sr); 
	srunner_free (sr); 
	suite_free (s); 
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE; 
}



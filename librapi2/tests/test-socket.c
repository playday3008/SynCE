/* $Id$ */
#include <check.h>
#include <stdlib.h>

#include "rapi_socket.h"

#define HOST "127.0.0.1"
#define PORT 9877

static const char* MESSAGE = "2GooD Productions\n";

START_TEST(test_echo_server)
{
	RapiSocket* socket;
	RapiBuffer* send_buffer = rapi_buffer_new();
	RapiBuffer* recv_buffer = rapi_buffer_new();
	bool success;

	socket = rapi_socket_new();
	
	fail_unless( socket != NULL, "rapi_socket_new failed" );

	if (socket)
	{
		success = rapi_socket_connect(socket, HOST, PORT);
		fail_unless( success, "rapi_socket_connect failed" );

		success = rapi_buffer_write_data(send_buffer, MESSAGE, strlen(MESSAGE));
		fail_unless( success, "rapi_buffer_write_data failed" );

		success = rapi_socket_send(socket, send_buffer);
		fail_unless( success, "rapi_socket_send failed" );

		success = rapi_socket_recv(socket, recv_buffer);
		fail_unless( success, "rapi_socket_recv failed" );
		
		success = strcmp(MESSAGE, rapi_buffer_get_raw(recv_buffer)) == 0;
		fail_unless( success , "wrong data returned" );
	}

fail:
	
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



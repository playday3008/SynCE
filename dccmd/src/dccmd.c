/* $Id$ */
#include <synce_socket.h>
#include <synce_log.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#define DCCM_PORT           	5679
#define DCCM_PING_INTERVAL   	10         /* seconds */
#define DCCM_PING            	0x12345678
#define DCCM_MAX_PING_COUNT   5					/* max number of pings without reply */
#define DCCM_MAX_PACKET_SIZE	512
#define DCCM_MIN_PACKET_SIZE	0x24

#define RESULT_SUCCESS 0
#define RESULT_FAILURE 1

typedef struct _Client
{
	SynceSocket* socket;
	int ping_count;
	bool locked;
	bool expect_password_reply;
} Client;

/* Dump a block of data for debugging purposes */
void
dump(desc, data, len)
	const char *desc;
	void *data;
	size_t len;
{
	u_int8_t *buf = data;
	int i, j;
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
			u_int8_t c = buf[j + i];
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
}




bool is_daemon = true;

/**
 * Write help message to stderr
 */
void write_help(char *name)
{
	fprintf(
			stderr, 
			"Syntax:\n"
			"\n"
			"\t%s [-d] [-h]\n"
			"\n"
			"\t-h Show this help message\n"
			"\t-d Do not run as daemon\n",
			name);
}

/**
 * Parse parameters to daemon
 */
bool handle_parameters(int argc, char** argv)
{
	int c;

	while ((c = getopt(argc, argv, "dh")) != -1)
	{
		switch (c)
		{
			/*
			 * The -d parameter specifies that we don't want to run as a daemon
			 */
			case 'd':
				is_daemon = false;
				break;

			case 'h':
			default:
				write_help(argv[0]);
				return false;
		}
	}

	return true;
}

/**
 * Begin listening on server port
 */
SynceSocket* start_socket_server()
{
	SynceSocket* server = NULL;

	server = synce_socket_new();
	if (!server)
		goto error;

	if (!synce_socket_listen(server, NULL, DCCM_PORT))
		goto error;

	return server;

error:
	if (server)
		synce_socket_free(server);

	return NULL;
}

/**
 * Look at an offset in a buffer for a string offset pointing to a string
 * in the same buffer.
 */
static char* string_at(unsigned char* buffer, size_t size, size_t offset)
{
	size_t string_offset = letoh32(*(uint32_t*)(buffer + offset));

	if (string_offset < size)
	{
		return wstr_to_ascii((WCHAR*)(buffer + string_offset));
	}
	else
	{
		synce_error("String offset too large: 0x%08x", string_offset);
		return NULL;
	}
}

static bool read_client(Client* client)
{
	bool success = false;
	char* buffer = NULL;
	uint32_t header;

	if (!synce_socket_read(client->socket, &header, sizeof(header)))
	{
		synce_error("failed to read header");
		goto exit;
	}

	header = letoh32(header);
	synce_trace("Read header: 0x%08x", header);

	if (0 == header)
	{
		/*
		 * Empty - ignore
		 */
		synce_trace("empty package");
	}
	else if (DCCM_PING == header)
	{
		/*
		 * This is a ping reply
		 */
		synce_trace("this is a ping reply");
		client->ping_count = 0;
	}
	else if (header < DCCM_MAX_PACKET_SIZE)
	{
		/*
		 * This is an information message
		 */
		uint32_t offset = 0;
		char *name = NULL;
		char *class = NULL;
		char *hardware = NULL;

		synce_trace("this is an information message");

		if (header < DCCM_MIN_PACKET_SIZE)
		{
			synce_error("Packet is smaller than expected");
			goto exit;
		}

		buffer = malloc(header);

		if (!buffer)
		{
			synce_error("Failed to allocate %i (0x%08x) bytes", header, header);
			goto exit;
		}

		if (!synce_socket_read(client->socket, buffer, header))
		{
			synce_error("Failed to read package");
			goto exit;
		}

		dump("info package", buffer, header);

		name      = string_at(buffer, header, 0x18);
		class     = string_at(buffer, header, 0x1c);
		hardware  = string_at(buffer, header, 0x20);

		synce_trace("name    : %s", name);
		synce_trace("class   : %s", class);
		synce_trace("hardware: %s", hardware);

		wstr_free_string(name);
		wstr_free_string(class);
		wstr_free_string(hardware);

		free(buffer);
		buffer = NULL;

		if (client->locked)
			client->expect_password_reply = true;
	}
	else
	{
		/*
		 * This is a password challenge
		 */

		int key = header & 0xff;

		synce_trace("this is a password challenge");

		if (!synce_password_send("1234", key, client->socket))
		{
			synce_error("failed to send password");
			goto exit;
		}

		/* XXX: not read anything? */

		client->locked = true;
	}

	success = true;

exit:
	if (buffer)
		free(buffer);

	return success;
}

/**
 * Take care of a client
 */
static bool handle_client(SynceSocket* socket)
{
	bool success = false;
	Client client;
	SocketEvents events;
	const uint32_t ping = htole32(DCCM_PING);

	memset(&client, 0, sizeof(client));
	client.socket = socket;
	
	while (client.ping_count < DCCM_MAX_PING_COUNT)
	{
		/*
		 * Wait for event on socket
		 */
		events = EVENT_READ | EVENT_TIMEOUT;

		if (!synce_socket_wait(socket, DCCM_PING_INTERVAL, &events))
		{
			synce_error("synce_socket_wait failed");
			goto exit;	
		}

		synce_trace("got events 0x%08x", events);

		if (events & EVENT_READ)
		{
			if (client.expect_password_reply)
			{
				uint16_t reply;
				if (!synce_socket_read(socket, &reply, sizeof(reply)))
				{
					synce_error("failed to read password reply");
					goto exit;	
				}

				synce_trace("password reply = 0x%04x (%i)", reply, reply);

				synce_trace("Password was %s", reply ? "correct!" : "incorrect :-(");
				if (!reply)
					goto exit;

				client.expect_password_reply = false;
			}
			else
			{
				if (!read_client(&client))
				{
					synce_error("failed to read from client");
					goto exit;
				}
			}
		}
		else if (events & EVENT_TIMEOUT)
		{
			synce_trace("timeout event: sending ping");
			
			if (!synce_socket_write(socket, &ping, sizeof(ping)))
			{
				synce_error("failed to send ping");
				goto exit;	
			}

			client.ping_count++;
		}
		else
		{
			synce_error("unexpected events: %i", events);
			goto exit;
		}
	}

	synce_trace("Finished with client");

	success = true;

exit:
	return success;
}

/**
 * Start here...
 */
int main(int argc, char** argv)
{
	int result = RESULT_FAILURE;
	SynceSocket* server = NULL;
	SynceSocket* client = NULL;
	
	if (!handle_parameters(argc, argv))
		goto exit;

	if (is_daemon)
	{
		synce_trace("Forking into background");
		daemon(0,0);
	}
	else
		synce_trace("Running in foreground");

	server = start_socket_server();
	if (!server)
	{
		synce_error("Failed to start socket server");
		goto exit;
	}

	for (;;)
	{
		/*
		 * Currently only handles one client...
		 */

		struct sockaddr_in client_address;

		client = synce_socket_accept(server, &client_address);
		if (!client)
		{
			synce_error("Failed to accept client");
			goto exit;
		}

		synce_trace("client is connected");

		if (!handle_client(client))
		{
			synce_error("Failed to handle client connection");
			goto exit;
		}

		synce_socket_free(client);
	}

	result = RESULT_SUCCESS;

exit:
	synce_socket_free(server);
	synce_socket_free(client);
	
	return result;
}


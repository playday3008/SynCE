/* $Id$ */
#include <synce_socket.h>
#include <synce_log.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>

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
	struct sockaddr_in address;
	int ping_count;
	bool locked;
	bool expect_password_reply;
	int key;
	char* name;
	char* class;
	char* hardware;
} Client;

/* Dump a block of data for debugging purposes */
static void
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




static bool is_daemon = true;
static char* password = NULL;

/**
 * Write help message to stderr
 */
static void write_help(char *name)
{
	fprintf(
			stderr, 
			"Syntax:\n"
			"\n"
			"\t%s [-d] [-f] [-h] [-p password]\n"
			"\n"
			"\t-d LEVEL  Set debug log level\n"
			"\t              0 - No logging (default)\n"
			"\t              1 - Errors only\n"
			"\t              2 - Errors and warnings\n"
			"\t              3 - Everything\n"
			"\t-f        Do not run as daemon\n"
			"\t-h        Show this help message\n",
			name);
}

/**
 * Parse parameters to daemon
 */
static bool handle_parameters(int argc, char** argv)
{
	int c;
	int log_level = SYNCE_LOG_LEVEL_LOWEST;

	while ((c = getopt(argc, argv, "d:fhp:")) != -1)
	{
		switch (c)
		{
			case 'd':
				log_level = atoi(optarg);
				break;
			
			/*
			 * The -f parameter specifies that we want to run in the foreground
			 */
			case 'f':
				is_daemon = false;
				break;

			case 'p':
				if (password)
					free(password);
				password = strdup(optarg);
				break;

			case 'h':
			default:
				write_help(argv[0]);
				return false;
		}
	}

	synce_log_set_level(log_level);

	return true;
}

/**
 * Begin listening on server port
 */
static SynceSocket* start_socket_server()
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

static bool client_write_file(Client* client)
{
	bool success = false;
	char* filename = NULL;
	FILE* file = NULL;
	char ip_str[16];

	if (!inet_ntop(AF_INET, &client->address.sin_addr, ip_str, sizeof(ip_str)))
	{
		synce_error("inet_ntop failed");
		goto exit;
	}

	if (!synce_get_connection_filename(&filename))
	{
		synce_error("Unable to get connection filename");
		goto exit;
	}

	if ((file = fopen(filename, "w")) == NULL)
	{
		synce_error("Failed to open file for writing: %s", filename);
		goto exit;
	}

	fprintf(file,
			"# Modifications to this file will be lost next time a client connects to dccmd\n"
			"\n"
			"[dccmd]\n"
			"pid=%i\n"
			"\n"
			"[device]\n"
			"name=%s\n"
			"class=%s\n"
			"hardware=%s\n"
			"ip=%s\n"
			"port=%i\n",
			getpid(),
			client->name,
			client->class,
			client->hardware,
			ip_str,
			ntohs(client->address.sin_port));
	
	if (client->locked)
	{
		fprintf(file,
				"password=%s\n"
				"key=%i\n",
				password,
				client->key
				);
	}

	success = true;
			
exit:
	if (file)
		fclose(file);
	if (filename)
		free(filename);
	return success;
}

static void remove_connection_file()
{
	char* filename = NULL;
	
	if (!synce_get_connection_filename(&filename))
	{
		synce_error("Unable to get connection filename");
		goto exit;
	}

	unlink(filename);

exit:
	if (filename)
		free(filename);
}


static bool client_read(Client* client)
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

		client->name      = string_at(buffer, header, 0x18);
		client->class     = string_at(buffer, header, 0x1c);
		client->hardware  = string_at(buffer, header, 0x20);

		synce_trace("name    : %s", client->name);
		synce_trace("class   : %s", client->class);
		synce_trace("hardware: %s", client->hardware);

		free(buffer);
		buffer = NULL;

		client_write_file(client);

		if (client->locked)
			client->expect_password_reply = true;
	}
	else
	{
		/*
		 * This is a password challenge
		 */

		client->key = header & 0xff;

		synce_trace("this is a password challenge");

		if (!synce_password_send(client->socket, password, client->key))
		{
			synce_error("failed to send password");
			goto exit;
		}

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
static bool client_handler(Client* client)
{
	bool success = false;
	SocketEvents events;
	const uint32_t ping = htole32(DCCM_PING);

	while (client->ping_count < DCCM_MAX_PING_COUNT)
	{
		/*
		 * Wait for event on socket
		 */
		events = EVENT_READ | EVENT_TIMEOUT;

		if (!synce_socket_wait(client->socket, DCCM_PING_INTERVAL, &events))
		{
			synce_error("synce_socket_wait failed");
			goto exit;	
		}

		synce_trace("got events 0x%08x", events);

		if (events & EVENT_READ)
		{
			if (client->expect_password_reply)
			{
				bool password_correct = false;

				if (!synce_password_recv_reply(client->socket, 2, &password_correct))
				{
					synce_error("failed to read password reply");
					goto exit;	
				}

				if (!password_correct)
					goto exit;

				client->expect_password_reply = false;
			}
			else
			{
				if (!client_read(client))
				{
					synce_error("failed to read from client");
					goto exit;
				}
			}
		}
		else if (events & EVENT_TIMEOUT)
		{
			synce_trace("timeout event: sending ping");
			
			if (!synce_socket_write(client->socket, &ping, sizeof(ping)))
			{
				synce_error("failed to send ping");
				goto exit;	
			}

			client->ping_count++;
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
	
	if (!handle_parameters(argc, argv))
		goto exit;

	if (is_daemon)
	{
		synce_trace("Forking into background");
		daemon(0,0);
	}
	else
		synce_trace("Running in foreground");

	/* ignore broken pipes */
	signal(SIGPIPE, SIG_IGN);

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

		Client client;
		memset(&client, 0, sizeof(client));

		client.socket = synce_socket_accept(server, &client.address);
		if (!client.socket)
		{
			synce_error("Failed to accept client");
			goto exit;
		}

		synce_trace("client is connected");

		if (!client_handler(&client))
		{
			synce_error("Failed to handle client connection");
			/* Better luck on next client? :-) */
		}

		wstr_free_string(client.name);
		wstr_free_string(client.class);
		wstr_free_string(client.hardware);

		synce_socket_free(client.socket);
		remove_connection_file();
	}

	result = RESULT_SUCCESS;

exit:
	if (password)
		free(password);

	synce_socket_free(server);
	
	return result;
}


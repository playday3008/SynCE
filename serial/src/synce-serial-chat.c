/* $Id$ */
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define CLIENT_SIZE        6
#define CLIENTSERVER_SIZE  (CLIENT_SIZE + 6)

static const char client[] = "CLIENT";
static const char clientserver[] = "CLIENTSERVER";

int main(int argc, char** argv)
{
	char buffer[CLIENT_SIZE+1];
	ssize_t bytes_left = CLIENT_SIZE;
	ssize_t bytes_read;
	ssize_t bytes_read_total = 0;

	if (1 != argc)
	{
		printf("%s: A program that expects the string '%s' and responds with '%s'\n",
				argv[0], client, clientserver);
		return 1;
	}
	
	memset(buffer, 0, sizeof(buffer));

	bytes_left = CLIENT_SIZE;

	while (bytes_left > 0)
	{
		bytes_read = read(STDIN_FILENO, buffer+bytes_read_total, bytes_left);

		if (bytes_read < 0)
			break;
	
		bytes_left -= bytes_read;	
		bytes_read_total += bytes_read;
	}

	if (bytes_left != 0)
	{
		fprintf(stderr, "Unable to read %i bytes\n", CLIENT_SIZE);
		return 1;
	}

	if (memcmp(buffer, client, CLIENT_SIZE) != 0)
	{
		fprintf(stderr, "Received string not 'CLIENT' but '%s'\n", buffer);
		return 1;
	}

	if (write(STDOUT_FILENO, clientserver, CLIENTSERVER_SIZE) != CLIENTSERVER_SIZE)
	{
		fprintf(stderr, "Unable to write '%s'\n", clientserver);
		return 1;
	}
	
	return 0;
}

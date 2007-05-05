/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>

#define LISTENQ  1024


int main()
{
	int result = 1;
	int listenfd;
	int connfd;
	struct sockaddr_in servaddr;
	struct sockaddr_in cliaddr;
	socklen_t clilen;
	size_t size;
	uint32_t size_le;
	char* buffer;
	int i;
	
	if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
		goto fail;

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_port = htons(0xde0);
	if ( inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) <= 0 )
		goto fail;

	if ( bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0 )
	{
		printf("Bind failed.\n");
		goto fail;
	}

	if ( listen(listenfd, LISTENQ) < 0 )
		goto fail;
	
	clilen = sizeof(cliaddr);
	if ( (connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen)) < 0 )
		goto fail;

	printf("Connected.\n");
	
	close(listenfd);

	if ( sizeof(size_le) != read(connfd, &size_le, sizeof(size_le)) )
		goto fail;

	printf("Read size = %08x.\n", size_le);
	
	/* XXX: size_le is little endian */
	size = size_le;
	
	if ( (buffer = malloc(size)) == NULL )
		goto fail;

	if ( size != read(connfd, buffer, size) )
		goto fail;

	for (i = 0; i < size; i++)
	{
		if (i > 0)
			printf(",");
		
		if (isprint(buffer[i]))
			printf("'%c'", buffer[i]);
		else
			printf("%02x", buffer[i]);
	}
	printf("\n");

	if ( sizeof(size_le) != write(connfd, &size_le, sizeof(size_le)) )
		goto fail;
	
	if ( size_le != write(connfd, buffer, size_le) )
		goto fail;
	
	printf("Write finished.\n");

	close(connfd);
	
	result = 0;

fail:
	return result;
}


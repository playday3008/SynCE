/* $Id$ */
#include "synce.h"
#include "synce_log.h"
#include "synce_socket.h"
#include <string.h>

/**
 * Free an encoded password returned from synce_password_encode()
 */
static void synce_password_free(unsigned char *encodedPassword)
{
	wstr_free_string(encodedPassword);
}

/**
 * Encode a password with a key
 */
static bool synce_password_encode(
		const char *asciiPassword,
		unsigned char key,
		unsigned char **encodedPassword,
		size_t *encodedPasswordSize)
{	
	int length;
	int i;
	
	*encodedPassword = NULL;
	
	if (!asciiPassword)
	{
		synce_error("password parameter is NULL");
		goto error;
	}

	length = strlen(asciiPassword);

	if (4 != length)
	{
		synce_error("password is not four bytes");
		goto error;
	}

	*encodedPassword      = (unsigned char*)wstr_from_ascii(asciiPassword);
	*encodedPasswordSize  = 2 * (length + 1);

	for (i = 0; i < *encodedPasswordSize; i++)
	{
		(*encodedPassword)[i] ^= key;
	}

	return true;

error:
	synce_password_free(*encodedPassword);
	*encodedPassword = NULL;
	return false;
}

/**
 * Encode and send password on a socket
 */
bool synce_password_send(
		SynceSocket* socket,
		const char* asciiPassword,
		unsigned char key)
{
	bool success = false;
	unsigned char* encoded_password = NULL;
	size_t size = 0;
	uint16_t size_le = 0;
	
	if (!synce_password_encode(asciiPassword, key, &encoded_password, &size))
	{
		synce_error("failed to encode password");
	}

	size_le = htole16((uint16_t)size);

	if ( !synce_socket_write(socket, &size_le, sizeof(uint16_t)) )
	{
		synce_error("failed to write buffer size to socket");
		goto exit;
	}

	if ( !synce_socket_write(socket, encoded_password, size) )
	{
		synce_error("failed to write encoded password to socket");
		goto exit;
	}

	success = true;

exit:
	synce_password_free(encoded_password);
	return success;
}

bool synce_password_recv_reply(
		SynceSocket* socket,
		bool* passwordCorrect)
{
	bool success = false;
	uint16_t reply;

	if (!synce_socket_read(socket, &reply, sizeof(reply)))
	{
		synce_error("failed to read password reply");
		goto exit;	
	}

	synce_trace("password reply = 0x%04x (%i)", reply, reply);

	synce_trace("Password was %s", reply ? "correct!" : "incorrect :-(");

	*passwordCorrect = reply;
	success = true;

exit:
	return success;
}


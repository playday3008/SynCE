/* $Id$ */
#include "synce.h"
#include "synce_log.h"
#include "synce_socket.h"
#include <string.h>

/** 
 * @defgroup SynceMisc Error and password handling
 * @ingroup SynceUtils
 * @brief Functions for reporting errors, and handling device passwords
 *
 * @{ 
 */ 

/**
 * Free an encoded password returned from synce_password_encode()
 */
static void synce_password_free(unsigned char *encodedPassword)
{
	wstr_free_string(encodedPassword);
}

/** @brief Encode a password with a key
 * 
 * This function encodes the given password with the
 * given key, ready to send to a locked device.
 * 
 * @param[in] asciiPassword the password to encode
 * @param[in] key the key to use for encoding
 * @param[out] location to store the encoded password
 * @param[out] size of the encoded password
 * @return TRUE on success, FALSE on failure
 */ 
bool synce_password_encode(
		const char *asciiPassword,
		unsigned char key,
		unsigned char **encodedPassword,
		size_t *encodedPasswordSize)
{	
	size_t length;
	unsigned i;
	
	*encodedPassword = NULL;
	
	if (!asciiPassword)
	{
		synce_error("password parameter is NULL");
		goto error;
	}

	length = strlen(asciiPassword);

	*encodedPassword      = (unsigned char*)wstr_from_utf8(asciiPassword);
	*encodedPasswordSize  = sizeof(WCHAR) * (length + 1);

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

/** @brief Encode and send password on a socket
 * 
 * This function encodes the given password with the
 * given key, and sends the result on the specified
 * connected socket.
 * 
 * @param[in] socket client socket to send the password over
 * @param[in] asciiPassword the password to encode
 * @param[in] key the key to use for encoding
 * @return TRUE on success, FALSE on failure
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

/** @brief Receive response to a sent password
 * 
 * This function reads the response from a locked device
 * after a password has been sent.
 * 
 * The reply is 2 bytes long, if this is called with a 1 byte bool
 * then the other byte must subsequently be read and discarded.
 *
 * @param[in] socket client socket to receive teh response
 * @param[in] size size of the passwordCorrect parameter
 * @param[in] passwordCorrect whether the password was accepted
 * @return TRUE on success, FALSE on failure
 */ 
bool synce_password_recv_reply(
		SynceSocket* socket,
		size_t size,
		bool* passwordCorrect)
{
	bool success = false;
	union
	{
		uint8_t   byte;
		uint16_t  word;
	} reply;

	if (size < 1 || size > 2)
	{
		synce_error("invalid size");
		goto exit;	
	}

	if (!synce_socket_read(socket, &reply, size))
	{
		synce_error("failed to read password reply");
		goto exit;	
	}

	if (size == 1)
	{
		/*synce_trace("password reply = 0x%02x (%i)", reply.byte, reply.byte);*/
		*passwordCorrect = reply.byte;
	}
	else /* size == 2 */
	{
		reply.word = letoh16(reply.word);
		/*synce_trace("password reply = 0x%04x (%i)", reply.word, reply.word);*/
		*passwordCorrect = reply.word;
	}

	/*synce_trace("Password was %s", *passwordCorrect ? "correct!" : "incorrect :-(");*/
	success = true;

exit:
	return success;
}

/** @} */

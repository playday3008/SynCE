#ifndef __rra_h__
#define __rra_h__ 1

#include <synce_socket.h>

class Error { };
class SocketError : public Error { };
class ListenFailed : public SocketError { };
class WaitFailed : public SocketError { };
class ReadFailed : public SocketError { };
class WriteFailed : public SocketError { };
class AvailableFailed : public SocketError { };

class Socket
{
	public:
		Socket()
		{
			mSocket = synce_socket_new();
		}

		~Socket()
		{
			synce_socket_free(mSocket);
		}

		Socket* Accept(struct sockaddr_in& address)
		{
			SynceSocket* socket = synce_socket_accept(mSocket, &address);
			if (socket)
				return new Socket(socket);
			else
				return NULL;
		}

		void Listen(const char* host, int port)
		{
			if (!synce_socket_listen(mSocket, host, port))
				throw ListenFailed();
		}

		void Wait(int timeout, SocketEvents& events)
		{
			if (!synce_socket_wait(mSocket, timeout, &events))
				throw WaitFailed();
		}

		void Read(void* data, unsigned size)
		{
			if (!synce_socket_read(mSocket, data, size))
				throw ReadFailed();
		}

		uint16_t Read16()
		{
			uint16_t value;
			Read(&value, sizeof(value));
			return value;
		}

		uint32_t Read32()
		{
			uint32_t value;
			Read(&value, sizeof(value));
			return value;
		}

		uint8_t* Read(unsigned size)
		{
			uint8_t* data = new uint8_t[size];
			if (!synce_socket_read(mSocket, data, size))
			{
				delete[] data;
				throw ReadFailed();
			}
			return data;
		}

		void Write(void* data, unsigned size)
		{
			if (!synce_socket_write(mSocket, data, size))
				throw WriteFailed();
		}

		unsigned Available()
		{
			unsigned count = 0;
			if (!synce_socket_available(mSocket, &count))
				throw AvailableFailed();
			return count;
		}
	

	private:
		Socket(SynceSocket* socket)
		{
			mSocket = socket;
		}

	private:
		SynceSocket* mSocket;
};

#endif


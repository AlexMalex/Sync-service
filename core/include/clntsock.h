#if !defined(CLNTSOCK_H)
#define CLNTSOCK_H

#include "socket.h"

using namespace socket_base;

class TClientSocket: public TBaseSocket {
public:
	TClientSocket(socket_type type=TCP);
	virtual ~TClientSocket();

	bool connect(in_addr address, int port);
	bool connect(char* address, int port);
};

typedef TClientSocket* PClientSocket;
typedef TClientSocket& RClientSocket;

class TServerClientSocket: public TBaseSocket {
protected:
	in_addr peer;

public:
	TServerClientSocket(SOCKET s, in_addr address);
	virtual ~TServerClientSocket();

	in_addr getPeerAddress();
};

inline in_addr TServerClientSocket::getPeerAddress()
{
	return peer;
}

typedef TServerClientSocket* PServerClientSocket;
typedef TServerClientSocket& RServerClientSocket;

#endif

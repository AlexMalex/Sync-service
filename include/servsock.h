#if !defined(SERVSOCK_H)
#define SERVSOCK_H

#include "clntsock.h"

using namespace socket_base;

class TServerSocket: public TBaseSocket {
public:
	TServerSocket(int port);
	virtual ~TServerSocket();

	bool waitforclient(DWORD);

	PServerClientSocket accept(DWORD wait=0);
};

typedef TServerSocket* PServerSocket;
typedef TServerSocket& RServerSocket;

#endif

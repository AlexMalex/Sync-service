#include "servsock.h"

#pragma warning (disable: 4127) // // while (true)

TServerSocket::TServerSocket(int port): TBaseSocket(TCP)
{
	int err;
	in_addr address;

	address.S_un.S_addr=INADDR_ANY;

	TBaseSocket::bind(address, port);

	err=::listen(s, MAX_CLIENTS);

	if (err==0) connected=true;
}

TServerSocket::~TServerSocket()
{
}

bool TServerSocket::waitforclient(DWORD msec)
{
	int err;
	fd_set read_set;
	TIMEVAL time;

	if (!isValid()) return FALSE;
	
	time.tv_sec=0;
	time.tv_usec=msec;

	FD_ZERO(&read_set);

	FD_SET(s, &read_set);

	err=select(s+1, &read_set, NULL, NULL, &time);
	
	if ((err==SOCKET_ERROR)||(err==0)) return FALSE;
	
	return TRUE;
}

PServerClientSocket TServerSocket::accept(DWORD wait)
{
	SOCKET client;
	sockaddr_in address;
	int size=sizeof(address);

	if (!isConnected()) return NULL;

	if (!waitforclient(wait)) return NULL;
		
	client=::accept(s, (sockaddr*)&address, &size);

	if (client==INVALID_SOCKET) return NULL;

	return new TServerClientSocket(client, address.sin_addr);
}

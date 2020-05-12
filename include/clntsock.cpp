#include "clntsock.h"

TClientSocket::TClientSocket(socket_type type): TBaseSocket(type)
{
}

TClientSocket::~TClientSocket()
{
}

bool TClientSocket::connect(in_addr address, int port)
{
	sockaddr_in addr;
	int err, size=sizeof(addr);

	addr.sin_family=AF_INET;
	addr.sin_addr=address;
	addr.sin_port=htons((u_short)port);

	err=::connect(s, (const sockaddr*)&addr, size);

	if (err==SOCKET_ERROR) return FALSE;

	connected=true;

	return TRUE;
}

bool TClientSocket::connect(char* address, int port)
{
	hostent* ip_addr;
	in_addr addr;

	ip_addr=gethostbyname(address);

	if (ip_addr==NULL) return false;
	
	if (ip_addr->h_addr_list[0]==NULL) return false;

	addr.s_addr=*(u_long*)ip_addr->h_addr_list[0];

	if ((addr.S_un.S_addr==INADDR_NONE)||(addr.S_un.S_addr==INADDR_ANY)) return FALSE;

	return TClientSocket::connect(addr, port);
}

TServerClientSocket::TServerClientSocket(SOCKET s, in_addr address):TBaseSocket(s), peer(address)
{
}

TServerClientSocket::~TServerClientSocket()
{
}

#include "socket.h"
#include "mswsock.h"

using namespace socket_base;

// static function
bool TBaseSocket::Init() 
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	
	if (!Initialized) {
		wVersionRequested=MAKEWORD(2, 2);

		err=WSAStartup(wVersionRequested, &wsaData);

		if (err==NO_ERROR) Initialized=TRUE;
	};

	return Initialized;
}

// static function
void TBaseSocket::Done()
{
	if (Initialized) {
		WSACleanup();

		Initialized=FALSE;
	};
}

TBaseSocket::TBaseSocket(socket_type type):s_type(type), s(INVALID_SOCKET), read_bytes(0), connected(false), datagram_max_size(0)
{
	int protocol=IPPROTO_TCP;

	if (type==UDP) protocol=IPPROTO_UDP;
	
	s=socket(AF_INET, type, protocol);

	setTimeOut(STANDART_WAIT);

	getSystemMaxMsgSize();
}

TBaseSocket::TBaseSocket(SOCKET in): s_type(TCP), s(in), read_bytes(0), connected(false),  datagram_max_size(0)
{
	DWORD sock_t, connected_time;
	int size=sizeof(sock_t), err;

	err=getsockopt(s, SOL_SOCKET, SO_TYPE, (char*)&sock_t, &size);

	if (err==NO_ERROR) {
		if (sock_t!=SOCK_STREAM) s_type=UDP;
		else s_type=TCP;
		
		setTimeOut(STANDART_WAIT);

		if (s_type==TCP) {
			err=getsockopt(s, SOL_SOCKET, SO_CONNECT_TIME, (char*)&connected_time, &size);

			if (err==NO_ERROR) connected=true;
		};

		getSystemMaxMsgSize();
	};
}

TBaseSocket::~TBaseSocket()
{
	if (isConnected()) shutdown(s, SD_BOTH);

	if (isValid()) closesocket(s);
}

bool TBaseSocket::bind(char* address, int port)
{
	in_addr addr;

	addr.S_un.S_addr=inet_addr(address);

	if ((addr.S_un.S_addr==INADDR_NONE)||(addr.S_un.S_addr==INADDR_ANY)) return FALSE;

	return TBaseSocket::bind(addr, port);
}

bool TBaseSocket::bind(in_addr address, int port)
{
	sockaddr_in addr;

	if (!isValid()) return FALSE;

	addr.sin_family=AF_INET;
	addr.sin_port=htons((u_short)port);
	addr.sin_addr=address;
	
	return (::bind(s, (const sockaddr*)&addr, sizeof(addr))==NO_ERROR);
}

void TBaseSocket::setTimeOut(DWORD msec)
{
	if (isValid()) setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&msec, sizeof(DWORD));
}

void TBaseSocket::getSystemMaxMsgSize()
{
	UINT  msg_size;
	int size=sizeof(msg_size), err;

	if (!isValid()) return;

	if (s_type==UDP) {
		err=getsockopt(s, SOL_SOCKET, SO_MAX_MSG_SIZE, (char*)&msg_size, &size);

		if (err==NO_ERROR) datagram_max_size=msg_size;
	};
}

bool TBaseSocket::read(void* buf, int size)
{
	int err;

	read_bytes=0;

	if (!isConnected()) return FALSE;

	err=recv(s, (char*)buf, size, 0);

	if (err==SOCKET_ERROR) {
		err=WSAGetLastError();

		if (err!=WSAETIMEDOUT) connected=false;

		return FALSE;
	};

	if (err==0) {
		read_bytes=0;

		shutdown(s, SD_BOTH);

		connected=false;

		return FALSE;
	};

	read_bytes=err;

	return TRUE;
}

bool TBaseSocket::write(void* buf, int size)
{
	int err;

	if (!isConnected()) return FALSE;

	err=send(s, (char*)buf, size, 0);

	if (err==SOCKET_ERROR) {
		connected=false;

		return FALSE;
	};

	if (err<size) return FALSE;

	return TRUE;
}

void TBaseSocket::set_write_buffer(int size)
{
	if (isValid()) setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*)&size, sizeof(size));
}

void TBaseSocket::set_read_buffer(int size)
{
	if (!isValid()) setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&size, sizeof(size));
}

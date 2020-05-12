#if !defined(SOCKET_H)
#define SOCKET_H

#include <winsock2.h>

namespace socket_base {

#define STANDART_WAIT 200
#define MAX_CLIENTS 25

enum socket_type {TCP=SOCK_STREAM, UDP=SOCK_DGRAM};

static bool Initialized=FALSE;

class TBaseSocket {
protected:
	SOCKET s;
	socket_type s_type;
	int read_bytes;
	bool connected;
	UINT datagram_max_size;

	bool bind(char* address, int port=0);
	bool bind(in_addr address, int port=0);

	TBaseSocket(socket_type type=TCP);
	TBaseSocket(SOCKET in);

	void getSystemMaxMsgSize();
public:
	virtual ~TBaseSocket();

	void setTimeOut(DWORD msec);

	bool read(void* buf, int size);
	bool write(void* buf, int size);

	void set_write_buffer(int size);
	void set_read_buffer(int size);

	int get_read_bytes();

	UINT getMaxMsgSize();

	bool isValid();
	bool isConnected();

	static bool Init();
	static void Done();
};

typedef TBaseSocket* PBaseSocket;
typedef TBaseSocket& RBaseSocket;

inline int TBaseSocket::get_read_bytes()
{
	return read_bytes;
}

inline bool TBaseSocket::isValid()
{
	return (s!=INVALID_SOCKET)&&(s!=NULL);
}


inline bool TBaseSocket::isConnected()
{
	 return connected;
}

inline UINT TBaseSocket::getMaxMsgSize()
{
	return datagram_max_size;
}

}

#endif

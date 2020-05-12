#if !defined(WEB_SRV_H)
#define WEB_SRV_H

#include "servsock.h"
#include "sync.h"
#include "thread.h"

class TWebSrv: public TThread, TServerSocket {
private:
	PServerClientSocket socket;

	PEvent stopEvent;
	BOOL terminated;

	void error(int);

	void process(int);

	void status();
	void bad_points();

public:
	TWebSrv(int);
	~TWebSrv();

	virtual DWORD execute();

	void stat(PServerClientSocket);

	void terminate();
};

inline void TWebSrv::terminate()
{
	terminated=true;
}

typedef TWebSrv* PWebSrv;
typedef TWebSrv& RWebSrv;

#endif

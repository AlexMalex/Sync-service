#if !defined(CLNTTHREAD_H)
#define CLNTTHREAD_H

#include "servsock.h"
#include "thread.h"
#include "mainclass.h"

class TClientThread: public TThread, public TMainClass {
public:
	TClientThread(PServerClientSocket);
	virtual ~TClientThread();

	virtual DWORD execute();
};

typedef TClientThread* PClientThread;
typedef TClientThread& RClientThread;

#endif

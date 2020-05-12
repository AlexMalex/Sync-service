#if !defined(THREAD_H)
#define THREAD_H

#include "handle.h"

#define MAX_WAIT_TIME	2000 // max time wait thread to stop

enum ThreadState { tsCreated,
                   tsRunning,
                   tsSuspended,
                   tsEnded
                 };

class TThread;

typedef TThread* PThread;
typedef TThread& RThread;

class TThread : public THandleObject {
private:
	DWORD thid;

protected:
	TThread(bool Suspended=false);

public:
	ThreadState threadState;

	virtual ~TThread();

	virtual DWORD execute()=0;		// abstrtact method - must override

	void resume();
	void suspend();

	void kill();

	int getPriority();
	void setPriority(int);

	BOOL waitFor(DWORD);
  
	DWORD getId();

	ThreadState getState();

	BOOL threadTerminated();
};

inline int TThread::getPriority()
{
	return GetThreadPriority(handle);
}

inline void TThread::setPriority(int priority)
{
	SetThreadPriority(handle, priority);
}

inline BOOL TThread::waitFor(DWORD timeOut)
{
	return (WaitForSingleObject(handle, timeOut)==WAIT_OBJECT_0); 
}

inline DWORD TThread::getId()
{
	return thid;
}

inline BOOL TThread::threadTerminated()
{
	return (threadState==tsEnded);
}

#endif

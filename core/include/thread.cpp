#include "thread.h"

DWORD __stdcall threadFunc(PThread thread)
{
	DWORD ret;
 
	thread->threadState=tsRunning;
	ret=thread->execute();
	thread->threadState=tsEnded;

	return ret;
}

TThread::TThread(bool suspended): THandleObject()
{
	DWORD flags=0;

	threadState=tsCreated;

	if (suspended) {
		threadState=tsSuspended;
  
		flags=CREATE_SUSPENDED;
	};

	handle=CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&threadFunc, (void*)this, flags, &thid);
}

TThread::~TThread()
{
	if (threadState!=tsEnded) {
		if (threadState==tsSuspended) resume();
   
		if (!waitFor(MAX_WAIT_TIME)) TerminateThread(handle, 1);
	};
}

void TThread::resume()
{
	if (threadState==tsSuspended) {
		if (ResumeThread(handle)!=0xFFFFFFFF) threadState=tsRunning;
	};
}

void TThread::suspend()
{
	if (threadState==tsRunning) {
		if (SuspendThread(handle)!=0xFFFFFFFF) threadState=tsSuspended;
	};
}

void TThread::kill()
{
	if (threadState!=tsEnded) {
		if (threadState==tsSuspended) resume();

		if (threadState==tsRunning) {
			TerminateThread(handle, 1);

			threadState=tsEnded;
		};
	};
}

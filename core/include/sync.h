#if !defined(SYNC_H)
#define SYNC_H

#include "handle.h"

class TSyncObject: public THandleObject {
private:
	bool sharedFlag;

protected:
	TSyncObject();

	void setShared();
  
public:
	DWORD waitFor(DWORD);
	bool isShared();
};

inline DWORD TSyncObject::waitFor(DWORD interval)
{
	return WaitForSingleObject(handle, interval);
}

inline bool TSyncObject::isShared()
{
	return sharedFlag;
}

inline void TSyncObject::setShared()
{
	sharedFlag=(GetLastError()==ERROR_ALREADY_EXISTS);
}

class TEvent: public TSyncObject {
public:
	TEvent(char*, bool=false, bool=false);

	void setEvent();
	void resetEvent();
	void pulseEvent();
};

typedef TEvent* PEvent;
typedef TEvent& REvent;

inline void TEvent::setEvent()
{
	SetEvent(handle);
}

inline void TEvent::resetEvent()
{
	ResetEvent(handle);
}

inline void TEvent::pulseEvent()
{
	PulseEvent(handle);
}

class TMutex: public TSyncObject {
public:
	TMutex(char*, bool=false);

	void release();
};

typedef TMutex* PMutex;
typedef TMutex& RMutex;

inline void TMutex::release()
{
	ReleaseMutex(handle);
}

class TSemaphore: public TSyncObject {
public:
	TSemaphore(char*, DWORD, DWORD=0);

	void release(DWORD=1);
};

typedef TSemaphore* PSemaphore;
typedef TSemaphore& RSemaphore;

inline void TSemaphore::release(DWORD releaseCount)
{
	ReleaseSemaphore(handle, releaseCount, NULL);
}

class TCriticalSection {
private:
	CRITICAL_SECTION section;

public:
	TCriticalSection();
	virtual ~TCriticalSection();

	void enter();
	void leave();
};

typedef TCriticalSection* PCriticalSection;
typedef TCriticalSection& RCriticalSection;

inline void TCriticalSection::enter()
{
	EnterCriticalSection(&section);
}

inline void TCriticalSection::leave()
{
	LeaveCriticalSection(&section);
}

#endif

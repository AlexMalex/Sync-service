#include "sync.h"

TSyncObject::TSyncObject(): THandleObject()
{
	sharedFlag=false;
}

TEvent::TEvent(char* name, bool manualReset, bool state): TSyncObject()
{
	handle=CreateEvent(NULL, manualReset, state, name);

	setShared();
}

TMutex::TMutex(char* name, bool owned): TSyncObject()
{
	handle=CreateMutex(NULL, owned, name);

	setShared();
}

TSemaphore::TSemaphore(char* name, DWORD maxCount, DWORD startCount): TSyncObject()
{
	handle=CreateSemaphore(NULL, startCount, maxCount, name);

	setShared(); 
}

TCriticalSection::TCriticalSection()
{
	InitializeCriticalSection(&section);
}

TCriticalSection::~TCriticalSection()
{
	DeleteCriticalSection(&section);
}

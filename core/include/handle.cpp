#include "handle.h"

THandleObject::THandleObject()
{
	handle=NULL;
}

THandleObject::~THandleObject()
{
	if (isValid())
		CloseHandle(handle);
}

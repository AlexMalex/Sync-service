#include "protobj.h"

TProtectedObject::TProtectedObject(): TCriticalSection(), need_free(false)
{
}

TProtectedObject::~TProtectedObject()
{
}

#include "configobj.h"

TConfigObject::TConfigObject(char* str, DWORD new_id):TString(str), id(new_id)
{
}

TConfigObject::~TConfigObject()
{
}

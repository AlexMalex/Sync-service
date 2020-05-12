#include "filestruct.h"

TFileStruct::TFileStruct(char* name, object_type what): type(what)
{
	path=new TString(name);
}

TFileStruct::~TFileStruct()
{
	if (path!=NULL) delete path;
}

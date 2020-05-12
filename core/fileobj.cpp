#include "fileobj.h"
#include "tstring.h"

TFileObject::TFileObject(char* temp_str, char* file_str, DWORD attr, FILETIME date_write, DWORD id, char* cur_str): temp_file(NULL), file(NULL), cur_file(NULL)
{
	temp_file=new TString(temp_str);

	file=new TString(file_str);

	attributes=attr;

	date=date_write;

	objectId=id;

	if (cur_str!=NULL) cur_file=new TString(cur_str);
}

TFileObject::~TFileObject()
{
	if (cur_file!=NULL) delete cur_file;

	if (file!=NULL) delete file;

	if (temp_file!=NULL) delete temp_file;
}


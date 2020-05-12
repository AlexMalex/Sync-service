#if !defined(FILEOBJ_H)
#define FILEOBJ_H

#include "object.h"
#include "tstring.h"

#include <windows.h>

class TFileObject : public TObject {
public:
	TFileObject(char*, char*, DWORD, FILETIME, DWORD, char* =NULL);
	virtual ~TFileObject();

	PString temp_file;
	PString file;
	PString cur_file;

	DWORD attributes;
	FILETIME date;

	DWORD objectId;
};

typedef TFileObject* PFileObject;
typedef TFileObject& RFileObject;

#endif

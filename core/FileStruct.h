#if !defined(FILSTRUCT_H)
#define FILSTRUCT_H

#include "object.h"
#include "tstring.h"

#include <windows.h>

enum object_type {TYPE_FILE, TYPE_DIRECTORY};

class TFileStruct: public TObject {
public:
	object_type type;
	PString path;

	TFileStruct(char*, object_type);
	~TFileStruct();
};

typedef TFileStruct* PFileStruct;
typedef TFileStruct& RFileStruct;

#endif

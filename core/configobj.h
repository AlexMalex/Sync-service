#if !defined(CONFIGOBJ_H)
#define CONFIGOBJ_H

#include "tstring.h"

#include <windows.h>

class TConfigObject: public TString {
private:
	DWORD id;

public:
	TConfigObject(char* =NULL, DWORD=0);
	virtual ~TConfigObject();

	void set_id(DWORD);
	DWORD get_id();
};

inline void TConfigObject::set_id(DWORD new_id)
{
	id=new_id;
}

inline DWORD TConfigObject::get_id()
{
	return id;
}

typedef TConfigObject* PConfigObject;
typedef TConfigObject& RConfigObject;

#endif

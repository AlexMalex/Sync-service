#if !defined(HANDLE_H)
#define HANDLE_H

#include <windows.h>
#include "object.h"

class THandleObject: public TObject {
protected:
	HANDLE handle;
  
	THandleObject();
  
public:
	virtual ~THandleObject();

	HANDLE getHandle();
	bool isValid();  
};

inline HANDLE THandleObject::getHandle()
{
	return handle;
}

inline bool THandleObject::isValid()
{
	return ((handle!=NULL)&&(handle!=INVALID_HANDLE_VALUE));
}

typedef THandleObject* PHandleObject;
typedef THandleObject& RHandleObject;

#endif

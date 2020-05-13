#if !defined(PROTOBJ_H)
#define PROTOBJ_H

#include "sync.h"
#include <windows.h>

class TProtectedObject: public TCriticalSection {
private:
	bool need_free;
	
public:
	TProtectedObject();
	virtual ~TProtectedObject();

	bool need_destroy();

	void set_destroy_state();
};

inline bool TProtectedObject::need_destroy()
{
	return need_free;
}

inline void TProtectedObject::set_destroy_state()
{
	need_free=true;
}

typedef TProtectedObject* PProtectedObject;
typedef TProtectedObject& RProtectedObject;

#endif

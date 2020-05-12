#if !defined(TIMEOBJ_H)
#define TIMEOBJ_H

#include "qtimer.h"

#include <windows.h>

class TimeObject: public QTimer {
private:
	bool timed_out;

public:
	TimeObject();
	virtual ~TimeObject();

	void init(DWORD);
	void done();

	bool timeout();

	virtual void execute();
};

inline bool TimeObject::timeout()
{
	return timed_out;
}

typedef TimeObject* PTimeObject;
typedef TimeObject& RTimeObject;

#endif
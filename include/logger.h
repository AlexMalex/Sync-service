#if !defined(FILE_H)
#define FILE_H

#include "handle.h"
#include "sync.h"

enum LogType {USE_CONSOLE, USE_FILE};

enum EventType {INFO, WARN, ERR};

enum SubSystem {KERNEL};

class TLogger: public THandleObject, public TCriticalSection {
protected:
	LogType ltype;

public:
	TLogger(LogType type, char* name=NULL);
	virtual ~TLogger();

	void logEvent(char*, EventType, int=0);
};

typedef TLogger* PLogger;
typedef TLogger& RLogger;

#endif

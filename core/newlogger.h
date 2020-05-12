#if !defined(NEWLOGGER_H)
#define NEWLOGGER_H

#include "tstring.h"
#include "logger.h"

class TNewLogger: public TLogger {
private:
	PString file_name;

	SYSTEMTIME start_time;

public:
	TNewLogger(char*);
	virtual ~TNewLogger();

	void restart_daily();
};

typedef TNewLogger* PNewLogger;
typedef TNewLogger& RNewLogger;

#endif

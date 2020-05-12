#if !defined(SVCOBJ_H)
#define SVCOBJ_H

#include "threadlist.h"
#include "servsock.h"
#include "sync.h"
#include "scheduler.h"
#include "filesupd.h"
#include "stat.h"
#include "websrv.h"

class TServiceObject: public TServerSocket, public TThreadList {
private:
	PEvent stopEvent;
	PEvent syncEvent;

	PScheduler scheduler;
	
	PFileUpdate updater;

	PStat stat;

	PWebSrv web;

	bool terminated;

	time_t uptime;

public:
	TServiceObject(int);
	virtual ~TServiceObject();

	void terminate();

	// do client tasks
	void schedule();

	// actual service code
	void work();

	PScheduler get_scheduler();

	time_t get_uptime();
};

inline void TServiceObject::terminate()
{
	terminated=true;
}

inline PScheduler TServiceObject::get_scheduler()
{
	return scheduler;
}

inline time_t TServiceObject::get_uptime()
{
	return uptime;
}

typedef TServiceObject* PServiceObject;

#endif

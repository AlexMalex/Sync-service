#if !defined(SCHEDULER_H)
#define SCHEDULER_H

#include "tasklist.h"
#include "thread.h"
#include "sync.h"

// sleep period
#define SCHEDULE_TICK 200

class TScheduler: public TTaskList, public TThread {
private:
	PEvent syncEvent;

	boolean terminated;

	void work();

	void schedule();

public:
	TScheduler();
	virtual ~TScheduler();

	void terminate();

	virtual DWORD execute();
};

inline void TScheduler::terminate()
{
	terminated=true;
}

typedef TScheduler* PScheduler;
typedef TScheduler& RScheduler;

#endif

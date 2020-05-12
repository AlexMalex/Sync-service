#if !defined(THREADLIST_H)
#define THREADLIST_H

#include "const.h"
#include "clntthread.h"
#include "clienttask.h"

class TThreadList {
private:
	PClientThread threads[MAX_THREADS];

	int thread_count;
	int task_count;

public:
	TThreadList();
	virtual ~TThreadList();

	bool add_thread(PClientThread);

	void scan_threads(bool=false);
};

#endif

#if !defined(CLIENTTTASK_H)
#define CLIENTTTASK_H

#include "clientjob.h"
#include "thread.h"


class TClientTask: public TThread, public TClientJob {
private:
	DWORD job;

public:
	TClientTask(DWORD);
	virtual ~TClientTask();

	virtual DWORD execute();

	DWORD get_task_id();
};

inline DWORD TClientTask::get_task_id()
{
	return job;
}

typedef TClientTask* PClientTask;
typedef TClientTask& RClientTask;

#endif

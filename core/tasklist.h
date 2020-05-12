#if !defined(TASK_LIST)
#define TASK_LIST

#include "const.h"
#include "clienttask.h"

class TTaskList {
private:
	PClientTask tasks[MAX_TASKS];

	int task_count;

public:
	TTaskList();
	virtual ~TTaskList();

	bool add_task(PClientTask);

	void scan_tasks(bool=false);

	bool task_is_running(DWORD);
};

typedef TTaskList* PTaskList;
typedef TTaskList& RTaskList;

#endif

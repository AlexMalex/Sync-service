#include "tasklist.h"

TTaskList::TTaskList(): task_count(0)
{
	for (register int i=0; i<MAX_TASKS; i++) {
		tasks[i]=NULL;
	};
}

TTaskList::~TTaskList()
{
}

bool TTaskList::add_task(PClientTask task)
{
	bool found=false;

	if (task_count==MAX_TASKS) return false;

	for (register int i=0; i<MAX_TASKS; i++) {
		if (tasks[i]==NULL) {
			found=true;

			tasks[i]=task;

			task_count++;

			break;
		};
	};

	return found;
}

void TTaskList::scan_tasks(bool finish)
{
	PClientTask task;

	for (register int i=0; i<MAX_TASKS; i++) {
		task=tasks[i];

		if (task!=NULL) {
			if ((task->threadTerminated())||(finish)) {
				delete task;

				tasks[i]=NULL;

				task_count--;
			};
		};
	};
}

bool TTaskList::task_is_running(DWORD task_id)
{
	PClientTask task;

	if (task_count==0) return false;

	for (register int i=0; i<MAX_TASKS; i++) {
		task=tasks[i];

		if (task!=NULL) {
			if ((task->get_task_id()==task_id)&&(!task->threadTerminated())) {
				return true;
			};
		};
	};

	return false;
}

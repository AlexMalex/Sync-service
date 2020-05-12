#include "const.h"
#include "scheduler.h"
#include "newlogger.h"
#include "errors.h"

extern char* info_str[];

extern PNewLogger log;

extern PDQueue jobs;

extern char* SyncEventName;

TScheduler::TScheduler(): TTaskList(), TThread(true), terminated(false)
{
	syncEvent=new TEvent(SyncEventName, true, true);
}

TScheduler::~TScheduler()
{
	if (!threadTerminated()) {
		terminate();

		if (!waitFor(MAX_WAIT_TIME)) kill();
	};

	if (syncEvent!=NULL) delete syncEvent;
}

DWORD TScheduler::execute()
{
	log->logEvent(info_str[SCHEDULER_START_STR], INFO);

	try {
		work();
	}
	catch (...) {
		log->logEvent(info_str[SCHEDULER_CRASH_STR], ERR, EXCEPTION_ERROR);
	};

	log->logEvent(info_str[SCHEDULER_STOP_STR], INFO);

	return 0;
}

void TScheduler::work()
{
	while (!terminated) {
		// schedule client tasks
		schedule();

		// collect ended tasks
		scan_tasks();

		// save CPU time - sleep 0,2 sec
		Sleep(SCHEDULE_TICK);

		// wait if needed
		syncEvent->waitFor(MAX_WAIT_FOR_UPDATE);
	};

	// end all tasks
	scan_tasks(true);
}

void TScheduler::schedule()
{
	PBlock block;
	PJob job;
	PClientTask task;

	// start jobs, if needed
	block=jobs->head;

	while (block!=NULL) {
		job=(PJob)block->obj;

		if (job!=NULL) {
			job->nex_day_init();

			if (job->is_active()) {
				if (job->job_scheduled()) {
					if (!job->waiting_for_nex_day()) {
						if (!task_is_running(job->get_id())) {
							if (job->ready_to_run()) {
								task=new TClientTask(job->get_id());

								if (task!=NULL) {
									add_task(task);

									job->inc_started();
								};
							};
						}
						else job->check_next_time_to_run(); // job still running - adjust next_time_run if needed
					};
				};
			};
		};

		block=block->next;
	};

	// need logger restart
	log->restart_daily();
}

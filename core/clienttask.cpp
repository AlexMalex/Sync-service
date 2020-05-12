#include "clienttask.h"
#include "newlogger.h"
#include "errors.h"
#include "const.h"

extern PNewLogger log;
extern char* info_str[];

TClientTask::TClientTask(DWORD job_id):TThread(true), TClientJob(), job(job_id)
{
	// start thread after all inits 
	resume();
}

TClientTask::~TClientTask()
{
	if (threadState!=tsEnded) {
		finished=true;

		waitFor(MAX_WAIT_TIME);
	};
}

DWORD TClientTask::execute()
{
	log->logEvent(info_str[CLIENT_TASK_START_STR], INFO, job);

	try {
		work(job);
	}
	catch (...) {
		log->logEvent(info_str[CLIENT_TASK_CRASH_STR], ERR, EXCEPTION_ERROR);
	};

	log->logEvent(info_str[CLIENT_TASK_STOP_STR], INFO, job);

	return 0;
}

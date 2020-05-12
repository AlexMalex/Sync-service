#include "clntthread.h"
#include "newlogger.h"
#include "errors.h"

extern char* info_str[];
extern PNewLogger log;

TClientThread::TClientThread(PServerClientSocket s):TThread(true), TMainClass(s)
{
	// start thread after all inits
	resume();
}

TClientThread::~TClientThread()
{
	if (threadState!=tsEnded) {
		finished=true;

		waitFor(MAX_WAIT_TIME);
	};
}

DWORD TClientThread::execute()
{
	log->logEvent(info_str[CLIENT_THREAD_START_STR], INFO);

	try {
		while (!finished) {
			if (!process_messages()) break;

			Sleep(0);
		};
	}
	catch (...) {
		log->logEvent(info_str[CLIENT_THREAD_CRASH_STR], ERR, EXCEPTION_ERROR);
	};

	log->logEvent(info_str[CLIENT_THREAD_STOP_STR], INFO);

	return 0;
}

#include "svcobj.h"
#include "const.h"
#include "errors.h"
#include "newlogger.h"
#include "service.h"
#include "job.h"
#include "filesupd.h"

extern char* StopEventName;
extern char* SyncEventName;

extern char* info_str[];

extern char* bind_error;

extern PNewLogger log;

extern PService service_ptr;

extern PDQueue jobs;

extern DWORD sync_period;

extern int web_port;

PServiceObject serviceProcess=NULL;

TServiceObject::TServiceObject(int port):TServerSocket(port), TThreadList(), terminated(false), scheduler(NULL), updater(NULL), stat(NULL), web(NULL)
{
	serviceProcess=this;

	stopEvent=new TEvent(StopEventName);

	syncEvent=new TEvent(SyncEventName, true, true);

	//can't bind socket to port - shutdown
	if (!connected) {
		service_ptr->windows_log(bind_error, SOCKET_BIND_ERROR);

		terminated=true;

		return;
	};

	scheduler=new TScheduler();

	updater=new TFileUpdate();

	stat=new TStat();

	web=new TWebSrv(web_port);

	uptime=time(NULL);
}

TServiceObject::~TServiceObject()
{
	if (web!=NULL) delete web;

	if (stat!=NULL) delete stat;

	if (updater!=NULL) delete updater;

	if (scheduler!=NULL) delete scheduler;

	if (syncEvent!=NULL) delete syncEvent;

	if (stopEvent!=NULL) delete stopEvent;
}

void TServiceObject::work()
{
	PServerClientSocket socket=NULL;
	PClientThread thread;

	//got error in initialization
	if (terminated) return;
	
	log->logEvent(info_str[MAIN_THREAD_START_STR], INFO);
	
	try {
		if (scheduler!=NULL) scheduler->resume();

		if (web!=NULL) web->resume();

		if (updater!=NULL) updater->startTimeout(sync_period);
		
		if (stat!=NULL) stat->startTimeout(SHOW_STAT_PERIOD);

		while (!terminated) {
			// we have been signaled
			if (stopEvent->waitFor(0)==WAIT_OBJECT_0) {
				break;
			};

			socket=accept(MAX_WAIT);

			if (socket!=NULL) {
				// got client
				thread=new TClientThread(socket);

				if (thread!=NULL) add_thread(thread);
			};

			// collect ended threads
			scan_threads();

			// save CPU time
			Sleep(0);

			// wait if needed
			syncEvent->waitFor(MAX_WAIT_FOR_UPDATE);
		};
	
		// end all threads
		scan_threads(true);
	}
	catch (...) {
		log->logEvent(info_str[MAIN_THREAD_CRASH_STR], INFO);
	};

	log->logEvent(info_str[MAIN_THREAD_STOP_STR], INFO);
}

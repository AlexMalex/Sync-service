#include "threadlist.h"

TThreadList::TThreadList():thread_count(0), task_count(0)
{
	for (register int i=0; i<MAX_THREADS; i++) {
		threads[i]=NULL;
	};
}

TThreadList::~TThreadList()
{
}

bool TThreadList::add_thread(PClientThread thread)
{
	bool found=false;

	if (thread_count==MAX_THREADS) return false;

	for (register int i=0; i<MAX_THREADS; i++) {
		if (threads[i]==NULL) {
			found=true;

			threads[i]=thread;

			thread_count++;

			break;
		};
	};

	return found;
}

void TThreadList::scan_threads(bool finish)
{
	PClientThread thread;

	for (register int i=0; i<MAX_THREADS; i++) {
		thread=threads[i];

		if (thread!=NULL) {
			if ((thread->threadTerminated())||(finish)) {
				delete thread;

				threads[i]=NULL;

				thread_count--;
			};
		};
	};
}

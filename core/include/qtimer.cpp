#include "qtimer.h"

#pragma warning (disable: 4100) // unused params

void __stdcall timer_callback(void* timer, BOOLEAN TimerOrWaitFired)
{
	((PTimer)timer)->execute();
}

QTimer::QTimer(): queue(NULL), timer(NULL), started(false)
{
	queue=CreateTimerQueue();
}

QTimer::~QTimer()
{
	stopTimeout();

	DeleteTimerQueue(queue);
}

bool QTimer::startTimeout(DWORD timeout, bool periodically)
{
	DWORD period_time=0;

	stopTimeout();

	if (periodically) period_time=timeout;

	started=(CreateTimerQueueTimer(&timer, queue, &timer_callback, (void*)this, timeout, period_time, WT_EXECUTELONGFUNCTION)!=0);

	return started;
}

void QTimer::stopTimeout()
{
	if (started) {
		DeleteTimerQueueTimer(queue, timer, NULL);

		timer=NULL;

		started=false;
	};
}

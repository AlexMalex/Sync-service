#if !defined(QTIMER_H)
#define QTIMER_H

#include <windows.h>

class QTimer {
private:
	HANDLE queue;
	HANDLE timer;

	bool started;

public:
	QTimer();
	virtual ~QTimer();

	bool startTimeout(DWORD, bool=true);
	void stopTimeout();

	virtual void execute()=0;
};

typedef QTimer* PTimer;
typedef QTimer& RTimer;

#endif

#if !defined(STAT_H)
#define STAT_H

#include "qtimer.h"

class TStat: public QTimer {
private:
	bool started;

public:
	TStat();
	virtual ~TStat();

	virtual void execute();

	void rotate();
};

typedef TStat* PStat;
typedef TStat& RStat;

#endif

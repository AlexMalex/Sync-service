#if !defined(FILESUPD_H)
#define FILESUPD_H

#include "sync.h"
#include "qtimer.h"

class TFileUpdate: public QTimer {
private:
	PEvent syncEvent;

	static bool busy;

public:
	TFileUpdate();
	virtual ~TFileUpdate();

	virtual void execute();
};

typedef TFileUpdate* PFileUpdate;
typedef TFileUpdate& RFileUpdate;

#endif

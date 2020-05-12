#if !defined(MAINSERVICE_H)
#define MAINSERVICE_H

#include "svcobj.h"
#include "service.h"

class TMainService: public TService {
private:
	PServiceObject service;

public:
	TMainService(char*, char*, char*);
	virtual ~TMainService();

	virtual void work();
	virtual void on_need_stop();
};

typedef TMainService* PMainService;
typedef TMainService& RMainService;

#endif

#include "mainservice.h"
#include "support.h"

extern int port;

TMainService::TMainService(char* name, char* visible_name, char* description): TService(name, visible_name, description, false)
{
}

TMainService::~TMainService()
{
}

void TMainService::work()
{
	if (init_all()==NO_EXIT) {
		service=new TServiceObject(port);

		if (service==NULL) goto out;

		service->work();

		delete service;
	};

out:
	done_all();
}

void TMainService::on_need_stop()
{
	service->terminate();
}

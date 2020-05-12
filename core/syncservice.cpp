#include "const.h"
#include "mainservice.h"
#include "support.h"

#pragma comment(lib, "psapi.lib ")
#pragma comment(lib, "wsock32.lib ")
#pragma comment(lib, "shlwapi.lib")

extern char* ServiceName;
extern char* ServiceDisplayName;
extern char* ServiceDescription;

#pragma warning (disable: 4100) // unused params

int main(int argc, char* argv[])
{
	PMainService service=new TMainService(ServiceName, ServiceDisplayName, ServiceDescription);

	if (service==NULL) return -1;

#if !defined(_DEBUG)
	service->run();
#else
	service->work();
#endif

	delete service;
}

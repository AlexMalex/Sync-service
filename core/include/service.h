#if !defined(SERVICE_H)
#define SERVICE_H

#include "tstring.h"

#include <windows.h>

#define NOERROR	0

#define UNKNOWN				0
#define INTERNAL_SERVICE_ERR		1       
#define CTRL_DISP_ERR			2
#define REGISTER_SERVICE_ERR		3
#define UNHANDLED_EXCEPTION		4
#define START_EXCEPTION			5
#define STOP_EXCEPTION			6
#define SESSION_EXCEPTION		7

class TService : public TObject {
private:
	PString service_name;
	PString display_name;
	PString description_string;

	bool dependent;

	SERVICE_TABLE_ENTRY DispatchTable[2];
	HANDLE hEventSource;

	DWORD parse_command_line();

	bool parentProcess();

public:
	TService(char*, char* =NULL, char* =NULL, bool=false);
	virtual ~TService();

	// must be overloaded
	virtual void work()=0;

	// may be overloaded
	virtual void on_start();
	virtual void on_stop();

	virtual void on_session_change(DWORD, DWORD);

	virtual void on_need_stop();

	void run();

	void windows_log(char*, int);

	char* get_name();

	// command line support
	bool install();
	bool uninstall();
	bool start();
	bool send_control(DWORD);
};

inline char* TService::get_name()
{
	return service_name->get_string();
}

struct session_param {
	DWORD eventType;
	DWORD sessionId;
};

typedef session_param* psession_param;

typedef TService* PService;
typedef TService& RService;

#endif

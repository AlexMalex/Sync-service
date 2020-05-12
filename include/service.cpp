#include "service.h"

#include <strsafe.h>
#include <stdlib.h>
#include <tlhelp32.h>

#pragma warning(disable: 4800) // converting to bool type
#pragma warning (disable: 4100) // unused params

PService service_ptr=NULL;

char* logStrFormat="%s; windows error - %08lX";

char* commands[4]={"install", "uninstall", "start", "stop"};

char* ctrl_dispetcher="control dispetcher error";
char* ctrl_handler="cant register control handler";
char* unhandled_exception="unhandled exception in working thread";
char* start_exception="exception in start func";
char* stop_exception="exception in stop func";
char* session_exception="exception in session func";

char* command_install="service %s installed\r\n";
char* command_uninstall="service %s uninstalled\r\n";
char* command_start="service %s started\r\n";
char* command_exec="control code %d sended to service %s\r\n";
char* command_error="error code: %d\r\n";

char* netMan="Netman\0";

SERVICE_STATUS_HANDLE statusHandle;
SERVICE_STATUS svcStatus;
DWORD dwCheckPoint=0;

char* parentProcessName="services.exe";

void reportSvcStatus(DWORD CurrentState, DWORD Win32ExitCode=0, DWORD WaitHint=0)
{
	svcStatus.dwServiceType=SERVICE_WIN32_OWN_PROCESS;
	svcStatus.dwCurrentState=CurrentState;
	svcStatus.dwWin32ExitCode=0;
	svcStatus.dwServiceSpecificExitCode=0;
	svcStatus.dwWaitHint=WaitHint;

	if (Win32ExitCode==SERVICE_STOPPED) svcStatus.dwWin32ExitCode=Win32ExitCode;
	
	if ((CurrentState==SERVICE_START_PENDING)||(CurrentState==SERVICE_STOP_PENDING)) {
		svcStatus.dwControlsAccepted=0;

		svcStatus.dwCheckPoint=dwCheckPoint++;
	}
	else {
		svcStatus.dwControlsAccepted=SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_SHUTDOWN|SERVICE_ACCEPT_SESSIONCHANGE;

		svcStatus.dwCheckPoint=0;
	};

	SetServiceStatus(statusHandle, &svcStatus);
}

DWORD WINAPI thread_func(LPVOID lpParameter)
{
	try {
		service_ptr->on_session_change(((psession_param)lpParameter)->eventType, ((psession_param)lpParameter)->sessionId);
	}
	catch (...) {
		service_ptr->windows_log(session_exception, UNKNOWN);
	};

	HeapFree(GetProcessHeap(), 0, lpParameter);

	return 0;
}

DWORD WINAPI control_handler_ex(DWORD dwCtrl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
	psession_param session;

	if ((dwCtrl==SERVICE_CONTROL_STOP)||(dwCtrl==SERVICE_CONTROL_SHUTDOWN)) {
		service_ptr->on_need_stop();

		reportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 3000);

		return NO_ERROR;
	};

	if (dwCtrl==SERVICE_CONTROL_SESSIONCHANGE) {
		session=(session_param*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(session_param));

		session->eventType=dwEventType;
		session->sessionId=((PWTSSESSION_NOTIFICATION)lpEventData)->dwSessionId;

		CreateThread(NULL, 0, thread_func, session, 0, NULL);

		return NO_ERROR;
	};

	if (dwCtrl==SERVICE_CONTROL_INTERROGATE) {
		return NO_ERROR;
	};

	return ERROR_CALL_NOT_IMPLEMENTED;
}

void WINAPI main_body(DWORD args, char** argv)
{
	statusHandle=RegisterServiceCtrlHandlerEx(service_ptr->get_name(), control_handler_ex, NULL);

	if (statusHandle==0) {
		service_ptr->windows_log(ctrl_handler, GetLastError());

		return;
	};

	if (service_ptr==NULL) {
		reportSvcStatus(SERVICE_STOPPED, INTERNAL_SERVICE_ERR);

		return;
	};

	reportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
	
	// init service
	try {
		service_ptr->on_start();
	}
	catch (...) {
		service_ptr->windows_log(start_exception, UNKNOWN);

		reportSvcStatus(SERVICE_STOPPED, REGISTER_SERVICE_ERR);

		return;
	};

	reportSvcStatus(SERVICE_RUNNING, NO_ERROR);

	// service working
	try {
		service_ptr->work();
	}
	catch (...) {
		service_ptr->windows_log(unhandled_exception, UNKNOWN);
	};

	// done service
	try {
		service_ptr->on_stop();
	}
	catch (...) {
		service_ptr->windows_log(stop_exception, UNKNOWN);
	};

	reportSvcStatus(SERVICE_STOPPED, NO_ERROR);
}

TService::TService(char* name, char* visible_name, char* description, bool depend_on):TObject(), service_name(NULL), display_name(NULL), description_string(NULL), dependent(depend_on)
{
	if (name!=NULL) service_name=new TString(name);

	if (visible_name!=NULL) display_name=new TString(visible_name);

	if (description!=NULL) description_string=new TString(description);

	hEventSource=RegisterEventSource(NULL, service_name->get_string());

	DispatchTable[0].lpServiceName=service_name->get_string();
	DispatchTable[0].lpServiceProc=main_body;

	DispatchTable[1].lpServiceName=NULL;
	DispatchTable[1].lpServiceProc=NULL;

	// avoid double registration
	if (service_ptr==NULL) service_ptr=this;
}

TService::~TService()
{
	if (hEventSource!=NULL) DeregisterEventSource(hEventSource);

	if (description_string!=NULL) delete description_string;

	if (display_name!=NULL) delete display_name;

	if (service_name!=NULL) delete service_name;
}

void TService::on_start()
{
}

void TService::on_stop()
{
}

void TService::on_session_change(DWORD eventType, DWORD sessionId)
{
}

void TService::on_need_stop()
{
}

void TService::run()
{
	// check command line
	if (!(parse_command_line()==NOERROR)) return;

	// check if started from services.exe
	if (!parentProcess()) return; 

	// start service
	if (!StartServiceCtrlDispatcher(DispatchTable)) {
		windows_log(ctrl_dispetcher, GetLastError());
	};
}

void TService::windows_log(char* what, int code)
{
	char buffer[1024];
	LPCSTR lpszStrings[1];

	if (hEventSource!=NULL) {
		if (SUCCEEDED(StringCchPrintf(buffer, sizeof(buffer), logStrFormat, what, code))) {
			lpszStrings[0]=buffer;

			ReportEvent(hEventSource, EVENTLOG_ERROR_TYPE, 0, code, NULL, 1, 0, lpszStrings, NULL);
		};
	};
}

DWORD TService::parse_command_line()
{
	if (__argc>1) {
		for (register int i=1; i<__argc; i++) {
			if (!(( __argv[i][0]=='/') || ( __argv[i][0]=='-'))) continue;

			for (register int j=0; commands[j]!='\x0'; j++) {
				if (StrCmpI(__argv[i]+1, commands[j])==0) {
					switch (j) {
						case 0: {
							install();

							return 1; 
						};

						case 1: {
							uninstall();

							return 1;
						};

						case 2: {
							start();

							return 1;
						};

						case 3: {
							send_control(SERVICE_CONTROL_STOP);

							return 1;
						};
					};
				};
			};
		};
	};

	return NO_ERROR;
}

bool TService::install()
{
    SC_HANDLE schSCManager;
    SC_HANDLE schService;
    char szPath[MAX_PATH];
	SERVICE_DESCRIPTIONA scDescr;
	char* depend_on=NULL;

	if(!GetModuleFileNameA(NULL, szPath, MAX_PATH )) goto error;

    schSCManager=OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
 
    if (schSCManager==NULL) goto error;

	if (dependent) depend_on=netMan;

	schService=CreateService(schSCManager, service_name->get_string(), display_name->get_string(),	SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, szPath, NULL, NULL, depend_on, NULL, NULL);
 
    if (schService==NULL) {
		CloseServiceHandle(schSCManager);

error:
		printf(command_error, GetLastError());

		return false;
    }; 

	scDescr.lpDescription=description_string->get_string();

	ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &scDescr);

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);

	printf(command_install, service_name->get_string());

	return true;
}

bool TService::uninstall()
{
	SC_HANDLE schSCManager;
    SC_HANDLE schService;

	schSCManager=OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (schSCManager==NULL) goto error;

	schService=OpenService(schSCManager, service_name->get_string(), SERVICE_ALL_ACCESS);

    if (schService==NULL) {
		CloseServiceHandle(schSCManager);
error:
		return false;
    }; 

	DeleteService(schService);

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);

	printf(command_uninstall, service_name->get_string());

	return true;
}

bool TService::start()
{
	SC_HANDLE schSCManager;
    SC_HANDLE schService;
	BOOL result;

	schSCManager=OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (schSCManager==NULL) goto error;

	schService=OpenService(schSCManager, service_name->get_string(), SERVICE_ALL_ACCESS);

    if (schService==NULL) {
		CloseServiceHandle(schSCManager);
error:
		printf(command_error, GetLastError());

		return false;
    }; 

	result=StartService(schService, NULL, NULL);

	if (result!=0) printf(command_start, service_name->get_string());
	else printf(command_error, GetLastError());

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);

	return bool(result);
}

bool TService::send_control(DWORD code)
{
	SC_HANDLE schSCManager;
    SC_HANDLE schService;
	SERVICE_STATUS scStatus;
	BOOL result;

	schSCManager=OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (schSCManager==NULL) goto error;

	schService=OpenServiceA(schSCManager, service_name->get_string(), SERVICE_ALL_ACCESS);

    if (schService==NULL) {
		CloseServiceHandle(schSCManager);
error:
		return false;
    }; 

	result=ControlService(schService, code, &scStatus);

	if (result!=0) printf(command_exec, code, service_name->get_string());
	else printf(command_error, GetLastError());

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);

	return bool(result);
}

bool TService::parentProcess()
{
	DWORD currentPId=GetCurrentProcessId(), parentPid=0;
	HANDLE snapshot;
	PROCESSENTRY32 process;
	bool result=false, flag;
	
	process.dwSize=sizeof(process);

	snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	flag=Process32First(snapshot, &process);

	while (flag) {
			if (process.th32ProcessID==currentPId) {
			parentPid=process.th32ParentProcessID;

			break;
		};

		flag=Process32Next(snapshot, &process);
	};

	if (parentPid!=0) {
		CloseHandle(snapshot);

		snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

		flag=Process32First(snapshot, &process);

		while (flag) {
			if (process.th32ProcessID==parentPid) {
				if (StrCmpI(process.szExeFile, parentProcessName)==0) result=true;

				break;
			};

			flag=Process32Next(snapshot, &process);
		};
	};

	CloseHandle(snapshot);

	return result;
}

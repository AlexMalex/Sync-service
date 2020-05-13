#include "logger.h"

#include <strsafe.h>

// fromat strings for logger
char* dbg_fmt_str="%02d.%02d.%04d %02d:%02d:%02d %s %04lX %s\n";
char* dbg_fmt_str_error="%02d.%02d.%04d %02d:%02d:%02d %s %04lX %s %d\n";

// type of logging
char* warn_str[3]={"I", "W", "E"};

TLogger::TLogger(LogType type, char* name): TCriticalSection(), ltype(type)
{
	switch (type) {
		case USE_FILE: {
			handle=CreateFileA(name, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			SetFilePointer(handle, 0, NULL, FILE_END);

			break;
		};

		case USE_CONSOLE: {
			AllocConsole();

			handle=GetStdHandle(STD_OUTPUT_HANDLE);

			break;
		};
	};
}

TLogger::~TLogger()
{
	if ((ltype==USE_FILE)&&(isValid())) FlushFileBuffers(handle);
}

void TLogger::logEvent(char* str, EventType type, int error)
{
	SYSTEMTIME time;
	char buffer[1024];
	DWORD bytes, written;
	char* temp_str;

	if (!isValid()) return;

	enter();

	try {
		GetLocalTime(&time);

		if (error==0) temp_str=dbg_fmt_str;
		else temp_str=dbg_fmt_str_error;

		if SUCCEEDED(StringCbPrintf(buffer, sizeof(buffer), temp_str, time.wDay, time.wMonth, time.wYear, time.wHour, time.wMinute, time.wSecond, warn_str[type], GetCurrentThreadId(), str, error)) {
			if SUCCEEDED(StringCbLength(buffer, sizeof(buffer), (size_t*)&bytes)) {
				switch (ltype) {
					case USE_FILE: {
						WriteFile(handle, buffer, bytes, &written, NULL);

						break;
					};
					case USE_CONSOLE: {
						WriteConsoleA(handle, buffer, bytes, &written, NULL);

						break;
					};					  
				};
			};
		};
	}
	catch(...) {
	};

	leave();
}

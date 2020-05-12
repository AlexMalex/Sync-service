#include "support.h"
#include "stat.h"
#include "const.h"
#include "point.h"
#include "dqueue.h"
#include "file.h"

#include "newlogger.h"

#include <strsafe.h>

#pragma warning (disable: 4244) // doble -> DWORD

extern PDQueue points;

extern PNewLogger log;

extern char* dir_log;

extern char* info_str[];

extern char* stat_name;

extern DWORD self_id;

extern DWORD days_keep_logs;

char* point_str="point id: %d, name: %s";
char* sessions_str="sessions: %d";
char* files_str="files: %d";

extern char* amount[];
extern char* speed[];

char* last_access_str="last access: %02d.%02d.%04d %02d:%02d:%02d";
char* inactive_time_str="inactive time: %d days %d hours %d minutes";

char* read="read";
char* write="write";

char* log_file_deleted="log file: %s - deleted";

char* wildcard_log="*.log";

TStat::TStat(): started(false)
{
}

TStat::~TStat()
{
	rotate();
}

void TStat::execute() 
{
	rotate();
}

void TStat::rotate()
{
	PBlock block;
	TString file_name;
	char buffer[1024];
	DQueue logs("logs", false);
	DWORD result, recurse_level=0;
	PString cur_file;
	TFile file;
	FILETIME file_time;
	SYSTEMTIME time;
	ULARGE_INTEGER large_int, file_int;
	double temp;
	
	if (started) return;

	started=true;

	// clean logs
	GetLocalTime(&time);

	// do it only in 00:xx
	if (time.wHour!=0) goto exit;

	log->logEvent(info_str[CLEAN_LOGS_STARTED_STR], INFO);

	result=file_search(dir_log, NULL, wildcard_log, &logs, false, recurse_level);

	if (result>0) goto exit;

	GetSystemTime(&time); // in UTC

	SystemTimeToFileTime(&time, &file_time);

	large_int.HighPart=file_time.dwHighDateTime;
	large_int.LowPart=file_time.dwLowDateTime;

	temp=24*60*60*1000*days_keep_logs;

	temp=temp*1000*10;

	large_int.QuadPart=large_int.QuadPart-temp;

	block=logs.head;

	while (block!=NULL) {
		cur_file=(PString)block->obj;

		if (cur_file==NULL) goto jam;

		file_name.set_string(dir_log);

		file_name.add_string(cur_file->get_string());

		file.open(file_name.get_string());

		if (!file.isValid()) goto jam;

		if (!file.get_time(&file_time)) goto jam;

		file_int.HighPart=file_time.dwHighDateTime;
		file_int.LowPart=file_time.dwLowDateTime;

		if (file_int.HighPart>large_int.HighPart) goto jam;

		if ((file_int.HighPart==large_int.HighPart)&&(file_int.LowPart>large_int.LowPart)) goto jam;

		file.close();

		if (DeleteFile(file_name.get_string())!=FALSE) {
			StringCbPrintf(buffer, sizeof(buffer), log_file_deleted, cur_file->get_string());

			log->logEvent(buffer, INFO);
		};
jam:
		block=block->next;
	};

	log->logEvent(info_str[CLEAN_LOGS_FINISHED_STR], INFO);

exit:
	started=false;
}

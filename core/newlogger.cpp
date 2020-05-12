#include "const.h"
#include "support.h"
#include "newlogger.h"
#include "point.h"

#include <strsafe.h>

extern PDQueue points;

char* format_str="%02d%02d%04d";
char* ext_str=".log";

TNewLogger::TNewLogger(char* name):TLogger(LogType::USE_FILE, name)
{
	file_name=new TString(name);

	GetLocalTime(&start_time);
}

TNewLogger::~TNewLogger()
{
	if (file_name!=NULL) delete file_name;
}

void TNewLogger::restart_daily()
{
	char buffer[MAX_PATH_LENGTH], date_str[10];
	char* pos, *current=file_name->get_string();
	UINT i=0;
	SYSTEMTIME current_time;
	PBlock block;
	PPoint point;

	GetLocalTime(&current_time);

	//check for day change and in midnight
	if ((current_time.wDay!=start_time.wDay)&&(current_time.wHour==0)) {
		pos=find_filename(current);

		if (pos==current) return;

		while (pos!=(current+i)) {
			buffer[i]=current[i];

			i++;
		};

		buffer[i]='\x0';

		if (!SUCCEEDED(StringCbPrintf(date_str, sizeof(date_str), format_str, start_time.wDay, start_time.wMonth, start_time.wYear))) return;

		if (!SUCCEEDED(StringCchCat(buffer, sizeof(buffer), date_str))) return;

		if (!SUCCEEDED(StringCchCat(buffer, sizeof(buffer), ext_str))) return;

		enter();

		try {
			CloseHandle(getHandle());

			MoveFile(file_name->get_string(), buffer);
		
			handle=CreateFileA(file_name->get_string(), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		}
		catch (...) {
		};

		leave();

		start_time.wDay=current_time.wDay;
		start_time.wMonth=current_time.wMonth;
		start_time.wYear=current_time.wYear;

		//clear stat
		block=points->head;

		while (block!=NULL) {
			point=(PPoint)block->obj;

			if (point==NULL) goto next;

			point->clear_stats();
		
		next:
			block=block->next;
		};
	};

}

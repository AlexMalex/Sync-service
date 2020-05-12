#include "job.h"

#include <strsafe.h>
#include "aobject.h"

#pragma warning (disable: 4482) // use enum in full name

char* str_week_names[7]={"mon", "tue", "wen", "thu", "fri", "sat", "sun"};
char* str_month_names[12]={"jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec"};

WORD month_d[12]={31, 28, 31, 30, 31, 30, 31, 31, 30 , 31, 30, 31};

TJob::TJob():TConfigObject(NULL, 0), active(false), to_do(action::none), after_action(postprocess::no_action), destination(NULL), scheduled(false), started(0), transaction(false)
{
	source=new DQueue(NULL);

	objects=new DQueue(NULL);

	init_schedule();
}

TJob::~TJob()
{
	if (objects!=NULL) delete objects;

	if (source!=NULL) delete source;
}

void TJob::init_daily()
{
	work.type=schedule_type::daily;

	scheduled=is_scheduled();
}

void TJob::init_weekly(week_names new_week)
{
	if ((new_week==NULL)) return;

	if (!has_values(new_week, sizeof(week_names))) return;

	work.type=schedule_type::weekly;

	for (register int i=0; i<sizeof(week_names); i++) {
		work.variant.weekly.week[i]=new_week[i];
	};

	scheduled=is_scheduled();
}

void TJob::init_month_daily(month_days new_days, month_names new_monthes)
{
	if ((new_days==NULL)||(new_monthes==NULL)) return;

	if (!has_values(new_days, sizeof(month_days))) return;

	if (!has_values(new_monthes, sizeof(month_names))) return;

	work.type=schedule_type::monthly;
	work.variant.monthly.month_type=schedule_month::by_day;

	for (register int i=0; i<sizeof(month_names); i++) {
		work.variant.monthly.month[i]=new_monthes[i];
	};

	for (register int i=0; i<sizeof(month_days); i++) {
		work.variant.monthly.month_variant.by_days.days[i]=new_days[i];
	};

	scheduled=is_scheduled();
}

void TJob::init_month_weekly(week_names new_weeks, weeks_month new_week_month, month_names new_monthes)
{
	if ((new_weeks==NULL)||(new_monthes==NULL)||(new_week_month==NULL)) return;

	if (!has_values(new_weeks, sizeof(week_names))) return;

	if (!has_values(new_week_month, sizeof(weeks_month))) return;

	if (!has_values(new_monthes, sizeof(month_names))) return;

	work.type=schedule_type::monthly;
	work.variant.monthly.month_type=schedule_month::by_week;

	for (register int i=0; i<sizeof(month_names); i++) {
		work.variant.monthly.month[i]=new_monthes[i];
	};

	for (register int i=0; i<sizeof(week_names); i++) {
		work.variant.monthly.month_variant.by_weeks.week[i]=new_weeks[i];
	};

	for (register int i=0; i<sizeof(weeks_month); i++) {
		work.variant.monthly.month_variant.by_weeks.weeks[i]=new_week_month[i];
	};

	scheduled=is_scheduled();
}

void TJob::week_from_str(week_names week, char* str)
{
	FillMemory(week, sizeof(week_names), '\x0');
	
	for (register int i=0; i<sizeof(week_names); i++) {
		if (StrStrI(str, str_week_names[i])!=NULL) {
			week[i]=1;
		};
	};
}

void TJob::month_from_str(month_names month, char* str)
{
	FillMemory(month, sizeof(month_names), '\x0');

	for (register int i=0; i<sizeof(month_names); i++) {
		if (StrStrI(str, str_month_names[i])!=NULL) {
			month[i]=1;
		};
	};
}

void TJob::days_from_str(byte* days, char* str, int array_size)
{
	size_t len, pos, start;
	int value;

	char buffer[12];

	FillMemory(days, sizeof(byte)*array_size, '\x0');

	if (!SUCCEEDED(StringCchLength(str, STRSAFE_MAX_CCH, &len))) return;

	pos=0;
	start=0;

	while (pos<len) {
		if (str[pos]==',') {
			if (((pos-start)>0)&&(pos-start<12)) {
				for (register unsigned int i=start; i<pos; i++) {
					buffer[i-start]=str[i];
				};

				buffer[pos-start]='\x0';

				StrTrim(buffer, "\x20\x09");

				value=StrToInt(buffer);

				if ((value>0)&&(value<=array_size)) {
					days[value-1]=1;
				};
			};

			start=pos+1;
		};

		pos++;
	};

	if (((pos-start)>0)&&((pos-start)<12)) {
		for (register unsigned int i=start; i<pos; i++) {
			buffer[i-start]=str[i];
		};

		buffer[pos-start]='\x0';

		StrTrim(buffer, "\x20\x09");

		value=StrToInt(buffer);

		if ((value>0)&&(value<=array_size)) {
			days[value-1]=1;
		};
	};
}

void TJob::time_from_str(WORD& hour, WORD& min, char* str)
{
	size_t len;
	char* buffer, *am_pos;

	int pos;

	hour=0;
	min=0;

	if (str==NULL) return;

	if SUCCEEDED(StringCchLength(str, STRSAFE_MAX_CCH, &len)) {
		buffer=new char[len+1];

		if SUCCEEDED(StringCchCopy(buffer, len+1, str)) {
			am_pos=StrStrI(buffer, "am");

			if (am_pos!=NULL) *am_pos='\x0';
			else {
				am_pos=StrStrI(buffer, "pm");

				if (am_pos!=NULL) {
					*am_pos='\x0';

					hour=12;
				};
			};

			pos=0;

			for (register unsigned int str_pos=0; str_pos<len; str_pos++) {
				if (buffer[str_pos]==':') {
					pos=str_pos;

					break;
				};
			};

			if (pos!=0) {
				buffer[pos]='\x0';

				StrTrim(buffer, "\x20\x09");

				hour=hour+(WORD)StrToInt(buffer);

				if (hour>23) hour=0;

				StrTrim(&buffer[pos+1], "\x20\x09");

				min=(WORD)StrToInt(&buffer[pos+1]);

				if (min>59) min=0;
			};
		};

		delete buffer;
	};
}

DWORD TJob::period_from_str(char* str)
{
	char* buffer, *h_pos;

	size_t len, multiplier=1;

	DWORD value=0;

	if (str==NULL) return value;

	if SUCCEEDED(StringCchLength(str, STRSAFE_MAX_CCH, &len)) {
		buffer=new char[len+1];

		if SUCCEEDED(StringCchCopy(buffer, len+1, str)) {
			h_pos=StrChrI(buffer, 'h');

			if (h_pos!=NULL) {
				*h_pos='\x0';

				multiplier=60;

				goto out;
			};

			h_pos=StrChrI(buffer, 'm');

			if (h_pos!=NULL) {
				*h_pos='\x0';
			};
out:	
			value=StrToInt(buffer);

			value=value*multiplier;
		};

		delete buffer;
	};

	return value;
}

bool TJob::has_values(byte* dim, int size)
{
	for (register int i=0; i<size; i++) {
		if (dim[i]!=0) 
			return true;
	};

	return false;
}

bool TJob::is_scheduled()
{
	if (work.type==schedule_type::manual) return false;

	switch (work.type) {
		case (schedule_type::daily) : {
			break;
		};

		case (schedule_type::weekly) : {
			return has_values((byte*)&work.variant.weekly, sizeof(week_names));
		};

		case (schedule_type::monthly) : {
			if (!has_values((byte*)&work.variant.monthly.month, sizeof(month_names))) return false;

			if (work.variant.monthly.month_type==schedule_month::by_day) {
				if (!has_values((byte*)&work.variant.monthly.month_variant.by_days, sizeof(month_days))) return false;
			};

			if (work.variant.monthly.month_type==schedule_month::by_week) {
				if (!has_values((byte*)&work.variant.monthly.month_variant.by_weeks.weeks, sizeof(weeks_month))) return false;

				if (!has_values((byte*)&work.variant.monthly.month_variant.by_weeks.week, sizeof(week_names))) return false;
			};

			break;
		};
	};

	return true;
}

#pragma warning (disable: 4244) // DWORD -> WORD transformation
bool TJob::ready_to_run()
{
	SYSTEMTIME time;

	// skip system jobs
	if (get_id()<100) return false;

	GetLocalTime(&time);

	// first - test current day
	switch (work.type) {
		case schedule_type::daily: {
			break;
		};

		case schedule_type::weekly: {
			if ((work.variant.weekly.week[time.wDayOfWeek])==0) return false;

			break;
		};

		case schedule_type::monthly: {
			if ((work.variant.monthly.month[time.wDay])==0) return false;

			if (work.variant.monthly.month_type==schedule_month::by_day) {
				if ((work.variant.monthly.month_variant.by_days.days[time.wDay])==0) return false;
			};

			if (work.variant.monthly.month_type==schedule_month::by_week) {
				if ((work.variant.monthly.month_variant.by_weeks.weeks[(time.wDay/7)+1])==0) return false;

				if ((work.variant.monthly.month_variant.by_weeks.week[time.wDayOfWeek])==0) return false;
			};

			break;
		};
	};

	// first - test start time
	if (work.start_time.hour>time.wHour) return false;

	if ((work.start_time.hour==time.wHour)&&(work.start_time.minute>time.wMinute)) return false;

	// second - test end time - period ended ?
	if (work.end_time.hour<time.wHour) return false;

	if ((work.end_time.hour==time.wHour)&&(work.end_time.minute<time.wMinute)) return false;

	// check period for start
	if (work.next_time.hour>time.wHour) return false;

	if ((work.next_time.hour==time.wHour)&&(work.next_time.minute>time.wMinute)) return false;

	// set next time start
	set_next_time_run(time.wHour, time.wMinute);

	// period==0, work once a day
	if (work.period==0) work.wait_for_next_day=true;

	return true;
};

void TJob::init_schedule()
{
	SYSTEMTIME time;

	GetLocalTime(&time);

	// init schedule
	FillMemory(&work, sizeof(schedule), '\x0');

	// not scheduled
	work.type=schedule_type::manual;

	work.start_time.hour=0;
	work.start_time.minute=0;

	work.end_time.hour=23;
	work.end_time.minute=59;

	work.period=0;

	work.wait_for_next_day=false;
}

void TJob::set_duration(DWORD new_duration)
{
	WORD hour, minute;
	SYSTEMTIME time;

	GetLocalTime(&time);

	work.duration=new_duration;

	hour=new_duration/60;
	minute=new_duration-(hour*60);

	work.end_time.hour=work.start_time.hour+hour;

	if (work.end_time.hour>23) {
max_time:
		work.end_time.hour=23;
		work.end_time.minute=59;

		goto out;
	};

	work.end_time.minute=work.start_time.minute+minute;

	if (work.end_time.minute>59) {
		work.end_time.minute=work.end_time.minute-60;

		work.end_time.hour++;

		if (work.end_time.hour>23) goto max_time;
	};

out:
	set_next_time_run(time.wHour, time.wMinute);
}

void TJob::set_time(WORD new_hour, WORD new_minute)
{
	SYSTEMTIME time;

	GetLocalTime(&time);

	work.start_time.hour=new_hour;
	work.start_time.minute=new_minute;

	set_next_time_run(new_hour, new_minute);

	scheduled=is_scheduled();
}

void TJob::set_period(DWORD new_period)
{
	SYSTEMTIME time;

	GetLocalTime(&time);

	work.period=new_period;

	set_next_time_run(time.wHour, time.wMinute);
}

void TJob::set_next_time_run(WORD hour, WORD minute)
{
	WORD hour_in_period, minute_in_period;

	// set next time start
	if (work.period==0) return;

	hour_in_period=work.period/60;
	minute_in_period=work.period-(hour_in_period*60);

	work.next_time.hour=hour+hour_in_period;

	if (work.next_time.hour>23) goto set_start_time_day;

	work.next_time.minute=minute+minute_in_period;

	if (work.next_time.minute>59) {
		work.next_time.minute=work.next_time.minute-60;

		work.next_time.hour++;

		if (work.next_time.hour>23) goto set_start_time_day;
	};

	if (work.next_time.hour>work.end_time.hour) {
set_start_time_day:
		work.wait_for_next_day=true;

		work.next_time.hour=work.start_time.hour;
		work.next_time.minute=work.start_time.minute;

		return;
	};

	if ((work.next_time.hour==work.end_time.hour)&&(work.next_time.minute>work.end_time.minute)) goto set_start_time_day;
}

void TJob::check_next_time_to_run()
{
	SYSTEMTIME time;

	if (work.period==0) return;

	GetLocalTime(&time);
	
	if ((time.wHour==work.next_time.hour)&&(time.wMinute==work.next_time.minute)) set_next_time_run(time.wHour, time.wMinute);

	return;
}

void TJob::nex_day_init()
{
	SYSTEMTIME time;

	GetLocalTime(&time);

	// once a day at midnight - init schedule
	if ((time.wHour==0)&&(time.wMinute==0)&&(work.wait_for_next_day)) {
		work.wait_for_next_day=false;

		work.next_time.hour=work.start_time.hour;
		work.next_time.minute=work.start_time.minute;
	};
}

bool TJob::waiting_for_nex_day()
{
	return work.wait_for_next_day;
}

void TJob::init_wait_changes()
{
	PBlock block;
	PAObject object;

	if ((to_do==action::write)&&(after_action==postprocess::delete_files)) {
		block=(PBlock)objects->head;

		while (block!=NULL) {
			object=(PAObject)block->obj;
	
			if (object!=NULL) object->init_changes();

			block=block->next;
		};
	};
}

void TJob::done_wait_changes()
{
	PBlock block;
	PAObject object;

	if ((to_do==action::write)&&(after_action==postprocess::delete_files)) {
		block=(PBlock)objects->head;

		while (block!=NULL) {
			object=(PAObject)block->obj;
	
			if (object!=NULL) object->done_changes();

			block=block->next;
		};
	};
}

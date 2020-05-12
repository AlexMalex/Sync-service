#if !defined(JOB_H)
#define JOB_H

#include "configobj.h"
#include "dqueue.h"
#include "point.h"

#pragma warning (disable: 4482) // use enum in full name

enum schedule_type {manual, daily, weekly, monthly};

enum schedule_month {by_day, by_week};

#pragma pack(push) 

#pragma pack(1)

struct date {
	WORD day;
	WORD month;
	WORD dayofweek;
};

struct work_date {
	WORD day;
	WORD month;
	WORD year;
};

struct ttime {
	WORD hour;
	WORD minute;
};

typedef byte week_names[7];
typedef byte month_names[12];
typedef byte month_days[31];
typedef byte weeks_month[5];

struct schedule {
	schedule_type type;
	ttime start_time;
	ttime end_time;
	ttime next_time;
	bool wait_for_next_day;
//	work_date work_day;
	DWORD period;			   // in minutes
	DWORD duration;            // in minutes
	union variant_s {
		struct daily_s {
		} daily;
		struct weekly_s {
			week_names week;
		} weekly;
		struct monthly_s {
			schedule_month month_type;
			month_names month;
			union month_variant_s{
				struct by_days_s {
					month_days days;
				} by_days;
				struct by_weeks_s {
					weeks_month weeks;
					week_names week;
				} by_weeks;
			} month_variant;
		} monthly;
	} variant;
};

#pragma pack(pop)

enum action {none, read, write};

enum postprocess {no_action, delete_files, sync_files};

class TJob: public TConfigObject {
private:
	bool active;

	bool transaction;

	schedule work;

	PDQueue objects;
	
	PDQueue source;

	PPoint destination;

	action  to_do;

	postprocess after_action;

	bool is_scheduled();

	bool scheduled;

	DWORD started;

	void set_next_time_run(WORD, WORD);

//	void next_day();

public:
	TJob();
	virtual ~TJob();

	bool job_scheduled();

	bool is_active();
	void set_active(bool);

	void inc_started();

	PDQueue get_source();

	PPoint get_destination();
	void set_destination(PPoint);

	PDQueue get_objects();

	action get_action();
	void set_action(action);
	
	postprocess get_postprocess();
	void set_postprocess(postprocess);

	void init_schedule();
	void set_time(WORD, WORD);

	void init_daily();
	void init_weekly(week_names);
	void init_month_daily(month_days, month_names);
	void init_month_weekly(week_names, weeks_month, month_names);

	bool ready_to_run();
	void check_next_time_to_run();

	DWORD get_period();
	void set_period(DWORD);

	DWORD get_duration();
	void set_duration(DWORD);

	void nex_day_init();
	bool waiting_for_nex_day();

	bool get_transaction();
	void set_transaction(bool);

	void init_wait_changes();
	void done_wait_changes();

	static void week_from_str(week_names, char*);
	static void month_from_str(month_names, char*);

	static void days_from_str(byte*, char*, int);

	static bool has_values(byte*, int);

	static void time_from_str(WORD&, WORD&, char*);

	static DWORD period_from_str(char*);
};

inline bool TJob::is_active()
{
	return active;
}

inline void TJob::set_active(bool is_active)
{
	active=is_active;
}

inline PDQueue TJob::get_source()
{
	return source;
}

inline PDQueue TJob::get_objects()
{
	return objects;
}

inline DWORD TJob::get_period()
{
	return work.period;
}

inline DWORD TJob::get_duration()
{
	return work.duration;
}

inline action TJob::get_action()
{
	return to_do;
}

inline void TJob::set_action(action set_action)
{
	to_do=set_action;
}

inline postprocess TJob::get_postprocess()
{
	return after_action;
}

inline void TJob::set_postprocess(postprocess set_postprocess)
{
	after_action=set_postprocess;
}
inline PPoint TJob::get_destination()
{
	return destination;
}

inline void TJob::set_destination(PPoint point)
{
	destination=point;
}

inline void TJob::inc_started()
{
	started++;
}

inline bool TJob::job_scheduled()
{
	return scheduled;
}

inline bool TJob::get_transaction()
{
	return transaction;
}

inline void TJob::set_transaction(bool in_transaction)
{
	transaction=in_transaction;
}

typedef TJob* PJob;
typedef TJob& RJob;

#endif

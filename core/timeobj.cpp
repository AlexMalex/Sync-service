#include "timeobj.h"

TimeObject::TimeObject(): timed_out(false)
{
}

TimeObject::~TimeObject()
{
}

void TimeObject::init(DWORD period)
{
	timed_out=false;

	startTimeout(period*1000, false);
}

void TimeObject::done()
{
	stopTimeout();
}

void TimeObject::execute()
{
	timed_out=true;
}

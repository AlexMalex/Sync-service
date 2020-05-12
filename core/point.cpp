#include "point.h"

TPoint::TPoint():TConfigObject(NULL, 0), sessions(0), files(0), bytes_readed(0), bytes_writed(0), time_read(0), time_write(0)
{
	ip=new TString(NULL);

	altip=new TString(NULL);

	FillMemory(&last_access, sizeof(time_t), '\x0');
}

TPoint::~TPoint()
{
	if (altip!=NULL) delete altip;

	if (ip!=NULL) delete ip;
}

void TPoint::set_ip(char* ip_text)
{
	if (ip!=NULL) ip->set_string(ip_text);
}

char* TPoint::get_ip()
{
	if (ip!=NULL) return ip->get_string();
	else return NULL;
}

void TPoint::set_altip(char* ip_text)
{
	if (altip!=NULL) altip->set_string(ip_text);
}

char* TPoint::get_altip()
{
	if (altip!=NULL) return altip->get_string();
	else return NULL;
}

void TPoint::clear_stats()
{
	enter();

	sessions=0;
	files=0;
	bytes_readed=0;
	bytes_writed=0;
	time_read=0;
	time_write=0;

	leave();
}

void TPoint::update_write_stat(__int64 in_bytes_writed, DWORD in_times)
{
	// make stats
	enter();

	try {
		files++;

		bytes_writed=bytes_writed+in_bytes_writed;

		if (in_times>0)
			time_write=time_write+((double)in_times/1000);
		else 
			time_write=time_write+0.001;

	}
	catch (...) {
	};

	leave();
}

void TPoint::update_read_stat(__int64 in_bytes_readed, DWORD in_times)
{
	enter();

	try {
		files++;

		bytes_readed=bytes_readed+in_bytes_readed;

		if (in_times>0) 
			time_read=time_read+((double)in_times/1000);
		else 
			time_read=time_read+0.001;
	}
	catch (...) {
	};

	leave();
}

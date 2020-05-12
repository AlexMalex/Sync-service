#if !defined(POINT_H)
#define POINT_H

#include <windows.h>
#include <time.h>

#include "configobj.h"
#include "sync.h"

class TPoint: public TConfigObject, public TCriticalSection {
private:
	PString ip;
	PString altip;

public:
	TPoint();
	virtual ~TPoint();

	// stat vars
	DWORD sessions;
	DWORD files;

	__int64 bytes_readed;
	__int64 bytes_writed;

	double time_read;
	double time_write;
	time_t last_access;

	void set_ip(char*);
	char* get_ip();

	void set_altip(char*);
	char* get_altip();

	void clear_stats();

	void update_write_stat(__int64, DWORD);
	void update_read_stat(__int64, DWORD);
};

typedef TPoint* PPoint;
typedef TPoint& RPoint;

#endif

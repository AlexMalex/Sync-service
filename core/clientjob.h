#if !defined(CLIENT_JOB)
#define CLIENT_JOB

#include "clntsock.h"
#include "job.h"
#include "dqueue.h"
#include "aobject.h"
//#include "newlogger.h"

class TClientJob {
private:
	DWORD sessionId;

	DWORD client_block_size;

	PClientSocket socket;

	PJob job;
	PPoint point;

	DWORD recurse_level;

	char* buffer;

	bool do_get_file();
	bool do_put_file();

	bool do_open();
	bool do_close();

	bool connect();

	PDQueue local_macros;

	void add_local_object(PAObject);
	void remove_local_object();

	void init();

	void print_info(DWORD, char*, char* =NULL);
	void print_summary(DWORD, __int64, DWORD);
	void print_attempt(DWORD job, UINT id);

	char* get_point_id();

public:
	TClientJob();
	virtual ~TClientJob();

	bool finished;

	DWORD files_count;

	void work(DWORD);
};

typedef TClientJob* PClientJob;
typedef TClientJob& RClientJob;

#endif

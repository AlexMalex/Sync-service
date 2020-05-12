#if !defined(MAINCLASS_H)
#define MAINCLASS_H

#include "const.h"
#include "servsock.h"
#include "timeobj.h"
#include "point.h"
#include "dqueue.h"
#include "aobject.h"
#include "job.h"

class TMainClass {
private:
	PServerClientSocket socket;
	PTimeObject timer;

	PPoint point;
	DWORD jobId;

	PDQueue local_macros;

	PDQueue files;

	char* input_buffer;
	DWORD position;

	char* client_buffer;

	bool connected;

	DWORD sessionId;

	bool in_transaction;

	bool parse_buffer(DWORD);

	void parse_command();

	void error_frame(DWORD, DWORD, bool=true);
	void ok_frame(DWORD=0);

	// commands
	bool command_open(DWORD, DWORD);
	void command_close();

	void command_get_hash();

	void command_get_file();

	void command_put_file();

	void command_transaction();

	void command_enum_files();

	void command_delete_file();

	void command_clean_files();

	//support
	bool init(DWORD);
	void done();

	void replace_local_macros(PString);

	void add_local_object(PAObject);
	void remove_local_object();

	void print_info(DWORD, char*, char* =NULL);
	void print_summary(DWORD, __int64, DWORD);

	char* get_point_id();

public:
	TMainClass(PServerClientSocket);
	virtual ~TMainClass();

	bool finished;

	bool process_messages();
};

typedef TMainClass* PMainClass;
typedef TMainClass& RMainClass;

#endif

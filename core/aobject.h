#if !defined(AOBJECT_H)
#define AOBJECT_H

#include "const.h"
#include "configobj.h"
#include "dqueue.h"

class TAObject: public TConfigObject {
private:
	PEvent event;
	HANDLE handle;
	DWORD bytes;
	OVERLAPPED hOverlapped;

	void cleanup_handles();

#pragma pack(push)
#pragma pack(8)
	char buffer[MAX_EVENT_BUFFER_SIZE];
#pragma pack(pop)

public:
	bool overwrite;

	bool recursive;

	PString client_path;
	PString server_path;

	PString file;

	bool has_error;

	TAObject();
	virtual ~TAObject();

	inline bool get_error() { return has_error;};
	inline void clear_error() { has_error=false;};
	inline void set_error() { has_error=true;};

	void init_changes();
	BOOL has_no_changes();
	void done_changes();
};

typedef TAObject* PAObject;
typedef TAObject& RAObject;

#endif

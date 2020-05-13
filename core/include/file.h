#if !defined(TFILE_H)
#define TFILE_H

#include "handle.h"

class TFile: public THandleObject {
private:
	bool open_file(char*, bool=false, bool=false, bool=false);

public:
	TFile();
	TFile(char*, bool=false, bool=false, bool=false);

	virtual ~TFile();

	bool open(char*, bool=false, bool=false, bool=false);
	void close();

	bool read(char*, DWORD);
	bool write(char*, DWORD);

	bool get_time(FILETIME*);
	bool set_time(FILETIME*);

	bool set_pointer(__int64, DWORD);

	__int64 size();
};

typedef TFile* PFile;
typedef TFile& RFile;

#endif

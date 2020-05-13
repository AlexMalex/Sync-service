#include "file.h"

TFile::TFile()
{
}

TFile::TFile(char* name, bool need_write, bool rewrite_existing, bool exclusive)
{
	open_file(name, need_write, rewrite_existing, exclusive);
}

TFile::~TFile()
{
	FlushFileBuffers(handle);
}

bool TFile::open_file(char* name, bool need_write, bool rewrite_existing, bool exclusive)
{
	DWORD mode=OPEN_EXISTING;
	DWORD open_mode=GENERIC_READ;
	DWORD share_mode=FILE_SHARE_READ;

	if (rewrite_existing) mode=CREATE_ALWAYS;

	if (need_write) open_mode=open_mode|GENERIC_WRITE;

	if (!exclusive) share_mode=share_mode|FILE_SHARE_WRITE;

	handle=CreateFile(name, open_mode, share_mode, NULL, mode, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	return (isValid());
}

bool TFile::open(char* name, bool need_write, bool rewrite_existing, bool exclusive)
{
	if (isValid()) CloseHandle(handle);

	return open_file(name, need_write, rewrite_existing, exclusive);
}

void TFile::close()
{
	if (!isValid()) return;

	CloseHandle(handle);

	handle=NULL;
}

bool TFile::read(char* buffer, DWORD size)
{
	DWORD readed;

	if (!isValid()) return false;

	if (ReadFile(handle, buffer, size, &readed, NULL)!=FALSE) {
		if (size!=readed) return false;
		else return true;
	};

	return false;
}

bool TFile::write(char* buffer, DWORD size)
{
	DWORD written;

	if (!isValid()) return false;

	if (WriteFile(handle, buffer, size, &written, NULL)!=FALSE) {
		if (size!=written) return false;
		else return true;
	};

	return false;
}

__int64 TFile::size()
{
	__int64 file_size;

	GetFileSizeEx(handle, (PLARGE_INTEGER)&file_size);

	return file_size;
}

bool TFile::get_time(FILETIME* write_time)
{
	if (!isValid()) return false;

	return (GetFileTime(handle, NULL, NULL, write_time)!=FALSE);
}

bool TFile::set_time(FILETIME* write_time)
{
	if (!isValid()) return false;

	return (SetFileTime(handle, NULL, NULL, write_time)!=FALSE);
}

bool TFile::set_pointer(__int64 position, DWORD method)
{
	LARGE_INTEGER value;

	value.QuadPart=position;

	return (SetFilePointerEx(handle, value, NULL, method)!=INVALID_SET_FILE_POINTER);
}

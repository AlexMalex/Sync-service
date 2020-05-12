#include "support.h"
#include "aobject.h"

#pragma warning(disable: 4127)

extern PDQueue macros;

TAObject::TAObject():TConfigObject(NULL, 0), event(NULL), overwrite(true), recursive(false), has_error(false), handle(INVALID_HANDLE_VALUE)
{
	client_path=new TString(NULL);

	server_path=new TString(NULL);

	file=new TString(NULL);

	event=new TEvent(NULL);

	hOverlapped.hEvent=event->getHandle();
}

TAObject::~TAObject()
{
	if (event!=NULL) delete event;

	if (file!=NULL) delete file;

	if (server_path!=NULL) delete server_path;

	if (client_path!=NULL) delete client_path;
}

void TAObject::init_changes()
{
	BOOL result;
	TString client_path_local;

	client_path_local.set_string(client_path->get_string());

	replace_templates(&client_path_local, macros);

	handle=CreateFile(client_path_local.get_string(), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED, 0);

	if (handle==INVALID_HANDLE_VALUE) return;

	result=ReadDirectoryChangesW(handle, buffer, sizeof(buffer), TRUE, FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_DIR_NAME, &bytes, &hOverlapped, NULL);
}

BOOL TAObject::has_no_changes()
{
	BOOL NoChanges=TRUE;
	DWORD next, written, result, action;

	while (NoChanges) {
		next=(DWORD)&buffer;

		result=GetOverlappedResult(handle, &hOverlapped, &written, FALSE);

		if ((result==0)&&(GetLastError()==ERROR_IO_INCOMPLETE)) break;

		if (result==0) return NoChanges;

		if (result!=0) {
			do {
				action=((PFILE_NOTIFY_INFORMATION)next)->Action;

				if (action==FILE_ACTION_ADDED) {
					NoChanges=FALSE;

					break;
				};
				
				if (((PFILE_NOTIFY_INFORMATION)next)->NextEntryOffset==0) break;

				next=next+((PFILE_NOTIFY_INFORMATION)next)->NextEntryOffset;
			} while (true);
		};

		if (NoChanges) result=ReadDirectoryChangesW(handle, buffer, sizeof(buffer), TRUE, FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_DIR_NAME, &bytes, &hOverlapped, NULL);
	};

	return NoChanges;
}

void TAObject::done_changes()
{
	cleanup_handles();
}

void TAObject::cleanup_handles()
{
	if (handle!=INVALID_HANDLE_VALUE) {
		CloseHandle(handle);

		handle=INVALID_HANDLE_VALUE;
	};
}

#include "svcobj.h"
#include "filesupd.h"
#include "support.h"
#include "newlogger.h"

#include <strsafe.h>

extern char* SyncEventName;

extern PServiceObject serviceProcess;

// points
extern PDQueue points;
// objects
extern PDQueue objects;
// jobs
extern PDQueue jobs;
// jobs
extern PDQueue macros;

extern PPoint self;
extern PPoint sync_point;

extern DWORD self_id; 
extern DWORD sync_id; 

extern char* system_queue_points;
extern char* system_queue_jobs;

extern hash_array conf_files[5];
extern hash_array exe_hash[2];

extern char* exe_names[2];

extern char* dir_config;
extern char* dir_update;

extern char* config_names[5];

bool TFileUpdate::busy=false;

extern PNewLogger log;

extern char* info_str[];

extern char* image_name;
extern char* image_path;

extern char* image_updater;

TFileUpdate::TFileUpdate()
{
	syncEvent=new TEvent(SyncEventName, true, true);
}

TFileUpdate::~TFileUpdate()
{
	if (syncEvent!=NULL) delete syncEvent;
}

void TFileUpdate::execute()
{
	int error;
	PPoint point;
	hash_array hash;
	char file_name[MAX_PATH_LENGTH], source[MAX_PATH_LENGTH];
	bool need_update_cfg=false, need_update_exe=false, need_update_updater=false;
	PClientJob job;
	STARTUPINFO si;
    PROCESS_INFORMATION pi;

	if (busy) return;

	busy=true;

	// test if we need update files
	if (sync_id==0) {
unbusy:
		busy=false;

		return;
	};

	// remote update
	if (self_id!=sync_id) {
		// update cfg files
		job=new TClientJob();

		if (job==NULL) goto unbusy;

		log->logEvent(info_str[CLIENT_TASK_START_STR], INFO, 1);
		
		job->work(1);

		log->logEvent(info_str[CLIENT_TASK_STOP_STR], INFO, 1);
		
		if (job->files_count>0) need_update_cfg=true;

		if (job!=NULL) delete job;

		// update syncservice.exe image
		job=new TClientJob();

		if (job==NULL) goto unbusy;

		log->logEvent(info_str[CLIENT_TASK_START_STR], INFO, 2);
		
		job->work(2);

		log->logEvent(info_str[CLIENT_TASK_STOP_STR], INFO, 2);
		
		if (job->files_count>0) need_update_exe=true;

		if (job!=NULL) delete job;

		// update syncupdater.exe image
		job=new TClientJob();

		if (job==NULL) goto unbusy;

		log->logEvent(info_str[CLIENT_TASK_START_STR], INFO, 3);
		
		job->work(3);

		log->logEvent(info_str[CLIENT_TASK_STOP_STR], INFO, 3);
		
		if (job->files_count>0) need_update_updater=true;

		if (job!=NULL) delete job;
	}
	// local update
	else {
		// skip update service.xml
		for (UINT i=1; i<sizeof(conf_files)/sizeof(hash_array); i++) {
			StringCchCopy(file_name, MAX_PATH_LENGTH, dir_config);

			StringCchCat(file_name, MAX_PATH_LENGTH, config_names[i]);

			if (!get_file_hash(file_name, hash)) continue;

			for (UINT j=0; j<MD5LEN; j++) {
				if (conf_files[i][j]!=hash[j]) {
					need_update_cfg=true;

					break;
				};
			};
		};

		for (UINT i=0; i<sizeof(exe_hash)/sizeof(hash_array); i++) {
			StringCchCopy(file_name, MAX_PATH_LENGTH, dir_update);

			StringCchCat(file_name, MAX_PATH_LENGTH, exe_names[i]);

			if (!get_file_hash(file_name, hash)) continue;

			for (UINT j=0; j<MD5LEN; j++) {
				if (exe_hash[i][j]!=hash[j]) {
					switch (i) {
						case 0: 
							need_update_exe=true;

							break;
						case 1:
							need_update_updater=true;

							break;
						default:;
					};
					
					break;
				};
			};
		}
	};

	if (need_update_updater) {
		// destination name
		StringCchCopy(file_name, MAX_PATH_LENGTH, image_path);

		StringCchCat(file_name, MAX_PATH_LENGTH, image_updater);

		// source name
		StringCchCopy(source, MAX_PATH_LENGTH, dir_update);

		StringCchCat(source, MAX_PATH_LENGTH, image_updater);

		if (CopyFile(source, file_name, FALSE)!=FALSE) {
			log->logEvent(info_str[UPDATE_UPDATER_OK], INFO);

			get_file_hash(file_name, exe_hash[1]);
		}
		else log->logEvent(info_str[UPDATE_UPDATER_ERROR], ERR, GetLastError());
	};

	if (!((need_update_cfg)||(need_update_exe))) goto unbusy;

	if ((!need_update_exe)&&(need_update_cfg)) {
		// main thread+scheduler stop work
		syncEvent->resetEvent();

		try  {
			log->logEvent(info_str[UPDATE_CONFIG_START_STR], INFO);

			// end all client threads
			serviceProcess->scan_threads(true);

			// end all job threads
			serviceProcess->get_scheduler()->scan_tasks(true);

			// clear all
			points->clear();
			objects->clear();
			jobs->clear();
			macros->clear();

			define_macros();

			// skip to reload service.xml !!!
			error=init_config(1);

			if (error>0) {
	error_out:
				serviceProcess->terminate();

				serviceProcess->get_scheduler()->terminate();

				goto out;
			};

			if (points->isEmpty()) goto error_out;

			point=(PPoint)find_object_by_id(self_id, system_queue_points);

			if (point==NULL) goto error_out;
		
			self=point;

			get_sys_files_hash();

			if (sync_id!=0) {
				point=(PPoint)find_object_by_id(sync_id, system_queue_points);

				if (point!=NULL) sync_point=point;
				else sync_id=0;
			};

			if (!make_system_objects()) goto error_out;

			log->logEvent(info_str[UPDATE_CONFIG_ENDED_STR], INFO);
		}
		catch(...) {
		};
out:
		// main thread_scheduler start work
		syncEvent->setEvent();
	}
	else {
		log->logEvent(info_str[UPDATE_EXE_STARTED_STR], INFO);

		serviceProcess->terminate();

		serviceProcess->get_scheduler()->terminate();

		ZeroMemory(&si, sizeof(si));

		si.cb=sizeof(si);

		ZeroMemory(&pi, sizeof(pi));

		StringCchCopy(file_name, MAX_PATH_LENGTH, image_path);

		StringCchCat(file_name, MAX_PATH_LENGTH, image_updater);

		if (CreateProcess(file_name, NULL, NULL, NULL, FALSE, 0, NULL, NULL,  &si, &pi)==NULL) log->logEvent(info_str[UPDATE_EXE_FAILED_STR], ERR, GetLastError());

		// Close process and thread handles. 
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	};

	busy=false;
}

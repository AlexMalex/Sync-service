#include "const.h"
#include "support.h"
#include "newlogger.h"
#include "point.h"
#include "errors.h"
#include "tmacro.h"
#include "service.h"
#include "xmlparse.h"
#include "timeobj.h"
#include "protocol.h"
#include "thash.h"
#include "file.h"
#include "aobject.h"
#include "job.h"
#include "tstring.h"
#include "filestruct.h"

#include <strsafe.h>
#include <shlobj.h>
#include <time.h>

#pragma warning (disable: 4482) // use enum in full name
#pragma warning (disable: 4127) // while (true)

extern PNewLogger log;

extern char* image_path;
extern char* image_name;

extern char* log_name;
extern char* log_path;

extern char* config_path;
extern char* recieve_path;
extern char* update_path;

extern char* dir_recieve;
extern char* dir_log;
extern char* dir_config;
extern char* dir_update;

extern char* memory_error;
extern char* winsock_error;
extern char* logger_error;
extern char* xml_error;
extern char* dir_path_error;
extern char* ctrl_dispetcher;
extern char* create_objects_error;
extern char* bad_service_config;
extern char* bad_points_config;
extern char* recv_dir_error;
extern char* bad_system_objects;

extern DWORD self_id;

extern PPoint self;

extern PDQueue points;
extern PDQueue jobs;
extern PDQueue objects;
extern PDQueue macros;

extern DWORD sync_id;

extern PPoint sync_point;

extern PDQueue queues;

extern char* system_queue_name;
extern char* system_queue_points;
extern char* system_queue_objects;
extern char* system_queue_jobs;
extern char* system_queue_macros;

extern macro_struct macro[3];

extern int port;

extern DWORD timeout;

extern PService service_ptr;

extern char* invalid_chars;

extern char* macro_date_name;
extern char* format_date_str;

extern char* config_names[5];
extern hash_array conf_files[5];

extern char* config_points_name;
extern char* config_objects_name;
extern char* config_jobs_name;
extern char* config_macros_name;

extern char* sys_update;
extern char* sys_image_update;

extern char* create_file;

extern char* ServiceDisplayName;
extern char* image_name;

extern char* exe_names[2];
extern hash_array exe_hash[2];

extern DWORD sync_period;

extern char* image_updater;
extern char* sys_image_updater;

extern WORD monthes[12];

extern char* info_str[];

extern char* slash;
extern char* wildcard;

extern char* operation_delete_file;

extern char* OK_STR;
extern char* ERROR_STR;

using namespace socket_base;

int check_path(char* name, int num)
{
	// create dir if not exists
	if (!PathFileExists(name)) {
		if (CreateDirectory(name, NULL)==0) {
			service_ptr->windows_log(dir_path_error, OPEN_DIRECTORY_ERROR_CFG+num);

			return -1;
		};
	};

	return NO_ERROR;
}

int create_objects()
{
	queues=new DQueue(system_queue_name, false);

	if (queues==NULL) return -1;

	points=new DQueue(system_queue_points, true);

	if (points==NULL) return -1;

	queues->addElement(points, false);

	objects=new DQueue(system_queue_objects, true);

	if (objects==NULL) return -1;

	queues->addElement(objects, false);

	jobs=new DQueue(system_queue_jobs, true);

	if (jobs==NULL) return -1;

	queues->addElement(jobs, false);

	macros=new DQueue(system_queue_macros, true);

	if (macros==NULL) return -1;

	queues->addElement(macros, false);

	return NO_ERROR;
}

void destroy_objects()
{
	if (macros!=NULL) delete macros;

	if (points!=NULL) delete points;

	if (objects!=NULL) delete objects;

	if (jobs!=NULL) delete jobs;

	if (queues!=NULL) delete queues;
}

// define well known macros
void define_macros()
{
	PMacro macro_def;
	char buffer[MAX_PATH];

	for (UINT i=0; i<(sizeof(macro)/sizeof(macro_struct)); i++)	{
		if (SHGetSpecialFolderPath(0, buffer, macro[i].folder, FALSE)==TRUE) {
			macro_def=new TMacro(macro[i].macro_def, buffer);

			if (macro_def!=NULL) {
				macros->addElement(macro_def, true);
			};
		};
	};
}

bool init_base_path()
{
	char buffer[MAX_PATH_LENGTH+1];
	char* slash;
	size_t size;

	// image_path - base path where we started
	StringCchCopy(buffer, MAX_PATH_LENGTH, __argv[0]);

	for (UINT i=0; buffer[i]!='\0'; i++) {
		buffer[i]=(char)tolower(buffer[i]);
	};

	slash=StrStr(buffer, image_name);

	*slash='\0';

	StringCchLength(buffer, MAX_PATH_LENGTH, &size);

	size++;

	image_path=new char[size];

	if (image_path==NULL) {
		service_ptr->windows_log(memory_error, 1);

		return false;
	};

	StringCchCopy(image_path, size, buffer);

	// prepare recieve dir
	StringCchCopy(buffer, MAX_PATH_LENGTH, image_path);
	
	StringCchCat(buffer, MAX_PATH_LENGTH, recieve_path);

	StringCchLength(buffer, MAX_PATH_LENGTH, &size);

	size++;

	dir_recieve=new char[size];

	if (dir_recieve==NULL) {
		service_ptr->windows_log(memory_error, 1);

		return false;
	};

	StringCchCopy(dir_recieve, size, buffer);

	// prepare log dir
	StringCchCopy(buffer, MAX_PATH_LENGTH, image_path);

	StringCbCat(buffer, MAX_PATH_LENGTH, log_path);

	StringCchLength(buffer, MAX_PATH_LENGTH, &size);

	size++;

	dir_log=new char[size];

	if (dir_log==NULL) {
		service_ptr->windows_log(memory_error, 1);

		return false;
	};

	StringCchCopy(dir_log, size, buffer);

	// prepare system dir
	StringCchCopy(buffer, MAX_PATH_LENGTH, image_path);
	
	StringCchCat(buffer, MAX_PATH_LENGTH, config_path);

	StringCchLength(buffer, MAX_PATH_LENGTH, &size);

	size++;

	dir_config=new char[size];

	if (dir_config==NULL) {
		service_ptr->windows_log(memory_error, 1);

		return false;
	};

	StringCchCopy(dir_config, size, buffer);

	// prepare update dir
	StringCchCopy(buffer, MAX_PATH_LENGTH, image_path);
	
	StringCchCat(buffer, MAX_PATH_LENGTH, update_path);

	StringCchLength(buffer, MAX_PATH_LENGTH, &size);

	size++;

	dir_update=new char[size];

	if (dir_update==NULL) {
		service_ptr->windows_log(memory_error, 1);

		return false;
	};

	StringCchCopy(dir_update, size, buffer);

	// init recieve and log dirs
	if (check_path(dir_recieve, 3)!=NO_ERROR) return false;

	if (check_path(dir_log, 2)!=NO_ERROR) return false;

	if (check_path(dir_config, 1)!=NO_ERROR) return false;

	if (check_path(dir_update, 4)!=NO_ERROR) return false;

	return true;
}

void done_base_path()
{
	if (image_path!=NULL) delete image_path;

	if (dir_config!=NULL) delete dir_config;

	if (dir_log!=NULL) delete dir_log;

	if (dir_recieve!=NULL) delete dir_recieve;
}

int init_all()
{
	char buffer[MAX_PATH_LENGTH+1];

	PPoint point;

	if (!init_base_path()) {
		service_ptr->windows_log(dir_path_error, START_OPEN_DIRECTIORY);

		return -1;
	};

	// init socket library
	if (!TBaseSocket::Init()) {
		service_ptr->windows_log(winsock_error, START_WINSOCK_ERROR);

		return -1;
	};

	// init objects - before xml parsing !!!
	if (create_objects()!=NO_ERROR) {
		service_ptr->windows_log(create_objects_error, ERROR_CREATE_OBJECTS);

		return -1;
	};

	define_macros();

	int error=init_config(0);

	if (error!=NO_ERROR) {
		service_ptr->windows_log(xml_error, error);

		return -1;
	};

	if (self_id==0) {
		service_ptr->windows_log(bad_service_config, ERROR_SELF_ID);

		return -1;
	};

	if (points->isEmpty()) {
		service_ptr->windows_log(bad_points_config, ERROR_NO_POINTS);

		return -1;
	};

	point=(PPoint)find_object_by_id(self_id, system_queue_points);

	if (point==NULL) {
		service_ptr->windows_log(bad_service_config, ERROR_SELF_LINK);

		return -1;
	};

	// make self reference to point
	self=point;

	get_sys_files_hash();

	if (sync_id!=0) {
		point=(PPoint)find_object_by_id(sync_id, system_queue_points);

		if (point!=NULL) sync_point=point;
		else sync_id=0;
	};

	if (!make_system_objects()) {
		service_ptr->windows_log(bad_system_objects, ERROR_SYSTEM_OBJECTS);

		return -1;
	};

	// init log
	StringCchCopy(buffer, MAX_PATH_LENGTH, dir_log);

	StringCbCat(buffer, MAX_PATH_LENGTH, log_name);

	log=new TNewLogger(buffer);

	if (log==NULL) {
invalid_log:
		service_ptr->windows_log(logger_error, START_LOGGER_ERROR);

		return -1;
	};

	if (!log->isValid()) goto invalid_log;

	return NO_EXIT;
}

void done_all()
{
	// done log
	if (log!=NULL) delete log;

	destroy_objects();

	// done socket library
	TBaseSocket::Done();

	done_base_path();
}

// internal helper
PDQueue get_queue(char* name)
{
	PDQueue result=NULL;
	PBlock head;

	head=queues->head;

	while (head!=NULL) {
		if ((*(PDQueue)head->obj)==name) {
			return (PDQueue)head->obj;
		};

		head=head->next;
	};

	return result;
}

PConfigObject find_object_by_id(DWORD id, char* name)
{
	PBlock head;
	PConfigObject object, result=NULL;
	PDQueue queue=NULL;

	queue=get_queue(name);

	if (queue==NULL) return NULL;

	queue->enter();

	try {
		head=queue->head;

		while (head!=NULL) {
			object=(PConfigObject)head->obj;

			if (object!=NULL) {
				if (object->get_id()==id) {
					result=(PConfigObject)object;

					break;
				};
			};

			head=head->next;
		};
	}
	catch (...) {
	};

	queue->leave();

	return result;
}

char* get_name_by_id(DWORD id, char* name)
{
	char* result=NULL;
	PBlock head;
	PConfigObject object;
	PDQueue queue;

	queue=get_queue(name);

	if (queue==NULL) return NULL;

	queue->enter();

	try {
		head=queue->head;

		while (head!=NULL) {
			object=(PConfigObject)head->obj;

			if (object->get_id()==id) {
				result=object->get_string();

				break;
			};
		};
	}
	catch (...) {
	};

	queue->leave();

	return result;
}

void replace_templates(PString str, PDQueue queue)
{
	PBlock block=NULL;
	PMacro macro;

	if (queue==NULL) return;

	if (!(str->has_symbol(macro_symbol))) return;

	block=queue->head;

	while (block!=NULL) {
		macro=(PMacro)block->obj;

		if (macro!=NULL) {
			str->replace_str(macro->get_hash(), macro->get_value());
		};

		if (!(str->has_symbol(macro_symbol))) return;

		block=block->next;
	};
}

bool has_trailing_slash(char* str)
{
	size_t length;

	if (str==NULL) return false;

	if (SUCCEEDED(StringCchLength(str, STRSAFE_MAX_CCH, &length))) {
		if (length==0) return false;

		if (str[length-1]=='\\') return true;
	};

	return false;
}

DWORD get_answer(PClientSocket socket, char* buffer, DWORD size, bool &flag_to_exit, bool use_standart_timeout)
{
	DWORD position, bytes_to_read, result=ERR_NO, error;
	TimeObject time;

	position=0;
	bytes_to_read=sizeof(frame);

	if (use_standart_timeout)
		time.init(timeout);
	else {
		time.init(REMOTE_HASHING_TIMEOUT);
	};

	do {
		if (flag_to_exit) {
			result=ERR_SHUTDOWN;

			goto exit;
		};

		if (socket->read(&buffer[position], bytes_to_read)) {
			position=position+socket->get_read_bytes();
			
			bytes_to_read=bytes_to_read-socket->get_read_bytes();

			if (bytes_to_read==0) {
				if (((frame*)buffer)->type==frame_type::ok) break;
				
				result=ERR_UNKNOWN;

				goto exit;
			};
		};

		if (!socket->isConnected()) {
			result=ERR_DISCONNECTED;

			goto exit;
		};

		if (time.timeout()) {
			result=ERR_TIMEDOUT;

			goto exit;
		};

		// yield some time to windows
		Sleep(0);

	}  while (true);

	if (size==sizeof(frame)) goto exit;

	position=sizeof(frame);
	bytes_to_read=size-sizeof(frame);

	time.init(timeout);

	do {
		if (flag_to_exit) {
			result=ERR_SHUTDOWN;

			break;
		};

		if (socket->read(&buffer[position], bytes_to_read)) {
			position=position+socket->get_read_bytes();
			
			bytes_to_read=bytes_to_read-socket->get_read_bytes();

			if (bytes_to_read==0) break;
		};

		if (!socket->isConnected()) {
			result=ERR_DISCONNECTED;

			break;
		};

		if (time.timeout()) {
			result=ERR_TIMEDOUT;

			break;
		};

		// yield some time to windows
		Sleep(0);

	}  while (true);

exit:
	switch (result) {
		case ERR_DISCONNECTED: { 
			log->logEvent(info_str[SOCKET_DISCONNECTED_STR], ERR, SOCKET_DISCONNECTED);

			break;
		};

		case ERR_TIMEDOUT: {
			log->logEvent(info_str[READ_TIMEOUT_STR], ERR, ERR_TIMEOUT);
				
			break;
		};

		case ERR_SHUTDOWN: {
			log->logEvent(info_str[PROGRAM_TERMINATING_STR], ERR, PROGRAMM_TERMINATING);

			break;
		};

		default: {
			if (((frame*)(buffer))->type==frame_type::error) {
				error=((frame*)(buffer))->command;

				if (error==DONT_NEED_OVERWRITE) {
					log->logEvent(info_str[NO_NEED_OVERWRITE_STR], WARN, DONT_NEED_OVERWRITE);

					return DONT_NEED_OVERWRITE;
				};

				if (error>500) error=PROTOCOL_PACKET_CORRUPTED;

				log->logEvent(info_str[SERVER_PROTO_ERROR_STR], ERR, error);

				result=error;
			};
		};
	};

	return result;
}

DWORD getSessionId()
{
	LARGE_INTEGER count;

	QueryPerformanceCounter(&count);

	return count.LowPart;
}

bool has_invalid_chars(char* str, BYTE length)
{
	size_t size;

	if (!SUCCEEDED(StringCchLength(invalid_chars, STRSAFE_MAX_CCH, &size))) return false;

	if (str==NULL) return false;

	if (length==0) return false;

	for (UINT i=0; i<length; i++) {
		for (UINT j=0; j<size; j++) {
			if (str[i]==invalid_chars[j]) return true;
		};

		if (((BYTE)str[i])<32) return true;
	};

	return false;
}

bool create_dir(char* dir)
{
	char temp_buffer[MAX_PATH_LENGTH*2];
	char buffer[MAX_PATH_LENGTH];
	char* pos=dir, *net_path;

	if (dir==NULL) return false;

	// if path UNC, skip server+share
	net_path=StrStr(pos, "\\\\");

	if (net_path!=NULL) {
		pos=StrChr(net_path+2, '\\');

		if (pos==NULL) return false;

		pos++;

		pos=StrChr(pos, '\\');

		if (pos==NULL) return false;

		pos++;
	};

	do {
		pos=StrChr(pos, '\\');

		if (pos==NULL) break;

		CopyMemory(buffer, dir, pos-dir);

		buffer[pos-dir]='\0';

		if (PathFileExists(buffer)) goto next;

		if (!CreateDirectory(buffer, NULL)) {
			if (GetLastError()==ERROR_ALREADY_EXISTS) goto next;

			if (SUCCEEDED(StringCbPrintf(temp_buffer, sizeof(temp_buffer), create_file, buffer))) {
				log->logEvent(temp_buffer, INFO, GetLastError());
			};

			return false;
		};
next:
		pos++;

	} while (1);

	return true;
}

void get_date_str(char* buffer, DWORD size)
{
	SYSTEMTIME current_time;

	GetLocalTime(&current_time);
	
	StringCbPrintf(buffer, size, format_date_str, current_time.wYear, current_time.wMonth, current_time.wDay);
}

bool get_file_hash(char* file_name, char* hash)
{
	THash file_hash;
	char* file_buffer;
	TFile file;
	__int64 pages, remainder;
	bool has_error=false;

	if (!file_hash.isValid()) return false;

	file_buffer=new char[HASH_BUFFER_SIZE];

	if (file_buffer==NULL) return false;

	file.open(file_name);

	if (!file.isValid()) {
error:
		has_error=true;

		goto clean_buffer;
	};

	pages=file.size()/HASH_BUFFER_SIZE;

	for (UINT j=0; j<pages; j++) {
		if (!file.read(file_buffer, HASH_BUFFER_SIZE)) goto error;

		if (!file_hash.hash_data(file_buffer, HASH_BUFFER_SIZE)) goto error;
	};

	remainder=file.size()-(pages*HASH_BUFFER_SIZE);

	if (remainder>0) {
		if (!file.read(file_buffer, (DWORD)remainder)) goto error;

		if (!file_hash.hash_data(file_buffer, (DWORD)remainder)) goto error;
	};

	if (!file_hash.get_hash(hash)) has_error=true;

clean_buffer:
	if (file_buffer!=NULL) delete file_buffer;

	return !has_error;
}

void get_sys_files_hash()
{
	char file_name[MAX_PATH_LENGTH];

	for (UINT i=0; i<(sizeof(conf_files)/sizeof(hash_array)); i++) {
		FillMemory(conf_files[i], sizeof(hash_array), '\x0');

		StringCchCopy(file_name, MAX_PATH_LENGTH, dir_config);

		StringCchCat(file_name, MAX_PATH_LENGTH, config_names[i]);

		get_file_hash(file_name, conf_files[i]);
	};

	for (UINT i=0; i<(sizeof(exe_hash)/sizeof(hash_array)); i++) {
		FillMemory(exe_hash[i], sizeof(hash_array), '\x0');

		StringCchCopy(file_name, MAX_PATH_LENGTH, image_path);

		StringCchCat(file_name, MAX_PATH_LENGTH, exe_names[i]);

		get_file_hash(file_name, exe_hash[i]);
	};
}

bool make_system_objects()
{
	PAObject object;
	PDQueue queue;
	PJob job;
	PPoint point;
	DWORD period;

	// get obects queue
	queue=get_queue(system_queue_objects);

	// make points.xml object
	object=new TAObject();

	if (object==NULL) return false;

	object->set_id(1);

	object->set_string(config_points_name);

	object->client_path->set_string(dir_config);
	object->server_path->set_string(dir_config);

	object->file->set_string(config_points_name);

	queue->addElement(object);

	// make objects.xml object
	object=new TAObject();

	if (object==NULL) return false;

	object->set_id(2);

	object->set_string(config_objects_name);

	object->client_path->set_string(dir_config);
	object->server_path->set_string(dir_config);

	object->file->set_string(config_objects_name);

	queue->addElement(object);

	// make jobs.xml object
	object=new TAObject();

	if (object==NULL) return false;

	object->set_id(3);

	object->set_string(config_jobs_name);

	object->client_path->set_string(dir_config);
	object->server_path->set_string(dir_config);

	object->file->set_string(config_jobs_name);

	queue->addElement(object);

	// make macros.xml object
	object=new TAObject();

	if (object==NULL) return false;

	object->set_id(4);

	object->set_string(config_macros_name);

	object->client_path->set_string(dir_config);
	object->server_path->set_string(dir_config);

	object->file->set_string(config_macros_name);

	queue->addElement(object);

	// make syncservice.exe object
	object=new TAObject();

	if (object==NULL) return false;

	object->set_id(5);

	object->set_string(ServiceDisplayName);

	object->client_path->set_string(dir_update);
	object->server_path->set_string(dir_update);

	object->file->set_string(image_name);

	queue->addElement(object);

	// make syncupdater.exe object
	object=new TAObject();

	if (object==NULL) return false;

	object->set_id(6);

	object->set_string(image_updater);

	object->client_path->set_string(dir_update);
	object->server_path->set_string(dir_update);

	object->file->set_string(image_updater);

	queue->addElement(object);

	// get jobs queue
	queue=get_queue(system_queue_jobs);
	
	//make job - update system files
	job=new TJob();

	if (job==NULL) return false;

	job->set_id(1);

	job->set_string(sys_update);

	job->set_active(true);

	job->set_action(action ::read);

	job->set_transaction(true);

	object=(PAObject)find_object_by_id(4, system_queue_objects);

	if (object==NULL) return false;

	job->get_objects()->addElement(object, false);

	object=(PAObject)find_object_by_id(3, system_queue_objects);

	if (object==NULL) return false;

	job->get_objects()->addElement(object, false);

	object=(PAObject)find_object_by_id(2, system_queue_objects);

	if (object==NULL) return false;

	job->get_objects()->addElement(object, false);

	object=(PAObject)find_object_by_id(1, system_queue_objects);

	if (object==NULL) return false;

	job->get_objects()->addElement(object, false);

	// from midnight
	job->set_time(0, 0);

	period=sync_period/60000;  // msec -> min

	if (period==0) period=1;

	job->set_period(period);

	// all day in minutes
	job->set_duration(1440);

	job->init_daily();

	point=(PPoint)find_object_by_id(sync_id, system_queue_points);

	if (point==NULL) return false;

	job->set_destination(point);

	queue->addElement(job);

	//make job - update program image
	job=new TJob();

	if (job==NULL) return false;

	job->set_id(2);

	job->set_string(sys_image_update);

	job->set_active(true);

	job->set_action(action ::read);

	job->set_transaction(false);

	object=(PAObject)find_object_by_id(5, system_queue_objects);

	if (object==NULL) return false;

	job->get_objects()->addElement(object, false);

	// from midnight
	job->set_time(0, 0);

	period=sync_period/60000; // msec -> min

	if (period==0) period=1;

	job->set_period(period);

	// all day in minutes
	job->set_duration(1440);

	job->init_daily();

	point=(PPoint)find_object_by_id(sync_id, system_queue_points);

	if (point==NULL) return false;

	job->set_destination(point);

	queue->addElement(job);

	//make job - update updater image
	job=new TJob();

	if (job==NULL) return false;

	job->set_id(3);

	job->set_string(sys_image_updater);

	job->set_active(true);

	job->set_action(action ::read);

	job->set_transaction(false);

	object=(PAObject)find_object_by_id(6, system_queue_objects);

	if (object==NULL) return false;

	job->get_objects()->addElement(object, false);

	// from midnight
	job->set_time(0, 0);

	period=sync_period/60000; // msec -> min

	if (period==0) period=1;

	job->set_period(period);

	// all day in minutes
	job->set_duration(1440);

	job->init_daily();

	point=(PPoint)find_object_by_id(sync_id, system_queue_points);

	if (point==NULL) return false;

	job->set_destination(point);

	queue->addElement(job);

	return true;
}

char* find_filename(char* str)
{
	char* pos;
	size_t length;

	if (str==NULL) return str;

	if (!SUCCEEDED(StringCchLength(str, STRSAFE_MAX_CCH, &length))) return str;

	pos=str+(length-1);

	while (pos!=str) {
		if (*pos=='\\') {
			pos++;

			return pos;
		};

		pos--;
	};

	return str;
}

DWORD read_data(PClientSocket socket, char* buffer, DWORD size, bool &flag_to_exit)
{
	DWORD position, bytes_to_read, result=ERR_UNKNOWN;
	TimeObject time;

	position=0;
	bytes_to_read=size;

	time.init(timeout);

	do {
		if (flag_to_exit) {
			result=ERR_SHUTDOWN;
			
			break;
		};

		if (socket->read(&buffer[position], bytes_to_read)) {
			position=position+socket->get_read_bytes();
			
			bytes_to_read=bytes_to_read-socket->get_read_bytes();

			if (bytes_to_read==0) return ERR_NO;

			time.init(timeout);
		};

		if (!socket->isConnected()) {
			result=ERR_DISCONNECTED;

			break;
		};

		if (time.timeout()) {
			result=ERR_TIMEDOUT;
			
			break;
		};

		// yield some time to windows
		Sleep(0);

	}  while (1);

	switch (result) {
		case ERR_DISCONNECTED: { 
			log->logEvent(info_str[SOCKET_DISCONNECTED_STR], ERR, SOCKET_DISCONNECTED);

			break;
		};

		case ERR_TIMEDOUT: {
			log->logEvent(info_str[READ_TIMEOUT_STR], ERR, ERR_TIMEOUT);
				
			break;
		};

		case ERR_SHUTDOWN: {
			log->logEvent(info_str[PROGRAM_TERMINATING_STR], ERR, PROGRAMM_TERMINATING);

			break;
		};
	};
	
	return result;
}

void calc_time_diff(WORD& days, WORD& hours, WORD& minutes, time_t& old_time)
{
	time_t curr_time, diff;

	days=0;
	hours=0;
	minutes=0;

	curr_time=time(NULL);

	diff=curr_time-old_time;

	days=(WORD)(diff/DAY_IN_SECONDS); // diff in seconds

	hours=(WORD)((diff-(days*DAY_IN_SECONDS))/HOUR_IN_SECONDS);

	minutes=(WORD)((diff-(days*DAY_IN_SECONDS)-(hours*HOUR_IN_SECONDS))/MINUTE_IN_SECONDS);
}

// not case sensitive
bool string_match(char* str, char* mask)
{
	char buffer[MAX_PATH_LENGTH];
	char* pos=mask, *str_pos=str, *token_pos,  *find_str;
	size_t length;

	if ((str==NULL)||(mask==NULL)) return false;

	while ((*pos!='\x0')&&(*str_pos!='\x0')) {
		if (*pos=='*') {
			pos++;

			token_pos=buffer;

			while ((*pos!='\x0')&&((*pos!='*'))&&(*pos!='?')) {
				*token_pos=*pos;

				token_pos++;

				pos++;
			};

			*token_pos='\x0';
			
			StringCchLength(buffer, STRSAFE_MAX_CCH, &length);

			if (length==0) return true;

			find_str=StrStrI(str_pos, buffer);

			if (find_str==NULL) return false;

			str_pos=find_str+length;

			continue;
		};

		if (*pos=='?') {
			str_pos++;

			pos++;

			continue;
		};

		if (tolower(*pos)!=tolower(*str_pos)) return false;

		str_pos++;

		pos++;
	};

	return true;
}

DWORD file_search(char* base, char* path, char* name, PDQueue files_list, bool recursive, DWORD& recurse_level)
{
	HANDLE file_handle;
	WIN32_FIND_DATA find_file;
	TString filename, send_filename, find_filename;
	DWORD result=0;

	if ((base==NULL)||(name==NULL)) return 1;

	if (recurse_level>MAX_RECURSE_LEVEL) return 2;

	recurse_level++;

	find_filename.set_string(base);

	if (!has_trailing_slash(find_filename.get_string()))
		find_filename.add_string(slash);

	find_filename.add_string(path);

	if (!has_trailing_slash(find_filename.get_string()))
		find_filename.add_string(slash);

	find_filename.add_string(wildcard);

	file_handle=FindFirstFile(find_filename.get_string(), &find_file);
				
	if (file_handle==INVALID_HANDLE_VALUE) {
		if (GetLastError()==ERROR_FILE_NOT_FOUND) result=100;
		else result=3;

		goto out;
	};

	do  {
		// check for recursive search
		if (find_file.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
			if (!recursive) continue;

			if (find_file.cFileName[0]=='.') continue;

			filename.set_string(path);

			filename.add_string(find_file.cFileName);

			filename.add_string(slash);

			if (file_search(base, filename.get_string(), name, files_list, recursive, recurse_level)!=0) {
				result=4;

				goto out;
			};

			continue;
		};

		if (!string_match(find_file.cFileName, name)) continue;  // check for mask

		if (path!=NULL) {
			send_filename.set_string(path);

			if (!has_trailing_slash(send_filename.get_string()))
				send_filename.add_string(slash);

			send_filename.add_string(find_file.cFileName);
		}
		else send_filename.set_string(find_file.cFileName);

		files_list->addElement(new TString(send_filename.get_string()), true);
	} while (FindNextFile(file_handle, &find_file)==TRUE);

out:
	recurse_level--;

	if (file_handle!=INVALID_HANDLE_VALUE) FindClose(file_handle);

	return result;
}

DWORD file_del(char* base, char* path, PDQueue del_files, DWORD& recurse_level)
{
	HANDLE file_handle;
	WIN32_FIND_DATA find_file;
	TString filename, del_filename, find_filename;
	DWORD result=0;
	object_type type;

	if (base==NULL) return 1;

	if (recurse_level>MAX_RECURSE_LEVEL) return 2;

	recurse_level++;

	find_filename.set_string(base);

	if (!has_trailing_slash(find_filename.get_string()))
		find_filename.add_string(slash);

	if (path!=NULL) {
		find_filename.add_string(path);

		if (!has_trailing_slash(find_filename.get_string()))
			find_filename.add_string(slash);
	};

	find_filename.add_string(wildcard);

	file_handle=FindFirstFile(find_filename.get_string(), &find_file);
				
	if (file_handle==INVALID_HANDLE_VALUE) {
		if (GetLastError()==ERROR_FILE_NOT_FOUND) result=100;
		else result=3;

		goto out;
	};

	do  {
		type=object_type::TYPE_FILE;

		// check for recursive search
		if (find_file.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
			type=object_type::TYPE_DIRECTORY;

			if (find_file.cFileName[0]=='.') continue;

			filename.set_string(NULL);

			if (path!=NULL) {
				filename.set_string(path);

				if (!has_trailing_slash(find_filename.get_string()))
					filename.add_string(slash);
			};

			filename.add_string(find_file.cFileName);

			filename.add_string(slash);

			if (file_del(base, filename.get_string(), del_files, recurse_level)!=0) {
				result=4;

				goto out;
			};
		};

		del_filename.set_string(base);

		if (!has_trailing_slash(del_filename.get_string()))
			del_filename.add_string(slash);

		if (path!=NULL) {
			del_filename.add_string(path);

			if (!has_trailing_slash(del_filename.get_string()))
				del_filename.add_string(slash);
		};

		del_filename.add_string(find_file.cFileName);

		del_files->addElement_tail((PObject) new TFileStruct(del_filename.get_string(), type), true);
	} while (FindNextFile(file_handle, &find_file)==TRUE);

out:
	recurse_level--;

	if (file_handle!=INVALID_HANDLE_VALUE) FindClose(file_handle);

	return result;
}

char* format_value(double& value, char** str)
{
	DWORD counter=0;

	while ((value>1024)&&(counter<5)) {
		value=value/1024;

		counter++;
	};

	// byte
	return str[counter];
}

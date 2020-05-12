#include "clientjob.h"
#include "newlogger.h"
#include "const.h"
#include "protocol.h"
#include "support.h"
#include "aobject.h"
#include "tmacro.h"
#include "tmacro.h"
#include "file.h"
#include "thash.h"
#include "timeobj.h"
#include "errors.h"
#include "fileobj.h"
#include "filestruct.h"

#include <strsafe.h>

#pragma warning (disable: 4127) // while (true)

extern char* system_queue_jobs;

extern PNewLogger log;

extern char* info_str[];

extern int client_port;

extern DWORD timeout;

extern magic_sign magic_number;

extern char* dir_recieve;

extern char* temp_ext;

extern PDQueue macros;

extern char* local_macros_objects;

extern char* macro_point_name;
extern char* macro_point_id;

extern char* macro_object_name;
extern char* macro_object_id;

extern char* macro_date_name;

extern char* slash;

extern char* info_string;
extern char* info_string_short;

extern char* operation_get_file;
extern char* operation_put_file;
extern char* operation_get_hash;
extern char* operation_open_transaction;
extern char* operation_commit_transaction;
extern char* operation_undo_transaction;
extern char* operation_enum_files;
extern char* operation_close_transaction;
extern char* operation_delete_file;
extern char* operation_delete_dir;
extern char* operation_clean_files;
extern char* operation_clean_files_cancel;

extern char* OK_STR;
extern char* ERROR_STR;
extern char* PEER_ERROR;

extern char* info_attempt;
extern char* summary_string;

extern DWORD block_size;

extern DWORD self_id;

extern DWORD min_file_size;

extern char* wildcard;

TClientJob::TClientJob(): client_block_size(0), buffer(NULL), files_count(0), finished(false), recurse_level(false)
{
	local_macros=new DQueue(local_macros_objects, false);
}

TClientJob::~TClientJob()
{
	if (local_macros!=NULL) delete local_macros;

	if (buffer!=NULL) delete buffer;
}

void TClientJob::init()
{
	PMacro macro;
	char temp_buffer[MAX_DIGITS];
	char date[10];

	// %PointName%
	macro=new TMacro(macro_point_name, point->get_string());

	local_macros->addElement(macro, true);

	// %PointId%
	StringCbPrintf(temp_buffer, sizeof(temp_buffer), "%03d", point->get_id());

	macro=new TMacro(macro_point_id, temp_buffer);

	local_macros->addElement(macro, true);

	// %Date%
	get_date_str(date, sizeof(date));

	local_macros->addElement(new TMacro(macro_date_name, date), true);
}

bool TClientJob::connect()
{
	bool result=true;

	point=(PPoint)job->get_destination();

	if (point==NULL) return false;

	init();

	if (point->get_ip()==NULL) {
		log->logEvent(info_str[BAD_POINT_IP_STR], ERR, point->get_id());

		return false;
	};

	result=socket->connect(point->get_ip(), client_port);

	if (!result) {
		log->logEvent(info_str[NOT_CONNECTED_MAIN_IP_STR], ERR, point->get_id());

		if (point->get_altip()==NULL) return false;

		result=socket->connect(point->get_altip(), client_port);

		if (!result) {
			log->logEvent(info_str[NOT_CONNECTED_ALT_IP_STR], ERR, point->get_id());

			return false;
		};
	};

	return true;
}

void TClientJob::work(DWORD job_id)
{
	bool result=false;

	job=(PJob)find_object_by_id(job_id, system_queue_jobs);

	if (job==NULL) {
		log->logEvent(info_str[BAD_JOB_ID_STR], INFO, job_id);

		return;
	};

	job->init_wait_changes();

	// if error, try to repeat 3 times
	for (UINT i=1; i<=MAX_TRY; i++) {
		print_attempt(job_id, i);

		socket=new TClientSocket();

		if (!connect()) goto repeat;

		if (!do_open()) goto repeat;

		switch (job->get_action()) {
			case action::read : {
				result=do_get_file();

				break;
			};

			case action::write : {
				result=do_put_file();

				break;
			};
		};

		do_close();
repeat:
		if (socket!=NULL) delete socket;

		if (result) break;

		Sleep(TRY_PERIOD);
	};

	job->done_wait_changes();
}

bool TClientJob::do_open()
{
	frame_open open;
	frame_open_ok open_ok;
	DWORD result;

	open.header.magic.dw=magic_number.dw;
	open.header.type=frame_type::command;
	open.header.command=command_type::open;
	open.header.sessionId=0;
	open.header.size=sizeof(DWORD)*2;

	open.data.pointId=self_id;
	open.data.jobId=job->get_id();

	if (!socket->write(&open, sizeof(frame_open))) {
		log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());

		return false;
	};

    result=get_answer(socket, (char*)&open_ok, sizeof(frame_open_ok), finished);

	if (result!=ERR_NO) return false;

	if ((open_ok.header.magic.dw==magic_number.dw)&&(open_ok.header.type==frame_type::ok)&&(open_ok.header.command==command_type::open)) {
		sessionId=open_ok.header.sessionId;
		client_block_size=open_ok.data.block_size;

		buffer=new char[sizeof(frame)+client_block_size];

		if (buffer==NULL) return false;

		return true;
	};

	log->logEvent(info_str[SERVER_PROTO_ERROR_STR], ERR, PROTOCOL_PACKET_CORRUPTED);

	return false;
}

bool TClientJob::do_close()
{
	frame_close close;
	DWORD result;

	if (!socket->isConnected()) return false;

	close.header.magic.dw=magic_number.dw;
	close.header.type=frame_type::command;
	close.header.command=command_type::close;
	close.header.sessionId=sessionId;
	close.header.size=0;

	if (!socket->write(&close, sizeof(frame_close))) {
		log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());

		return false;
	};

	if (finished) return false;

    result=get_answer(socket, buffer, sizeof(frame_close_ok), finished);

	if (result!=ERR_NO) return false;

	if ((((frame_close_ok*)buffer)->header.magic.dw==magic_number.dw)&&(((frame_close_ok*)buffer)->header.type==frame_type::ok)&&(((frame_close_ok*)buffer)->header.command==command_type::close)) {
		return true;
	};

	log->logEvent(info_str[SERVER_PROTO_ERROR_STR], ERR, PROTOCOL_PACKET_CORRUPTED);

	return false;
}

bool TClientJob::do_get_file()
{
	PAObject object;
	PDQueue objects;
	PBlock	block, client_block, server_block;
	TString temp_file_path, file_path, dir_path;
	DWORD result, counter, position;
	frame_get_hash_ok get_hash_ok;
	TFile file;
	THash file_hash;
	frame_get_file_ok get_file_ok;
	TimeObject time;
	DWORD files_readed=0, first_tick, times=0;
	PString temp_str, file_str, cur_file;
	DQueue get_files("get files", false), files("transaction files", false), sync_files("sync files", false);
	PBlock file_block;
	BYTE name_length;	
	size_t filename_length;
	PFileObject fileobj;
	bool result_var=true, found_file;
	frame_delete_file_ok delete_ok;
	DWORD attr;
	//frame_clean_files clean_files;
	__int64 start_block, file_size, file_size_first, bytes_to_read, need_write, bytes_readed=0;
	frame_error frame;

	objects=job->get_objects();

	if (objects==NULL) return true;

	block=objects->head;

	while (block!=NULL) {
		if (finished) {
			log->logEvent(info_str[PROGRAM_TERMINATING_STR], ERR, PROGRAMM_TERMINATING);
			
			return true; // if shutdown - dont repeat try
		};

		object=(PAObject)block->obj;

		if (object==NULL) goto next;

		add_local_object(object);

		get_files.clear();

		try {
			// path to file
			dir_path.set_string(object->client_path->get_string());

			replace_templates(&dir_path, macros); // replace global macro

			replace_templates(&dir_path, local_macros); // replace local macro

			if (!has_trailing_slash(dir_path.get_string()))
				dir_path.add_string(slash);

			// if path not exist, create
			if (!PathFileExists(dir_path.get_string())) {
				if (!create_dir(dir_path.get_string())) {
					log->logEvent(info_str[ERROR_CREATE_DEST_DIR_STR], ERR, BAD_DESTINATION_DIR);

					if (job->get_transaction()) goto out; // return true - dont repeat try
					else goto remove_obj;
				};
			};

			if ((StrChrI(object->file->get_string(), '?')!=NULL)||(StrChrI(object->file->get_string(), '*')!=NULL)) {
				// log action and file name
				temp_file_path.set_string(object->server_path->get_string());

				replace_templates(&temp_file_path, macros); // replace global macro

				replace_templates(&temp_file_path, local_macros); // replace local macro

				if (!has_trailing_slash(temp_file_path.get_string()))
					temp_file_path.add_string(slash);

				temp_file_path.add_string(object->file->get_string());

				print_info(object->get_id(), operation_enum_files, temp_file_path.get_string());

				//packet - enum files in destination dir
				((frame_enum_files*)buffer)->header.magic.dw=magic_number.dw;
				((frame_enum_files*)buffer)->header.type=frame_type::command;
				((frame_enum_files*)buffer)->header.command=command_type::enum_files;
				((frame_enum_files*)buffer)->header.sessionId=sessionId;
				((frame_enum_files*)buffer)->header.size=sizeof(DWORD);

				((frame_enum_files*)buffer)->data.object_id=object->get_id();

				if (!socket->write(buffer, sizeof(frame)+sizeof(DWORD))) {
write_error:
					log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());

					result_var=false; // repeat try

					goto out;
				};

				while (true) { // wait for all find files packets
					result=get_answer(socket, (char*)buffer, sizeof(frame)+sizeof(bool)+sizeof(BYTE), finished);

					if (result!=ERR_NO) {
						if (result!=ERR_SHUTDOWN) result_var=false; // repeat try

						goto out;
					};

					if (!((((frame_enum_files_ok*)buffer)->header.magic.dw==magic_number.dw)&&(((frame_enum_files_ok*)buffer)->header.type==frame_type::ok)&&(((frame_enum_files_ok*)buffer)->header.command==command_type::enum_files)&&(((frame_enum_files_ok*)buffer)->header.sessionId==sessionId))) {
	protocol_error:
						log->logEvent(info_str[SERVER_PROTO_ERROR_STR], ERR, ERROR_SERVER_PROTOCOL);

						result_var=false; // repeat try

						goto out;
					};

					if ((!((frame_enum_files_ok*)buffer)->data.has_data)||(((frame_enum_files_ok*)buffer)->data.name.size==0)) break; // got last packet

					// get actual file name
					name_length=((frame_enum_files_ok*)buffer)->data.name.size;

					result=read_data(socket, buffer, name_length, finished);

					if (result!=ERR_NO) {
						if (result!=ERR_SHUTDOWN) result_var=false; // repeat try

						goto out;
					};

					buffer[name_length]='\x0';

					cur_file=new TString(buffer);

					if (cur_file!=NULL)
						get_files.addElement(cur_file, true);
				}
			}
			else get_files.addElement(object->file, false);

			print_info(object->get_id(), operation_enum_files, OK_STR);

			file_block=get_files.head;
			
			while (file_block!=NULL) {
				if (finished) {
					log->logEvent(info_str[PROGRAM_TERMINATING_STR], ERR, PROGRAMM_TERMINATING);

					goto out; // return true - dont repeat try
				};

				start_block=0;

				cur_file=(PString)file_block->obj;

				file_path.set_string(dir_path.get_string());
				file_path.add_string(*cur_file);

				// if file exist and don't need to overwrite - next
				if (PathFileExists(file_path.get_string())&&(!object->overwrite)) {
					print_info(object->get_id(), operation_get_file, cur_file->get_string());

					log->logEvent(info_str[NO_NEED_OVERWRITE_STR], WARN, DONT_NEED_OVERWRITE);

					goto next_block;
				};

				if (!PathFileExists(file_path.get_string())) {
					if (!create_dir(file_path.get_string())) {
						log->logEvent(info_str[ERROR_CREATE_DEST_DIR_STR], ERR, BAD_DESTINATION_DIR);

						if (job->get_transaction()) goto out; // return true - dont repeat try
						else goto next_block;
					};
				};

				// path to temp file
				temp_file_path.set_string(dir_recieve);

				if (!has_trailing_slash(temp_file_path.get_string()))
					temp_file_path.add_string(slash);

				// extract filename from path
				temp_file_path.add_string(find_filename(cur_file->get_string()));
				
				temp_file_path.add_string(get_point_id());

				temp_file_path.add_string(temp_ext);

				// test if file exist - get hash
				if (!PathFileExists(file_path.get_string())) goto get_file;

				// log action and file name
				print_info(object->get_id(), operation_get_hash, cur_file->get_string());

				((frame_get_hash*)buffer)->header.magic.dw=magic_number.dw;
				((frame_get_hash*)buffer)->header.type=frame_type::command;
				((frame_get_hash*)buffer)->header.command=command_type::get_hash;
				((frame_get_hash*)buffer)->header.sessionId=sessionId;

				((frame_get_hash*)buffer)->data.object_id=object->get_id();

				if (!SUCCEEDED(StringCchLength(cur_file->get_string(), MAX_PATH_LENGTH, &filename_length))) goto print_hash_err;

				((frame_get_hash*)buffer)->header.size=sizeof(DWORD)+sizeof(BYTE)+filename_length;
				((frame_get_hash*)buffer)->data.name.size=(BYTE)filename_length;

				CopyMemory(((frame_get_hash*)buffer)->data.name.filename, cur_file->get_string(), filename_length);

				if (!socket->write(buffer, sizeof(frame)+sizeof(DWORD)+sizeof(BYTE)+filename_length)) goto write_error;

				result=get_answer(socket, (char*)&get_hash_ok, sizeof(frame_get_hash_ok), finished, false);

				if (result!=ERR_NO) {
					if (result!=ERR_SHUTDOWN) result_var=false; // repeat try

					goto out;
				};

				if (!((get_hash_ok.header.magic.dw==magic_number.dw)&&(get_hash_ok.header.type==frame_type::ok)&&(get_hash_ok.header.command==command_type::get_hash)&&(get_hash_ok.header.sessionId==sessionId))) {
					goto protocol_error;
				};

				// get file hash
				if (!get_file_hash(file_path.get_string(), buffer)) {
					log->logEvent(info_str[GET_HASH_ERROR_STR], ERR, ERROR_GET_HASH);

					goto print_hash_err;
				};

				for (UINT i=0; i<MD5LEN; i++) {
					// hashes don't match - need to get file from server
					if (buffer[i]!=get_hash_ok.data.hash[i]) goto print_hash_err;
				};

				// log action and file name
				print_info(object->get_id(), operation_get_hash, OK_STR);

				// if hashes match - go out !!!
				goto post_process;

print_hash_err:
				print_info(object->get_id(), operation_get_hash, ERROR_STR);
get_file:
				// log action and file name
				print_info(object->get_id(), operation_get_file, cur_file->get_string());

				if (!PathFileExists(temp_file_path.get_string()))
					file.open(temp_file_path.get_string(), true, true, true); // write data, overwrite always, deny write
				else 
					file.open(temp_file_path.get_string(), true, false, true); // write data, open existing, deny write

				if (!file.isValid()) {
					log->logEvent(info_str[FILE_STATUS_NOT_VALID_STR], ERR, GetLastError());

					if (job->get_transaction()) {
						result_var=false; // repeat try

						goto out;
					}
					else goto next_block;
				};

				// check if we can use old recieved data
				if (file.size()>min_file_size) {
					start_block=file.size()/client_block_size;

					if (!file.set_pointer(start_block*client_block_size, FILE_BEGIN)) {
						log->logEvent(info_str[FILE_MOVE_POINTER_STR], WARN, FILE_MOVE_POINTER);

						start_block=0;

						file.set_pointer(0, FILE_BEGIN);
					};
				};

				// send packet - GET FILE
				((frame_get_file*)buffer)->header.magic.dw=magic_number.dw;
				((frame_get_file*)buffer)->header.type=frame_type::command;
				((frame_get_file*)buffer)->header.command=command_type::get_file;
				((frame_get_file*)buffer)->header.sessionId=sessionId;
				((frame_get_file*)buffer)->data.object_id=object->get_id();
				((frame_get_file*)buffer)->data.start_block=start_block;

				if (!SUCCEEDED(StringCchLength(cur_file->get_string(), MAX_PATH_LENGTH, &filename_length))) {
					if (job->get_transaction()) goto out; // dont try to repeat
					else goto next_block;
				};

				((frame_get_file*)buffer)->header.size=sizeof(__int64)+sizeof(DWORD)+sizeof(BYTE)+filename_length;
				((frame_get_file*)buffer)->data.name.size=(BYTE)filename_length;

				CopyMemory(((frame_get_file*)buffer)->data.name.filename, cur_file->get_string(), filename_length);

				if (!socket->write(buffer, sizeof(frame)+sizeof(__int64)+sizeof(DWORD)+sizeof(BYTE)+filename_length)) goto write_error;

				result=get_answer(socket, (char*)&get_file_ok, sizeof(frame_get_file_ok), finished);

				if (result!=ERR_NO) {
					if (result!=ERR_SHUTDOWN) result_var=false; // repeat try

					goto out;
				};

				if (!((get_file_ok.header.magic.dw==magic_number.dw)&&(get_file_ok.header.type==frame_type::ok)&&(get_file_ok.header.command==command_type::get_file)&&(get_file_ok.header.sessionId==sessionId))) {
					goto protocol_error;
				};

				file_size_first=get_file_ok.data.size;
				file_size=file_size_first;

				if (file_size_first==0) goto skip_read;

				file_size=file_size-(start_block*client_block_size); // adjust if not from first block

				counter=1;

				time.init(timeout);

				bytes_to_read=min(client_block_size, file_size);
				need_write=bytes_to_read;
				bytes_to_read=bytes_to_read+sizeof(frame_data);

				position=0;

				first_tick=GetTickCount();

				do {
					if (socket->read(&buffer[position], (DWORD)bytes_to_read)) {
						position=position+socket->get_read_bytes();

						bytes_to_read=bytes_to_read-socket->get_read_bytes();

						if (bytes_to_read==0) {
							if ((((frame_data*)buffer)->header.magic.dw==magic_number.dw)&&(((frame_data*)buffer)->header.type==frame_type::data)&&(((frame_data*)buffer)->header.command==counter)&&(((frame_data*)buffer)->header.sessionId==sessionId)) {
								if (!file.write(&buffer[sizeof(frame_data)], (DWORD)need_write)) {
									log->logEvent(info_str[FILE_WRITE_ERROR_STR], ERR, GetLastError());

									time.done();

									if (job->get_transaction()) {
										result_var=false; // repeat try

										goto out;
									}
									else goto next_block;
								};

								file_size=file_size-need_write;

								if (file_size<=0) break;

								bytes_to_read=min(client_block_size, file_size);
								need_write=bytes_to_read;
								bytes_to_read=bytes_to_read+sizeof(frame_data);

								position=0;

								counter++;

								time.init(timeout);

								continue;
							};

							// we got error packet
							if ((((frame_data*)buffer)->header.magic.dw==magic_number.dw)&&(((frame_data*)buffer)->header.type==frame_type::error)) {
								log->logEvent(info_str[ERROR_SERVER_PROTOCOL], ERR, ERROR_READ_FILE);

								time.done();

								if (job->get_transaction()) {
									result_var=false; // repeat try

									goto out;
								}
								else goto next_block;
							};
						};
					};

					if (!socket->isConnected()) {
						log->logEvent(info_str[SOCKET_DISCONNECTED_STR], ERR, SOCKET_DISCONNECTED);

						result_var=false; // repeat try

						goto out;
					};

					if (finished) {
						log->logEvent(info_str[PROGRAM_TERMINATING_STR], ERR, PROGRAMM_TERMINATING);
						
						goto out; // dont repeat try
					};

					if (time.timeout()) {
						log->logEvent(info_str[READ_TIMEOUT_STR], ERR, ERR_TIMEOUT);

						result_var=false; // repeat try

						goto out;
					};

					// yield some time to windows
					Sleep(0);
				} while (true);

				time.done();

				times=times+(GetTickCount()-first_tick);

				bytes_readed=bytes_readed+(get_file_ok.data.size-(start_block*client_block_size));

				files_readed++;
skip_read:
				files_count++;

				file.close();

				frame.header.magic.dw=magic_number.dw;
				frame.header.type=frame_type::ok;
				frame.header.command=command_type::get_file;
				frame.header.sessionId=sessionId;
				frame.header.size=0;

				if (file_size_first==0) goto jam;

				// CHECK if hashes match
				if (!get_file_hash(temp_file_path.get_string(), buffer)) {
					log->logEvent(info_str[GET_HASH_ERROR_STR], ERR, ERROR_GET_HASH);

					frame.header.type=frame_type::error;

					if (!socket->write(&frame, sizeof(frame_error))) goto write_error;

					if (job->get_transaction()) {
						result_var=false; // repeat try

						goto out;
					}
					else goto next_block;
				};

				for (UINT i=0; i<MD5LEN; i++) {
					// hashes don't match - need to resend file
					if (buffer[i]!=get_file_ok.data.file_hash[i]) {
						log->logEvent(info_str[HASH_MISMATCH_STR], ERR, FILE_HASH_MISMATCH);

						// delete file with bad hash !!!
						DeleteFile(temp_file_path.get_string());

						frame.header.type=frame_type::error;

						if (!socket->write(&frame, sizeof(frame_error))) goto write_error;

						if (job->get_transaction()) {
							result_var=false; // repeat try

							goto out;
						}
						else goto next_block;
					};
				};

jam:
				// all OK - send ACK
				if (!socket->write(&frame, sizeof(frame_error))) {
					log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());

					goto out;
				};

				//log action - ok
				print_info(object->get_id(), operation_get_file, OK_STR);

				if (!job->get_transaction()) {
					// if file exist - replace it
					if (!CopyFile(temp_file_path.get_string(), file_path.get_string(), false)) {
						log->logEvent(info_str[ERROR_COPY_FILE_DEST_STR], ERR, GetLastError());
					}
					else {
						SetFileAttributes(file_path.get_string(), get_file_ok.data.attributes);

						file.open(file_path.get_string(), true);

						file.set_time(&get_file_ok.data.date);

						file.close();

						// if read only, clear file attribute
						attr = GetFileAttributes(temp_file_path.get_string());
						attr = attr&~FILE_ATTRIBUTE_READONLY;

						SetFileAttributes(temp_file_path.get_string(), attr);

						// delete only if CopyFile succeed
						DeleteFile(temp_file_path.get_string());
					};

post_process:
					//do post process - delete files
					if (job->get_postprocess()==postprocess::delete_files) {
						// send packet - DELETE FILE
						print_info(object->get_id(), operation_delete_file, cur_file->get_string());

						((frame_delete_file*)buffer)->header.magic.dw=magic_number.dw;
						((frame_delete_file*)buffer)->header.type=frame_type::command;
						((frame_delete_file*)buffer)->header.command=command_type::delete_file;
						((frame_delete_file*)buffer)->header.sessionId=sessionId;
						((frame_delete_file*)buffer)->header.size=sizeof(DWORD)+sizeof(BYTE)+filename_length;

						((frame_delete_file*)buffer)->data.object_id=object->get_id();

						((frame_delete_file*)buffer)->data.name.size=(BYTE)filename_length;

						CopyMemory(((frame_delete_file*)buffer)->data.name.filename, cur_file->get_string(), filename_length);

						if (!socket->write(buffer, sizeof(frame)+sizeof(DWORD)+sizeof(BYTE)+filename_length)) {
							print_info(object->get_id(), operation_delete_file, ERROR_STR);

							goto write_error;
						};

						result=get_answer(socket, (char*)&delete_ok, sizeof(frame_delete_file_ok), finished);

						if (result!=ERR_NO) {
							if (result!=ERR_SHUTDOWN) result_var=false; // repeat try

							print_info(object->get_id(), operation_delete_file, ERROR_STR);

							goto out;
						};

						if (!((delete_ok.header.magic.dw==magic_number.dw)&&(delete_ok.header.type==frame_type::ok)&&(delete_ok.header.command==command_type::delete_file)&&(delete_ok.header.sessionId==sessionId))) {
							print_info(object->get_id(), operation_delete_file, ERROR_STR);

							goto protocol_error;
						};

						print_info(object->get_id(), operation_delete_file, OK_STR);
					};
				}
				else files.addElement(new TFileObject(temp_file_path.get_string(), file_path.get_string(), get_file_ok.data.attributes, get_file_ok.data.date, object->get_id(), cur_file->get_string()), true);
next_block:
				file_block=file_block->next;
			};
		}
		catch (...) {
		};

remove_obj:
		remove_local_object();

next:
		block=block->next;
	};

	// copy all files to destination
	if (job->get_transaction()) {
		block=files.head;

		while (block!=NULL) {
			fileobj=(PFileObject)block->obj;
				
			if (fileobj==NULL) goto move_next;

			temp_str=fileobj->temp_file;

			file_str=fileobj->file;

			if ((temp_str==NULL)||(file_str==NULL)) goto move_next;

			// log action and file name
			print_info(job->get_id(), operation_commit_transaction, find_filename(file_str->get_string()));

			if (!CopyFile(temp_str->get_string(), file_str->get_string(), false)) {
				print_info(job->get_id(), find_filename(file_str->get_string()), ERROR_STR);
			}
			else {
				print_info(job->get_id(), find_filename(file_str->get_string()), OK_STR);

				SetFileAttributes(file_str->get_string(), fileobj->attributes);

				file.open(file_str->get_string(), true);

				file.set_time(&fileobj->date);

				file.close();

				// if read only, clear file attribute
				attr=GetFileAttributes(temp_str->get_string());
				attr=attr&~FILE_ATTRIBUTE_READONLY;

				SetFileAttributes(temp_str->get_string(), attr);

				// delete only if CopyFile succeed
				DeleteFile(temp_str->get_string());
			};

			cur_file=fileobj->cur_file;

			if (cur_file==NULL) goto move_next;

			if (job->get_postprocess()==postprocess::delete_files) {
				// send packet - DELETE FILE
				print_info(fileobj->objectId, operation_delete_file, cur_file->get_string());

				((frame_delete_file*)buffer)->header.magic.dw=magic_number.dw;
				((frame_delete_file*)buffer)->header.type=frame_type::command;
				((frame_delete_file*)buffer)->header.command=command_type::delete_file;
				((frame_delete_file*)buffer)->header.sessionId=sessionId;

				((frame_delete_file*)buffer)->data.object_id=fileobj->objectId;

				if (!SUCCEEDED(StringCchLength(cur_file->get_string(), MAX_PATH_LENGTH, &filename_length))) goto move_next;

				((frame_delete_file*)buffer)->header.size=sizeof(DWORD)+sizeof(BYTE)+filename_length;

				((frame_delete_file*)buffer)->data.name.size=(BYTE)filename_length;

				CopyMemory(((frame_delete_file*)buffer)->data.name.filename, cur_file->get_string(), filename_length);

				if (!socket->write(buffer, sizeof(frame)+sizeof(DWORD)+sizeof(BYTE)+filename_length)) {
					print_info(fileobj->objectId, operation_delete_file, ERROR_STR);

					result_var=false; // repeat try

					goto out;
				};

				result=get_answer(socket, (char*)&delete_ok, sizeof(frame_delete_file_ok), finished);

				if (result!=ERR_NO) {
					if (result!=ERR_SHUTDOWN) result_var=false; // repeat try

					print_info(fileobj->objectId, operation_delete_file, ERROR_STR);

					goto out;
				};

				if (!((delete_ok.header.magic.dw==magic_number.dw)&&(delete_ok.header.type==frame_type::ok)&&(delete_ok.header.command==command_type::delete_file)&&(delete_ok.header.sessionId==sessionId))) {
					print_info(fileobj->objectId, operation_delete_file, ERROR_STR);

					goto move_next;
				};

				print_info(fileobj->objectId, operation_delete_file, OK_STR);
			};

move_next:
			block=block->next;
		};
	};

	//do post process - SYNC files
	if (job->get_postprocess()==postprocess::sync_files) {
		objects=job->get_objects();

		if (objects==NULL) return true;

		block=objects->head;

		while (block!=NULL) {
			if (finished) {
				log->logEvent(info_str[PROGRAM_TERMINATING_STR], ERR, PROGRAMM_TERMINATING);
			
				return true; // if shutdown - dont repeat try
			};

			object=(PAObject)block->obj;

			if (object==NULL) goto next_step;

			add_local_object(object);

			get_files.clear();

			files.clear();

			try {
				// path to file
				dir_path.set_string(object->client_path->get_string());

				replace_templates(&dir_path, macros); // replace global macro

				replace_templates(&dir_path, local_macros); // replace local macro

				if (!has_trailing_slash(dir_path.get_string()))
					dir_path.add_string(slash);

				// if path not exist, create
				if (!PathFileExists(dir_path.get_string())) {
					log->logEvent(info_str[ERROR_CREATE_DEST_DIR_STR], ERR, BAD_DESTINATION_DIR);

					goto out;
				};

				if ((StrChrI(object->file->get_string(), '?')!=NULL)||(StrChrI(object->file->get_string(), '*')!=NULL)) {
					// log action and file name
					temp_file_path.set_string(object->server_path->get_string());

					replace_templates(&temp_file_path, macros); // replace global macro

					replace_templates(&temp_file_path, local_macros); // replace local macro

					if (!has_trailing_slash(temp_file_path.get_string()))
						temp_file_path.add_string(slash);

					temp_file_path.add_string(object->file->get_string());

					//packet - enum files in destination dir
					((frame_enum_files*)buffer)->header.magic.dw=magic_number.dw;
					((frame_enum_files*)buffer)->header.type=frame_type::command;
					((frame_enum_files*)buffer)->header.command=command_type::enum_files;
					((frame_enum_files*)buffer)->header.sessionId=sessionId;
					((frame_enum_files*)buffer)->header.size=sizeof(DWORD);

					((frame_enum_files*)buffer)->data.object_id=object->get_id();

					if (!socket->write(buffer, sizeof(frame)+sizeof(DWORD))) {
						log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());

						result_var=false; // repeat try

						goto out;
					};

					while (true) {
						result=get_answer(socket, (char*)buffer, sizeof(frame)+sizeof(bool)+sizeof(BYTE), finished);

						if (result!=ERR_NO) {
							if (result!=ERR_SHUTDOWN) result_var=false; // repeat try

							goto out;
						};

						if (!((((frame_enum_files_ok*)buffer)->header.magic.dw==magic_number.dw)&&(((frame_enum_files_ok*)buffer)->header.type==frame_type::ok)&&(((frame_enum_files_ok*)buffer)->header.command==command_type::enum_files)&&(((frame_enum_files_ok*)buffer)->header.sessionId==sessionId))) {
							log->logEvent(info_str[SERVER_PROTO_ERROR_STR], ERR, ERROR_SERVER_PROTOCOL);

							result_var=false; // repeat try

							goto out;
						};

						if ((!((frame_enum_files_ok*)buffer)->data.has_data)||(((frame_enum_files_ok*)buffer)->data.name.size==0)) break;

						// get actual file name
						name_length=((frame_enum_files_ok*)buffer)->data.name.size;

						result=read_data(socket, buffer, name_length, finished);

						if (result!=ERR_NO) {
							if (result!=ERR_SHUTDOWN) result_var=false; // repeat try

							goto out;
						};

						buffer[name_length]='\x0';

						cur_file=new TString(buffer);

						if (cur_file!=NULL) get_files.addElement(cur_file, true);
					};
				}
				else get_files.addElement(object->file, true);

				sync_files.clear();

				recurse_level=0;

				result=file_search(dir_path.get_string(), NULL, object->file->get_string(), &sync_files, object->recursive, recurse_level);

				if (result>0) {
					if (result==100) log->logEvent(info_str[FILE_NOT_FOUND_STR], ERR, FILE_NOT_FOUND);
					else log->logEvent(info_str[FIND_FILE_ERR_STR], ERR, FIND_FILE_ERROR);

					goto out;
				};

				// compare two lists, make client files that not match
				client_block=sync_files.head;

				while (client_block!=NULL) {
					found_file=false;

					server_block=get_files.head;

					while (server_block!=NULL) {
						if (StrCmpI(((PString)client_block->obj)->get_string(), ((PString)server_block->obj)->get_string())==0) {
							found_file=true;

							break;
						};

						server_block=server_block->next;
					};

					if (!found_file) files.addElement(new TString(((PString)client_block->obj)->get_string()), true);
					
					client_block=client_block->next;
				};

				// delete found files
				log->logEvent(info_str[SYNC_FILES_STARTED_STR], INFO);


				client_block=files.head;

				while (client_block!=NULL) {
					file_path.set_string(dir_path.get_string());

					file_path.add_string(PString(client_block->obj)->get_string());

					// if read only, clear file attribute
					attr=GetFileAttributes(file_path.get_string());
					attr=attr&~FILE_ATTRIBUTE_READONLY;

					SetFileAttributes(file_path.get_string(), attr);

					if (DeleteFile(file_path.get_string())!=FALSE) {
						print_info(object->get_id(), operation_delete_file, PString(client_block->obj)->get_string());
						print_info(object->get_id(), operation_delete_file, OK_STR);
					};

					client_block=client_block->next;
				};

				log->logEvent(info_str[SYNC_FILES_STOPPED_STR], INFO);
			}
			catch (...) {
			};

			remove_local_object();

next_step:
			block=block->next;
		};
	};

	// clean server !!! dir from ANY files
	if (job->get_postprocess()==postprocess::delete_files) {
//		objects=job->get_objects();
//
//		if (objects==NULL) return true;
//
//		block=objects->head;
//
//		while (block!=NULL) {
//			object=(PAObject)block->obj;
//
//			if (object==NULL) goto next_step_del;
//
//			if (object->get_error()==true) goto next_step_del; // if has error - don't del object files
//
//			add_local_object(object);
//
//			try {
//				clean_files.header.magic.dw=magic_number.dw;
//
//				clean_files.header.type=frame_type::command;
//				clean_files.header.command=command_type::clean_files;
//				clean_files.header.sessionId=sessionId;
//				clean_files.header.size=sizeof(files_clean);
//
//				clean_files.data.object_id=object->get_id();
//
//				if (!socket->write(&clean_files, sizeof(clean_files))) {
//					log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());
//
//					result_var=false; // repeat try
//
//					goto out;
//				};
//
//				result=get_answer(socket, (char*)buffer, sizeof(frame), finished);
//
//				if (result!=ERR_NO) {
//					if (result!=ERR_SHUTDOWN) result_var=false; // repeat try
//
//					goto out;
//				};
//
//				if (!((((frame_clean_files_ok*)buffer)->header.magic.dw==magic_number.dw)&&(((frame_clean_files_ok*)buffer)->header.type==frame_type::ok)&&(((frame_clean_files_ok*)buffer)->header.command==command_type::clean_files)&&(((frame_clean_files_ok*)buffer)->header.sessionId==sessionId))) {
//					log->logEvent(info_str[SERVER_PROTO_ERROR_STR], ERR, ERROR_SERVER_PROTOCOL);
//
//					result_var=false; // repeat try
//
//					goto out;
//				};
//
//				// debug info print
//				file_path.set_string(object->server_path->get_string());
//
//				replace_templates(&file_path, macros);
//
//				replace_templates(&file_path, local_macros);
//
//				print_info(object->get_id(), operation_clean_files, file_path.get_string());
//			}
//			catch (...) {
//			};
//
//			remove_local_object();
//
//next_step_del:
//			block=block->next;
//		};
	};

	//debug summary
	if (files_readed>0) {
		//debug print action
		print_summary(files_readed, (DWORD)bytes_readed, times);
	};

	return true;

out:
	remove_local_object();

	return result_var;
}

bool TClientJob::do_put_file()
{
	PAObject object;
	PDQueue objects;
	PBlock	block, file_block, client_block, server_block;
	TString file_path, dir_path;
	PString cur_file;
	TFile file;
	BYTE size;
	DWORD result, counter, recurse_level;
	DQueue put_files("put files", false), postprocess_files("postprocess files", false), sync_files("sync_files", false);
	DWORD files_writed=0, first_tick, times=0;
	size_t filename_length;
	frame_put_file_ok put_file;
	frame_get_hash_ok get_hash_ok;
	frame_delete_file_ok delete_ok;
	frame_error frame;
	bool result_var=true;
	BYTE name_length;
	boolean found_file;
	DWORD attr;
	PFileStruct file_to_del;
	__int64 start_block=0, pages, remainder, bytes_writed=0;
	hash_array file_hash;

	objects=job->get_objects();

	if (objects==NULL) return true;

	if (job->get_transaction()) {
		((frame_set_transaction*)buffer)->header.magic.dw=magic_number.dw;
		((frame_set_transaction*)buffer)->header.sessionId=sessionId;
		((frame_set_transaction*)buffer)->header.type=frame_type::command;
		((frame_set_transaction*)buffer)->header.command=command_type::transaction;
		((frame_set_transaction*)buffer)->header.size=sizeof(DWORD);

		((frame_set_transaction*)buffer)->data.value=transaction_state::open_transaction;

		if (!socket->write(buffer, sizeof(frame_set_transaction))) {
			log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());

			return false; // try to repeat
		};

		result=get_answer(socket, (char*)buffer, sizeof(frame_set_transaction_ok), finished);

		if (result!=ERR_NO) {
			if (result!=ERR_SHUTDOWN) return false; // try to repeat

			return true; // dont try to repeat
		};

		// log action and file name
		print_info(job->get_id(), operation_open_transaction);
	};

	block=objects->head;

	while (block!=NULL) {
		if (finished) {
			log->logEvent(info_str[PROGRAM_TERMINATING_STR], ERR, PROGRAMM_TERMINATING);

			return true; // dont try to repeat
		};

		object=(PAObject)block->obj;

		if (object==NULL) goto next;

		object->clear_error(); // clear error state

		put_files.clear();

		add_local_object(object);

		try {
			// path to file
			dir_path.set_string(object->client_path->get_string());

			replace_templates(&dir_path, macros); // replace global macro

			replace_templates(&dir_path, local_macros); // replace local macro

			if (!has_trailing_slash(dir_path.get_string()))
				dir_path.add_string(slash);

			if ((StrChrI(object->file->get_string(), '?')!=NULL)||(StrChrI(object->file->get_string(), '*')!=NULL)) {
				recurse_level=0;

				result=file_search(dir_path.get_string(), NULL, object->file->get_string(), &put_files, object->recursive, recurse_level);

				if (result>0) {
					object->set_error();

					if (result==100) log->logEvent(info_str[FILE_NOT_FOUND_STR], ERR, FILE_NOT_FOUND);
					else log->logEvent(info_str[FIND_FILE_ERR_STR], ERR, FIND_FILE_ERROR);

					if (job->get_transaction()) goto out; // true- dont try to repeat
					else goto remove_obj;
				};
			}
			else put_files.addElement(object->file, false);

			file_block=put_files.head;

			while (file_block!=NULL) {
				if (finished) {
					log->logEvent(info_str[PROGRAM_TERMINATING_STR], ERR, PROGRAMM_TERMINATING);

					result_var=true; // dont try to repeat

					goto out;
				};

				cur_file=(PString)file_block->obj;

				if (cur_file==NULL) goto next_block;

				file_path.set_string(dir_path.get_string());
				file_path.add_string(*cur_file);

				if (!PathFileExists(file_path.get_string())) {
					object->set_error();

					log->logEvent(info_str[FILE_NOT_FOUND_STR], ERR, FILE_NOT_FOUND);

					if (job->get_transaction()) {
						result_var=false; // try to repeat

						goto out;
					}
					else goto next_block;
				};

				if (!get_file_hash(file_path.get_string(), file_hash)) {
					object->set_error();

					log->logEvent(info_str[GET_HASH_ERROR_STR], ERR, ERROR_GET_HASH);

					if (job->get_transaction()) {
						result_var=false; // try to repeat

						goto out;
					}
					else goto next_block;
				};

				// log action and file name
				print_info(object->get_id(), operation_get_hash, cur_file->get_string());

				((frame_get_hash*)buffer)->header.magic.dw=magic_number.dw;
				((frame_get_hash*)buffer)->header.type=frame_type::command;
				((frame_get_hash*)buffer)->header.command=command_type::get_hash;
				((frame_get_hash*)buffer)->header.sessionId=sessionId;

				((frame_get_hash*)buffer)->data.object_id=object->get_id();

				if (!SUCCEEDED(StringCchLength(cur_file->get_string(), MAX_PATH_LENGTH, &filename_length))) {
					object->set_error();

					log->logEvent(info_str[INTERNAL_ERROR_STR], ERR, INTERNAL_ERROR);

					if (job->get_transaction()) {
						result_var=false; // try to repeat

						goto out;
					}
					else goto next_block;
				};

				((frame_get_hash*)buffer)->header.size=sizeof(DWORD)+sizeof(BYTE)+filename_length;
				((frame_get_hash*)buffer)->data.name.size=(BYTE)filename_length;

				CopyMemory(((frame_get_hash*)buffer)->data.name.filename, cur_file->get_string(), filename_length);

				if (!socket->write(buffer, sizeof(frame)+sizeof(DWORD)+sizeof(BYTE)+filename_length)) {
write_error:
					log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());

					result_var=false; // try to repeat

					goto socket_error;
				};

				result=get_answer(socket, (char*)&get_hash_ok, sizeof(frame_get_hash_ok), finished, false);

				if (result!=ERR_NO) {
					if (result==FILE_NOT_FOUND) goto print_hash_err;

					object->set_error();

					if (job->get_transaction()) {
						result_var=false; // try to repeat

						goto out;
					}
					else goto next_block;
				};

				if (!((get_hash_ok.header.magic.dw==magic_number.dw)&&(get_hash_ok.header.type==frame_type::ok)&&(get_hash_ok.header.command==command_type::get_hash)&&(get_hash_ok.header.sessionId==sessionId))) {
protocol_error:
					log->logEvent(info_str[SERVER_PROTO_ERROR_STR], ERR, ERROR_SERVER_PROTOCOL);

					result_var=false; // try to repeat

					goto out;
				};

				for (UINT i=0; i<MD5LEN; i++) {
					// hashes don't match - need to get file from server
					if (file_hash[i]!=get_hash_ok.data.hash[i]) goto print_hash_err;
				};

				// log action and file name
				print_info(object->get_id(), operation_get_hash, OK_STR);

				// if hashes match - go out !!!
				goto post_process;

print_hash_err:
				print_info(object->get_id(), operation_get_hash, ERROR_STR);

				//log action and file name
				print_info(object->get_id(), operation_put_file, cur_file->get_string());

				file.open(file_path.get_string()); // read, fail if not exist, allow read&write

				if (!file.isValid()) {
					object->set_error();

					log->logEvent(info_str[FILE_STATUS_NOT_VALID_STR], ERR, GetLastError());

					if (job->get_transaction()) {
						result_var=false; // try to repeat

						goto out;
					}
					else goto next_block;
				};

				size=(BYTE)cur_file->length();

				((frame_put_file*)buffer)->header.magic.dw=magic_number.dw;
				((frame_put_file*)buffer)->header.sessionId=sessionId;
				((frame_put_file*)buffer)->header.type=frame_type::command;
				((frame_put_file*)buffer)->header.command=command_type::put_file;
				((frame_put_file*)buffer)->header.size=sizeof(DWORD)+sizeof(__int64)+sizeof(DWORD)+sizeof(FILETIME)+sizeof(hash_array)+sizeof(BYTE)+size;

				((frame_put_file*)buffer)->data.object_id=object->get_id();
				((frame_put_file*)buffer)->data.size=file.size();
				((frame_put_file*)buffer)->data.attributes=GetFileAttributes(file_path.get_string());

				GetFileTime(file.getHandle(), NULL, NULL, &((frame_put_file*)buffer)->data.date);

				CopyMemory(((frame_put_file*)buffer)->data.file_hash, file_hash, sizeof(hash_array));

				((frame_put_file*)buffer)->data.name.size=size;

				CopyMemory(((frame_put_file*)buffer)->data.name.filename, cur_file->get_string(), size);

				if (!socket->write(buffer, sizeof(frame)+sizeof(DWORD)+sizeof(__int64)+sizeof(DWORD)+sizeof(FILETIME)+sizeof(hash_array)+sizeof(BYTE)+size)) goto write_error;

				result=get_answer(socket, (char*)&put_file, sizeof(frame_put_file_ok), finished);

				if (result!=ERR_NO) {
					if (result==DONT_NEED_OVERWRITE) goto next_block;

					if (result!=ERR_SHUTDOWN) result_var=false; // try to repeat

					goto out;
				};

				if (!((put_file.header.magic.dw==magic_number.dw)&&(put_file.header.type==frame_type::ok)&&(put_file.header.command==command_type::put_file)&&(put_file.header.sessionId==sessionId))) {
					goto protocol_error;
				};

				if (file.size()==0) goto skip_read;

				start_block=put_file.data.start_block;

				if (!file.set_pointer(start_block*client_block_size, FILE_BEGIN) ) {
					object->set_error();

					log->logEvent(info_str[FILE_MOVE_POINTER_STR], WARN, FILE_MOVE_POINTER);

					if (job->get_transaction()) {
						result_var=false; // try to repeat

						goto out;
					}
					else goto next_block;
				};

				// file sends in client_block_size bytes
				counter=1;

				((frame_data*)buffer)->header.magic.dw=magic_number.dw;
				((frame_data*)buffer)->header.type=frame_type::data;
				((frame_data*)buffer)->header.command=counter;
				((frame_data*)buffer)->header.sessionId=sessionId;
				((frame_data*)buffer)->header.size=client_block_size;

				pages=file.size()/client_block_size;

				first_tick=GetTickCount();

				for (__int64 i=start_block; i<pages; i++) {
					if (finished) {
						log->logEvent(info_str[PROGRAM_TERMINATING_STR], ERR, PROGRAMM_TERMINATING);

						result_var=true; // dont try to repeat

						goto out;
					};

					if (!file.read(&buffer[sizeof(frame_data)], client_block_size)) {
						object->set_error();

						log->logEvent(info_str[FILE_READ_ERROR_STR], ERR, GetLastError());

						if (job->get_transaction()) {
							result_var=false; // try to repeat

							goto out;
						}
						else {
							((frame_data*)buffer)->header.magic.dw=magic_number.dw;
							((frame_data*)buffer)->header.type=frame_type::error;
							((frame_data*)buffer)->header.command=ERROR_READ_FILE;
							((frame_data*)buffer)->header.sessionId=sessionId;
							((frame_data*)buffer)->header.size=0;

							if (!socket->write(buffer, sizeof(frame_data))) goto write_error;

							goto next_block; // break transmition on server side
						};
					};

					if (!socket->write(buffer, sizeof(frame_data)+client_block_size)) goto write_error;
						
					counter++;

					((frame_data*)buffer)->header.command=counter;

					Sleep(0);
				};

				remainder=file.size()-(pages*client_block_size);

				if (remainder>0) {
					((frame_data*)buffer)->header.size=(DWORD)remainder;

					if (!file.read(&buffer[sizeof(frame_data)], (DWORD)remainder)) {
						object->set_error();

						log->logEvent(info_str[FILE_READ_ERROR_STR], ERR, GetLastError());

						if (job->get_transaction()) {
							result_var=false; // try to repeat

							goto out;
						}
						else {
							((frame_data*)buffer)->header.magic.dw=magic_number.dw;
							((frame_data*)buffer)->header.type=frame_type::error;
							((frame_data*)buffer)->header.command=ERROR_READ_FILE;
							((frame_data*)buffer)->header.sessionId=sessionId;
							((frame_data*)buffer)->header.size=0;

							if (!socket->write(buffer, sizeof(frame_data))) goto write_error;

							goto next_block; // break transmition on server side
						};
					};

					if (!socket->write(buffer, (DWORD)(sizeof(frame_data)+remainder))) goto write_error;
				};

				times=times+(GetTickCount()-first_tick);

				bytes_writed=bytes_writed+(file.size()-(start_block*client_block_size));
				files_writed++;
skip_read:
				files_count++;

				file.close();

				// MUST get ack from server
				result=get_answer(socket, (char*)&frame, sizeof(frame_error), finished);

				if (result!=ERR_NO) {
					object->set_error();

					if (result!=ERR_SHUTDOWN) {
						result_var=false; // try to repeat

						object->set_error();
					};

					//log action - ok
					print_info(object->get_id(), operation_put_file, PEER_ERROR);

					goto out;
				};

				//log action - ok
				print_info(object->get_id(), operation_put_file, OK_STR);

post_process:
				// do postprocess
				if (!job->get_transaction()) {
					if (job->get_postprocess()==postprocess::delete_files) {
						//postprocess - delete
						print_info(object->get_id(), operation_delete_file, cur_file->get_string());

						// if read only, clear file attribute
						attr=GetFileAttributes(file_path.get_string());
						attr=attr&~FILE_ATTRIBUTE_READONLY;

						SetFileAttributes(file_path.get_string(), attr);

						if (DeleteFile(file_path.get_string())!=FALSE) print_info(object->get_id(), operation_delete_file, OK_STR);
						else print_info(object->get_id(), operation_delete_file, ERROR_STR);
					};
				}
				else {
					if (job->get_postprocess()==postprocess::delete_files) postprocess_files.addElement(new TString(file_path.get_string()), true);
				};
				

next_block:
				file_block=file_block->next;
			};
		}
		catch (...) {
		};

remove_obj:
		remove_local_object();

next:
		block=block->next;
	};

	//debug summary
	if (files_writed>0) {
		//debug print action
		print_summary(files_writed, (DWORD)bytes_writed, times);
	};

	if (job->get_transaction()) {
		((frame_set_transaction*)buffer)->header.magic.dw=magic_number.dw;
		((frame_set_transaction*)buffer)->header.sessionId=sessionId;
		((frame_set_transaction*)buffer)->header.type=frame_type::command;
		((frame_set_transaction*)buffer)->header.command=command_type::transaction;
		((frame_set_transaction*)buffer)->header.size=sizeof(DWORD);

		((frame_set_transaction*)buffer)->data.value=transaction_state::commit_transaction;

		if (!socket->write(buffer, sizeof(frame_set_transaction))) {
			log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());

			return false; // try to repeat
		};

		result=get_answer(socket, (char*)buffer, sizeof(frame_set_transaction_ok), finished);

		if (result!=ERR_NO) {
			if (result!=ERR_SHUTDOWN) return false;

			return true;
		};

		// log action and file name
		print_info(job->get_id(), operation_close_transaction);

		//postprocess - delete
		if (job->get_postprocess()==postprocess::delete_files) {
			block=postprocess_files.head;

			while (block!=NULL) {
				cur_file=(PString)block->obj;

				if (cur_file==NULL) goto skip_obj;

				print_info(job->get_id(), operation_delete_file, find_filename(cur_file->get_string()));

				// if read only, clear file attribute
				attr=GetFileAttributes(cur_file->get_string());
				attr=attr&~FILE_ATTRIBUTE_READONLY;

				SetFileAttributes(cur_file->get_string(), attr);

				if (DeleteFile(cur_file->get_string())!=FALSE) print_info(job->get_id(), operation_delete_file, OK_STR);
				else print_info(job->get_id(), operation_delete_file, ERROR_STR);

skip_obj:
				block=block->next;
			};
		};
	};

	//do post process - SYNC files
	if (job->get_postprocess()==postprocess::sync_files) {
		objects=job->get_objects();

		if (objects==NULL) return true;

		block=objects->head;

		while (block!=NULL) {
			object=(PAObject)block->obj;

			if (object==NULL) goto next_step;

			put_files.clear();

			postprocess_files.clear();

			add_local_object(object);

			try {
				// path to file
				dir_path.set_string(object->client_path->get_string());

				replace_templates(&dir_path, macros); // replace global macro

				replace_templates(&dir_path, local_macros); // replace local macro

				if (!has_trailing_slash(dir_path.get_string()))
					dir_path.add_string(slash);

				if ((StrChrI(object->file->get_string(), '?')!=NULL)||(StrChrI(object->file->get_string(), '*')!=NULL)) {
					recurse_level=0;

					result=file_search(dir_path.get_string(), NULL, object->file->get_string(), &put_files, object->recursive, recurse_level);

					if (result>0) {
						if (result==100) log->logEvent(info_str[FILE_NOT_FOUND_STR], ERR, FILE_NOT_FOUND);
						else log->logEvent(info_str[FIND_FILE_ERR_STR], ERR, FIND_FILE_ERROR);

						goto socket_error; // true- dont try to repeat
					};
				};

				sync_files.clear();

				//packet - enum files in destination dir
				((frame_enum_files*)buffer)->header.magic.dw=magic_number.dw;
				((frame_enum_files*)buffer)->header.type=frame_type::command;
				((frame_enum_files*)buffer)->header.command=command_type::enum_files;
				((frame_enum_files*)buffer)->header.sessionId=sessionId;
				((frame_enum_files*)buffer)->header.size=sizeof(DWORD);

				((frame_enum_files*)buffer)->data.object_id=object->get_id();

				if (!socket->write(buffer, sizeof(frame)+sizeof(DWORD))) {
					log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());

					result_var=false; // repeat try

					goto socket_error;
				};

				while (true) {
					result=get_answer(socket, (char*)buffer, sizeof(frame)+sizeof(bool)+sizeof(BYTE), finished);

					if (result!=ERR_NO) {
						if (result!=ERR_SHUTDOWN) result_var=false; // repeat try

						goto socket_error;
					};

					if (!((((frame_enum_files_ok*)buffer)->header.magic.dw==magic_number.dw)&&(((frame_enum_files_ok*)buffer)->header.type==frame_type::ok)&&(((frame_enum_files_ok*)buffer)->header.command==command_type::enum_files)&&(((frame_enum_files_ok*)buffer)->header.sessionId==sessionId))) {
						log->logEvent(info_str[SERVER_PROTO_ERROR_STR], ERR, ERROR_SERVER_PROTOCOL);

						result_var=false; // repeat try

						goto socket_error;
					};

					if ((!((frame_enum_files_ok*)buffer)->data.has_data)||(((frame_enum_files_ok*)buffer)->data.name.size==0)) break;

					// get actual file name
					name_length=((frame_enum_files_ok*)buffer)->data.name.size;

					result=read_data(socket, buffer, name_length, finished);

					if (result!=ERR_NO) {
						if (result!=ERR_SHUTDOWN) result_var=false; // repeat try

						goto socket_error;
					};

					buffer[name_length]='\x0';

					cur_file=new TString(buffer);

					if (cur_file!=NULL) sync_files.addElement(cur_file, true);
				};

				// compare two lists, make client files that not match
				server_block=sync_files.head;

				while (server_block!=NULL) {
					found_file=false;

					client_block=put_files.head;

					while (client_block!=NULL) {
						if (StrCmpI(((PString)client_block->obj)->get_string(), ((PString)server_block->obj)->get_string())==0) {
							found_file=true;

							break;
						};

						client_block=client_block->next;
					};

					if (!found_file) postprocess_files.addElement(new TString(((PString)server_block->obj)->get_string()), true);
					
					server_block=server_block->next;
				};

				// delete found files
				log->logEvent(info_str[SYNC_FILES_STARTED_STR], INFO);

				server_block=postprocess_files.head;

				while (server_block!=NULL) {
					file_path.set_string(dir_path.get_string());

					file_path.add_string(PString(server_block->obj)->get_string());

					// send packet - DELETE FILE
					if (!SUCCEEDED(StringCchLength(PString(server_block->obj)->get_string(), MAX_PATH_LENGTH, &filename_length))) goto socket_error;

					((frame_delete_file*)buffer)->header.magic.dw=magic_number.dw;
					((frame_delete_file*)buffer)->header.type=frame_type::command;
					((frame_delete_file*)buffer)->header.command=command_type::delete_file;
					((frame_delete_file*)buffer)->header.sessionId=sessionId;
					((frame_delete_file*)buffer)->header.size=sizeof(DWORD)+sizeof(BYTE)+filename_length;

					((frame_delete_file*)buffer)->data.object_id=object->get_id();

					((frame_delete_file*)buffer)->data.name.size=(BYTE)filename_length;

					CopyMemory(((frame_delete_file*)buffer)->data.name.filename, PString(server_block->obj)->get_string(), filename_length);

					if (!socket->write(buffer, sizeof(frame)+sizeof(DWORD)+sizeof(BYTE)+filename_length)) {
						goto socket_error;
					};

					result=get_answer(socket, (char*)&delete_ok, sizeof(frame_delete_file_ok), finished);

					if (result!=ERR_NO) {
						if (result!=ERR_SHUTDOWN) result_var=false; // repeat try

						goto socket_error;
					};

					if (!((delete_ok.header.magic.dw==magic_number.dw)&&(delete_ok.header.type==frame_type::ok)&&(delete_ok.header.command==command_type::delete_file)&&(delete_ok.header.sessionId==sessionId))) {
						goto socket_error;
					};

					print_info(object->get_id(), operation_delete_file, PString(server_block->obj)->get_string());
					print_info(object->get_id(), operation_delete_file, OK_STR);

					server_block=server_block->next;
				};

				log->logEvent(info_str[SYNC_FILES_STOPPED_STR], INFO);
			}
			catch (...) {
			}

			remove_local_object();
next_step:
			block=block->next;
		};
	};

	// clean dir from ANY files
	if (job->get_postprocess()==postprocess::delete_files) {
		objects=job->get_objects();

		if (objects==NULL) return true;

		block=objects->head;

		while (block!=NULL) {
			object=(PAObject)block->obj;

			if (object==NULL) goto next_step_del;

			if (object->get_error()==true) goto next_step_del; // if has error - don't del object files

			if (!object->has_no_changes()) { // detect file added when sending files
				print_info(object->get_id(), operation_clean_files_cancel, dir_path.get_string());

				goto next_step_del;
			};

			add_local_object(object);

			try {
				// path to file
				dir_path.set_string(object->client_path->get_string());

				replace_templates(&dir_path, macros); // replace global macro

				replace_templates(&dir_path, local_macros); // replace local macro

				if (!has_trailing_slash(dir_path.get_string()))
					dir_path.add_string(slash);

				postprocess_files.clear();

				// debug info print
				print_info(object->get_id(), operation_clean_files, dir_path.get_string());

				recurse_level=0;

				result=file_del(dir_path.get_string(), NULL, &postprocess_files, recurse_level);

				if (result==0) {
					file_block=postprocess_files.head;

					while (file_block!=NULL) {
						file_to_del=(PFileStruct)file_block->obj;

						if (file_to_del!=NULL) {
							// if read only, clear file attribute
							attr=GetFileAttributes(file_to_del->path->get_string());
							attr=attr&~FILE_ATTRIBUTE_READONLY;

							SetFileAttributes(file_to_del->path->get_string(), attr);

							if (file_to_del->type==object_type::TYPE_FILE) {
								DeleteFile(file_to_del->path->get_string());

								print_info(object->get_id(), operation_delete_file, file_to_del->path->get_string());
							}
							else {
								RemoveDirectory(file_to_del->path->get_string());

								print_info(object->get_id(), operation_delete_dir, file_to_del->path->get_string());
							};
						};

						file_block=file_block->next;
					};
				};


			}
			catch (...) {
			};

			remove_local_object();

next_step_del:
			block=block->next;
		};
	};

	return true;

out:
	if (job->get_transaction()) {
		((frame_set_transaction*)buffer)->header.magic.dw=magic_number.dw;
		((frame_set_transaction*)buffer)->header.sessionId=sessionId;
		((frame_set_transaction*)buffer)->header.type=frame_type::command;
		((frame_set_transaction*)buffer)->header.command=command_type::transaction;
		((frame_set_transaction*)buffer)->header.size=sizeof(DWORD);

		((frame_set_transaction*)buffer)->data.value=transaction_state::undo_transaction;

		if (!socket->write(buffer, sizeof(frame_set_transaction))) {
			log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());

			if (!finished) result_var=false; // try to repeat
			
			goto socket_error;
		};

		if (finished) goto socket_error; // dont try to repeat

		result=get_answer(socket, (char*)buffer, sizeof(frame_set_transaction_ok), finished);

		if (result!=ERR_NO) {
			result_var=false;

			goto socket_error;
		};

		// log action and file name
		print_info(job->get_id(), operation_undo_transaction);
	};

socket_error:
	remove_local_object();

	return result_var;
}

void TClientJob::add_local_object(PAObject object)
{
	PMacro macro;
	char buffer[MAX_DIGITS];

	// %ObjectName%
	macro=new TMacro(macro_object_name, object->get_string());

	local_macros->addElement(macro, true);

	// %ObjectId%
	StringCbPrintf(buffer, sizeof(buffer), "%03d", object->get_id());

	macro=new TMacro(macro_object_id, buffer);

	local_macros->addElement(macro, true);
}

void TClientJob::remove_local_object()
{
	PBlock block, old_block;
	PMacro macro;

	block=local_macros->head;

	while (block!=NULL) {
		macro=(PMacro)block->obj;

		old_block=block;

		block=block->next;

		if (macro!=NULL) {
			if ((StrCmpI(macro->get_hash(), macro_object_name)==0)||(StrCmpI(macro->get_hash(), macro_object_id)==0)) {
				local_macros->removeElement(old_block);
			};
		};
	};
}

void TClientJob::print_info(DWORD id, char* operation, char* file)
{
	char buffer[MAX_PATH_LENGTH*2];

	if (file!=NULL) {
		if (SUCCEEDED(StringCbPrintf(buffer, sizeof(buffer), info_string, id, operation, file))) {
			log->logEvent(buffer, INFO, 0);
		}
	}
	else {
		if (SUCCEEDED(StringCbPrintf(buffer, sizeof(buffer), info_string_short, id, operation))) {
			log->logEvent(buffer, INFO, 0);
		};
	};
}

void TClientJob::print_summary(DWORD files, __int64 bytes, DWORD msec)
{
	char buffer[MAX_PATH_LENGTH*2];

	if (SUCCEEDED(StringCbPrintf(buffer, sizeof(buffer), summary_string, files, bytes, ((double)msec/1000)))) {
		log->logEvent(buffer, INFO, 0);
	};
}

char* TClientJob::get_point_id()
{
	PBlock block;
	PMacro macro;

	block=local_macros->head;

	while (block!=NULL) {
		macro=(PMacro)block->obj;

		if (macro!=NULL) {
			if (StrCmpI(macro->get_hash(), macro_point_id)==0) {
				return macro->get_value();
			};
		};

		block=block->next;
	};

	return NULL;
}

void TClientJob::print_attempt(DWORD job, UINT id)
{
	char buffer[MAX_PATH_LENGTH*2];

	if (SUCCEEDED(StringCbPrintf(buffer, sizeof(buffer), info_attempt, job, id))) {
		log->logEvent(buffer, INFO, 0);
	};
}

#include "mainclass.h"
#include "const.h"
#include "protocol.h"
#include "newlogger.h"
#include "aobject.h"
#include "point.h"
#include "support.h"
#include "errors.h"
#include "thash.h"
#include "tmacro.h"
#include "file.h"
#include "fileobj.h"
#include "filestruct.h"

#include <strsafe.h>

#pragma warning (disable: 4482)
#pragma warning (disable: 4127) // while (true)

extern PNewLogger log;

extern char* info_str[];

extern DWORD timeout;
extern DWORD block_size;

extern char* system_queue_objects;
extern char* system_queue_points;
extern char* local_macros_objects;
extern char* system_queue_jobs;

extern char* macro_point_name;
extern char* macro_point_id;

extern char* macro_date_name;

extern char* macro_object_name;
extern char* macro_object_id;

extern char* temp_ext;

extern char* dir_recieve;

extern PDQueue macros;

extern char* slash;

extern char* info_string;
extern char* info_string_short;

extern char* operation_get_hash;
extern char* operation_get_file;
extern char* operation_put_file;
extern char* operation_open_transaction;
extern char* operation_commit_transaction;
extern char* operation_undo_transaction;
extern char* operation_enum_files;
extern char* operation_close_transaction;
extern char* operation_delete_file;
extern char* operation_clean_files;

extern char* OK_STR;
extern char* ERROR_STR;
extern char* PEER_ERROR;

extern magic_sign magic_number;

extern char* summary_string;

extern DWORD block_size;

extern DWORD min_file_size;

TMainClass::TMainClass(PServerClientSocket s): socket(s), input_buffer(NULL), client_buffer(0), position(0), sessionId(0), connected(false), finished(false), point(NULL), in_transaction(false)
{
	input_buffer=new char[block_size+sizeof(frame)];

	client_buffer=new char[block_size+sizeof(frame)];
	
	sessionId=getSessionId();

	local_macros=new DQueue(local_macros_objects, false);

	timer=new TimeObject();

	timer->init(timeout);

	files=new DQueue(NULL, false);
}

TMainClass::~TMainClass()
{
	if (files!=NULL) delete files;

	if (timer!=NULL) {
		timer->done();

		delete timer;
	};

	if (local_macros!=NULL) delete local_macros;

	if (client_buffer!=NULL) delete client_buffer;

	if (input_buffer!=NULL) delete input_buffer;

	if (socket!=NULL) delete socket;
}

bool TMainClass::process_messages()
{
	DWORD read_bytes;

	if (finished) return false;

	if (!socket->isConnected()) {
		return false;
	};

	if (socket->read(&input_buffer[position], block_size+sizeof(frame))-position) {
		read_bytes=socket->get_read_bytes();

		if (read_bytes>0) {
			if (!parse_buffer(read_bytes)) {
				log->logEvent(info_str[SERVER_PROTO_ERROR_STR], ERR);

				return false;
			};

			timer->init(timeout);
		};
	};
	
	// we got timeout
	if (timer->timeout()) {
		log->logEvent(info_str[READ_TIMEOUT_STR], ERR);

		return false;
	};

	return true;
}

bool TMainClass::parse_buffer(DWORD read_bytes)
{
	DWORD msg_size;

	position=position+read_bytes;

	if (read_bytes<sizeof(magic_sign)) return true;

	// first 4 bytes - magic
	if ((((frame*)input_buffer)->magic.dw)!=magic_number.dw) {
		log->logEvent(info_str[CLIENT_PROTO_ERROR_STR], ERR, PROTOCOL_PACKET_CORRUPTED);

		return false;
	};

	// buffer size is smaller than protocol header, nothing to parse
	if (position<sizeof(frame)) return true;

	switch (((frame*)input_buffer)->type) {
		case frame_type::command: {
			if (connected) {
				if (sessionId!=((frame*)input_buffer)->sessionId) {
					error_frame(CLIENT_PROTO_ERROR_STR, INVALID_SESSION);

					return false;
				};
			};

			msg_size=(((frame*)input_buffer)->size+sizeof(frame));

			if (position<msg_size) return true;
			
			parse_command();

			if (position>msg_size) {
				CopyMemory(input_buffer, &input_buffer[msg_size], position-msg_size);

				position=position-msg_size;
			}
			else position=0;

			break;
		};

		default: {
			error_frame(CLIENT_PROTO_ERROR_STR, INVALID_COMMAND);

			position=0;
		};
	};

	return true;
}

void TMainClass::parse_command()
{
	switch(((frame*)input_buffer)->command) {
		case command_type::open: {
			connected=command_open(((frame_open*)input_buffer)->data.pointId, ((frame_open*)input_buffer)->data.jobId);

			break;
		};

		case command_type::close: {
			command_close();

			break;
		};

		case command_type::get_hash: {
			if (connected) {
				command_get_hash();
			}
			else error_frame(CLIENT_PROTO_ERROR_STR, NOT_CONNECTED, false);

			break;
		};

		case command_type::get_file: {
			if (connected) {
				command_get_file();
			}
			else error_frame(CLIENT_PROTO_ERROR_STR, NOT_CONNECTED, false);

			break;
		};

		case command_type::put_file: {
			if (connected) {
				command_put_file();
			}
			else error_frame(CLIENT_PROTO_ERROR_STR, NOT_CONNECTED, false);

			break;
		};

		case command_type::transaction: {
			if (connected) {
				command_transaction();
			}
			else error_frame(CLIENT_PROTO_ERROR_STR, NOT_CONNECTED, false);

			break;
		};

		case command_type::enum_files: {
			if (connected) {
				command_enum_files();
			}
			else error_frame(CLIENT_PROTO_ERROR_STR, NOT_CONNECTED, false);

			break;
		};

		case command_type::delete_file: {
			if (connected) {
				command_delete_file();
			}
			else error_frame(CLIENT_PROTO_ERROR_STR, NOT_CONNECTED, false);

			break;
		};

		case command_type::clean_files: {
			if (connected) {
				command_clean_files();
			}
			else error_frame(CLIENT_PROTO_ERROR_STR, NOT_CONNECTED, false);

			break;
		};

		default: {
			error_frame(CLIENT_PROTO_ERROR_STR, INVALID_COMMAND, true);

			return;
		};
	};
}

void TMainClass::error_frame(DWORD str, DWORD code, bool recoverable)
{
	frame_error error;

	error.header.magic.dw=magic_number.dw;

	error.header.type=frame_type::error;
	error.header.command=code;
	error.header.sessionId=sessionId;
	error.header.size=0;

	if (!socket->write(&error, sizeof(frame))) goto bad;

	if (!recoverable) {
bad:
		done();
	};

	log->logEvent(info_str[str], ERR, code);
}

bool TMainClass::command_open(DWORD pointId, DWORD job_Id)
{
	frame_open_ok open;

	jobId=job_Id;

	if (init(pointId)) {
		open.header.magic.dw=magic_number.dw;

		open.header.type=frame_type::ok;
		open.header.command=command_type::open;
		open.header.sessionId=sessionId;
		open.header.size=sizeof(DWORD);

		open.data.block_size=block_size;

		if (!socket->write(&open, sizeof(frame_open_ok))) {
			log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());

			done();

			return false;
		}
		else {
			log->logEvent(info_str[POINT_CONNECTED_STR], INFO, pointId);

			connected=true;

			// make stats
			point->enter();

			try {
				point->sessions++;

				time(&point->last_access);
			}
			catch (...) {
			};

			point->leave();

			return true;
		};
	}
	else error_frame(BAD_POINT_ID_STR, pointId, false);

	return false;
}

void TMainClass::command_close()
{
	frame_close_ok close;

	if (connected) {
		close.header.magic.dw=magic_number.dw;

		close.header.type=frame_type::ok;
		close.header.command=command_type::close;
		close.header.sessionId=sessionId;
		close.header.size=0;

		if (!socket->write(&close, sizeof(frame_close_ok))) {
			done();

			return;
		};

		log->logEvent(info_str[POINT_DISCONNECTED_STR], INFO, point->get_id());
	};

	connected=false;

	point=NULL;
}

void TMainClass::command_get_hash()
{
	char file_name_buffer[MAX_PATH_LENGTH];
	BYTE length;
	DWORD id;
	PAObject object;
	TString file_path;
	frame_get_hash_ok hash_ok;

	id=((frame_get_hash*)input_buffer)->data.object_id;

	object=(PAObject)find_object_by_id(id, system_queue_objects);

	if (object==NULL) {
		error_frame(BAD_OBJECT_ID_STR, id);

		return;
	};

	add_local_object(object);

	try {
		file_path.set_string(object->server_path->get_string());

		replace_templates(&file_path, macros); // replace global macro

		replace_templates(&file_path, local_macros); // replace local macro

		if (!has_trailing_slash(file_path.get_string())) file_path.add_string(slash);

		length=((frame_get_hash*)input_buffer)->data.name.size;

		if (length>0) {
			if (has_invalid_chars(((frame_get_hash*)input_buffer)->data.name.filename, length)) {
				error_frame(BAD_FILE_NAME_STR, BAD_CHARS_IN_FILENAME);

				goto remove_obj;
			};

			CopyMemory(file_name_buffer, ((frame_get_hash*)input_buffer)->data.name.filename, length);

			file_name_buffer[length]='\0';

			file_path.add_string(file_name_buffer);
		}
		else file_path.add_string(object->file->get_string());

		// debug info print
		if (length>0) 
			print_info(object->get_id(), operation_get_hash, file_name_buffer);
		else
			print_info(object->get_id(), operation_get_hash, object->file->get_string());

		hash_ok.header.magic.dw=magic_number.dw;

		hash_ok.header.type=frame_type::ok;
		hash_ok.header.command=command_type::get_hash;
		hash_ok.header.sessionId=sessionId;
		hash_ok.header.size=sizeof(hash_ret);

		FillMemory(hash_ok.data.hash, sizeof(hash_array), '\0');

		if (!PathFileExists(file_path.get_string())) {
			error_frame(FILE_NOT_FOUND_STR, FILE_NOT_FOUND);
			
			goto remove_obj;
		};

		if (!get_file_hash(file_path.get_string(), hash_ok.data.hash)) {
			error_frame(GET_HASH_ERROR_STR, ERROR_GET_HASH);

			goto remove_obj;
		};

		if (!socket->write(&hash_ok, sizeof(frame_get_hash_ok))) {
			log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());

			done();

			goto remove_obj;
		};
	}
	catch (...) {
		error_frame(GET_HASH_ERROR_STR, ERROR_GET_HASH);
	};

	// debug output
	print_info(object->get_id(), operation_get_hash, OK_STR);

remove_obj:
	remove_local_object();
}

void TMainClass::command_get_file()
{
	char file_name_buffer[MAX_PATH_LENGTH];
	BYTE length;
	DWORD id, counter, times=0, result;
	PAObject object;
	TString file_path;
	frame_get_file_ok file_ok;
	TFile file;
	DWORD first_tick;
	__int64 start_block, pages, remainder, bytes_writed=0;
	frame_error frame;

	id=((frame_get_file*)input_buffer)->data.object_id;

	object=(PAObject)find_object_by_id(id, system_queue_objects);

	start_block=((frame_get_file*)input_buffer)->data.start_block;

	if (object==NULL) {
		error_frame(BAD_OBJECT_ID_STR, id);

		return;
	};

	add_local_object(object);

	try {
		file_path.set_string(object->server_path->get_string());

		replace_templates(&file_path, macros); // replace global macro

		replace_templates(&file_path, local_macros); // replace local macro

		if (!has_trailing_slash(file_path.get_string())) file_path.add_string(slash);

		length=((frame_get_file*)input_buffer)->data.name.size;

		if (length>0) {
			if (has_invalid_chars(((frame_get_file*)input_buffer)->data.name.filename, length)) {
				error_frame(BAD_FILE_NAME_STR, BAD_CHARS_IN_FILENAME);

				if (!in_transaction) goto remove_obj;
				else goto out;
			};

			CopyMemory(file_name_buffer, ((frame_get_file*)input_buffer)->data.name.filename, length);

			file_name_buffer[length]='\0';

			file_path.add_string(file_name_buffer);
		}
		else file_path.add_string(object->file->get_string());

		// debug print
		if (length>0)
			print_info(object->get_id(), operation_get_file, file_name_buffer);
		else
			print_info(object->get_id(), operation_get_file, object->file->get_string());

		if (!PathFileExists(file_path.get_string())) {
			error_frame(FILE_NOT_FOUND_STR, FILE_NOT_FOUND);
			
			if (!in_transaction) goto remove_obj;
			else goto out;
		};

		// get file hash
		get_file_hash(file_path.get_string(), file_ok.data.file_hash);

		file.open(file_path.get_string()); // read, fail if not exist, share read&write

		if (!file.isValid()) {
			error_frame(FILE_STATUS_NOT_VALID_STR, GetLastError());
			
			if (!in_transaction) goto remove_obj;
			else goto out;
		};

		if (start_block>0) {
			if (!file.set_pointer(start_block*block_size, FILE_BEGIN)) {
				error_frame(FILE_MOVE_POINTER_STR, FILE_MOVE_POINTER);

				if (!in_transaction) goto remove_obj;
				else goto out;
			};
		};

		// send ok answer
		file_ok.header.magic.dw=magic_number.dw;

		file_ok.header.type=frame_type::ok;
		file_ok.header.command=command_type::get_file;
		file_ok.header.sessionId=sessionId;
		file_ok.header.size=sizeof(DWORD)+sizeof(__int64)+sizeof(FILETIME)+sizeof(hash_array);

		file_ok.data.size=file.size();
		file_ok.data.attributes=GetFileAttributes(file_path.get_string());

		file.get_time(&file_ok.data.date);

		if (!socket->write(&file_ok, sizeof(frame_get_file_ok))) {
			log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());
out:
			done();

			goto remove_obj;
		}

		if (file.size()==0) goto skip_read;

		counter=1;

		((frame_data*)client_buffer)->header.magic.dw=magic_number.dw;

		((frame_data*)client_buffer)->header.type=frame_type::data;
		((frame_data*)client_buffer)->header.command=counter;
		((frame_data*)client_buffer)->header.sessionId=sessionId;
		((frame_data*)client_buffer)->header.size=block_size;

		pages=(file.size()/block_size);

		// count ticks
		first_tick=GetTickCount();

		for (__int64 i=start_block; i<pages; i++) {
			if (finished) {
				log->logEvent(info_str[PROGRAM_TERMINATING_STR], ERR, PROGRAMM_TERMINATING);

				goto out;
			};

			if (!file.read(&client_buffer[sizeof(frame_data)], block_size)) {
				error_frame(FILE_READ_ERROR_STR, GetLastError());

				if (!in_transaction) goto remove_obj;
				else goto out;
			};

			if (!socket->write(client_buffer, sizeof(frame_data)+block_size)) {
				log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());

				goto out;
			};

			counter++;

			((frame_data*)client_buffer)->header.command=counter;

			// yield some time to windows
			Sleep(0);
		};

		remainder=file.size()-(pages*block_size);

		if (remainder>0) {
			file_ok.header.size=(DWORD)remainder;

			if (!file.read(&client_buffer[sizeof(frame_data)], (DWORD)remainder)) {
				log->logEvent(info_str[FILE_READ_ERROR_STR], ERR, GetLastError());

				if (!in_transaction) goto remove_obj;
				else goto out;
			};

			if (!socket->write(client_buffer, (DWORD)(sizeof(frame_data)+remainder))) {
				log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());

				goto out;

			};
		};

		times=GetTickCount()-first_tick;

		bytes_writed=file.size()-(start_block*block_size);

		// make stats
		point->update_write_stat(bytes_writed, times);

skip_read:		
		file.close();

		// MUST get ack from client
		result=get_answer((PClientSocket)socket, (char*)&frame, sizeof(frame_error), finished);

		if (result!=ERR_NO) {
			//log action - ok
			print_info(object->get_id(), operation_put_file, PEER_ERROR);

			goto remove_obj;
		};

		// debug output
		print_info(object->get_id(), operation_get_file, OK_STR);

		print_summary(1, (DWORD)bytes_writed, times);
	}
	catch (...) {
	};

remove_obj:
	remove_local_object();
}

void TMainClass::command_put_file()
{
	char file_name_buffer[MAX_PATH_LENGTH];
	BYTE length;
	DWORD id, counter, position, times=0;
	PAObject object;
	TString file_path, temp_file_path;
	frame_put_file_ok file_ok;
	TFile file;
	TimeObject time;
	DWORD first_tick;
	DWORD attr;
	__int64 bytes_readed=0, start_block=0, file_size, bytes_to_read, need_write;
	hash_array file_hash;
	frame_ok frame_answer;

	id=((frame_put_file*)input_buffer)->data.object_id;

	object=(PAObject)find_object_by_id(id, system_queue_objects);

	if (object==NULL) {
		error_frame(BAD_OBJECT_ID_STR, id);

		return;
	};

	file_size=((frame_put_file*)input_buffer)->data.size;

	add_local_object(object);

	try {
		file_path.set_string(object->server_path->get_string());

		replace_templates(&file_path, macros); // replace global macro

		replace_templates(&file_path, local_macros); // replace local macro

		if (!has_trailing_slash(file_path.get_string()))
			file_path.add_string(slash);

		length=((frame_put_file*)input_buffer)->data.name.size;

		if (length>0) {
			if (has_invalid_chars(((frame_put_file*)input_buffer)->data.name.filename, length)) {
				error_frame(BAD_FILE_NAME_STR, BAD_CHARS_IN_FILENAME);

				if (!in_transaction) goto remove_obj;
				else goto out;
			};

			CopyMemory(file_name_buffer, ((frame_put_file*)input_buffer)->data.name.filename, length);

			file_name_buffer[length]='\0';

			file_path.add_string(file_name_buffer);
		}
		else file_path.add_string(object->file->get_string());

		// if path not exist, create
		if (!PathFileExists(file_path.get_string())) {
			if (!create_dir(file_path.get_string())) {
				error_frame(ERROR_CREATE_DEST_DIR_STR, BAD_DESTINATION_DIR);

				if (!in_transaction) goto remove_obj;
				else goto out;
			};
		};

		temp_file_path.set_string(dir_recieve);

		if (!has_trailing_slash(temp_file_path.get_string()))
			temp_file_path.add_string(slash);

		if (length>0)
			temp_file_path.add_string(find_filename(file_name_buffer));
		else
			temp_file_path.add_string(object->file->get_string());

		temp_file_path.add_string(get_point_id());

		temp_file_path.add_string(temp_ext);

		if (!in_transaction) {
			// debug operation output
			if (length>0)
				print_info(object->get_id(), operation_put_file, file_name_buffer);
			else
				print_info(object->get_id(), operation_put_file, object->file->get_string());
		};

		// if file exist and don't need to overwrite - exit
		if (PathFileExists(file_path.get_string())&&(!object->overwrite)) {
			error_frame(NO_NEED_OVERWRITE_STR, DONT_NEED_OVERWRITE);

			goto remove_obj;
		};

		// create file if not exist, allow write
		if (!PathFileExists(temp_file_path.get_string())) 
			file.open(temp_file_path.get_string(), true, true, true); // write, create if not exist, share read
		else 
			file.open(temp_file_path.get_string(), true, false, true); // write, open existing, share read

		if (!file.isValid()) {
			error_frame(FILE_STATUS_NOT_VALID_STR, GetLastError());

			if (!in_transaction) goto remove_obj;
			else goto out;
		};

		// check if we can use old recieved data
		if (file.size()>min_file_size) {
			start_block=file.size()/block_size;

			if (!file.set_pointer(start_block*block_size, FILE_BEGIN)) {
				log->logEvent(info_str[FILE_MOVE_POINTER_STR], WARN, FILE_MOVE_POINTER);

				start_block=0;

				file.set_pointer(0, FILE_BEGIN);
			};
		};

		// send OK answer
		file_ok.header.magic.dw=magic_number.dw;

		file_ok.header.type=frame_type::ok;
		file_ok.header.command=command_type::put_file;
		file_ok.header.sessionId=sessionId;
		file_ok.header.size=sizeof(__int64);

		file_ok.data.start_block=start_block;

		if (!socket->write(&file_ok, sizeof(frame_put_file_ok))) {
			log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());
out:
			done();

			goto remove_obj;
		};

		if (file_size==0) goto skip_read;

		file_size=file_size-(start_block*block_size);  // adjust if not from first block

		counter=1;

		time.init(timeout);

		bytes_to_read=min(block_size, file_size);
		need_write=bytes_to_read;
		bytes_to_read=bytes_to_read+sizeof(frame_data);

		position=0;

		first_tick=GetTickCount();

		do {
			if (socket->read(&client_buffer[position], (DWORD)bytes_to_read)) {
				position=position+socket->get_read_bytes();

				bytes_to_read=bytes_to_read-socket->get_read_bytes();

				if (bytes_to_read==0) {
					if ((((frame_data*)client_buffer)->header.magic.dw==magic_number.dw)&&(((frame_data*)client_buffer)->header.type==frame_type::data)&&(((frame_data*)client_buffer)->header.command==counter)) {
						if (!file.write(&client_buffer[sizeof(frame_data)], (DWORD)need_write)) {
							error_frame(FILE_WRITE_ERROR_STR, GetLastError());

							time.done();

							if (!in_transaction) goto remove_obj;
							else goto out;
						};

						file_size=file_size-need_write;

						if (file_size<=0) break;

						bytes_to_read=min(block_size, file_size);
						need_write=bytes_to_read;
						bytes_to_read=bytes_to_read+sizeof(frame_data);

						position=0;

						counter++;

						time.init(timeout);

						continue;
					};

					// got error packet
					if ((((frame_data*)client_buffer)->header.magic.dw==magic_number.dw)&&(((frame_data*)client_buffer)->header.type==frame_type::error)) {
						log->logEvent(info_str[ERROR_SERVER_PROTOCOL], ERR, ERROR_READ_FILE);

						time.done();

						if (!in_transaction) goto remove_obj;
						else goto out;
					};
				};
			};

			if (!socket->isConnected()) {
				log->logEvent(info_str[SOCKET_DISCONNECTED_STR], ERR, GetLastError());

				goto out;
			};

			if (finished) {
				error_frame(PROGRAM_TERMINATING_STR, PROGRAMM_TERMINATING);

				goto out;
			};

			if (time.timeout()) {
				error_frame(READ_TIMEOUT_STR, ERR_TIMEOUT);

				goto out;
			};

			// yield some time to windows
			Sleep(0);

		} while (true);

		time.done();

		times=GetTickCount()-first_tick;

		bytes_readed=file.size()-(start_block*block_size);
		
		// make stats
		point->update_read_stat(bytes_readed, times);

skip_read:
		file.close();

		// CHECK recieved file hash
		if (!get_file_hash(temp_file_path.get_string(), file_hash)) {
			error_frame(GET_HASH_ERROR_STR, ERROR_GET_HASH);

			if (!in_transaction) goto remove_obj;
			else goto out;
		};

		for (UINT i=0; i<MD5LEN; i++) {
			// hashes don't match - need to resend file
			if (file_hash[i]!=((frame_put_file*)input_buffer)->data.file_hash[i]) {
				error_frame(HASH_MISMATCH_STR, FILE_HASH_MISMATCH);

				if (!in_transaction) goto remove_obj;
				else goto out;
			};
		};

		// send OK answer
		frame_answer.header.magic.dw=magic_number.dw;

		frame_answer.header.type=frame_type::ok;
		frame_answer.header.command=command_type::put_file;
		frame_answer.header.sessionId=sessionId;
		frame_answer.header.size=0;

		if (!socket->write(&frame_answer, sizeof(frame_ok))) {
			log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, GetLastError());

			if (!in_transaction) goto remove_obj;
			else goto out;
		};

		if (!in_transaction) {
			// if file exist - replace it
			if (!CopyFile(temp_file_path.get_string(), file_path.get_string(), false)) {
				log->logEvent(info_str[ERROR_COPY_FILE_DEST_STR], ERR, GetLastError());
			}
			else {
				SetFileAttributes(file_path.get_string(), ((frame_put_file*)input_buffer)->data.attributes);

				file.open(file_path.get_string(), true);

				SetFileTime(file.getHandle(), NULL, NULL, &((frame_put_file*)input_buffer)->data.date);

				file.close();

				// debug output
				print_info(object->get_id(), operation_put_file, OK_STR);

				print_summary(1, (DWORD)bytes_readed, times);

				attr = GetFileAttributes(temp_file_path.get_string());
				attr = attr&~FILE_ATTRIBUTE_READONLY;

				SetFileAttributes(temp_file_path.get_string(), attr);

				DeleteFile(temp_file_path.get_string());
			};
		}
		else files->addElement(new TFileObject(temp_file_path.get_string(), file_path.get_string(), ((frame_put_file*)input_buffer)->data.attributes, ((frame_put_file*)input_buffer)->data.date, id));
	}
	catch (...) {
	};

remove_obj:
	remove_local_object();
}

void TMainClass::command_transaction()
{
	PBlock block;
	PString temp_str, file_str;
	frame_set_transaction_ok ok;
	PFileObject fileobj;
	TFile file;
	DWORD attr;

	switch (((frame_set_transaction*)input_buffer)->data.value) {
		case transaction_state::open_transaction : {
			if (in_transaction) goto error;

			files->clear();

			in_transaction=true;

			print_info(jobId, operation_open_transaction);

			break;
		};

		case transaction_state::commit_transaction : {
			if (!in_transaction) goto error;

			block=files->head;

			while (block!=NULL) {
				fileobj=(PFileObject)block->obj;

				if (fileobj==NULL) goto move_next;

				temp_str=fileobj->temp_file;

				file_str=fileobj->file;

				if ((temp_str==NULL)||(file_str==NULL)) goto move_next;

				print_info(jobId, operation_commit_transaction, find_filename(file_str->get_string()));

				if (!CopyFile(temp_str->get_string(), file_str->get_string(), false)) 
					print_info(jobId, operation_commit_transaction, ERROR_STR);
				else {
					print_info(jobId, operation_commit_transaction, OK_STR);

					SetFileAttributes(file_str->get_string(), fileobj->attributes);

					file.open(file_str->get_string(), true);

					file.set_time(&fileobj->date);

					file.close();

					// clear READ ONLY attribute
					attr = GetFileAttributes(temp_str->get_string());
					attr = attr&~FILE_ATTRIBUTE_READONLY;

					SetFileAttributes(temp_str->get_string(), attr);

					// delete if CopyFile succceed
					DeleteFile(temp_str->get_string());
				};

move_next:
				block=block->next;
			};

			files->clear();

			print_info(jobId, operation_close_transaction);

			in_transaction=false;

			break;
		};

		case transaction_state::undo_transaction : {
			if (!in_transaction) goto error;

			files->clear();

			in_transaction=false;

			print_info(jobId, operation_undo_transaction);

			break;
		};
		default: {
			error_frame(CLIENT_PROTO_ERROR_STR, INVALID_TRANSACTION);

			return;
		};
	};

	ok.header.magic.dw=magic_number.dw;

	ok.header.type=frame_type::ok;
	ok.header.command=command_type::transaction;
	ok.header.sessionId=sessionId;
	ok.header.size=0;

	if (!socket->write(&ok, sizeof(frame_set_transaction_ok))) {
		log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());

		done();
	};

	return;

error:
	error_frame(TRANSACTION_ERR_STR, INVALID_TRANSACTION_STATE);
}

void TMainClass::command_enum_files()
{
	PAObject object;
	TString file_path, temp_str;
	size_t name_length;
	DWORD result, recurse_level;
	DQueue files_list("enum_files", false);
	PBlock file;
	PString file_name;

	object=(PAObject)find_object_by_id(((frame_enum_files*)input_buffer)->data.object_id, system_queue_objects);

	if (object==NULL) {
		error_frame(BAD_OBJECT_ID_STR, ERROR_OBJECT_ID);

		return;
	};

	add_local_object(object);

	try {
		file_path.set_string(object->server_path->get_string());

		replace_templates(&file_path, macros); // replace global macro

		replace_templates(&file_path, local_macros); // replace local macro

		if (!has_trailing_slash(file_path.get_string()))
			file_path.add_string(slash);

		// if path not exist, fail
		if (!PathFileExists(file_path.get_string())) {
			error_frame(ERROR_PATH_NOT_EXIST_STR, PATH_NOT_EXIST);

			goto remove_obj;
		};

		temp_str.set_string(file_path.get_string());

		temp_str.add_string(object->file->get_string());

		// debug output
		print_info(object->get_id(), operation_enum_files, temp_str.get_string());

		recurse_level=0;

		result=file_search(file_path.get_string(), NULL, object->file->get_string(), &files_list, object->recursive, recurse_level);

		if (result>0) {
			if (result==100) error_frame(FILE_NOT_FOUND_STR, FILE_NOT_FOUND);
			else error_frame(FIND_FILE_ERR_STR, FIND_FILE_ERROR);

			// debug output
			print_info(object->get_id(), operation_enum_files, ERROR_STR);

			goto remove_obj;
		};

		((frame_enum_files_ok*)client_buffer)->header.magic.dw=magic_number.dw;
		((frame_enum_files_ok*)client_buffer)->header.sessionId=sessionId;
		((frame_enum_files_ok*)client_buffer)->header.type=frame_type::ok;
		((frame_enum_files_ok*)client_buffer)->header.command=command_type::enum_files;

		file=files_list.head;

		while  (file!=NULL) {
			file_name=(PString)file->obj;

			if (file_name==NULL) goto next;

			if (!SUCCEEDED(StringCchLength(file_name->get_string(), MAX_PATH_LENGTH, &name_length))) goto next;

			if (name_length==0)  goto next;

			((frame_enum_files_ok*)client_buffer)->header.size=sizeof(bool)+sizeof(BYTE)+name_length;

			((frame_enum_files_ok*)client_buffer)->data.has_data=true;
			((frame_enum_files_ok*)client_buffer)->data.name.size=(BYTE)name_length;

			CopyMemory(((frame_enum_files_ok*)client_buffer)->data.name.filename, file_name->get_string(), name_length);

			if (!socket->write(client_buffer, sizeof(frame)+sizeof(bool)+sizeof(BYTE)+name_length)) {
				log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());
out:
				done();

				goto remove_obj;
			};
next:
			file=file->next;
		};

		((frame_enum_files_ok*)client_buffer)->header.size=sizeof(frame)+sizeof(bool)+sizeof(BYTE);

		((frame_enum_files_ok*)client_buffer)->data.has_data=false;
		((frame_enum_files_ok*)client_buffer)->data.name.size=0;

		if (!socket->write(client_buffer, sizeof(frame)+sizeof(bool)+sizeof(BYTE))) {
			log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());

			goto out;

		};

		// debug output
		print_info(object->get_id(), operation_enum_files, OK_STR);
	}
	catch(...) {
	}

remove_obj:
	remove_local_object();
}

void TMainClass::command_delete_file()
{
	char file_name_buffer[MAX_PATH_LENGTH];
	BYTE length;
	DWORD id;
	PAObject object;
	TString file_path;
	frame_delete_file_ok delete_ok;
	DWORD attr;

	id=((frame_delete_file*)input_buffer)->data.object_id;

	object=(PAObject)find_object_by_id(id, system_queue_objects);

	if (object==NULL) {
		error_frame(BAD_OBJECT_ID_STR, id);

		return;
	};

	add_local_object(object);

	try {
		file_path.set_string(object->server_path->get_string());

		replace_templates(&file_path, macros); // replace global macro

		replace_templates(&file_path, local_macros); // replace local macro

		if (!has_trailing_slash(file_path.get_string())) file_path.add_string(slash);

		length=((frame_delete_file*)input_buffer)->data.name.size;

		if (length>0) {
			if (has_invalid_chars(((frame_delete_file*)input_buffer)->data.name.filename, length)) {
				error_frame(BAD_FILE_NAME_STR, BAD_CHARS_IN_FILENAME);

				goto remove_obj;
			};

			CopyMemory(file_name_buffer, ((frame_delete_file*)input_buffer)->data.name.filename, length);

			file_name_buffer[length]='\0';

			file_path.add_string(file_name_buffer);
		}
		else file_path.add_string(object->file->get_string());

		// debug info print
		if (length>0)
			print_info(object->get_id(), operation_delete_file, file_name_buffer);
		else
			print_info(object->get_id(), operation_delete_file, object->file->get_string());

		delete_ok.header.magic.dw=magic_number.dw;

		delete_ok.header.type=frame_type::ok;
		delete_ok.header.command=command_type::delete_file;
		delete_ok.header.sessionId=sessionId;
		delete_ok.header.size=0;

		if (!PathFileExists(file_path.get_string())) {
			error_frame(FILE_NOT_FOUND_STR, FILE_NOT_FOUND);
			
			goto remove_obj;
		};

		// if read only, clear file attribute
		attr=GetFileAttributes(file_path.get_string());
		attr=attr&~FILE_ATTRIBUTE_READONLY;

		SetFileAttributes(file_path.get_string(), attr);

		if (DeleteFile(file_path.get_string())==FALSE) {
			error_frame(DELETE_FILE_ERROR_STR, DELETE_FILE_ERROR);

			goto remove_obj;
		};
	}
	catch (...) {
	};

	if (!socket->write(&delete_ok, sizeof(delete_ok))) {
		log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());

		done();

		goto remove_obj;
	};

	// debug output
	print_info(object->get_id(), operation_delete_file, OK_STR);
	
remove_obj:
	remove_local_object();
}

void TMainClass::command_clean_files()
{
	DWORD id, recurse_level, result;
	PAObject object;
	TString file_path;
	frame_clean_files_ok clean_ok;
	DQueue postprocess_files("postprocess files", false);
	PBlock file_block;
	DWORD attr;
	PFileStruct file_to_del;

	id=((frame_clean_files*)input_buffer)->data.object_id;

	object=(PAObject)find_object_by_id(id, system_queue_objects);

	if (object==NULL) {
		error_frame(BAD_OBJECT_ID_STR, id);

		return;
	};

	add_local_object(object);

	try {
		file_path.set_string(object->server_path->get_string());

		replace_templates(&file_path, macros); // replace global macro

		replace_templates(&file_path, local_macros); // replace local macro

		if (!has_trailing_slash(file_path.get_string()))
			file_path.add_string(slash);

		clean_ok.header.magic.dw=magic_number.dw;

		clean_ok.header.type=frame_type::ok;
		clean_ok.header.command=command_type::clean_files;
		clean_ok.header.sessionId=sessionId;
		clean_ok.header.size=0;

		// debug info print
		print_info(object->get_id(), operation_clean_files, file_path.get_string());

		recurse_level=0;

		result=file_del(file_path.get_string(), NULL, &postprocess_files, recurse_level);

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
					}
					else {
						RemoveDirectory(file_to_del->path->get_string());
					};
				};

				file_block=file_block->next;
			};
		};
	}
	catch (...) {
	};

	if (!socket->write(&clean_ok, sizeof(clean_ok))) {
		log->logEvent(info_str[WRITE_SOCKET_ERROR_STR], ERR, WSAGetLastError());

		done();

		goto remove_obj;
	};

	// debug output
	print_info(object->get_id(), operation_clean_files, OK_STR);
	
remove_obj:
	remove_local_object();
}

bool TMainClass::init(DWORD pointId)
{
	PMacro macro;
	char buffer[MAX_DIGITS];
	char date[10];

	point=(PPoint)find_object_by_id(pointId, system_queue_points);

	if (point!=NULL) {
		macro=new TMacro(macro_point_name, point->get_string());

		local_macros->addElement(macro);

		StringCbPrintf(buffer, sizeof(buffer), "%03d", point->get_id());

		macro=new TMacro(macro_point_id, buffer);

		local_macros->addElement(macro);

		get_date_str(date, sizeof(date));

		local_macros->addElement(new TMacro(macro_date_name, date), true);

		return true;
	}
	else return false;
}

void TMainClass::done()
{
	point=NULL;

	connected=false;

	finished=true;
}

void TMainClass::replace_local_macros(PString str)
{
	replace_templates(str, local_macros);
}

void TMainClass::add_local_object(PAObject object)
{
	PMacro macro;
	char buffer[MAX_DIGITS];

	macro=new TMacro(macro_object_name, object->get_string());

	local_macros->addElement(macro);

	StringCbPrintf(buffer, sizeof(buffer), "%03d", object->get_id());

	macro=new TMacro(macro_object_id, buffer);

	local_macros->addElement(macro);
}

void TMainClass::remove_local_object()
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

void TMainClass::print_info(DWORD id, char* operation, char* file)
{
	char temp_buffer[MAX_PATH_LENGTH*2];

	if (file!=NULL) {
		if (SUCCEEDED(StringCbPrintf(temp_buffer, sizeof(temp_buffer), info_string, id, operation, file))) {
			log->logEvent(temp_buffer, INFO, 0);
		}
	}
	else {
		if (SUCCEEDED(StringCbPrintf(temp_buffer, sizeof(temp_buffer), info_string_short, id, operation))) {
			log->logEvent(temp_buffer, INFO, 0);
		};
	};
}

void TMainClass::print_summary(DWORD files, __int64 bytes, DWORD msec)
{
	char buffer[MAX_PATH_LENGTH*2];

	if (SUCCEEDED(StringCbPrintf(buffer, sizeof(buffer), summary_string, files, bytes, ((double)msec/1000)))) {
		log->logEvent(buffer, INFO, 0);
	};
}

char* TMainClass::get_point_id()
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

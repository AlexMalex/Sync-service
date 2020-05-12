#include "svcobj.h"
#include "newlogger.h"
#include "point.h"
#include "dqueue.h"
#include "protocol.h"

#include <shlobj.h>

magic_sign magic_number={0xA13D};

// string constants
char* ServiceName="syncService";
char* ServiceDisplayName="Sync service";
char* ServiceDescription="Service sync files";

// stop event name
char* StopEventName="Global\\syncservice.stop.event";

// sync event name
char* SyncEventName="Local\\syncservice.sync.event";

// name of the service image
char* image_name="syncservice.exe";

// path to service image
char* image_path=NULL;

// path to service send/recieve files temp dirs
char* dir_recieve=NULL;
char* dir_log=NULL;
char* dir_config=NULL;
char* dir_update=NULL;

// logger strings
char* info_str[64]={"main thread started", "main thread stopped", "main thread crashed",
		   "client thread started", "client thread stopped", "client thread crushed", "client thread protocol error", "client thread recieve file error", 
		   "point connected id:", "point disconnected id:",
		   "socket write error", "get hash error",
		   "bad point id", "bad object id", "bad job id",
		   "get file error", "put file error", "read socket error",
		   "client job started id:", "client job stopped id:", "client job crushed",
		   "main ip not reachable point:", "alt ip not reachable point:", "not implemented",
		   "memory error", "file not found", "server protocol error", "find file failed",
		   "file read error", "file write error", "server recieve error",
		   "invalid chars in file name", "invalid file status", "client socket timeout", 
		   "scheduler started", "scheduler stopped", "scheduler crashed",
		   "error create destination directory", "error copy file", "file exist, no need to overwrite",
		   "transaction error", "path not exist", "point has no IP address",
		   "system files update start", "system files update stop", "update syncservice.exe started",
		   "update syncupdater.exe success", "update syncupdater.exe failed", 
		   "update syncservice.exe failed", "stat process started", "stat process finished", "stat process crashed",
		   "programm terminating", "socket disconnected", "delete file error", "move file pointer error",
		   "clean logs started", "clean logs finished", "sync files started", "sync files stopped",
		   "web server started", "web server stopped",
		   "hash mismatch", "internal error"
};

// logger object
PNewLogger log=NULL;

// log path
char* log_path="logs\\";
char* log_name="syncservice.log";

char* stat_name="stat.log";

// config paths
char* config_path="conf\\";
char* recieve_path="recieve\\";
char* update_path="update\\";

char* config_main_name="service.xml";
char* config_points_name="points.xml";
char* config_objects_name="objects.xml";
char* config_jobs_name="jobs.xml";
char* config_macros_name="macros.xml";

char* config_names[5]= {config_main_name, config_points_name, config_objects_name, config_jobs_name, config_macros_name};

char* xml_names[5]={"service", "points", "objects", "jobs", "macros"};

hash_array conf_files[5]={};

// error strings
char* memory_error="Memory allocation failure";
char* winsock_error="Can't init Winsock library";
char* create_objects_error="Can't create main objects";
char* dir_path_error="Can't open directory";
char* logger_error="Logger error";
char* bind_error="Can't bind socket to port";
char* xml_error="XML parse error";
char* bad_service_config="Bad main config file";
char* bad_points_config="Bad points config file";
char* bad_system_objects="Can't create system objects";

// windows log error template
char* windowsLogErr="%s failed with code %d";

// main config data
//
// port
int port=5000;
int client_port=5000;
int web_port=9091;

// client timeout, seconds
DWORD timeout=CLIENT_TIMEOUT;

// sync conf period
DWORD sync_period=CHECK_CFG;

// data block size
DWORD block_size=PACKET_BUFFER_SIZE;

// self id
DWORD self_id=0;

// sync id
DWORD sync_id=0;

// self
PPoint self=NULL;

// sync point
PPoint sync_point=NULL;

// points
PDQueue points=NULL;

// objects
PDQueue objects=NULL;

// jobs
PDQueue jobs=NULL;

// macros
PDQueue macros=NULL;

// system queue
PDQueue queues=NULL;

char* system_queue_name="system.queue";
char* system_queue_points="system.points";
char* system_queue_objects="system.objects";
char* system_queue_jobs="system.jobs";
char* system_queue_macros="system.macros";

char* local_macros_objects="thread.local.macros";

char* temp_ext=".temp";

// well known macros
macro_struct macro[3]={{"%ProgramFiles%", CSIDL_PROGRAM_FILES}, {"%WinDir%", CSIDL_WINDOWS}, {"%WinSystem%", CSIDL_SYSTEM}};

char* macro_point_name="%PointName%";
char* macro_point_id="%PointId%";

char* macro_object_name="%ObjectName%";
char* macro_object_id="%ObjectId%";

char* macro_date_name="%Date%";
char* format_date_str="%04d%02d%02d";

// strings
char* slash="\\";

char* info_string="object: %d; operation: %s; file: %s";
char* info_string_short="object: %d; operation: %s";

char* operation_get_hash="GET_HASH";
char* operation_get_file="GET_FILE";
char* operation_put_file="PUT_FILE";
char* operation_open_transaction="OPEN TRANSACTION";
char* operation_commit_transaction="COMMIT TRANSACTION";
char* operation_undo_transaction="UNDO TRANSACTION";
char* operation_enum_files="ENUM_FILES";
char* operation_close_transaction="CLOSE TRANSACTION";
char* operation_delete_file="DELETE_FILE";
char* operation_delete_dir="DELETE_DIRECTORY";
char* operation_clean_files="CLEAN_FILES";
char* operation_clean_files_cancel="CLEAN_FILES - changes detected CANCEL";

char* OK_STR="OK";
char* ERROR_STR="ERROR";

char* PEER_ERROR="PEER ERROR";

char* invalid_chars="*|\"<>?/";

char* summary_string="summary: files: %d; bytes: %I64d; time: %.2f";

char* sys_update="system files update";
char* sys_image_update="system image update";

char* create_file="CREATE FILE: %s";

// name of the service image
char* image_updater="syncupdater.exe";

char* exe_names[2]={"syncservice.exe", "syncupdater.exe"};

hash_array exe_hash[2];

char* sys_image_updater="system image updater update";

WORD monthes[12]={31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

char* info_attempt="job : %d; attempt number - %d";

char* wildcard="*.*";

DWORD min_file_size=MIN_FILE_SIZE;

DWORD days_keep_logs=DAYS_KEEP_LOGS;

char* bytes_read_write_str="bytes %s: %.2f bytes";
char* kbytes_read_write_str="bytes %s: %.2f Kbytes";
char* mbytes_read_write_str="bytes %s: %.2f Mbytes";
char* gbytes_read_write_str="bytes %s: %.2f Gbytes";
char* tbytes_read_write_str="bytes %s: %.2f Tbytes";

char* amount[5]={bytes_read_write_str, kbytes_read_write_str, mbytes_read_write_str, gbytes_read_write_str, tbytes_read_write_str};

char* avg_speed_bytes_str="%s speed: %.2f bytes/sec";
char* avg_speed_kbytes_str="%s speed: %.2f Kbytes/sec";
char* avg_speed_mbytes_str="%s speed: %.2f Mbytes/sec";
char* avg_speed_gbytes_str="%s speed: %.2f Gbytes/sec";
char* avg_speed_tbytes_str="%s speed: %.2f Tbytes/sec";

char* speed[5]={avg_speed_bytes_str, avg_speed_kbytes_str, avg_speed_mbytes_str, avg_speed_gbytes_str, avg_speed_tbytes_str};

char* abytes_read_write_str="%.2f bytes";
char* akbytes_read_write_str="%.2f Kbytes";
char* ambytes_read_write_str="%.2f Mbytes";
char* agbytes_read_write_str="%.2f Gbytes";
char* atbytes_read_write_str="%.2f Tbytes";

char* aamount[5]={abytes_read_write_str, akbytes_read_write_str, ambytes_read_write_str, agbytes_read_write_str, atbytes_read_write_str};

char* aavg_speed_bytes_str="%.2f bytes/sec";
char* aavg_speed_kbytes_str="%.2f Kbytes/sec";
char* aavg_speed_mbytes_str="%.2f Mbytes/sec";
char* aavg_speed_gbytes_str="%.2f Gbytes/sec";
char* aavg_speed_tbytes_str="%.2f Tbytes/sec";

char* aspeed[5]={aavg_speed_bytes_str, aavg_speed_kbytes_str, aavg_speed_mbytes_str, aavg_speed_gbytes_str, aavg_speed_tbytes_str};

#if !defined(EXTERN_H)
#define EXTERN_H

#include "newlogger.h"
#include "point.h"
#include "const.h"
#include "dqueue.h"
#include "service.h"
#include "protocol.h"

extern magic_sign magic_number;

extern char* temp_ext;

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

extern char* xml_names[5];

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

extern char* OK_STR;
extern char* ERROR_STR;

extern char* info_attempt;
extern char* summary_string;

extern DWORD block_size;

extern DWORD self_id;

#endif

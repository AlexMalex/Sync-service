#if !defined(SUPPORT_H)
#define SUPPORT_H

#include "const.h"
#include "clntsock.h"
#include "configobj.h"
#include "dqueue.h"

enum answer_errors {ERR_NO, ERR_TIMEDOUT, ERR_DISCONNECTED, ERR_SHUTDOWN, ERR_UNKNOWN};

int init_all();
void done_all();

PConfigObject find_object_by_id(DWORD, char*);

char* get_name_by_id(DWORD, char*);

void init_macros();

void replace_templates(PString, PDQueue);

bool has_trailing_slash(char* str);

DWORD get_answer(PClientSocket, char*, DWORD, bool&, bool use_standart_timeout=true);

DWORD read_data(PClientSocket, char*, DWORD, bool&);

DWORD getSessionId();

bool has_invalid_chars(char*, BYTE);

bool create_dir(char*);

void get_date_str(char* buffer, DWORD size);

int init_config(int);

void define_macros();

bool get_file_hash(char*, char*);

void get_sys_files_hash();

bool make_system_objects();

char* find_filename(char*);

void calc_time_diff(WORD&, WORD&, WORD&, time_t&);

bool string_match(char*, char*);

DWORD file_search(char*, char*, char*, PDQueue, bool, DWORD&);

DWORD file_del(char*, char*, PDQueue, DWORD&);

char* format_value(double&, char**);

#endif

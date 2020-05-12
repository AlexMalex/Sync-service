#if !defined(PROTOCOL_H)
#define PROTOCOL_H

#include "const.h"

#include <windows.h>

enum frame_type {command=10, error=20, ok=30, data=40};

enum command_type {open=1, close=2, get_hash=3, get_file=4, put_file=5, transaction=6, enum_files=7, delete_file=8, clean_files=9};

#pragma pack(push)
#pragma pack(1)

union magic_sign {
	DWORD dw;
	char ch[4];
};

struct frame {
	magic_sign magic;
	DWORD type;
	DWORD command;
	DWORD sessionId;
	DWORD size;
};

// server answer - ok
struct frame_ok {frame header;};

// server answer - error
struct frame_error {frame header;};

// client command - OPEN
struct open_data {DWORD pointId; DWORD jobId;};

struct frame_open {frame header; open_data data;};

// server answer - OPEN
struct open_ok {DWORD block_size;};

struct frame_open_ok {frame header; open_ok data;};

// client command - CLOSE
struct frame_close {frame header;};

// server answer - CLOSE
struct frame_close_ok {frame header;};

// client command - GET HASH
struct hash_get {DWORD object_id; string name;};

struct frame_get_hash {frame header; hash_get data;};

// server answer - GET HASH
struct hash_ret {hash_array hash;};

struct frame_get_hash_ok {frame header; hash_ret data;};

// client command - GET FILE
struct file_get {DWORD object_id; __int64 start_block; string name;};

struct frame_get_file {frame header; file_get data;};

// server answer - GET FILE
struct file_ret {__int64 size; DWORD attributes; FILETIME date; hash_array file_hash;};

struct frame_get_file_ok {frame header; file_ret data;};

// server answer - DATA FILE
struct frame_data {frame header;};

// client command - PUT FILE
struct file_put {DWORD object_id; __int64 size; DWORD attributes; FILETIME date; hash_array file_hash; string name;};

struct frame_put_file {frame header; file_put data;};

// server answer - PUT FILE
struct frame_put_ok {__int64 start_block;};

struct frame_put_file_ok {frame header; frame_put_ok data;};

// client command - LIST FILES
struct file_list {DWORD object_id;};

// client command - TRANSACTION
enum transaction_state {open_transaction=1, commit_transaction=2, undo_transaction=3};

struct set_transaction {DWORD value;};

struct frame_set_transaction {frame header; set_transaction data;};

// server answer - TRANSACTION
struct frame_set_transaction_ok {frame header;};

// client command - ENUM_FILES
struct set_enum {DWORD object_id;};

struct frame_enum_files {frame header; set_enum data;};

// server answer - ENUM_FILES
struct file_enum_name {bool has_data; string name;};

struct frame_enum_files_ok {frame header; file_enum_name data;};

// client command - DELETE FILE
struct file_delete {DWORD object_id; string name;};

struct frame_delete_file {frame header; file_delete data;};

// server answer - DELETE FILE
struct frame_delete_file_ok {frame header;};

// client command - CLEAN FILES
struct files_clean {DWORD object_id;};

struct frame_clean_files {frame header; files_clean data;};

// server answer - CLEAN FILES
struct frame_clean_files_ok {frame header;};

#pragma pack(pop)

#endif

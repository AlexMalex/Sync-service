#if !defined(CONST_H)
#define CONST_H

// exit flag
#define NO_EXIT		0

// maximum working threads
#define	MAX_THREADS	100

// maximum working jobs
#define	MAX_TASKS	50

// maximum client wait - msec
#define	MAX_WAIT	50

// period for check system files update - 10 minutes in msec
#define	CHECK_CFG 10*60*1000

// period for stat information - 60 minutes in msec
#define SHOW_STAT_PERIOD 60*60*1000

// period wait for cfg update in in msec
#define	MAX_WAIT_FOR_UPDATE 5000 

// period bettween tries in client 
#define	MAX_TRY 5

// period between tries in client job - sec
#define	TRY_PERIOD 2000

// string messages
#define	MAIN_THREAD_START_STR		0
#define	MAIN_THREAD_STOP_STR		1
#define	MAIN_THREAD_CRASH_STR		2

#define	CLIENT_THREAD_START_STR		3
#define	CLIENT_THREAD_STOP_STR		4
#define	CLIENT_THREAD_CRASH_STR		5
#define	CLIENT_PROTO_ERROR_STR		6
#define	CLIENT_RECIEVE_ERROR_STR	7

#define	POINT_CONNECTED_STR			8
#define	POINT_DISCONNECTED_STR		9

#define	WRITE_SOCKET_ERROR_STR		10
#define	GET_HASH_ERROR_STR			11

#define	BAD_POINT_ID_STR			12
#define	BAD_OBJECT_ID_STR			13
#define	BAD_JOB_ID_STR				14

#define	GET_FILE_ERROR_STR			15
#define	PUT_FILE_ERROR_STR			16
#define	READ_SOCKET_ERROR_STR		17

#define	CLIENT_TASK_START_STR		18
#define	CLIENT_TASK_STOP_STR		19
#define	CLIENT_TASK_CRASH_STR		20

#define	NOT_CONNECTED_MAIN_IP_STR	21
#define	NOT_CONNECTED_ALT_IP_STR	22
#define	NOT_IMPLEMENTED_STR			23

#define	MEMORY_ERROR_STR			24
#define	FILE_NOT_FOUND_STR			25
#define	SERVER_PROTO_ERROR_STR		26
#define	FIND_FILE_ERR_STR			27

#define	FILE_READ_ERROR_STR			28
#define	FILE_WRITE_ERROR_STR		29

#define	SERVER_RECIEVE_ERROR_STR	30
#define	BAD_FILE_NAME_STR			31
#define	FILE_STATUS_NOT_VALID_STR	32
#define	READ_TIMEOUT_STR			33

#define	SCHEDULER_START_STR			34
#define	SCHEDULER_STOP_STR			35
#define	SCHEDULER_CRASH_STR			36

#define	ERROR_CREATE_DEST_DIR_STR	37
#define	ERROR_COPY_FILE_DEST_STR	38

#define	NO_NEED_OVERWRITE_STR		39

#define	TRANSACTION_ERR_STR			40

#define	ERROR_PATH_NOT_EXIST_STR	41

#define	BAD_POINT_IP_STR			42

#define	UPDATE_CONFIG_START_STR		43
#define	UPDATE_CONFIG_ENDED_STR		44

#define	UPDATE_EXE_STARTED_STR		45

#define	UPDATE_UPDATER_OK			46
#define	UPDATE_UPDATER_ERROR		47

#define	UPDATE_EXE_FAILED_STR		48

#define	STAT_PROCESS_STARTED_STR	49
#define	STAT_PROCESS_FINISHED_STR	50
#define	STAT_PROCESS_CRASHED_STR	51

#define	PROGRAM_TERMINATING_STR		52

#define	SOCKET_DISCONNECTED_STR		53

#define	DELETE_FILE_ERROR_STR		54

#define	FILE_MOVE_POINTER_STR		55

#define	CLEAN_LOGS_STARTED_STR		56
#define	CLEAN_LOGS_FINISHED_STR		57

#define	SYNC_FILES_STARTED_STR		58
#define	SYNC_FILES_STOPPED_STR		59

#define	WEB_SERVER_START_STR		60
#define	WEB_SERVER_STOP_STR			61

#define	HASH_MISMATCH_STR			62
#define	INTERNAL_ERROR_STR			63

// size of buffer for TCP/IP packets
#define PACKET_BUFFER_SIZE	0x1000

// buffer for hashing
#define HASH_BUFFER_SIZE	0x10000

// file name
#define MAX_PATH_LENGTH		255

// how many times wait answer
#define MAX_ATTEMPTS		10

// size of MD5 hash
#define MD5LEN				16

// server answer - GET HASH
typedef char hash_array[MD5LEN];

struct macro_struct {
	char* macro_def;
	int folder;
};

struct string {
	unsigned char size;
	char filename[1];
};

#define macro_symbol '%'

#define MAX_DIGITS	14

#define MAX_RECURSE_LEVEL	15

#define MIN_FILE_SIZE	0x200000

#define DAYS_KEEP_LOGS 30

#define DAY_IN_SECONDS 86400
#define HOUR_IN_SECONDS 3600
#define MINUTE_IN_SECONDS 60

#define MAX_EVENT_BUFFER_SIZE 0x8000

#define DEFAULT_TIMEOUT 0

// client wait timeout, sec
#define CLIENT_TIMEOUT 30

// const time for remote hashing in sec
#define REMOTE_HASHING_TIMEOUT 300

#endif

#include "svcobj.h"
#include "websrv.h"
#include "tstring.h"
#include "support.h"
#include "newlogger.h"

#include <strsafe.h>
#include <shlwapi.h>
#include <time.h>

#define	MAX_WAIT_WEB 2000 // msec
#define	CLIENT_WAIT 100  // msec
#define	SLEEP_WAIT 100  // msec
#define	MAX_BUFFER_SIZE_WEB 2048  // bytes
#define	MAX_TEMP_BUFFER_SIZE 4096  // bytes
#define	MAX_URL_LENGTH 256  // bytes

extern PNewLogger log;

extern DWORD self_id;

extern PDQueue queues;
extern PDQueue points;

extern char* StopEventName;

extern PServiceObject serviceProcess;

extern char* info_str[];

char* end_of_packet="\r\n\r\n";
char* get_command="GET";

char* http_error[3]={"HTTP/1.1 400 Bad request\r\n\r\n", "HTTP/1.1 404 Not found\r\n\r\n", "HTTP/1.1 405 Method not allowed\r\nAllow:GET\r\n\r\n"};

char* http_body="<!DOCTYPE html> <head><meta charset=""windows-1251""></head><body><style>table {border: 0; border-spacing: 1px;} td {border: 0; padding: 2px;}</style><h1>Server status</h1><hr>";
char* http_body_points="<!DOCTYPE html><head><meta charset=""windows-1251""></head><body><style>table {border: 0; border-spacing: 1px;} td {border: 0; padding: 2px;}</style><h1>Bad points</h1>";

char* http_ok="HTTP/1.1 200 OK\r\nContent-length:";

char* time_str="uptime since : %02d.%02d.%04d %02d:%02d:%02d <br>";

char* wlast_access_str="%02d.%02d.%04d %02d:%02d:%02d";
char* winactive_time_str="%d days %d hours %d minutes";

extern char* aamount[];
extern char* aspeed[];

#pragma pack(push)
#pragma pack(1)

struct URIpath {
	char* URI;
	int code;
};

URIpath web_path[3]={{"/", 1}, {"/status", 1}, {"/bad", 2}};

#pragma pack(pop)

TWebSrv::TWebSrv(int port):TThread(true), TServerSocket(port), terminated(false), stopEvent(NULL)
{
	stopEvent=new TEvent(StopEventName);
}

TWebSrv::~TWebSrv()
{
	if (!threadTerminated()) {
		terminate();

		if (!waitFor(MAX_WAIT_TIME)) kill();
	};


	if (stopEvent!=NULL) delete stopEvent;
}

DWORD TWebSrv::execute()
{
	log->logEvent(info_str[WEB_SERVER_START_STR], INFO);

	while (!terminated&&connected) {
		// we have been signaled
		if (stopEvent->waitFor(0)==WAIT_OBJECT_0) {
			break;
		};

		socket=accept(CLIENT_WAIT);

		if (socket!=NULL) {
			stat(socket);

			delete socket;
		};

		Sleep(SLEEP_WAIT);
	};

	log->logEvent(info_str[WEB_SERVER_STOP_STR], INFO);

	return 0;
}

void TWebSrv::stat(PServerClientSocket socket)
{
	char* buffer=new char[MAX_BUFFER_SIZE_WEB];
	char command[4];
	char path[MAX_URL_LENGTH];
	int pos=0;

	if (socket->read(buffer, MAX_BUFFER_SIZE_WEB)) {
		if (StrStr(buffer, end_of_packet)==NULL) {
			error(0);

			goto out;
		};

		CopyMemory(command, buffer, 3);

		command[3]='\x0';

		if (StrCmpI(command, get_command)!=0) {
			error(2);

			goto out;
		};

		while (pos<MAX_URL_LENGTH) {
			if (buffer[pos+4]==' ') break;

			path[pos]=buffer[pos+4];

			pos++;
		};

		path[pos]='\x0';

		bool found=false;

		for (int i=0; i<sizeof(web_path)/sizeof(URIpath); i++) {
			if (StrCmpI(web_path[i].URI, path)==0) {
				process(web_path[i].code);

				found=true;

				break;
			};
		};

		if (!found) {
			error(1);
		};
	}
	else error(0);

out:
	delete buffer;

	return;
}

void TWebSrv::error(int code)
{
	TString answer(http_error[code]);

	socket->write(answer.get_string(), answer.length());
}

void TWebSrv::process(int code)
{
	switch (code) {
		case 1: {
			status();

			break;
		};

		case 2: {
			bad_points();

			break;
		};

		break;
	};
}

void TWebSrv::status()
{
	TString body(http_body);
	TString answer(http_ok);
	char buffer[MAX_TEMP_BUFFER_SIZE];
	PDQueue queue;
	PBlock block;
	time_t time;
	PPoint point;
	size_t size;
	char* str;
	double value;
	WORD days, hours, minutes;
	tm timeinfo;

	time=serviceProcess->get_uptime();

	localtime_s(&timeinfo, &time);

	StringCbPrintf(buffer, sizeof(buffer), time_str, timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year+1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

	body.add_string(buffer);

	block=queues->head;

	while (block!=NULL) {
		queue=(PDQueue)block->obj;

		body.add_string(queue->get_string());

		body.add_string(" : ");

		StringCbPrintf(buffer, sizeof(buffer), "%d", queue->size());

		body.add_string(buffer);

		body.add_string(" objects<br>");

		block=block->next;
	};

	block=points->head;

	while (block!=NULL) {
		point=(PPoint)block->obj;

		if (point->get_id()!=self_id) {
			body.add_string("<hr><table cols=""2"">");

			body.add_string("<tr><td><b>Name</b></td><td><b>");
			body.add_string(point->get_string());
			body.add_string("</b></td></tr>");

			body.add_string("<tr><td>Id</td><td>");
			StringCbPrintf(buffer, sizeof(buffer), "%d", point->get_id());
			body.add_string(buffer);
			body.add_string("</td></tr>");

			body.add_string("<tr><td>sessions </td><td>");
			StringCbPrintf(buffer, sizeof(buffer), "%d", point->sessions);

			body.add_string(buffer);
			body.add_string("</td></tr>");

			body.add_string("<tr><td>files </td><td>");
			StringCbPrintf(buffer, sizeof(buffer), "%d", point->files);

			body.add_string(buffer);
			body.add_string("</td></tr>");

			body.add_string("<tr><td>readed</td><td>");

			value=(double)point->bytes_readed;
			str=format_value(value, aamount);

			StringCbPrintf(buffer, sizeof(buffer), str, value);

			body.add_string(buffer);
			body.add_string("</td></tr>");

			body.add_string("<tr><td>writed</td><td>");

			value=(double)point->bytes_writed;
			str=format_value(value, aamount);

			StringCbPrintf(buffer, sizeof(buffer), str, value);

			body.add_string(buffer);
			body.add_string("</td></tr>");

			body.add_string("<tr><td>read speed</td><td>");

			if (point->time_read!=0)
				value=point->bytes_readed/point->time_read;
			else 
				value=0;

			str=format_value(value, aspeed);

			StringCbPrintf(buffer, sizeof(buffer), str, value);

			body.add_string(buffer);
			body.add_string("</td></tr>");

			body.add_string("<tr><td>write speed</td><td>");

			if (point->time_write!=0)
				value=point->bytes_writed/point->time_write;
			else 
				value=0;

			str=format_value(value, aspeed);

			StringCbPrintf(buffer, sizeof(buffer), str, value);

			body.add_string(buffer);
			body.add_string("</td></tr>");

			body.add_string("<tr><td>last access</td><td>");

			localtime_s(&timeinfo, &point->last_access);

			if (point->last_access!=0) {
				StringCbPrintf(buffer, sizeof(buffer), wlast_access_str, timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year+1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

				body.add_string(buffer);
			}
			else body.add_string(" never");

			body.add_string("</td></tr>");

			body.add_string("<tr><td>inactive time</td><td>");

			if (point->last_access!=0) {
				calc_time_diff(days, hours, minutes, point->last_access);

				StringCbPrintf(buffer, sizeof(buffer), winactive_time_str, days, hours, minutes);

				body.add_string(buffer);
			}
			else body.add_string("not active");

			body.add_string("</td></tr>");

			body.add_string("</table>");
		};

		block=block->next;
	};

	body.add_string("</body>");

	size=body.length();

	StringCbPrintf(buffer, sizeof(buffer), "%d", size);

	answer.add_string(buffer);
	answer.add_string("\r\n\r\n");

	answer.add_string(body);

	socket->write(answer.get_string(), answer.length());
}

void TWebSrv::bad_points()
{
	TString body(http_body_points);
	TString answer(http_ok);
	char buffer[MAX_TEMP_BUFFER_SIZE];
	PBlock block;
	PPoint point;
	WORD days=0, hours=0, minutes=0;
	size_t size;
	tm timeinfo;

	block=points->head;

	while (block!=NULL) {
		point=(PPoint)block->obj;

		if (point!=NULL) {
			if (point->get_id()!=self_id) {
				if (point->last_access!=0) {
					calc_time_diff(days, hours, minutes, point->last_access);

					if ((days==0)&&(hours==0)&&(minutes<30)) goto next_block;
				};	

				body.add_string("<hr><table cols=""2"">");

				body.add_string("<tr><td><b>Name</b></td><td><b>");
				body.add_string(point->get_string());
				body.add_string("</b></td></tr>");

				body.add_string("<tr><td>Id</td><td>");
				StringCbPrintf(buffer, sizeof(buffer), "%d", point->get_id());
				body.add_string(buffer);
				body.add_string("</td></tr>");

				body.add_string("<tr><td>sessions </td><td>");
				StringCbPrintf(buffer, sizeof(buffer), "%d", point->sessions);

				body.add_string(buffer);
				body.add_string("</td></tr>");

				body.add_string("<tr><td>files </td><td>");
				StringCbPrintf(buffer, sizeof(buffer), "%d", point->files);

				body.add_string(buffer);
				body.add_string("</td></tr>");

				body.add_string("<tr><td>last access</td><td>");

				localtime_s(&timeinfo, &point->last_access);

				if (point->last_access!=0) {
					StringCbPrintf(buffer, sizeof(buffer), wlast_access_str, timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year+1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

					body.add_string(buffer);
				}
				else body.add_string(" never");

				body.add_string("</td></tr>");

				body.add_string("<tr><td>inactive time</td><td>");

				if (point->last_access!=0) {
					StringCbPrintf(buffer, sizeof(buffer), winactive_time_str, days, hours, minutes);

					body.add_string(buffer);
				}
				else body.add_string("not active");

				body.add_string("</td></tr>");

				body.add_string("</table>");
			};
		};

next_block:
		block=block->next;
	};

	body.add_string("</body>");

	size=body.length();

	StringCbPrintf(buffer, sizeof(buffer), "%d", size);

	answer.add_string(buffer);
	answer.add_string("\r\n\r\n");

	answer.add_string(body);

	socket->write(answer.get_string(), answer.length());
}

#include "support.h"
#include "xmlparse.h"
#include "point.h"
#include "dqueue.h"
#include "aobject.h"
#include "job.h"
#include "tmacro.h"
#include "const.h"
#include "errors.h"

#include <strsafe.h>
#include <shlwapi.h>
#include <comutil.h>
#include <shlobj.h>

#import <msxml6.dll>

extern DWORD timeout;
extern int port;
extern int client_port;
extern int web_port;

extern DWORD block_size;

extern DWORD self_id;

extern DWORD sync_id;

extern PDQueue points;
extern PDQueue jobs;
extern PDQueue objects;
extern PDQueue macros;

extern char* config_names[5];

extern char* xml_names[5];

extern char* dir_config;

extern char* system_queue_name;
extern char* system_queue_points;
extern char* system_queue_objects;
extern char* system_queue_jobs;
extern char* system_queue_macros;

extern char* config_main_name;

extern DWORD sync_period;

extern DWORD min_file_size;

extern DWORD days_keep_logs;

#pragma warning (disable: 4482) // use enum in full name
#pragma warning (disable: 4244) // DWORD -> WORD
#pragma warning (disable: 4127) // while (true)

// XML parsing
#define SAFE_RELEASE(p)     do { if ((p)) { (p)->Release(); (p) = NULL; } } while(0)

void parse_service(IXMLDOMNodeList *pChildList)
{
	HRESULT hr;

	IXMLDOMNode *pChildNode;

	long child_counter;
	DWORD value=0;

	_bstr_t child_name;
	_bstr_t child_value;

	_bstr_t node_name;
	_bstr_t node_value;

	pChildList->get_length(&child_counter);

	for (register int i=0; i<child_counter; i++) {
		hr=pChildList->get_item(i, &pChildNode);

		if (SUCCEEDED(hr)) {
			pChildNode->get_nodeName(child_name.GetAddress());
			pChildNode->get_text(child_value.GetAddress());

			if (child_name==_bstr_t("timeout")) {
				value=StrToInt(child_value);

				if (value!=0) {
					timeout=value;
				};

				goto out;
			};

			if (child_name==_bstr_t("port")) {
				value=StrToInt(child_value);

				if (value!=0) {
					port=value;
				};

				goto out;
			};

			if (child_name==_bstr_t("client_port")) {
				value=StrToInt(child_value);

				if (value!=0) {
					client_port=value;
				};

				goto out;
			};

			if (child_name==_bstr_t("web_port")) {
				value=StrToInt(child_value);

				if (value!=0) {
					web_port=value;
				};

				goto out;
			};

			if (child_name==_bstr_t("point")) {
				value=StrToInt(child_value);

				if (value!=0) {
					self_id=value;
				};

				goto out;
			};

			if (child_name==_bstr_t("block_size")) {
				value=StrToInt(child_value);

				if (value!=0) {
					block_size=value*1024;
				};

				goto out;
			};

			if (child_name==_bstr_t("syncronize")) {
				value=StrToInt(child_value);

				if (value!=0) {
					sync_id=value;
				};
			};

			if (child_name==_bstr_t("sync_period")) {
				value=StrToInt(child_value);

				if (value>0) {
					sync_period=value*1000;
				};
			};

			if (child_name==_bstr_t("min_file_size")) {
				value=StrToInt(child_value);

				if (value>=0) {
					min_file_size=value*1024*1024;
				};
			};

			if (child_name==_bstr_t("days_keep_logs")) {
				value=StrToInt(child_value);

				if (value>0) {
					days_keep_logs=value;
				};
			};
		};
out:
		SAFE_RELEASE(pChildNode);
	};
}

void parse_points(IXMLDOMNodeList *pChildList)
{
	HRESULT hr;

	IXMLDOMNode *pChildNode, *pNode;
	IXMLDOMNodeList	*pList;

	long child_counter, counter;
	DWORD value=0;

	_bstr_t child_name;
	_bstr_t child_value;

	_bstr_t node_name;
	_bstr_t node_value;

	PPoint point;

	pChildList->get_length(&child_counter);

	for (register int i=0; i<child_counter; i++) {
		hr=pChildList->get_item(i, &pChildNode);

		if (SUCCEEDED(hr)) {
			pChildNode->get_nodeName(child_name.GetAddress());
			pChildNode->get_text(child_value.GetAddress());

			if (child_name==_bstr_t("point")) {
				hr=pChildNode->get_childNodes(&pList);

				if (SUCCEEDED(hr)) {
					point=new TPoint();

					pList->get_length(&counter);

					for (register int j=0; j<counter; j++) {
						hr=pList->get_item(j, &pNode);

						if (SUCCEEDED(hr)) {
							pNode->get_nodeName(node_name.GetAddress());
							pNode->get_text(node_value.GetAddress());

							if (node_name==_bstr_t("name")) {
								point->set_string(node_value);

								goto out;
							};

							if (node_name==_bstr_t("id")) {
								value=StrToInt(node_value);

								point->set_id(value);

								goto out;
							};

							if (node_name==_bstr_t("ip")) {
								point->set_ip(node_value);

								goto out;
							};

							if (node_name==_bstr_t("altip")) {
								point->set_altip(node_value);

								goto out;
							};
						};
	out:
						SAFE_RELEASE(pNode);
					};

					if (point->get_id()!=0) {
						if (!find_object_by_id(point->get_id(), system_queue_points)) points->addElement(point);
						else delete point;
					}
					else delete point;

					SAFE_RELEASE(pList);
				};
			};
		};

		SAFE_RELEASE(pChildNode);
	};
}

void parse_objects(IXMLDOMNodeList *pChildList)
{
	HRESULT hr;

	IXMLDOMNode *pChildNode, *pNode;
	IXMLDOMNodeList	*pList;

	long child_counter, counter;
	DWORD value=0;

	_bstr_t child_name;
	_bstr_t child_value;

	_bstr_t node_name;
	_bstr_t node_value;

	_bstr_t file_node_name;
	_bstr_t file_node_value;

	PAObject object;

	pChildList->get_length(&child_counter);

	for (register int i=0; i<child_counter; i++) {
		hr=pChildList->get_item(i, &pChildNode);

		if (SUCCEEDED(hr)) {
			pChildNode->get_nodeName(child_name.GetAddress());
			pChildNode->get_text(child_value.GetAddress());

			if (child_name==_bstr_t("object")) {
				hr=pChildNode->get_childNodes(&pList);

				if (SUCCEEDED(hr)) {
					object=new TAObject();

					pList->get_length(&counter);

					for (register int j=0; j<counter; j++) {
						hr=pList->get_item(j, &pNode);

						if (SUCCEEDED(hr)) {
							pNode->get_nodeName(node_name.GetAddress());
							pNode->get_text(node_value.GetAddress());

							if (node_name==_bstr_t("name")) {
								object->set_string(node_value);

								goto out;
							};

							if (node_name==_bstr_t("id")) {
								value=StrToInt(node_value);

								// value 0-99 - system
								if (value>=100) {
									object->set_id(value);
								};

								goto out;
							};

							if (node_name==_bstr_t("overwrite")) {
								if ((node_value==_bstr_t("true"))||(node_value==_bstr_t("True"))) 
									object->overwrite=true;
								
								if ((node_value==_bstr_t("false"))||(node_value==_bstr_t("False"))) 
									object->overwrite=false;

								goto out;
							};

							if (node_name==_bstr_t("recursive")) {
								if ((node_value==_bstr_t("true"))||(node_value==_bstr_t("True")))
									object->recursive=true;

								if ((node_value==_bstr_t("false"))||(node_value==_bstr_t("False")))
									object->recursive=false;

								goto out;
							};

							if (node_name==_bstr_t("client_path")) {
								object->client_path->set_string(node_value);

								goto out;
							};

							if (node_name==_bstr_t("server_path")) {
								object->server_path->set_string(node_value);

								goto out;
							};

							if (node_name==_bstr_t("file")) {
								object->file->set_string(node_value);
							};
						};
	out:
						SAFE_RELEASE(pNode);
					};

					// check items existence and id
					if ((!object->isEmpty())&&(object->get_id()>=100)&&(!object->client_path->isEmpty())&&(!object->server_path->isEmpty())&&(!object->file->isEmpty())) {
						objects->addElement(object);
					}
					else delete object;

					SAFE_RELEASE(pList);
				};
			};
		};

		SAFE_RELEASE(pChildNode);
	};
}

void parse_jobs(IXMLDOMNodeList *pChildList)
{
	HRESULT hr;

	IXMLDOMNode *pChildNode, *pJobNode, *pSourceNode, *pItemNode, *pSubItemNode;
	IXMLDOMNodeList	*pList, *pSourceFiles, *pItemFiles, *pSubItemFiles;

	long child_counter, job_item_counter, source_item_counter, item_list_counter, sub_item_list_counter;
	DWORD value=0;

	_bstr_t child_name;
	_bstr_t child_value;

	_bstr_t node_name;
	_bstr_t node_value;

	_bstr_t item_node_name;
	_bstr_t item_node_value;

	_bstr_t sub_item_node_name;
	_bstr_t sub_item_node_value;

	_bstr_t	child_item_node_name;
	_bstr_t child_item_node_value;

	PPoint point;
	PJob job;
	PAObject object;

	bool scheduled, day_value, month_value, found;

	char *buffer;

	int len;

	WORD hour, min;

	week_names week;
	month_names month;
	month_days days;
	weeks_month weeks;

	PBlock block;

	pChildList->get_length(&child_counter);

	for (register int i=0; i<child_counter; i++) {
		hr=pChildList->get_item(i, &pChildNode);

		if (SUCCEEDED(hr)) {
			pChildNode->get_nodeName(child_name.GetAddress());
			pChildNode->get_text(child_value.GetAddress());

			if (child_name==_bstr_t("job")) {
				hr=pChildNode->get_childNodes(&pList);

				if (SUCCEEDED(hr)) {
					job=new TJob();

					scheduled=false;

					pList->get_length(&job_item_counter);

					for (register int job_items=0; job_items<job_item_counter; job_items++) {
						hr=pList->get_item(job_items, &pJobNode);

						if (SUCCEEDED(hr)) {
							pJobNode->get_nodeName(node_name.GetAddress());
							pJobNode->get_text(node_value.GetAddress());

							if (node_name==_bstr_t("name")) {
								job->set_string(node_value);

								goto out;
							};

							if (node_name==_bstr_t("id")) {
								value=StrToInt(node_value);

								// value 0-99 - system
								if (value>=100) {
									job->set_id(value);
								};

								goto out;
							};

							if (node_name==_bstr_t("active")) {
								if ((node_value==_bstr_t("true"))||(node_value==_bstr_t("True")))
									job->set_active(true);

								if ((node_value==_bstr_t("false"))||(node_value==_bstr_t("False")))
									job->set_active(false);

								goto out;
							};

							if (node_name==_bstr_t("action")) {
								if (node_value==_bstr_t("read")) {
									job->set_action(action::read);
								};

								if (node_value==_bstr_t("write")) {
									job->set_action(action::write);
								};

								goto out;
							};

							if (node_name==_bstr_t("postprocess")) {
								if (node_value==_bstr_t("delete")) {
									job->set_postprocess(postprocess::delete_files);
								};

								if (node_value==_bstr_t("sync")) {
									job->set_postprocess(postprocess::sync_files);
								};

								goto out;
							};

							if (node_name==_bstr_t("transaction")) {
								if ((node_value==_bstr_t("true"))||(node_value==_bstr_t("True")))
									job->set_transaction(true);

								if ((node_value==_bstr_t("false"))||(node_value==_bstr_t("False"))) 
									job->set_transaction(false);

								goto out;
							};

							if (node_name==_bstr_t("objects")) {
								hr=pJobNode->get_childNodes(&pSourceFiles);

								if (SUCCEEDED(hr)) {
									pSourceFiles->get_length(&source_item_counter);

									for (register int source_item=0; source_item<source_item_counter; source_item++) {
										hr=pSourceFiles->get_item(source_item, &pSourceNode);

										if (SUCCEEDED(hr)) {
											pSourceNode->get_nodeName(item_node_name.GetAddress());
											pSourceNode->get_text(item_node_value.GetAddress());

											if (item_node_name==_bstr_t("object")) {
												value=StrToInt(item_node_value);

												object=(PAObject)find_object_by_id(value, system_queue_objects);

												if (object!=NULL) {
													job->get_objects()->addElement(object, false);
												};
											};
										};

										SAFE_RELEASE(pSourceNode);
									};
								};

								SAFE_RELEASE(pSourceFiles);

								goto out;
							};

							if (node_name==_bstr_t("destination")) {
								value=StrToInt(node_value);

								point=(PPoint)find_object_by_id(value, system_queue_points);

								if ((point!=NULL)&&(job->get_destination()==NULL)) {
									job->set_destination(point);
								};

								goto out;
							};

							if (node_name==_bstr_t("source")) {
								hr=pJobNode->get_childNodes(&pSourceFiles);

								if (SUCCEEDED(hr)) {
									pSourceFiles->get_length(&source_item_counter);

									for (register int source_item=0; source_item<source_item_counter; source_item++) {
										hr=pSourceFiles->get_item(source_item, &pSourceNode);

										if (SUCCEEDED(hr)) {
											pSourceNode->get_nodeName(item_node_name.GetAddress());
											pSourceNode->get_text(item_node_value.GetAddress());

											if (item_node_name==_bstr_t("point")) {
												value=StrToInt(item_node_value);

												if (value!=0) {
													point=(PPoint)find_object_by_id(value, system_queue_points);

													if (point!=NULL) {
														job->get_source()->addElement(point, false);
													};
												};
											};
										};

										SAFE_RELEASE(pSourceNode);
									};
								};

								SAFE_RELEASE(pSourceFiles);

								goto out;
							};

							if (node_name==_bstr_t("schedule")) {
								hr=pJobNode->get_childNodes(&pSourceFiles);

								if (SUCCEEDED(hr)) {
									pSourceFiles->get_length(&source_item_counter);

									for (register int schedule_item=0; schedule_item<source_item_counter; schedule_item++) {
										hr=pSourceFiles->get_item(schedule_item, &pSourceNode);

										if (SUCCEEDED(hr)) {
											pSourceNode->get_nodeName(item_node_name.GetAddress());
											pSourceNode->get_text(item_node_value.GetAddress());

											// parse time in 24 hour format
											if (item_node_name==_bstr_t("time")) {
												job->time_from_str(hour, min, item_node_value);

												job->set_time(hour, min);

												goto local_out;
											};

											if ((item_node_name==_bstr_t("daily"))&&(!scheduled)) {
												job->init_daily();

												scheduled=true;

												goto local_out;
											};

											if ((item_node_name==_bstr_t("weekly"))&&(!scheduled)) {
												value=0;

												hr=pSourceNode->get_childNodes(&pItemFiles);

												if (SUCCEEDED(hr)) {
													pItemFiles->get_length(&item_list_counter);

													for (register int item_counter=0; item_counter<item_list_counter; item_counter++) {
														hr=pItemFiles->get_item(item_counter, &pItemNode);

														if (SUCCEEDED(hr)) {
															pItemNode->get_nodeName(sub_item_node_name.GetAddress());
															pItemNode->get_text(sub_item_node_value.GetAddress());

															if (sub_item_node_name==_bstr_t("days")) {
																len=sub_item_node_value.length()+1;

																buffer=new char[len];

																if SUCCEEDED(StringCchCopy(buffer, len, sub_item_node_value)) {
																	job->week_from_str(week, buffer);
																};

																delete buffer;

																goto weekly_out;
															};
														};
weekly_out:
														SAFE_RELEASE(pItemNode);
													};
												};

												SAFE_RELEASE(pItemFiles);

												if ((value!=0)&&(job->has_values(week, 7))) {
													job->init_weekly(week);

													scheduled=true;
												};

												goto local_out;
											};

											if ((item_node_name==_bstr_t("monthly"))&&(!scheduled)) {
												day_value=false;
												month_value=false;

												hr=pSourceNode->get_childNodes(&pItemFiles);

												if (SUCCEEDED(hr)) {
													pItemFiles->get_length(&item_list_counter);

													for (register int item_counter=0; item_counter<item_list_counter; item_counter++) {
														hr=pItemFiles->get_item(item_counter, &pItemNode);

														if (SUCCEEDED(hr)) {
															pItemNode->get_nodeName(sub_item_node_name.GetAddress());
															pItemNode->get_text(sub_item_node_value.GetAddress());

															if (sub_item_node_name==_bstr_t("month")) {
																job->month_from_str(month, sub_item_node_value);

																goto monthly_out;
															};

															if ((sub_item_node_name==_bstr_t("day"))&&(!month_value)) {
																job->days_from_str(days, sub_item_node_value, 31);

																day_value=true;

																goto monthly_out;
															};

															if ((sub_item_node_name==_bstr_t("week"))&&(!day_value)) {
																hr=pItemNode->get_childNodes(&pSubItemFiles);

																if (SUCCEEDED(hr)) {
																	pSubItemFiles->get_length(&sub_item_list_counter);

																	for (register int sub_item_counter=0; sub_item_counter<sub_item_list_counter; sub_item_counter++) {
																		hr=pSubItemFiles->get_item(sub_item_counter, &pSubItemNode);

																		if (SUCCEEDED(hr)) {
																			pSubItemNode->get_nodeName(child_item_node_name.GetAddress());
																			pSubItemNode->get_text(child_item_node_value.GetAddress());

																			if (child_item_node_name==_bstr_t("weeks")) {
																				job->days_from_str(weeks, child_item_node_value, 5);
																			};

																			if (child_item_node_name==_bstr_t("days")) {
																				job->week_from_str(week, child_item_node_value);
																			};
																		};

																		SAFE_RELEASE(pSubItemNode);
																	};

																	month_value=true;

																	SAFE_RELEASE(pSubItemFiles);
																};

																goto monthly_out;
															};
														};
monthly_out:
														SAFE_RELEASE(pItemNode);
													};
												};

												SAFE_RELEASE(pItemFiles);

												if (job->has_values(month, 12)) {
													if (day_value) 
														job->init_month_daily(days, month);

													if (month_value) 
														job->init_month_weekly(week, weeks, month);

													scheduled=true;
												};


												goto local_out;
											};

											if (item_node_name==_bstr_t("period")) {
												value=job->period_from_str(item_node_value);

												if (value!=0) {
													if (value>1440) value=1440; // value in minutes, limit to 1 day

													job->set_period(value);
												};

												goto local_out;
											};

											if (item_node_name==_bstr_t("duration")) {
												value=job->period_from_str(item_node_value);

												if (value!=0) {
													if (value>1440)	value=1440;  // value in minutes, limit to 1 day

													job->set_duration(value);
												};

												goto local_out;
											};
										};
local_out:
										SAFE_RELEASE(pSourceNode);
									};
								};

								SAFE_RELEASE(pSourceFiles);

								goto out;
							};
						};
	out:
						SAFE_RELEASE(pJobNode);
					};

					// check object existence and id
					if ((job->get_id()>=100)&&(!job->get_objects()->isEmpty())&&(!job->get_source()->isEmpty())&&(job->get_destination()!=NULL)&&(!job->get_action()==action::none)) {
						found=false;

						block=job->get_source()->head;

						while (block!=NULL) {
							point=(PPoint)block->obj;
							
							//add job only for current point - once
							if (point!=NULL) {
								if (point->get_id()==self_id) {
									jobs->addElement(job);

									found=true;

									break;
								};
							};

							block=block->next;
						};

						if (!found) delete job;
					}
					else delete job;

					SAFE_RELEASE(pList);
				};
			};
		};

		SAFE_RELEASE(pChildNode);
	};
}

void parse_macros(IXMLDOMNodeList *pChildList)
{
	HRESULT hr;

	IXMLDOMNode *pChildNode;

	long child_counter;

	_bstr_t child_name;
	_bstr_t child_value;

	PMacro macro_def;

	pChildList->get_length(&child_counter);

	for (register int i=0; i<child_counter; i++) {
		hr=pChildList->get_item(i, &pChildNode);

		if (SUCCEEDED(hr)) {
			pChildNode->get_nodeName(child_name.GetAddress());
			pChildNode->get_text(child_value.GetAddress());

			macro_def=new TMacro("%"+child_name+"%", child_value);

			if (macro_def!=NULL) macros->addElement(macro_def, true);
		};

		SAFE_RELEASE(pChildNode);
	};
}

int parse_xml(IXMLDOMDocument *pDoc, char* name)
{
	int result=NO_ERROR;

	HRESULT hr;

	IXMLDOMNodeList	*pList=NULL, *pChildList=NULL;
	IXMLDOMNode *pNode=NULL;

	_bstr_t node_name;

    _variant_t varFileName;

	VARIANT_BOOL varStatus;
	long counter;
	
	char buffer[MAX_PATH_LENGTH+1];

	StringCbCopy(buffer, MAX_PATH_LENGTH, dir_config);

	StringCbCat(buffer, MAX_PATH_LENGTH, name);

	varFileName.SetString(buffer);

	pDoc->load(varFileName.GetVARIANT(), &varStatus);

	if (varStatus == VARIANT_TRUE) {
		hr=pDoc->get_childNodes(&pList);

		if (SUCCEEDED(hr)) {
			pList->get_length(&counter);

			for (register int i=0; i<counter; i++) {
				hr=pList->get_item(i, &pNode);

				if (SUCCEEDED(hr)) {
					pNode->get_nodeName(node_name.GetAddress());

					for (register int j=0; j<(sizeof(xml_names)/sizeof(char*)); j++) {
						if (node_name==_bstr_t(xml_names[j])) {
							hr=pNode->get_childNodes(&pChildList);

							if (SUCCEEDED(hr)) {
								switch (j) {
									// service
									case 0: {
										parse_service(pChildList);

										break;
									};
									// points
									case 1: {
										parse_points(pChildList);

										break;
									};
									// objects
									case 2: {
										parse_objects(pChildList);

										break;
									};
									// jobs
									case 3: {
										parse_jobs(pChildList);

										break;
									};
									// macros
									case 4: {
										parse_macros(pChildList);

										break;
									};
								}
							}
							else result=COM_XML_ERROR_SYSTEM; 

							SAFE_RELEASE(pChildList);
						};
					};
				}
				else result=COM_XML_ERROR_SYSTEM;

				SAFE_RELEASE(pNode);
			};
		} 
		else result=COM_XML_ERROR_SYSTEM; 
		
		SAFE_RELEASE(pList);
	}
	else result=OPEN_XML_ERROR_SYSTEM;

	return result;
}

int init_config(int start_from=0)
{
	int result=NO_ERROR;
	int error;

	HRESULT hr;

	IXMLDOMDocument	*pDoc=NULL;

	hr=CoInitialize(NULL);

	if(SUCCEEDED(hr)) {
		hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&pDoc);

		if (SUCCEEDED(hr)) {
			pDoc->put_async(VARIANT_FALSE);
			pDoc->put_validateOnParse(VARIANT_FALSE);
			pDoc->put_resolveExternals(VARIANT_FALSE);

			for (register int i=start_from; i<(sizeof(config_names)/sizeof(char*)); i++) {
				error=parse_xml(pDoc, config_names[i]);

				if (error!=NO_ERROR) {
					result=error+i;

					break;
				};
			};

			SAFE_RELEASE(pDoc);
		};
	}
	else result=COM_INIT_ERROR;
	
	CoUninitialize();

	return result;
}
// end of XML parsing

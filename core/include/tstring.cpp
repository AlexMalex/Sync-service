#include <windows.h>
#include <strsafe.h>

#include "tstring.h"

TString::TString():string(NULL)
{
}

TString::TString(char* str):string(NULL)
{
	realloc(str);
}

TString::TString(TString& str):string(NULL)
{
	realloc(str.get_string());
}

TString::~TString()
{
	if (string!=NULL) delete string;
}

void TString::realloc(char* str)
{
	size_t size;

	if (string!=NULL) delete string;

	if (str!=NULL) {
		if (SUCCEEDED(StringCchLength(str, STRSAFE_MAX_CCH, &size))) {
			size++;
			
			string=new char[size];

			StringCchCopy(string, size, str);
		};
	}
	else string=NULL;
}

int TString::length()
{
	size_t size=0;

	if (string==NULL) return 0;

	if (SUCCEEDED(StringCchLength(string, STRSAFE_MAX_CCH, &size))) return size;

	return size;
}

void TString::add_string(char* str)
{
	char* new_string;

	size_t size;

	if (str!=NULL) {
		if (SUCCEEDED(StringCchLength(str, STRSAFE_MAX_CCH, &size))) {
			size++;

			size=size+length();

			new_string=new char[size];

			if (new_string!=NULL) {
				if (string!=NULL) {
					StringCchCopy(new_string, size, string);

					StringCchCat(new_string, size, str);
				}
				else StringCchCopy(new_string, size, str);

				if (string!=NULL) delete string;

				string=new_string;
			};
		};
	};
}

void TString::add_string(TString& str)
{
	add_string(str.get_string());
}


void TString::replace_str(char* template_str, char* str)
{
	size_t template_size, str_size, size;
	char* pos=NULL;
	char* new_string;

	if ((template_str==NULL)||(str==NULL)) return;

	pos=StrStr(string, template_str);

	if (pos==NULL) return;

	if (!SUCCEEDED(StringCchLength(template_str, STRSAFE_MAX_CCH, &template_size))) return;

	if (!SUCCEEDED(StringCchLength(str, STRSAFE_MAX_CCH, &str_size))) return;

	size=(str_size-template_size)+length()+1;

	new_string=new char[size];

	if (new_string!=NULL) {
		*pos='\x0';

		//copy head
		StringCchCopy(new_string, size, string);

		//copy new string
		StringCchCat(new_string, size, str);

		//copy rest string
		StringCchCat(new_string, size, pos+template_size);

		if (string!=NULL) delete string;

		string=new_string;
	};
}

#if !defined(TSTRING_H)
#define TSTRING_H

#include "object.h"
#include <shlwapi.h>

class TString: public TObject {
private:
	char* string;

	void realloc(char*);
public:
	TString();
	TString(char*);
	TString(TString&);

	virtual ~TString();

	bool operator==(char*);

	void add_string(char*);
	void add_string(TString&);

	char* get_string();
	void set_string(char*);

	void replace_str(char*, char*);
	bool has_symbol(char);

	int length();

	bool isEmpty();
};

inline bool TString::operator==(char* str)
{
	return (StrCmp(string, str)==0);
}

inline char* TString::get_string()
{
	return string;
}

inline void TString::set_string(char* str)
{
	realloc(str);
}

inline bool TString::isEmpty()
{
	if (length()==0) return true;
	else return false;
}

inline bool TString::has_symbol(char ch)
{
	return !(StrChr(string, ch)==NULL);
}

typedef TString* PString;
typedef TString& RString;

#endif

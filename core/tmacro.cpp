#include "tmacro.h"

TMacro::TMacro(char* str, char* val)
{
	hash=new TString(str);
	value=new TString(val);
}

TMacro::~TMacro()
{
	if (value!=NULL) delete value;
	if (hash!=NULL) delete hash;
}

void TMacro::set_hash(char* str)
{
	if (hash!=NULL)	hash->set_string(str);
}

void TMacro::set_value(char* val)
{
	if (value!=NULL) value->set_string(val);
}

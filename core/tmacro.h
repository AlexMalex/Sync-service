#if !defined(TMACRO_H)
#define TMACRO_H

#include "tstring.h"
#include "object.h"

class TMacro: public TObject {
private:
	PString hash;
	PString value;

public:
	TMacro(char*, char*);
	virtual ~TMacro();

	char* get_hash();
	char* get_value();

	void set_hash(char*);
	void set_value(char*);
};

inline char* TMacro::get_hash()
{
	return hash->get_string();
}

inline char* TMacro::get_value()
{
	return value->get_string();
}

typedef TMacro* PMacro;
typedef TMacro& RMacro;

#endif

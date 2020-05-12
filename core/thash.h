#if !defined(THASH_H)
#define THASH_H

#include <windows.h>

#define MD5LEN  16

class THash {
private:
	HCRYPTPROV provider;
	HCRYPTHASH hash;
public:
	THash();
	virtual ~THash();

	bool hash_data(char*, DWORD);

	bool isValid();

	bool get_hash(char*);
};

typedef THash* PHash;
typedef THash& RHash;

#endif

#include "thash.h"

THash::THash():provider(NULL)
{
	if (CryptAcquireContext(&provider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
		CryptCreateHash(provider, CALG_MD5, 0, 0, &hash);
	};
}

THash::~THash()
{
	if (hash!=NULL) CryptDestroyHash(hash);

	if (provider!=NULL) CryptReleaseContext(provider, 0);
}

bool THash::hash_data(char* buffer, DWORD size)
{
	if (!isValid()) return false;

	return (CryptHashData(hash, (BYTE*)buffer, size, 0)==TRUE);
}

bool THash::isValid()
{
	return ((provider!=NULL)&&(hash!=NULL));
}

bool THash::get_hash(char* buffer)
{
	DWORD size=MD5LEN;

	if (!isValid()) return false;

	return (CryptGetHashParam(hash, HP_HASHVAL, (BYTE*)buffer, &size, 0)==TRUE);
}
